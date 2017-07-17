#ifndef _UAPI_IMX_DRM_H_
#define _UAPI_IMX_DRM_H_

#include "drm.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define DRM_IMX_DPU_BLIT                        0x00
#define DRM_IMX_DPU_WAIT                        0x01

#define DRM_IOCTL_IMX_DPU_BLIT          DRM_IOWR(DRM_COMMAND_BASE + DRM_IMX_DPU_BLIT, struct drm_imx_dpu_blit)
#define DRM_IOCTL_IMX_DPU_WAIT          DRM_IOWR(DRM_COMMAND_BASE + DRM_IMX_DPU_WAIT, struct drm_imx_dpu_wait)

struct fetch_unit {
	uint32_t in_pipeline;
	uint32_t control;
	uint32_t tiling;
	uint32_t burst_buf;
	uint32_t buf_address;
	uint32_t buf_attributes;
	uint32_t buf_dimension;
	uint32_t color_bits;
	uint32_t color_shift;
	uint32_t layer_offset;
	uint32_t clip_offset;
	uint32_t clip_dimension;
	uint32_t const_color;
	uint32_t layer_property;
	uint32_t frame_dimension;
	uint32_t frame_resample;
};
struct store_unit {
	uint32_t in_pipeline;
	uint32_t control;
	uint32_t burst_buf;
	uint32_t buf_address;
	uint32_t buf_attributes;
	uint32_t buf_dimension;
	uint32_t frame_offset;
	uint32_t color_bits;
	uint32_t color_shift;
};
struct rop_unit {
	uint32_t in_pipeline;
	uint32_t control;
};
struct matrix_unit {
	uint32_t in_pipeline;
	uint32_t control;
	uint32_t red0;
	uint32_t red1;
	uint32_t green0;
	uint32_t green1;
	uint32_t blue0;
	uint32_t blue1;
	uint32_t alpha0;
	uint32_t alpha1;
};
struct hscaler_unit {
	uint32_t in_pipeline;
	uint32_t control;
	uint32_t setup1;
	uint32_t setup2;
};
struct vscaler_unit {
	uint32_t in_pipeline;
	uint32_t control;
	uint32_t setup1;
	uint32_t setup2;
	uint32_t setup3;
	uint32_t setup4;
	uint32_t setup5;
};
struct blitblend_unit {
	uint32_t in_pipeline;
	uint32_t control;
	uint32_t const_color;
	uint32_t red_func;
	uint32_t green_func;
	uint32_t blue_func;
	uint32_t alpha_func;
	uint32_t blend_mode1;
	uint32_t blend_mode2;
};
struct engcfg_unit {
	uint32_t fetchpersp9_dynamic;
	uint32_t fetchdecode9_dynamic;
	uint32_t rop9_dynamic;
	uint32_t matrix9_dynamic;
	uint32_t hscaler9_dynamic;
	uint32_t vscaler9_dynamic;
	uint32_t blitblend9_dynamic;
	uint32_t store9_dynamic;
};

/**
 * struct drm_imx_dpu_blit - ioctl argument for waiting for
 * DRM_IMX_DPU_BLIT.
 */
struct drm_imx_dpu_blit {
	struct fetch_unit fetch_decode;
	struct fetch_unit fetch_persp;
	struct fetch_unit fetch_eco;
	struct store_unit store;
	struct rop_unit rop;
	struct matrix_unit matrix;
	struct hscaler_unit hscaler;
	struct vscaler_unit vscaler;
	struct blitblend_unit blitblend;
	struct engcfg_unit engcfg;

	int imxdpuv1_id;
};

/**
 * struct drm_imx_dpu_wait - ioctl argument for waiting for
 * DRM_IMX_DPU_WAIT.
 *
 */
struct drm_imx_dpu_wait {
	int imxdpuv1_id;
};

#if defined(__cplusplus)
}
#endif

#endif /* _UAPI_IMX_DRM_H_ */

