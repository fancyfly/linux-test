#include <drm/drmP.h>


struct imx_drm_dpu_private {
        struct device *dev;
};

struct drm_imx_file_private {
        struct imx_drm_dpu_private *dpu_priv[2];
};

struct imx_drm_subdrv {
	struct list_head list;
	struct device *dev;
	struct drm_device *drm_dev;

	int (*probe)(struct drm_device *drm_dev, struct device *dev);
	void (*remove)(struct drm_device *drm_dev, struct device *dev);
	int (*open)(struct drm_device *drm_dev, struct device *dev,
			struct drm_file *file);
	void (*close)(struct drm_device *drm_dev, struct device *dev,
			struct drm_file *file);
};

int imx_drm_subdrv_register(struct imx_drm_subdrv *subdrv);
int imx_drm_subdrv_unregister(struct imx_drm_subdrv *subdrv);

int imx_drm_device_subdrv_probe(struct drm_device *dev);
int imx_drm_device_subdrv_remove(struct drm_device *dev);

int imx_drm_subdrv_open(struct drm_device *dev, struct drm_file *file);
void imx_drm_subdrv_close(struct drm_device *dev, struct drm_file *file);
