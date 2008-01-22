/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file arch-mxc/hardware.h
 * @brief This file contains the hardware definitions of the board.
 *
 * @ingroup MSL_MX27 MSL_MX31 MSL_MXC91321     
 */
#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#define __ASM_ARCH_MXC_HARDWARE_H__

#include <asm/sizes.h>

/*!
 * makes bool as int type
 */
#define bool int

/*!
 * defines false as 0
 */
#define false 		0

/*!
 * defines true as 1
 */
#define true		1

/*!
 * defines PCIO_BASE (not used but needed for compilation)
 */
#define PCIO_BASE		0

/*!
 * This macro is used to get certain bit field from a number
 */
#define MXC_GET_FIELD(val, len, sh)          ((val >> sh) & ((1 << len) - 1))

/*!
 * This macro is used to set certain bit field inside a number
 */
#define MXC_SET_FIELD(val, len, sh, nval)    ((val & ~(((1 << len) - 1) << sh)) | nval << sh)

/*
 * ---------------------------------------------------------------------------
 * Processor specific defines
 * ---------------------------------------------------------------------------
 */
#define CHIP_REV_1_0		0x10
#define CHIP_REV_1_1		0x11
#define CHIP_REV_1_2		0x12
#define CHIP_REV_1_3		0x13
#define CHIP_REV_2_0		0x20
#define CHIP_REV_2_1		0x21
#define CHIP_REV_2_2		0x22
#define CHIP_REV_2_3		0x23
#define CHIP_REV_3_0		0x30
#define CHIP_REV_3_1		0x31
#define CHIP_REV_3_2		0x32

#ifndef __ASSEMBLY__
extern unsigned int system_rev;
#endif
#define mxc_set_system_rev(part, rev) {			\
	system_rev = (part << 12) | rev;		\
}

#define mxc_cpu()		(system_rev >> 12)
#define mxc_is_cpu(part)	((mxc_cpu() == part) ? 1 : 0)
#define mxc_cpu_rev()		(system_rev & 0xFF)
#define mxc_cpu_rev_major()	((system_rev >> 4) & 0xF)
#define mxc_cpu_rev_minor()	(system_rev & 0xF)
#define mxc_cpu_is_rev(rev)	\
	((mxc_cpu_rev() == rev) ? 1 : ((mxc_cpu_rev() < rev) ? -1 : 2))
#define MXC_REV(type)				\
static inline int type## _rev (int rev)		\
{						\
	return (type() ? mxc_cpu_is_rev(rev) : 0);	\
}

#ifdef CONFIG_ARCH_MXC91231
#include <asm/arch/mxc91231.h>
#define cpu_is_mxc91231()	(1)
#else
#define cpu_is_mxc91231()	(0)
#endif

#ifdef CONFIG_ARCH_MXC91221
#include <asm/arch/mxc91221.h>
#define cpu_is_mxc91221()	(1)
#else
#define cpu_is_mxc91221()	(0)
#endif

#if defined(CONFIG_ARCH_MXC91321)
#include <asm/arch/mxc91321.h>
#define cpu_is_mxc91321()	(1)
#else
#define cpu_is_mxc91321()	(0)
#endif

#if defined(CONFIG_ARCH_MXC91311)
#include <asm/arch/mxc91311.h>
#define cpu_is_mxc91311()	(1)
#else
#define cpu_is_mxc91311()	(0)
#endif

#if defined(CONFIG_ARCH_MXC92323)
#include <asm/arch/mxc92323.h>
#define cpu_is_mxc92323()	(1)
#else
#define cpu_is_mxc92323()	(0)
#endif

#ifdef CONFIG_ARCH_MXC91131
#include <asm/arch/mxc91131.h>
#define cpu_is_mxc91131()	(1)
#else
#define cpu_is_mxc91131()	(0)
#endif

#ifdef CONFIG_ARCH_MX3
#include <asm/arch/mx31.h>
#define cpu_is_mx31()		(mxc_is_cpu(0x31))	/*system_rev got from Redboot */
#define cpu_is_mx32()		(mxc_is_cpu(0x32))	/*system_rev got from Redboot */
#else
#define cpu_is_mx31()		(0)
#define cpu_is_mx32()		(0)
#endif

#ifdef CONFIG_ARCH_MX33
#include <asm/arch/mx33.h>
#define cpu_is_mx33()   (1)
#else
#define cpu_is_mx33()   (0)
#endif

#ifdef CONFIG_ARCH_MX21
#include <asm/arch/mx21.h>
#define cpu_is_mx21()		(1)
#else
#define cpu_is_mx21()		(0)
#endif

#ifdef CONFIG_ARCH_MX27
#include <asm/arch/mx27.h>
#define cpu_is_mx27()		(1)
#else
#define cpu_is_mx27()		(0)
#endif

#ifndef __ASSEMBLY__

/*
 * Create inline functions to test for cpu revision
 * Function name is cpu_is_<cpu name>_rev(rev)
 *
 * Returns:
 *	 0 - not the cpu queried
 *	 1 - cpu and revision match
 *	 2 - cpu matches, but cpu revision is greater than queried rev
 *	-1 - cpu matches, but cpu revision is less than queried rev
 */
MXC_REV(cpu_is_mxc91231);
MXC_REV(cpu_is_mxc91221);
MXC_REV(cpu_is_mxc91321);
MXC_REV(cpu_is_mxc91311);
MXC_REV(cpu_is_mxc92323);
MXC_REV(cpu_is_mxc91131);
MXC_REV(cpu_is_mx21);
MXC_REV(cpu_is_mx27);
MXC_REV(cpu_is_mx31);
MXC_REV(cpu_is_mx32);
#endif

#include <asm/arch/mxc.h>

#define MXC_MAX_GPIO_LINES      (GPIO_NUM_PIN * GPIO_PORT_NUM)

#define MXC_EXP_IO_BASE		(MXC_MAX_INT_LINES + MXC_MAX_GPIO_LINES)
#define MXC_MAX_EXP_IO_LINES	16

#define MXC_MAX_VIRTUAL_INTS	16
#define MXC_VIRTUAL_INTS_BASE	(MXC_EXP_IO_BASE + MXC_MAX_EXP_IO_LINES)
#define MXC_SDIO1_CARD_IRQ	MXC_VIRTUAL_INTS_BASE
#define MXC_SDIO2_CARD_IRQ	(MXC_VIRTUAL_INTS_BASE + 1)
#define MXC_SDIO3_CARD_IRQ	(MXC_VIRTUAL_INTS_BASE + 2)

/*
 * Has 64 interrupts at ARM Interrupt Controller Level, GPIO and other
 * INTERRUPTs specified will further be multiplexed
 */

#define MXC_MAX_INTS            (MXC_MAX_INT_LINES + \
                                MXC_MAX_GPIO_LINES + \
                                MXC_MAX_EXP_IO_LINES + \
                                MXC_MAX_VIRTUAL_INTS)

#endif				/* __ASM_ARCH_MXC_HARDWARE_H__ */
