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
#include "dpu-blit.h"

#include "imx-drm.h"

static DEFINE_MUTEX(dpu_bliteng_lock);
static LIST_HEAD(dpu_bliteng_list);

static int imx_dpu_num;

struct dpu_bliteng *dpu_bliteng_find_by_of_id(int32_t id)
{
	struct dpu_bliteng *dpu_be;

	mutex_lock(&dpu_bliteng_lock);

	list_for_each_entry(dpu_be, &dpu_bliteng_list, list)
		if (id == dpu_be->id) {
			mutex_unlock(&dpu_bliteng_lock);
			return dpu_be;
	}

	mutex_unlock(&dpu_bliteng_lock);

	return NULL;
}

int imx_drm_dpu_blit_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file)

{
	struct drm_imx_dpu_blit *blit;
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;
	int id = 0;

	blit = data;
	id = blit->imxdpu_id;
	if (!(id == 0 || id == 1)) {
		return -EINVAL;
	}

	dpu_be = dpu_bliteng_find_by_of_id(id);
	if (!dpu_be) {
		printk(KERN_DEBUG "Failed to get struct dpu_bliteng\n");
		return -ENODEV;
	}

	dpu = dev_get_drvdata(dpu_be->dev->parent);
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
	struct drm_imx_dpu_wait *wait;
	struct dpu_soc *dpu;
	struct dpu_bliteng *dpu_be;
	int id = 0;

	wait = data;
	id = wait->imxdpu_id;
	if (!(id == 0 || id == 1)) {
		return -EINVAL;
	}

	dpu_be = dpu_bliteng_find_by_of_id(id);
	if (!dpu_be)
		return -ENODEV;
	
	dpu = dev_get_drvdata(dpu_be->dev->parent);
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

	list_del(&dpu_bliteng->list);

	devm_kfree(dev, dpu_bliteng);

	dev_info(dev, "Successfully removed dpu-blit engine\n");

	return 0;
}

int dpu_bliteng_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dpu_bliteng *dpu_bliteng;
	int ret;

	if (!dev->platform_data)
		return -EINVAL;

	dpu_bliteng = devm_kzalloc(dev, sizeof(*dpu_bliteng), GFP_KERNEL);
	if (!dpu_bliteng)
		return -ENOMEM;
	INIT_LIST_HEAD(&dpu_bliteng->list);
	dpu_bliteng->id = imx_dpu_num;

	dpu_bliteng->dev = dev;

	ret = dpu_bliteng_init(dpu_bliteng);
	if (ret)
		return ret;

	mutex_lock(&dpu_bliteng_lock);
	list_add_tail(&dpu_bliteng->list, &dpu_bliteng_list);
	mutex_unlock(&dpu_bliteng_lock);

	dev_set_drvdata(dev, dpu_bliteng);

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

