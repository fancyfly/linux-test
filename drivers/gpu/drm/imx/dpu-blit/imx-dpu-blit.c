#include <drm/drmP.h>
#include <drm/imx_drm.h>
#include <uapi/drm/imx_drm.h>
#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <video/dpu.h>

#include "imx_drm_subdrv.h"
#include "dpu-kms.h"
#include "dpu-plane.h"
#include "dpu-blit.h"

#include "imx-drm.h"

static int imx_dpu_num;

int dpu_bliteng_open(struct drm_device *drm_dev, struct device *dev,
		     struct drm_file *file)
{
	struct drm_imx_file_private *file_priv = file->driver_priv;
	struct imx_drm_dpu_private *dpu_priv;
	int i;

	dpu_priv = kzalloc(sizeof(*dpu_priv), GFP_KERNEL);
	if (!dpu_priv)
		return -ENOMEM;

	dpu_priv->dev = dev;

	for (i = 0; i < 2; i++) {
		if (!file_priv->dpu_priv[i])
			break;
	}

	file_priv->dpu_priv[i] = dpu_priv;

	dev_info(dev, "dpu_bliteng_open()\n");

	return 0;
}

void dpu_bliteng_close(struct drm_device *drm_dev, struct device *dev,
		       struct drm_file *file)
{
	struct drm_imx_file_private *file_priv = file->driver_priv;
	int i;

	for (i = 0; i < 2; i++) {
		if (file_priv->dpu_priv[i]) {
			kfree(file_priv->dpu_priv[i]);
			file_priv->dpu_priv[i] = NULL;
		}
	}

	dev_info(dev, "dpu_bliteng_close()\n");
}

int imx_drm_dpu_blit_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file)

{
	struct device *dev;
	struct drm_imx_file_private *file_priv;
	struct imx_drm_dpu_private *dpu_priv;
	struct drm_imx_dpu_blit *blit;
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;
	int id = 0;

	if (!file) {
		printk(KERN_DEBUG "Failed to get struct drm_file\n");
		return -ENODEV;
	}

	file_priv = file->driver_priv;
	
	if (!file_priv) {
		printk(KERN_DEBUG "Failed to get struct drm_imx_file_private\n");
		return -ENODEV;
	}

	blit = data;
	id = blit->imxdpu_id;
	if (!(id == 0 || id == 1)) {
		return -EINVAL;
	}

	dpu_priv = file_priv->dpu_priv[id];

	if (!dpu_priv) {
		printk(KERN_DEBUG "Failed to get imx_drm_dpu_private\n");
		return -ENODEV;
	}

	printk(KERN_DEBUG "OK1\n");

	dev = dpu_priv->dev;
	if (!dev) {
		printk(KERN_DEBUG "Failed to get struct device\n");
		return -ENODEV;
	}

	printk(KERN_DEBUG "OK2\n");

	dpu_be = dev_get_drvdata(dev);
	if (!dpu_be) {
		printk(KERN_DEBUG "Failed to get struct dpu_bliteng\n");
		return -ENODEV;
	}

	printk(KERN_DEBUG "OK2\n");

	dpu = dev_get_drvdata(dev->parent);
	if (!dpu) {
		printk(KERN_DEBUG "Failed to get struct device\n");
		return -ENODEV;
	}

retry:
	dpu_be = dpu_be_get(dpu);
	if (dpu_be == ERR_PTR(-EBUSY)) {
		goto retry;
	}

	dpu_be_blit_cfg(dpu_be, blit);
	/*dpu_be_blit(dpu_be, cmdlist, cmdnum);*/

	dpu_be_put(dpu_be);

	return 0;
}

EXPORT_SYMBOL_GPL(imx_drm_dpu_blit_ioctl);

int imx_drm_dpu_wait_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file)

{
	int ret;
	struct device *dev;
	struct drm_imx_file_private *file_priv;
	struct imx_drm_dpu_private *dpu_priv;
	struct drm_imx_dpu_wait *wait;
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;
	int id = 0;

	if (!file) {
		return -ENODEV;
	}

	file_priv = file->driver_priv;

	if (!file_priv)
		return -ENODEV;

	wait = data;
	id = wait->imxdpu_id;
	if (!(id == 0 || id == 1)) {
		return -EINVAL;
	}

	dpu_priv = file_priv->dpu_priv[id];

	if (!dpu_priv) {
		return -ENODEV;
	}

	dev = dpu_priv->dev;
        if (!dev)
                return -ENODEV;
	
	dpu_be = dev_get_drvdata(dev);
	if (!dpu_be)
		return -ENODEV;
	
	dpu = dev_get_drvdata(dev->parent);
	if (!dpu)
		return -ENODEV;

retry:
	dpu_be = dpu_be_get(dpu);
	if (dpu_be == ERR_PTR(-EBUSY)) {
		goto retry;
	}

	ret = dpu_be_wait(dpu_be);

	dpu_be_put(dpu_be);

	return ret;
}

int imx_drm_dpu_get_param_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file)
{
	int ret;
	enum drm_imx_dpu_param *param = data;

	switch (*param) {
	case (DRM_IMX_MAX_DPUS):
		ret = imx_dpu_num;
		break;
	default:
		ret = -EINVAL;
		DRM_ERROR("Unknown param![%d]\n", *param);
		break;
	}

	return ret;
}

int dpu_bliteng_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dpu_bliteng *dpu_bliteng = dev_get_drvdata(dev);
	int ret;

	/* un-register from imx-drv-subdrv */
	ret = imx_drm_subdrv_unregister(&dpu_bliteng->subdrv);
	if (ret) {
		dev_err(dev, "failed to un-register dpu-blit engine\n");
	}

	devm_kfree(dev, dpu_bliteng);

	dev_info(dev, "Successfully removed dpu-blit engine\n");

	return 0;
}

int dpu_bliteng_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dpu_bliteng *dpu_bliteng;
	struct imx_drm_subdrv *subdrv;
	int ret;

	if (!dev->platform_data)
		return -EINVAL;

	dpu_bliteng = devm_kzalloc(dev, sizeof(*dpu_bliteng), GFP_KERNEL);
	if (!dpu_bliteng)
		return -ENOMEM;

	dpu_bliteng->dev = dev;

	ret = dpu_bliteng_init(dpu_bliteng);
	if (ret)
		return ret;

	dev_set_drvdata(dev, dpu_bliteng);

	/* register dpu blit device as imx_drm subdrv */
	subdrv = &dpu_bliteng->subdrv;
	subdrv->dev = dev;
	subdrv->probe = NULL;
	subdrv->remove = NULL;
	subdrv->open = dpu_bliteng_open;
	subdrv->close = dpu_bliteng_close;

	ret = imx_drm_subdrv_register(subdrv);
	if (ret < 0) {
		dev_err(dev, "failed to register dpu-blit engine\n");
	}

	imx_dpu_num++;
	dev_info(dev, "Successfully probed dpu-blit engine\n");

	return 0;
}

struct platform_driver dpu_bliteng_driver = {
	.driver = {
		.name = "imx-dpu-bliteng",
	},
	.probe = dpu_bliteng_probe,
	.remove = dpu_bliteng_remove,
};

EXPORT_SYMBOL_GPL(imx_drm_dpu_wait_ioctl);

module_platform_driver(dpu_bliteng_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Semiconductor");
MODULE_DESCRIPTION("i.MX DRM DPU BLITENG");

