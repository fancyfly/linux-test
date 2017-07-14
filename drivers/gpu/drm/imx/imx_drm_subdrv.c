#include <drm/drmP.h>
#include "imx_drm_subdrv.h"


static LIST_HEAD(imx_drm_subdrv_list);

int imx_drm_subdrv_register(struct imx_drm_subdrv *subdrv)
{
	if (!subdrv)
		return -EINVAL;

	list_add_tail(&subdrv->list, &imx_drm_subdrv_list);

	return 0;
}
EXPORT_SYMBOL(imx_drm_subdrv_register);

int imx_drm_subdrv_unregister(struct imx_drm_subdrv *subdrv)
{
	if (!subdrv)
		return -EINVAL;

	list_del(&subdrv->list);

	return 0;
}
EXPORT_SYMBOL(imx_drm_subdrv_unregister);

int imx_drm_device_subdrv_probe(struct drm_device *dev)
{
	struct imx_drm_subdrv *subdrv, *n;
	int err;

	if (!dev)
		return -EINVAL;

	list_for_each_entry_safe(subdrv, n, &imx_drm_subdrv_list, list) {
		if (subdrv->probe) {
			subdrv->drm_dev = dev;

			/*
			 * this probe callback would be called by sub driver
			 * after setting of all resources to this sub driver,
			 * such as clock, irq and register map are done.
			 */
			err = subdrv->probe(dev, subdrv->dev);
			if (err) {
				DRM_DEBUG("exynos drm subdrv probe failed.\n");
				list_del(&subdrv->list);
				continue;
			}
		}
	}

	return 0;
}

int imx_drm_device_subdrv_remove(struct drm_device *dev)
{
	struct imx_drm_subdrv *subdrv;

	if (!dev) {
		WARN(1, "Unexpected drm device unregister!\n");
		return -EINVAL;
	}

	list_for_each_entry(subdrv, &imx_drm_subdrv_list, list) {
		if (subdrv->remove)
			subdrv->remove(dev, subdrv->dev);
	}

	return 0;
}

int imx_drm_subdrv_open(struct drm_device *dev, struct drm_file *file)
{
	struct imx_drm_subdrv *subdrv;
	int ret;

	list_for_each_entry(subdrv, &imx_drm_subdrv_list, list) {
		if (subdrv->open) {
			ret = subdrv->open(dev, subdrv->dev, file);
			if (ret)
				goto err;
		}
	}

	return 0;

err:
	list_for_each_entry_continue_reverse(subdrv, &imx_drm_subdrv_list, list) {
		if (subdrv->close)
			subdrv->close(dev, subdrv->dev, file);
	}
	return ret;
}

EXPORT_SYMBOL(imx_drm_subdrv_open);

void imx_drm_subdrv_close(struct drm_device *dev, struct drm_file *file)
{
	struct imx_drm_subdrv *subdrv;

	list_for_each_entry(subdrv, &imx_drm_subdrv_list, list) {
		if (subdrv->close)
			subdrv->close(dev, subdrv->dev, file);
	}
}
