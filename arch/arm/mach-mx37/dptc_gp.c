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
 * @file dptc_gp.c
 *
 * @brief DPTC table for the Freescale Semiconductor MXC DPTC module.
 *
 * @ingroup PM
 */

#include <asm/arch/hardware.h>
#include <asm/arch/hardware.h>

struct dptc_wp dptc_gp_wp_allfreq[DPTC_GP_WP_SUPPORTED] = {
	/* 532MHz */
	/* dcvr0          dcvr1             dcvr2           dcvr3      regulator  voltage */
	/* wp0 */
	{DCVR(107, 108, 112), DCVR(122, 123, 127), DCVR(133, 134, 139),
	 DCVR(115, 116, 121), "DCDC1", 1000},
	{DCVR(107, 108, 113), DCVR(122, 123, 127), DCVR(133, 134, 139),
	 DCVR(115, 117, 122), "DCDC1", 975},
	{DCVR(107, 109, 113), DCVR(122, 123, 127), DCVR(133, 134, 139),
	 DCVR(115, 117, 122), "DCDC1", 950},
	{DCVR(107, 109, 114), DCVR(122, 123, 127), DCVR(133, 135, 140),
	 DCVR(115, 117, 122), "DCDC1", 925},
	{DCVR(108, 109, 115), DCVR(122, 123, 127), DCVR(133, 136, 142),
	 DCVR(115, 117, 123), "DCDC1", 900},
	{DCVR(108, 110, 115), DCVR(122, 123, 127), DCVR(133, 136, 142),
	 DCVR(115, 117, 123), "DCDC1", 875},
	{DCVR(108, 110, 115), DCVR(122, 124, 128), DCVR(133, 136, 143),
	 DCVR(115, 118, 124), "DCDC1", 850},
};
