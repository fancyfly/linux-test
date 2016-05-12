/*
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/imxdpu.h>
#include <linux/of_platform.h>

#define DEBUG

#include "imxdpu_registers.h"
#include "imxdpu_events.h"
#include "imxdpu_intsteer.h"
#include "imxdpu_private.h"

#define DRIVER_NAME	"imxdpu"

static void __iomem *ss_base;

static const struct of_device_id imxdpu_of_match[] = {
	{.compatible = "fsl,imx6-imxdpu",},
	{.compatible = "fsl,imx8dv-imxdpu",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, imxdpu_of_match);

static irqreturn_t handle_irq(int irq, void *data)
{
	if (((void *)imxdpu_get_soc(0)) == data) {
		if (IMXDPU_TRUE == imxdpu_handle_irq(0)) {
			return IRQ_HANDLED;
		} else {
			pr_warn("%s() nothing to do!\n", __func__);
			return IRQ_NONE;
		}
	} else if (((void *)imxdpu_get_soc(1)) == data) {
		if (IMXDPU_TRUE == imxdpu_handle_irq(1)) {
			return IRQ_HANDLED;
		} else {
			pr_warn("%s() nothing to do!\n", __func__);
			return IRQ_NONE;
		}
	} else {
		pr_warn("%s() soc %p != data %p, instance mismatch!\n",
			__func__, imxdpu_get_soc(0), data);
		return IRQ_NONE;
	}
}

static int imxdpu_probe(struct platform_device *pdev)
{
	int irq, ret = 0, i, instance = 0;
	struct resource *mem;
	struct device_node *np = pdev->dev.of_node;
	const char * temp;
	
	pr_debug("%s: %s()\n", DRIVER_NAME, __func__); 

	/* add probe code here */

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "No memory resource\n");
		return -EINVAL;
	}
	dev_dbg(&pdev->dev, "mem->start %p size %u\n", (void *)mem->start,
		(uint32_t) resource_size(mem));

	//FIXME: add all of the individual regions
	ss_base = ioremap(mem->start, (uint32_t) resource_size(mem));
	if (!ss_base) {
		pr_err("%s(): can't ioremap IMXDPU physical base \n", __func__);
		return -ENOMEM;
	}

	temp = of_node_full_name(np); 
	dev_dbg(&pdev->dev, "%s(): full name %s\n", __func__, temp);
	if (strncmp(temp, "/imxdpu", 7) == 0) {
		if (temp[7] == '0') {
			instance = 0;
		} else if (temp[7] == '1') {
			instance = 1;
		} else { 
			dev_err(&pdev->dev, "Can't parse full name property\n");
			return -EINVAL;
		}
	}

	/* init IMXDPU */
	imxdpu_init(instance, ss_base + IMXDPU_REGS_BASE_OFFSET);

	/* init IMXDPU INT STEER for BLT */
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE9_SHDLOAD_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE9_FRAMECOMPLETE_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE9_SEQCOMPLETE_IRQ);

	/* init IMXDPU INT STEER */
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_EXTDST0_SHDLOAD_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_EXTDST1_SHDLOAD_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ);
	//imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE4_SHDLOAD_IRQ);        
	//imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE5_SHDLOAD_IRQ);        
	//imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE4_SEQCOMPLETE_IRQ);    
	//imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE5_SEQCOMPLETE_IRQ);    
	//imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE4_FRAMECOMPLETE_IRQ);  
	//imxdpu_intsteer_enable_irq(ss_base, IMXDPU_STORE5_FRAMECOMPLETE_IRQ);  
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_FRAMEGEN0_INT0_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_FRAMEGEN1_INT0_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_COMCTRL_SW0_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_COMCTRL_SW1_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_DISENGCFG_SHDLOAD0_IRQ);
	imxdpu_intsteer_enable_irq(ss_base, IMXDPU_DISENGCFG_SHDLOAD1_IRQ);

	/* add blit interrupts */

	imxdpu_ss_write(ss_base, IMXDPU_IRQSTEER_CHANnCTL_OFFSET,
			IMXDPU_IRQSTEER_CHANnCTL_0);

	for (i = 0; i < 8; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(&pdev->dev, "no IRQ defined\n");
			ret = -ENODEV;
			goto failed_free_mem;
		}

		ret =
		    request_irq(irq, handle_irq, IRQF_SHARED, "imx8 imxdpu",
				imxdpu_get_soc(instance));
	}
	dev_info(&pdev->dev, "%s(): %s probed.\n", __func__, temp); 

	return ret;

 failed_free_mem:

	iounmap(ss_base);
	return ret;
}

static int imxdpu_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int imxdpu_runtime_suspend(struct device *dev)
{
	return 0;
}

static int imxdpu_runtime_resume(struct device *dev)
{
	return 0;
}

#if CONFIG_PM
static int imxdpu_suspend(struct device *dev)
{
	return 0;
}

static int imxdpu_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops imxdpu_pm_ops = {
	SET_RUNTIME_PM_OPS(imxdpu_runtime_suspend, imxdpu_runtime_resume, NULL)
	    SET_SYSTEM_SLEEP_PM_OPS(imxdpu_suspend, imxdpu_resume)
};
#endif				// CONFIG_PM

static struct platform_driver imxdpu_driver = {
	.driver = {
		   .name = DRIVER_NAME,
		   .of_match_table = imxdpu_of_match,
#ifdef CONFIG_PM
		   .pm = &imxdpu_pm_ops,
#endif
		   },
	.probe = imxdpu_probe,
	.remove = imxdpu_remove,
};

static int __init imxdpu_module_init(void)
{
	int ret;

	pr_debug("%s: %s()\n", DRIVER_NAME, __func__);

	ret = platform_driver_register(&imxdpu_driver);

	return ret;

}

static void __exit imxdpu_cleanup(void)
{
	platform_driver_unregister(&imxdpu_driver);
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Linux IMXDPU driver for Freescale i.MX");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(imxdpu_init);
EXPORT_SYMBOL(imxdpu_disp_setup_channel);
EXPORT_SYMBOL(imxdpu_disp_setup_frame_gen);
EXPORT_SYMBOL(imxdpu_disp_init);
EXPORT_SYMBOL(imxdpu_disp_setup_constframe);
EXPORT_SYMBOL(imxdpu_disp_check_shadow_loads);

module_exit(imxdpu_cleanup);
subsys_initcall(imxdpu_module_init);
