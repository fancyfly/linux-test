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
 * @defgroup DPM_MXC91321 Power Management
 * @ingroup MSL_MXC91321
 */
/*!
 * @file mach-mxc91321/mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CCM_MCU, DPLL and DSM modules.
 *
 * @ingroup DPM_MXC91321
 */

/*
 * Include Files
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>

#include <asm/arch/mxc_pm.h>
#include <asm/arch/system.h>
#include <asm/arch/clock.h>
#include <asm/hardware.h>
#include <asm/arch/dptc.h>

#include "crm_regs.h"

/*
 * MAX clock divider values
 */
#define MAX_PDF_3               0x00000010
#define MAX_PDF_4               0x00000018
#define MAX_PDF_5		0x00000020

/*
 * BRMM divider values
 */
#define MPDR0_BRMM_0            0x00000000
#define MPDR0_BRMM_1            0x00000001
#define MPDR0_BRMM_2            0x00000002
#define MPDR0_BRMM_3            0x00000003
#define MPDR0_BRMM_4            0x00000004

/*
 * MPDR0 Register Mask for Turbo and Normal mode switch
 */
#define TURBO_MASK              0x0000083F

/*
 * PMIC HS count values for max DVS speed setting
 */
#define PMIC_HS_CNT		0x204

#ifdef AHB_CLOCK_100
/*!
 * Enumerations of operating points
 * Normal mode is 399MHz and Turbo mode is 500MHz
 * CORE_NORMAL_4 is PLL is 399MHz and BRMM is 4 when AHB is 100MHz
 */
typedef enum {
	CORE_NORMAL_4 = 100000000,
	CORE_NORMAL_3 = 133000000,
	CORE_NORMAL_2 = 160000000,
	CORE_NORMAL_1 = 200000000,
	CORE_NORMAL = 399000000,
	CORE_TURBO = 500000000,
	/*
	 * The following points are currently not implemented.
	 * These operating points are used only when
	 * the user really wants to lower frequency fast when
	 * working with TPLL otherwise it should not be used since
	 * these points operate at 1.6V and hence will not save power
	 */
	CORE_TURBO_4 = 100160000,	/* 100MHz at 1.6V */
	CORE_TURBO_3 = 136000000,
	CORE_TURBO_2 = 166000000,
	CORE_TURBO_1 = 214000000,
} dvfs_op_point_t;
#else
/*!
 * Enumerations of operating points
 */
typedef enum {
	CORE_NORMAL_4 = 133000000,
	CORE_NORMAL_3 = 171000000,
	CORE_NORMAL_2 = 200000000,
	CORE_NORMAL_1 = 240000000,
	CORE_NORMAL = 399000000,
	CORE_TURBO = 532000000,
	/*
	 * The following points are currently not implemented.
	 * These operating points are used only when
	 * the user really wants to lower frequency fast when
	 * working with TPLL otherwise it should not be used since
	 * these points operate at 1.6V and hence will not save power
	 */
	CORE_TURBO_4 = 133160000,	/* 133MHz at 1.6V */
	CORE_TURBO_3 = 177000000,
	CORE_TURBO_2 = 212000000,
	CORE_TURBO_1 = 266000000,
} dvfs_op_point_t;
#endif

/*!
 * Enumerations of voltage levels
 */
typedef enum {
	LO_VOLT = 0,
	HI_VOLT = 1,
} dvs_t;

/*!
 * Enumerations of Turbo mode
 */
typedef enum {
	TPSEL_CLEAR = 0,
	TPSEL_SET = 1,
} tpsel_t;

/*!
 * Enumerations of DFSP switch status
 */
typedef enum {
	DFS_SWITCH = 0,
	DFS_NO_SWITCH = 1,
} dfs_switch_t;

/*
 * Timer Interrupt
 */
#define AVIC_TIMER_EN		(0x1 << ARCH_TIMER_IRQ)

/*
 * GPIO calls
 */
extern void gpio_pmhs_active(void);
extern void gpio_pmhs_inactive(void);

extern int mxc_jtag_enabled;

/*
 * Clock structures 
 */
static struct clk *mcu_clk;
static struct clk *ahb_clk;
static struct clk *ipg_clk;
static struct clk *nfc_clk;
static struct clk *tpll_clk;
static struct clk *mpll_clk;

/*!
 * Spinlock to protect CRM register accesses
 */
static DEFINE_SPINLOCK(mxc_crm_lock);

/*!
 * This function is called to modify the contents of a CCM_MCU register
 *
 * @param reg_offset the CCM_MCU register that will read
 * @param mask       the mask to be used to clear the bits that are to be modified
 * @param data       the data that should be written to the register
 */
void mxc_ccm_modify_reg(unsigned int reg_offset, unsigned int mask,
			unsigned int data)
{
	unsigned long flags;
	unsigned long reg;

	spin_lock_irqsave(&mxc_crm_lock, flags);
	reg = __raw_readl(reg_offset);
	reg = (reg & (~mask)) | data;
	__raw_writel(reg, reg_offset);
	spin_unlock_irqrestore(&mxc_crm_lock, flags);
}

/*!
 * To check status of DFSP, frequency switch
 *
 * @param   dfs_switch_t      DFS_SWITCH - switch between MPLL and
 *                            TPLL is in progress
 *                            DFS_NO_SWITCH - switch between MPLL and
 *                            TPLL is not in progress
 */
static dfs_switch_t mxc_pm_chk_dfsp(void)
{
	if ((__raw_readl(MXC_CCM_PMCR0) & (MXC_CCM_PMCR0_DFSP)) ==
	    MXC_CCM_PMCR0_DFSP) {
		return DFS_SWITCH;
	} else {
		return DFS_NO_SWITCH;
	}
}

/*!
 * To set Dynamic Voltage Scaling
 *
 */
static int mxc_pm_dvsenable(void)
{
	unsigned long pmcr;

	if (mxc_pm_chk_dfsp() == DFS_NO_SWITCH) {
		pmcr = ((((__raw_readl(MXC_CCM_PMCR0) | (MXC_CCM_PMCR0_DVSE)) |
			  (MXC_CCM_PMCR0_PMICHE)) | (MXC_CCM_PMCR0_EMIHE)) |
			(MXC_CCM_PMCR0_DFSIM));
		__raw_writel(pmcr, MXC_CCM_PMCR0);
	} else {
		return ERR_DFSP_SWITCH;
	}
	return 0;
}

/*!
 * To check status of TPSEL
 *
 * @param   tpsel_t      TPSEL_SET - Turbo mode set
 *                       TPSEL_CLEAR - Turbo mode is clear
 */
static tpsel_t mxc_pm_chk_tpsel(void)
{
	if ((__raw_readl(MXC_CCM_MPDR0) & (MXC_CCM_MPDR0_TPSEL)) != 0) {
		return TPSEL_SET;
	} else {
		return TPSEL_CLEAR;
	}
}

/*!
 * Setting possible BRMM value
 *
 * @param   value      Possible values are 0,1,2,3.
 *
 */
static void mxc_pm_setbrmm(int value)
{
	mxc_ccm_modify_reg(MXC_CCM_MPDR0, MXC_CCM_MPDR0_BRMM_MASK, value);
}

static void mxc_pm_set_parent_pll(dvfs_op_point_t dvfs_op_reqd,
				  struct clk *parent_clk)
{
	if (dvfs_op_reqd == CORE_NORMAL) {
		/* We switched core from NORMAL to TURBO */
		/* PLL is already switched from MCU PLL to TURBO PLL */
		/* Step 1: Disable current PLL clk and its children */
		if ((mxc_pm_chk_tpsel() == TPSEL_SET)
		    || (mxc_pm_chk_dfsp() == DFS_SWITCH)) {
			clk_disable(mpll_clk);
		}
	} else {
		/* We switched core from TURBO to NORMAL */
		/* PLL is already switched from TPLL to MPLL */
		/* Step 1: Disable current PLL clk and its children */
		if ((mxc_pm_chk_tpsel() == TPSEL_CLEAR)
		    || (mxc_pm_chk_dfsp() == DFS_SWITCH)) {
			clk_disable(tpll_clk);
		}
	}

	/* Step 2: set parent for core, AHB and IPG to be Parent PLL */
	mcu_clk->parent = parent_clk;
	ahb_clk->parent = parent_clk;
	ipg_clk->parent = parent_clk;
	nfc_clk->parent = parent_clk;
}

/*!
 * To set Dynamic Voltage Scaling
 *
 */
static int mxc_pm_setturbo(dvfs_op_point_t dvfs_op_reqd)
{
	unsigned long mpdr = 0;

	/*
	 * For Turbo mode, TPSEL, MAX_PDF and BRMM should
	 * be set in one write. IPG_PDF is constant
	 */
	switch (dvfs_op_reqd) {

	case CORE_NORMAL:
		/*
		 * Core is at 399 and we need to move to 500 or 532MHz. So,
		 * Set TPE and TPSEL to support Turbo mode and
		 * reprogram MAX an IPG clock dividers
		 * to maintain frequency at 100 or 133MHz
		 */
		/* Enable TPLL */
		clk_enable(tpll_clk);

		mpdr = __raw_readl(MXC_CCM_MPDR0) | (MXC_CCM_MPDR0_TPSEL);
		/*
		 * Set MAX clock divider to 4 or 5 for AHB at 133 or 100MHz
		 */
		if (clk_get_rate(ahb_clk) == 133000000) {
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_4);
		} else {
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_5);
		}
		break;

	case CORE_TURBO:
		/*
		 * Core is at 500 or 532 and we need to move to 399. So,
		 * Clear TPSEL and set MPE to support Normal mode and
		 * reprogram MAX an IPG clock dividers
		 * to maintain frequency at 100 or 133MHz
		 */
		/* Enable MCU PLL */
		clk_enable(mpll_clk);

		mpdr = __raw_readl(MXC_CCM_MPDR0) & (~MXC_CCM_MPDR0_TPSEL);
		/*
		 * Set MAX clock divider to 3 or 4 for AHB at 133 or 100MHz
		 */
		if (clk_get_rate(ahb_clk) == 133000000) {
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_3);
		} else {
			mpdr = (mpdr & (~MXC_CCM_MPDR0_MAX_PDF_MASK)) |
			    (MAX_PDF_4);
		}
		break;

	default:
		/* Do nothing */
		break;

	}

	clk_put(ahb_clk);

	/* Set BRMM to 0 to indicate turbo mode */
	mpdr = (mpdr & (~MXC_CCM_MPDR0_BRMM_MASK)) | MPDR0_BRMM_0;
	if (mxc_pm_chk_dfsp() == DFS_NO_SWITCH) {
		mxc_ccm_modify_reg(MXC_CCM_MPDR0, TURBO_MASK, mpdr);
	} else {
		return ERR_DFSP_SWITCH;
	}
	if (dvfs_op_reqd == CORE_NORMAL) {
		mxc_pm_set_parent_pll(dvfs_op_reqd, tpll_clk);
	} else if (dvfs_op_reqd == CORE_TURBO) {
		mxc_pm_set_parent_pll(dvfs_op_reqd, mpll_clk);
	}

	return 0;
}

/*!
 * To change AP core frequency and/or voltage suitably
 *
 * @param   dvfs_op    The values are,
 *                     CORE_NORMAL - ARM desired to run @399MHz, LoV (1.2V)
 *                     CORE_NORMAL_4 - ARM desired to run @100MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @133MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_NORMAL_3 - ARM desired to run @133MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @171MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_NORMAL_2 - ARM desired to run @160MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @200MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_NORMAL_1 - ARM desired to run @200MHz, LoV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @240MHz, LoV (1.2V)
 *                                     if AHB is 133MHz
 *                     CORE_TURBO    - ARM desired to run @500MHz, HiV (1.6V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @532MHz, HiV (1.6V)
 *                                     if AHB is 133MHz
 *                     CORE_TURBO_1  - ARM desired to run @214MHz, HiV (1.2V)
 *                                     if AHB is 100MHz and
 *                                     ARM desired to run @266MHz, HiV (1.2V)
 *                                     if AHB is 133MHz
 *                                     The reason for supporting 214MHz is that this
 *                                     is the only operating point closer to 266MHz. Rest
 *                                     of the Hi Voltage but low frequency operating
 *                                     are not supported because they are not the best
 *                                     power saving option. But can be added upon request.
 *
 * @return             Returns -ERR_FREQ_INVALID if none of the above choice
 *                     is selected
 */
static int mxc_pm_chgfreq(dvfs_op_point_t dvfs_op)
{
	int retval = 0;

	switch (dvfs_op) {

	case CORE_NORMAL_4:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_4);

		break;

	case CORE_NORMAL_3:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_3);

		break;

	case CORE_NORMAL_2:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_2);

		break;

	case CORE_NORMAL_1:
		/*
		 * Set Core to run at 399MHz and then set brmm to 4
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * CORE_NORMAL (399MHz) case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_NORMAL);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_1);

		break;

	case CORE_TURBO_1:
		/*
		 * Set Core to run at 532MHz or 500MHz and then set brmm to 1
		 */
		if (mxc_pm_chk_tpsel() == TPSEL_CLEAR) {
			/*
			 * CORE_TURBO case should handle the right voltage level
			 */
			mxc_pm_chgfreq(CORE_TURBO);
		}
		mxc_pm_setbrmm(MPDR0_BRMM_1);

		break;

	case CORE_NORMAL:

		/* Check TPSEL bit status */

		if (mxc_pm_chk_tpsel() == TPSEL_SET) {
			/*
			 * If TPSEL is set then the PLL freq is 500 or 532MHz.
			 * So, switch from TPLL to MPLL should occur.
			 */
			/*
			 * Set Normal Mode
			 */
			retval = mxc_pm_setturbo(CORE_TURBO);
			if (retval == ERR_DFSP_SWITCH) {
				return -ERR_DFSP_SWITCH;
			}

			/*
			 * Check if MPLL is locked
			 */
			while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_MPL) !=
			       MXC_CCM_MCR_MPL) ;

			/*
			 * Switch from TPLL to MPLL will occur now
			 * Wait until switch is complete. Interrupt
			 * will occur as soon as the switch completes
			 * by setting DFSI
			 */
			while (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) != 1) ;

			/*
			 * Clear DFSI interrupt
			 */
			__raw_writel((__raw_readl(MXC_CCM_PMCR0) |
				      MXC_CCM_PMCR0_DFSI), MXC_CCM_PMCR0);

#ifdef CONFIG_MXC_DPTC
			/*
			 * Suspend DPTC and adjust voltage
			 */
			dptc_suspend();
			dptc_set_turbo_mode(0);
#else
			/*
			 * Suspend DPTC and adjust voltage
			 */
			dptc_disable();
#endif
		} else {
			/*
			 * This implies PLL is already locked at 399MHz and
			 * previous transitions had been Integer scaling from 399MHz
			 */
			mxc_pm_setbrmm(MPDR0_BRMM_0);
		}
		break;

	case CORE_TURBO:

		/* Check TPSEL bit status */

		if (mxc_pm_chk_tpsel() == TPSEL_CLEAR) {
			/*
			 * If TPSEL is clear then the PLL freq is 399MHz.
			 * So, switch from MPLL to TPLL should occur.
			 */

#ifdef CONFIG_MXC_DPTC
			/*
			 * Adjust voltage
			 */
			dptc_set_turbo_mode(1);
#endif
			/*
			 * Set Turbo Mode
			 */
			retval = mxc_pm_setturbo(CORE_NORMAL);
			if (retval == ERR_DFSP_SWITCH) {
				return -ERR_DFSP_SWITCH;
			}

			/*
			 * Check if TPLL is locked
			 */
			while ((__raw_readl(MXC_CCM_MCR) & MXC_CCM_MCR_TPL) !=
			       MXC_CCM_MCR_TPL) ;

			/*
			 * Switch from MPLL to TPLL will occur now
			 * Wait until switch is complete. Interrupt
			 * will occur as soon as the switch completes
			 * by setting DFSI
			 */
			while (((__raw_readl(MXC_CCM_PMCR0) >> 24) & 1) != 1) ;

			/*
			 * Clear DFSI interrupt
			 */
			__raw_writel((__raw_readl(MXC_CCM_PMCR0) |
				      MXC_CCM_PMCR0_DFSI), MXC_CCM_PMCR0);

#ifdef CONFIG_MXC_DPTC
			/*
			 * Resume DPTC
			 */
			dptc_resume();
#else
			/*
			 * Resume DPTC
			 */
			dptc_enable();
#endif

		} else {
			/*
			 * This implies PLL is already locked at 532MHz and
			 * previous transitions had been Integer scaling from 532MHz
			 */
			mxc_pm_setbrmm(MPDR0_BRMM_0);
		}
		break;

	default:
		return -ERR_FREQ_INVALID;
		break;
	}
	return 0;
}

/*!
 * To change AP core frequency and/or voltage suitably
 *
 * @param   armfreq    The desired ARM frequency
 * @param   ahbfreq    The desired AHB frequency: constant value 133 MHz
 * @param   ipfreq     The desired IP frequency : constant value 66.5 MHz
 *
 * @return             Returns -ERR_FREQ_INVALID on failure
 *                     Returns 0 on success
 */
int mxc_pm_dvfs(unsigned long armfreq, long ahbfreq, long ipfreq)
{
	int ret_val = -1;
	unsigned long flags;

	local_irq_save(flags);

	/* Changing Frequency/Voltage */
	ret_val = mxc_pm_chgfreq(armfreq);

	local_irq_restore(flags);

	return ret_val;
}

/*!
 * Implementing steps required to transition to low-power modes
 *
 * @param   mode    The desired low-power mode. Possible values are,
 *                  WAIT_MODE, DOZE_MODE, STOP_MODE or DSM_MODE
 *
 */
void mxc_pm_lowpower(int mode)
{
	unsigned int crm_ctrl;
	unsigned long flags;

	if (mxc_jtag_enabled == 1) {
		/*
		 * If JTAG is connected then WFI is not enabled
		 * So return
		 */
		return;
	}

	/* Check parameter */
	if ((mode != WAIT_MODE) && (mode != STOP_MODE) && (mode != DSM_MODE) &&
	    (mode != DOZE_MODE)) {
		return;
	}

	local_irq_save(flags);

	switch (mode) {
	case WAIT_MODE:
		/*
		 * Handled by kernel using arch_idle()
		 * There are no LPM bits for WAIT mode which
		 * are set by software
		 */
		break;
	case DOZE_MODE:
		/* Writing '10' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_2));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
		break;
	case STOP_MODE:
		__raw_writel((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_DPE)),
			     MXC_CCM_MCR);
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_3));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
		break;
	case DSM_MODE:
		__raw_writel((__raw_readl(MXC_CCM_MCR) | (MXC_CCM_MCR_WBEN)),
			     MXC_CCM_MCR);
		__raw_writel((__raw_readl(MXC_CCM_MCR) | (MXC_CCM_MCR_SBYOS)),
			     MXC_CCM_MCR);
		__raw_writel((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_DPE)),
			     MXC_CCM_MCR);
		/* Writing '11' to CRM_lpmd bits */
		crm_ctrl =
		    ((__raw_readl(MXC_CCM_MCR) & (~MXC_CCM_MCR_LPM_MASK)) |
		     (MXC_CCM_MCR_LPM_3));
		__raw_writel(crm_ctrl, MXC_CCM_MCR);
		break;
	}
	/* Executing CP15 (Wait-for-Interrupt) Instruction */
	cpu_do_idle();

	local_irq_restore(flags);
}

/*
 * This API is not supported on MXC91321
 */
int mxc_pm_intscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

/*
 * This API is not supported on MXC91321
 */
int mxc_pm_pllscale(long armfreq, long ahbfreq, long ipfreq)
{
	return -MXC_PM_API_NOT_SUPPORTED;
}

/*!
 * This function is used to load the module.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
	int retval;

	mcu_clk = clk_get(NULL, "cpu_clk");
	ahb_clk = clk_get(NULL, "ahb_clk");
	ipg_clk = clk_get(NULL, "ipg_clk");
	nfc_clk = clk_get(NULL, "nfc_clk");
	tpll_clk = clk_get(NULL, "turbo_pll");
	mpll_clk = clk_get(NULL, "mcu_pll");

	retval =
	    request_irq(INT_CCM_MCU_DVFS, NULL, 0, "mxc_pm_mxc91321", NULL);

	/*
	 * Set DVSE bit to enable Dynamic Voltage Scaling
	 */
	retval = mxc_pm_dvsenable();
	if (retval == ERR_DFSP_SWITCH) {
		return -1;
	}

	__raw_writel(((__raw_readl(MXC_CCM_PMCR1) & 0xFFFFC000) | PMIC_HS_CNT),
		     MXC_CCM_PMCR1);

	/*
	 * IOMUX configuration for GPIO9 and GPIO20
	 */
	gpio_pmhs_active();

	pr_info("Low-Level PM Driver module loaded\n");
	return 0;
}

/*!
 * This function is used to unload the module
 */
static void __exit mxc_pm_cleanup_module(void)
{
	gpio_pmhs_inactive();
	pr_info("Low-Level PM Driver module Unloaded\n");
}

module_init(mxc_pm_init_module);
module_exit(mxc_pm_cleanup_module);

EXPORT_SYMBOL(mxc_pm_dvfs);
EXPORT_SYMBOL(mxc_pm_lowpower);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_intscale);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC91321 Low-level PM Driver");
MODULE_LICENSE("GPL");
