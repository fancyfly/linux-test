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

/*!
 * @defgroup Framebuffer Framebuffer Driver for SDC and ADC.
 */

/*!
 * @file mxcfb_epson_vga.c
 *
 * @brief MXC Frame buffer driver for SDC
 *
 * @ingroup Framebuffer
 */

/*!
 * Include files
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <asm/arch/mxcfb.h>
#include <asm/arch/pmic_power.h>

static struct spi_device *lcd_spi;

static void lcd_init(void);
static void lcd_poweron(void);
static void lcd_poweroff(void);

static void (*lcd_reset) (void);

static struct fb_videomode video_modes[] = {
	{
	 /* 480x640 @ 60 Hz */
	 "Epson-VGA", 60, 480, 640, 41701, 0, 41, 10, 5, 20, 10,
	 FB_SYNC_CLK_INVERT,
	 FB_VMODE_NONINTERLACED,
	 0,},
};

static void lcd_init_fb(struct fb_info *info)
{
	struct fb_var_screeninfo var;

	memset(&var, 0, sizeof(var));

	fb_videomode_to_var(&var, &video_modes[0]);

	var.activate = FB_ACTIVATE_ALL;
	var.yres_virtual = var.yres * 2;

	acquire_console_sem();
	info->flags |= FBINFO_MISC_USEREVENT;
	fb_set_var(info, &var);
	info->flags &= ~FBINFO_MISC_USEREVENT;
	release_console_sem();
}

static int lcd_fb_event(struct notifier_block *nb, unsigned long val, void *v)
{
	struct fb_event *event = v;

	if (strcmp(event->info->fix.id, "DISP3 BG")) {
		return 0;
	}

	switch (val) {
	case FB_EVENT_FB_REGISTERED:
		lcd_init_fb(event->info);
		lcd_poweron();
		break;
	case FB_EVENT_BLANK:
		if ((event->info->var.xres != 480) ||
		    (event->info->var.yres != 640)) {
			break;
		}
		if (*((int *)event->data) == FB_BLANK_UNBLANK) {
			lcd_poweron();
		} else {
			lcd_poweroff();
		}
		break;
	}
	return 0;
}

static struct notifier_block nb = {
	.notifier_call = lcd_fb_event,
};

/*!
 * This function is called whenever the SPI slave device is detected.
 *
 * @param	spi	the SPI slave device
 *
 * @return 	Returns 0 on SUCCESS and error on FAILURE.
 */
static int __devinit lcd_spi_probe(struct spi_device *spi)
{
	int i;
	t_regulator_voltage voltage;
	t_regulator_lp_mode lpmode = LOW_POWER_CTRL_BY_PIN;

	lcd_spi = spi;

	/* open the VGEN of pmic to supply VDDI */
	voltage.vgen = VGEN_1_8V;
	pmic_power_regulator_set_voltage(REGU_VGEN, voltage);
	pmic_power_regulator_set_lp_mode(REGU_VGEN, lpmode);
	pmic_power_regulator_on(REGU_VGEN);

	/* open the VMMC1 of pmic to supply VDDI */
	voltage.vmmc1 = VMMC_2_8V;
	pmic_power_regulator_set_voltage(REGU_VMMC1, voltage);
	pmic_power_regulator_set_lp_mode(REGU_VMMC1, lpmode);
	pmic_power_regulator_on(REGU_VMMC1);

	if (spi->dev.platform_data) {
		lcd_reset = (void (*)(void))spi->dev.platform_data;
		lcd_reset();
	}

	spi->bits_per_word = 9;
	spi_setup(spi);

	lcd_init();

	for (i = 0; i < num_registered_fb; i++) {
		if (strcmp(registered_fb[i]->fix.id, "DISP3 BG") == 0) {
			lcd_init_fb(registered_fb[i]);
			fb_show_logo(registered_fb[i], 0);
			lcd_poweron();
		}
	}

	fb_register_client(&nb);

	return 0;
}

static int __devexit lcd_spi_remove(struct spi_device *spi)
{
	fb_unregister_client(&nb);
	lcd_poweroff();
	return 0;
}

/*!
 * spi driver structure for LTV350QV
 */
static struct spi_driver lcd_spi_dev_driver = {

	.driver = {
		   .name = "lcd_spi",
		   .owner = THIS_MODULE,
		   },
	.probe = lcd_spi_probe,
	.remove = __devexit_p(lcd_spi_remove),
};

#define param(x) ((x) | 0x100)

/*
 * Send init commands to L4F00242T03
 *
 */
static void lcd_init(void)
{
	const u16 cmd[] = { 0x36, param(0), 0x3A, param(0x60) };

	dev_dbg(&lcd_spi->dev, "initializing LCD\n");
	spi_write(lcd_spi, (const u8 *)cmd, ARRAY_SIZE(cmd));
}

static int lcd_on;
/*
 * Send Power On commands to L4F00242T03
 *
 */
static void lcd_poweron(void)
{
	const u16 slpout = 0x11;
	const u16 dison = 0x29;

	if (lcd_on)
		return;

	dev_dbg(&lcd_spi->dev, "turning on LCD\n");
	spi_write(lcd_spi, (const u8 *)&slpout, 1);
	msleep(60);
	spi_write(lcd_spi, (const u8 *)&dison, 1);
	lcd_on = 1;
}

/*
 * Send Power Off commands to L4F00242T03
 *
 */
static void lcd_poweroff(void)
{
	const u16 slpin = 0x10;
	const u16 disoff = 0x28;

	if (!lcd_on)
		return;

	dev_dbg(&lcd_spi->dev, "turning off LCD\n");
	spi_write(lcd_spi, (const u8 *)&disoff, 1);
	msleep(60);
	spi_write(lcd_spi, (const u8 *)&slpin, 1);
	lcd_on = 0;
}

static int __init epson_lcd_init(void)
{
	return spi_register_driver(&lcd_spi_dev_driver);
}

static void __exit epson_lcd_exit(void)
{
	spi_unregister_driver(&lcd_spi_dev_driver);
}

module_init(epson_lcd_init);
module_exit(epson_lcd_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Epson VGA LCD init driver");
MODULE_LICENSE("GPL");
