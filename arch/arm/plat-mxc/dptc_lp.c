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
 * @brief Driver for the Freescale Semiconductor MXC DPTC LP module.
 *
 * The DPTC LP driver is designed to control the MXC DPTC hardware.
 * Upon initialization, the DPTC LP driver initializes the DPTC Peripheral hardware
 * sets up driver nodes attaches to the DPTC interrupt and initializes internal
 * data structures. When the DPTC PER interrupt occurs the driver checks the cause
 * of the interrupt (lower frequency, increase frequency or emergency) and changes
 * the CPU voltage according to translation table that is loaded into the driver.
 * The driver read method is used to read the log buffer.
 * Driver ioctls are used to change driver parameters and enable/disable the
 * DVFS operation.
 *
 * @ingroup PM
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <linux/i2c.h>
#include <linux/regulator/regulator.h>
#include <asm/arch/hardware.h>

#include "../mach-mx37/crm_regs.h"

#define MXC_DPTCCR		(MXC_DPTC_LP_BASE + 0x00)
#define MXC_DPTCDBG		(MXC_DPTC_LP_BASE + 0x04)
#define MXC_DCVR0		(MXC_DPTC_LP_BASE + 0x08)
#define MXC_DCVR1		(MXC_DPTC_LP_BASE + 0x0C)
#define MXC_DCVR2		(MXC_DPTC_LP_BASE + 0x10)
#define MXC_DCVR3		(MXC_DPTC_LP_BASE + 0x14)

#define MXC_DPTCCR_DRCE3                     0x00400000
#define MXC_DPTCCR_DRCE2                     0x00200000
#define MXC_DPTCCR_DRCE1                     0x00100000
#define MXC_DPTCCR_DRCE0                     0x00080000
#define MXC_DPTCCR_DCR_256                   0x00060000
#define MXC_DPTCCR_DCR_128                   0x00040000
#define MXC_DPTCCR_DCR_64                    0x00020000
#define MXC_DPTCCR_DCR_32                    0x00000000
#define MXC_DPTCCR_DSMM                      0x00000040
#define MXC_DPTCCR_DPNVCR                    0x00000020
#define MXC_DPTCCR_DPVV                      0x00000010
#define MXC_DPTCCR_VAIM                      0x00000008
#define MXC_DPTCCR_VAI_OFFSET                1
#define MXC_DPTCCR_VAI_MASK                  0x00000006
#define MXC_DPTCCR_DEN                       0x00000001

#define MXC_GPCCNTR_GPCIRQ                   0x00100000
#define MXC_GPCCNTR_DPTC1CR                  0x00080000
#define MXC_GPCCNTR_ADU                      0x00008000

static int dptc_lp_is_active;

static int curr_wp;
static u32 ptvai;
static struct delayed_work dptc_lp_work;
static struct dptc_wp *dptc_lp_wp_allfreq;
static struct device *dptc_lp_dev;
static struct clk *ahb_clk;
struct regulator *lp_per;

DEFINE_SPINLOCK(mxc_dptc_lp_lock);

enum {
	DPTC_PTVAI_NOCHANGE = 0x0,
	DPTC_PTVAI_DECREASE,
	DPTC_PTVAI_INCREASE,
	DPTC_PTVAI_EMERG,
};

static void update_dptc_wp(u32 wp)
{
	int voltage_uV;
	int ret = 0;

	voltage_uV = (dptc_lp_wp_allfreq[wp].voltage) * 1000;

	__raw_writel(dptc_lp_wp_allfreq[wp].dcvr0, MXC_DCVR0);
	__raw_writel(dptc_lp_wp_allfreq[wp].dcvr1, MXC_DCVR1);
	__raw_writel(dptc_lp_wp_allfreq[wp].dcvr2, MXC_DCVR2);
	__raw_writel(dptc_lp_wp_allfreq[wp].dcvr3, MXC_DCVR3);

	/* Set the voltage for the LP domain. */
	if (lp_per != NULL) {
		ret = regulator_set_voltage(lp_per, voltage_uV);
		if (ret < 0)
			printk(KERN_DEBUG "COULD NOT SET LP VOLTAGE!!!!!\n");
	}

	pr_debug("dcvr0-3: 0x%x, 0x%x, 0x%x, 0x%x; vol: %d\n",
		 dptc_lp_wp_allfreq[wp].dcvr0,
		 dptc_lp_wp_allfreq[wp].dcvr1,
		 dptc_lp_wp_allfreq[wp].dcvr2,
		 dptc_lp_wp_allfreq[wp].dcvr3, dptc_lp_wp_allfreq[wp].voltage);
}

static irqreturn_t dptc_lp_irq(int irq, void *dev_id)
{
	u32 dptccr = __raw_readl(MXC_DPTCCR);
	u32 gpc_cntr = __raw_readl(MXC_GPC_CNTR);

	gpc_cntr = (gpc_cntr & MXC_GPCCNTR_DPTC1CR);

	if (gpc_cntr) {
		ptvai = (dptccr & MXC_DPTCCR_VAI_MASK) >> MXC_DPTCCR_VAI_OFFSET;

		/* disable DPTC and mask its interrupt */
		dptccr = (dptccr & ~(MXC_DPTCCR_DEN)) | MXC_DPTCCR_VAIM;
		dptccr = (dptccr & ~(MXC_DPTCCR_DPNVCR));
		__raw_writel(dptccr, MXC_DPTCCR);

		schedule_delayed_work(&dptc_lp_work, 0);
	}

	return IRQ_RETVAL(1);
}

static void dptc_lp_workqueue_handler(struct work_struct *work)
{
	u32 dptccr = __raw_readl(MXC_DPTCCR);

	switch (ptvai) {
	case DPTC_PTVAI_DECREASE:
		curr_wp++;
		break;
	case DPTC_PTVAI_INCREASE:
	case DPTC_PTVAI_EMERG:
		curr_wp--;
		if (curr_wp < 0) {
			/* already max voltage */
			curr_wp = 0;
			printk(KERN_WARNING "dptc: already maximum voltage\n");
		}
		break;

		/* Unknown interrupt cause */
	default:
		BUG();
	}

	if (curr_wp > DPTC_LP_WP_SUPPORTED || curr_wp < 0) {
		panic("Can't support this working point: %d\n", curr_wp);
	}
	update_dptc_wp(curr_wp);

	/* enable DPTC and unmask its interrupt */
	dptccr =
	    (dptccr & ~(MXC_DPTCCR_VAIM)) | MXC_DPTCCR_DPNVCR | MXC_DPTCCR_DEN;
	__raw_writel(dptccr, MXC_DPTCCR);
}

/* Start DPTC unconditionally */
static int start_dptc(void)
{
	u32 dptccr, flags;
	unsigned long ahb_rate;

	/* Set the voltage for the LP domain. */
	lp_per = regulator_get(NULL, "DCDC4");

	spin_lock_irqsave(&mxc_dptc_lp_lock, flags);

	ahb_rate = clk_get_rate(ahb_clk);

	if (ahb_rate < 133000000) {
		goto err;
	}

	/* Enable PER domain frequency and/or voltage update needed and enable ARM IRQ */
	__raw_writel(MXC_GPCCNTR_GPCIRQ, MXC_GPC_CNTR);

	dptccr = __raw_readl(MXC_DPTCCR);

	/* Enable DPTC and unmask its interrupt */
	dptccr = ((dptccr & ~(MXC_DPTCCR_VAIM)) | MXC_DPTCCR_DEN) |
	    (MXC_DPTCCR_DPNVCR | MXC_DPTCCR_DPVV | MXC_DPTCCR_DSMM);

	__raw_writel(dptccr, MXC_DPTCCR);

	spin_unlock_irqrestore(&mxc_dptc_lp_lock, flags);

	pr_info("DPTC LP has been started \n");

	return 0;

      err:
	spin_unlock_irqrestore(&mxc_dptc_lp_lock, flags);
	pr_info("DPTC LP is not enabled\n");
	return -1;
}

/* Stop DPTC unconditionally */
static void stop_dptc(void)
{
	u32 dptccr;

	dptccr = __raw_readl(MXC_DPTCCR);

	/* disable DPTC and mask its interrupt */
	dptccr = ((dptccr & ~(MXC_DPTCCR_DEN)) | MXC_DPTCCR_VAIM) &
	    (~MXC_DPTCCR_DPNVCR);

	__raw_writel(dptccr, MXC_DPTCCR);

	/* Restore Turbo Mode voltage to highest wp */
	update_dptc_wp(0);
	curr_wp = 0;

	regulator_put(lp_per, NULL);

	pr_info("DPTC LP has been stopped\n");
}

/*
  this function does not change the working point. It can be
 called from an interrupt context.
*/
void dptc_lp_suspend(void)
{
	u32 dptccr;

	if (!dptc_lp_is_active)
		return;

	dptccr = __raw_readl(MXC_DPTCCR);

	/* disable DPTC and mask its interrupt */
	dptccr = ((dptccr & ~(MXC_DPTCCR_DEN)) | MXC_DPTCCR_VAIM) &
	    (~MXC_DPTCCR_DPNVCR);

	__raw_writel(dptccr, MXC_DPTCCR);
}

/*!
 * This function is called to put the DPTC in a low power state.
 *
 */
void dptc_lp_disable(void)
{
	if (!dptc_lp_is_active)
		return;

	stop_dptc();
	dptc_lp_is_active = 0;
}

/*!
 * This function is called to resume the DPTC from a low power state.
 *
 */
void dptc_lp_enable(void)
{
	if (dptc_lp_is_active)
		return;
	start_dptc();
	dptc_lp_is_active = 1;
}

static ssize_t dptc_lp_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	if (dptc_lp_is_active)
		return sprintf(buf, "DPTC LP is enabled\n");
	else
		return sprintf(buf, "DPTC LP is disabled\n");
}

static ssize_t dptc_lp_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t size)
{
	if (strstr(buf, "0") != NULL) {
		dptc_lp_disable();
	} else if (strstr(buf, "1") != NULL) {
		dptc_lp_enable();
	}

	return size;
}

static DEVICE_ATTR(enable, 0644, dptc_lp_show, dptc_lp_store);

/*!
 * This is the probe routine for the DPTC driver.
 *
 * @param   pdev   The platform device structure
 *
 * @return         The function returns 0 on success
 *
 */
static int __devinit mxc_dptc_lp_probe(struct platform_device *pdev)
{
	int res = 0;
	u32 dptccr;

	printk(KERN_INFO "DPTC mxc_dptc_lp_probe()\n");
	dptc_lp_dev = &pdev->dev;
	dptc_lp_wp_allfreq = pdev->dev.platform_data;
	if (dptc_lp_wp_allfreq == NULL) {
		printk(KERN_ERR "DPTC: Pointer to DPTC table is NULL\
				not started\n");
		return -1;
	}

	INIT_DELAYED_WORK(&dptc_lp_work, dptc_lp_workqueue_handler);

	/* request the DPTC interrupt */
	res =
	    request_irq(MXC_INT_GPC1, dptc_lp_irq, IRQF_SHARED, "mxc-dptc-lp",
			pdev);
	if (res) {
		printk(KERN_ERR "DPTC: Unable to attach to DPTC interrupt");
		return res;
	}
	/* Enable Reference Circuits */
	dptccr =
	    (MXC_DPTCCR_DRCE0 | MXC_DPTCCR_DRCE1 | MXC_DPTCCR_DRCE2 |
	     MXC_DPTCCR_DRCE3) | (MXC_DPTCCR_DCR_128 | MXC_DPTCCR_DPNVCR |
				  MXC_DPTCCR_DPVV);
	__raw_writel(dptccr, MXC_DPTCCR);

	res = sysfs_create_file(&dptc_lp_dev->kobj, &dev_attr_enable.attr);
	if (res) {
		printk(KERN_ERR
		       "DPTC: Unable to register sysdev entry for dptc");
		return res;
	}

	if (res != 0) {
		printk(KERN_ERR "DPTC: Unable to start");
		return res;
	}

	curr_wp = 0;
	update_dptc_wp(curr_wp);

	ahb_clk = clk_get(NULL, "ahb_clk");

	return 0;
}

/*!
 * This function is called to put DPTC in a low power state.
 *
 * @param   pdev  the device structure
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxc_dptc_lp_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (dptc_lp_is_active)
		stop_dptc();

	return 0;
}

/*!
 * This function is called to resume the MU from a low power state.
 *
 * @param   dev   the device structure
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_dptc_lp_resume(struct platform_device *pdev)
{
	if (dptc_lp_is_active)
		start_dptc();

	return 0;
}

static struct platform_driver mxc_dptc_lp_driver = {
	.driver = {
		   .name = "mxc_dptc_lp",
		   },
	.probe = mxc_dptc_lp_probe,
	.suspend = mxc_dptc_lp_suspend,
	.resume = mxc_dptc_lp_resume,
};

/*!
 * This function is called to resume the MU from a low power state.
 *
 * @param   dev   the device structure used to give information on which MU
 *                device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */

static int __init dptc_lp_init(void)
{
	printk(KERN_INFO "DPTC dptc_lp_init()\n");
	if (platform_driver_register(&mxc_dptc_lp_driver) != 0) {
		printk(KERN_ERR "mxc_dptc_lp_driver register failed\n");
		return -ENODEV;
	}

	printk(KERN_INFO "DPTC LP driver module loaded\n");
	return 0;
}

static void __exit dptc_lp_cleanup(void)
{
	stop_dptc();

	/* release the DPTC interrupt */
	free_irq(MXC_INT_GPC1, NULL);

	sysfs_remove_file(&dptc_lp_dev->kobj, &dev_attr_enable.attr);

	/* Unregister the device structure */
	platform_driver_unregister(&mxc_dptc_lp_driver);

	printk("DPTC LP driver module unloaded\n");
}

module_init(dptc_lp_init);
module_exit(dptc_lp_cleanup);

EXPORT_SYMBOL(dptc_lp_disable);
EXPORT_SYMBOL(dptc_lp_enable);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("DPTC LP driver");
MODULE_LICENSE("GPL");
