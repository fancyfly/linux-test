/*
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
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

#ifndef IMXDPU_H
#define IMXDPU_H

#include <linux/bitops.h>
#include <linux/types.h>

#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/types.h>

//#define DEBUG
//#define ENABLE_IMXDPU_TRACE
//#define ENABLE_IMXDPU_TRACE_REG
//#define ENABLE_IMXDPU_TRACE_IRQ
//#define ENABLE_IMXDPU_TRACE_IRQ_READ
//#define ENABLE_IMXDPU_TRACE_IRQ_WRITE

#ifdef ENABLE_IMXDPU_TRACE
#define IMXDPU_TRACE(fmt, ...) \
printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define IMXDPU_TRACE(fmt, ...) \
no_printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#endif

#ifdef ENABLE_IMXDPU_TRACE_IRQ
#define IMXDPU_TRACE_IRQ(fmt, ...) \
printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define IMXDPU_TRACE_IRQ(fmt, ...) \
no_printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#endif

#ifdef ENABLE_IMXDPU_TRACE_REG
#define IMXDPU_TRACE_REG(fmt, ...) \
printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define IMXDPU_TRACE_REG(fmt, ...) \
no_printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#endif

/* todo: this need to come from device tree */
#define IMXDPU_TCON0_MAP_24BIT_0_23
#if 1
#ifdef IMXDPU_FPGA_BUILD
#define IMXDPU_MAX_NUM		1
#define IMXDPU_NUM_DI           2
#define IMXDPU_REGS_BASE_PHY0	0x01000000
#define IMXDPU_REGS_BASE_PHY1	0x01000000
#define IMXDPU_REGS_BASE_SIZE	0x14000
#else
#define IMXDPU_MAX_NUM		2
#define IMXDPU_NUM_DI           2
#define IMXDPU_REGS_BASE_OFFSET	0x180000
#define IMXDPU_REGS_BASE_PHY0	0x56180000
#define IMXDPU_REGS_BASE_PHY1	0x57180000
#define IMXDPU_REGS_BASE_SIZE	0x14000
#endif
#endif
//#define IMXDPU_MAX_NUM                2
//#define IMXDPU_REGS_BASE_SIZE 0x14000
//#define IMXDPU_NUM_DI           2
//#define IMXDPU_REGS_BASE_OFFSET       0x180000

#define IMXDPU_ENABLE_INTSTAT2

#define IMXDPU_SET_FIELD(field, value) (( (value) << (field ## _SHIFT))&(field ## _MASK))
#define IMXDPU_GET_FIELD(field, reg) (((reg)&(field ## _MASK)) >> (field  ## _SHIFT))

/*
    IMXDPU windows, planes, layers, streams

    IMXDPU hardware documentation confuses the meaning of layers and
	planes. These are software usages of these terms.

    window - a logical buffer of pixels in a rectangular arrangment.
	Image, Integral and video planes suport one window.
	Fractional and warp plane support 8 windows. Blending is not
	supported between the sub-windows of a fractional or warp plane.

    sub-window - one of the eight logical windows of a fractional or warp
	plane.

    channel - the logical DMA configuration for etiher a fetch or store unit

    plane - a plane is a hardware supported feature. There are four types
	of display planes:

	video x2
	fractional x2
	intergral x2
	warp

    layer - each of the 7 planes is fed to a layer blender. Full Alpha
	blending is supported for all of the planes fed to the layer
	blender.

    streams - the layer bleder produces four streams: two normal streams
	(0 and 1) and two panic streams (4 and 5).

	In normal mode, streams 0 and 1 are fed to the displays.
	In panic mode, streams 4 and 5 are fed to the displays.
*/

/*!
 * Enumeration of IMXDPU blend mode flags
 */
typedef enum {
	IMXDPU_PLANE_CLUT               = 1<<0,	/* Color lookup */
	IMXDPU_PLANE_DECODE             = 1<<1,	/* Decode compressed bufers */
	IMXDPU_PLANE_ETERNAL_ALPHA      = 1<<2,	/* supports external alpha buffer  */
	IMXDPU_PLANE_VIDEO_PROC         = 1<<2,	/* Gamma, Matrix, Scaler, histogram  */
	IMXDPU_PLANE_PLANAR             = 1<<3,	/* Support Planar pixel buffers*/
	IMXDPU_PLANE_WARP               = 1<<4,	/* Warping */
	IMXDPU_PLANE_MULTIWINDOW        = 1<<5,	/* Support multiple buffers per plane */
	IMXDPU_PLANE_CAPTURE            = 1<<6,	/* Video capture */
} imxdpu_plane_features_t;

/*!
 * Enumeration of IMXDPU layer blend mode flags
 */
typedef enum {
	IMXDPU_LAYER_NONE               = 1<<0,	/* Disable blending */
	IMXDPU_LAYER_TRANSPARENCY       = 1<<1,	/* Transparency */
	IMXDPU_LAYER_GLOBAL_ALPHA       = 1<<2,	/* Global alpha mode */
	IMXDPU_LAYER_LOCAL_ALPHA        = 1<<3,	/* Alpha contained in source buffer */
	IMXDPU_LAYER_EXTERN_ALPHA       = 1<<4,	/* Alpha is contained in a separate plane */
	IMXDPU_LAYER_PRE_MULITPLY       = 1<<5,	/* Pre-multiply alpha mode */
} imxdpu_layer_blend_modes_t;

/*!
 * Enumeration of IMXDPU layers
 */
typedef enum {
	IMXDPU_LAYER_0 = 0,
	IMXDPU_LAYER_1,
	IMXDPU_LAYER_2,
	IMXDPU_LAYER_3,
	IMXDPU_LAYER_4,
	IMXDPU_LAYER_5,
	IMXDPU_LAYER_6,
	IMXDPU_LAYER_MAX,
} imxdpu_layer_idx_t;

/*!
 * Enumeration of IMXDPU sub-windows
 */
typedef enum {
	IMXDPU_SUBWINDOW_NONE = 0,
	IMXDPU_SUBWINDOW_1,
	IMXDPU_SUBWINDOW_2,
	IMXDPU_SUBWINDOW_3,
	IMXDPU_SUBWINDOW_4,
	IMXDPU_SUBWINDOW_5,
	IMXDPU_SUBWINDOW_6,
	IMXDPU_SUBWINDOW_7,
	IMXDPU_SUBWINDOW_8,
} imxdpu_subwindow_id_t;

/*!
 * Enumeration of IMXDPU display streams
 */
typedef enum {
	IMXDPU_DISPLAY_STREAM_NONE = (1U<<0),
	IMXDPU_DISPLAY_STREAM_0 = (1U<<0),
	IMXDPU_DISPLAY_STREAM_1 = (1U<<1),
	IMXDPU_DISPLAY_STREAM_4 = (1U<<4),
	IMXDPU_DISPLAY_STREAM_5 = (1U<<5),
} imxdpu_display_stream_t;

/*!
 * Enumeration of IMXDPU rotation modes
 */
typedef enum {
	/* todo: these need to aligh to imxdpu scan direction */
	IMXDPU_ROTATE_NONE = 0,
//      IMXDPU_ROTATE_VERT_FLIP = 1,
//      IMXDPU_ROTATE_HORIZ_FLIP = 2,
//      IMXDPU_ROTATE_180 = 3,
//      IMXDPU_ROTATE_90_RIGHT = 4,
//      IMXDPU_ROTATE_90_RIGHT_VFLIP = 5,
//      IMXDPU_ROTATE_90_RIGHT_HFLIP = 6,
//      IMXDPU_ROTATE_90_LEFT = 7,
} imxdpu_rotate_mode_t;

/*!
 * Enumeration of types of buffers for a logical channel.
 */
typedef enum {
	IMXDPU_OUTPUT_BUFFER = 0,	/*!< Buffer for output from IMXDPU BLIT or capture */
	IMXDPU_ALPHA_IN_BUFFER = 1,	/*!< Buffer for alpha input to IMXDPU */
	IMXDPU_GRAPH_IN_BUFFER = 2,	/*!< Buffer for graphics input to IMXDPU */
	IMXDPU_VIDEO_IN_BUFFER = 3,	/*!< Buffer for video input to IMXDPU */
//	IMXDPU_INPUT_BUFFER = IPU_VIDEO_IN_BUFFER,
//	IMXDPU_SEC_INPUT_BUFFER = IPU_GRAPH_IN_BUFFER,
} imxdpu_buffer_t;

/*!
 * Enumeration of IMXDPU logical block ids
 * NOTE: these match the hardware layout and are not arbitrary
 */
typedef enum {
	IMXDPU_ID_NONE = 0,
	IMXDPU_ID_FETCHDECODE9,
	IMXDPU_ID_FETCHPERSP9,
	IMXDPU_ID_FETCHECO9,
	IMXDPU_ID_ROP9,
	IMXDPU_ID_CLUT9,
	IMXDPU_ID_MATRIX9,
	IMXDPU_ID_HSCALER9,
	IMXDPU_ID_VSCALER9,
	IMXDPU_ID_FILTER9,
	IMXDPU_ID_BLITBLEND9,
	IMXDPU_ID_STORE9,
	IMXDPU_ID_CONSTFRAME0,
	IMXDPU_ID_EXTDST0,
	IMXDPU_ID_CONSTFRAME4,
	IMXDPU_ID_EXTDST4,
	IMXDPU_ID_CONSTFRAME1,
	IMXDPU_ID_EXTDST1,
	IMXDPU_ID_CONSTFRAME5,
	IMXDPU_ID_EXTDST5,
	IMXDPU_ID_EXTSRC4,
	IMXDPU_ID_STORE4,
	IMXDPU_ID_EXTSRC5,
	IMXDPU_ID_STORE5,
	IMXDPU_ID_FETCHDECODE2,
	IMXDPU_ID_FETCHDECODE3,
	IMXDPU_ID_FETCHWARP2,
	IMXDPU_ID_FETCHECO2,
	IMXDPU_ID_FETCHDECODE0,
	IMXDPU_ID_FETCHECO0,
	IMXDPU_ID_FETCHDECODE1,
	IMXDPU_ID_FETCHECO1,
	IMXDPU_ID_FETCHLAYER0,
	IMXDPU_ID_FETCHLAYER1,
	IMXDPU_ID_GAMMACOR4,
	IMXDPU_ID_MATRIX4,
	IMXDPU_ID_HSCALER4,
	IMXDPU_ID_VSCALER4,
	IMXDPU_ID_HISTOGRAM4,
	IMXDPU_ID_GAMMACOR5,
	IMXDPU_ID_MATRIX5,
	IMXDPU_ID_HSCALER5,
	IMXDPU_ID_VSCALER5,
	IMXDPU_ID_HISTOGRAM5,
	IMXDPU_ID_LAYERBLEND0,
	IMXDPU_ID_LAYERBLEND1,
	IMXDPU_ID_LAYERBLEND2,
	IMXDPU_ID_LAYERBLEND3,
	IMXDPU_ID_LAYERBLEND4,
	IMXDPU_ID_LAYERBLEND5,
	IMXDPU_ID_LAYERBLEND6,
	IMXDPU_ID_EXTSRC0,
	IMXDPU_ID_EXTSRC1,
	IMXDPU_ID_DISENGCFG,
	IMXDPU_ID_FRAMEGEN0,
	IMXDPU_ID_MATRIX0,
	IMXDPU_ID_GAMMACOR0,
	IMXDPU_ID_DITHER0,
	IMXDPU_ID_TCON0,
	IMXDPU_ID_SIG0,
	IMXDPU_ID_FRAMEGEN1,
	IMXDPU_ID_MATRIX1,
	IMXDPU_ID_GAMMACOR1,
	IMXDPU_ID_DITHER1,
	IMXDPU_ID_TCON1,
	IMXDPU_ID_SIG1,
	IMXDPU_ID_FRAMECAP4,
	IMXDPU_ID_FRAMECAP5,
} imxdpu_id_t;

typedef struct {
	imxdpu_id_t primary;
	imxdpu_id_t secondary;
	//imxdpu_id_t dest;
	imxdpu_display_stream_t stream;
	bool enable;
} imxdpu_layer_t;

typedef enum {
	/* Fetch Channels */
	IMXDPU_CHAN_IDX_IN_FIRST = 0,
	IMXDPU_CHAN_IDX_00 = 0,	/* IMXDPU_ID_SRC_FETCHDECODE2 */
	IMXDPU_CHAN_IDX_01,	/* IMXDPU_ID_SRC_FETCHDECODE0 */
	IMXDPU_CHAN_IDX_02,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_03,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_04,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_05,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_06,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_07,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_08,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_09,	/* IMXDPU_ID_SRC_FETCHLAYER0 */
	IMXDPU_CHAN_IDX_10,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_11,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_12,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_13,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_14,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_15,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_16,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_17,	/* IMXDPU_ID_SRC_FETCHWARP2 */
	IMXDPU_CHAN_IDX_18,	/* IMXDPU_ID_SRC_FETCHDECODE3 */
	IMXDPU_CHAN_IDX_19,	/* IMXDPU_ID_SRC_FETCHDECODE1 */
	IMXDPU_CHAN_IDX_20,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_21,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_22,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_23,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_24,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_25,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_26,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_27,	/* IMXDPU_ID_SRC_FETCHLAYER1 */
	IMXDPU_CHAN_IDX_28,	/* IMXDPU_ID_SRC_ECO0 */
	IMXDPU_CHAN_IDX_29,	/* IMXDPU_ID_SRC_ECO1 */
	IMXDPU_CHAN_IDX_30,	/* IMXDPU_ID_SRC_ECO2 */
	IMXDPU_CHAN_IDX_IN_MAX,	/* Last fetch channel + 1 */

	/* Store Channels */
	IMXDPU_CHAN_IDX_OUT_FIRST = 32,
	IMXDPU_CHAN_IDX_32 = 32,/* IMXDPU_ID_DST_STORE4 */
	IMXDPU_CHAN_IDX_33,	/* IMXDPU_ID_DST_STORE5 */
	IMXDPU_CHAN_IDX_OUT_MAX,/* Last fetch channel + 1 */
	IMXDPU_CHAN_IDX_MAX = IMXDPU_CHAN_IDX_OUT_MAX,
} imxdpu_chan_idx_t;

typedef enum {
	IMXDPU_SUB_NONE = 0,
	IMXDPU_SUB_1 = 1U << 0,	/* IMXDPU_ID_FETCHLAYER0, layer 1 */
	IMXDPU_SUB_2 = 1U << 1,	/* IMXDPU_ID_FETCHLAYER0, layer 2 */
	IMXDPU_SUB_3 = 1U << 2,	/* IMXDPU_ID_FETCHLAYER0, layer 3 */
	IMXDPU_SUB_4 = 1U << 3,	/* IMXDPU_ID_FETCHLAYER0, layer 4 */
	IMXDPU_SUB_5 = 1U << 4,	/* IMXDPU_ID_FETCHLAYER0, layer 5 */
	IMXDPU_SUB_6 = 1U << 5,	/* IMXDPU_ID_FETCHLAYER0, layer 6 */
	IMXDPU_SUB_7 = 1U << 6,	/* IMXDPU_ID_FETCHLAYER0, layer 7 */
	IMXDPU_SUB_8 = 1U << 7,	/* IMXDPU_ID_FETCHLAYER0, layer 8 */
} imxdpu_chan_sub_idx_t;

/*  IMXDPU Channel
 *	Consistist of four fields
 *	src - block id of source or destination
 *	sec - block id of secondary source for fetcheco
 *	sub - sub index of block for fetchlayer or fetchwarp
 *	idx - logical channel index
 *
 */
#define make_channel(__blk_id, __eco_id, __sub, __idx) \
(((__u32)(__idx)<<0)|((__u32)(__eco_id)<<8)|((__u32)(__sub)<<16)|((__u32)(__blk_id)<<24))

#define get_channel_blk(chan) (((__u32)(chan)>>24)&0xff )
#define get_channel_sub(chan) (((__u32)(chan)>>16)&0xff )
#define get_eco_idx(chan)     (((__u32)(chan)>> 8)&0xff )
#define get_channel_idx(chan) (((__u32)(chan)>> 0)&0xff )
#define IMXDPU_SUBCHAN_LAYER_OFFSET 0x28

typedef enum {
	/* Fetch Channels */
	IMXDPU_CHAN_00 = make_channel(IMXDPU_ID_FETCHDECODE2, IMXDPU_ID_NONE, IMXDPU_SUB_NONE, 0),
	IMXDPU_CHAN_01 = make_channel(IMXDPU_ID_FETCHDECODE0,             28, IMXDPU_SUB_NONE, 1),
	IMXDPU_CHAN_02 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_1, 2),
	IMXDPU_CHAN_03 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_2, 3),
	IMXDPU_CHAN_04 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_3, 4),
	IMXDPU_CHAN_05 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_4, 5),
	IMXDPU_CHAN_06 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_5, 6),
	IMXDPU_CHAN_07 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_6, 7),
	IMXDPU_CHAN_08 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_7, 8),
	IMXDPU_CHAN_09 = make_channel(IMXDPU_ID_FETCHLAYER0,  IMXDPU_ID_NONE, IMXDPU_SUB_8, 9),
	IMXDPU_CHAN_10 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_1, 10),
	IMXDPU_CHAN_11 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_2, 11),
	IMXDPU_CHAN_12 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_3, 12),
	IMXDPU_CHAN_13 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_4, 13),
	IMXDPU_CHAN_14 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_5, 14),
	IMXDPU_CHAN_15 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_6, 15),
	IMXDPU_CHAN_16 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_7, 16),
	IMXDPU_CHAN_17 = make_channel(IMXDPU_ID_FETCHWARP2,               30, IMXDPU_SUB_8, 17),
	IMXDPU_CHAN_18 = make_channel(IMXDPU_ID_FETCHDECODE3,             30, IMXDPU_SUB_NONE, 18),
	IMXDPU_CHAN_19 = make_channel(IMXDPU_ID_FETCHDECODE1,             29, IMXDPU_SUB_NONE, 19),
	IMXDPU_CHAN_20 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_1, 20),
	IMXDPU_CHAN_21 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_2, 21),
	IMXDPU_CHAN_22 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_3, 22),
	IMXDPU_CHAN_23 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_4, 23),
	IMXDPU_CHAN_24 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_5, 24),
	IMXDPU_CHAN_25 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_6, 25),
	IMXDPU_CHAN_26 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_7, 26),
	IMXDPU_CHAN_27 = make_channel(IMXDPU_ID_FETCHLAYER1,  IMXDPU_ID_NONE, IMXDPU_SUB_8, 27),
	IMXDPU_CHAN_28 = make_channel(IMXDPU_ID_FETCHECO0,    IMXDPU_ID_NONE, IMXDPU_SUB_NONE, 28),
	IMXDPU_CHAN_29 = make_channel(IMXDPU_ID_FETCHECO1,    IMXDPU_ID_NONE, IMXDPU_SUB_NONE, 29),
	IMXDPU_CHAN_30 = make_channel(IMXDPU_ID_FETCHECO2,    IMXDPU_ID_NONE, IMXDPU_SUB_NONE, 30),
	/* Store Channels */
	IMXDPU_CHAN_32 = make_channel(IMXDPU_ID_STORE4, IMXDPU_ID_NONE, IMXDPU_SUB_NONE, 32),
	IMXDPU_CHAN_33 = make_channel(IMXDPU_ID_STORE5, IMXDPU_ID_NONE, IMXDPU_SUB_NONE, 33),
} imxdpu_chan_t;

/* Aliases for Channels */
#define IMXDPU_CHAN_VIDEO_0        IMXDPU_CHAN_01
#define IMXDPU_CHAN_VIDEO_1        IMXDPU_CHAN_19

#define IMXDPU_CHAN_INTERGRAL_0    IMXDPU_CHAN_00
#define IMXDPU_CHAN_INTERGRAL_1    IMXDPU_CHAN_18

#define IMXDPU_CHAN_FRACTIONAL_0_1 IMXDPU_CHAN_02
#define IMXDPU_CHAN_FRACTIONAL_0_2 IMXDPU_CHAN_03
#define IMXDPU_CHAN_FRACTIONAL_0_3 IMXDPU_CHAN_04
#define IMXDPU_CHAN_FRACTIONAL_0_4 IMXDPU_CHAN_05
#define IMXDPU_CHAN_FRACTIONAL_0_5 IMXDPU_CHAN_06
#define IMXDPU_CHAN_FRACTIONAL_0_6 IMXDPU_CHAN_07
#define IMXDPU_CHAN_FRACTIONAL_0_7 IMXDPU_CHAN_08
#define IMXDPU_CHAN_FRACTIONAL_0_8 IMXDPU_CHAN_09

#define IMXDPU_CHAN_FRACTIONAL_1_1 IMXDPU_CHAN_20
#define IMXDPU_CHAN_FRACTIONAL_1_2 IMXDPU_CHAN_21
#define IMXDPU_CHAN_FRACTIONAL_1_3 IMXDPU_CHAN_22
#define IMXDPU_CHAN_FRACTIONAL_1_4 IMXDPU_CHAN_23
#define IMXDPU_CHAN_FRACTIONAL_1_5 IMXDPU_CHAN_24
#define IMXDPU_CHAN_FRACTIONAL_1_6 IMXDPU_CHAN_25
#define IMXDPU_CHAN_FRACTIONAL_1_7 IMXDPU_CHAN_26
#define IMXDPU_CHAN_FRACTIONAL_1_8 IMXDPU_CHAN_27

#define IMXDPU_CHAN_WARP_2_1 IMXDPU_CHAN_10
#define IMXDPU_CHAN_WARP_2_2 IMXDPU_CHAN_11
#define IMXDPU_CHAN_WARP_2_3 IMXDPU_CHAN_12
#define IMXDPU_CHAN_WARP_2_4 IMXDPU_CHAN_13
#define IMXDPU_CHAN_WARP_2_5 IMXDPU_CHAN_14
#define IMXDPU_CHAN_WARP_2_6 IMXDPU_CHAN_15
#define IMXDPU_CHAN_WARP_2_7 IMXDPU_CHAN_16
#define IMXDPU_CHAN_WARP_2_8 IMXDPU_CHAN_17

/*  IMXDPU Pixel format definitions */
/*  Four-character-code (FOURCC) */
#ifdef fourcc
#warning "fourcc is already defined ... redeifining it here!"
#undef  fourcc
#endif
#define fourcc(a, b, c, d)\
	 (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

/*! @} */
/*! @name Generic Formats */
/*! @{ */
#define IMXDPU_PIX_FMT_GENERIC fourcc('D', 'P', 'U', '0')	/*!< IPU Generic Data */
#define IMXDPU_PIX_FMT_GENERIC_32 fourcc('D', 'P', 'U', '1')	/*!< IPU Generic Data */
#define IMXDPU_PIX_FMT_GENERIC_16 fourcc('D', 'P', 'U', '2')	/*!< IPU Generic Data */

/*! @} */
/*! @name RGB Formats */
/*! @{ */
#define IMXDPU_PIX_FMT_RGB332  fourcc('R', 'G', 'B', '1')	/*!<  8  RGB-3-3-2    */
#define IMXDPU_PIX_FMT_RGB555  fourcc('R', 'G', 'B', 'O')	/*!< 16  RGB-5-5-5    */
#define IMXDPU_PIX_FMT_RGB565  fourcc('R', 'G', 'B', 'P')	/*!< 16  RGB-5-6-5    */
#define IMXDPU_PIX_FMT_BGRA4444 fourcc('4', '4', '4', '4')	/*!< 16  RGBA-4-4-4-4 */
#define IMXDPU_PIX_FMT_BGRA5551 fourcc('5', '5', '5', '1')	/*!< 16  RGBA-5-5-5-1 */
#define IMXDPU_PIX_FMT_RGB666  fourcc('R', 'G', 'B', '6')	/*!< 18  RGB-6-6-6    */
#define IMXDPU_PIX_FMT_BGR666  fourcc('B', 'G', 'R', '6')	/*!< 18  BGR-6-6-6    */
#define IMXDPU_PIX_FMT_BGR24   fourcc('B', 'G', 'R', '3')	/*!< 24  BGR-8-8-8    */
#define IMXDPU_PIX_FMT_RGB24   fourcc('R', 'G', 'B', '3')	/*!< 24  RGB-8-8-8    */
#define IMXDPU_PIX_FMT_GBR24   fourcc('G', 'B', 'R', '3')	/*!< 24  GBR-8-8-8    */
#define IMXDPU_PIX_FMT_BGR32   fourcc('B', 'G', 'R', '4')	/*!< 32  BGR-8-8-8-8  */
#define IMXDPU_PIX_FMT_BGRA32  fourcc('B', 'G', 'R', 'A')	/*!< 32  BGR-8-8-8-8  */
#define IMXDPU_PIX_FMT_RGB32   fourcc('R', 'G', 'B', '4')	/*!< 32  RGB-8-8-8-8  */
#define IMXDPU_PIX_FMT_RGBA32  fourcc('R', 'G', 'B', 'A')	/*!< 32  RGB-8-8-8-8  */
#define IMXDPU_PIX_FMT_ABGR32  fourcc('A', 'B', 'G', 'R')	/*!< 32  ABGR-8-8-8-8 */
#define IMXDPU_PIX_FMT_ARGB32  fourcc('A', 'R', 'G', 'B')	/*!< 32  ARGB-8-8-8-8 */

/*! @} */
/*! @name YUV Interleaved Formats */
/*! @{ */
#define IMXDPU_PIX_FMT_YUYV    fourcc('Y', 'U', 'Y', 'V')	/*!< 16 YUV 4:2:2 */
#define IMXDPU_PIX_FMT_UYVY    fourcc('U', 'Y', 'V', 'Y')	/*!< 16 YUV 4:2:2 */
#define IMXDPU_PIX_FMT_YVYU    fourcc('Y', 'V', 'Y', 'U')	/*!< 16 YVYU 4:2:2 */
#define IMXDPU_PIX_FMT_VYUY    fourcc('V', 'Y', 'U', 'Y')	/*!< 16 VYYU 4:2:2 */
#define IMXDPU_PIX_FMT_Y41P    fourcc('Y', '4', '1', 'P')	/*!< 12 YUV 4:1:1 */
#define IMXDPU_PIX_FMT_YUV444  fourcc('Y', '4', '4', '4')	/*!< 24 YUV 4:4:4 */
#define IMXDPU_PIX_FMT_VYU444  fourcc('V', '4', '4', '4')	/*!< 24 VYU 4:4:4 */
#define IMXDPU_PIX_FMT_AYUV    fourcc('A', 'Y', 'U', 'V')	/*!< 32 AYUV 4:4:4:4 */

/* two planes -- one Y, one Cb + Cr interleaved  */
#define IMXDPU_PIX_FMT_NV12    fourcc('N', 'V', '1', '2')	/* 12  Y/CbCr 4:2:0  */
#define IMXDPU_PIX_FMT_NV16    fourcc('N', 'V', '1', '6')	/* 16  Y/CbCr 4:2:2  */

struct imxdpu_soc;
/*!
 * Definition of IMXDPU rectangle structure
 */
typedef struct {
	int16_t top;		/* y coordinate of top/left pixel */
	int16_t left;		/* x coordinate top/left pixel */
	int16_t width;
	int16_t height;
} imxdpu_rect_t;

/*!
 * Union of initialization parameters for a logical channel.
 */
typedef union {
	struct {
		imxdpu_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id;
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
	} common;
	struct {
		imxdpu_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id;
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		uint32_t h_scale_factor;	/* downscaling  out/in */
		uint32_t h_phase;
		uint32_t v_scale_factor;	/* downscaling  out/in */
		uint32_t v_phase[2][2];
		bool use_video_proc;
		bool interlaced;
	} store;
	struct {
		imxdpu_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id;
		int16_t dest_top;
		int16_t dest_left;
		uint16_t dest_width;
		uint16_t dest_height;
		uint32_t const_color;
		uint32_t h_scale_factor;	/* downscaling  out/in */
		uint32_t h_phase;
		uint32_t v_scale_factor;	/* downscaling  out/in */
		uint32_t v_phase[2][2];
		bool use_video_proc;
		bool interlaced;
	} fetch_decode;
	struct {
		imxdpu_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id;
		int16_t dest_top;
		int16_t dest_left;
		uint32_t dest_width;
		uint32_t dest_height;
		uint32_t const_color;
	} fetch_layer;
	struct {
		imxdpu_chan_t chan;
		uint32_t src_pixel_fmt;
		uint16_t src_width;
		uint16_t src_height;
		int16_t clip_top;
		int16_t clip_left;
		uint16_t clip_width;
		uint16_t clip_height;
		uint16_t stride;
		uint32_t dest_pixel_fmt;
		uint8_t blend_mode;
		uint8_t blend_layer;
		uint8_t disp_id;
		int16_t dest_top;
		int16_t dest_left;
		uint32_t dest_width;
		uint32_t dest_height;
		uint32_t const_color;
		// todo: add warp parameters
	} fetch_warp;
} imxdpu_channel_params_t;

/*!
 * Enumeration of IMXDPU video mode flags
 */
enum imxdpu_disp_flags {
	IMXDPU_DISP_FLAGS_HSYNC_LOW     = 1<<0,
	IMXDPU_DISP_FLAGS_HSYNC_HIGH    = 1<<1,
	IMXDPU_DISP_FLAGS_VSYNC_LOW     = 1<<2,
	IMXDPU_DISP_FLAGS_VSYNC_HIGH    = 1<<3,
	IMXDPU_DISP_FLAGS_DE_LOW        = 1<<4,
	IMXDPU_DISP_FLAGS_DE_HIGH       = 1<<5,

	/* drive data on positive .edge */
	IMXDPU_DISP_FLAGS_POSEDGE       = 1<<6,
	/* drive data on negative edge */
	IMXDPU_DISP_FLAGS_NEGEDGE       = 1<<7,

	IMXDPU_DISP_FLAGS_INTERLACED    = 1<<8 ,

	/* Left/Right Synchronous display mode,  both display pipe are
	   combined to make one display. All mode timings are divided by
	   two for each half screen.
	   Note: This may not be needed we may force this for any width
	   over ~2048
	 */
	IMXDPU_DISP_FLAGS_LRSYNC        = 1<<16,

	/* Split mode each pipe is split into two displays  */
	IMXDPU_DISP_FLAGS_SPLIT         = 1<<16,
};

struct imxdpu_videomode {
	char name[64];		/* may not be needed */

	uint32_t pixelclock;	/* Hz */

	/* htotal (pixels) = hlen + hfp + hsync + hbp */
	uint32_t hlen;
	uint32_t hfp;
	uint32_t hbp;
	uint32_t hsync;

	/* vtotal (lines) = vlen + vfp + vsync + vbp */
	uint32_t vlen;
	uint32_t vfp;
	uint32_t vbp;
	uint32_t vsync;

	enum imxdpu_disp_flags flags;
};

#define IMXDPU_ENABLE  1
#define IMXDPU_DISABLE 0

#define IMXDPU_TRUE    1
#define IMXDPU_FALSE   0
#define IMXDPU_OFFSET_INVALID 0x10000000	/* this should force an access error */
#define IMXDPU_CHANNEL_INVALID 0x0	/* this should force an access error */

#define IMXDPU_MIN(_X,_Y) ((_X) < (_Y) ? (_X) : (_Y))

/* Native color type */
#define IMXDPU_COLOR_CONSTALPHA_MASK 0xFFU
#define IMXDPU_COLOR_CONSTALPHA_SHIFT 0U
#define IMXDPU_COLOR_CONSTBLUE_MASK 0xFF00U
#define IMXDPU_COLOR_CONSTBLUE_SHIFT 8U
#define IMXDPU_COLOR_CONSTGREEN_MASK 0xFF0000U
#define IMXDPU_COLOR_CONSTGREEN_SHIFT 16U
#define IMXDPU_COLOR_CONSTRED_MASK  0xFF000000U
#define IMXDPU_COLOR_CONSTRED_SHIFT 24U

int imxdpu_enable_irq(int8_t imxdpu_id, uint32_t irq);
int imxdpu_disable_irq(int8_t imxdpu_id, uint32_t irq);
int imxdpu_clear_all_irqs(int8_t imxdpu_id);
int imxdpu_clear_irq(int8_t imxdpu_id, uint32_t irq);
int imxdpu_init_irqs(int8_t imxdpu_id);
int imxdpu_request_irq(int8_t imxdpu_id,
	uint32_t irq,
	int(*handler) (int, void *),
	uint32_t irq_flags,
	const char *devname, void *data) ;
int imxdpu_free_irq(int8_t imxdpu_id, uint32_t irq, void *data);
int imxdpu_uninit_interrupts(int8_t imxdpu_id);
int imxdpu_handle_irq(int32_t imxdpu_id);
struct imxdpu_soc *imxdpu_get_soc(int8_t imxdpu_id);
int imxdpu_init(int8_t imxdpu_id, void __iomem * imxdpu_base);
int imxdpu_init_sync_panel(int8_t imxdpu_id, int8_t disp,
	uint32_t pixel_fmt,
	struct imxdpu_videomode mode);
int imxdpu_uninit_sync_panel(int8_t imxdpu_id, int8_t disp);
int imxdpu_reset_disp_panel(int8_t imxdpu_id, int8_t disp);
int imxdpu_disp_init(int8_t imxdpu_id, int8_t disp);
int imxdpu_disp_setup_frame_gen(
	int8_t imxdpu_id,
	int8_t disp,
	const struct imxdpu_videomode *mode,
	uint16_t cc_red,	/* 10 bits */
	uint16_t cc_green,	/* 10 bits */
	uint16_t cc_blue,	/* 10 bits */
	uint8_t cc_alpha,
	bool test_mode_enable);
int imxdpu_disp_enable_frame_gen(int8_t imxdpu_id,
	int8_t disp,
	bool enable);
int imxdpu_disp_setup_constframe(int8_t imxdpu_id,
	int8_t disp,
	uint8_t bg_red,
	uint8_t bg_green,
	uint8_t bg_blue,
	uint8_t bg_alpha);
int imxdpu_disp_setup_layer(int8_t imxdpu_id,
	const imxdpu_layer_t *layer,
	imxdpu_layer_idx_t layer_idx);
void imxdpu_disp_dump_mode(const struct imxdpu_videomode *mode);
int imxdpu_bytes_per_pixel(uint32_t fmt);
int imxdpu_init_channel_buffer(int8_t imxdpu_id,
	imxdpu_chan_t chan,
	uint32_t stride,
	imxdpu_rotate_mode_t rot_mode,
	dma_addr_t phyaddr_0,
//	dma_addr_t phyaddr_1,
//	dma_addr_t phyaddr_2,
	uint32_t u_offset,
	uint32_t v_offset);
int32_t imxdpu_update_channel_buffer(int8_t imxdpu_id,
	imxdpu_chan_t chan,
	dma_addr_t phyaddr_0);
int imxdpu_init_channel(int8_t imxdpu_id,
	imxdpu_channel_params_t *params);
int imxdpu_disp_set_layer_global_alpha(int8_t imxdpu_id,
	imxdpu_layer_idx_t layer_idx,
	uint8_t alpha);
int imxdpu_disp_set_layer_position(int8_t imxdpu_id,
	imxdpu_layer_idx_t layer_idx,
	int16_t x, int16_t y);
int imxdpu_disp_set_chan_position(int8_t imxdpu_id,
	imxdpu_chan_t chan,
	int16_t x, int16_t y);
int imxdpu_disp_update_fgen_status(int8_t imxdpu_id, int8_t disp);
int imxdpu_disp_show_fgen_status(int8_t imxdpu_id);
void dump_int_stat(int8_t imxdpu_id);
void dump_layerblend(int8_t imxdpu_id);
int imxdpu_disp_force_shadow_load(int8_t imxdpu_id,
	int8_t disp,
	uint64_t mask);
int imxdpu_disp_set_chan_crop(int8_t imxdpu_id,
	imxdpu_chan_t chan,
	int16_t  clip_top,
	int16_t  clip_left,
	uint16_t clip_width,
	uint16_t clip_height,
	int16_t  dest_top,
	int16_t  dest_left,
	uint16_t dest_width,
	uint16_t dest_height);
void dump_pixencfg_status(int8_t imxdpu_id);
int dump_channel(int8_t imxdpu_id, imxdpu_chan_t chan);
uint32_t imxdpu_get_planes(uint32_t fmt);

int imxdpu_disp_setup_channel(int8_t imxdpu_id,
			      imxdpu_chan_t chan,
			      uint32_t src_pixel_fmt,
			      uint16_t src_width,
			      uint16_t src_height,
			      int16_t clip_top,
			      int16_t clip_left,
			      uint16_t clip_width,
			      uint16_t clip_height, uint16_t stride,
			      //      uint32_t dest_pixel_fmt,
			      //      uint8_t  blend_mode,
			      //      uint8_t  blend_layer,
			      uint8_t disp_id,
			      int16_t dest_top,
			      int16_t dest_left,
			      uint16_t dest_width,
			      uint16_t dest_height,
			      uint32_t const_color, unsigned int disp_addr);
int imxdpu_disp_check_shadow_loads(int8_t imxdpu_id, int8_t disp);
#endif				/* IMXDPU_H */
