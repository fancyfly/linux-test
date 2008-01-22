/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file dptc.h
 *
 * @brief MXC91321 dvfs & dptc table file.
 *
 * @ingroup PM_MXC91321
 */
#ifndef __DVFS_DPTC_TABLE_MXC91321_H__
#define __DVFS_DPTC_TABLE_MXC91321_H__

/*!
 * Default DPTC table definition
 */
#define NUM_OF_FREQS 1
#define NUM_OF_WP    17

static char *default_table_str = "WORKING POINT 17\n\
\n\
WP 0x1c\n\
WP 0x1b\n\
WP 0x1a\n\
WP 0x19\n\
WP 0x18\n\
WP 0x17\n\
WP 0x16\n\
WP 0x15\n\
WP 0x14\n\
WP 0x13\n\
WP 0x12\n\
WP 0x11\n\
WP 0x10\n\
WP 0xf\n\
WP 0xe\n\
WP 0xd\n\
WP 0xc\n\
\n\
DCVR 0x7fe00000 0x46489d11 0x820fe9f9 0x6ccd55a7\n\
DCVR 0x7fe00000 0x46689d11 0x826fedf9 0x6d2d59a7\n\
DCVR 0x7fe00000 0x46a8a111 0x82cff5fa 0x6d6d5da8\n\
DCVR 0x7fe00000 0x46c8a111 0x832ff9fa 0x6dcd61a8\n\
DCVR 0x7fe00000 0x46e8a111 0x838ffdfa 0x6e0d65a8\n\
DCVR 0x7fe00000 0x4708a511 0x83affdfa 0x6e4d65a8\n\
DCVR 0x7fe00000 0x4728a512 0x841001fa 0x6e8d69a8\n\
DCVR 0x7fe00000 0x4748a912 0x845009fb 0x6ecd6da9\n\
DCVR 0x7fe00000 0x4788a912 0x849009fb 0x6f0d71a9\n\
DCVR 0x7fe00000 0x47a8ad12 0x84f00dfb 0x6f4d75a9\n\
DCVR 0x7fe00000 0x47c8ad12 0x855015fb 0x6fad79a9\n\
DCVR 0x7fe00000 0x4808b112 0x859015fc 0x6fed7da9\n\
DCVR 0x7fe00000 0x4828b512 0x85f01dfc 0x704d81aa\n\
DCVR 0x7fe00000 0x4868b513 0x867021fc 0x70ad85aa\n\
DCVR 0x7fe00000 0x48a8b913 0x86d029fd 0x710d89aa\n\
DCVR 0x7fe00000 0x48c8bd13 0x875031fd 0x716d91ab\n\
DCVR 0x7fe00000 0x4928c113 0x87d035fe 0x71ed99ab\n\
";

#endif
