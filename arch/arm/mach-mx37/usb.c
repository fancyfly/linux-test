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
 * @defgroup USB_MX37 ARC OTG USB Driver for i.MX37
 * @ingroup USB
 */

/*!
 * @file mach-mx37/usb.c
 *
 * @brief platform related part of usb driver.
 * @ingroup USB_MX37
 */

/*!
 *Include files
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/clk.h>
#include <linux/usb/fsl_xcvr.h>
#include <linux/usb/otg.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/arch/arc_otg.h>

extern struct platform_device *host_pdev_register(struct resource *res,
						  int n_res,
						  struct fsl_usb2_platform_data
						  *config);

extern int usbotg_init(struct platform_device *pdev);
extern void usbotg_uninit(struct fsl_usb2_platform_data *pdata);
extern int gpio_usbotg_hs_active(void);
extern void gpio_usbotg_hs_inactive(void);

#if defined(CONFIG_USB_EHCI_ARC_OTG) || defined(CONFIG_USB_GADGET_ARC)

static int usbotg_init_ext(struct platform_device *pdev);
static void usbotg_uninit_ext(struct fsl_usb2_platform_data *pdata);

static struct resource otg_resources[] = {
	{
	 .start = (u32) (OTG_BASE_ADDR),
	 .end = (u32) (OTG_BASE_ADDR + 0x1ff),
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = MXC_INT_USB_OTG,
	 .flags = IORESOURCE_IRQ,
	 },
};
static struct fsl_usb2_platform_data mxc_utmi_host_config = {
	.name = "OTG",
	.platform_init = usbotg_init_ext,
	.platform_uninit = usbotg_uninit_ext,
	.usbmode = (u32) &UOG_USBMODE,
	.viewport = (u32) &UOG_ULPIVIEW,
	.does_otg = 0,
	.operating_mode = FSL_USB2_DR_HOST,
	.power_budget = 150,	/* 150 mA max power */
	.gpio_usb_active = gpio_usbotg_hs_active,
	.gpio_usb_inactive = gpio_usbotg_hs_inactive,
	.transceiver = "utmi",
};

static int usbotg_init_ext(struct platform_device *pdev)
{
	struct clk *usb_clk;
	struct fsl_usb2_platform_data *pdata;

	usb_clk = clk_get(NULL, "usboh2_clk");
	clk_enable(usb_clk);
	clk_put(usb_clk);

	usb_clk = clk_get(NULL, "usb_phy_clk");
	clk_enable(usb_clk);
	clk_put(usb_clk);

	/*derive clock from oscillator */
	usb_clk = clk_get(NULL, "usb_utmi_clk");
	clk_disable(usb_clk);
	clk_put(usb_clk);

	pdata = (struct fsl_usb2_platform_data *)pdev->dev.platform_data;
	pdata->viewport = (u32) pdev;
	return usbotg_init(pdev);
}

static void usbotg_uninit_ext(struct fsl_usb2_platform_data *pdata)
{
	struct clk *usb_clk;

	usb_clk = clk_get(NULL, "usboh2_clk");
	clk_disable(usb_clk);
	clk_put(usb_clk);

	usb_clk = clk_get(NULL, "usb_phy_clk");
	clk_disable(usb_clk);
	clk_put(usb_clk);

	usbotg_uninit(pdata);
}


#endif

#if defined(CONFIG_USB_GADGET_ARC)
/*!
 * OTG Gadget device
 */
static void udc_release(struct device *dev)
{
	/* normally not freed */
}

static u64 udc_dmamask = ~(u32) 0;

static struct fsl_usb2_platform_data mxc_utmi_peripheral_config = {
	.name = "OTG",
	.platform_init = usbotg_init_ext,
	.platform_uninit = usbotg_uninit_ext,
	.usbmode = (u32) &UOG_USBMODE,
	.does_otg = 0,
	.operating_mode = FSL_USB2_DR_DEVICE,
	.power_budget = 150,	/* 150 mA max power */
	.gpio_usb_active = gpio_usbotg_hs_active,
	.gpio_usb_inactive = gpio_usbotg_hs_inactive,
	.transceiver = "utmi",
};
static struct platform_device otg_udc_device = {
	.name = "arc_udc",
	.id = -1,
	.dev = {
		.release = udc_release,
		.dma_mask = &udc_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &mxc_utmi_peripheral_config,
		},
	.resource = otg_resources,
	.num_resources = ARRAY_SIZE(otg_resources),
};
#endif
/* *INDENT-ON* */

static int __init mx37_usb_init(void)
{
	pr_debug("%s: \n", __func__);

#if defined(CONFIG_USB_GADGET_ARC)
	if (platform_device_register(&otg_udc_device)) {
		printk(KERN_ERR "can't register OTG Gadget\n");
	} else {
		pr_debug("usb: OTG Gadget registered\n");
	}
#endif
#ifdef CONFIG_USB_EHCI_ARC_OTG
	host_pdev_register(otg_resources, ARRAY_SIZE(otg_resources),
			   &mxc_utmi_host_config);
#endif

	return 0;
}

module_init(mx37_usb_init);
