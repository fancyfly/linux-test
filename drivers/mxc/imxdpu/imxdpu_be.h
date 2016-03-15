#ifndef IMXDPU_BE_H
#define IMXDPU_BE_H

struct fetch_unit {
	uint32_t in_pipeline;
	uint32_t control;
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

struct be_blit_cfg {
	struct fetch_unit fetch_decode;
	struct fetch_unit fetch_persp;
	struct store_unit store;
	struct rop_unit rop;
	struct matrix_unit matrix;
	struct hscaler_unit hscaler;
	struct vscaler_unit vscaler;
	struct blitblend_unit blitblend;
	struct engcfg_unit engcfg;
};

/* PRIVATE DATA */
struct imxdpu_info {
	/*reg */
	void __iomem *base;
};

#define IMXDPU_IOC_MAGIC       'i'
#define IMXDPU_IOC_BLIT       _IOW(IMXDPU_IOC_MAGIC,1,struct be_blit_cfg)
#define IMXDPU_IOC_WAIT       _IO(IMXDPU_IOC_MAGIC,2)

irqreturn_t imxdpu_isr(int irq, void *data);
int imxdpu_be_init(void);
int imxdpu_be_blit(struct imxdpu_info *imxdpu, struct be_blit_cfg *cfg);
int imxdpu_be_wait_shadow_load(struct imxdpu_info *imxdpu);
int imxdpu_be_wait_complete(struct imxdpu_info *imxdpu);
int imxdpu_be_load(struct imxdpu_info *imxdpu, void __user * p);
int imxdpu_be_wait(struct imxdpu_info *imxdpu);
#endif
