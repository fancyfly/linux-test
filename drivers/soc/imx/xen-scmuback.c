/*
 * Copyright 2017 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "xen-scmuback: " fmt

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <xen/events.h>
#include <xen/xenbus.h>
#include <soc/imx8/soc.h>

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>
#include "sc/main/rpc.h"
#include "sc/svc/pm/rpc.h"
#include "sc/svc/rm/rpc.h"
#include "sc/svc/irq/rpc.h"
#include "sc/svc/misc/rpc.h"

struct xen_scmu_shared {
	u32 have_a2b;
	sc_rpc_msg_t a2b;
	u32 have_b2a;
	sc_rpc_msg_t b2a;
};

struct xen_scmuback {
	struct xenbus_device *dev;
	struct xen_scmu_shared *shared;
	DECLARE_BITMAP(allowed_resources, SC_R_LAST);
	struct work_struct work;
	struct xenbus_watch watch;
	grant_ref_t gref;
	int evtchn;
	int irq;
};

/*
 * Determines which resource a message is about.
 *
 * Returns positive rsrc_t or -errno when not applicable or for unsupported calls.
 */
static int sc_msg_get_resource_target(sc_rpc_msg_t *msg)
{
	/*
	 * It would have been nice if this was a standardized field,
	 * but switch statements are required instead.
	 */
	switch (RPC_SVC(msg))
	{
	case SC_RPC_SVC_PM:
		switch (RPC_FUNC(msg))
		{
		case PM_FUNC_SET_RESOURCE_POWER_MODE:
		case PM_FUNC_GET_RESOURCE_POWER_MODE:
		case PM_FUNC_GET_CLOCK_RATE:
		case PM_FUNC_SET_CLOCK_PARENT:
		case PM_FUNC_GET_CLOCK_PARENT:
		case PM_FUNC_CLOCK_ENABLE:
			return RPC_U16(msg, 0);

		case PM_FUNC_SET_CLOCK_RATE:
			return RPC_U16(msg, 4);
		}
		return -ENOENT;

	case SC_RPC_SVC_MISC:
		switch (RPC_FUNC(msg))
		{
		case MISC_FUNC_SET_CONTROL:
			return RPC_U16(msg, 8);
		case MISC_FUNC_GET_CONTROL:
			return RPC_U16(msg, 4);
		}
		return -ENOENT;

	case SC_RPC_SVC_IRQ:
		switch (RPC_FUNC(msg))
		{
		case IRQ_FUNC_ENABLE:
			return RPC_U16(msg, 4);
		}
		return -ENOENT;
	}

	return -ENOENT;
}

static void xen_scmuback_handle(struct xen_scmuback *priv, sc_rpc_msg_t *msg)
{
	int resource = -ENOSYS;

	if (RPC_VER(msg) != SC_RPC_VERSION) {
		dev_dbg(&priv->dev->dev, "SC_ERR_VERSION got %d instead of %d\n",
				RPC_VER(msg), SC_RPC_VERSION);
		RPC_R8(msg) = SC_ERR_VERSION;
		return;
	}

	/* Special handling for rm_is_resource_owned */
	if (RPC_SVC(msg) == SC_RPC_SVC_RM && RPC_FUNC(msg) == RM_FUNC_IS_RESOURCE_OWNED) {
		resource = RPC_U16(msg, 0);
		if (resource < 0 || resource >= SC_R_LAST)
			goto err_noaccess;
		/* Return value is not a sc_err_t */
		RPC_R8(msg) = !!test_bit(resource, priv->allowed_resources);
		return;
	}

	/* Determine resource and check access */
	resource = sc_msg_get_resource_target(msg);
	if (resource < 0 || resource >= SC_R_LAST)
		goto err_noaccess;
	if (!test_bit(resource, priv->allowed_resources))
		goto err_noaccess;

	sc_call_rpc(0, msg, false);
	return;

err_noaccess:
	dev_dbg(&priv->dev->dev, "sent SC_ERR_NOACCESS svc=%d func=%d resource=%d\n",
			(int)RPC_SVC(msg), (int)RPC_FUNC(msg), resource);
	RPC_R8(msg) = SC_ERR_NOACCESS;
	return;
}

static void xen_scmuback_reply(struct xen_scmuback *priv, sc_rpc_msg_t *msg)
{
	/* If there is already a reply overwrite it, do not block.
	 * It is frontend's job to read responses before writing a new request.
	 */
	memcpy(&priv->shared->b2a, msg, msg->size * sizeof(u32));
	virt_wmb();
	priv->shared->have_b2a = 1;
}

static void xen_scmuback_read(struct xen_scmuback *priv)
{
	sc_rpc_msg_t msg;

	u8 size;

	if (!priv->shared->have_a2b)
		return;

	size = priv->shared->a2b.size;
	if (size < 1)
		size = 1;
	if (size > SC_RPC_MAX_MSG)
		size = SC_RPC_MAX_MSG;
	memcpy(&msg, &priv->shared->a2b, size * sizeof(u32));
	priv->shared->have_a2b = 0;

	xen_scmuback_handle(priv, &msg);
	xen_scmuback_reply(priv, &msg);
}

static void xen_scmuback_workfunc(struct work_struct *work)
{
	struct xen_scmuback *priv = container_of(work, struct xen_scmuback, work);

	xen_scmuback_read(priv);
}

irqreturn_t xen_scmuback_int(int irq, void *_priv)
{
	struct xen_scmuback *priv = _priv;

	schedule_work(&priv->work);

	return IRQ_HANDLED;
}

static int xen_scmuback_connect(struct xen_scmuback *priv)
{
	int err = 0;

	err = xenbus_scanf(XBT_NIL, priv->dev->otherend, "event-channel", "%u",
			  &priv->evtchn);
	if (err != 1)
		goto fail;
	err = xenbus_scanf(XBT_NIL, priv->dev->otherend, "grant-ref", "%u",
			  &priv->gref);
	if (err != 1)
		goto fail;

	err = xenbus_map_ring_valloc(priv->dev, &priv->gref, 1, (void**)&(priv->shared));
	if (err)
		goto fail;

	err = bind_interdomain_evtchn_to_irqhandler(
			priv->dev->otherend_id, priv->evtchn,
			xen_scmuback_int, 0,
			"imxscmu", priv);
	if (err < 0) {
		xenbus_unmap_ring_vfree(priv->dev, priv->shared);
		goto fail;
	}
	priv->irq = err;

	xenbus_switch_state(priv->dev, XenbusStateConnected);
	return 0;

fail:
	dev_err(&priv->dev->dev, "failed connect: %d\n", err);
	return err;
}

static int xen_scmuback_disconnect(struct xen_scmuback *priv)
{
	dev_dbg(&priv->dev->dev, "disconnect\n");

	if (priv->shared) {
		xenbus_unmap_ring_vfree(priv->dev, priv->shared);
		priv->shared = NULL;
	}
	if (priv->irq) {
		unbind_from_irqhandler(priv->irq, priv);
		priv->irq = 0;
	}
	xenbus_switch_state(priv->dev, XenbusStateClosed);

	return 0;
}

static void xen_scmuback_watch(struct xenbus_watch *watch,
			       const char **vec, unsigned int len)
{
	struct xen_scmuback *priv = container_of(watch, struct xen_scmuback, watch);
	int err;
	unsigned int buflen;
	char *buf;

	buf = xenbus_read(XBT_NIL, priv->dev->nodename, "allowed-resources", &buflen);
	if (IS_ERR(buf)) {
		dev_warn(&priv->dev->dev, "failed xenbus_read allowed-resources: %ld\n", PTR_ERR(buf));
		return;
	}
	pr_err("buf=%s\n", buf);
	err = bitmap_parselist(buf, priv->allowed_resources, SC_R_LAST);
	if (err != 0)
		dev_warn(&priv->dev->dev, "failed bitmap_parselist allowed-resources: %d\n", err);
	kfree(buf);

	dev_info(&priv->dev->dev, "allowed-resources: %*pbl\n",
			SC_R_LAST, priv->allowed_resources);
}

static int xen_scmuback_probe(struct xenbus_device *dev,
			      const struct xenbus_device_id *id)
{
	int err;
	struct xen_scmuback *priv;

	pr_debug("%s %p %d\n", __func__, dev, dev->otherend_id);

	priv = kzalloc(sizeof(struct xen_scmuback), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->watch.node = dev->nodename;
	priv->watch.callback = xen_scmuback_watch;
	err = register_xenbus_watch(&priv->watch);
	if (err)
		goto fail_watch;

	INIT_WORK(&priv->work, xen_scmuback_workfunc);
	priv->dev = dev;
	dev_set_drvdata(&dev->dev, priv);
	return 0;

fail_watch:
	kfree(priv);
	return err;
}

static int xen_scmuback_remove(struct xenbus_device *dev)
{
	struct xen_scmuback *priv = dev_get_drvdata(&dev->dev);

	dev_dbg(&dev->dev, "remove\n");
	xen_scmuback_disconnect(priv);

	return 0;
}

static void xen_scmuback_frontend_changed(struct xenbus_device *dev,
					enum xenbus_state frontend_state)
{
	struct xen_scmuback *priv = dev_get_drvdata(&dev->dev);

	dev_dbg(&dev->dev, "frontend changed state=%d\n", frontend_state);

	switch (frontend_state) {
	case XenbusStateInitialised:
	case XenbusStateConnected:
		if (dev->state == XenbusStateConnected)
			break;

		xen_scmuback_connect(priv);
		break;

	case XenbusStateClosed:
	case XenbusStateUnknown:
		xen_scmuback_disconnect(priv);
		break;

	default:
		break;
	}
}

static const struct xenbus_device_id xen_scmuback_ids[] = {
	{ "imxscmu" },
	{ "" }
};

static struct xenbus_driver xen_scmuback_driver = {
	.ids  = xen_scmuback_ids,
	.probe = xen_scmuback_probe,
	.remove = xen_scmuback_remove,
	.otherend_changed = xen_scmuback_frontend_changed,
};

static int __init xen_scmuback_init(void)
{
	return xenbus_register_backend(&xen_scmuback_driver);
}
module_init(xen_scmuback_init);

static void __exit xen_scmuback_exit(void)
{
	xenbus_unregister_driver(&xen_scmuback_driver);
}
module_exit(xen_scmuback_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("xen-backend:imxscmu");
