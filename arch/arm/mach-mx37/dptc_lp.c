/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file dptc_lp.c
 *
 * @brief DPTC LP table for the Freescale Semiconductor MXC DPTC LP module.
 *
 * @ingroup PM
 */

#include <asm/arch/hardware.h>

struct dptc_wp dptc_lp_wp_allfreq[DPTC_LP_WP_SUPPORTED] = {
	/* 532MHz */
	/* dcvr0          dcvr1             dcvr2           dcvr3      regulator  voltage */
	/* wp0 */
	{DCVR(141, 143, 149), DCVR(155, 157, 162), DCVR(106, 108, 112),
	 DCVR(124, 126, 130), "DCDC4", 1200},
	{DCVR(141, 143, 149), DCVR(155, 157, 162), DCVR(106, 108, 113),
	 DCVR(124, 126, 131), "DCDC4", 1175},
	{DCVR(141, 144, 150), DCVR(155, 157, 163), DCVR(106, 108, 113),
	 DCVR(124, 126, 131), "DCDC4", 1150},
	{DCVR(141, 144, 151), DCVR(155, 157, 163), DCVR(106, 108, 114),
	 DCVR(124, 126, 131), "DCDC4", 1125},
	{DCVR(142, 144, 152), DCVR(155, 157, 163), DCVR(107, 109, 114),
	 DCVR(125, 127, 132), "DCDC4", 1100},
	{DCVR(142, 145, 153), DCVR(155, 157, 164), DCVR(107, 109, 115),
	 DCVR(125, 127, 133), "DCDC4", 1075},
	{DCVR(142, 145, 153), DCVR(155, 158, 164), DCVR(107, 109, 116),
	 DCVR(125, 127, 133), "DCDC4", 1050},
	{DCVR(142, 145, 154), DCVR(155, 158, 165), DCVR(107, 110, 117),
	 DCVR(125, 127, 134), "DCDC4", 1025},
	{DCVR(142, 146, 156), DCVR(155, 158, 165), DCVR(107, 110, 117),
	 DCVR(125, 128, 135), "DCDC4", 1000},
};
