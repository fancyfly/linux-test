/*
 * Copyright 2005-2015 Freescale Semiconductor, Inc.
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
/* Instance: imxdpu_prinvate.h */
#ifndef IMXDPU_PRIVATE_H
#define IMXDPU_PRIVATE_H

#include <linux/io.h>
#include <linux/types.h>
#include <linux/imxdpu.h>

typedef enum {
	IMXDPU_BURST_UNKNOWN = 0,
	IMXDPU_BURST_LEFT_RIGHT_DOWN,
	IMXDPU_BURST_HORIZONTAL,
	IMXDPU_BURST_VERTICAL,
	IMXDPU_BURST_FREE,
} imxdpu_burst_t;

#define IMXDPU_INTERRUPT_MAX  (66+1)	/* IMXDPU_FRAMECAP5_SYNC_OFF_IRQ
					   (66) is last interrupt */
#define INTSTAT0_BIT(__bit__) (1U<<(__bit__))
#define INTSTAT1_BIT(__bit__) (1U<<((__bit__)-32))
#define INTSTAT2_BIT(__bit__) (1U<<((__bit__)-64))

struct imxdpu_irq_node {
	int(*handler) (int, void *);
	const char *name;
	void *data;
	uint32_t  flags;
};

/* Generic definitions that are common to many registers */
#define IMXDPU_COLOR_BITSALPHA0_MASK 0xFU
#define IMXDPU_COLOR_BITSALPHA0_SHIFT 0U
#define IMXDPU_COLOR_BITSBLUE0_MASK 0xF00U
#define IMXDPU_COLOR_BITSBLUE0_SHIFT 8U
#define IMXDPU_COLOR_BITSGREEN0_MASK 0xF0000U
#define IMXDPU_COLOR_BITSGREEN0_SHIFT 16U
#define IMXDPU_COLOR_BITSRED0_MASK 0xF000000U
#define IMXDPU_COLOR_BITSRED0_SHIFT 24U

#define IMXDPU_COLOR_SHIFTALPHA0_MASK 0x1FU
#define IMXDPU_COLOR_SHIFTALPHA0_SHIFT 0U
#define IMXDPU_COLOR_SHIFTBLUE0_MASK 0x1F00U
#define IMXDPU_COLOR_SHIFTBLUE0_SHIFT 8U
#define IMXDPU_COLOR_SHIFTGREEN0_MASK 0x1F0000U
#define IMXDPU_COLOR_SHIFTGREEN0_SHIFT 16U
#define IMXDPU_COLOR_SHIFTRED0_MASK 0x1F000000U
#define IMXDPU_COLOR_SHIFTRED0_SHIFT 24U

#define IMXDPU_COLOR_CONSTALPHA_MASK 0xFFU
#define IMXDPU_COLOR_CONSTALPHA_SHIFT 0U
#define IMXDPU_COLOR_CONSTBLUE_MASK 0xFF00U
#define IMXDPU_COLOR_CONSTBLUE_SHIFT 8U
#define IMXDPU_COLOR_CONSTGREEN_MASK 0xFF0000U
#define IMXDPU_COLOR_CONSTGREEN_SHIFT 16U
#define IMXDPU_COLOR_CONSTRED_MASK  0xFF000000U
#define IMXDPU_COLOR_CONSTRED_SHIFT 24U

#define IMXDPU_BUFF_ATTR_STRIDE_MASK 0xFFFFU
#define IMXDPU_BUFF_ATTR_STRIDE_SHIFT 0U
#define IMXDPU_BUFF_ATTR_BITSPERPIXEL_MASK 0x3F0000U
#define IMXDPU_BUFF_ATTR_BITSPERPIXEL_SHIFT 16U

#define IMXDPU_BUFF_DIMEN_LINECOUNT_SHIFT 16U
#define IMXDPU_BUFF_DIMEN_LINEWIDTH_MASK 0x3FFFU
#define IMXDPU_BUFF_DIMEN_LINEWIDTH_SHIFT 0U
#define IMXDPU_BUFF_DIMEN_LINECOUNT_MASK 0x3FFF0000U

#define IMXDPU_LAYER_XOFFSET_MASK 0x7FFFU
#define IMXDPU_LAYER_XOFFSET_SHIFT 0U
#define IMXDPU_LAYER_XSBIT_MASK 0x4000U
#define IMXDPU_LAYER_XSBIT_SHIFT 0U

#define IMXDPU_LAYER_YOFFSET_MASK 0x7FFF0000U
#define IMXDPU_LAYER_YOFFSET_SHIFT 16U
#define IMXDPU_LAYER_YSBIT_MASK 0x4000U
#define IMXDPU_LAYER_YSBIT_SHIFT 16U

#define IMXDPU_CLIP_XOFFSET_MASK 0x7FFFU
#define IMXDPU_CLIP_XOFFSET_SHIFT 0U
#define IMXDPU_CLIP_YOFFSET_MASK 0x7FFF0000U
#define IMXDPU_CLIP_YOFFSET_SHIFT 16U

#define IMXDPU_CLIP_WIDTH_MASK 0x3FFFU
#define IMXDPU_CLIP_WIDTH_SHIFT 0U
#define IMXDPU_CLIP_HEIGHT_MASK 0x3FFF0000U
#define IMXDPU_CLIP_HEIGHT_SHIFT 16U

#define IMXDPU_FRAMEWIDTH_MASK 0x3FFFU
#define IMXDPU_FRAMEWIDTH_SHIFT 0U
#define IMXDPU_FRAMEHEIGHT_MASK 0x3FFF0000U
#define IMXDPU_FRAMEHEIGHT_SHIFT 16U
#define IMXDPU_EMPTYFRAME_MASK 0x80000000U
#define IMXDPU_EMPTYFRAME_SHIFT 31U

#define IMXDPU_PIXENGCFG_SRC_SEL__DISABLE 0U
#define IMXDPU_PIXENGCFG_SRC_SEL_MASK 0x3FU
#define IMXDPU_PIXENGCFG_SRC_SEL_SHIFT 0U

#define IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL_MASK 0x3FU
#define IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL_SHIFT 0U
#define IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE 0U

#define IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL_MASK 0x3F00U
#define IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL_SHIFT 8U
#define IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE 0U

#define IMXDPU_PIXENGCFG_CLKEN_MASK 0x3000000U
#define IMXDPU_PIXENGCFG_CLKEN_SHIFT 24U
/* Field Value: _CLKEN__DISABLE, Clock for block is disabled  */
#define IMXDPU_PIXENGCFG_CLKEN__DISABLE 0U
/* Field Value: _CLKEN__AUTOMATIC, Clock is enabled if unit is used,
 * frequency is defined by the register setting for this pipeline (see
 * [endpoint_name]_Static register)  */
#define IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC 0x1U
/* Field Value: _CLKEN__FULL, Clock for block is without gating  */
#define IMXDPU_PIXENGCFG_CLKEN__FULL 0x3U

/* Register: IMXDPU_LayerProperty0 Common Bits */
#define IMXDPU_LAYERPROPERTY_OFFSET          ((uint32_t)(0x40))
#define IMXDPU_LAYERPROPERTY_RESET_VALUE     0x80000100U
#define IMXDPU_LAYERPROPERTY_RESET_MASK      0xFFFFFFFFU
#define IMXDPU_LAYERPROPERTY_PALETTEENABLE_MASK 0x1U
#define IMXDPU_LAYERPROPERTY_PALETTEENABLE_SHIFT 0U
#define IMXDPU_LAYERPROPERTY_TILEMODE_MASK  0x30U
#define IMXDPU_LAYERPROPERTY_TILEMODE_SHIFT 4U
/* Field Value: TILEMODE0__TILE_FILL_ZERO, Use zero value  */
#define IMXDPU_LAYERPROPERTY_TILEMODE__TILE_FILL_ZERO 0U
/* Field Value: TILEMODE0__TILE_FILL_CONSTANT, Use constant color register
 * value  */
#define IMXDPU_LAYERPROPERTY_TILEMODE__TILE_FILL_CONSTANT 0x1U
/* Field Value: TILEMODE0__TILE_PAD, Use closest pixel from source buffer.
 * Must not be used for DECODE or YUV422 operations or when SourceBufferEnable
 * is 0.  */
#define IMXDPU_LAYERPROPERTY_TILEMODE__TILE_PAD 0x2U
/* Field Value: TILEMODE0__TILE_PAD_ZERO, Use closest pixel from source buffer
 * but zero for alpha component. Must not be used for DECODE or YUV422
 * operations or when SourceBufferEnable is 0.  */
#define IMXDPU_LAYERPROPERTY_TILEMODE__TILE_PAD_ZERO 0x3U
#define IMXDPU_LAYERPROPERTY_ALPHASRCENABLE_MASK 0x100U
#define IMXDPU_LAYERPROPERTY_ALPHASRCENABLE_SHIFT 8U
#define IMXDPU_LAYERPROPERTY_ALPHACONSTENABLE_MASK 0x200U
#define IMXDPU_LAYERPROPERTY_ALPHACONSTENABLE_SHIFT 9U
#define IMXDPU_LAYERPROPERTY_ALPHAMASKENABLE_MASK 0x400U
#define IMXDPU_LAYERPROPERTY_ALPHAMASKENABLE_SHIFT 10U
#define IMXDPU_LAYERPROPERTY_ALPHATRANSENABLE_MASK 0x800U
#define IMXDPU_LAYERPROPERTY_ALPHATRANSENABLE_SHIFT 11U
#define IMXDPU_LAYERPROPERTY_RGBALPHASRCENABLE_MASK 0x1000U
#define IMXDPU_LAYERPROPERTY_RGBALPHASRCENABLE_SHIFT 12U
#define IMXDPU_LAYERPROPERTY_RGBALPHACONSTENABLE_MASK 0x2000U
#define IMXDPU_LAYERPROPERTY_RGBALPHACONSTENABLE_SHIFT 13U
#define IMXDPU_LAYERPROPERTY_RGBALPHAMASKENABLE_MASK 0x4000U
#define IMXDPU_LAYERPROPERTY_RGBALPHAMASKENABLE_SHIFT 14U
#define IMXDPU_LAYERPROPERTY_RGBALPHATRANSENABLE_MASK 0x8000U
#define IMXDPU_LAYERPROPERTY_RGBALPHATRANSENABLE_SHIFT 15U
#define IMXDPU_LAYERPROPERTY_PREMULCONSTRGB_MASK 0x10000U
#define IMXDPU_LAYERPROPERTY_PREMULCONSTRGB_SHIFT 16U
#define IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE_MASK 0x60000U
#define IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE_SHIFT 17U
/* Field Value: YUVCONVERSIONMODE0__OFF, No conversion.  */
#define IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__OFF 0U
/* Field Value: YUVCONVERSIONMODE0__ITU601, Conversion from YCbCr (YUV) to
 * RGB according to ITU recommendation BT.601-6 (standard definition TV).
 * Input range is 16..235 for Y and 16..240 for U/V.  */
#define IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601 0x1U
/* Field Value: YUVCONVERSIONMODE0__ITU601_FR, Conversion from YCbCr (YUV)
 * to RGB according to ITU recommendation BT.601-6, but assuming full range
 * YUV inputs (0..255). Most typically used for computer graphics (e.g.
 * for JPEG encoding).  */
#define IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601_FR 0x2U
/* Field Value: YUVCONVERSIONMODE0__ITU709, Conversion from YCbCr (YUV) to
 * RGB according to ITU recommendation BT.709-5 part 2 (high definition
 * TV). Input range is 16..235 for Y and 16..240 for U/V.  */
#define IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__ITU709 0x3U
#define IMXDPU_LAYERPROPERTY_GAMMAREMOVEENABLE_MASK 0x100000U
#define IMXDPU_LAYERPROPERTY_GAMMAREMOVEENABLE_SHIFT 20U
#define IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE_MASK 0x40000000U
#define IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE_SHIFT 30U
#define IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE_MASK 0x80000000U
#define IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE_SHIFT 31U

typedef struct {
	/* Source buffer base address of layer 0. */
	uint32_t baseaddress0;
	/* Source buffer attributes for layer 0. */
	uint32_t sourcebufferattributes0;
	/* Source buffer dimension of layer 0. */
	uint32_t sourcebufferdimension0;
	/* Size of color components for RGB, YUV and index formats (layer 0). */
	uint32_t colorcomponentbits0;
	/* Bit position of color components for RGB, YUV and index
	   formats (layer 0). */
	uint32_t colorcomponentshift0;
	/* Position of layer 0 within the destination frame. */
	uint32_t layeroffset0;
	/* Clip window position for layer 0. */
	uint32_t clipwindowoffset0;
	/* Clip window size for layer 0. */
	uint32_t clipwindowdimensions0;
	/* Constant color for layer 0. */
	uint32_t constantcolor0;
	/* Common properties of layer 0. */
	uint32_t layerproperty0;
} fetch_layer_setup_t;

typedef enum {
	//IMXDPU_SHDLD_FETCHDECODE9 =  1U << 1,
	//IMXDPU_SHDLD_FETCHPERSP9  =  1U << 2,
	//IMXDPU_SHDLD_FETCHECO9    =  1U << 3,
	IMXDPU_SHDLD_CONSTFRAME0  =  1U << 4,
	IMXDPU_SHDLD_CONSTFRAME4  =  1U << 5,
	IMXDPU_SHDLD_CONSTFRAME1  =  1U << 6,
	IMXDPU_SHDLD_CONSTFRAME5  =  1U << 7,
	IMXDPU_SHDLD_EXTSRC4      =  1U << 8,
	IMXDPU_SHDLD_EXTSRC5      =  1U << 9,
	IMXDPU_SHDLD_FETCHDECODE2 =  1U <<10,
	IMXDPU_SHDLD_FETCHDECODE3 =  1U <<11,
	IMXDPU_SHDLD_FETCHWARP2   =  1U <<12,
	IMXDPU_SHDLD_FETCHECO2    =  1U <<13,
	IMXDPU_SHDLD_FETCHDECODE0 =  1U <<14,
	IMXDPU_SHDLD_FETCHECO0    =  1U <<15,
	IMXDPU_SHDLD_FETCHDECODE1 =  1U <<16,
	IMXDPU_SHDLD_FETCHECO1    =  1U <<17,
	IMXDPU_SHDLD_FETCHLAYER0  =  1U <<18,
	IMXDPU_SHDLD_FETCHLAYER1  =  1U <<19,
	IMXDPU_SHDLD_EXTSRC0      =  1U <<20,
	IMXDPU_SHDLD_EXTSRC1      =  1U <<21,
} imxdpu_shadow_load_req_id_t;

typedef enum {
	IMXDPU_SHDLD_IDX_DISP0   =  (0),
	IMXDPU_SHDLD_IDX_DISP1   =  (1),
	IMXDPU_SHDLD_IDX_CONST0  =  (2), /* IMXDPU_ID_CONSTFRAME0 */
	IMXDPU_SHDLD_IDX_CONST1  =  (3), /* IMXDPU_ID_CONSTFRAME1 */
	IMXDPU_SHDLD_IDX_CHAN_00 =  (4), /* IMXDPU_ID_FETCHDECODE2 */
	IMXDPU_SHDLD_IDX_CHAN_01 =  (5), /* IMXDPU_ID_FETCHDECODE0 */
	IMXDPU_SHDLD_IDX_CHAN_02 =  (6), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_03 =  (7), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_04 =  (8), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_05 =  (9), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_06 = (10), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_07 = (11), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_08 = (12), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_09 = (13), /* IMXDPU_ID_FETCHLAYER0 */
	IMXDPU_SHDLD_IDX_CHAN_10 = (14), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_11 = (15), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_12 = (16), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_13 = (17), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_14 = (18), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_15 = (19), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_16 = (20), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_17 = (21), /* IMXDPU_ID_FETCHWARP2 */
	IMXDPU_SHDLD_IDX_CHAN_18 = (22), /* IMXDPU_ID_FETCHDECODE3 */
	IMXDPU_SHDLD_IDX_CHAN_19 = (23), /* IMXDPU_ID_FETCHDECODE1 */
	IMXDPU_SHDLD_IDX_CHAN_20 = (24), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_21 = (25), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_22 = (26), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_23 = (27), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_24 = (28), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_25 = (29), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_26 = (30), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_27 = (31), /* IMXDPU_ID_FETCHLAYER1*/
	IMXDPU_SHDLD_IDX_CHAN_28 = (32), /* IMXDPU_ID_FETCHECO0*/
	IMXDPU_SHDLD_IDX_CHAN_29 = (33), /* IMXDPU_ID_FETCHECO1*/
	IMXDPU_SHDLD_IDX_CHAN_30 = (34), /* IMXDPU_ID_FETCHECO2*/
	IMXDPU_SHDLD_IDX_MAX     = (35),
} imxdpu_shadow_load_index_t;

typedef struct {
	bool prim_sync_state;
	bool sec_sync_state;
	uint32_t prim_sync_count;
	uint32_t sec_sync_count;
	uint32_t skew_error_count;
	uint32_t prim_fifo_empty_count;
	uint32_t sec_fifo_empty_count;
	uint32_t frame_count;
} frame_gen_stats_t;

/*!
 * Definition of IMXDPU channel structure
 */
typedef struct {
	int8_t disp_id;	/* Iris instance id of "owner" */

	imxdpu_chan_t chan;
	uint32_t src_pixel_fmt;
	int16_t src_top;
	int16_t src_left;
	uint16_t src_width;
	uint16_t src_height;
	int16_t clip_top;
	int16_t clip_left;
	uint16_t clip_width;
	uint16_t clip_height;
	uint16_t stride;
	uint32_t dest_pixel_fmt;
	int16_t dest_top;
	int16_t dest_left;
	uint16_t dest_width;
	uint16_t dest_height;
	uint16_t const_color;

	uint32_t h_scale_factor;	/* downscaling  out/in */
	uint32_t h_phase;
	uint32_t v_scale_factor;	/* downscaling  out/in */
	uint32_t v_phase[2][2];

	bool use_video_proc;
	bool interlaced;
	bool use_eco_fetch;
	bool use_global_alpha;
	bool use_local_alpha;

	/* note: dma_addr_t changes for 64-bit arch */
	dma_addr_t phyaddr_0;

	uint32_t u_offset;
	uint32_t v_offset;

	uint8_t blend_layer;
	uint8_t destination_stream;

	//uint8_t         store1_id; /* same as chan_id */
	//uint8_t         fetch1_id; /* same as chan_id */
	//uint8_t         fetch2_id; /* for semi-planar format only for video and warp */
	//uint8_t         fetch3_id; /* for fully planar format requires
	//				video + integral or warp + integral */

	imxdpu_rotate_mode_t rot_mode;

	/* todo add features sub-windows, upscaling, warping */
	fetch_layer_setup_t fetch_layer_prop;

	//imxdpu_burst_t burst;
	bool in_use;

	/* todo: add channel features */
} chan_private_t;

typedef union {
	struct {
		uint8_t request;
		uint8_t processing;
		uint8_t complete;
		uint8_t trys;
	} state;
	uint32_t word;
} imxdpu_shadow_state_t;

/* PRIVATE DATA */
struct imxdpu_soc {
	//int8_t id;
	int8_t devtype;
	int8_t online;
	uint32_t enabled_int[3];
	struct imxdpu_irq_node irq_list[IMXDPU_INTERRUPT_MAX];

	struct device *dev;
	struct imxdpu_videomode video_mode[IMXDPU_NUM_DI];
	frame_gen_stats_t fgen_stats[IMXDPU_NUM_DI];
	uint32_t irq_count;
	/*
	 * Bypass reset to avoid display channel being
	 * stopped by probe since it may starts to work
	 * in bootloader.
	 */
	int8_t bypass_reset;

	/* todo: need to decide where the locking is implemented */
	//spinlock_t lock;
	//struct mutex channel_lock;

	/*clk*/

	/*irq*/

	/*reg*/
	void __iomem           *base;

	/*use count*/
	imxdpu_layer_t blend_layer[IMXDPU_LAYER_MAX];
	chan_private_t chan_data[IMXDPU_CHAN_IDX_MAX];
	uint8_t	shadow_load_pending[IMXDPU_NUM_DI][IMXDPU_SHDLD_IDX_MAX];
	imxdpu_shadow_state_t shadow_load_state[IMXDPU_NUM_DI][IMXDPU_SHDLD_IDX_MAX];
};

/* PRIVATE FUNCTIONS */
#ifdef ENABLE_IMXDPU_TRACE_REG
uint32_t _imxdpu_read(struct imxdpu_soc *dpu, u32 offset, char *file, int line);
#define imxdpu_read(_inst_, _offset_) _imxdpu_read(_inst_, _offset_, __FILE__, __LINE__)
#else
static inline uint32_t imxdpu_read(struct imxdpu_soc *dpu, uint32_t offset)
{
	return __raw_readl(dpu->base + offset);
}
#endif

#ifdef ENABLE_IMXDPU_TRACE_IRQ_READ
uint32_t _imxdpu_read_irq(struct imxdpu_soc *dpu, u32 offset, char *file, int line);
#define imxdpu_read_irq(_inst_, _offset_) _imxdpu_read_irq(_inst_, _offset_, __FILE__, __LINE__)
#else
static inline uint32_t imxdpu_read_irq(struct imxdpu_soc *dpu, uint32_t offset)
{
	return __raw_readl(dpu->base + offset);
}
#endif

#ifdef ENABLE_IMXDPU_TRACE_REG
void _imxdpu_write(struct imxdpu_soc *dpu, uint32_t value, uint32_t offset, char *file, int line);
#define imxdpu_write(_inst_, _value_, _offset_) _imxdpu_write(_inst_, _value_, _offset_, __FILE__, __LINE__)
#else
static inline void imxdpu_write(struct imxdpu_soc *dpu, uint32_t offset, uint32_t value)
{
	__raw_writel(value, dpu->base + offset);
}
#endif

#ifdef ENABLE_IMXDPU_TRACE_IRQ_WRITE
void _imxdpu_write_irq(struct imxdpu_soc *dpu, uint32_t value, uint32_t offset, char *file, int line);
#define imxdpu_write_irq(_inst_, _value_, _offset_) _imxdpu_write_irq(_inst_, _value_, _offset_, __FILE__, __LINE__)
#else
static inline void imxdpu_write_irq(struct imxdpu_soc *dpu, uint32_t offset, uint32_t value)
{
	__raw_writel(value, dpu->base + offset);
}
#endif

void _imxdpu_write_block(struct imxdpu_soc *imxdpu, uint32_t offset, void *values, uint32_t cnt, char * file, int line);
#define imxdpu_write_block(_inst_, _values_, _offset_, _cnt_) _imxdpu_write_block(_inst_, _values_, _offset_, _cnt_, __FILE__, __LINE__)

/* mapping of RGB, Tcon, or static values to output */
#define IMXDPU_TCON_MAPBIT__RGB(_x_)   ((_x_))
#define IMXDPU_TCON_MAPBIT__Tsig(_x_)  ((_x_)+ 30)
#define IMXDPU_TCON_MAPBIT__HIGH 42U
#define IMXDPU_TCON_MAPBIT__LOW  43U

/* these match the bit definitions for the shadlow load
   request registers
 */
typedef enum {
	IMXDPU_SHLDREQID_FETCHDECODE9 = 0,
	IMXDPU_SHLDREQID_FETCHPERSP9,
	IMXDPU_SHLDREQID_FETCHECO9,
	IMXDPU_SHLDREQID_CONSTFRAME0,
	IMXDPU_SHLDREQID_CONSTFRAME4,
	IMXDPU_SHLDREQID_CONSTFRAME1,
	IMXDPU_SHLDREQID_CONSTFRAME5,
	IMXDPU_SHLDREQID_EXTSRC4,
	IMXDPU_SHLDREQID_EXTSRC5,
	IMXDPU_SHLDREQID_FETCHDECODE2,
	IMXDPU_SHLDREQID_FETCHDECODE3,
	IMXDPU_SHLDREQID_FETCHWARP2,
	IMXDPU_SHLDREQID_FETCHECO2,
	IMXDPU_SHLDREQID_FETCHDECODE0,
	IMXDPU_SHLDREQID_FETCHECO0,
	IMXDPU_SHLDREQID_FETCHDECODE1,
	IMXDPU_SHLDREQID_FETCHECO1,
	IMXDPU_SHLDREQID_FETCHLAYER0,
	IMXDPU_SHLDREQID_FETCHLAYER1,
	IMXDPU_SHLDREQID_EXTSRC0,
	IMXDPU_SHLDREQID_EXTSRC1
} imxdpu_shadow_load_req_t;

#define IMXDPU_PIXENGCFG_DIVIDER_RESET 0x80

#endif /* IMXDPU_PRIVATE_H */
