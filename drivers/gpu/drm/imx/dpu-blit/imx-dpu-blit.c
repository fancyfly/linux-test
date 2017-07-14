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

#include "dpu-kms.h"
#include "dpu-plane.h"

#include "imx-drm.h"
#include "imx_drm_subdrv.h"

int imx_drm_dpu_blit_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file)

{
	int ret;
	struct device *dev;
	struct drm_imx_file_private *file_priv = file->driver_priv;
	struct imx_drm_dpu_private *dpu_priv = file_priv->dpu_priv;
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;

	dev = dpu_priv->dev;
	if (!dev)
		return -ENODEV;

	dpu_be = dev_get_drvdata(dev);
	if (!dpu_be)
		return -ENODEV;

	dpu = dev_get_drvdata(dev->parent);
	if (!dpu)
		return -ENODEV;

	uint32_t *cmdlist;
	uint32_t cmdnum = 100;

	cmdlist = devm_kzalloc(drm_dev->dev,
		sizeof(uint32_t)*cmdnum, GFP_KERNEL);
	if(!cmdlist)
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
	struct drm_imx_file_private *file_priv = file->driver_priv;
        struct imx_drm_dpu_private *dpu_priv = file_priv->dpu_priv;
	struct dpu_soc *dpu;	
	struct dpu_bliteng *dpu_be;

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

EXPORT_SYMBOL_GPL(imx_drm_dpu_wait_ioctl);
