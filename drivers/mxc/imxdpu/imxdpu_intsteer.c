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
#include "imxdpu_intsteer.h"
/* Interrupt Steer Mapping                  */
/*  Interupt steer input line  ---------+   */
/*  Interrupt Steer Group ---+          |   */
/*  IMXDPU Interrupt Line--+ |          |   */
/*                         | |          |   */
/*                         v v          |   */
/*               DC_IMXDPU_3_0_         v   */

#define DC_IMXDPU_0_IRQ_STEER		448
#define DC_IMXDPU_1_IRQ_STEER		449
#define DC_IMXDPU_2_IRQ_STEER		450
#define DC_IMXDPU_3_0_IRQ_STEER		64
#define DC_IMXDPU_4_0_IRQ_STEER		65
#define DC_IMXDPU_5_0_IRQ_STEER		66
#define DC_IMXDPU_6_0_IRQ_STEER		67
#define DC_IMXDPU_7_0_IRQ_STEER		68
#define DC_IMXDPU_8_0_IRQ_STEER		69
#define DC_IMXDPU_3_1_IRQ_STEER		128
#define DC_IMXDPU_4_1_IRQ_STEER		129
#define DC_IMXDPU_5_1_IRQ_STEER		130
#define DC_IMXDPU_6_1_IRQ_STEER		131
#define DC_IMXDPU_7_1_IRQ_STEER		132
#define DC_IMXDPU_8_1_IRQ_STEER		133
#define DC_IMXDPU_9_0_IRQ_STEER		70
#define DC_IMXDPU_9_1_IRQ_STEER		134
#define DC_IMXDPU_9_2_IRQ_STEER		192
#define DC_IMXDPU_9_3_IRQ_STEER		256
#define DC_IMXDPU_10_0_IRQ_STEER	193
#define DC_IMXDPU_11_0_IRQ_STEER	194
#define DC_IMXDPU_12_0_IRQ_STEER	195
#define DC_IMXDPU_13_0_IRQ_STEER	196
#define DC_IMXDPU_14_0_IRQ_STEER	197
#define DC_IMXDPU_10_1_IRQ_STEER	257
#define DC_IMXDPU_11_1_IRQ_STEER	258
#define DC_IMXDPU_12_1_IRQ_STEER	259
#define DC_IMXDPU_13_1_IRQ_STEER	260
#define DC_IMXDPU_14_1_IRQ_STEER	261
#define DC_IMXDPU_15_IRQ_STEER		320
#define DC_IMXDPU_16_IRQ_STEER		321
#define DC_IMXDPU_17_IRQ_STEER		322
#define DC_IMXDPU_18_IRQ_STEER		384
#define DC_IMXDPU_19_IRQ_STEER		385
#define DC_IMXDPU_20_IRQ_STEER		386
#define DC_IMXDPU_22_IRQ_STEER		323
#define DC_IMXDPU_24_IRQ_STEER		387
#define DC_IMXDPU_25_0_IRQ_STEER	71
#define DC_IMXDPU_25_1_IRQ_STEER	135
#define DC_IMXDPU_26_0_IRQ_STEER	198
#define DC_IMXDPU_26_1_IRQ_STEER	262
#define DC_IMXDPU_27_0_IRQ_STEER	72
#define DC_IMXDPU_28_0_IRQ_STEER	73
#define DC_IMXDPU_29_0_IRQ_STEER	74
#define DC_IMXDPU_30_0_IRQ_STEER	75
#define DC_IMXDPU_31_0_IRQ_STEER	76
#define DC_IMXDPU_32_0_IRQ_STEER	77
#define DC_IMXDPU_33_0_IRQ_STEER	78
#define DC_IMXDPU_34_0_IRQ_STEER	79
#define DC_IMXDPU_35_0_IRQ_STEER	80
#define DC_IMXDPU_36_0_IRQ_STEER	81
#define DC_IMXDPU_27_1_IRQ_STEER	136
#define DC_IMXDPU_28_1_IRQ_STEER	137
#define DC_IMXDPU_29_1_IRQ_STEER	138
#define DC_IMXDPU_30_1_IRQ_STEER	139
#define DC_IMXDPU_31_1_IRQ_STEER	140
#define DC_IMXDPU_32_1_IRQ_STEER	141
#define DC_IMXDPU_33_1_IRQ_STEER	142
#define DC_IMXDPU_34_1_IRQ_STEER	143
#define DC_IMXDPU_35_1_IRQ_STEER	144
#define DC_IMXDPU_36_1_IRQ_STEER	145
#define DC_IMXDPU_37_0_IRQ_STEER	199
#define DC_IMXDPU_38_0_IRQ_STEER	200
#define DC_IMXDPU_39_0_IRQ_STEER	201
#define DC_IMXDPU_40_0_IRQ_STEER	202
#define DC_IMXDPU_41_0_IRQ_STEER	203
#define DC_IMXDPU_42_0_IRQ_STEER	204
#define DC_IMXDPU_43_0_IRQ_STEER	205
#define DC_IMXDPU_44_0_IRQ_STEER	206
#define DC_IMXDPU_45_0_IRQ_STEER	207
#define DC_IMXDPU_46_0_IRQ_STEER	208
#define DC_IMXDPU_37_1_IRQ_STEER	263
#define DC_IMXDPU_38_1_IRQ_STEER	264
#define DC_IMXDPU_39_1_IRQ_STEER	265
#define DC_IMXDPU_40_1_IRQ_STEER	266
#define DC_IMXDPU_41_1_IRQ_STEER	267
#define DC_IMXDPU_42_1_IRQ_STEER	268
#define DC_IMXDPU_43_1_IRQ_STEER	269
#define DC_IMXDPU_44_1_IRQ_STEER	270
#define DC_IMXDPU_45_1_IRQ_STEER	271
#define DC_IMXDPU_46_1_IRQ_STEER	272
#define DC_IMXDPU_47_0_IRQ_STEER	324
#define DC_IMXDPU_47_1_IRQ_STEER	388
#define DC_IMXDPU_48_0_IRQ_STEER	389
#define DC_IMXDPU_48_1_IRQ_STEER	451
#define DC_IMXDPU_50_0_IRQ_STEER	0
#define DC_IMXDPU_50_1_IRQ_STEER	452
#define DC_IMXDPU_51_IRQ_STEER		1
#define DC_IMXDPU_52_IRQ_STEER		2
#define DC_IMXDPU_53_IRQ_STEER		3
#define DC_IMXDPU_54_IRQ_STEER		4
#define DC_IMXDPU_55_0_IRQ_STEER	82
#define DC_IMXDPU_56_0_IRQ_STEER	83
#define DC_IMXDPU_57_0_IRQ_STEER	84
#define DC_IMXDPU_58_0_IRQ_STEER	85
#define DC_IMXDPU_55_1_IRQ_STEER	146
#define DC_IMXDPU_56_1_IRQ_STEER	147
#define DC_IMXDPU_57_1_IRQ_STEER	148
#define DC_IMXDPU_58_1_IRQ_STEER	149
#define DC_IMXDPU_59_0_IRQ_STEER	209
#define DC_IMXDPU_60_0_IRQ_STEER	210
#define DC_IMXDPU_61_0_IRQ_STEER	211
#define DC_IMXDPU_62_0_IRQ_STEER	212
#define DC_IMXDPU_59_1_IRQ_STEER	273
#define DC_IMXDPU_60_1_IRQ_STEER	274
#define DC_IMXDPU_61_1_IRQ_STEER	275
#define DC_IMXDPU_62_1_IRQ_STEER	276
#define DC_IMXDPU_63_IRQ_STEER		325
#define DC_IMXDPU_64_IRQ_STEER		326
#define DC_IMXDPU_65_IRQ_STEER		390
#define DC_IMXDPU_66_IRQ_STEER		391

#define DC_IMXDPU_RESERVED              -1

/* imxdpu_int_steer_map provides a static mapping for each of the 
 *   IMXDPU Interrupts. Only one of the inputs is mapped here. 
 */
#define STEER_MAP_SIZE 67
short imxdpu_int_steer_map[STEER_MAP_SIZE] = {
	DC_IMXDPU_0_IRQ_STEER,	/* IMXDPU_STORE9_SHDLOAD_IRQ */
	DC_IMXDPU_1_IRQ_STEER,	/* IMXDPU_STORE9_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_2_IRQ_STEER,	/* IMXDPU_STORE9_SEQCOMPLETE_IRQ */

	DC_IMXDPU_3_0_IRQ_STEER,	/* IMXDPU_EXTDST0_SHDLOAD_IRQ */
	DC_IMXDPU_4_0_IRQ_STEER,	/* IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_5_0_IRQ_STEER,	/* IMXDPU_EXTDST0_SEQCOMPLETE_IRQ */
	DC_IMXDPU_6_0_IRQ_STEER,	/* IMXDPU_EXTDST4_SHDLOAD_IRQ */
	DC_IMXDPU_7_0_IRQ_STEER,	/* IMXDPU_EXTDST4_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_8_0_IRQ_STEER,	/* IMXDPU_EXTDST4_SEQCOMPLETE_IRQ */
	//DC_IMXDPU_3_1_IRQ_STEER,    /* IMXDPU_EXTDST0_SHDLOAD_IRQ */
	//DC_IMXDPU_4_1_IRQ_STEER,    /* IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ */
	//DC_IMXDPU_5_1_IRQ_STEER,    /* IMXDPU_EXTDST0_SEQCOMPLETE_IRQ */
	//DC_IMXDPU_6_1_IRQ_STEER,    /* IMXDPU_EXTDST4_SHDLOAD_IRQ */
	//DC_IMXDPU_7_1_IRQ_STEER,    /* IMXDPU_EXTDST4_FRAMECOMPLETE_IRQ */
	//DC_IMXDPU_8_1_IRQ_STEER,    /* IMXDPU_EXTDST4_SEQCOMPLETE_IRQ */

	//DC_IMXDPU_9_0_IRQ_STEER,    /* IMXDPU_EXTDST1_SHDLOAD_IRQ */
	//DC_IMXDPU_9_1_IRQ_STEER,    /* IMXDPU_EXTDST1_SHDLOAD_IRQ */
	DC_IMXDPU_9_2_IRQ_STEER,	/* IMXDPU_EXTDST1_SHDLOAD_IRQ */
	//DC_IMXDPU_9_3_IRQ_STEER,    /* IMXDPU_EXTDST1_SHDLOAD_IRQ */

	DC_IMXDPU_10_0_IRQ_STEER,	/* IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_11_0_IRQ_STEER,	/* IMXDPU_EXTDST1_SEQCOMPLETE_IRQ */
	DC_IMXDPU_12_0_IRQ_STEER,	/* IMXDPU_EXTDST5_SHDLOAD_IRQ */
	DC_IMXDPU_13_0_IRQ_STEER,	/* IMXDPU_EXTDST5_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_14_0_IRQ_STEER,	/* IMXDPU_EXTDST5_SEQCOMPLETE_IRQ */
	//DC_IMXDPU_10_1_IRQ_STEER,   /* IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ */
	//DC_IMXDPU_11_1_IRQ_STEER,   /* IMXDPU_EXTDST1_SEQCOMPLETE_IRQ */
	//DC_IMXDPU_12_1_IRQ_STEER,   /* IMXDPU_EXTDST5_SHDLOAD_IRQ */
	//DC_IMXDPU_13_1_IRQ_STEER,   /* IMXDPU_EXTDST5_FRAMECOMPLETE_IRQ */
	//DC_IMXDPU_14_1_IRQ_STEER,   /* IMXDPU_EXTDST5_SEQCOMPLETE_IRQ */

	DC_IMXDPU_15_IRQ_STEER,	/* IMXDPU_STORE4_SHDLOAD_IRQ */
	DC_IMXDPU_16_IRQ_STEER,	/* IMXDPU_STORE4_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_17_IRQ_STEER,	/* IMXDPU_STORE4_SEQCOMPLETE_IRQ */
	DC_IMXDPU_18_IRQ_STEER,	/* IMXDPU_STORE5_SHDLOAD_IRQ */
	DC_IMXDPU_19_IRQ_STEER,	/* IMXDPU_STORE5_FRAMECOMPLETE_IRQ */
	DC_IMXDPU_20_IRQ_STEER,	/* IMXDPU_STORE5_SEQCOMPLETE_IRQ */

	DC_IMXDPU_RESERVED,	/* IMXDPU_RESERVED21_IRQ */
	DC_IMXDPU_22_IRQ_STEER,	/* IMXDPU_HISTOGRAM4_VALID_IRQ */
	DC_IMXDPU_RESERVED,	/* IMXDPU_RESERVED23_IRQ */
	DC_IMXDPU_24_IRQ_STEER,	/* IMXDPU_HISTOGRAM5_VALID_IRQ */

	//DC_IMXDPU_25_0_IRQ_STEER,   /* IMXDPU_FRAMEDUMP0_ERROR_IRQ */
	DC_IMXDPU_25_1_IRQ_STEER,	/* IMXDPU_FRAMEDUMP0_ERROR_IRQ */
	//DC_IMXDPU_26_0_IRQ_STEER,   /* IMXDPU_FRAMEDUMP1_ERROR_IRQ */
	DC_IMXDPU_26_1_IRQ_STEER,	/* IMXDPU_FRAMEDUMP1_ERROR_IRQ */

	DC_IMXDPU_27_0_IRQ_STEER,	/* IMXDPU_DISENGCFG_SHDLOAD0_IRQ */
	DC_IMXDPU_28_0_IRQ_STEER,	/* IMXDPU_DISENGCFG_FRAMECOMPLETE0_IRQ */
	DC_IMXDPU_29_0_IRQ_STEER,	/* IMXDPU_DISENGCFG_SEQCOMPLETE0_IRQ */
	DC_IMXDPU_30_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_INT0_IRQ */
	DC_IMXDPU_31_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_INT1_IRQ */
	DC_IMXDPU_32_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_INT2_IRQ */
	DC_IMXDPU_33_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_INT3_IRQ */
	DC_IMXDPU_34_0_IRQ_STEER,	/* IMXDPU_SIG0_SHDLOAD_IRQ */
	DC_IMXDPU_35_0_IRQ_STEER,	/* IMXDPU_SIG0_VALID_IRQ */
	DC_IMXDPU_36_0_IRQ_STEER,	/* IMXDPU_SIG0_ERROR_IRQ */
	//DC_IMXDPU_27_1_IRQ_STEER,   /* IMXDPU_DISENGCFG_SHDLOAD0_IRQ */
	//DC_IMXDPU_28_1_IRQ_STEER,   /* IMXDPU_DISENGCFG_FRAMECOMPLETE0_IRQ */
	//DC_IMXDPU_29_1_IRQ_STEER,   /* IMXDPU_DISENGCFG_SEQCOMPLETE0_IRQ */
	//DC_IMXDPU_30_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_INT0_IRQ */
	//DC_IMXDPU_31_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_INT1_IRQ */
	//DC_IMXDPU_32_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_INT2_IRQ */
	//DC_IMXDPU_33_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_INT3_IRQ */
	//DC_IMXDPU_34_1_IRQ_STEER,   /* IMXDPU_SIG0_SHDLOAD_IRQ */
	//DC_IMXDPU_35_1_IRQ_STEER,   /* IMXDPU_SIG0_VALID_IRQ */
	//DC_IMXDPU_36_1_IRQ_STEER,   /* IMXDPU_SIG0_ERROR_IRQ */

	DC_IMXDPU_37_0_IRQ_STEER,	/* IMXDPU_DISENGCFG_SHDLOAD1_IRQ */
	DC_IMXDPU_38_0_IRQ_STEER,	/* IMXDPU_DISENGCFG_FRAMECOMPLETE1_IRQ */
	DC_IMXDPU_39_0_IRQ_STEER,	/* IMXDPU_DISENGCFG_SEQCOMPLETE1_IRQ */
	DC_IMXDPU_40_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_INT0_IRQ */
	DC_IMXDPU_41_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_INT1_IRQ */
	DC_IMXDPU_42_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_INT2_IRQ */
	DC_IMXDPU_43_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_INT3_IRQ */
	DC_IMXDPU_44_0_IRQ_STEER,	/* IMXDPU_SIG1_SHDLOAD_IRQ */
	DC_IMXDPU_45_0_IRQ_STEER,	/* IMXDPU_SIG1_VALID_IRQ */
	DC_IMXDPU_46_0_IRQ_STEER,	/* IMXDPU_SIG1_ERROR_IRQ */
	//DC_IMXDPU_37_1_IRQ_STEER,   /* IMXDPU_DISENGCFG_SHDLOAD1_IRQ */
	//DC_IMXDPU_38_1_IRQ_STEER,   /* IMXDPU_DISENGCFG_FRAMECOMPLETE1_IRQ */
	//DC_IMXDPU_39_1_IRQ_STEER,   /* IMXDPU_DISENGCFG_SEQCOMPLETE1_IRQ */
	//DC_IMXDPU_40_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_INT0_IRQ */
	//DC_IMXDPU_41_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_INT1_IRQ */
	//DC_IMXDPU_42_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_INT2_IRQ */
	//DC_IMXDPU_43_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_INT3_IRQ */
	//DC_IMXDPU_44_1_IRQ_STEER,   /* IMXDPU_SIG1_SHDLOAD_IRQ */
	//DC_IMXDPU_45_1_IRQ_STEER,   /* IMXDPU_SIG1_VALID_IRQ */
	//DC_IMXDPU_46_1_IRQ_STEER,   /* IMXDPU_SIG1_ERROR_IRQ */

	DC_IMXDPU_47_0_IRQ_STEER,	/* IMXDPU_ITUIFC4_ERROR_IRQ */
	//DC_IMXDPU_47_1_IRQ_STEER,   /* IMXDPU_ITUIFC4_ERROR_IRQ */
	DC_IMXDPU_48_0_IRQ_STEER,	/* IMXDPU_ITUIFC5_ERROR_IRQ */
	//DC_IMXDPU_48_1_IRQ_STEER,   /* IMXDPU_ITUIFC5_ERROR_IRQ */

	DC_IMXDPU_RESERVED,	/* IMXDPU_RESERVED49_IRQ */
	DC_IMXDPU_50_0_IRQ_STEER,	/* IMXDPU_CMDSEQ_ERROR_IRQ */
	//DC_IMXDPU_50_1_IRQ_STEER,   /* IMXDPU_CMDSEQ_ERROR_IRQ */

	DC_IMXDPU_51_IRQ_STEER,	/* IMXDPU_COMCTRL_SW0_IRQ */
	DC_IMXDPU_52_IRQ_STEER,	/* IMXDPU_COMCTRL_SW1_IRQ */
	DC_IMXDPU_53_IRQ_STEER,	/* IMXDPU_COMCTRL_SW2_IRQ */
	DC_IMXDPU_54_IRQ_STEER,	/* IMXDPU_COMCTRL_SW3_IRQ */

	DC_IMXDPU_55_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_PRIMSYNC_ON_IRQ */
	DC_IMXDPU_56_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_PRIMSYNC_OFF_IRQ */
	DC_IMXDPU_57_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_SECSYNC_ON_IRQ */
	DC_IMXDPU_58_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN0_SECSYNC_OFF_IRQ */
	//DC_IMXDPU_55_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_PRIMSYNC_ON_IRQ */
	//DC_IMXDPU_56_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_PRIMSYNC_OFF_IRQ */
	//DC_IMXDPU_57_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_SECSYNC_ON_IRQ */
	//DC_IMXDPU_58_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN0_SECSYNC_OFF_IRQ */
	DC_IMXDPU_59_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_PRIMSYNC_ON_IRQ */
	DC_IMXDPU_60_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_PRIMSYNC_OFF_IRQ */
	DC_IMXDPU_61_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_SECSYNC_ON_IRQ */
	DC_IMXDPU_62_0_IRQ_STEER,	/* IMXDPU_FRAMEGEN1_SECSYNC_OFF_IRQ */
	//DC_IMXDPU_59_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_PRIMSYNC_ON_IRQ */
	//DC_IMXDPU_60_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_PRIMSYNC_OFF_IRQ */
	//DC_IMXDPU_61_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_SECSYNC_ON_IRQ */
	//DC_IMXDPU_62_1_IRQ_STEER,   /* IMXDPU_FRAMEGEN1_SECSYNC_OFF_IRQ */
	DC_IMXDPU_63_IRQ_STEER,	/* IMXDPU_FRAMECAP4_SYNC_ON_IRQ */
	DC_IMXDPU_64_IRQ_STEER,	/* IMXDPU_FRAMECAP4_SYNC_OFF_IRQ */
	DC_IMXDPU_65_IRQ_STEER,	/* IMXDPU_FRAMECAP5_SYNC_ON_IRQ */
	DC_IMXDPU_66_IRQ_STEER,	/* IMXDPU_FRAMECAP5_SYNC_OFF_IRQ */
};

/*!
 * This function returns the interrupt steer line (input) for the given IMXDPU interrupt 
 *
 * @param	imxdpu_irq 		imxdpu interrupt number
 * 
 * @return      This function returns -1 on failure or a interrupt steer line number
 *
 */
int imxdpu_intsteer_get_line(int imxdpu_irq)
{
	if ((imxdpu_irq < 0) || (imxdpu_irq >= STEER_MAP_SIZE)) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	return imxdpu_int_steer_map[imxdpu_irq];
}

/*!
 * This function returns the interrupt register offset for the given IMXDPU interrupt 
 *
 * @param	imxdpu_irq 		imxdpu interrupt number
 * 
 * @return      This function returns -1 on failure or a valid register offset
 *
 */
int imxdpu_intsteer_get_reg_offset(int imxdpu_irq)
{
	int line = imxdpu_intsteer_get_line(imxdpu_irq);
	if (line < 0) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	return ((15 - (line / 32)) * 4);
}

/*!
 * This function returns the bit shift for the interrupt steer registers for
 * the given IMXDPU interrupt 
 *
 * @param	imxdpu_irq 		imxdpu interrupt number
 * 
 * @return      This function returns -1 on failure or a valid bit shift.
 *
 */
int imxdpu_intsteer_get_reg_shift(int imxdpu_irq)
{
	int line = imxdpu_intsteer_get_line(imxdpu_irq);
	if (line < 0) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	return line % 32;
}

/*!
 * This function returns the irq index (interrupt steer output) for the
 * given IMXDPU interrupt. 
 *
 * @param	imxdpu_irq 		imxdpu interrupt number
 * 
 * @return      This function returns -1 on failure or a valid bit shift.
 *
 */
int imxdpu_intsteer_get_irq_offset(int imxdpu_irq)
{
	int line = imxdpu_intsteer_get_line(imxdpu_irq);
	if (line < 0) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	return 7 - (line / 64);
}

/*!
 * This function returns the irq index (interrupt steer output) for the
 * given IMXDPU interrupt. 
 *
 * @param	imxdpu_irq 		imxdpu interrupt number
 * 
 * @return      This function returns -1 on failure or a valid bit shift.
 *
 */
int imxdpu_intsteer_map_irq(int imxdpu_irq)
{
	int line = imxdpu_intsteer_get_line(imxdpu_irq);
	if (line < 0) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	return 7 - (line / 64);
}

uint32_t imxdpu_ss_read(void __iomem * ss_base, uint32_t offset)
{
	uint32_t val = 0;
	val = __raw_readl(ss_base + offset);
	return val;
}

void imxdpu_ss_write(void __iomem * ss_base, uint32_t offset, uint32_t value)
{
	__raw_writel(value, ss_base + offset);
}

int imxdpu_intsteer_enable_irq(void __iomem * ss_base, int imxdpu_irq)
{
	int line = imxdpu_intsteer_get_line(imxdpu_irq);
	int offset;
	int shift;
	uint32_t reg;

	if (line < 0) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	offset = imxdpu_intsteer_get_reg_offset(imxdpu_irq);
	shift = imxdpu_intsteer_get_reg_shift(imxdpu_irq);

	/* FIXME: do we need a lock here.? */
	reg = imxdpu_ss_read(ss_base, IMXDPU_IRQSTEER_CHnMASKn_OFFSET(offset));
	reg |= 1 << shift;
	imxdpu_ss_write(ss_base, IMXDPU_IRQSTEER_CHnMASKn_OFFSET(offset), reg);

	pr_debug("%s(): line %d, offset %d, shift %d\n", __func__, line, offset, 
		shift);

	return line;
}

int imxdpu_intsteer_disable_irq(void __iomem * ss_base, int imxdpu_irq)
{
	int line = imxdpu_intsteer_get_line(imxdpu_irq);
	int offset;
	int shift;
	uint32_t reg;

	if (line < 0) {
		pr_err("%s() invalid line\n", __func__);
		return -1;
	}
	offset = imxdpu_intsteer_get_reg_offset(imxdpu_irq);
	shift = imxdpu_intsteer_get_reg_shift(imxdpu_irq);

	/* FIXME: do we need a lock here? */
	reg = imxdpu_ss_read(ss_base, IMXDPU_IRQSTEER_CHnMASKn_OFFSET(offset));
	reg &= ~(1 << shift);
	imxdpu_ss_write(ss_base, IMXDPU_IRQSTEER_CHnMASKn_OFFSET(offset), reg);

	pr_debug("%s(): line %d, offset %d, shift %d\n", __func__, line, offset,
		shift);

	return line;
}
