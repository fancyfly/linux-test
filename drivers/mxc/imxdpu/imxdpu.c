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
#include <linux/io.h>
#include <linux/types.h>

#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/imxdpu.h>

#include "imxdpu_private.h"
#include "imxdpu_registers.h"
#include "imxdpu_events.h"
#include "imxdpu_be.h"

#define ptr_to_uint32(__ptr__) ((uint32_t)((uint64_t)(__ptr__ )))
/* Private data*/
static struct imxdpu_soc imxdpu_array[IMXDPU_MAX_NUM];

typedef struct {
	uint8_t len;
	uint8_t buffers;
} imxdpu_burst_entry_t;

static const imxdpu_burst_entry_t burst_param[] = {
	{ 0, 0 },         /* IMXDPU_SCAN_DIR_UNKNOWN */
	{ 8, 32 },        /* IMXDPU_SCAN_DIR_LEFT_RIGHT_DOWN */
	{ 16, 16 },       /* IMXDPU_SCAN_DIR_HORIZONTAL */
	{ 8, 32 },        /* IMXDPU_SCAN_DIR_VERTICAL possibly 8/32 here */
	{ 8, 32 },        /* IMXDPU_SCAN_DIR_FREE */
};

typedef struct {
	uint32_t extdst;
	uint32_t sub;
} trigger_entry_t;

static const trigger_entry_t trigger_list[IMXDPU_SHDLD_IDX_MAX] = {
	/*  IMXDPU_SHDLD_* extdst,          sub */
	/* _DISP0    */{ 1, 0 },
	/* _DISP1    */{ 1, 0 },
	/* _CONST0   */{ IMXDPU_SHDLD_CONSTFRAME0, 0 },
	/* _CONST1   */{ IMXDPU_SHDLD_CONSTFRAME1, 0 },
	/* _CHAN_00  */{ IMXDPU_SHDLD_FETCHDECODE2, 0 },
	/* _CHAN_01  */{ IMXDPU_SHDLD_FETCHDECODE0, 0 },
	/* _CHAN_02  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_1 },
	/* _CHAN_03  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_2 },
	/* _CHAN_04  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_3 },
	/* _CHAN_05  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_4 },
	/* _CHAN_06  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_5 },
	/* _CHAN_07  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_6 },
	/* _CHAN_08  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_7 },
	/* _CHAN_09  */{ IMXDPU_SHDLD_FETCHLAYER0, IMXDPU_SUB_8 },
	/* _CHAN_10  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_1 << 16 },
	/* _CHAN_11  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_2 << 16 },
	/* _CHAN_12  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_3 << 16 },
	/* _CHAN_13  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_4 << 16 },
	/* _CHAN_14  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_5 << 16 },
	/* _CHAN_15  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_6 << 16 },
	/* _CHAN_16  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_7 << 16 },
	/* _CHAN_17  */{ IMXDPU_SHDLD_FETCHWARP2, IMXDPU_SUB_8 << 16 },
	/* _CHAN_18  */{ IMXDPU_SHDLD_FETCHDECODE3, 0 },
	/* _CHAN_19  */{ IMXDPU_SHDLD_FETCHDECODE1, 0 },
	/* _CHAN_20  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_1 << 8 },
	/* _CHAN_21  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_2 << 8 },
	/* _CHAN_22  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_3 << 8 },
	/* _CHAN_23  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_4 << 8 },
	/* _CHAN_24  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_5 << 8 },
	/* _CHAN_25  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_6 << 8 },
	/* _CHAN_26  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_7 << 8 },
	/* _CHAN_27  */{ IMXDPU_SHDLD_FETCHLAYER1, IMXDPU_SUB_8 << 8 },
	/* _CHAN_28  */{ IMXDPU_SHDLD_FETCHECO0, 0 },
	/* _CHAN_29  */{ IMXDPU_SHDLD_FETCHECO1, 0 },
	/* _CHAN_30  */{ IMXDPU_SHDLD_FETCHECO2, 0 }
};


#ifdef ENABLE_IMXDPU_TRACE_REG
uint32_t _imxdpu_read(struct imxdpu_soc *imxdpu, uint32_t offset, char *file,
		      int line)
{
	uint32_t val = 0;
	val = __raw_readl(imxdpu->base + offset);
	IMXDPU_TRACE_REG("%s:%d R reg 0x%08x --> val 0x%08x\n", file, line,
			 (uint32_t)offset, (uint32_t)val);
	return val;
}

void _imxdpu_write(struct imxdpu_soc *imxdpu, uint32_t offset, uint32_t value,
		   char *file, int line)
{
	__raw_writel(value, imxdpu->base + offset);
	IMXDPU_TRACE_REG("%s:%d W reg 0x%08x <-- val 0x%08x\n", file, line,
			 (uint32_t)offset, (uint32_t)value);
}

#endif

void _imxdpu_write_block(struct imxdpu_soc *imxdpu, uint32_t offset,
			 void *values, uint32_t cnt, char *file, int line)
{
	int i;
	uint32_t *dest = (uint32_t *)(imxdpu->base + offset);
	uint32_t *src = (uint32_t *)values;
	for (i = 0; i < cnt; i++) {
		dest[i] = src[i];
	}
	//memcpy((void *)(imxdpu->base + offset), values, cnt * 4);
	IMXDPU_TRACE_REG("%s:%d W reg 0x%08x <-- cnt 0x%08x\n", file, line,
			 (uint32_t)offset, (uint32_t)cnt);
}

#ifdef ENABLE_IMXDPU_TRACE_IRQ_READ
uint32_t _imxdpu_read_irq(struct imxdpu_soc *imxdpu, uint32_t offset,
			  char *file, int line)
{
	uint32_t val = 0;
	val = __raw_readl(imxdpu->base + offset);
	IMXDPU_TRACE_IRQ("%s:%d IRQ R reg 0x%08x --> val 0x%08x\n", file, line,
			 (uint32_t)offset, (uint32_t)val);
	return val;
}
#endif

#ifdef ENABLE_IMXDPU_TRACE_IRQ_WRITE
void _imxdpu_write_irq(struct imxdpu_soc *imxdpu, uint32_t offset,
		       uint32_t value, char *file, int line)
{
	__raw_writel(value, imxdpu->base + offset);
	IMXDPU_TRACE_IRQ("%s:%d IRQ W reg 0x%08x <-- val 0x%08x\n", file, line,
			 (uint32_t)offset, (uint32_t)value);
}
#endif

/* static prototypes */
int dump_channel(int8_t imxdpu_id, imxdpu_chan_t chan);
static int imxdpu_disp_start_shadow_loads(int8_t imxdpu_id, int8_t disp);
void dump_pixencfg_status(int8_t imxdpu_id);
static bool imxdpu_is_yuv(uint32_t fmt);

static bool imxdpu_is_yuv(uint32_t fmt);
/*!
 * Returns IMXDPU_TRUE for a valid channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_chan(imxdpu_chan_t chan)
{
	imxdpu_chan_idx_t chan_idx = get_channel_idx(chan);

	if ((chan_idx >= IMXDPU_CHAN_IDX_IN_FIRST) &&
		(chan_idx < IMXDPU_CHAN_IDX_IN_MAX)) return IMXDPU_TRUE;
	if ((chan_idx >= IMXDPU_CHAN_IDX_OUT_FIRST) &&
		(chan_idx < IMXDPU_CHAN_IDX_OUT_MAX)) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE for a valid store channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_store_chan(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_STORE4) || (blk_id == IMXDPU_ID_STORE4)) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE for a valid fetch channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_fetch_eco_chan(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_FETCHECO0) ||
		(blk_id == IMXDPU_ID_FETCHECO1) ||
		(blk_id == IMXDPU_ID_FETCHECO2))  return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE for a valid fetch decode channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_fetch_decode_chan(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_FETCHDECODE0) ||
		(blk_id == IMXDPU_ID_FETCHDECODE1) ||
		(blk_id == IMXDPU_ID_FETCHDECODE2) ||
		(blk_id == IMXDPU_ID_FETCHDECODE3)) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE if a fetch channel has an eco fetch
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int has_fetch_eco_chan(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_FETCHDECODE0) ||
		(blk_id == IMXDPU_ID_FETCHDECODE1) ||
		(blk_id == IMXDPU_ID_FETCHWARP2)) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE for a valid fetch warp channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_fetch_warp_chan(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_FETCHWARP2)) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE for a valid fetch layer channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_fetch_layer_chan(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_FETCHLAYER0) ||
		(blk_id == IMXDPU_ID_FETCHLAYER1)) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns IMXDPU_TRUE for a valid layer sub1 channel
 *
 * @param	channel to test
 *
 * @return 	This function returns IMXDPU_TRUE on success or
 *		IMXDPU_FALSE if the test fails.
 */
static int is_fetch_layer_sub_chan1(imxdpu_chan_t chan)
{
	imxdpu_id_t blk_id = get_channel_blk(chan);
	if ((blk_id == IMXDPU_ID_FETCHLAYER0) ||
		(blk_id == IMXDPU_ID_FETCHLAYER1) ||
		(blk_id == IMXDPU_ID_FETCHWARP2)) if (get_channel_sub(chan) == IMXDPU_SUB_1) return IMXDPU_TRUE;
	return IMXDPU_FALSE;
}

/*!
 * Returns subindex of a channel
 *
 * @param	channel
 *
 * @return 	returns number of bits per pixel or zero
 * 		if the format is not matched.
 */
static int imxdpu_get_channel_subindex(imxdpu_chan_t chan)
{
	switch (get_channel_sub(chan)) {
		case IMXDPU_SUB_2:
			return 1;
		case IMXDPU_SUB_3:
			return 2;
		case IMXDPU_SUB_4:
			return 3;
		case IMXDPU_SUB_5:
			return 4;
		case IMXDPU_SUB_6:
			return 5;
		case IMXDPU_SUB_7:
			return 6;
		case IMXDPU_SUB_8:
			return 7;
		case IMXDPU_SUB_1:
		case IMXDPU_SUBWINDOW_NONE:
		default:
			return 0;
	}
}

/*!
 * Returns returns the eco channel for a channel index
 *
 * @param       chan
 *
 * @return 	returns number of bits per pixel or zero
 * 		if the format is not matched.
 */
imxdpu_chan_t imxdpu_get_eco(imxdpu_chan_t chan)
{
	switch (get_eco_idx(chan)) {
		case get_channel_idx(IMXDPU_CHAN_28):
			return IMXDPU_CHAN_28;
		case get_channel_idx(IMXDPU_CHAN_29):
			return IMXDPU_CHAN_29;
		case get_channel_idx(IMXDPU_CHAN_30):
			return IMXDPU_CHAN_30;
		default:
			return 0;
	}
}
/*!
 * Returns the start address offset for a given block ID
 *
 * @param	block id
 *
 * @return 	This function returns the address offset if the block id
 *		matches a valid block. Otherwise, IMXDPU_OFFSET_INVALID
 *		is returned.
 */
uint32_t id2blockoffset(imxdpu_id_t block_id)
{
	switch (block_id) {
		/*case IMXDPU_ID_NONE:         return IMXDPU_NONE_LOCKUNLOCK; */
		case IMXDPU_ID_FETCHDECODE9:
			return IMXDPU_FETCHDECODE9_LOCKUNLOCK;
		case IMXDPU_ID_FETCHPERSP9:
			return IMXDPU_FETCHPERSP9_LOCKUNLOCK;
		case IMXDPU_ID_FETCHECO9:
			return IMXDPU_FETCHECO9_LOCKUNLOCK;
		case IMXDPU_ID_ROP9:
			return IMXDPU_ROP9_LOCKUNLOCK;
		case IMXDPU_ID_CLUT9:
			return IMXDPU_CLUT9_LOCKUNLOCK;
		case IMXDPU_ID_MATRIX9:
			return IMXDPU_MATRIX9_LOCKUNLOCK;
		case IMXDPU_ID_HSCALER9:
			return IMXDPU_HSCALER9_LOCKUNLOCK;
		case IMXDPU_ID_VSCALER9:
			return IMXDPU_VSCALER9_LOCKUNLOCK;
		case IMXDPU_ID_FILTER9:
			return IMXDPU_FILTER9_LOCKUNLOCK;
		case IMXDPU_ID_BLITBLEND9:
			return IMXDPU_BLITBLEND9_LOCKUNLOCK;
		case IMXDPU_ID_STORE9:
			return IMXDPU_STORE9_LOCKUNLOCK;
		case IMXDPU_ID_CONSTFRAME0:
			return IMXDPU_CONSTFRAME0_LOCKUNLOCK;
		case IMXDPU_ID_EXTDST0:
			return IMXDPU_EXTDST0_LOCKUNLOCK;
		case IMXDPU_ID_CONSTFRAME4:
			return IMXDPU_CONSTFRAME4_LOCKUNLOCK;
		case IMXDPU_ID_EXTDST4:
			return IMXDPU_EXTDST4_LOCKUNLOCK;
		case IMXDPU_ID_CONSTFRAME1:
			return IMXDPU_CONSTFRAME1_LOCKUNLOCK;
		case IMXDPU_ID_EXTDST1:
			return IMXDPU_EXTDST1_LOCKUNLOCK;
		case IMXDPU_ID_CONSTFRAME5:
			return IMXDPU_CONSTFRAME5_LOCKUNLOCK;
		case IMXDPU_ID_EXTDST5:
			return IMXDPU_EXTDST5_LOCKUNLOCK;
		case IMXDPU_ID_EXTSRC4:
			return IMXDPU_EXTSRC4_LOCKUNLOCK;
		case IMXDPU_ID_STORE4:
			return IMXDPU_STORE4_LOCKUNLOCK;
		case IMXDPU_ID_EXTSRC5:
			return IMXDPU_EXTSRC5_LOCKUNLOCK;
		case IMXDPU_ID_STORE5:
			return IMXDPU_STORE5_LOCKUNLOCK;
		case IMXDPU_ID_FETCHDECODE2:
			return IMXDPU_FETCHDECODE2_LOCKUNLOCK;
		case IMXDPU_ID_FETCHDECODE3:
			return IMXDPU_FETCHDECODE3_LOCKUNLOCK;
		case IMXDPU_ID_FETCHWARP2:
			return IMXDPU_FETCHWARP2_LOCKUNLOCK;
		case IMXDPU_ID_FETCHECO2:
			return IMXDPU_FETCHECO2_LOCKUNLOCK;
		case IMXDPU_ID_FETCHDECODE0:
			return IMXDPU_FETCHDECODE0_LOCKUNLOCK;
		case IMXDPU_ID_FETCHECO0:
			return IMXDPU_FETCHECO0_LOCKUNLOCK;
		case IMXDPU_ID_FETCHDECODE1:
			return IMXDPU_FETCHDECODE1_LOCKUNLOCK;
		case IMXDPU_ID_FETCHECO1:
			return IMXDPU_FETCHECO1_LOCKUNLOCK;
		case IMXDPU_ID_FETCHLAYER0:
			return IMXDPU_FETCHLAYER0_LOCKUNLOCK;
		case IMXDPU_ID_FETCHLAYER1:
			return IMXDPU_FETCHLAYER1_LOCKUNLOCK;
		case IMXDPU_ID_GAMMACOR4:
			return IMXDPU_GAMMACOR4_LOCKUNLOCK;
		case IMXDPU_ID_MATRIX4:
			return IMXDPU_MATRIX4_LOCKUNLOCK;
		case IMXDPU_ID_HSCALER4:
			return IMXDPU_HSCALER4_LOCKUNLOCK;
		case IMXDPU_ID_VSCALER4:
			return IMXDPU_VSCALER4_LOCKUNLOCK;
		case IMXDPU_ID_HISTOGRAM4:
			return IMXDPU_HISTOGRAM4_CONTROL;
		case IMXDPU_ID_GAMMACOR5:
			return IMXDPU_GAMMACOR5_LOCKUNLOCK;
		case IMXDPU_ID_MATRIX5:
			return IMXDPU_MATRIX5_LOCKUNLOCK;
		case IMXDPU_ID_HSCALER5:
			return IMXDPU_HSCALER5_LOCKUNLOCK;
		case IMXDPU_ID_VSCALER5:
			return IMXDPU_VSCALER5_LOCKUNLOCK;
		case IMXDPU_ID_HISTOGRAM5:
			return IMXDPU_HISTOGRAM5_CONTROL;
		case IMXDPU_ID_LAYERBLEND0:
			return IMXDPU_LAYERBLEND0_LOCKUNLOCK;
		case IMXDPU_ID_LAYERBLEND1:
			return IMXDPU_LAYERBLEND1_LOCKUNLOCK;
		case IMXDPU_ID_LAYERBLEND2:
			return IMXDPU_LAYERBLEND2_LOCKUNLOCK;
		case IMXDPU_ID_LAYERBLEND3:
			return IMXDPU_LAYERBLEND3_LOCKUNLOCK;
		case IMXDPU_ID_LAYERBLEND4:
			return IMXDPU_LAYERBLEND4_LOCKUNLOCK;
		case IMXDPU_ID_LAYERBLEND5:
			return IMXDPU_LAYERBLEND5_LOCKUNLOCK;
		case IMXDPU_ID_LAYERBLEND6:
			return IMXDPU_LAYERBLEND6_LOCKUNLOCK;
		case IMXDPU_ID_EXTSRC0:
			return IMXDPU_EXTSRC0_LOCKUNLOCK;
		case IMXDPU_ID_EXTSRC1:
			return IMXDPU_EXTSRC1_LOCKUNLOCK;
		case IMXDPU_ID_DISENGCFG:
			return IMXDPU_DISENGCFG_LOCKUNLOCK0;
		case IMXDPU_ID_FRAMEGEN0:
			return IMXDPU_FRAMEGEN0_LOCKUNLOCK;
		case IMXDPU_ID_MATRIX0:
			return IMXDPU_MATRIX0_LOCKUNLOCK;
		case IMXDPU_ID_GAMMACOR0:
			return IMXDPU_GAMMACOR0_LOCKUNLOCK;
		case IMXDPU_ID_DITHER0:
			return IMXDPU_DITHER0_LOCKUNLOCK;
		case IMXDPU_ID_TCON0:
			return IMXDPU_TCON0_LOCKUNLOCK;
		case IMXDPU_ID_SIG0:
			return IMXDPU_SIG0_LOCKUNLOCK;
		case IMXDPU_ID_FRAMEGEN1:
			return IMXDPU_FRAMEGEN1_LOCKUNLOCK;
		case IMXDPU_ID_MATRIX1:
			return IMXDPU_MATRIX1_LOCKUNLOCK;
		case IMXDPU_ID_GAMMACOR1:
			return IMXDPU_GAMMACOR1_LOCKUNLOCK;
		case IMXDPU_ID_DITHER1:
			return IMXDPU_DITHER1_LOCKUNLOCK;
		case IMXDPU_ID_TCON1:
			return IMXDPU_TCON1_LOCKUNLOCK;
		case IMXDPU_ID_SIG1:
			return IMXDPU_SIG1_LOCKUNLOCK;
		case IMXDPU_ID_FRAMECAP4:
			return IMXDPU_FRAMECAP4_LOCKUNLOCK;
		case IMXDPU_ID_FRAMECAP5:
			return IMXDPU_FRAMECAP5_LOCKUNLOCK;

		default:
			return IMXDPU_OFFSET_INVALID;
	}
}

/*!
 * Returns the start address offset for the dynamic configuraiton for
 * a given block ID
 *
 * @param	block id
 *
 * @return 	This function returns the address offset if the block id
 *		matches a valid block. Otherwise, IMXDPU_OFFSET_INVALID
 *		is returned.
 */
uint32_t id2dynamicoffset(imxdpu_id_t block_id)
{
	switch (block_id) {
		case IMXDPU_ID_FETCHDECODE9:
			return IMXDPU_PIXENGCFG_FETCHDECODE9_DYNAMIC;
		case IMXDPU_ID_FETCHPERSP9:
			return IMXDPU_PIXENGCFG_FETCHPERSP9_DYNAMIC;
		case IMXDPU_ID_ROP9:
			return IMXDPU_PIXENGCFG_ROP9_DYNAMIC;
		case IMXDPU_ID_CLUT9:
			return IMXDPU_PIXENGCFG_CLUT9_DYNAMIC;
		case IMXDPU_ID_MATRIX9:
			return IMXDPU_PIXENGCFG_MATRIX9_DYNAMIC;
		case IMXDPU_ID_HSCALER9:
			return IMXDPU_PIXENGCFG_HSCALER9_DYNAMIC;
		case IMXDPU_ID_VSCALER9:
			return IMXDPU_PIXENGCFG_VSCALER9_DYNAMIC;
		case IMXDPU_ID_FILTER9:
			return IMXDPU_PIXENGCFG_FILTER9_DYNAMIC;
		case IMXDPU_ID_BLITBLEND9:
			return IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC;
		case IMXDPU_ID_STORE9:
			return IMXDPU_PIXENGCFG_STORE9_DYNAMIC;
		case IMXDPU_ID_EXTDST0:
			return IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC;
		case IMXDPU_ID_EXTDST4:
			return IMXDPU_PIXENGCFG_EXTDST4_DYNAMIC;
		case IMXDPU_ID_EXTDST1:
			return IMXDPU_PIXENGCFG_EXTDST1_DYNAMIC;
		case IMXDPU_ID_EXTDST5:
			return IMXDPU_PIXENGCFG_EXTDST5_DYNAMIC;
		case IMXDPU_ID_STORE4:
			return IMXDPU_PIXENGCFG_STORE4_DYNAMIC;
		case IMXDPU_ID_STORE5:
			return IMXDPU_PIXENGCFG_STORE5_DYNAMIC;
		case IMXDPU_ID_FETCHDECODE2:
			return IMXDPU_PIXENGCFG_FETCHDECODE2_DYNAMIC;
		case IMXDPU_ID_FETCHDECODE3:
			return IMXDPU_PIXENGCFG_FETCHDECODE3_DYNAMIC;
		case IMXDPU_ID_FETCHWARP2:
			return IMXDPU_PIXENGCFG_FETCHWARP2_DYNAMIC;
		case IMXDPU_ID_FETCHDECODE0:
			return IMXDPU_PIXENGCFG_FETCHDECODE0_DYNAMIC;
		case IMXDPU_ID_FETCHDECODE1:
			return IMXDPU_PIXENGCFG_FETCHDECODE1_DYNAMIC;
		case IMXDPU_ID_GAMMACOR4:
			return IMXDPU_PIXENGCFG_GAMMACOR4_DYNAMIC;
		case IMXDPU_ID_MATRIX4:
			return IMXDPU_PIXENGCFG_MATRIX4_DYNAMIC;
		case IMXDPU_ID_HSCALER4:
			return IMXDPU_PIXENGCFG_HSCALER4_DYNAMIC;
		case IMXDPU_ID_VSCALER4:
			return IMXDPU_PIXENGCFG_VSCALER4_DYNAMIC;
		case IMXDPU_ID_HISTOGRAM4:
			return IMXDPU_PIXENGCFG_HISTOGRAM4_DYNAMIC;
		case IMXDPU_ID_GAMMACOR5:
			return IMXDPU_PIXENGCFG_GAMMACOR5_DYNAMIC;
		case IMXDPU_ID_MATRIX5:
			return IMXDPU_PIXENGCFG_MATRIX5_DYNAMIC;
		case IMXDPU_ID_HSCALER5:
			return IMXDPU_PIXENGCFG_HSCALER5_DYNAMIC;
		case IMXDPU_ID_VSCALER5:
			return IMXDPU_PIXENGCFG_VSCALER5_DYNAMIC;
		case IMXDPU_ID_HISTOGRAM5:
			return IMXDPU_PIXENGCFG_HISTOGRAM5_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND0:
			return IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND1:
			return IMXDPU_PIXENGCFG_LAYERBLEND1_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND2:
			return IMXDPU_PIXENGCFG_LAYERBLEND2_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND3:
			return IMXDPU_PIXENGCFG_LAYERBLEND3_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND4:
			return IMXDPU_PIXENGCFG_LAYERBLEND4_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND5:
			return IMXDPU_PIXENGCFG_LAYERBLEND5_DYNAMIC;
		case IMXDPU_ID_LAYERBLEND6:
			return IMXDPU_PIXENGCFG_LAYERBLEND6_DYNAMIC;
		default:
			return IMXDPU_OFFSET_INVALID;
	}
}

/*!
 * Returns the start address offset for a given shadow index
 *
 * @param	block id
 *
 * @return 	This function returns the address offset if the shadow
 *		index matches a valid block. Otherwise, IMXDPU_OFFSET_INVALID
 *		is returned.
 */
imxdpu_chan_t shadowindex2channel(imxdpu_shadow_load_index_t shadow_index)
{
	switch (shadow_index) {
		case IMXDPU_SHDLD_IDX_CHAN_00:
			return IMXDPU_CHAN_00;
		case IMXDPU_SHDLD_IDX_CHAN_01:
			return IMXDPU_CHAN_01;
		case IMXDPU_SHDLD_IDX_CHAN_02:
			return IMXDPU_CHAN_02;
		case IMXDPU_SHDLD_IDX_CHAN_03:
			return IMXDPU_CHAN_03;
		case IMXDPU_SHDLD_IDX_CHAN_04:
			return IMXDPU_CHAN_04;
		case IMXDPU_SHDLD_IDX_CHAN_05:
			return IMXDPU_CHAN_05;
		case IMXDPU_SHDLD_IDX_CHAN_06:
			return IMXDPU_CHAN_06;
		case IMXDPU_SHDLD_IDX_CHAN_07:
			return IMXDPU_CHAN_07;
		case IMXDPU_SHDLD_IDX_CHAN_08:
			return IMXDPU_CHAN_08;
		case IMXDPU_SHDLD_IDX_CHAN_09:
			return IMXDPU_CHAN_09;
		case IMXDPU_SHDLD_IDX_CHAN_10:
			return IMXDPU_CHAN_10;
		case IMXDPU_SHDLD_IDX_CHAN_11:
			return IMXDPU_CHAN_11;
		case IMXDPU_SHDLD_IDX_CHAN_12:
			return IMXDPU_CHAN_12;
		case IMXDPU_SHDLD_IDX_CHAN_13:
			return IMXDPU_CHAN_13;
		case IMXDPU_SHDLD_IDX_CHAN_14:
			return IMXDPU_CHAN_14;
		case IMXDPU_SHDLD_IDX_CHAN_15:
			return IMXDPU_CHAN_15;
		case IMXDPU_SHDLD_IDX_CHAN_16:
			return IMXDPU_CHAN_16;
		case IMXDPU_SHDLD_IDX_CHAN_17:
			return IMXDPU_CHAN_17;
		case IMXDPU_SHDLD_IDX_CHAN_18:
			return IMXDPU_CHAN_18;
		case IMXDPU_SHDLD_IDX_CHAN_19:
			return IMXDPU_CHAN_19;
		case IMXDPU_SHDLD_IDX_CHAN_20:
			return IMXDPU_CHAN_20;
		case IMXDPU_SHDLD_IDX_CHAN_21:
			return IMXDPU_CHAN_21;
		case IMXDPU_SHDLD_IDX_CHAN_22:
			return IMXDPU_CHAN_22;
		case IMXDPU_SHDLD_IDX_CHAN_23:
			return IMXDPU_CHAN_23;
		case IMXDPU_SHDLD_IDX_CHAN_24:
			return IMXDPU_CHAN_24;
		case IMXDPU_SHDLD_IDX_CHAN_25:
			return IMXDPU_CHAN_25;
		case IMXDPU_SHDLD_IDX_CHAN_26:
			return IMXDPU_CHAN_26;
		case IMXDPU_SHDLD_IDX_CHAN_27:
			return IMXDPU_CHAN_27;
		case IMXDPU_SHDLD_IDX_CHAN_28:
			return IMXDPU_CHAN_28;
		case IMXDPU_SHDLD_IDX_CHAN_29:
			return IMXDPU_CHAN_29;
		case IMXDPU_SHDLD_IDX_CHAN_30:
			return IMXDPU_CHAN_30;
		default:
			return IMXDPU_CHANNEL_INVALID;
	}
}

/*!
 * This function returns the pointer to the imxdpu structutre
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return 	This function returns the pointer to the imxdpu structutre
 * 		return a NULL pointer for a failure.
 */
struct imxdpu_soc* imxdpu_get_soc(int8_t imxdpu_id)
{
	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return NULL;
	}
	return &(imxdpu_array[imxdpu_id]);
}

/*!
 * This function enables the interrupt for the specified interrupt line.
 * The interrupt lines are defined in imxdpu_events.h.
 *
 * @param	imxdpu		imxdpu instance
 * @param       irq             Interrupt line to enable interrupt for.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_enable_irq(int8_t imxdpu_id, uint32_t irq)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (irq < IMXDPU_INTERRUPT_MAX) {
		if (irq < 32) {
			imxdpu->enabled_int[0] |= INTSTAT0_BIT(irq);
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTENABLE0,
					 imxdpu->enabled_int[0]);
		} else if (irq < 64) {
			imxdpu->enabled_int[1] |= INTSTAT1_BIT(irq);
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTENABLE1,
					 imxdpu->enabled_int[1]);
		} else {
			imxdpu->enabled_int[2] |= INTSTAT2_BIT(irq);
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTENABLE2,
					 imxdpu->enabled_int[2]);
		}
	} else return -EINVAL;

	return ret;
}

/*!
 * This function disables the interrupt for the specified interrupt line.
 * The interrupt lines are defined in imxdpu_events.h.
 *
 * @param	imxdpu		imxdpu instance
 * @param       irq             Interrupt line to disable interrupt for.
 *
 */
int imxdpu_disable_irq(int8_t imxdpu_id, uint32_t irq)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (irq < IMXDPU_INTERRUPT_MAX) {
		if (irq < 32) {
			imxdpu->enabled_int[0] &= ~INTSTAT0_BIT(irq);
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTENABLE0,
					 imxdpu->enabled_int[0]);
		} else if (irq < 64) {
			imxdpu->enabled_int[1] &= ~INTSTAT1_BIT(irq);
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTENABLE1,
					 imxdpu->enabled_int[1]);
		} else {
			imxdpu->enabled_int[2] &= ~INTSTAT2_BIT(irq);
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTENABLE2,
					 imxdpu->enabled_int[2]);
		}
	} else return -EINVAL;

	return ret;
}

/*!
 * This function clears all interrupts.
 *
 * @param	imxdpu		imxdpu instance
 *
 */
int imxdpu_clear_all_irqs(int8_t imxdpu_id)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_USERINTERRUPTCLEAR0,
			 IMXDPU_COMCTRL_USERINTERRUPTCLEAR0_USERINTERRUPTCLEAR0_MASK);
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_USERINTERRUPTCLEAR1,
			 IMXDPU_COMCTRL_USERINTERRUPTCLEAR1_USERINTERRUPTCLEAR1_MASK);
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_USERINTERRUPTCLEAR2,
			 IMXDPU_COMCTRL_USERINTERRUPTCLEAR2_USERINTERRUPTCLEAR2_MASK);
#if 1
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_INTERRUPTCLEAR0,
			 IMXDPU_COMCTRL_INTERRUPTCLEAR0_INTERRUPTCLEAR0_MASK);
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_INTERRUPTCLEAR1,
			 IMXDPU_COMCTRL_INTERRUPTCLEAR1_INTERRUPTCLEAR1_MASK);
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_INTERRUPTCLEAR2,
			 IMXDPU_COMCTRL_INTERRUPTCLEAR2_INTERRUPTCLEAR2_MASK);
#endif
	return ret;
}

/*!
 * This function disables all interrupts.
 *
 * @param	imxdpu		imxdpu instance
 *
 */
int imxdpu_disable_all_irqs(int8_t imxdpu_id)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu_write_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE0, 0);
	imxdpu_write_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE1, 0);
	imxdpu_write_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE2, 0);

#if 1
	imxdpu_write_irq(imxdpu, IMXDPU_COMCTRL_INTERRUPTENABLE0, 0);
	imxdpu_write_irq(imxdpu, IMXDPU_COMCTRL_INTERRUPTENABLE1, 0);
	imxdpu_write_irq(imxdpu, IMXDPU_COMCTRL_INTERRUPTENABLE2, 0);
#endif

	imxdpu->enabled_int[0] = 0;
	imxdpu->enabled_int[1] = 0;
	imxdpu->enabled_int[2] = 0;

	return ret;
}

/*!
 * This function clears the interrupt for the specified interrupt line.
 * The interrupt lines are defined in ipu_irq_line enum.
 *
 * @param	imxdpu 		imxdpu instance
 * @param       irq             Interrupt line to clear interrupt for.
 *
 */
int imxdpu_clear_irq(int8_t imxdpu_id, uint32_t irq)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (irq < IMXDPU_INTERRUPT_MAX) {
		if (irq < 32) {
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTCLEAR0,
					 1U << irq);
		}
		if (irq < 64) {
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTCLEAR1,
					 1U << (irq - 32));
		} else {
			imxdpu_write_irq(imxdpu,
					 IMXDPU_COMCTRL_USERINTERRUPTCLEAR2,
					 1U << (irq - 64));
		}
	} else return -EINVAL;

	return ret;
}

/*!
 * This function initializes the imxdpu interrupts
 *
 * @param	imxdpu 		imxdpu instance
 *
 */
int imxdpu_init_irqs(int8_t imxdpu_id)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu_disable_all_irqs(imxdpu_id);
	imxdpu_clear_all_irqs(imxdpu_id);

	/* Set all irq to user mode */
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_USERINTERRUPTMASK0,
			 IMXDPU_COMCTRL_USERINTERRUPTMASK0_USERINTERRUPTMASK0_MASK);
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_USERINTERRUPTMASK1,
			 IMXDPU_COMCTRL_USERINTERRUPTMASK1_USERINTERRUPTMASK1_MASK);
	imxdpu_write_irq(imxdpu,
			 IMXDPU_COMCTRL_USERINTERRUPTMASK2,
			 IMXDPU_COMCTRL_USERINTERRUPTMASK2_USERINTERRUPTMASK2_MASK);

	/* enable needed interupts */
	imxdpu_enable_irq(imxdpu_id, IMXDPU_EXTDST0_SHDLOAD_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_EXTDST1_SHDLOAD_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ);

	//imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE4_SHDLOAD_IRQ);
	//imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE5_SHDLOAD_IRQ);
	//imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE4_SEQCOMPLETE_IRQ);
	//imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE5_SEQCOMPLETE_IRQ);
	//imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE4_FRAMECOMPLETE_IRQ);
	//imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE5_FRAMECOMPLETE_IRQ);

	imxdpu_enable_irq(imxdpu_id, IMXDPU_FRAMEGEN0_INT0_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_FRAMEGEN1_INT0_IRQ);

	imxdpu_enable_irq(imxdpu_id, IMXDPU_COMCTRL_SW0_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_COMCTRL_SW1_IRQ);

	imxdpu_enable_irq(imxdpu_id, IMXDPU_DISENGCFG_SHDLOAD0_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_DISENGCFG_SHDLOAD1_IRQ);

	IMXDPU_TRACE("%s() enabled_int[0] 0x%08x\n", __func__,
		     imxdpu->enabled_int[0]);
	IMXDPU_TRACE("%s() enabled_int[1] 0x%08x\n", __func__,
		     imxdpu->enabled_int[1]);
	IMXDPU_TRACE("%s() enabled_int[2] 0x%08x\n", __func__,
		     imxdpu->enabled_int[2]);

	return ret;
}

/*!
 * This function checks pending shadow loads
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_check_shadow_loads(int8_t imxdpu_id, int8_t disp)
{
	int ret = 0;
	uint32_t addr_extdst = IMXDPU_OFFSET_INVALID;   /* address for extdst */
	uint32_t extdst = 0;
	uint32_t extdst_stat = 0;
	uint32_t fgen = 1;
	uint32_t fgen_stat = 0;
	uint32_t sub = 0;
	uint32_t sub_stat = 0;
	uint32_t stat;

	int32_t i;

	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	stat = imxdpu_read_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTSTATUS0);
	if (disp == 0) {
		addr_extdst = IMXDPU_PIXENGCFG_EXTDST0_REQUEST;
		if (stat & IMXDPU_DISENGCFG_SHDLOAD0_IRQ) {
			fgen = 0;
		}
	} else if (disp == 1) {
		addr_extdst = IMXDPU_PIXENGCFG_EXTDST1_REQUEST;
		if (stat & IMXDPU_DISENGCFG_SHDLOAD1_IRQ) {
			fgen = 0;
		}
	} else return -EINVAL;

	sub |= (imxdpu_read(imxdpu, IMXDPU_FETCHLAYER0_TRIGGERENABLE)) & 0xff;
	sub |= (imxdpu_read(imxdpu, IMXDPU_FETCHLAYER1_TRIGGERENABLE) << 8) & 0xff00;
	sub |= (imxdpu_read(imxdpu, IMXDPU_FETCHWARP2_TRIGGERENABLE) << 16) & 0xff0000;
	extdst = imxdpu_read(imxdpu, addr_extdst);

	/* this loop may need to be optimized */
	for (i = 0; i < IMXDPU_SHDLD_IDX_CHAN_00; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.complete) {
			if (imxdpu->shadow_load_state[disp][i].state.trys > 0) {
				IMXDPU_TRACE_IRQ
					("shadow index complete after retry: index %d trys %d\n",
					 i,
					 imxdpu->shadow_load_state[disp][i].
					 state.trys);
			} else {
				IMXDPU_TRACE_IRQ("shadow index complete: index %d\n", i);
			}
			imxdpu->shadow_load_state[disp][i].word = 0;
		} else if (imxdpu->shadow_load_state[disp][i].state.processing) {
			if (i > IMXDPU_SHDLD_IDX_CONST1) {
				if (!(extdst & trigger_list[i].extdst) && !fgen) {
					//IMXDPU_TRACE_IRQ("Checking Fgen shadow request\n");
					imxdpu->shadow_load_state[disp][i].
						state.complete = 1;
				} else {
					extdst_stat |= trigger_list[i].extdst;
					fgen_stat |= 1 << i;
				}
			} else if (!(extdst & trigger_list[i].extdst)) {
				imxdpu->shadow_load_state[disp][i].
					state.complete = 1;
			} else {
				imxdpu->shadow_load_state[disp][i].state.trys++;
				extdst |= trigger_list[i].extdst;
				IMXDPU_TRACE_IRQ
					("shadow index retry: index %d trys %d\n",
					 i,
					 imxdpu->shadow_load_state[disp][i].
					 state.trys);
			}
		}
	}

	for (i = IMXDPU_SHDLD_IDX_CHAN_00; i < IMXDPU_SHDLD_IDX_MAX; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.complete) {

			if (imxdpu->shadow_load_state[disp][i].state.trys > 0) {
				IMXDPU_TRACE_IRQ
					("shadow index complete after retry: index %d trys %d\n",
					 i,
					 imxdpu->shadow_load_state[disp][i].
					 state.trys);
			} else {
				IMXDPU_TRACE_IRQ("shadow index complete: index %d\n", i);
			}
			imxdpu->shadow_load_state[disp][i].word = 0;
		} else if (imxdpu->shadow_load_state[disp][i].state.processing) {
			/* fetch layer and fetchwarp */
			if ((trigger_list[i].extdst != 0) &&
				(trigger_list[i].sub != 0)) {
				if (!(extdst & trigger_list[i].extdst) &&
					!(sub & trigger_list[i].sub)) {
					imxdpu->shadow_load_state[disp][i].
						state.complete = 1;
				} else {
					extdst_stat |= trigger_list[i].extdst;
					sub_stat |= trigger_list[i].sub;
				}
			} else if (!(extdst & trigger_list[i].extdst)) {
				imxdpu->shadow_load_state[disp][i].
					state.complete = 1;
			} else {
				imxdpu->shadow_load_state[disp][i].state.trys++;
				extdst_stat |= trigger_list[i].extdst;
				IMXDPU_TRACE_IRQ
					("shadow index retry: index %d trys %d\n",
					 i,
					 imxdpu->shadow_load_state[disp][i].
					 state.trys);
			}
		}
	}

	if ((extdst_stat == 0) && (sub_stat == 0) && (fgen_stat == 0)) {
		/* clear interrupt */
		IMXDPU_TRACE_IRQ("shadow requests are complete.\n");
	} else {
		IMXDPU_TRACE_IRQ
			("shadow requests are not complete: extdst 0x%08x, sub 0x%08x, fgen 0x%08x\n",
			 extdst, sub, fgen);
		IMXDPU_TRACE_IRQ
			("shadow requests are not complete: extdst_stat 0x%08x, sub_stat 0x%08x, fgen_stat 0x%08x\n",
			 extdst_stat, sub_stat, fgen_stat);
	}

	return ret;
}

/*!
 * This function starts pending shadow loads
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
static int imxdpu_disp_start_shadow_loads(int8_t imxdpu_id, int8_t disp)
{
	int ret = 0;
	uint32_t addr_extdst;   /* address for extdst */
	uint32_t addr_fgen; /* address for frame generator */
	uint32_t extdst = 0;
	uint32_t fgen = 0;
	uint32_t sub = 0;
	int32_t i;

	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (disp == 0) {
		addr_fgen = IMXDPU_FRAMEGEN0_FGSLR;
		addr_extdst = IMXDPU_PIXENGCFG_EXTDST0_REQUEST;

	} else if (disp == 1) {
		addr_fgen = IMXDPU_FRAMEGEN1_FGSLR;
		addr_extdst = IMXDPU_PIXENGCFG_EXTDST1_REQUEST;
	} else return -EINVAL;

	/* this loop may need to be optimized */
	for (i = 0; i < IMXDPU_SHDLD_IDX_CHAN_00; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.request &&
			(imxdpu->shadow_load_state[disp][i].state.processing == 0)) {
			imxdpu->shadow_load_state[disp][i].state.processing = 1;
			extdst |= trigger_list[i].extdst;
			/* only trigger frame generator for const frames */
			if (i >= IMXDPU_SHDLD_IDX_CONST0) {
				fgen |= 1;
			}
		}
	}
	for (i = IMXDPU_SHDLD_IDX_CHAN_00; i < IMXDPU_SHDLD_IDX_MAX; i++) {
		if (imxdpu->shadow_load_state[disp][i].state.request &&
			(imxdpu->shadow_load_state[disp][i].state.processing == 0)) {
			imxdpu->shadow_load_state[disp][i].state.processing = 1;
			/*todo: need a completion handler */
			extdst |= trigger_list[i].extdst;
			sub |= trigger_list[i].sub;
		}
	}

	if (sub) {
		IMXDPU_TRACE_IRQ("Fetch layer shadow request 0x%08x\n", sub);
		if (sub & 0xff) {   /* FETCHLAYER0 */
			imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_TRIGGERENABLE,
				     sub & 0xff);
		}
		if (sub & 0xff00) { /* FETCHLAYER1 */
			imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_TRIGGERENABLE,
				     (sub >> 8) & 0xff);
		}
		if (sub & 0xff0000) {   /* FETCHWARP2 */
			imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_TRIGGERENABLE,
				     (sub >> 16) & 0xff);
		}
	}

	if (extdst) {
		IMXDPU_TRACE_IRQ("Extdst shadow request  0x%08x\n", extdst);
		imxdpu_write(imxdpu, addr_extdst, extdst);
	}

	if (fgen) {
		IMXDPU_TRACE_IRQ("Fgen shadow request  0x%08x\n", fgen);
		imxdpu_write(imxdpu, addr_fgen, fgen);
	}

	return ret;
}

/*!
 * This function handles the VYNC interrupt for a display
 *
 * @param	imxdpu		imxdpu instance
 * @param	disp		display index
 *
 */
static void imxdpu_disp_vsync_handler(int8_t imxdpu_id, int8_t disp)
{
	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return;
	}

	if (!((disp == 0) || (disp == 1))) return;

	/* send notifications
	   shadow load finished
	 */

	imxdpu_disp_start_shadow_loads(imxdpu_id, disp);
	imxdpu_disp_update_fgen_status(imxdpu_id, disp);

#if 0	/* this is just for debug */
	if (((imxdpu_array->fgen_stats[0].frame_count +
	      imxdpu_array->fgen_stats[1].frame_count + 1) % 30) == 0) {
		imxdpu_disp_show_fgen_status(0);
		imxdpu_disp_show_fgen_status(1);
	}
#endif
	return;

}

/*!
 * This function calls a register handler for an interrupt
 *
 * @param	imxdpu		imxdpu instance
 * @param	irq		interrupt line
 *
 */
static void imxdpu_handle_registered_irq(int8_t imxdpu_id, int8_t irq)
{
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if ((irq < 0) || (irq >= IMXDPU_INTERRUPT_MAX)) return;

	if (imxdpu->irq_list[irq].handler == NULL) return;

	imxdpu->irq_list[irq].handler(irq, imxdpu->irq_list[irq].data);

	return;

}

/* todo: this irq handler assumes all irq are ORed together.
     The irqs may be grouped so this function can be
     optimized if that is the case*/
/*!
 * This function processes all IRQs for the IMXDPU
 *
 * @param	data	pointer to the imxdpu structure
 *
 */
int imxdpu_handle_irq(int32_t imxdpu_id)
{
	uint32_t int_stat[3];
	uint32_t int_temp[3];
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		IMXDPU_TRACE_IRQ("%s(): invalid imxdpu_id\n", __func__);
		return IMXDPU_FALSE;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu->irq_count++;

	/* Get and clear interrupt status */
	int_temp[0] =
		      imxdpu_read_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTSTATUS0);
	int_stat[0] = imxdpu->enabled_int[0] & int_temp[0];
	int_temp[1] =
		      imxdpu_read_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTSTATUS1);
	int_stat[1] = imxdpu->enabled_int[1] & int_temp[1];

#ifdef IMXDPU_ENABLE_INTSTAT2
	/* Enable this  (IMXDPU_ENABLE_INTSTAT2) if intstat2 interrupts
	   are needed */
	int_temp[2] =
		      imxdpu_read_irq(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTSTATUS2);
	int_stat[2] = imxdpu->enabled_int[2] & int_temp[2];
#endif

	/* No interrupts are pending */
	if ((int_temp[0] == 0) && (int_temp[1] == 0)
#ifdef IMXDPU_ENABLE_INTSTAT2
		&& (int_temp[2] == 0)
#endif
		) {
#ifdef IMXDPU_FPGA_BUILD

		//emergency_sync();
		//emergency_restart();
		//printk("rebooting    ");
		//printk("imxdpu_handle_irq(): int stats 0x%08x 0x%08x 0x%08x\n", int_temp[0],int_temp[1],int_temp[2]);

		/* Just ignore interrupt */
		return IMXDPU_FALSE;
#endif
	}
	//printk("imxdpu_handle_irq(): int stats 0x%08x 0x%08x 0x%08x\n", int_temp[0],int_temp[1],int_temp[2]);
	/* No enabled interrupts are pending */
	if ((int_stat[0] == 0) && (int_stat[1] == 0)
#ifdef IMXDPU_ENABLE_INTSTAT2
		&& (int_stat[2] == 0)
#endif
		) {
		IMXDPU_TRACE_IRQ
			("Error: No enabled interrupts, 0x%08x 0x%08x\n",
			 int_temp[0] & ~imxdpu->enabled_int[0],
			 int_temp[1] & ~imxdpu->enabled_int[1]);
		return IMXDPU_FALSE;
	}

	/* Clear the enabled interrupts */
	if (int_stat[0]) {
		imxdpu_write_irq(imxdpu,
				 IMXDPU_COMCTRL_USERINTERRUPTCLEAR0,
				 int_temp[0]);
	}
	if (int_stat[1]) {
		imxdpu_write_irq(imxdpu,
				 IMXDPU_COMCTRL_USERINTERRUPTCLEAR1,
				 int_temp[1]);
	}
#ifdef IMXDPU_ENABLE_INTSTAT2
	if (int_stat[2]) {
		imxdpu_write_irq(imxdpu,
				 IMXDPU_COMCTRL_USERINTERRUPTCLEAR2,
				 int_temp[2]);
	}
#endif

#ifdef IMXDPU_ENABLE_INTSTAT2
	if (int_stat[1] != 0) {
		/* add int_stat[2] if needed */
	}
#endif
	/* now handle the interrupts that are pending */
	if (int_stat[0] != 0) {
		if (int_stat[0] & 0xff) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_STORE9_SHDLOAD_IRQ)) {
				IMXDPU_TRACE_IRQ
				    ("IMXDPU_STORE9_SHDLOAD_IRQ irq\n");
				imxdpu_be_irq_handler(imxdpu_id,
							     IMXDPU_STORE9_SHDLOAD_IRQ);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE9_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_STORE9_FRAMECOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ
				    ("IMXDPU_STORE9_FRAMECOMPLETE_IRQ irq\n");
				imxdpu_be_irq_handler(imxdpu_id,
							     IMXDPU_STORE9_FRAMECOMPLETE_IRQ);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE9_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_STORE9_SEQCOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ
				    ("IMXDPU_STORE9_SEQCOMPLETE_IRQ irq\n");
				imxdpu_be_irq_handler(imxdpu_id,
							     IMXDPU_STORE9_SEQCOMPLETE_IRQ);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE9_SEQCOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_EXTDST0_SHDLOAD_IRQ)) {
				IMXDPU_TRACE_IRQ
					("IMXDPU_EXTDST0_SHDLOAD_IRQ irq\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_EXTDST0_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ
					("IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ\n");
				/* todo: move */
				imxdpu_disp_check_shadow_loads(imxdpu_id, 0);

				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ);
			}
		}
		if (int_stat[0] & 0xff00) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_EXTDST1_SHDLOAD_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_EXTDST1_SHDLOAD_IRQ irq\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_EXTDST1_SHDLOAD_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(
					IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ\n");
				/* todo: move */
				imxdpu_disp_check_shadow_loads(imxdpu_id, 1);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_STORE4_SHDLOAD_IRQ)) {
				IMXDPU_TRACE_IRQ("IMXDPU_STORE4_SHDLOAD_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE4_SHDLOAD_IRQ);
			}
		}
		if (int_stat[0] & 0xff0000) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_STORE4_FRAMECOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_STORE4_FRAMECOMPLETE_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE4_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_STORE4_SEQCOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_STORE4_SEQCOMPLETE_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE4_SEQCOMPLETE_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_HISTOGRAM4_VALID_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_HISTOGRAM4_VALID_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_HISTOGRAM4_VALID_IRQ);
			}
		}
		if (int_stat[0] & 0xff000000) {
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_HISTOGRAM5_VALID_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_HISTOGRAM5_VALID_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_HISTOGRAM5_VALID_IRQ);
			}
			if (int_stat[0] &
				INTSTAT0_BIT(IMXDPU_FRAMEGEN0_INT0_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_FRAMEGEN0_INT0_IRQ\n");
				imxdpu_disp_vsync_handler(imxdpu_id, 0);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_FRAMEGEN0_INT0_IRQ);
			}
		}
	}

	if (int_stat[1] != 0) {
		if (int_stat[1] & 0xff) {
			if (int_stat[1] &
				INTSTAT0_BIT(IMXDPU_STORE9_SHDLOAD_IRQ)) {
				IMXDPU_TRACE_IRQ
				    ("IMXDPU_STORE9_SHDLOAD_IRQ irq\n");
				imxdpu_be_irq_handler(imxdpu_id,
							     IMXDPU_STORE9_SHDLOAD_IRQ);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE9_SHDLOAD_IRQ);
			}
			if (int_stat[1] &
				INTSTAT0_BIT(IMXDPU_STORE9_FRAMECOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ
				    ("IMXDPU_STORE9_FRAMECOMPLETE_IRQ irq\n");
				imxdpu_be_irq_handler(imxdpu_id,
							     IMXDPU_STORE9_FRAMECOMPLETE_IRQ);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE9_FRAMECOMPLETE_IRQ);
			}
			if (int_stat[1] &
				INTSTAT0_BIT(IMXDPU_STORE9_SEQCOMPLETE_IRQ)) {
				IMXDPU_TRACE_IRQ
				    ("IMXDPU_STORE9_SEQCOMPLETE_IRQ irq\n");
				imxdpu_be_irq_handler(imxdpu_id,
							     IMXDPU_STORE9_SEQCOMPLETE_IRQ);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_STORE9_SEQCOMPLETE_IRQ);
			}
		}
		if (int_stat[1] & 0xff00) {
			if (int_stat[1] &
				INTSTAT1_BIT(IMXDPU_FRAMEGEN1_INT0_IRQ)) {
				IMXDPU_TRACE_IRQ(
					"IMXDPU_FRAMEGEN1_INT0_IRQ\n");
				imxdpu_disp_vsync_handler(imxdpu_id, 1);
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_FRAMEGEN1_INT0_IRQ);
			}
		}
		if (int_stat[0] & 0xff0000) {
			if (int_stat[0] &
				INTSTAT1_BIT(IMXDPU_COMCTRL_SW0_IRQ)) {
				IMXDPU_TRACE_IRQ("IMXDPU_COMCTRL_SW0_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_COMCTRL_SW0_IRQ);
			}
			if (int_stat[1] & INTSTAT1_BIT(IMXDPU_COMCTRL_SW2_IRQ)) {
				IMXDPU_TRACE_IRQ("IMXDPU_COMCTRL_SW2_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_COMCTRL_SW2_IRQ);
			}
			if (int_stat[1] & INTSTAT1_BIT(IMXDPU_COMCTRL_SW3_IRQ)) {
				IMXDPU_TRACE_IRQ("IMXDPU_COMCTRL_SW3_IRQ\n");
				imxdpu_handle_registered_irq(imxdpu_id,
							     IMXDPU_COMCTRL_SW3_IRQ);
			}
			/* Reserved for command sequencer debug */
			//if (int_stat[0] &
			// INTSTAT1_BIT(IMXDPU_COMCTRL_SW1_IRQ)) {
			//IMXDPU_TRACE_IRQ("IMXDPU_COMCTRL_SW1_IRQ\n");
			//imxdpu_handle_registered_irq(imxdpu_id,
			// IMXDPU_COMCTRL_SW1_IRQ);
			//}
		}
	}

	return IMXDPU_TRUE;
}

/*!
 * This function registers an interrupt handler function for the specified
 * irq line. The interrupt lines are defined in imxdpu_events.h
 *
 * @param	imxdpu		imxdpu instance
 * @param       irq             Interrupt line to get status for.
 *
 * @param       handler         Input parameter for address of the handler
 *                              function.
 *
 * @param       irq_flags       Flags for interrupt mode. Currently not used.
 *
 * @param       devname         Input parameter for string name of driver
 *                              registering the handler.
 *
 * @param       data            Input parameter for pointer of data to be
 *                              passed to the handler.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_request_irq(int8_t imxdpu_id,
		       uint32_t irq,
		       int (*handler)(int, void *),
		       uint32_t irq_flags, const char *devname, void *data)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (imxdpu->irq_list[irq].handler != NULL) {
		pr_err("handler already installed on irq %d\n", irq);
		ret = -EINVAL;
		goto out;
	}

	imxdpu->irq_list[irq].handler = handler;
	imxdpu->irq_list[irq].flags = irq_flags;
	imxdpu->irq_list[irq].data = data;
	imxdpu->irq_list[irq].name = devname;

	/* Clear and enable the IRQ */
	imxdpu_clear_irq(imxdpu_id, irq);
	imxdpu_enable_irq(imxdpu_id, irq);
out:
	return ret;
}

/*!
 * This function unregisters an interrupt handler for the specified interrupt
 * line. The interrupt lines are defined in imxdpu_events.h
 *
 * @param 	imxdpu		imxdpu instance
 * @param       irq             Interrupt line to get status for.
 *
 * @param       data          Input parameter for pointer of data to be passed
 *                              to the handler. This must match value passed to
 *                              ipu_request_irq().
 *
 */
int imxdpu_free_irq(int8_t imxdpu_id, uint32_t irq, void *data)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu_disable_irq(imxdpu_id, irq);
	imxdpu_clear_irq(imxdpu_id, irq);
	if (imxdpu->irq_list[irq].data == data) memset(&imxdpu->irq_list[irq], 0,
						       sizeof(imxdpu->irq_list[irq]));

	return ret;
}

/*!
 * This function un-initializes the imxdpu interrupts
 *
 * @param	imxdpu 		imxdpu instance
 *
 */
int imxdpu_uninit_interrupts(int8_t imxdpu_id)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu->enabled_int[0] = 0;
	imxdpu->enabled_int[1] = 0;
	imxdpu->enabled_int[2] = 0;

	imxdpu_clear_all_irqs(imxdpu_id);

	/* Set all interrupt to user mode */
	imxdpu_write(imxdpu,
		     IMXDPU_COMCTRL_USERINTERRUPTMASK0,
		     IMXDPU_COMCTRL_USERINTERRUPTMASK0_USERINTERRUPTMASK0_MASK);
	imxdpu_write(imxdpu,
		     IMXDPU_COMCTRL_USERINTERRUPTMASK1,
		     IMXDPU_COMCTRL_USERINTERRUPTMASK1_USERINTERRUPTMASK1_MASK);
	imxdpu_write(imxdpu,
		     IMXDPU_COMCTRL_USERINTERRUPTMASK2,
		     IMXDPU_COMCTRL_USERINTERRUPTMASK2_USERINTERRUPTMASK2_MASK);

	/* Set all interrupts to user mode. this will to change to
	   enable panic mode */
	imxdpu_write(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE0, 0);
	imxdpu_write(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE1, 0);
	imxdpu_write(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE2, 0);

	/* enable needed interupts */
	return ret;
}

/*!
 * This function initializes the imxdpu and the required data structures
 *
 * @param	imxdpu_id	id of the diplay unit
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
/* todo: replace with probe function or call from probe
   use device tree as needed */
int imxdpu_init(int8_t imxdpu_id, void __iomem *imxdpu_base)
{
	int ret = 0;
	int i;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	/* todo: add resource mapping for xrdc, layers, blit, display, ... */

	/* imxdpu_id starts from 0 */
	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}

	/* Map the channels to display streams
	   todo:
	   make this mapping dynamic
	   add channel features
	   map capture channels
	 */
	for (i = IMXDPU_CHAN_IDX_IN_FIRST; i < IMXDPU_CHAN_IDX_MAX; i++) {
		if (i <= IMXDPU_CHAN_IDX_17) imxdpu_array[imxdpu_id].chan_data[i].disp_id = 0;
		else if (i < IMXDPU_CHAN_IDX_IN_MAX) imxdpu_array[imxdpu_id].chan_data[i].disp_id = 1;
		else if (i < IMXDPU_CHAN_IDX_OUT_FIRST) imxdpu_array[imxdpu_id].chan_data[i].disp_id = 0;
		else if (i < IMXDPU_CHAN_IDX_OUT_MAX) imxdpu_array[imxdpu_id].chan_data[i].disp_id = 1;
		else imxdpu_array[imxdpu_id].chan_data[i].disp_id = 0;
	}

	imxdpu = &imxdpu_array[imxdpu_id];
	imxdpu->irq_count = 0;

	imxdpu->base = imxdpu_base;
	if (!imxdpu->base) {
		pr_err("%s(): invaild IMXDPU physical base \n", __func__);
		return -ENOMEM;
	}

	/* todo: may need to check resource allocaiton/ownership for these */
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY0,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY0_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY1,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY1_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY2,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY2_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY3,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY3_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY4,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY4_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY5,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY5_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY6,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY6_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_LAYERPROPERTY7,
		     IMXDPU_FETCHLAYER0_LAYERPROPERTY7_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_TRIGGERENABLE,
		     IMXDPU_FETCHLAYER0_TRIGGERENABLE_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY0,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY0_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY1,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY1_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY2,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY2_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY3,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY3_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY4,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY4_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY5,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY5_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY6,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY6_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_LAYERPROPERTY7,
		     IMXDPU_FETCHLAYER1_LAYERPROPERTY7_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_TRIGGERENABLE,
		     IMXDPU_FETCHLAYER1_TRIGGERENABLE_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY0,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY0_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY1,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY1_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY2,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY2_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY3,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY3_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY4,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY4_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY5,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY5_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY6,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY6_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_LAYERPROPERTY7,
		     IMXDPU_FETCHWARP2_LAYERPROPERTY7_RESET_VALUE);
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_TRIGGERENABLE,
		     IMXDPU_FETCHWARP2_TRIGGERENABLE_RESET_VALUE);

	/* Initial StaticControl configuration - reset values */
	/* IMXDPU_FETCHDECODE9_STATICCONTROL  */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE9_STATICCONTROL,
		     IMXDPU_FETCHDECODE9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHPERSP9_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHPERSP9_STATICCONTROL,
		     IMXDPU_FETCHPERSP9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHECO9_STATICCONTROL     */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO9_STATICCONTROL,
		     IMXDPU_FETCHECO9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_ROP9_STATICCONTROL          */
	imxdpu_write(imxdpu, IMXDPU_ROP9_STATICCONTROL,
		     IMXDPU_ROP9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_CLUT9_STATICCONTROL         */
	imxdpu_write(imxdpu, IMXDPU_CLUT9_STATICCONTROL,
		     IMXDPU_CLUT9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_MATRIX9_STATICCONTROL       */
	imxdpu_write(imxdpu, IMXDPU_MATRIX9_STATICCONTROL,
		     IMXDPU_MATRIX9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_HSCALER9_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_HSCALER9_STATICCONTROL,
		     IMXDPU_HSCALER9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_VSCALER9_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_VSCALER9_STATICCONTROL,
		     IMXDPU_VSCALER9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FILTER9_STATICCONTROL       */
	imxdpu_write(imxdpu, IMXDPU_FILTER9_STATICCONTROL,
		     IMXDPU_FILTER9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_BLITBLEND9_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_BLITBLEND9_STATICCONTROL,
		     IMXDPU_BLITBLEND9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_STORE9_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_STORE9_STATICCONTROL,
		     IMXDPU_STORE9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_CONSTFRAME0_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_CONSTFRAME0_STATICCONTROL,
		     IMXDPU_CONSTFRAME0_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_EXTDST0_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTDST0_STATICCONTROL,
		     IMXDPU_EXTDST0_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_EXTDST4_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTDST4_STATICCONTROL,
		     IMXDPU_EXTDST4_STATICCONTROL_RESET_VALUE);

	/* todo: IMXDPU_CONSTFRAME4_STATICCONTROL    */

	/* IMXDPU_CONSTFRAME1_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_CONSTFRAME1_STATICCONTROL,
		     IMXDPU_CONSTFRAME1_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_EXTDST1_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTDST1_STATICCONTROL,
		     IMXDPU_EXTDST1_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_EXTDST5_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTDST5_STATICCONTROL,
		     IMXDPU_EXTDST5_STATICCONTROL_RESET_VALUE);

	/* todo: IMXDPU_CONSTFRAME5_STATICCONTROL    */

	/* IMXDPU_EXTSRC4_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTSRC4_STATICCONTROL,
		     IMXDPU_EXTSRC4_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_STORE4_STATICCONTROL         */
	imxdpu_write(imxdpu, IMXDPU_STORE4_STATICCONTROL,
		     IMXDPU_STORE4_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_EXTSRC5_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTSRC5_STATICCONTROL,
		     IMXDPU_EXTSRC5_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_STORE5_STATICCONTROL         */
	imxdpu_write(imxdpu, IMXDPU_STORE5_STATICCONTROL,
		     IMXDPU_STORE5_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHDECODE2_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE2_STATICCONTROL,
		     IMXDPU_FETCHDECODE2_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHDECODE3_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE3_STATICCONTROL,
		     IMXDPU_FETCHDECODE3_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHWARP2_STATICCONTROL     */
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_STATICCONTROL,
		     IMXDPU_FETCHWARP2_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHECO2_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO9_STATICCONTROL,
		     IMXDPU_FETCHECO9_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHDECODE0_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE0_STATICCONTROL,
		     IMXDPU_FETCHDECODE0_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHECO0_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO0_STATICCONTROL,
		     IMXDPU_FETCHECO0_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHDECODE1_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE1_STATICCONTROL,
		     IMXDPU_FETCHDECODE1_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_FETCHECO1_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO1_STATICCONTROL,
		     IMXDPU_FETCHECO1_STATICCONTROL_RESET_VALUE);

	/* todo: IMXDPU_MATRIX5_STATICCONTROL        */
	/* todo: IMXDPU_HSCALER5_STATICCONTROL       */
	/* todo: IMXDPU_VSCALER5_STATICCONTROL       */
	/* IMXDPU_LAYERBLEND0_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND0_STATICCONTROL,
		     IMXDPU_LAYERBLEND0_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_LAYERBLEND1_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND1_STATICCONTROL,
		     IMXDPU_LAYERBLEND1_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_LAYERBLEND2_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND2_STATICCONTROL,
		     IMXDPU_LAYERBLEND2_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_LAYERBLEND3_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND3_STATICCONTROL,
		     IMXDPU_LAYERBLEND3_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_LAYERBLEND4_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND4_STATICCONTROL,
		     IMXDPU_LAYERBLEND4_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_LAYERBLEND5_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND5_STATICCONTROL,
		     IMXDPU_LAYERBLEND5_STATICCONTROL_RESET_VALUE);

	/* IMXDPU_LAYERBLEND6_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND6_STATICCONTROL,
		     IMXDPU_LAYERBLEND6_STATICCONTROL_RESET_VALUE);

	/* Dynamic config */
	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHPERSP9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_ROP9_DYNAMIC,
		     IMXDPU_SET_FIELD
		     (IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_PRIM_SEL,
		      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD
		     (IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_SEC_SEL,
		      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD
		     (IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_TERT_SEL,
		      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_CLUT9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_MATRIX9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_HSCALER9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_VSCALER9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FILTER9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC,
		     IMXDPU_SET_FIELD
		     (IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_PRIM_SEL,
		      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD
		     (IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_SEC_SEL,
		      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE2_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE3_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHWARP2_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE0_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE1_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_GAMMACOR4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_MATRIX4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_HSCALER4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_VSCALER4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_HISTOGRAM4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_GAMMACOR5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_MATRIX5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_HSCALER5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_VSCALER5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_HISTOGRAM5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
				      IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
				      IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND1_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND2_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND3_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND4_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND5_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND6_DYNAMIC,
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL,
				      IMXDPU_PIXENGCFG_LAYERBLEND_PRIM_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL,
					IMXDPU_PIXENGCFG_LAYERBLEND_SEC_SEL__DISABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	/* Static configuration - reset values */
	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_STATIC,
		     IMXDPU_PIXENGCFG_STORE9_STATIC_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_STATIC,
		     IMXDPU_PIXENGCFG_EXTDST0_STATIC_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST4_STATIC,
		     IMXDPU_PIXENGCFG_EXTDST4_STATIC_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_STATIC,
		     IMXDPU_PIXENGCFG_EXTDST1_STATIC_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST5_STATIC,
		     IMXDPU_PIXENGCFG_EXTDST5_STATIC_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE4_STATIC,
		     IMXDPU_PIXENGCFG_STORE4_STATIC_RESET_VALUE);

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_STATIC,
		     IMXDPU_PIXENGCFG_STORE9_STATIC_RESET_VALUE);

	/* Static configuration - initial settings */
	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_STATIC,
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_POWERDOWN,
		      IMXDPU_FALSE) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE,
		      IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE__SINGLE) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SW_RESET,
			IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SW_RESET__OPERATION) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_DIV,
				      IMXDPU_PIXENGCFG_DIVIDER_RESET));

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_STATIC,
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_POWERDOWN,
		      IMXDPU_FALSE) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SYNC_MODE,
		      IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SYNC_MODE__AUTO) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SW_RESET,
			IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SW_RESET__OPERATION) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_DIV,
		      IMXDPU_PIXENGCFG_DIVIDER_RESET));

	/* todo: IMXDPU_PIXENGCFG_EXTDST4_STATIC_OFFSET */

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_STATIC,
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_POWERDOWN,
		      IMXDPU_FALSE) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SYNC_MODE,
		      IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SYNC_MODE__AUTO) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SW_RESET,
			IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_SW_RESET__OPERATION) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_EXTDST1_STATIC_EXTDST1_DIV,
		      IMXDPU_PIXENGCFG_DIVIDER_RESET));

	/* todo: IMXDPU_PIXENGCFG_EXTDST5_STATIC_OFFSET */

	imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_STORE4_STATIC,
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_POWERDOWN,
		      IMXDPU_FALSE) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_SYNC_MODE,
		      IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_SYNC_MODE__SINGLE) |
		IMXDPU_SET_FIELD(
			IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_SW_RESET,
			IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_SW_RESET__OPERATION) |
		     IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_STORE4_STATIC_STORE4_DIV,
				      IMXDPU_PIXENGCFG_DIVIDER_RESET));

	/* todo: IMXDPU_PIXENGCFG_STORE4_STATIC */
#if 1
	/* Static Control configuration */
	/* IMXDPU_FETCHDECODE9_STATICCONTROL  */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE9_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE9_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHDECODE9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHPERSP9_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHPERSP9_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHPERSP9_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHPERSP9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHECO9_STATICCONTROL     */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_FETCHECO9_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHECO9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_ROP9_STATICCONTROL          */
	imxdpu_write(imxdpu, IMXDPU_ROP9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_ROP9_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_CLUT9_STATICCONTROL         */
	imxdpu_write(imxdpu, IMXDPU_CLUT9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_CLUT9_STATICCONTROL_SHDEN, 1));

	imxdpu_write(imxdpu, IMXDPU_CLUT9_UNSHADOWEDCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_CLUT9_UNSHADOWEDCONTROL_B_EN,
				      IMXDPU_CLUT9_UNSHADOWEDCONTROL_B_EN__ENABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_CLUT9_UNSHADOWEDCONTROL_G_EN,
					IMXDPU_CLUT9_UNSHADOWEDCONTROL_G_EN__ENABLE)
		     | IMXDPU_SET_FIELD(IMXDPU_CLUT9_UNSHADOWEDCONTROL_R_EN,
					IMXDPU_CLUT9_UNSHADOWEDCONTROL_R_EN__ENABLE));

	/* IMXDPU_MATRIX9_STATICCONTROL       */
	imxdpu_write(imxdpu, IMXDPU_MATRIX9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_MATRIX9_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_HSCALER9_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_HSCALER9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_HSCALER9_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_VSCALER9_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_VSCALER9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_VSCALER9_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_FILTER9_STATICCONTROL       */
	imxdpu_write(imxdpu, IMXDPU_FILTER9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_FILTER9_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_BLITBLEND9_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_BLITBLEND9_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_BLITBLEND9_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_STORE9_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_STORE9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_STORE9_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_STORE9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 1));

	/* IMXDPU_CONSTFRAME0_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_CONSTFRAME0_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_CONSTFRAME0_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_EXTDST0_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTDST0_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_EXTDST0_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_EXTDST0_STATICCONTROL_PERFCOUNTMODE, 0) |
		     IMXDPU_SET_FIELD(IMXDPU_EXTDST0_STATICCONTROL_KICK_MODE,
				      IMXDPU_EXTDST0_STATICCONTROL_KICK_MODE__EXTERNAL));

	/* todo: IMXDPU_CONSTFRAME4_STATICCONTROL    */
	/* todo: IMXDPU_EXTDST4_STATICCONTROL        */

	/* IMXDPU_CONSTFRAME1_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_CONSTFRAME1_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_CONSTFRAME1_STATICCONTROL_SHDEN, 1));

	/* IMXDPU_EXTDST1_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTDST1_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_EXTDST1_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_EXTDST1_STATICCONTROL_PERFCOUNTMODE, 0) |
		     IMXDPU_SET_FIELD(IMXDPU_EXTDST1_STATICCONTROL_KICK_MODE,
				      IMXDPU_EXTDST1_STATICCONTROL_KICK_MODE__EXTERNAL));

	/* todo: IMXDPU_CONSTFRAME5_STATICCONTROL    */
	/* todo: IMXDPU_EXTDST5_STATICCONTROL        */

	/* IMXDPU_EXTSRC4_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTSRC4_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_EXTSRC4_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_EXTSRC4_STATICCONTROL_STARTSEL,
				      IMXDPU_EXTSRC4_STATICCONTROL_STARTSEL__LOCAL));

	/* IMXDPU_STORE4_STATICCONTROL         */
	imxdpu_write(imxdpu, IMXDPU_STORE4_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_STORE4_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_STORE4_STATICCONTROL_BASEADDRESSAUTOUPDATE, 1));

	/* IMXDPU_EXTSRC5_STATICCONTROL        */
	imxdpu_write(imxdpu, IMXDPU_EXTSRC5_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_EXTSRC5_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_EXTSRC5_STATICCONTROL_STARTSEL,
				      IMXDPU_EXTSRC5_STATICCONTROL_STARTSEL__LOCAL));

	/* IMXDPU_STORE5_STATICCONTROL         */
	imxdpu_write(imxdpu, IMXDPU_STORE5_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_STORE5_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_STORE5_STATICCONTROL_BASEADDRESSAUTOUPDATE, 1));

	/* IMXDPU_FETCHDECODE2_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE2_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE2_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHDECODE2_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHDECODE3_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE3_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE3_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHDECODE3_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHWARP2_STATICCONTROL     */
	imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHWARP2_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHWARP2_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHWARP2_STATICCONTROL_SHDLDREQSTICKY, 0));

	/* IMXDPU_FETCHECO2_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO9_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_FETCHECO9_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHECO9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHDECODE0_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE0_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE0_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHDECODE0_STATICCONTROL_BASEADDRESSAUTOUPDATE,
		      0));

	/* IMXDPU_FETCHECO0_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO0_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_FETCHECO0_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHECO0_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHDECODE1_STATICCONTROL   */
	imxdpu_write(imxdpu, IMXDPU_FETCHDECODE1_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE1_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHDECODE1_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHECO1_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_FETCHECO1_STATICCONTROL,
		     IMXDPU_SET_FIELD(IMXDPU_FETCHECO1_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHECO1_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/* IMXDPU_FETCHLAYER0_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHLAYER0_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHLAYER0_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHLAYER0_STATICCONTROL_SHDLDREQSTICKY, 0));

	/* IMXDPU_FETCHLAYER1_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_FETCHLAYER1_STATICCONTROL_SHDEN, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHLAYER1_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0) |
		IMXDPU_SET_FIELD(
			IMXDPU_FETCHLAYER1_STATICCONTROL_SHDLDREQSTICKY, 0));

	/* IMXDPU_GAMMACOR4_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_GAMMACOR4_STATICCONTROL,
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR4_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR4_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR4_STATICCONTROL_REDWRITEENABLE, 1));

	/* todo: IMXDPU_MATRIX4_STATICCONTROL        */
	/* todo: IMXDPU_HSCALER4_STATICCONTROL       */
	/* todo: IMXDPU_VSCALER4_STATICCONTROL       */
	/* IMXDPU_GAMMACOR5_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_GAMMACOR5_STATICCONTROL,
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR5_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR5_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR5_STATICCONTROL_REDWRITEENABLE, 1));

	/* todo: IMXDPU_MATRIX5_STATICCONTROL        */
	/* todo: IMXDPU_HSCALER5_STATICCONTROL       */
	/* todo: IMXDPU_VSCALER5_STATICCONTROL       */

	/* IMXDPU_LAYERBLEND0_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND0_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND0_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND0_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND0_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND1_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND1_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND1_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND1_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND1_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND1_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND1_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND2_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND2_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND2_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND2_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND2_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND2_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND2_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND3_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND3_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND3_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND3_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND3_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND3_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND3_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND4_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND4_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND4_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND4_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND4_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND4_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND4_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND4_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND4_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND4_STATICCONTROL_SHDEN, 0) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND4_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND4_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND4_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND4_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND5_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND5_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND5_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND5_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND5_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND5_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND5_STATICCONTROL_SHDTOKSEL__BOTH));

	/* IMXDPU_LAYERBLEND6_STATICCONTROL    */
	imxdpu_write(imxdpu, IMXDPU_LAYERBLEND6_STATICCONTROL,
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND6_STATICCONTROL_SHDEN, 1) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND6_STATICCONTROL_SHDLDSEL,
			IMXDPU_LAYERBLEND6_STATICCONTROL_SHDLDSEL__SECONDARY) |
		IMXDPU_SET_FIELD(
			IMXDPU_LAYERBLEND6_STATICCONTROL_SHDTOKSEL,
		      IMXDPU_LAYERBLEND6_STATICCONTROL_SHDTOKSEL__BOTH));

	/* todo: IMXDPU_EXTSRC0_STATICCONTROL        */
	/* todo: IMXDPU_EXTSRC1_STATICCONTROL        */
	/* todo: IMXDPU_MATRIX0_STATICCONTROL        */
	/* IMXDPU_GAMMACOR0_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_GAMMACOR1_STATICCONTROL,
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR1_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR1_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR1_STATICCONTROL_REDWRITEENABLE, 1));
	/* todo: IMXDPU_SIG0_STATICCONTROL           */
	/* todo: IMXDPU_MATRIX1_STATICCONTROL        */
	/* IMXDPU_GAMMACOR1_STATICCONTROL      */
	imxdpu_write(imxdpu, IMXDPU_GAMMACOR1_STATICCONTROL,
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR1_STATICCONTROL_BLUEWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR1_STATICCONTROL_GREENWRITEENABLE, 1) |
		IMXDPU_SET_FIELD(
			IMXDPU_GAMMACOR1_STATICCONTROL_REDWRITEENABLE, 1));
	/* IMXDPU_SIG1_STATICCONTROL           */
#endif
	imxdpu_init_irqs(imxdpu_id);

	imxdpu_be_init(imxdpu_id, imxdpu_base);

	return ret;
}

int imxdpu_init_sync_panel(int8_t imxdpu_id,
			   int8_t disp,
			   uint32_t pixel_fmt, struct imxdpu_videomode mode)
{
	int ret = 0;
	IMXDPU_TRACE("%s()\n", __func__);
	return ret;
}

int imxdpu_uninit_sync_panel(int8_t imxdpu_id, int8_t disp)
{
	int ret = 0;
	IMXDPU_TRACE("%s()\n", __func__);
	return ret;
}

int imxdpu_reset_disp_panel(int8_t imxdpu_id, int8_t disp)
{
	int ret = 0;
	IMXDPU_TRACE("%s()\n", __func__);
	return ret;
}

/*!
 * This function initializes the display
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_init(int8_t imxdpu_id, int8_t disp)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;
	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (disp == 0) {
#ifdef IMXDPU_TCON0_MAP_24BIT_0_23
		/* Static  24-bit TCON bit mapping for FPGA */
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT7_4, 0x1d1c1b1a);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT3_0, 0x19181716);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT15_12, 0x13121110);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT11_8, 0x0f0e0d0c);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT23_20, 0x09080706);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT19_16, 0x05040302);
#else
		//  tcon mapping
		//  RR RRRR RRRR GGGG GGGG GGBB BBBB BBBB
		//  98 7654 3210 9876 5432 1098 7654 3210
		//  bits
		//  00 0000 0000 1111 1111 1122 2222 2222
		//  98 7654 3210 8765 5432 1098 7654 3210
		/* 30-bit timing controller setup from i.MX8DV */
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT31_28, 0x00000908);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT27_24, 0x07060504);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT23_20, 0x03020100);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT19_16, 0x13121110);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT15_12, 0x0f0e0d0c);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT11_8, 0x0b0a1d1c);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT7_4, 0x1b1a1918);
		imxdpu_write(imxdpu, IMXDPU_TCON0_MAPBIT3_0, 0x17161514);
#endif
		/* set data enable polarity */
		imxdpu_write(imxdpu, IMXDPU_DISENGCFG_POLARITYCTRL0,
			     IMXDPU_DISENGCFG_POLARITYCTRL0_POLEN0_MASK);

	} else if (disp == 1) {
#ifdef IMXDPU_TCON1_MAP_24BIT_0_23
		/* Static TCON bit mapping */
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT7_4, 0x1d1c1b1a);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT3_0, 0x19181716);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT15_12, 0x13121110);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT11_8, 0x0f0e0d0c);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT23_20, 0x09080706);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT19_16, 0x05040302);
#else
		//  tcon mapping
		//  RR RRRR RRRR GGGG GGGG GGBB BBBB BBBB
		//  98 7654 3210 9876 5432 1098 7654 3210
		//  bits
		//  00 0000 0000 1111 1111 1122 2222 2222
		//  98 7654 3210 8765 5432 1098 7654 3210
		/* 30-bit timing controller setup from i.MX8DV */
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT31_28, 0x00000908);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT27_24, 0x07060504);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT23_20, 0x03020100);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT19_16, 0x13121110);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT15_12, 0x0f0e0d0c);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT11_8, 0x0b0a1d1c);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT7_4, 0x1b1a1918);
		imxdpu_write(imxdpu, IMXDPU_TCON1_MAPBIT3_0, 0x17161514);
#endif
		/* set data enable polarity */
		imxdpu_write(imxdpu, IMXDPU_DISENGCFG_POLARITYCTRL1,
			     IMXDPU_DISENGCFG_POLARITYCTRL1_POLEN1_MASK);

	} else {
		return -EINVAL;
	}
	/* todo: initialize prefetch */

	return ret;
}

/*!
 * This function sets up the frame generator
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param 	enable 		state to set frame generator to
 * @param	mode 		to set the display to
 * @param 	cc_red		constant color red
 * @param 	cc_green	constant color green
 * @param 	cc_blue		constant color blue
 * @param 	cc_alpha 	constant color alpha
*
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_setup_frame_gen(
	int8_t imxdpu_id,
	int8_t disp,
	const struct imxdpu_videomode *mode,
	uint16_t cc_red,    /* 10 bits */
	uint16_t cc_green,  /* 10 bits */
	uint16_t cc_blue,   /* 10 bits */
	uint8_t cc_alpha,
	bool test_mode_enable)
{               /* 1 bits, yes 1 bit */
	int ret = 0;
	uint32_t b_off;     /* block offset for frame generator */
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (disp == 0) {
		b_off = IMXDPU_FRAMEGEN0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPU_FRAMEGEN1_LOCKUNLOCK;
	} else return -EINVAL;

	/* todo:
	   add video mode sanity check here
	   check if LRSYNC is required
	 */

	if (mode->flags & IMXDPU_DISP_FLAGS_LRSYNC) {
		/* todo: here we need to use two outputs to make one */
		if (disp == 0) {
			reg = IMXDPU_SET_FIELD(
				IMXDPU_FRAMEGEN0_FGSTCTRL_FGSYNCMODE,
				 IMXDPU_FRAMEGEN0_FGSTCTRL_FGSYNCMODE__MASTER);
		} else {
			reg = IMXDPU_SET_FIELD(
				IMXDPU_FRAMEGEN1_FGSTCTRL_FGSYNCMODE,
				 IMXDPU_FRAMEGEN1_FGSTCTRL_FGSYNCMODE__SLAVE_CYC);
		}
	} else {
		reg = IMXDPU_SET_FIELD(
			IMXDPU_FRAMEGEN0_FGSTCTRL_FGSYNCMODE,
				       IMXDPU_FRAMEGEN0_FGSTCTRL_FGSYNCMODE__OFF);
	}
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_FGSTCTRL_OFFSET, reg);

	//h_total= mode.hlen + mode.hfp + mode.hbp + mode.hsync;
	//v_total = mode->vlen + mode->vfp + mode->vbp + mode->vsync;

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_HTCFG1_HACT, mode->hlen) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_HTCFG1_HTOTAL,
		(mode->hlen + mode->hfp + mode->hbp + mode->hsync - 1));
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_HTCFG1_OFFSET, reg);

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_HTCFG2_HSYNC,
			       mode->hsync - 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_HTCFG2_HSBP,
				 mode->hbp + mode->hsync - 1) |
		/* shadow enable */
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_HTCFG2_HSEN, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_HTCFG2_OFFSET, reg);

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_VTCFG1_VACT, mode->vlen) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_VTCFG1_VTOTAL,
				 (mode->vlen + mode->vfp + mode->vbp + mode->vsync -
					 1));
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_VTCFG1_OFFSET, reg);

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_VTCFG2_VSYNC,
			       mode->vsync - 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_VTCFG2_VSBP,
				 mode->vbp + mode->vsync - 1) |
		/* shadow enable */
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_VTCFG2_VSEN, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_VTCFG2_OFFSET, reg);

	/* Interupt at position (0, 0) start of frame */
	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_INT0CONFIG_INT0COL, 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_INT0CONFIG_INT0HSEN, 0) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_INT0CONFIG_INT0ROW,
				 mode->vlen + 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_INT0CONFIG_INT0EN, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_INT0CONFIG_OFFSET, reg);

	/* Interupt at position (hlen, vlen) for end of frame interrupt */
	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN1_INT1CONFIG_INT1COL, 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN1_INT1CONFIG_INT1HSEN, 0) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN1_INT1CONFIG_INT1ROW,
				 mode->hlen + 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN1_INT1CONFIG_INT1EN, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN1_INT1CONFIG_OFFSET, reg);

	/* todo: these need to be checked
	   _SKICKCOL for verification: =(FW - 40) , for ref driver = 1 ?
	   _SKICKROW for verif.  =(FH - 1), ref driver = vlen-2
	 */
	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKCOL,
			       mode->hlen - 40) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKINT1EN, 0) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKROW,
				 mode->vlen - 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKEN, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_SKICKCONFIG_OFFSET, reg);

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_PACFG_PSTARTX, 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_PACFG_PSTARTY, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_PACFG_OFFSET, reg);

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_SACFG_SSTARTX, 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_SACFG_SSTARTY, 1);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_SACFG_OFFSET, reg);

	if (IMXDPU_ENABLE == test_mode_enable) {
		reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRL_FGDM,
				       IMXDPU_FRAMEGEN0_FGINCTRL_FGDM__TEST);
	} else {
		reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRL_FGDM,
				       IMXDPU_FRAMEGEN0_FGINCTRL_FGDM__SEC) |
			IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRL_ENPRIMALPHA, 0) |
			IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRL_ENSECALPHA, 0);
	}
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_FGINCTRL_OFFSET, reg);

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRLPANIC_FGDMPANIC,
		IMXDPU_FRAMEGEN0_FGINCTRLPANIC_FGDMPANIC__CONSTCOL) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRLPANIC_ENPRIMALPHAPANIC, 0) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGINCTRLPANIC_ENSECALPHAPANIC, 0);
	imxdpu_write(imxdpu, b_off +
		IMXDPU_FRAMEGEN0_FGINCTRLPANIC_OFFSET, reg);

	/* Set the constant color - ARGB 1-10-10-10 */
	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGCCR_CCRED, cc_red) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGCCR_CCBLUE, cc_blue) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGCCR_CCGREEN, cc_green) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGCCR_CCALPHA, cc_alpha);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_FGCCR_OFFSET, reg);

	/* save the mode */
	imxdpu->video_mode[disp] = *mode;

	imxdpu_disp_dump_mode(&imxdpu->video_mode[disp]);

	return ret;
}

/*!
 * This function updates the frame generator status
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_update_fgen_status(int8_t imxdpu_id, int8_t disp)
{
	int ret = 0;
	uint32_t b_off;     /* block offset for frame generator */
	uint32_t reg;
	uint32_t temp;
	struct imxdpu_soc *imxdpu;
	static uint32_t fcount[IMXDPU_NUM_DI] = { 0, 0 };

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (disp == 0) {
		b_off = IMXDPU_FRAMEGEN0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPU_FRAMEGEN1_LOCKUNLOCK;
	} else return -EINVAL;

	/* todo:
	   add video mode sanity check here
	   check if LRSYNC is required
	 */

	reg = imxdpu_read(imxdpu, b_off + IMXDPU_FRAMEGEN0_FGTIMESTAMP_OFFSET);
	IMXDPU_TRACE_IRQ("DISP %d: findex %d, lindex %d\n", disp,
			 IMXDPU_GET_FIELD
			 (IMXDPU_FRAMEGEN0_FGTIMESTAMP_FRAMEINDEX, reg),
			 IMXDPU_GET_FIELD
			 (IMXDPU_FRAMEGEN0_FGTIMESTAMP_LINEINDEX, reg));

	temp = IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGTIMESTAMP_FRAMEINDEX, reg);
	if (temp != fcount[disp]) {
		fcount[disp] = temp;
		/* Just increment we assume this is called one per frame */
		imxdpu->fgen_stats[disp].frame_count++;
	}

	reg = imxdpu_read(imxdpu, b_off + IMXDPU_FRAMEGEN0_FGCHSTAT_OFFSET);
	temp = IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGCHSTAT_SECSYNCSTAT, reg);

	/* Sync status bits should be set */
	if ((temp != imxdpu->fgen_stats[disp].sec_sync_state) && (temp == 1)) {
		imxdpu->fgen_stats[disp].sec_sync_count++;
		IMXDPU_TRACE_IRQ("DISP %d: sec in sync\n", disp);
	}
	if ((temp != imxdpu->fgen_stats[disp].sec_sync_state) && (temp == 0)) {
		IMXDPU_TRACE_IRQ("DISP %d: sec out of sync\n", disp);
	}
	imxdpu->fgen_stats[disp].sec_sync_state = temp;
	temp = IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGCHSTAT_PRIMSYNCSTAT, reg);

	/* Sync status bits should be set */
	if ((temp != imxdpu->fgen_stats[disp].prim_sync_state) &&
		(temp == 1)) {
		imxdpu->fgen_stats[disp].prim_sync_count++;
		IMXDPU_TRACE_IRQ("DISP %d: prim in sync\n", disp);
	}
	if ((temp != imxdpu->fgen_stats[disp].prim_sync_state) &&
		(temp == 0)) {
		IMXDPU_TRACE_IRQ("DISP %d: prim out of sync\n", disp);
	}
	imxdpu->fgen_stats[disp].prim_sync_state = temp;

	/* primary fifo bit should be clear if in use (panic stream) */
	if (IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGCHSTAT_PFIFOEMPTY, reg)) {
		IMXDPU_TRACE_IRQ("DISP %d: primary fifo empty\n", disp);
		imxdpu_write(imxdpu,
			     b_off + IMXDPU_FRAMEGEN0_FGCHSTATCLR_OFFSET,
			     IMXDPU_FRAMEGEN0_FGCHSTATCLR_CLRPRIMSTAT_MASK);
		imxdpu->fgen_stats[disp].prim_fifo_empty_count++;
	}
	/* secondary fifo and skew error bits should be clear
	   if in use (content stream) */
	if (IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGCHSTAT_SFIFOEMPTY, reg) ||
		IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGCHSTAT_SKEWRANGEERR, reg)) {
		if (IMXDPU_GET_FIELD(IMXDPU_FRAMEGEN0_FGCHSTAT_SFIFOEMPTY, reg)) {
			IMXDPU_TRACE_IRQ("DISP %d: secondary fifo empty\n",
					 disp);
			imxdpu->fgen_stats[disp].sec_fifo_empty_count++;
		}
		if (IMXDPU_GET_FIELD
			(IMXDPU_FRAMEGEN0_FGCHSTAT_SKEWRANGEERR, reg)) {
			IMXDPU_TRACE_IRQ("DISP %d: secondary skew error\n",
					 disp);
			imxdpu->fgen_stats[disp].skew_error_count++;
		}
		imxdpu_write(imxdpu,
			     b_off + IMXDPU_FRAMEGEN0_FGCHSTATCLR_OFFSET,
			     IMXDPU_FRAMEGEN0_FGCHSTATCLR_CLRSECSTAT_MASK);
	}
	return ret;
}

/*!
 * This function requests a shadow loads
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param 	shadow_load_idx  index of the shadow load requested
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_request_shadow_load(int8_t imxdpu_id,
				    int8_t disp,
				    imxdpu_shadow_load_index_t shadow_load_idx)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s(): imxdpu_id %d, disp %d, shadow_load_idx %d\n",
		     __func__, imxdpu_id, disp, shadow_load_idx);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];
	/* trigger configuration of the pipeline */

	if ((disp == 0) || (disp == 1)) {
		/* last request was complete or no request in progress,
		   then start a new request */
		if (imxdpu->shadow_load_state[disp][shadow_load_idx].word == 0) {
			imxdpu->shadow_load_state[disp][shadow_load_idx].state.
				request = IMXDPU_TRUE;
		} else {    /* check ifg the request is busy */
			IMXDPU_TRACE("%s(): shadow load not complete.", __func__);
			return -EBUSY;
		}
	} else return -EINVAL;

	return ret;
}

/*!
 * This function force a shadow loads
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param 	shadow_load_idx  index of the shadow load requested
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_force_shadow_load(int8_t imxdpu_id,
	int8_t disp,
	uint64_t mask)
{
	int ret = 0;
	uint32_t addr_extdst;   /* address for extdst */
	uint32_t addr_fgen; /* address for frame generator */
	uint32_t extdst = 0;
	uint32_t fgen = 0;
	uint32_t sub = 0;
	struct imxdpu_soc *imxdpu;
	int i;
	uint64_t temp_mask;

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (!((disp == 0) || (disp == 1))) {
		return -EINVAL;
	}

	if (mask == 0) {
		return -EINVAL;
	}

	if (disp == 0) {
		addr_fgen = IMXDPU_FRAMEGEN0_FGSLR;
		addr_extdst = IMXDPU_PIXENGCFG_EXTDST0_REQUEST;
	} else if (disp == 1) {
		addr_fgen = IMXDPU_FRAMEGEN1_FGSLR;
		addr_extdst = IMXDPU_PIXENGCFG_EXTDST1_REQUEST;
	} else return -EINVAL;

	for (i = 0; i < IMXDPU_SHDLD_IDX_MAX; i++) {
		temp_mask = 1 << i;
		if ((mask & temp_mask) == 0) continue;

		extdst |= trigger_list[i].extdst;
		sub |= trigger_list[i].sub;

		if ((i == IMXDPU_SHDLD_IDX_CONST0) ||
			(i == IMXDPU_SHDLD_IDX_CONST1)) {
			fgen |= 1;
		}
		mask &= ~temp_mask;
	}

	if (sub) {
		IMXDPU_TRACE_IRQ("Fetch layer shadow request 0x%08x\n", sub);
		if (sub & 0xff) {   /* FETCHLAYER0 */
			imxdpu_write(imxdpu, IMXDPU_FETCHLAYER0_TRIGGERENABLE,
				     sub & 0xff);
		}
		if (sub & 0xff00) { /* FETCHLAYER1 */
			imxdpu_write(imxdpu, IMXDPU_FETCHLAYER1_TRIGGERENABLE,
				     (sub >> 8) & 0xff);
		}
		if (sub & 0xff0000) {   /* FETCHWARP2 */
			imxdpu_write(imxdpu, IMXDPU_FETCHWARP2_TRIGGERENABLE,
				     (sub >> 16) & 0xff);
		}
	}

	if (extdst) {
		IMXDPU_TRACE_IRQ("Extdst shadow request  0x%08x\n", extdst);
		imxdpu_write(imxdpu, addr_extdst, extdst);
	}

	if (fgen) {
		IMXDPU_TRACE_IRQ("Fgen shadow request  0x%08x\n", fgen);
		imxdpu_write(imxdpu, addr_fgen, fgen);
	}

	return ret;
}

/*!
 * This function shows the frame generators status
 *
 * @param	imxdpu_id	id of the diplay unit
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_show_fgen_status(int8_t imxdpu_id)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE_IRQ("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	pr_info("IMXDPU %d stats                               fg0                fg1\n"
		"prim_sync_state:            %10d         %10d\n"
		"sec_sync_state:             %10d         %10d\n"
		"prim_sync_count:            %10d         %10d\n"
		"sec_sync_count:             %10d         %10d\n"
		"skew_error_count:           %10d         %10d\n"
		"prim_fifo_empty_count:      %10d         %10d\n"
		"sec_fifo_empty_count:       %10d         %10d\n"
		"frame_count:                %10d         %10d\n"
		"irq_count:                  %10u\n\n",
		imxdpu_id,
		imxdpu->fgen_stats[0].prim_sync_state,
		imxdpu->fgen_stats[1].prim_sync_state,
		imxdpu->fgen_stats[0].sec_sync_state,
		imxdpu->fgen_stats[1].sec_sync_state,
		imxdpu->fgen_stats[0].prim_sync_count,
		imxdpu->fgen_stats[1].prim_sync_count,
		imxdpu->fgen_stats[0].sec_sync_count,
		imxdpu->fgen_stats[1].sec_sync_count,
		imxdpu->fgen_stats[0].skew_error_count,
		imxdpu->fgen_stats[1].skew_error_count,
		imxdpu->fgen_stats[0].prim_fifo_empty_count,
		imxdpu->fgen_stats[1].prim_fifo_empty_count,
		imxdpu->fgen_stats[0].sec_fifo_empty_count,
		imxdpu->fgen_stats[1].sec_fifo_empty_count,
		imxdpu->fgen_stats[0].frame_count,
		imxdpu->fgen_stats[1].frame_count,
		imxdpu->irq_count);

	return ret;
}

/*!
 * This function enables the frame generator
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param 	enable 		state to set frame generator to
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_enable_frame_gen(int8_t imxdpu_id, int8_t disp, bool enable)
{
	int ret = 0;
	uint32_t b_off;
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (disp == 0) {
		b_off = IMXDPU_FRAMEGEN0_LOCKUNLOCK;
	} else if (disp == 1) {
		b_off = IMXDPU_FRAMEGEN1_LOCKUNLOCK;
	} else return -EINVAL;

	if (enable) {
		//dump_pixencfg_status(imxdpu_id);
		imxdpu_disp_start_shadow_loads(imxdpu_id, disp);
		//dump_pixencfg_status(imxdpu_id);
	}

	reg = enable ? IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGENABLE_FGEN, 1) :
		       IMXDPU_SET_FIELD(IMXDPU_FRAMEGEN0_FGENABLE_FGEN, 0);
	imxdpu_write(imxdpu, b_off + IMXDPU_FRAMEGEN0_FGENABLE_OFFSET, reg);

	return ret;
}

/*!
 * This function sets up the constframe generator
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param 	bg_red		background red
 * @param 	bg_green	background green
 * @param 	bg_blue		background blue
 * @param 	bg_alpha 	background alpha
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_setup_constframe(
	int8_t imxdpu_id,
	int8_t disp,
	uint8_t bg_red,
	uint8_t bg_green,
	uint8_t bg_blue,
	uint8_t bg_alpha)
{
	int ret = 0;
	uint32_t b_off;
	uint32_t reg;
	struct imxdpu_soc *imxdpu;
	imxdpu_shadow_load_index_t shadow_idx;
	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	/* todo: add constfram4 and constframe5 */
	if (disp == 0) {
		b_off = IMXDPU_CONSTFRAME0_LOCKUNLOCK;
		shadow_idx = IMXDPU_SHDLD_IDX_CONST0;
	} else if (disp == 1) {
		b_off = IMXDPU_CONSTFRAME1_LOCKUNLOCK;
		shadow_idx = IMXDPU_SHDLD_IDX_CONST1;
	} else return -EINVAL;

	if (imxdpu->video_mode[disp].flags & IMXDPU_DISP_FLAGS_LRSYNC) {
		/* todo: need to handle sync display case */
	}

	reg = IMXDPU_SET_FIELD(IMXDPU_FRAMEHEIGHT,
			       imxdpu->video_mode[disp].vlen - 1) |
		IMXDPU_SET_FIELD(IMXDPU_FRAMEWIDTH,
				 imxdpu->video_mode[disp].hlen - 1);
	imxdpu_write(imxdpu,
		     b_off + IMXDPU_CONSTFRAME0_FRAMEDIMENSIONS_OFFSET, reg);

	/* todo: add linear light correction if needed */
	imxdpu_write(imxdpu, b_off + IMXDPU_CONSTFRAME0_CONSTANTCOLOR_OFFSET,
		     IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, bg_red) |
		     IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, bg_green) |
		     IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, bg_blue) |
		     IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, bg_alpha));

	imxdpu_disp_request_shadow_load(imxdpu_id, disp, shadow_idx);

	/* todo: add linear light correction if needed */
	return ret;
}

/*!
 * This function sets up a layer
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param       disp		id of the diplay output pipe
 * @param 	layer 		layer data to use
 * @param	layer_idx 	layer index  to use
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_setup_layer(int8_t imxdpu_id,
			    const imxdpu_layer_t *layer,
			    imxdpu_layer_idx_t layer_idx)
{
	int ret = 0;
	uint32_t dynamic_offset;
	uint32_t static_offset;
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	imxdpu->blend_layer[layer_idx] = *layer;

	dynamic_offset = id2dynamicoffset(layer_idx + IMXDPU_ID_LAYERBLEND0);
	if (dynamic_offset == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	static_offset = id2blockoffset(layer_idx + IMXDPU_ID_LAYERBLEND0);
	if (static_offset == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	reg =
		IMXDPU_SET_FIELD(
		IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_PRIM_SEL,
		 imxdpu->blend_layer[layer_idx].primary) |
		IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_SEC_SEL,
		 imxdpu->blend_layer[layer_idx].secondary) |
		IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_CLKEN,
		 IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_CLKEN__AUTOMATIC);
	imxdpu_write(imxdpu, dynamic_offset, reg);

	if (imxdpu->blend_layer[layer_idx].stream & IMXDPU_DISPLAY_STREAM_0) {

		IMXDPU_TRACE("%s():  IMXDPU_DISPLAY_STREAM_0\n", __func__);
		reg = IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL,
			 	layer_idx + IMXDPU_ID_LAYERBLEND0);
		imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC, reg);

		/* trigger configuration of the pipeline */
		imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_TRIGGER,
			     IMXDPU_PIXENGCFG_EXTDST0_TRIGGER_EXTDST0_SYNC_TRIGGER_MASK);
		imxdpu_disp_request_shadow_load(imxdpu_id, 0,
						IMXDPU_SHDLD_IDX_DISP0);
	}
	if (imxdpu->blend_layer[layer_idx].stream & IMXDPU_DISPLAY_STREAM_1) {
		IMXDPU_TRACE_IRQ("%s():  IMXDPU_DISPLAY_STREAM_1\n", __func__);
		reg =
			IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL,
			 layer_idx + IMXDPU_ID_LAYERBLEND0);
		imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_DYNAMIC, reg);

		/* trigger configuration of the pipeline */
		imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_TRIGGER,
			     IMXDPU_PIXENGCFG_EXTDST1_TRIGGER_EXTDST1_SYNC_TRIGGER_MASK);
		imxdpu_disp_request_shadow_load(imxdpu_id, 1,
						IMXDPU_SHDLD_IDX_DISP1);
	}
#if 0				/* todo: add support for panic streams */
	if (imxdpu->blend_layer[layer_idx].stream & IMXDPU_DISPLAY_STREAM_4) {
		reg = IMXDPU_SET_FIELD (IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL,
			layer_idx + IMXDPU_ID_LAYERBLEND0);
		imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC, reg);
	}
	if (imxdpu->blend_layer[layer_idx].stream & IMXDPU_DISPLAY_STREAM_5) {
		reg = IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL,
			layer_idx + IMXDPU_ID_LAYERBLEND0);
		imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC, reg);
	}
#endif

	/* todo: add code to disable a layer */
	return ret;
}

/*!
 * This function sets global alpha for a blend layer
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param	layer_idx 	layer index  to use
 * @param 	alpha 		global alpha
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_set_layer_global_alpha(int8_t imxdpu_id,
				       imxdpu_layer_idx_t layer_idx,
				       uint8_t alpha)
{
	int ret = 0;
	uint32_t offset;
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	/* update imxdpu */

	offset = id2blockoffset(layer_idx + IMXDPU_ID_LAYERBLEND0);
	if (offset == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	reg = IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_BLENDALPHA,
			       alpha)
		| IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_C_BLD_FUNC,
				   IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_C_BLD_FUNC__ONE_MINUS_SEC_ALPHA)
		| IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_C_BLD_FUNC,
				   IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_C_BLD_FUNC__CONST_ALPHA)
		| IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_A_BLD_FUNC,
				   IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_A_BLD_FUNC__ONE_MINUS_SEC_ALPHA)
		| IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_A_BLD_FUNC,
				   IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_A_BLD_FUNC__ONE);
	imxdpu_write(imxdpu, offset + IMXDPU_LAYERBLEND0_BLENDCONTROL_OFFSET,
		     reg);

	// reg = IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_BLENDALPHA,
	//      alpha)
	// | IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_C_BLD_FUNC,
	//      IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_C_BLD_FUNC__ONE_MINUS_CONST_ALPHA)
	// | IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_C_BLD_FUNC,
	//      IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_C_BLD_FUNC__CONST_ALPHA)
	// | IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_A_BLD_FUNC,
	//      IMXDPU_LAYERBLEND0_BLENDCONTROL_PRIM_A_BLD_FUNC__ONE_MINUS_CONST_ALPHA)
	// | IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_A_BLD_FUNC,
	//      IMXDPU_LAYERBLEND0_BLENDCONTROL_SEC_A_BLD_FUNC__ONE);
	// imxdpu_write(imxdpu, offset + IMXDPU_LAYERBLEND0_BLENDCONTROL_OFFSET,
	//      reg);
	//
#if 0
/* todo: need to implement auto mask generation */
	if (layer_idx == 0) {
		reg =
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_MODE,
				 IMXDPU_LAYERBLEND0_CONTROL_MODE__BLEND) |
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKENABLE,
				 IMXDPU_ENABLE) |
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKMODE,
				 IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKMODE__SEC);
	} else {

		reg =
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_MODE,
				 IMXDPU_LAYERBLEND0_CONTROL_MODE__BLEND) |
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKENABLE,
				 IMXDPU_ENABLE) |
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKMODE,
				 IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKMODE__PRIM_OR_SEC);
	}
#else
	reg =
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_MODE,
				 IMXDPU_LAYERBLEND0_CONTROL_MODE__BLEND) |
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_CONTROL_ALPHAMASKENABLE,
				 IMXDPU_DISABLE);
#endif
	imxdpu_write(imxdpu, offset + IMXDPU_LAYERBLEND0_CONTROL_OFFSET, reg);

	return ret;
}

/*!
 * This function sets the position of the a blend layer secondary input
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param	layer_idx 	layer index  to use
 * @param 	x 		x position
 * @param 	y 		y position
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_set_layer_position(int8_t imxdpu_id,
				   imxdpu_layer_idx_t layer_idx,
				   int16_t x, int16_t y)
{
	int ret = 0;
	uint32_t offset;
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	/* update imxdpu */

	offset = id2blockoffset(layer_idx + IMXDPU_ID_LAYERBLEND0);
	if (offset == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	reg = IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_POSITION_XPOS, x) |
		IMXDPU_SET_FIELD(IMXDPU_LAYERBLEND0_POSITION_YPOS, y);
	imxdpu_write(imxdpu, offset + IMXDPU_LAYERBLEND0_POSITION_OFFSET, reg);

	return ret;
}

/*!
 * This function sets the position of the a channel (window) layer
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param	layer_idx 	layer index  to use
 * @param 	x 		x position
 * @param 	y 		y position
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_set_chan_position(int8_t imxdpu_id,
				  imxdpu_chan_t chan, int16_t x, int16_t y)
{
	int ret = 0;
	uint32_t offset;
	int idx;
	int sub_idx;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	/* update imxdpu */

	offset = id2blockoffset(get_channel_blk(chan));
	if (offset == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	idx = get_channel_idx(chan);
	if ((idx >= IMXDPU_CHAN_IDX_IN_MAX) || (idx < 0)) {
		return -EINVAL;
	}

	sub_idx = imxdpu_get_channel_subindex(chan);

	imxdpu->chan_data[idx].dest_top = y;
	imxdpu->chan_data[idx].dest_left = x;

	imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0 =
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE0_LAYEROFFSET0_LAYERXOFFSET0,
		imxdpu->chan_data[idx].dest_left) |
		IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE0_LAYEROFFSET0_LAYERYOFFSET0,
		imxdpu->chan_data[idx].dest_top);

	if (is_fetch_layer_chan(chan) || is_fetch_warp_chan(chan)) {
		imxdpu_write_block(imxdpu,
			offset +
			IMXDPU_FETCHLAYER0_BASEADDRESS0_OFFSET +
			((IMXDPU_SUBCHAN_LAYER_OFFSET * sub_idx)),
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
	} else if (is_fetch_decode_chan(chan)) {
		if (imxdpu->chan_data[idx].use_eco_fetch) {
			imxdpu_disp_set_chan_position(imxdpu_id,
				imxdpu_get_eco(chan),
				x, y);
		}
		imxdpu_write(imxdpu,
			offset + IMXDPU_FETCHDECODE0_LAYEROFFSET0_OFFSET,
			imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0);
	} else if (is_fetch_eco_chan(chan)) {
		imxdpu_write(imxdpu,
			offset + IMXDPU_FETCHECO0_LAYEROFFSET0_OFFSET,
			imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0);
	} else return -EINVAL;

	imxdpu_disp_request_shadow_load(imxdpu_id,
		imxdpu->chan_data[idx].disp_id,
		IMXDPU_SHDLD_IDX_CHAN_00 + idx);

	return ret;
}

/*!
 * This function sets the source and destination crop
 * position of the a channel (window) layer
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param	chan	 	chan to use
 * @param 	clip_top	source y position
 * @param 	clip_left	source x position
 * @param 	clip_width	source width
 * @param 	clip_height	source height
 * @param 	dest_top	destination y
 * @param 	dest_left	destination x
 * @param 	dest_width	destination width
 * @param 	dest_height	destination height
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_disp_set_chan_crop(
	int8_t imxdpu_id,
	imxdpu_chan_t chan,
	int16_t  clip_top,
	int16_t  clip_left,
	uint16_t clip_width,
	uint16_t clip_height,
	int16_t  dest_top,
	int16_t  dest_left,
	uint16_t dest_width,
	uint16_t dest_height)
{
	int ret = 0;
	uint32_t offset;
	int idx;
	int sub_idx;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];


	offset = id2blockoffset(get_channel_blk(chan));
	if (offset == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	idx = get_channel_idx(chan);
	if ((idx >= IMXDPU_CHAN_IDX_IN_MAX) || (idx < 0)) {
		return -EINVAL;
	}

	if ((imxdpu->chan_data[idx].clip_height < 0) ||
		(imxdpu->chan_data[idx].clip_width < 0)) {
		return -EINVAL;
	}

	sub_idx = imxdpu_get_channel_subindex(chan);

	imxdpu->chan_data[idx].dest_top    = dest_top;
	imxdpu->chan_data[idx].dest_left   = dest_left;
	imxdpu->chan_data[idx].dest_width  = IMXDPU_MIN(dest_width, clip_width);
	imxdpu->chan_data[idx].dest_height = IMXDPU_MIN(dest_height, clip_height);
	imxdpu->chan_data[idx].clip_top    = clip_top;
	imxdpu->chan_data[idx].clip_left   = clip_left;
	imxdpu->chan_data[idx].clip_width  = IMXDPU_MIN(dest_width, clip_width);
	imxdpu->chan_data[idx].clip_height = IMXDPU_MIN(dest_height, clip_height);

	/* Need to check more cases here */
	if ((imxdpu->chan_data[idx].clip_height != 0) &&
		(imxdpu->chan_data[idx].clip_width != 0)) {
		imxdpu->chan_data[idx].fetch_layer_prop.layerproperty0 |=
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE,
			IMXDPU_ENABLE);
		imxdpu->chan_data[idx].fetch_layer_prop.clipwindowdimensions0 =
			IMXDPU_SET_FIELD(IMXDPU_CLIP_HEIGHT,
			imxdpu->chan_data[idx].clip_height - 1) |
			IMXDPU_SET_FIELD(IMXDPU_CLIP_WIDTH,
			imxdpu->chan_data[idx].clip_width - 1);
	} else {
		imxdpu->chan_data[idx].fetch_layer_prop.layerproperty0 &=
			~IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE_MASK;
		imxdpu->chan_data[idx].fetch_layer_prop.clipwindowdimensions0 = 0;
	}
	imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0 =
		IMXDPU_SET_FIELD(IMXDPU_LAYER_XOFFSET,
		imxdpu->chan_data[idx].dest_left - imxdpu->chan_data[idx].clip_left) |
		IMXDPU_SET_FIELD(IMXDPU_LAYER_YOFFSET,
		imxdpu->chan_data[idx].dest_top - imxdpu->chan_data[idx].clip_top);
	imxdpu->chan_data[idx].fetch_layer_prop.clipwindowoffset0 =
		IMXDPU_SET_FIELD(IMXDPU_CLIP_XOFFSET,
		imxdpu->chan_data[idx].dest_left) |
		IMXDPU_SET_FIELD(IMXDPU_CLIP_YOFFSET,
		imxdpu->chan_data[idx].dest_top);


	if (is_fetch_layer_chan(chan) || is_fetch_warp_chan(chan)) {


		imxdpu_write_block(imxdpu,
			offset +
			IMXDPU_FETCHLAYER0_LAYEROFFSET0_OFFSET +
			((IMXDPU_SUBCHAN_LAYER_OFFSET * sub_idx)),
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop.baseaddress0,
			5);

	} else if (is_fetch_decode_chan(chan)) {
		if (imxdpu->chan_data[idx].use_eco_fetch) {
			imxdpu_disp_set_chan_crop(imxdpu_id,
				imxdpu_get_eco(chan),
				clip_top,
				clip_left,
				clip_width,
				clip_height,
				dest_top,
				dest_left,
				dest_width,
				dest_height);
		}


		imxdpu_write_block(imxdpu,
			offset +
			IMXDPU_FETCHDECODE0_LAYEROFFSET0_OFFSET, //IMXDPU_FETCHDECODE0_BASEADDRESS0_OFFSET,
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0,
			5);
	} else if (is_fetch_eco_chan(chan)) {
		imxdpu_write_block(imxdpu,
			offset + IMXDPU_FETCHECO0_LAYEROFFSET0_OFFSET,
			(void *)&imxdpu->chan_data[idx].fetch_layer_prop.layeroffset0,
			5);

	} else return -EINVAL;

	imxdpu_disp_request_shadow_load(imxdpu_id,
		imxdpu->chan_data[idx].disp_id,
		IMXDPU_SHDLD_IDX_CHAN_00 + idx);

	return ret;
}

/*!
 * This function sets initializes a channel and buffer
 *
 * @param	imxdpu_id	id of the diplay unit
 * @param	chan    	chan to use
 * @param       src_pixel_fmt   source pixel format
 * @param       clip_top	source y position
 * @param       clip_left	source x position
 * @param       clip_width	source width
 * @param       clip_height	source height
 * @param       stride		stride of the buffer
 * @param       disp_id		display id
 * @param       dest_top	destination y
 * @param       dest_left	destination x
 * @param       dest_width	destination width
 * @param       dest_height	destination height
 * @param       const_color     constant color for clip region
 * @param       disp_addr	display buffer physical address
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int imxdpu_disp_setup_channel(int8_t imxdpu_id,
	imxdpu_chan_t chan,
	uint32_t src_pixel_fmt,
	uint16_t src_width,
	uint16_t src_height,
	int16_t clip_top,
	int16_t clip_left,
	uint16_t clip_width,
	uint16_t clip_height,
	uint16_t stride,
	//      uint32_t dest_pixel_fmt,
	//      uint8_t  blend_mode,
	//      uint8_t  blend_layer,
	uint8_t disp_id,
	int16_t dest_top,
	int16_t dest_left,
	uint16_t dest_width,
	uint16_t dest_height,
	uint32_t const_color,
	unsigned int disp_addr)
{
	int ret = 0;
	imxdpu_chan_t eco_chan;
	imxdpu_channel_params_t channel;
	uint32_t uv_offset=0;

	channel.common.chan = chan;
	channel.common.src_pixel_fmt = src_pixel_fmt;
	channel.common.src_width = src_width;
	channel.common.src_height = src_height;
	channel.common.clip_top = clip_top;
	channel.common.clip_left = clip_left;
	channel.common.clip_width = clip_width;
	channel.common.clip_height = clip_height;
	channel.common.stride = stride;
//      channel.common.dest_pixel_fmt = dest_pixel_fmt;
//      channel.common.blend_mode =     blend_mode;
//      channel.common.blend_layer =    blend_layer;
	channel.common.disp_id = disp_id;
	channel.common.dest_top = dest_top;
	channel.common.dest_left = dest_left;
	channel.common.dest_width = dest_width;
	channel.common.dest_height = dest_height;
	channel.common.const_color = const_color;
	//channel.common.stride = FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	//channel.common.use_video_proc = IMXDPU_FALSE;
	//channel.common.disp_id = 0;

	if (imxdpu_get_planes(src_pixel_fmt) == 2) {
		uv_offset = src_width*src_height; /* works for NV12 and NV16*/
	}

	ret = imxdpu_init_channel(imxdpu_id, &channel);

	ret = imxdpu_init_channel_buffer(imxdpu_id, channel.common.chan, channel.common.stride, IMXDPU_ROTATE_NONE, //imxdpu_rotate_mode_t rot_mode,
					 disp_addr, //dma_addr_t phyaddr_0,
					 uv_offset, //uint32_t u_offset,
					 0);    //uint32_t v_offset)

	ret = imxdpu_disp_set_chan_crop(imxdpu_id,
					channel.common.chan,
					channel.common.clip_top,
					channel.common.clip_left,
					channel.common.clip_width,
					channel.common.clip_height,
					channel.common.dest_top,
					channel.common.dest_left,
					channel.common.dest_width,
					channel.common.dest_height);

	dump_channel(imxdpu_id, channel.common.chan);
	eco_chan = imxdpu_get_eco(channel.common.chan);
	if ( eco_chan != 0 ) {
		dump_channel(imxdpu_id, eco_chan);
	}

	return ret;
}

/*!
 * This function prints the video mode passed as a parameter
 *
 * @param	*mode	pointer to video mode struct to show
 */
void imxdpu_disp_dump_mode(const struct imxdpu_videomode *mode)
{
	IMXDPU_TRACE("%s():\n", __func__);
	IMXDPU_TRACE("\thlen   %4d\n", mode->hlen);
	IMXDPU_TRACE("\thfp    %4d\n", mode->hfp);
	IMXDPU_TRACE("\thbp    %4d\n", mode->hbp);
	IMXDPU_TRACE("\thsync  %4d\n", mode->hsync);
	IMXDPU_TRACE("\tvlen   %4d\n", mode->vlen);
	IMXDPU_TRACE("\tvfp    %4d\n", mode->vfp);
	IMXDPU_TRACE("\tvbp    %4d\n", mode->vbp);
	IMXDPU_TRACE("\tvsync  %4d\n", mode->vsync);
	IMXDPU_TRACE("\tvsync  %4d\n", mode->vsync);
	IMXDPU_TRACE("\tflags 0x%08x:\n", mode->flags);

	if (mode->flags & IMXDPU_DISP_FLAGS_HSYNC_LOW) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_HSYNC_LOW is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_HSYNC_HIGH) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_HSYNC_HIGH is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_VSYNC_LOW) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_VSYNC_LOW is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_VSYNC_HIGH) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_VSYNC_HIGH is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_DE_LOW) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_DE_LOW is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_DE_HIGH) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_DE_HIGH is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_POSEDGE) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_POSEDGE is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_NEGEDGE) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_NEGEDGE is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_INTERLACED) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_INTERLACED is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_LRSYNC) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_LRSYNC is set\n");
	if (mode->flags & IMXDPU_DISP_FLAGS_SPLIT) IMXDPU_TRACE("\t\tIMXDPU_DISP_FLAGS_SPLIT is set\n");
}

/*!
 * Returns the bytes per pixel
 *
 * @param	pixel format
 *
 * @return 	returns number of bytes per pixel or zero
 * 		if the format is not matched.
 */
int imxdpu_bytes_per_pixel(uint32_t fmt)
{
	IMXDPU_TRACE("%s():\n", __func__);
	switch (fmt) {
/* todo add NV12, and NV16 */
		case IMXDPU_PIX_FMT_NV12:
//      case IMXDPU_PIX_FMT_NV16:
			return 1; /* luma */
//              break;

		case IMXDPU_PIX_FMT_RGB565:
		case IMXDPU_PIX_FMT_YUYV:
		case IMXDPU_PIX_FMT_UYVY:
			return 2;
			break;
		case IMXDPU_PIX_FMT_BGR24:
		case IMXDPU_PIX_FMT_RGB24:
		case IMXDPU_PIX_FMT_YUV444:
			return 3;
			break;
		case IMXDPU_PIX_FMT_GENERIC_32:
		case IMXDPU_PIX_FMT_BGR32:
		case IMXDPU_PIX_FMT_BGRA32:
		case IMXDPU_PIX_FMT_RGB32:
		case IMXDPU_PIX_FMT_RGBA32:
		case IMXDPU_PIX_FMT_ABGR32:
		case IMXDPU_PIX_FMT_AYUV:
			return 4;
			break;
		default:
			pr_warn("%s(): unsupported pixel format", __func__);
			return 0;
	}
}

/*!
 * Returns the number of bits per color component for the color
 * component bits register
 *
 * @param	pixel format
 *
 * @return 	Returns the number of bits per color component for
 *   		the color component bits register.
 */
uint32_t imxdpu_get_colorcomponentbits(uint32_t fmt)
{
	IMXDPU_TRACE("%s():\n", __func__);
	switch (fmt) {
/* todo add NV12, NV16, YUYV, and  UYVY */
//      case IMXDPU_PIX_FMT_YUYV:
//      case IMXDPU_PIX_FMT_UYVY:
//              return 0x0;
//      case IMXDPU_PIX_FMT_NV16:
//              return 0x0;
		case IMXDPU_PIX_FMT_NV12:
			return
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSRED0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSGREEN0, 0x00) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSBLUE0, 0x00) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSALPHA0, 0x00);

		case IMXDPU_PIX_FMT_RGB565:
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSRED0, 0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSGREEN0, 5) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSBLUE0, 11) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSALPHA0, 0);

		case IMXDPU_PIX_FMT_BGR24:
		case IMXDPU_PIX_FMT_RGB24:
		case IMXDPU_PIX_FMT_YUV444:
		case IMXDPU_PIX_FMT_BGR32:
		case IMXDPU_PIX_FMT_RGB32:
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSRED0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSGREEN0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSBLUE0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSALPHA0, 0x0);

		case IMXDPU_PIX_FMT_GENERIC_32:
		case IMXDPU_PIX_FMT_BGRA32:
		case IMXDPU_PIX_FMT_RGBA32:
		case IMXDPU_PIX_FMT_ABGR32:
		case IMXDPU_PIX_FMT_ARGB32:
		case IMXDPU_PIX_FMT_AYUV:
				return
					IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSRED0, 0x08) |
					IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSGREEN0, 0x08) |
					IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSBLUE0, 0x08) |
					IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSALPHA0, 0x08);
		default:
			pr_warn("%s(): unsupported pixel format", __func__);
			return 0;
	}
	return 0;
}

/*!
 * Returns the number of planes for the pixel format
 *
 * @param	pixel format
 *
 * @return 	returns number of bytes per pixel or zero
 * 		if the format is not matched.
 */
uint32_t imxdpu_get_planes(uint32_t fmt)
{
	IMXDPU_TRACE("%s():\n", __func__);
	switch (fmt) {
		case IMXDPU_PIX_FMT_NV16:
		case IMXDPU_PIX_FMT_NV12:
			return  2;

		case IMXDPU_PIX_FMT_RGB565:
		case IMXDPU_PIX_FMT_YUYV:
		case IMXDPU_PIX_FMT_UYVY:
		case IMXDPU_PIX_FMT_BGRA32:
		case IMXDPU_PIX_FMT_RGBA32:
		case IMXDPU_PIX_FMT_ABGR32:
		case IMXDPU_PIX_FMT_AYUV:
		case IMXDPU_PIX_FMT_BGR24:
		case IMXDPU_PIX_FMT_RGB24:
		case IMXDPU_PIX_FMT_YUV444:
		case IMXDPU_PIX_FMT_BGR32:
		case IMXDPU_PIX_FMT_RGB32:
		case IMXDPU_PIX_FMT_ARGB32:
			return 1;
		default:
			return 0;
			pr_warn("%s(): unsupported pixel format", __func__);
	}
}

/*!
 * Returns the color component bit position shifts
 *
 * @param	pixel format
 *
 * @return 	returns the register setting for the
  *		colorcomponentshift register
  *
 */
uint32_t imxdpu_get_colorcomponentshift(uint32_t fmt)
{
	IMXDPU_TRACE("%s():\n", __func__);
	switch (fmt) {

		// case IMXDPU_PIX_FMT_NV16:
		case IMXDPU_PIX_FMT_NV12:
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x0);

		case IMXDPU_PIX_FMT_RGB565:
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 5) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 6) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 5) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0);
		case IMXDPU_PIX_FMT_YUYV:
		case IMXDPU_PIX_FMT_UYVY:
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x0);

		case IMXDPU_PIX_FMT_BGR24:
		case IMXDPU_PIX_FMT_BGR32:
		case IMXDPU_PIX_FMT_BGRA32:
			/* 0xaaRRGGBB */
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x10) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x00) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x18);

		case IMXDPU_PIX_FMT_AYUV:
			/* 0xVVUUYYAA  */
		case IMXDPU_PIX_FMT_ABGR32:
			/* 0xRRGGBBAA */
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x18) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x10) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x00);

		case IMXDPU_PIX_FMT_ARGB32:
			/* 0xBBGGRRAA */
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x10) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x18) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x00);

		case IMXDPU_PIX_FMT_GENERIC_32:
		case IMXDPU_PIX_FMT_RGB24:
		case IMXDPU_PIX_FMT_YUV444:
		case IMXDPU_PIX_FMT_RGB32:
		case IMXDPU_PIX_FMT_RGBA32:
			/* 0xaaBBGGRR or 0xaaUUVVYY */
			return IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x00) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x08) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x10) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x18);
		default:
			return 0;
			pr_warn("%s(): unsupported pixel format", __func__);
	}
}

/*!
 * Returns true is the format has local alpha
 *
 * @param	pixel format
 *
 * @return 	Returns true is the format has local alpha
 */
uint32_t imxdpu_has_localalpha(uint32_t fmt)
{
	IMXDPU_TRACE("%s():\n", __func__);
	switch (fmt) {
		case IMXDPU_PIX_FMT_BGRA32:
		case IMXDPU_PIX_FMT_AYUV:
		case IMXDPU_PIX_FMT_RGBA32:
			return IMXDPU_TRUE;
		default:
			return IMXDPU_FALSE;
	}
}

/*!
 * Returns the bits per pixel
 *
 * @param	pixel format
 *
 * @return 	returns number of bits per pixel or zero
 * 		if the format is not matched.
 */
int imxdpu_bits_per_pixel(uint32_t fmt)
{
	int ret = 0;
	switch (fmt) {
		case IMXDPU_PIX_FMT_NV12:
			ret = 8;
			break;
		case IMXDPU_PIX_FMT_NV16:
		case IMXDPU_PIX_FMT_RGB565:
		case IMXDPU_PIX_FMT_YUYV:
		case IMXDPU_PIX_FMT_UYVY:
			ret = 16;
			break;
		case IMXDPU_PIX_FMT_BGR24:
		case IMXDPU_PIX_FMT_RGB24:
		case IMXDPU_PIX_FMT_YUV444:
			ret = 24;
			break;

		case IMXDPU_PIX_FMT_GENERIC_32:
		case IMXDPU_PIX_FMT_BGR32:
		case IMXDPU_PIX_FMT_BGRA32:
		case IMXDPU_PIX_FMT_RGB32:
		case IMXDPU_PIX_FMT_RGBA32:
		case IMXDPU_PIX_FMT_ABGR32:
		case IMXDPU_PIX_FMT_ARGB32:
		case IMXDPU_PIX_FMT_AYUV:
			ret = 32;
			break;
		default:
			pr_warn("%s(): unsupported pixel format", __func__);
			break;
	}
	IMXDPU_TRACE("%s(): fmt 0x%08x, ret %d\n", __func__, fmt, ret);

	return ret;
}

/*!
 * Tests for YUV
 *
 * @param	pixel format
 *
 * @return 	returns true if the format is YUV.
 */
static bool imxdpu_is_yuv(uint32_t fmt)
{
	int ret = IMXDPU_FALSE;
	switch (fmt) {
		case IMXDPU_PIX_FMT_AYUV:
		case IMXDPU_PIX_FMT_NV12:
		case IMXDPU_PIX_FMT_NV16:
		case IMXDPU_PIX_FMT_YUYV:
		case IMXDPU_PIX_FMT_UYVY:
		case IMXDPU_PIX_FMT_YUV444:
			ret = IMXDPU_TRUE;
			break;
		case IMXDPU_PIX_FMT_GENERIC_32:
		case IMXDPU_PIX_FMT_BGR32:
		case IMXDPU_PIX_FMT_BGRA32:
		case IMXDPU_PIX_FMT_RGB32:
		case IMXDPU_PIX_FMT_RGBA32:
		case IMXDPU_PIX_FMT_ABGR32:
		case IMXDPU_PIX_FMT_ARGB32:
		case IMXDPU_PIX_FMT_RGB565:
		case IMXDPU_PIX_FMT_BGR24:
		case IMXDPU_PIX_FMT_RGB24:
			ret = IMXDPU_FALSE;
			break;

		default:
			IMXDPU_TRACE("%s(): unsupported pixel format", __func__);
			ret = IMXDPU_FALSE;
			break;
	}
	IMXDPU_TRACE("%s(): fmt 0x%08x, ret %d\n", __func__, fmt, ret);

	return ret;
}

/*!
 * Intializes buffers to be used for a channel
 *
 * @param       imxdpu_id       id of the diplay unit
 * @param       chan    	channel to use for this buffer
 * @param       pixel_fmt       pixel format of buffers
 * @param       stride  	total width in the buffer in pixels
 * @param       rot_mode	rotatation mode
 * @param       phyaddr_0       buffer 0 address
 * @param       u_offset	U offset
 * @param       v_offset	V offset
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int imxdpu_init_channel_buffer(
	int8_t imxdpu_id,
	imxdpu_chan_t chan,
	uint32_t stride,
	imxdpu_rotate_mode_t rot_mode,
	dma_addr_t phyaddr_0,
	uint32_t u_offset, uint32_t v_offset)
{
	int ret = 0;
	uint32_t b_off;
	struct imxdpu_soc *imxdpu;
	imxdpu_chan_idx_t chan_idx = get_channel_idx(chan);
	int sub_idx = imxdpu_get_channel_subindex(chan);
	bool enable_clip = IMXDPU_FALSE;
	bool enable_buffer = IMXDPU_TRUE;
	uint8_t enable_yuv = IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__OFF;
	uint8_t input_select = IMXDPU_FETCHDECODE2_CONTROL_INPUTSELECT__INACTIVE;
	uint32_t fwidth;
	uint32_t fheight;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}

	b_off = id2blockoffset(get_channel_blk(chan));
	if (b_off == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	imxdpu->chan_data[chan_idx].phyaddr_0 = phyaddr_0;
	imxdpu->chan_data[chan_idx].u_offset = u_offset;
	imxdpu->chan_data[chan_idx].v_offset = v_offset;

	/* update stride if provided */
	if (stride != 0) {
		/* todo: check stride range */
		imxdpu->chan_data[chan_idx].stride = stride;
	}
	/* default horizontal scan
	 * todo: add support for vertical and warp scans
	 */
	if (sub_idx == 0) {
		imxdpu_write(imxdpu,
			b_off +
			IMXDPU_FETCHDECODE0_BURSTBUFFERMANAGEMENT_OFFSET,
			IMXDPU_SET_FIELD(
				IMXDPU_FETCHDECODE2_BURSTBUFFERMANAGEMENT_SETBURSTLENGTH,
				burst_param[IMXDPU_BURST_HORIZONTAL].
				len) |
			IMXDPU_SET_FIELD(
				IMXDPU_FETCHDECODE2_BURSTBUFFERMANAGEMENT_SETNUMBUFFERS,
				burst_param[IMXDPU_BURST_HORIZONTAL].buffers));
	}
	/* todo: Add range checking here */
	imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 = phyaddr_0;
	imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferattributes0 =
		IMXDPU_SET_FIELD(IMXDPU_BUFF_ATTR_BITSPERPIXEL,
		imxdpu_bits_per_pixel(
			imxdpu->chan_data[chan_idx].src_pixel_fmt)) |
		IMXDPU_SET_FIELD(IMXDPU_BUFF_ATTR_STRIDE,
		imxdpu->chan_data[chan_idx].stride - 1);
	imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferdimension0 =
		IMXDPU_SET_FIELD(IMXDPU_BUFF_DIMEN_LINECOUNT,
		imxdpu->chan_data[chan_idx].src_height - 1) |
		IMXDPU_SET_FIELD(IMXDPU_BUFF_DIMEN_LINEWIDTH,
		imxdpu->chan_data[chan_idx].src_width - 1);
	imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentbits0 =
		imxdpu_get_colorcomponentbits(
		imxdpu->chan_data[chan_idx].src_pixel_fmt);
	imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentshift0 =
		imxdpu_get_colorcomponentshift(
		imxdpu->chan_data[chan_idx].src_pixel_fmt);
#if 1
	imxdpu->chan_data[chan_idx].fetch_layer_prop.layeroffset0 =
		IMXDPU_SET_FIELD(IMXDPU_LAYER_XOFFSET,
		imxdpu->chan_data[chan_idx].dest_left) |
		IMXDPU_SET_FIELD(IMXDPU_LAYER_YOFFSET,
		imxdpu->chan_data[chan_idx].dest_top);
	imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowoffset0 =
		IMXDPU_SET_FIELD(IMXDPU_CLIP_XOFFSET,
		imxdpu->chan_data[chan_idx].clip_left) |
		IMXDPU_SET_FIELD(IMXDPU_CLIP_YOFFSET,
		imxdpu->chan_data[chan_idx].clip_top);
	imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowdimensions0 =
		IMXDPU_SET_FIELD(IMXDPU_CLIP_HEIGHT,
		imxdpu->chan_data[chan_idx].clip_height - 1) |
		IMXDPU_SET_FIELD(IMXDPU_CLIP_WIDTH,
		imxdpu->chan_data[chan_idx].clip_width - 1);
	if ((imxdpu->chan_data[chan_idx].clip_height != 0) &&
		(imxdpu->chan_data[chan_idx].clip_width != 0)) {
		imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowdimensions0 =
			IMXDPU_SET_FIELD(IMXDPU_CLIP_HEIGHT,
			imxdpu->chan_data[chan_idx].clip_height - 1) |
			IMXDPU_SET_FIELD(IMXDPU_CLIP_WIDTH,
			imxdpu->chan_data[chan_idx].clip_width - 1);
		enable_clip = IMXDPU_ENABLE;
	} else {
		imxdpu->chan_data[chan_idx].fetch_layer_prop.clipwindowdimensions0 = 0;
	}
		
#endif

		
	imxdpu->chan_data[chan_idx].fetch_layer_prop.constantcolor0 =
		imxdpu->chan_data[chan_idx].const_color;

	if (imxdpu->chan_data[chan_idx].phyaddr_0 == 0) {
		enable_buffer = IMXDPU_FALSE;
	}
	if (imxdpu->chan_data[chan_idx].phyaddr_0 == 0) {
		enable_buffer = IMXDPU_FALSE;
	}

	if (imxdpu_is_yuv(imxdpu->chan_data[chan_idx].src_pixel_fmt)) {
		/* TODO: need to get correct encoding range */
		//enable_yuv = IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601_FR;
		enable_yuv = IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE__ITU601;
	}


	if (is_fetch_decode_chan(chan)) {
		IMXDPU_TRACE("%s(): fetch decode channel\n", __func__);
		if (imxdpu->chan_data[chan_idx].use_eco_fetch) {
			input_select = IMXDPU_FETCHDECODE2_CONTROL_INPUTSELECT__COMPPACK;
			if (chan == IMXDPU_CHAN_01) {
				imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE0_DYNAMIC,
					IMXDPU_SET_FIELD(
						IMXDPU_PIXENGCFG_SRC_SEL,
						IMXDPU_PIXENGCFG_FETCHDECODE0_DYNAMIC_FETCHDECODE0_SRC_SEL__FETCHECO0));
			} else if (chan == IMXDPU_CHAN_19) {
				imxdpu_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE1_DYNAMIC,
					IMXDPU_SET_FIELD(
						IMXDPU_PIXENGCFG_SRC_SEL,
						IMXDPU_PIXENGCFG_FETCHDECODE1_DYNAMIC_FETCHDECODE1_SRC_SEL__FETCHECO1));
			}
			imxdpu_init_channel_buffer(imxdpu_id,
				imxdpu_get_eco(chan),
				stride,
				rot_mode,
				phyaddr_0,
				u_offset, v_offset);

			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentbits0 =
				(0x08 << IMXDPU_FETCHDECODE0_COLORCOMPONENTBITS0_COMPONENTBITSRED0_SHIFT);
			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentshift0 =
				(0x00 << IMXDPU_FETCHDECODE0_COLORCOMPONENTSHIFT0_COMPONENTSHIFTRED0_SHIFT);

		} /* else need to handle Alpha, Warp, CLUT ... */
		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE,
			enable_buffer) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE,
			enable_yuv) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE,
			enable_clip);

		/* todo: handle all cases for control register */
		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHDECODE2_CONTROL_OFFSET,
			IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE2_CONTROL_YUV422UPSAMPLINGMODE,
				IMXDPU_FETCHDECODE2_CONTROL_YUV422UPSAMPLINGMODE__INTERPOLATE) |
			IMXDPU_FETCHDECODE0_CONTROL_PALETTEIDXWIDTH_MASK | /* needed ?*/
			IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE2_CONTROL_CLIPCOLOR, 1) |  /*needed for clip */
			IMXDPU_SET_FIELD(IMXDPU_FETCHDECODE2_CONTROL_INPUTSELECT, input_select)); /*needed for eco */

		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHDECODE2_FRAMEDIMENSIONS_OFFSET,
			IMXDPU_SET_FIELD
			(IMXDPU_FETCHDECODE2_FRAMEDIMENSIONS_FRAMEHEIGHT,
				imxdpu->chan_data[chan_idx].dest_height -
				1 /*fheight-1 */) |
			IMXDPU_SET_FIELD
			(IMXDPU_FETCHDECODE2_FRAMEDIMENSIONS_FRAMEWIDTH,
				imxdpu->chan_data[chan_idx].dest_width -
				1 /*fwidth-1 */));
		imxdpu_write_block(imxdpu,
			b_off + IMXDPU_FETCHDECODE0_BASEADDRESS0_OFFSET,
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_layer_chan(chan)) {
		IMXDPU_TRACE("%s(): fetch layer channel\n", __func__);
		/* here the frame is shared for all sub layers so we use
		   the video mode dimensions.
		   fetch layer sub 1 must be setup first
		   todo:  add a check so that any sub layer can set this */
		if (is_fetch_layer_sub_chan1(chan)) {
			IMXDPU_TRACE("%s(): fetch layer sub channel 1\n",
				__func__);
			fwidth =
				imxdpu_array[imxdpu_id].
				video_mode[imxdpu_array[imxdpu_id].
				chan_data[chan_idx].disp_id].hlen;
			fheight =
				imxdpu_array[imxdpu_id].
				video_mode[imxdpu_array[imxdpu_id].
				chan_data[chan_idx].disp_id].vlen;

			imxdpu_write(imxdpu,
				b_off + IMXDPU_FETCHLAYER0_CONTROL_OFFSET, 0x700);

			imxdpu_write(imxdpu,
				b_off +
				IMXDPU_FETCHLAYER0_FRAMEDIMENSIONS_OFFSET,
				IMXDPU_SET_FIELD(IMXDPU_FRAMEHEIGHT,
					/*imxdpu->chan_data[chan_idx].dest_height-1 */
					fheight - 1) |
				IMXDPU_SET_FIELD(IMXDPU_FRAMEWIDTH,
					/*imxdpu->chan_data[chan_idx].dest_width-1 */
					fwidth - 1));
		}
		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE,
				enable_buffer) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE,
				enable_yuv) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip);
		imxdpu_write_block(imxdpu,
			b_off +
			IMXDPU_FETCHLAYER0_BASEADDRESS0_OFFSET +
			((IMXDPU_SUBCHAN_LAYER_OFFSET * sub_idx)),
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
		//imxdpu_write(imxdpu,
		//	b_off + IMXDPU_FETCHLAYER0_TRIGGERENABLE_OFFSET,
		//	get_channel_sub(chan));
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_warp_chan(chan)) {
		/* here the frame is shared for all sub layers so we use
		   the video mode dimensions.
		   fetch layer sub 1 must be setup first
		   todo:  add a check so that any sub layer can set this */
		if (is_fetch_layer_sub_chan1(chan)) {
			IMXDPU_TRACE("%s(): fetch layer sub channel 1\n",
				__func__);
			fwidth =
				imxdpu_array[imxdpu_id].
				video_mode[imxdpu_array[imxdpu_id].
				chan_data[chan_idx].disp_id].hlen;
			fheight =
				imxdpu_array[imxdpu_id].
				video_mode[imxdpu_array[imxdpu_id].
				chan_data[chan_idx].disp_id].vlen;

			imxdpu_write(imxdpu,
				b_off + IMXDPU_FETCHWARP2_CONTROL_OFFSET, 0x700);

			imxdpu_write(imxdpu,
				b_off +
				IMXDPU_FETCHLAYER0_FRAMEDIMENSIONS_OFFSET,
				IMXDPU_SET_FIELD(IMXDPU_FRAMEHEIGHT,
					/*imxdpu->chan_data[chan_idx].dest_height-1 */
					fheight - 1) |
				IMXDPU_SET_FIELD(IMXDPU_FRAMEWIDTH,
					/*imxdpu->chan_data[chan_idx].dest_width-1 */
					fwidth - 1));
		}
		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE,
				enable_buffer) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE,
				enable_yuv) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip);
		imxdpu_write_block(imxdpu,
			b_off +
			IMXDPU_FETCHWARP2_BASEADDRESS0_OFFSET +
			(IMXDPU_SUBCHAN_LAYER_OFFSET * sub_idx),
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);
		//imxdpu_write(imxdpu,
		//	b_off + IMXDPU_FETCHWARP2_TRIGGERENABLE_OFFSET,
		//	get_channel_sub(chan));
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_eco_chan(chan)) {
		IMXDPU_TRACE("%s(): fetch eco setup\n", __func__);
		if (imxdpu->chan_data[chan_idx].src_pixel_fmt == IMXDPU_PIX_FMT_NV12){
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 =  phyaddr_0 + u_offset;
			imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferattributes0 =
				IMXDPU_SET_FIELD(IMXDPU_BUFF_ATTR_BITSPERPIXEL, 16) |
				IMXDPU_SET_FIELD(IMXDPU_BUFF_ATTR_STRIDE,
				imxdpu->chan_data[chan_idx].stride - 1);

			/* chroma resolution*/
			imxdpu->chan_data[chan_idx].fetch_layer_prop.sourcebufferdimension0 =
				IMXDPU_SET_FIELD(IMXDPU_BUFF_DIMEN_LINECOUNT,
				imxdpu->chan_data[chan_idx].src_height / 2 - 1) |
				IMXDPU_SET_FIELD(IMXDPU_BUFF_DIMEN_LINEWIDTH,
				imxdpu->chan_data[chan_idx].src_width / 2 - 1);
			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentbits0 =
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSRED0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSGREEN0, 0x8) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSBLUE0, 0x8) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_BITSALPHA0, 0x0);

			imxdpu->chan_data[chan_idx].fetch_layer_prop.colorcomponentshift0 =
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTRED0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTGREEN0, 0x0) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTBLUE0, 0x8) |
				IMXDPU_SET_FIELD(IMXDPU_COLOR_SHIFTALPHA0, 0x0);

			imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
				IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE,
				enable_buffer) |
				//IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE,
				//enable_yuv) |
				IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE,
				enable_clip);

			imxdpu_write(imxdpu,
				b_off + IMXDPU_FETCHECO0_FRAMERESAMPLING_OFFSET,
				IMXDPU_SET_FIELD(IMXDPU_FETCHECO0_FRAMERESAMPLING_DELTAX, 0x2) |
				IMXDPU_SET_FIELD(IMXDPU_FETCHECO0_FRAMERESAMPLING_DELTAY, 0x2)
				);
			/* todo: handle all cases for control register */
			imxdpu_write(imxdpu,
				b_off + IMXDPU_FETCHECO0_CONTROL_OFFSET,
				IMXDPU_SET_FIELD(IMXDPU_FETCHECO0_CONTROL_CLIPCOLOR, 1));

			/* luma resolution */
			imxdpu_write(imxdpu,
				b_off + IMXDPU_FETCHECO0_FRAMEDIMENSIONS_OFFSET,
				IMXDPU_SET_FIELD
				(IMXDPU_FETCHECO0_FRAMEDIMENSIONS_FRAMEHEIGHT,
					imxdpu->chan_data[chan_idx].dest_height -
					1 /*fheight-1 */) |
				IMXDPU_SET_FIELD
				(IMXDPU_FETCHECO0_FRAMEDIMENSIONS_FRAMEWIDTH,
					imxdpu->chan_data[chan_idx].dest_width -
					1 /*fwidth-1 */));

		} /* else need to handle Alpha, Warp, CLUT ... */ 
		imxdpu->chan_data[chan_idx].fetch_layer_prop.layerproperty0 =
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE,
			//IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_YUVCONVERSIONMODE,
			enable_buffer) |
			//enable_yuv) |
			IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_CLIPWINDOWENABLE,
			enable_clip);

		imxdpu_write_block(imxdpu,
			b_off + IMXDPU_FETCHECO0_BASEADDRESS0_OFFSET,
			(void *)&imxdpu->chan_data[chan_idx].
			fetch_layer_prop,
			sizeof(fetch_layer_setup_t) / 4);

		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	}
#if 0
	/* test code here
	   todo: add layer properties */
	imxdpu_write(imxdpu, b_off + IMXDPU_FETCHDECODE0_LAYERPROPERTY0_OFFSET,
		     IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_RGBALPHACONSTENABLE,
				      IMXDPU_ENABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_ALPHACONSTENABLE,
				      IMXDPU_ENABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_SOURCEBUFFERENABLE,
				      IMXDPU_ENABLE) |
		     IMXDPU_SET_FIELD(IMXDPU_LAYERPROPERTY_ALPHASRCENABLE,
				      IMXDPU_ENABLE)
		     );
	IMXDPU_SET_FIELD(IMXDPU_COLOR_COMP_CONSTRED, 0) |
	IMXDPU_SET_FIELD(IMXDPU_COLOR_COMP_CONSTGREEN, 0) |
	IMXDPU_SET_FIELD(IMXDPU_COLOR_COMP_CONSTBLUE, 0) |
	IMXDPU_SET_FIELD(IMXDPU_COLOR_COMP_CONSTALPHA, 0xbf));

/* is this needed ? */
imxdpu_write(imxdpu, b_off + IMXDPU_FETCHDECODE0_CONTROLTRIGGER_OFFSET,
	     IMXDPU_FETCHDECODE0_CONTROLTRIGGER_SHDTOKGEN_MASK);
#endif

	//dump_channel(imxdpu_id, chan);

	return ret;
}

/*!
 * Intializes a channel
 *
 * @param       imxdpu_id       id of the diplay unit
 * @param	chan    	channel to update
 * @param	phyaddr_0       physical address
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
int32_t imxdpu_update_channel_buffer(
	int8_t imxdpu_id,
	imxdpu_chan_t chan,
	dma_addr_t phyaddr_0)
{
	int ret = 0;
	uint32_t b_off;     /* block offset for frame generator */
	struct imxdpu_soc *imxdpu;
	imxdpu_chan_idx_t chan_idx = get_channel_idx(chan);

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}

	b_off = id2blockoffset(get_channel_blk(chan));
	if (b_off == IMXDPU_OFFSET_INVALID) {
		return -EINVAL;
	}

	if (imxdpu->chan_data[chan_idx].use_eco_fetch == IMXDPU_FALSE) {
		imxdpu->chan_data[chan_idx].phyaddr_0 = phyaddr_0;
		imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 = phyaddr_0;
	}

	if (is_fetch_decode_chan(chan)) {
		IMXDPU_TRACE("%s(): fetch decode channel\n", __func__);
		if (imxdpu->chan_data[chan_idx].use_eco_fetch) {
			imxdpu_update_channel_buffer(imxdpu_id,
				imxdpu_get_eco(chan),
				phyaddr_0);
		}
		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHDECODE0_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_layer_chan(chan)) {
		IMXDPU_TRACE("%s(): fetch layer channel\n", __func__);
		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHLAYER0_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHLAYER0_TRIGGERENABLE_OFFSET,
			get_channel_sub(chan));
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_warp_chan(chan)) {
		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHWARP2_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHWARP2_TRIGGERENABLE_OFFSET,
			get_channel_sub(chan));
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	} else if (is_fetch_eco_chan(chan))  {
		IMXDPU_TRACE("%s(): fetch eco channel\n", __func__);

		imxdpu->chan_data[chan_idx].phyaddr_0 = phyaddr_0 + imxdpu->chan_data[chan_idx].u_offset;
		imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0 = imxdpu->chan_data[chan_idx].phyaddr_0;

		imxdpu_write(imxdpu,
			b_off + IMXDPU_FETCHDECODE0_BASEADDRESS0_OFFSET,
			imxdpu->chan_data[chan_idx].fetch_layer_prop.baseaddress0);
		imxdpu_disp_request_shadow_load(imxdpu_id,
			imxdpu->chan_data[chan_idx].
			disp_id,
			IMXDPU_SHDLD_IDX_CHAN_00 +
			chan_idx);
	}

	imxdpu_disp_request_shadow_load(imxdpu_id,
		imxdpu->chan_data[chan_idx].
		disp_id,
		imxdpu->chan_data[chan_idx].
		disp_id + IMXDPU_SHDLD_IDX_DISP0);

	return ret;
}

/*!
 * Intializes a channel
 *
 * @param       imxdpu_id       id of the diplay unit
 * @param	params  	pointer to channel parameters
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
*/
int imxdpu_init_channel(int8_t imxdpu_id, imxdpu_channel_params_t *params)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;
	imxdpu_chan_t chan = params->common.chan;
	imxdpu_chan_idx_t chan_idx = get_channel_idx(chan);
	/* here we use the video mode for channel frame width, todo: we may need to
	   add a paramter for this */

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}
	imxdpu->chan_data[chan_idx].chan = chan;

	memset(&imxdpu->chan_data[chan_idx].fetch_layer_prop, 0,
		sizeof(fetch_layer_setup_t));
	imxdpu->chan_data[chan_idx].use_eco_fetch = IMXDPU_FALSE;

	if (is_fetch_decode_chan(chan)) {
		IMXDPU_TRACE("%s(): decode channel setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_decode.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_decode.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_decode.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_decode.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_decode.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_decode.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_decode.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_decode.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_decode.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_decode.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_decode.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_decode.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_decode.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_decode.const_color;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_decode.disp_id;

		if (imxdpu->chan_data[chan_idx].use_video_proc ==
				IMXDPU_TRUE) {
			imxdpu->chan_data[chan_idx].h_scale_factor =
				params->fetch_decode.h_scale_factor;
			imxdpu->chan_data[chan_idx].h_phase =
				params->fetch_decode.h_phase;
			imxdpu->chan_data[chan_idx].v_scale_factor =
				params->fetch_decode.v_scale_factor;
			imxdpu->chan_data[chan_idx].v_phase[0][0] =
				params->fetch_decode.v_phase[0][0];
			imxdpu->chan_data[chan_idx].v_phase[0][1] =
				params->fetch_decode.v_phase[0][1];
			imxdpu->chan_data[chan_idx].v_phase[1][0] =
				params->fetch_decode.v_phase[1][0];
			imxdpu->chan_data[chan_idx].v_phase[1][1] =
				params->fetch_decode.v_phase[1][1];
		}

		if (imxdpu_get_planes(imxdpu->chan_data[chan_idx].src_pixel_fmt) == 2) {
			if (has_fetch_eco_chan(chan)) {
				imxdpu_channel_params_t temp_params = *params;

				imxdpu->chan_data[chan_idx].use_eco_fetch = IMXDPU_TRUE;
				temp_params.fetch_decode.chan = imxdpu_get_eco(params->fetch_decode.chan);
				imxdpu_init_channel(imxdpu_id, &temp_params);
			} else {
				//error
				return -EINVAL;
			}
		}
	} else if (is_fetch_layer_chan(chan)) {
		IMXDPU_TRACE("%s(): layer channel setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_layer.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_layer.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_layer.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_layer.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_layer.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_layer.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_layer.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_layer.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_layer.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_layer.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_layer.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_layer.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_layer.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_layer.const_color;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_layer.disp_id;

	} else if (is_fetch_warp_chan(chan)) {
		IMXDPU_TRACE("%s(): warp channel setup\n", __func__);

		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_warp.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_warp.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_warp.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_warp.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_warp.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_warp.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_warp.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_warp.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_warp.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_warp.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_warp.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_warp.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_warp.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_warp.const_color;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_warp.disp_id;

	} else if (is_fetch_eco_chan(chan)) {

		IMXDPU_TRACE("%s(): fetch eco channel setup\n", __func__);
		imxdpu->chan_data[chan_idx].src_pixel_fmt =
			params->fetch_decode.src_pixel_fmt;
		imxdpu->chan_data[chan_idx].src_width =
			params->fetch_decode.src_width;
		imxdpu->chan_data[chan_idx].src_height =
			params->fetch_decode.src_height;
		imxdpu->chan_data[chan_idx].clip_top =
			params->fetch_decode.clip_top;
		imxdpu->chan_data[chan_idx].clip_left =
			params->fetch_decode.clip_left;
		imxdpu->chan_data[chan_idx].clip_width =
			params->fetch_decode.clip_width;
		imxdpu->chan_data[chan_idx].clip_height =
			params->fetch_decode.clip_height;
		imxdpu->chan_data[chan_idx].stride =
			params->fetch_decode.stride;
		imxdpu->chan_data[chan_idx].dest_pixel_fmt =
			params->fetch_decode.dest_pixel_fmt;
		imxdpu->chan_data[chan_idx].dest_top =
			params->fetch_decode.dest_top;
		imxdpu->chan_data[chan_idx].dest_left =
			params->fetch_decode.dest_left;
		imxdpu->chan_data[chan_idx].dest_width =
			params->fetch_decode.dest_width;
		imxdpu->chan_data[chan_idx].dest_height =
			params->fetch_decode.dest_height;
		imxdpu->chan_data[chan_idx].const_color =
			params->fetch_decode.const_color;
		imxdpu->chan_data[chan_idx].disp_id =
			params->fetch_decode.disp_id;

		if (imxdpu->chan_data[chan_idx].use_video_proc ==
				IMXDPU_TRUE) {
			imxdpu->chan_data[chan_idx].h_scale_factor =
				params->fetch_decode.h_scale_factor;
			imxdpu->chan_data[chan_idx].h_phase =
				params->fetch_decode.h_phase;
			imxdpu->chan_data[chan_idx].v_scale_factor =
				params->fetch_decode.v_scale_factor;
			imxdpu->chan_data[chan_idx].v_phase[0][0] =
				params->fetch_decode.v_phase[0][0];
			imxdpu->chan_data[chan_idx].v_phase[0][1] =
				params->fetch_decode.v_phase[0][1];
			imxdpu->chan_data[chan_idx].v_phase[1][0] =
				params->fetch_decode.v_phase[1][0];
			imxdpu->chan_data[chan_idx].v_phase[1][1] =
				params->fetch_decode.v_phase[1][1];
		}

	} else if (is_store_chan(chan)) {
		IMXDPU_TRACE("%s(): store setup\n", __func__);
		return -EINVAL;
	} else {
		IMXDPU_TRACE("%s(): ERROR, invalid channel type!\n", __func__);
		return -EINVAL;
	}

	//dump_channel(imxdpu_id, chan);

	return ret;
}

/*!
 * Dumps the fetch layer properties structure for a channel.
 *
 * @param       layer   	id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void dump_fetch_layer(fetch_layer_setup_t *layer)
{
	IMXDPU_TRACE("baseaddress             0x%08x\n"
		"sourcebufferattributes  0x%08x\n"
		"sourcebufferdimension   h %d  w %d\n"
		"colorcomponentbits      0x%08x\n"
		"colorcomponentshift     0x%08x\n"
		"layeroffset             y(top) %d  x(left) %d\n"
		"clipwindowoffset        y(top) %d  x(left) %d\n"
		"clipwindowdimensions    h %d  w %d\n"
		"constantcolor           0x%08x\n"
		"layerproperty           0x%08x\n",
		layer->baseaddress0,
		layer->sourcebufferattributes0,
		layer->sourcebufferdimension0 >> 16,
		layer->sourcebufferdimension0 & 0x3fff,
		layer->colorcomponentbits0, layer->colorcomponentshift0,
		layer->layeroffset0 >> 16, layer->layeroffset0 & 0x3fff,
		layer->clipwindowoffset0 >> 16,
		layer->clipwindowoffset0 & 0x3fff,
		layer->clipwindowdimensions0 >> 16,
		layer->clipwindowdimensions0 & 0x3fff,
		layer->constantcolor0, layer->layerproperty0);
	return;
}

/*!
 * Dumps the pixel engine configuration status
 *
 * @param       imxdpu_id       id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void dump_layerblend(int8_t imxdpu_id)
{
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND0_STATUS);
	IMXDPU_TRACE("LAYERBLEND0_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND0_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND0_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND1_STATUS);
	IMXDPU_TRACE("LAYERBLEND1_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND1_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND1_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND2_STATUS);
	IMXDPU_TRACE("LAYERBLEND2_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND2_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND2_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND3_STATUS);
	IMXDPU_TRACE("LAYERBLEND3_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND3_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND3_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND4_STATUS);
	IMXDPU_TRACE("LAYERBLEND4_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND4_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND4_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND5_STATUS);
	IMXDPU_TRACE("LAYERBLEND5_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND5_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND5_LOCKSTATUS: 0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND6_STATUS);
	IMXDPU_TRACE("LAYERBLEND6_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_LAYERBLEND6_LOCKSTATUS);
	IMXDPU_TRACE("LAYERBLEND6_LOCKSTATUS: 0x%08x\n", reg);

	return;
}

/*!
 * Dumps the pixel engine configuration status
 *
 * @param       imxdpu_id       id of the diplay unit
 *
 * @return      This function returns 0 on success or negative error code on
 *      	fail.
 */
void dump_pixencfg_status(int8_t imxdpu_id)
{
	uint32_t reg;
	struct imxdpu_soc *imxdpu;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return;
	}
	imxdpu = &imxdpu_array[imxdpu_id];

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_REQUEST);
	IMXDPU_TRACE("EXTDST0_REQUEST:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_REQUEST);
	IMXDPU_TRACE("EXTDST1_REQUEST:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST4_REQUEST);
	IMXDPU_TRACE("EXTDST4_REQUEST:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST5_REQUEST);
	IMXDPU_TRACE("EXTDST5_REQUEST:     0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST0_STATUS);
	IMXDPU_TRACE("EXTDST0_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST1_STATUS);
	IMXDPU_TRACE("EXTDST1_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST4_STATUS);
	IMXDPU_TRACE("EXTDST4_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_EXTDST5_STATUS);
	IMXDPU_TRACE("EXTDST5_STATUS:     0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE2_STATUS);
	IMXDPU_TRACE("FETCHDECODE2_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE3_STATUS);
	IMXDPU_TRACE("FETCHDECODE3_STATUS:     0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHWARP2_STATUS);
	IMXDPU_TRACE("FETCHWARP2_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHECO2_STATUS);
	IMXDPU_TRACE("FETCHECO2_STATUS:     0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE0_STATUS);
	IMXDPU_TRACE("FETCHDECODE0_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHECO0_STATUS);
	IMXDPU_TRACE("FETCHECO0_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE1_STATUS);
	IMXDPU_TRACE("FETCHDECODE1_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHECO1_STATUS);
	IMXDPU_TRACE("FETCHECO1_STATUS:     0x%08x\n", reg);

	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHLAYER0_STATUS);
	IMXDPU_TRACE("FETCHLAYER0_STATUS:     0x%08x\n", reg);
	reg = imxdpu_read(imxdpu, IMXDPU_PIXENGCFG_FETCHLAYER1_STATUS);
	IMXDPU_TRACE("FETCHLAYER1_STATUS:     0x%08x\n", reg);

	return;
}

/*!
 * Dumps the channel data
 *
 * @param       imxdpu_id       id of the diplay unit
 * @param       chan    	channel to dump
 *
 * @return 	This function returns 0 on success or negative error code on
 *              fail.
 */
int dump_channel(int8_t imxdpu_id, imxdpu_chan_t chan)
{
	int ret = 0;
	struct imxdpu_soc *imxdpu;
	imxdpu_chan_idx_t chan_idx = get_channel_idx(chan);

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return -EINVAL;
	}

	imxdpu = &imxdpu_array[imxdpu_id];

	if (!is_chan(chan)) {
		return -EINVAL;
	}

	IMXDPU_TRACE("chan_id        0x%x\n"
		"src_pixel_fmt  0x%08x\n"
		"src_width      %d\n"
		"src_height     %d\n"
		"clip_top       %d(0x%04x)\n"
		"clip_left      %d(0x%04x)\n"
		"clip_width     %d\n"
		"clip_height    %d\n"
		"stride         %d\n"
		"dest_pixel_fmt 0x%08x\n"
		"dest_top       %d(0x%04x)\n"
		"dest_left      %d(0x%04x)\n"
		"dest_width     %d\n"
		"dest_height    %d\n",
		(uint32_t)imxdpu->chan_data[chan_idx].chan,
		imxdpu->chan_data[chan_idx].src_pixel_fmt,
		imxdpu->chan_data[chan_idx].src_width,
		imxdpu->chan_data[chan_idx].src_height,
		imxdpu->chan_data[chan_idx].clip_top,
		imxdpu->chan_data[chan_idx].clip_top,
		imxdpu->chan_data[chan_idx].clip_left,
		imxdpu->chan_data[chan_idx].clip_left,
		imxdpu->chan_data[chan_idx].clip_width,
		imxdpu->chan_data[chan_idx].clip_height,
		imxdpu->chan_data[chan_idx].stride,
		imxdpu->chan_data[chan_idx].dest_pixel_fmt,
		imxdpu->chan_data[chan_idx].dest_top,
		imxdpu->chan_data[chan_idx].dest_top,
		imxdpu->chan_data[chan_idx].dest_left,
		imxdpu->chan_data[chan_idx].dest_left,
		imxdpu->chan_data[chan_idx].dest_width,
		imxdpu->chan_data[chan_idx].dest_height);


	IMXDPU_TRACE(
		//"h_scale_factor %d\n"
		//"h_phase        %d\n"
		//"v_scale_factor %d\n"
		//"v_phase[0][0]  %d\n"
		//"v_phase[0][1]  %d\n"
		//"v_phase[0][0]  %d\n"
		//"v_phase[0][1]  %d\n"
		"use_video_proc %d\n"
		"use_eco_fetch  %d\n"
		"interlaced     %d\n"
		"phyaddr_0      0x%08x\n"
		"u_offset       0x%08x\n"
		"v_offset       0x%08x\n"
		//"store1_id      %d\n"
		//"fetch1_id      %d\n"
		//"fetch2_id      %d\n"
		//"fetch3_id      %d\n"
		"rot_mode       %d\n"
		"in_use         %d\n",
		//imxdpu->chan_data[chan_idx].h_scale_factor,
		//imxdpu->chan_data[chan_idx].h_phase,
		//imxdpu->chan_data[chan_idx].v_scale_factor,
		//imxdpu->chan_data[chan_idx].v_phase[0][0],
		//imxdpu->chan_data[chan_idx].v_phase[0][1],
		//imxdpu->chan_data[chan_idx].v_phase[0][0],
		//imxdpu->chan_data[chan_idx].v_phase[0][1],
		imxdpu->chan_data[chan_idx].use_video_proc,
		imxdpu->chan_data[chan_idx].use_eco_fetch,
		imxdpu->chan_data[chan_idx].interlaced,
		ptr_to_uint32(imxdpu->chan_data[chan_idx].phyaddr_0),
		imxdpu->chan_data[chan_idx].u_offset,
		imxdpu->chan_data[chan_idx].v_offset,
		//imxdpu->chan_data[chan_idx].store1_id,
		//imxdpu->chan_data[chan_idx].fetch1_id,
		//imxdpu->chan_data[chan_idx].fetch2_id,
		//imxdpu->chan_data[chan_idx].fetch3_id,
		imxdpu->chan_data[chan_idx].rot_mode,
		imxdpu->chan_data[chan_idx].in_use
		);

	dump_fetch_layer(&imxdpu->chan_data[chan_idx].fetch_layer_prop);

	return ret;
}

/*!
 * Shows the interrupt status registers
 *
 * @param   id of the diplay unit
 *
 */
void dump_int_stat(int8_t imxdpu_id)
{
	int i;
	struct imxdpu_soc *imxdpu;
	uint32_t reg;

	IMXDPU_TRACE("%s()\n", __func__);

	if (!((imxdpu_id >= 0) && (imxdpu_id < IMXDPU_MAX_NUM))) {
		return;     // -EINVAL;
	}

	imxdpu = &imxdpu_array[imxdpu_id];

	for (i = 0; i < 3; i++) {
		reg = imxdpu_read_irq(imxdpu,
			IMXDPU_COMCTRL_USERINTERRUPTMASK0 +
			(i * 4));
		IMXDPU_TRACE("USERINTERRUPTMASK%d:   0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpu_read_irq(imxdpu,
			IMXDPU_COMCTRL_USERINTERRUPTENABLE0 +
			(i * 4));
		IMXDPU_TRACE("USERINTERRUPTENABLE%d: 0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpu_read_irq(imxdpu,
			IMXDPU_COMCTRL_USERINTERRUPTSTATUS0 +
			(i * 4));
		IMXDPU_TRACE("USERINTERRUPTSTATUS%d: 0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpu_read_irq(imxdpu,
			IMXDPU_COMCTRL_INTERRUPTENABLE0 + (i * 4));
		IMXDPU_TRACE("INTERRUPTENABLE%i:     0x%08x\n", i, reg);
	}
	for (i = 0; i < 3; i++) {
		reg = imxdpu_read_irq(imxdpu,
			IMXDPU_COMCTRL_INTERRUPTSTATUS0 + (i * 4));
		IMXDPU_TRACE("INTERRUPTSTATUS%i:     0x%08x\n", i, reg);
	}
}
