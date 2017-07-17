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
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;
	uint32_t *cmdlist;
	uint32_t cmdnum = 100;
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


	cmdlist = devm_kzalloc(drm_dev->dev,
		sizeof(uint32_t)*cmdnum, GFP_KERNEL);
	if (!cmdlist)
		return -ENOMEM;
	
	cmdlist[0] = 0x14000001;
	cmdlist[1] = 0x1054;
	cmdlist[2] = 0x0;
	cmdlist[3] = 0x14000001;
	cmdlist[4] = 0x100c;
	cmdlist[5] = 0x80001010;
	cmdlist[6] = 0x1400000c;
	cmdlist[7] = 0x101c;
	cmdlist[8] = 0x0;
	cmdlist[9] = 0x0;
	cmdlist[10] = 0x0;
	cmdlist[11] = 0x0;
	cmdlist[12] = 0x0;
	cmdlist[13] = 0x0;
	cmdlist[14] = 0x0;
	cmdlist[15] = 0x437077f;
	cmdlist[16] = 0xffff;
	cmdlist[17] = 0x40000110;
	cmdlist[18] = 0x437077f;
	cmdlist[19] = 0x104000;
	cmdlist[20] = 0x14000001;
	cmdlist[21] = 0x4030;
	cmdlist[22] = 0x0;
	cmdlist[23] = 0x14000001;
	cmdlist[24] = 0x400c;
	cmdlist[25] = 0x400;
	cmdlist[26] = 0x14000006;
	cmdlist[27] = 0x4018;
	cmdlist[28] = 0xe0300000;
	cmdlist[29] = 0x10000eff;
	cmdlist[30] = 0x437077f;
	cmdlist[31] = 0x0;
	cmdlist[32] = 0x5060500;
	cmdlist[33] = 0xb050000;
	cmdlist[34] = 0x14000001;
	cmdlist[35] = 0x848;
	cmdlist[36] = 0x0;
	cmdlist[37] = 0x14000001;
	cmdlist[38] = 0x828;
	cmdlist[39] = 0x0;
	cmdlist[40] = 0x14000001;
	cmdlist[41] = 0x868;
	cmdlist[42] = 0x0;
	cmdlist[43] = 0x14000001;
	cmdlist[44] = 0x8a8;
	cmdlist[45] = 0x0;
	cmdlist[46] = 0x14000001;
	cmdlist[47] = 0x8c8;
	cmdlist[48] = 0x0;
	cmdlist[49] = 0x14000001;
	cmdlist[50] = 0x8e8;
	cmdlist[51] = 0x0;
	cmdlist[52] = 0x14000001;
	cmdlist[53] = 0x928;
	cmdlist[54] = 0x0;
	cmdlist[55] = 0x14000001;
	cmdlist[56] = 0x94c;
	cmdlist[57] = 0x1;
	cmdlist[58] = 0x14000001;
	cmdlist[59] = 0x954;
	cmdlist[60] = 0x1;
	cmdlist[61] = 0x14000001;
	cmdlist[62] = 0x403c;
	cmdlist[63] = 0x1;
	cmdlist[64] = 0x20000100;
	cmdlist[65] = 0x14000001;
	cmdlist[66] = 0x60;
	cmdlist[67] = 0x1;
	cmdnum = 68;

	dpu_be = dpu_be_get(dpu);

	dpu_be_blit(dpu_be, cmdlist, cmdnum);

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
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;
	int id = 0;

	if (!file) {
		return -ENODEV;
	}

	file_priv = file->driver_priv;

	if (!file_priv)
		return -ENODEV;

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

	dpu_be = dpu_be_get(dpu);

	ret = dpu_be_wait(dpu_be);

	dpu_be_put(dpu_be);

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

