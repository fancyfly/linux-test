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

#define pr_fmt(fmt) "xen-scmufront: " fmt

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <xen/xenbus.h>
#include <xen/events.h>
#include <xen/grant_table.h>
#include <soc/imx8/soc.h>
#include "xen-scmufront.h"

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>
#include "sc/svc/pm/rpc.h"
#include "sc/svc/irq/rpc.h"
#include "sc/svc/misc/rpc.h"

#define XEN_SCMUFRONT_TIMEOUT_MS 10000

/* only one */
struct xen_scmufront *g_xen_scmufront;

struct xen_scmu_shared {
	u32 have_a2b;
	sc_rpc_msg_t a2b;
	u32 have_b2a;
	sc_rpc_msg_t b2a;
};

struct xen_scmufront {
	struct xenbus_device *dev;
	struct xen_scmu_shared *shared;

	int evtchn;
	grant_ref_t gref;
};

void xen_scmufront_write(struct xen_scmufront *priv, sc_rpc_msg_t *msg)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(XEN_SCMUFRONT_TIMEOUT_MS);

	while (priv->shared->have_a2b && time_before(jiffies, timeout))
		usleep_range(10000, 20000);
	if (priv->shared->have_a2b) {
		dev_err(&priv->dev->dev, "write timeout\n");
		return;
	}

	memcpy(&priv->shared->a2b, msg, msg->size * sizeof(u32));
	virt_wmb();
	priv->shared->have_a2b = 1;

	notify_remote_via_evtchn(priv->evtchn);
}

void xen_scmufront_read(struct xen_scmufront *priv, sc_rpc_msg_t *msg)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(XEN_SCMUFRONT_TIMEOUT_MS);
	unsigned long sleep_us = 1000;
#ifdef DEBUG
	uint8_t func = RPC_FUNC(msg);
#endif
	u8 size;

	if ((!priv->shared->have_b2a))
		udelay(100);
	/* Should use a channel instead!! */
	while ((!priv->shared->have_b2a) && time_before(jiffies, timeout)) {
		usleep_range(sleep_us, sleep_us * 2);
		sleep_us *= 2;
	}
	if (!priv->shared->have_b2a) {
		dev_err(&priv->dev->dev, "read reply timeout\n");
		/* Maybe create a generic error response? */
		return;
	}

	size = priv->shared->b2a.size;
	if (size < 1)
		size = 1;
	if (size > SC_RPC_MAX_MSG)
		size = SC_RPC_MAX_MSG;
	memcpy(msg, &priv->shared->b2a, size * sizeof(u32));
	priv->shared->have_b2a = 0;

#ifdef DEBUG
	/* This indicates guest misconfiguration so be verbose: */
	if (RPC_R8(msg) == SC_ERR_NOACCESS) {
		if (RPC_SVC(msg) == SC_RPC_SVC_MISC && func == MISC_FUNC_GET_CONTROL) {
			pr_warn("got SC_ERR_NOACCESS on sc_misc_get_control\n");
			dump_stack();
		} else if (RPC_SVC(msg) == SC_RPC_SVC_PM && func == PM_FUNC_GET_RESOURCE_POWER_MODE) {
			pr_warn("got SC_ERR_NOACCESS on sc_pm_get_resource_power_mode\n");
		} else if (RPC_SVC(msg) == SC_RPC_SVC_PM && func == PM_FUNC_GET_CLOCK_RATE) {
			pr_warn("got SC_ERR_NOACCESS on sc_pm_get_clock_rate resource=%d\n",
					(int)RPC_U16(msg, 0));
		} else if (RPC_SVC(msg) == SC_RPC_SVC_PM && func == PM_FUNC_SET_CLOCK_RATE) {
			pr_warn("got SC_ERR_NOACCESS on sc_pm_set_clock_rate resource=%d\n",
					(int)RPC_U16(msg, 4));
		} else if (RPC_SVC(msg) == SC_RPC_SVC_PM && func == PM_FUNC_CLOCK_ENABLE) {
			pr_warn("got SC_ERR_NOACCESS on sc_pm_clock_enable resource=%d\n",
					(int)RPC_U16(msg, 0));
			dump_stack();
		} else {
			pr_warn("got SC_ERR_NOACCESS svc=%d func=%hhu\n",
					(int)RPC_SVC(msg), func);
			dump_stack();
		}
	}
#endif
	/* notify? */
}

static int xen_scmufront_probe(struct xenbus_device *dev,
			    const struct xenbus_device_id *id)
{
	int err;
	struct xenbus_transaction xbt;
	struct xen_scmufront *priv;

	pr_debug("%s %p %d\n", __func__, dev, dev->otherend_id);

	if (g_xen_scmufront)
		return -EBUSY;

	priv = kzalloc(sizeof(struct xen_scmufront), GFP_KERNEL | __GFP_ZERO);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->shared = (struct xen_scmu_shared *)get_zeroed_page(GFP_NOIO | __GFP_HIGH);
	if (!priv->shared) {
		err = -ENOMEM;
		goto fail_alloc;
	}

	err = xenbus_grant_ring(dev, priv->shared, 1, &priv->gref);
	if (err < 0)
		goto fail_alloc;

	err = xenbus_alloc_evtchn(priv->dev, &priv->evtchn);
	if (err < 0)
		goto fail_evtchn;

again:
	err = xenbus_transaction_start(&xbt);
	if (err) {
		xenbus_dev_fatal(dev, err, "starting transaction");
		goto fail_transaction;
	}

	err = xenbus_printf(xbt, dev->nodename, "event-channel", "%u", priv->evtchn);
	if (err) {
		dev_err(&dev->dev, "failed write event-channel: %d\n", err);
		xenbus_transaction_end(xbt, 1);
		goto fail_transaction;
	}

	err = xenbus_printf(xbt, dev->nodename, "grant-ref", "%u", priv->gref);
	if (err) {
		dev_err(&dev->dev, "failed write grant-ref: %d\n", err);
		xenbus_transaction_end(xbt, 1);
		goto fail_transaction;
	}

	err = xenbus_transaction_end(xbt, 0);
	if (err == -EAGAIN)
		goto again;
	if (err) {
		xenbus_dev_fatal(dev, err, "completing transaction");
		goto fail_transaction;
	}

	dev_set_drvdata(&dev->dev, priv);
	g_xen_scmufront = priv;

	xenbus_switch_state(dev, XenbusStateInitialised);
	return 0;

fail_transaction:
	xenbus_free_evtchn(priv->dev, priv->evtchn);
fail_evtchn:
	gnttab_end_foreign_access_ref(priv->gref, 0);
fail_alloc:
	if (priv->shared)
		free_page((unsigned long)priv->shared);
	kfree(priv);
	return err;
}

static void xen_scmufront_backend_changed(struct xenbus_device *dev,
					 enum xenbus_state frontend_state)
{
	dev_dbg(&dev->dev, "backend changed state=%d\n", frontend_state);

	switch (frontend_state) {
	case XenbusStateInitialised:
	case XenbusStateConnected:
		xenbus_switch_state(dev, XenbusStateConnected);
		break;

	default:
		break;
	}
}

static const struct xenbus_device_id xen_scmufront_ids[] = {
	{ "imxscmu" },
	{ "" }
};

static struct xenbus_driver xen_scmufront_driver = {
	.ids  = xen_scmufront_ids,
	.probe = xen_scmufront_probe,
	.otherend_changed = xen_scmufront_backend_changed,
};

static int __init xen_scmufront_init(void)
{
	return xenbus_register_frontend(&xen_scmufront_driver);
}
/* HACK: */
fs_initcall(xen_scmufront_init);

MODULE_LICENSE("GPL");
MODULE_ALIAS("xen:imxscmu");
