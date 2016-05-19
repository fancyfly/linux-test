/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 */
/*
All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  o Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  o Redistributions in binary form must reproduce the above copyright notice,
    thislist of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  o Neither the name of Freescale Semiconductor, Inc. nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

struct be_blit_cfg {
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
};

struct cmdSeq_unit{
    uint32_t in_use;
    uint32_t hif;
    uint32_t lock_unlock_hif;
    uint32_t lock_status_hif;
    uint32_t lock_unlock;
    uint32_t lock_status;
    uint32_t buffer_address;
    uint32_t buffer_size;
    uint32_t watermark_control;
    uint32_t control;
    uint32_t status;
    uint32_t reserved;
};

/* PRIVATE DATA */
struct imxdpu_info {
	/*reg */
	void __iomem *base;
};

#define IMXDPU_IOC_MAGIC       'i'
#define IMXDPU_IOC_BLIT       _IOW(IMXDPU_IOC_MAGIC,1,struct be_blit_cfg)
#define IMXDPU_IOC_WAIT       _IO(IMXDPU_IOC_MAGIC,2)

void imxdpu_be_irq_handler(int8_t imxdpu_id, int8_t irq);
int imxdpu_be_init(int8_t imxdpu_id, void __iomem * imxdpu_base);
int imxdpu_be_blit(struct imxdpu_info *imxdpu, struct be_blit_cfg *cfg);
int imxdpu_be_wait_shadow_load(struct imxdpu_info *imxdpu);
int imxdpu_be_wait_complete(struct imxdpu_info *imxdpu);
int imxdpu_be_load(struct imxdpu_info *imxdpu, void __user * p);
int imxdpu_be_wait(struct imxdpu_info *imxdpu);

int imxdpu_cs_alloc_command_buffer(struct imxdpu_info *imxdpu);
int imxdpu_cs_static_setup(struct imxdpu_info *imxdpu, struct cmdSeq_unit *cmdSeq);
int imxdpu_cs_wait_fifo_space(struct imxdpu_info *imxdpu);
int imxdpu_cs_wait_idle(struct imxdpu_info *imxdpu);

#endif
