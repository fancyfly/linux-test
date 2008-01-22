/*
 * Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/clk.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/proc-fns.h>
#include <asm/system.h>
#include <asm/arch/clock.h>
#include "crm_regs.h"

/*!
 * @defgroup MSL_MX37 i.MX37 Machine Specific Layer (MSL)
 */

/*!
 * @file mach-mx37/system.c
 * @brief This file contains idle and reset functions.
 *
 * @ingroup MSL_MX37
 */

extern int mxc_jtag_enabled;

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	if (!mxc_jtag_enabled) {
		cpu_do_idle();
	}
}

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
	/* Assert SRS signal */
	mxc_wd_reset();
}
