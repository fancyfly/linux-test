/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/ata.h>
#include <linux/regulator/consumer.h>
#include <linux/android_pmem.h>
#include <linux/usb/android_composite.h>
#include <linux/pmic_external.h>
#include <linux/pmic_status.h>
#include <linux/ipu.h>
#include <linux/mxcfb.h>
#include <linux/pwm_backlight.h>
#include <linux/ahci_platform.h>
#include <linux/gpio_keys.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>
#include <linux/leds.h>
#include <linux/leds-mc34708.h>
#include <linux/powerkey.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/keypad.h>
#include <linux/spi/flash.h>
#include <mach/memory.h>
#include <mach/gpio.h>
#include <mach/mmc.h>
#include <mach/mxc_dvfs.h>
#include <mach/iomux-mx53.h>
#include <mach/i2c.h>
#include <mach/mxc_iim.h>
#include <mach/mxc_rfkill.h>

#include <linux/input/adxl34x.h>
#include <linux/input/lis3dh.h>
#include <linux/l3g4200d.h>
#include <linux/akm8975.h>
#include <linux/wlan_plat.h>
#include "crm_regs.h"
#include "devices.h"
#include "usb.h"

/*!
 * @file mach-mx5/mx53_pcba.c
 *
 * @brief This file contains MX53 pcba board specific initialization routines.
 *
 * @ingroup MSL_MX53
 */

/* 
 * MX53 PCBA rev.C GPIO-typed PINs' configurations 
 */

/*
 * Following GPIO definitions are for RGB panel, 800 x 480
 */
#if defined(CONFIG_AT070TN93)
#define MX53_PCBA_BL_PWR_EN		(3*32 + 7)	/* GPIO4_7 */
#define MX53_PCBA_LCD_MODE		(1*32 + 5)	/* GPIO2_5, Once choice one time, before reset (double check) */
#define MX53_PCBA_LCD_DITH			(1 * 32 + 7)	/* GPIO2_7, used as before in rev.B */
#endif

/*
 * Following GPIO definitions are for LVDS panel, 1024 x 600
 */
#if defined(CONFIG_AT070TN2_WSVGA)
#define MX53_PCBA_LCD_CABC_EN1		(1*32 + 14)	/* GPIO2_14 */
#define MX53_PCBA_LCD_CABC_EN2		(1*32 + 15)	/* GPIO2_15 */
#define MX53_PCBA_LCD_SEL		(6*32 + 8)	/* GPIO7_8 */
#define MX53_PCBA_LCD_GPIO0		(6*32 + 9)	/* GPIO7_9 */
#define MX53_PCBA_LCD_GPIO1		(6*32 + 10)	/* GPIO7_10 */
#define MX53_PCBA_LCD_STBY_N		(3*32 + 9)	/* GPIO4_9 */
#endif

/* Common used GPIO pins for both RGB panel & LVDS panel */
#define MX53_PCBA_LCD_UD		(1*32 + 12)	/* GPIO2_12, Up to Down */
#define MX53_PCBA_LCD_LR		(1*32 + 13)	/* GPIO2_13, Left to Right */
#define MX53_PCBA_LCD_PWR_EN		(2*32 + 22)	/* GPIO3_22, Turn VGH, VGL, and AVDD */
#define MX53_PCBA_LCD_RESET		(3*32 + 10)	/* GPIO4_10 */


// CAMERA 1 & 2 Related
#define MX53_PCBA_CAMERA1_RST		(4 * 32 + 20)	/* GPIO5_20 */
#define MX53_PCBA_CAM2_PWR_DOWN		(0*32 + 2)	/* GPIO1_2 */
#define MX53_PCBA_CAM1_ID0		(5*32 + 14)	/* GPIO6_14, i.MX53 input */
#define MX53_PCBA_CAM2_ID0		(5*32 + 15)	/* GPIO6_15, i.MX53 input */
#define MX53_PCBA_CAM2_RESET		(5*32 + 16)	/* GPIO6_16 */
#define MX53_PCBA_CAM1_PWR_DOWN		(6*32 + 13)	/* GPIO7_13 */
#define MX53_PCBA_CAM1_PWDN_VCM		(4 * 32 + 28)	/* GPIO5_28, motor-driven */

// GPS Related
#define MX53_PCBA_GPS_RESET		(0*32 + 4)	/* GPIO1_4 */
#define MX53_PCBA_GPS_1V8_ON		(0*32 + 23)	/* GPIO1_23 */
#define MX53_PCBA_GPS_ONOFF		(0*32 + 24)	/* GPIO1_24, double check in driver */
#define MX53_PCBA_GPS_TSYNC		(0*32 + 25)	/* GPIO1_25, i.MX53 output to GPS */

// MHL Related
#define MX53_PCBA_MHL_RST_N		(0*32 + 29)	/* GPIO1_29 */
#define MX53_PCBA_MHL_1V3_ON		(2*32 + 21)	/* GPIO2_7 */
#define MX53_PCBA_MHL_WAKE		(1*32 + 26)	/* GPIO2_26 */
#define MX53_PCBA_MHL_INT		(4*32 + 0)	/* GPIO5_0 */
#define MX53_PCBA_MHL_3V3_ON		(6*32 + 1)	/* GPIO7_1 */
#define MX53_PCBA_MHL_SW_I2C_SCL	(5 * 32 + 8)	/* GPIO6_8 */
#define MX53_PCBA_MHL_SW_I2C_SDA	(5 * 32 + 10)	/* GPIO6_10 */

// Modem Related
#define MX53_PCBA_MODEM_WAKEUP_OUT	(0*32 + 26)	/* GPIO1_26, i.MX53 input, Modem wake up AP */
#define MX53_PCBA_MODEM_WAKEUP_IN	(0*32 + 27)	/* GPIO1_27, i.MX53 output, to alert Modem */
#define MX53_PCBA_MODEM_PWR_ON		(0*32 + 30)	/* GPIO1_30 */
#define MX53_PCBA_USIM_CDT             (3*32 + 8)      /* GPIO4_8, because too consideration to make it completely work up */

// Audio Related
#define MX53_PCBA_AU_LDO_EN		(0*32 + 28)	/* GPIO1_28, i.MX53 open WM8994 internal LDO */
#define MX53_PCBA_AUD_REQ		(0*32 + 31)	/* GPIO1_31, WM8994's interrupt to i.MX53 */

// USB Related
#define MX53_PCBA_USB_OTG_PWR_EN	(1*32 + 4)	/* GPIO2_4, i.MX53 output */
#define MX53_PCBA_OTG_PWR_FAULT		(6*32 + 0)	/* GPIO7_0, i.MX53 input */

// Sensor Related
#define MX53_PCBA_GYRO_DRDY 		(1*32 + 25)	/* GPIO2_25, indicate to i.MX53 that data is ready from Gyro */
#define MX53_PCBA_COMPASS_INT		(2*32 + 15)	/* GPIO3_15 */
#define MX53_PCBA_GYRO_INT		(2*32 + 20)	/* GPIO3_20 */
#define MX53_PCBA_GSENSOR_INT1		(5*32 + 12)	/* GPIO6_12 */
#define MX53_PCBA_GSENSOR_INT2		(5*32 + 13)	/* GPIO6_13 */

// WLAN Related
#define MX53_PCBA_WLAN_WAKE_B		(1*32 + 23)	/* GPIO2_23, i.MX53 alerts WLAN */
#define MX53_PCBA_WLAN_HOST_WAKE_B	(1*32 + 24)	/* GPIO2_24, WLAN alerts i.MX53 */
#define MX53_PCBA_WLAN_VCC_EN		(3*32 + 20)	/* GPIO4_20 */
#define MX53_PCBA_WLAN_ENABLE		(5*32 + 11)	/* GPIO6_11 */

// Bluetooth Related
#define MX53_PCBA_BT_WAKE		(2*32 + 13)	/* GPIO3_13 */
#define MX53_PCBA_BT_HOST_WAKE		(2*32 + 14)	/* GPIO3_14 */
#define MX53_PCBA_BT_ENABLE		(5*32 + 9)	/* GPIO6_9 */

// SD & eMMC Related
#define MX53_PCBA_SD_PWR_EN		(1*32 + 6)	/* GPIO2_6 */
#define MX53_PCBA_SD1_CD		(3*32 + 6)	/* GPIO4_6 */

// PMIC Related
#define MX53_PCBA_PMIC_ICTEST		(4*32 + 2) 	/* GPIO5_2, PMIC work mode */
#define MX53_PCBA_PMIC_INT		(6*32 + 11)	/* GPIO7_11 */

// Key Input Related
#define MX53_PCBA_KEY_VOL_DOWN		(3*32 + 11)	/* GPIO4_11 */
#define MX53_PCBA_KEY_VOL_UP		(3*32 + 14)	/* GPIO4_14 */
#define MX53_PCBA_KEY_FLASH_LED3	(3*32 + 15)	/* GPIO4_15, i.MX53 output */

// Headset Related
#define MX53_PCBA_HEADSET_DECT		(6*32 + 2)	/* GPIO7_2 */

// Touch Pad Related
#define MX53_PCBA_TP_SHUTDOWN		(6*32 + 3)	/* GPIO7_3 */
#define MX53_PCBA_TOUCH_INT		(6*32 + 6)	/* GPIO7_6 */
#if defined(CONFIG_AT070TN93)
#define MX53_PCBA_TK_INT		(6*32 + 7)	/* GPIO7_7, Touch Key interrupt for old IO board */
#endif
#if defined(CONFIG_AT070TN2_WSVGA)
#define MX53_PCBA_TK_INT		(3*32 + 7)	/* GPIO4_7, Touch Key interrupt for new IO board */
#endif

#define MX53_PCBA_TOUCH_RST		(6*32 + 3)	/* GPIO7_3 */

// Misc Related
#define MX53_PCBA_POWER_ON_1V8_PE	(4 * 32 + 29)	/* GPIO5_29, Control the global 1v8 power network on board */
#define MX53_PCBA_WDT_OUTPUT		(0*32 + 9)	/* GPIO1_9, i.MX53 send out to PMIC */
#define MX53_PCBA_BTCFG10		(1*32 + 27)	/* GPIO2_27, if output set high, pre-charge to approach several mA current, PMIC */

#define VUSBSEL_LSH		2
#define VUSBSEL_WID		1
#define OTGEN_LSH		9
#define OTGEN_WID		1

#define ManualSW_LSH		1
#define ManualSW_WID		1
#define SWHOLD_LSH			12
#define SWHOLD_WID			1
#define DPSWITCHING_LSH     17
#define DPSWITCHING_WID     3
#define DMSWITCHING_LSH     20
#define DMSWITCHING_WID     3
#define CHREN_LSH			3
#define CHREN_WID           1
#define USBtiming_LSH       16
#define USBtiming_WID       4
#define REG_USB_TIMING      37
#define REG_BATTERY_PROFILE 51

extern int __init mx53_pcba_init_mc34708(void);
extern void mxc_mmc_force_detect(int id);
static iomux_v3_cfg_t mx53_pcba_pads[] = {
	/* GYRO_DRDY */
	MX53_PAD_EIM_OE__GPIO2_25,
	/* MHL_INT */
	MX53_PAD_EIM_WAIT__GPIO5_0,
	/* MHL_WAKE */
	MX53_PAD_EIM_RW__GPIO2_26,
	/* MHL_RST_N */
	MX53_PAD_FEC_TXD1__GPIO1_29,
	/* MHL_3V3_ON */
	MX53_PAD_PATA_BUFFER_EN__GPIO7_1,
	/* CSPI1 */
	MX53_PAD_EIM_EB2__ECSPI1_SS0,
	MX53_PAD_EIM_D16__ECSPI1_SCLK,
	MX53_PAD_EIM_D17__ECSPI1_MISO,
	MX53_PAD_EIM_D18__ECSPI1_MOSI,
	MX53_PAD_EIM_D19__ECSPI1_SS1,
	/* BT_UART3 */
	MX53_PAD_EIM_D24__UART3_TXD_MUX,
	MX53_PAD_EIM_D25__UART3_RXD_MUX,
	MX53_PAD_EIM_EB3__UART3_RTS,
	MX53_PAD_EIM_D23__UART3_CTS,
	/* WLAN_WAKE */
	MX53_PAD_EIM_CS0__GPIO2_23,
	/* WLAN_HOST_WAKE */
	MX53_PAD_EIM_CS1__GPIO2_24,
	/* DISP1 */
	MX53_PAD_EIM_A16__IPU_DI1_DISP_CLK,
	MX53_PAD_EIM_A17__IPU_DISP1_DAT_12,
	MX53_PAD_EIM_A18__IPU_DISP1_DAT_13,
	MX53_PAD_EIM_A19__IPU_DISP1_DAT_14,
	MX53_PAD_EIM_A20__IPU_DISP1_DAT_15,
	MX53_PAD_EIM_A21__IPU_DISP1_DAT_16,
	MX53_PAD_EIM_A22__IPU_DISP1_DAT_17,
	MX53_PAD_EIM_A23__IPU_DISP1_DAT_18,
	MX53_PAD_EIM_A24__IPU_DISP1_DAT_19,
	MX53_PAD_EIM_D26__IPU_DISP1_DAT_22,
	MX53_PAD_EIM_D27__IPU_DISP1_DAT_23,
	MX53_PAD_EIM_D30__IPU_DISP1_DAT_21,
	MX53_PAD_EIM_D31__IPU_DISP1_DAT_20,
	MX53_PAD_EIM_DA0__IPU_DISP1_DAT_9,
	MX53_PAD_EIM_DA1__IPU_DISP1_DAT_8,
	MX53_PAD_EIM_DA2__IPU_DISP1_DAT_7,
	MX53_PAD_EIM_DA3__IPU_DISP1_DAT_6,
	MX53_PAD_EIM_DA4__IPU_DISP1_DAT_5,
	MX53_PAD_EIM_DA5__IPU_DISP1_DAT_4,
	MX53_PAD_EIM_DA6__IPU_DISP1_DAT_3,
	MX53_PAD_EIM_DA7__IPU_DISP1_DAT_2,
	MX53_PAD_EIM_DA8__IPU_DISP1_DAT_1,
	MX53_PAD_EIM_DA9__IPU_DISP1_DAT_0,
	MX53_PAD_EIM_EB0__IPU_DISP1_DAT_11,
	MX53_PAD_EIM_EB1__IPU_DISP1_DAT_10,
	MX53_PAD_EIM_DA10__IPU_DI1_PIN15,
	MX53_PAD_EIM_DA11__IPU_DI1_PIN2,
	MX53_PAD_EIM_DA12__IPU_DI1_PIN3,
	/* PMIC_ICTEST */
	MX53_PAD_EIM_A25__GPIO5_2,
	/* GYRO_INT */
	MX53_PAD_EIM_D20__GPIO3_20,
	/* WLAN_VCC_EN */
	MX53_PAD_DI0_PIN4__GPIO4_20,
	/* GPS_UART2 */
	MX53_PAD_EIM_D29__UART2_RTS,
	MX53_PAD_EIM_D28__UART2_CTS,
	MX53_PAD_GPIO_7__UART2_TXD_MUX,
	MX53_PAD_GPIO_8__UART2_RXD_MUX,
	/* BT_WAKE */
	MX53_PAD_EIM_DA13__GPIO3_13,
	/* BT_HOST_WAKE */
	MX53_PAD_EIM_DA14__GPIO3_14,
	/* COMPASS_INT */
	MX53_PAD_EIM_DA15__GPIO3_15,
	/* GSENSOR INT2 */
	MX53_PAD_NANDF_RE_B__GPIO6_13,
	/* MHL_SW_I2C */
	MX53_PAD_NANDF_RB0__GPIO6_10,
	MX53_PAD_NANDF_ALE__GPIO6_8,
	/* BT_ENABLE */
	MX53_PAD_NANDF_WP_B__GPIO6_9,
	/* GSENSOR_INT1 */
	MX53_PAD_NANDF_WE_B__GPIO6_12,
	/* TOUCH_INT */
	MX53_PAD_PATA_DA_0__GPIO7_6,
	/* WLAN_ENABLE */
	MX53_PAD_NANDF_CS0__GPIO6_11,
	/* CAM1_ID0 */
	MX53_PAD_NANDF_CS1__GPIO6_14,
	/* CAM2_ID0 */
	MX53_PAD_NANDF_CS2__GPIO6_15,
	/* CAM2_RESET */
	MX53_PAD_NANDF_CS3__GPIO6_16,
	/* AUDMUX */
	MX53_PAD_CSI0_DAT4__AUDMUX_AUD3_TXC,
	MX53_PAD_CSI0_DAT5__AUDMUX_AUD3_TXD,
	MX53_PAD_CSI0_DAT6__AUDMUX_AUD3_TXFS,
	MX53_PAD_CSI0_DAT7__AUDMUX_AUD3_RXD,
	/* I2C1 */
	MX53_PAD_CSI0_DAT8__I2C1_SDA,
	MX53_PAD_CSI0_DAT9__I2C1_SCL,
	/* CSI0 */
	MX53_PAD_CSI0_DAT12__IPU_CSI0_D_12,
	MX53_PAD_CSI0_DAT13__IPU_CSI0_D_13,
	MX53_PAD_CSI0_DAT14__IPU_CSI0_D_14,
	MX53_PAD_CSI0_DAT15__IPU_CSI0_D_15,
	MX53_PAD_CSI0_DAT16__IPU_CSI0_D_16,
	MX53_PAD_CSI0_DAT17__IPU_CSI0_D_17,
	MX53_PAD_CSI0_DAT18__IPU_CSI0_D_18,
	MX53_PAD_CSI0_DAT19__IPU_CSI0_D_19,
	MX53_PAD_CSI0_VSYNC__IPU_CSI0_VSYNC,
	MX53_PAD_CSI0_MCLK__IPU_CSI0_HSYNC,
	MX53_PAD_CSI0_PIXCLK__IPU_CSI0_PIXCLK,
	/* CAMERA1_RST */
	MX53_PAD_CSI0_DATA_EN__GPIO5_20,
	/* MODEM_WAKEUP_IN */
	MX53_PAD_FEC_RXD0__GPIO1_27,
	/* MODEM_WAKEUP_OUT */
	MX53_PAD_FEC_RXD1__GPIO1_26,
	/* MODEM_PWR_ON */
	MX53_PAD_FEC_TXD0__GPIO1_30,
	/* GPS_ONOFF */
	MX53_PAD_FEC_RX_ER__GPIO1_24,
	/* AU_LDO_EN */
	MX53_PAD_FEC_TX_EN__GPIO1_28,
	/* GPS_1V8_ON */
	MX53_PAD_FEC_REF_CLK__GPIO1_23,
	/* GPS_TSYNC */
	MX53_PAD_FEC_CRS_DV__GPIO1_25,
	/* AUD_REQ */
	MX53_PAD_FEC_MDC__GPIO1_31,
	/* PMIC ON REQ2 */
	MX53_PAD_FEC_MDIO__GPIO1_22,
	/* KEY_VOL+ */
	MX53_PAD_KEY_COL4__GPIO4_14,
	/* KEY_VOL- */
	MX53_PAD_KEY_ROW2__GPIO4_11,
	/* I2C2 */
	MX53_PAD_KEY_COL3__I2C2_SCL,
	MX53_PAD_KEY_ROW3__I2C2_SDA,
	/* KEY FLASH LED3 */
	MX53_PAD_KEY_ROW4__GPIO4_15,
	/* SD1 */
	MX53_PAD_SD1_CMD__ESDHC1_CMD,
	MX53_PAD_SD1_CLK__ESDHC1_CLK,
	MX53_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX53_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX53_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX53_PAD_SD1_DATA3__ESDHC1_DAT3,
	/* SD2 */
	MX53_PAD_SD2_CMD__ESDHC2_CMD,
	MX53_PAD_SD2_CLK__ESDHC2_CLK,
	MX53_PAD_SD2_DATA0__ESDHC2_DAT0,
	MX53_PAD_SD2_DATA1__ESDHC2_DAT1,
	MX53_PAD_SD2_DATA2__ESDHC2_DAT2,
	MX53_PAD_SD2_DATA3__ESDHC2_DAT3,
	#if defined(CONFIG_AT070TN93)
	/* TK_INT */
	MX53_PAD_PATA_DA_1__GPIO7_7,
	#endif
	#if defined(CONFIG_AT070TN2_WSVGA)
	/* TK_INT */
	MX53_PAD_KEY_ROW0__GPIO4_7,
	#endif
	/* SD3 */
	MX53_PAD_PATA_DATA8__ESDHC3_DAT0,
	MX53_PAD_PATA_DATA9__ESDHC3_DAT1,
	MX53_PAD_PATA_DATA10__ESDHC3_DAT2,
	MX53_PAD_PATA_DATA11__ESDHC3_DAT3,
	MX53_PAD_PATA_DATA0__ESDHC3_DAT4,
	MX53_PAD_PATA_DATA1__ESDHC3_DAT5,
	MX53_PAD_PATA_DATA2__ESDHC3_DAT6,
	MX53_PAD_PATA_DATA3__ESDHC3_DAT7,
	MX53_PAD_PATA_IORDY__ESDHC3_CLK,
	MX53_PAD_PATA_RESET_B__ESDHC3_CMD,
	/* OTG_PWR_EN */
	MX53_PAD_PATA_DATA4__GPIO2_4,
	/* TP_SHUTDOWN */
	MX53_PAD_PATA_DIOR__GPIO7_3,
	/* UART1 */
	MX53_PAD_PATA_DIOW__UART1_TXD_MUX,
	MX53_PAD_PATA_DMACK__UART1_RXD_MUX,
	/* OTG_PWR_FAULT */
	MX53_PAD_PATA_DMARQ__GPIO7_0,
	/* HEADSET_DET */
	MX53_PAD_PATA_INTRQ__GPIO7_2,
	/* CAM_MCLK */
	MX53_PAD_GPIO_0__CCM_SSI_EXT1_CLK,
	/* CAM2_PWR_DWN */
	MX53_PAD_GPIO_2__GPIO1_2,
	/* AU_MCLK1 */
	MX53_PAD_GPIO_3__CCM_CLKO2,
	/* GPS_RESET */
	MX53_PAD_GPIO_4__GPIO1_4,
	/* I2C3 */
	MX53_PAD_GPIO_5__I2C3_SCL,
	MX53_PAD_GPIO_6__I2C3_SDA,
	/* PMIC_INT */
	MX53_PAD_GPIO_16__GPIO7_11,
	/* PMIC_ICTEST */
	MX53_PAD_EIM_A25__GPIO5_2,
	/* HDMI SPDIF_TX*/
	MX53_PAD_GPIO_17__SPDIF_OUT1,
	/* CAM1 POWER DOWN */
	MX53_PAD_GPIO_18__GPIO7_13,
	/* SD_PWR_EN */
	MX53_PAD_PATA_DATA6__GPIO2_6,

	/*
	 * Following mux definitions are used for both LVDS and RGB.
	 */
	/* LCD_UD */
	MX53_PAD_PATA_DATA12__GPIO2_12,
	/* LCD_LR */
	MX53_PAD_PATA_DATA13__GPIO2_13,
	/* LCD_BL_PWM */
	MX53_PAD_GPIO_1__PWM2_PWMO,
	/* LCD_PWR_EN */
	MX53_PAD_EIM_D22__GPIO3_22,
	/* LCD_RESET */
	MX53_PAD_KEY_COL2__GPIO4_10,

	#if defined(CONFIG_AT070TN2_WSVGA)
	/* LCD_GPIO0 */
	MX53_PAD_PATA_CS_0__GPIO7_9,
	/* LCD_GPIO1 */
	MX53_PAD_PATA_CS_1__GPIO7_10,
	/* LCD_SEL */
	MX53_PAD_PATA_DA_2__GPIO7_8,
	/* LCD_STBY_N */
	MX53_PAD_KEY_ROW1__GPIO4_9,
	/* LCD_CABC_EN1 */
	MX53_PAD_PATA_DATA14__GPIO2_14,
	/* LCD_CABC_EN2 */
	MX53_PAD_PATA_DATA15__GPIO2_15,
	/* LVDS */
	MX53_PAD_LVDS0_TX3_P__LDB_LVDS0_TX3,
	MX53_PAD_LVDS0_CLK_P__LDB_LVDS0_CLK,
	MX53_PAD_LVDS0_TX2_P__LDB_LVDS0_TX2,
	MX53_PAD_LVDS0_TX1_P__LDB_LVDS0_TX1,
	MX53_PAD_LVDS0_TX0_P__LDB_LVDS0_TX0,

	/*
	 * Following are MUX pins to be programmed in INPUT mode to save power.
	 */
	MX53_PAD_DI0_DISP_CLK__GPIO4_16,
	MX53_PAD_DI0_PIN15__GPIO4_17,
	MX53_PAD_DI0_PIN2__GPIO4_18,
	MX53_PAD_DI0_PIN3__GPIO4_19,
	MX53_PAD_DISP0_DAT0__GPIO4_21,
	MX53_PAD_DISP0_DAT1__GPIO4_22,
	MX53_PAD_DISP0_DAT2__GPIO4_23,
	MX53_PAD_DISP0_DAT3__GPIO4_24,
	MX53_PAD_DISP0_DAT4__GPIO4_25,
	MX53_PAD_DISP0_DAT5__GPIO4_26,
	MX53_PAD_DISP0_DAT6__GPIO4_27,
	MX53_PAD_DISP0_DAT7__GPIO4_28,
	MX53_PAD_DISP0_DAT8__GPIO4_29,
	MX53_PAD_DISP0_DAT9__GPIO4_30,
	MX53_PAD_DISP0_DAT10__GPIO4_31,
	MX53_PAD_DISP0_DAT11__GPIO5_5,
	MX53_PAD_DISP0_DAT12__GPIO5_6,
	MX53_PAD_DISP0_DAT13__GPIO5_7,
	MX53_PAD_DISP0_DAT14__GPIO5_8,
	MX53_PAD_DISP0_DAT15__GPIO5_9,
	MX53_PAD_DISP0_DAT16__GPIO5_10,
	MX53_PAD_DISP0_DAT17__GPIO5_11,
	MX53_PAD_DISP0_DAT18__GPIO5_12,
	MX53_PAD_DISP0_DAT19__GPIO5_13,
	MX53_PAD_DISP0_DAT20__GPIO5_14,
	MX53_PAD_DISP0_DAT21__GPIO5_15,
	MX53_PAD_DISP0_DAT22__GPIO5_16,
	MX53_PAD_DISP0_DAT23__GPIO5_17,

	/* LCD_MODE */
	MX53_PAD_PATA_DATA5__GPIO2_5,
	/* LCD_DITH */
	MX53_PAD_PATA_DATA7__GPIO2_7,

	#endif

	#if defined(CONIG_AT070TN93)
	/* DISPLAY */
	MX53_PAD_DI0_DISP_CLK__IPU_DI0_DISP_CLK,
	MX53_PAD_DI0_PIN15__IPU_DI0_PIN15,
	MX53_PAD_DI0_PIN2__IPU_DI0_PIN2,
	MX53_PAD_DI0_PIN3__IPU_DI0_PIN3,
	MX53_PAD_DISP0_DAT0__IPU_DISP0_DAT_0,
	MX53_PAD_DISP0_DAT1__IPU_DISP0_DAT_1,
	MX53_PAD_DISP0_DAT2__IPU_DISP0_DAT_2,
	MX53_PAD_DISP0_DAT3__IPU_DISP0_DAT_3,
	MX53_PAD_DISP0_DAT4__IPU_DISP0_DAT_4,
	MX53_PAD_DISP0_DAT5__IPU_DISP0_DAT_5,
	MX53_PAD_DISP0_DAT6__IPU_DISP0_DAT_6,
	MX53_PAD_DISP0_DAT7__IPU_DISP0_DAT_7,
	MX53_PAD_DISP0_DAT8__IPU_DISP0_DAT_8,
	MX53_PAD_DISP0_DAT9__IPU_DISP0_DAT_9,
	MX53_PAD_DISP0_DAT10__IPU_DISP0_DAT_10,
	MX53_PAD_DISP0_DAT11__IPU_DISP0_DAT_11,
	MX53_PAD_DISP0_DAT12__IPU_DISP0_DAT_12,
	MX53_PAD_DISP0_DAT13__IPU_DISP0_DAT_13,
	MX53_PAD_DISP0_DAT14__IPU_DISP0_DAT_14,
	MX53_PAD_DISP0_DAT15__IPU_DISP0_DAT_15,
	MX53_PAD_DISP0_DAT16__IPU_DISP0_DAT_16,
	MX53_PAD_DISP0_DAT17__IPU_DISP0_DAT_17,
	MX53_PAD_DISP0_DAT18__IPU_DISP0_DAT_18,
	MX53_PAD_DISP0_DAT19__IPU_DISP0_DAT_19,
	MX53_PAD_DISP0_DAT20__IPU_DISP0_DAT_20,
	MX53_PAD_DISP0_DAT21__IPU_DISP0_DAT_21,
	MX53_PAD_DISP0_DAT22__IPU_DISP0_DAT_22,
	MX53_PAD_DISP0_DAT23__IPU_DISP0_DAT_23,

	/* BL_PWR_EN */
	MX53_PAD_KEY_ROW0__GPIO4_7,
	/* LCD_MODE */
	MX53_PAD_PATA_DATA5__GPIO2_5,
	/* LCD_DITH */
	MX53_PAD_PATA_DATA7__GPIO2_7,
	#endif

	/* USIM_CDT */
	MX53_PAD_KEY_COL1__GPIO4_8,
	
	/* Following are the new added mux pins for rev.C */
	/* WDT_OUTPUT */
	MX53_PAD_GPIO_9__GPIO1_9,
	/* POWER_ON_1V8_PERI */
	MX53_PAD_CSI0_DAT11__GPIO5_29,
	/* CAM1 PWDN VCM */
	MX53_PAD_CSI0_DAT10__GPIO5_28,
	/* SDIO1_CD */
	MX53_PAD_KEY_COL0__GPIO4_6,
	/* MHL_1V3_ON */
	MX53_PAD_EIM_D21__GPIO3_21,
	/* BTCFG10 */
	MX53_PAD_EIM_LBA__GPIO2_27,
	/* Not used pin*/
	MX53_PAD_GPIO_10__GPIO4_0,
	MX53_PAD_GPIO_11__GPIO4_1,
	MX53_PAD_GPIO_12__GPIO4_2,
	MX53_PAD_GPIO_13__GPIO4_3,
	MX53_PAD_GPIO_14__GPIO4_4,
	MX53_PAD_KEY_ROW1__GPIO4_9,
};


static iomux_v3_cfg_t suspend_enter_pads[] = {
	/* Audio */
	/*HEADSET_DET*/
	MX53_PAD_PATA_INTRQ__GPIO7_2,
	/*AU_MCLK1*/
	MX53_PAD_GPIO_3__GPIO1_3,
	/*AU_LDO_EN Output*/
	MX53_PAD_FEC_TX_EN__GPIO1_28,
	/*AUD_REQ*/
	MX53_PAD_FEC_MDC__GPIO1_31_PD,
	/*AP_I2S_CLK*/
	MX53_PAD_CSI0_DAT4__GPIO5_22_PD,
	/*AP_I2S_DOUT*/
	MX53_PAD_CSI0_DAT5__GPIO5_23_PD,
	/*AP_I2S_SYNC*/
	MX53_PAD_CSI0_DAT6__GPIO5_24_PD,
	/*AP_I2S_DIN*/
	MX53_PAD_CSI0_DAT7__GPIO5_25_PD,

	/* Camera */
	/* CAM1_POWER_DOWN Output*/
	MX53_PAD_GPIO_18__GPIO7_13,
	/*CAM2_POWER_DOWN Output*/
	MX53_PAD_GPIO_2__GPIO1_2,
	/* CAM1_PWDN_VCM Output*/
	MX53_PAD_CSI0_DAT9__GPIO5_27,
	/* CAM1_ID0*/
	MX53_PAD_NANDF_CS1__GPIO6_14_PD,
	/* CAM2_ID0*/
	MX53_PAD_NANDF_CS2__GPIO6_15_PD,
  	/* CAM1_RESET*/
	MX53_PAD_CSI0_DATA_EN__GPIO5_20_PD,
	/* CAM2_RESET*/
	MX53_PAD_NANDF_CS3__GPIO6_16_PD,
	/* CAM_DATA4 */
	MX53_PAD_CSI0_DAT12__GPIO5_30_PD,
	/* CAM_DATA5 */
	MX53_PAD_CSI0_DAT13__GPIO5_31_PD,
	/* CAM_DATA6 */
	MX53_PAD_CSI0_DAT14__GPIO6_0_PD,
        /* CAM_DATA7 */
	MX53_PAD_CSI0_DAT15__GPIO6_1_PD,
	/* CAM_DATA8 */
	MX53_PAD_CSI0_DAT16__GPIO6_2_PD,
	/* CAM_DATA9 */
	MX53_PAD_CSI0_DAT17__GPIO6_3_PD,
	/* CAM_DATA10 */
	MX53_PAD_CSI0_DAT18__GPIO6_4_PD,
	/* CAM_DATA11 */
	MX53_PAD_CSI0_DAT19__GPIO6_5_PD,
	/* CAM_VSYNC */
	MX53_PAD_CSI0_VSYNC__GPIO5_21_PD,
	/* CAM_HSYNC*/
	MX53_PAD_CSI0_MCLK__GPIO5_19_PD,
	/* CAM_PCLK */
	MX53_PAD_CSI0_PIXCLK__GPIO5_18_PD,
	/*CAM_MCLK*/
	MX53_PAD_GPIO_0__GPIO1_0_PD,

	/* EMMC - NOT PAD CTRL */
	/*SDIO1_CLK*/
	MX53_PAD_SD1_CLK__GPIO1_20_PD,
	/*SDIO1_CMD*/
	MX53_PAD_SD1_CMD__GPIO1_18_PD,
	/*SDIO1_DATA0:3*/
	MX53_PAD_SD1_DATA0__GPIO1_16_PD,
	MX53_PAD_SD1_DATA1__GPIO1_17_PD,
	MX53_PAD_SD1_DATA2__GPIO1_19_PD,
	MX53_PAD_SD1_DATA3__GPIO1_21_PD,
	/*W_SDIO_CLK*/
	MX53_PAD_SD2_CLK__GPIO1_10_PD,
	/*W_SDIO_CMD*/
	MX53_PAD_SD2_CMD__GPIO1_11_PD,
	/*W_SDIO_DATA0:3*/
	MX53_PAD_SD2_DATA0__GPIO1_15_PD,
	MX53_PAD_SD2_DATA1__GPIO1_14_PD,
	MX53_PAD_SD2_DATA2__GPIO1_13_PD,
	MX53_PAD_SD2_DATA3__GPIO1_12_PD,
	/* SDIO1_CD */
	MX53_PAD_KEY_COL0__GPIO4_6,


	/* TOUCH_ID0 */
	MX53_PAD_KEY_COL1__GPIO4_8_PD,
	/* TOUCH_ID1*/
	MX53_PAD_KEY_COL2__GPIO4_10_PD,
	/*TOUCH_ATT*/
	MX53_PAD_PATA_DA_0__GPIO7_6,
	/* TOUCH_RST */
	MX53_PAD_PATA_DIOR__GPIO7_3_PD,

	/* GPS */
	/* GPS_RESET_N*/
	MX53_PAD_GPIO_4__GPIO1_4_PD,
	/* GPS_UART2_TXD*/
	MX53_PAD_GPIO_7__GPIO1_7_PD,
	/* GPS_UART2_RXD*/
	MX53_PAD_GPIO_8__GPIO1_8_PD,
	/* GPS_ON_OFF  Output*/
	MX53_PAD_FEC_RX_ER__GPIO1_24,
	/* GPS_TSYNC */
	MX53_PAD_FEC_CRS_DV__GPIO1_25_PD,
	/* TCXO_PWR_EN  Output*/
	MX53_PAD_FEC_REF_CLK__GPIO1_23,

	/* HDMI */
	/* HDMI_PWR_FAULT  */
	MX53_PAD_PATA_BUFFER_EN__GPIO7_1,
	/* HDMI_PWR_EN  Output*/
	MX53_PAD_PATA_DATA7__GPIO2_7,
	/* HDMI_RESET*/
	MX53_PAD_FEC_TXD1__GPIO1_29,
	/* DSS1_PCLK_BTCFG11*/
	MX53_PAD_EIM_A16__GPIO2_22_PD,
	/* DSS1_DAT12_BTCFG12*/
	MX53_PAD_EIM_A17__GPIO2_21_PD,
	/* DSS1_DAT13_BTCFG13*/
	MX53_PAD_EIM_A18__GPIO2_20_PD,
	/* DSS1_DAT14_BTCFG14*/
	MX53_PAD_EIM_A19__GPIO2_19_PD,
	/* DSS1_DAT15_BTCFG15*/
	MX53_PAD_EIM_A20__GPIO2_18_PD,
	/* DSS1_DAT16_BTCFG16*/
	MX53_PAD_EIM_A21__GPIO2_17_PD,
	/* DSS1_DAT17_BTCFG17*/
	MX53_PAD_EIM_A22__GPIO2_16_PD,
	/* DSS1_DAT18 */
	MX53_PAD_EIM_A23__GPIO6_6_PD,
	/* DSS1_DAT19 */
	MX53_PAD_EIM_A24__GPIO5_4_PD,
	/* DSS1_DAT22 */
	MX53_PAD_EIM_D26__GPIO3_26_PD,
	/* DSS1_DAT23 */
	MX53_PAD_EIM_D27__GPIO3_27_PD,
	/* DSS1_DAT21*/
	MX53_PAD_EIM_D30__GPIO3_30_PD,
	/* DSS1_DAT20 */
	MX53_PAD_EIM_D31__GPIO3_31_PD,
	/*DSS1_DAT9_BTCFG25*/
	MX53_PAD_EIM_DA0__GPIO3_0_PD,
	/*DSS1_DAT8_BTCFG24*/
	MX53_PAD_EIM_DA1__GPIO3_1_PD,
	/*DSS1_DAT7_BTCFG23*/
	MX53_PAD_EIM_DA2__GPIO3_2_PD,
	/*DSS1_DAT6_BTCFG22*/
	MX53_PAD_EIM_DA3__GPIO3_3_PD,
	/*DSS1_DAT5_BTCFG37*/
	MX53_PAD_EIM_DA4__GPIO3_4_PD,
	/*DSS1_DAT4_BTCFG36*/
	MX53_PAD_EIM_DA5__GPIO3_5_PD,
	/*DSS1_DAT3_BTCFG35*/
	MX53_PAD_EIM_DA6__GPIO3_6_PD,
	/*DSS1_DAT2_BTCFG34*/
	MX53_PAD_EIM_DA7__GPIO3_7_PD,
	/*DSS1_DAT1_BTCFG33*/
	MX53_PAD_EIM_DA8__GPIO3_8_PD,
	/*DSS1_DAT0_BTCFG32*/
	MX53_PAD_EIM_DA9__GPIO3_9_PD,
	/*DSS1_DRDY_BTCFG31*/
	MX53_PAD_EIM_DA10__GPIO3_10_PD,
	/*DSS1_HSYNC*/
	MX53_PAD_EIM_DA11__GPIO3_11_PD,
	/*DSS1_VSYNC*/
	MX53_PAD_EIM_DA12__GPIO3_12_PD,
	/*DSS1_DAT11_BTCFG27*/
	MX53_PAD_EIM_EB0__GPIO2_28,
	/*DSS1_DAT10_BTCFG26*/
	MX53_PAD_EIM_EB1__GPIO2_29,
	/*HDMI_CEC_AP*/
	MX53_PAD_EIM_RW__GPIO2_26_PD,
	/*HDMI_INT*/
	MX53_PAD_EIM_WAIT__GPIO5_0,
	/*SPDIF_TX*/
	MX53_PAD_GPIO_17__GPIO7_12_PD,

	/* i2c */

	/* keypad: already as gpio input */

	/* KEY_VOL+ */
	MX53_PAD_KEY_COL4__GPIO4_14_PD,
	/* KEY_VOL- */
	MX53_PAD_KEY_ROW2__GPIO4_11_PD,
	/* KEY_SEARCH */
	MX53_PAD_NANDF_RB0__GPIO6_10_PD,
	/* KEY_BACK */
	MX53_PAD_NANDF_ALE__GPIO6_8_PD,
	/* KEY_MENU */
	MX53_PAD_NANDF_CLE__GPIO6_7_PD,
	/*KEY_HOME*/
	MX53_PAD_CSI0_DAT8__GPIO5_26_PD,
	#if defined(CONFIG_AT070TN93)
	/* BL_PWR_EN Output*/
	MX53_PAD_KEY_ROW0__GPIO4_7,
	#endif

	/* LCD_ID */
	MX53_PAD_PATA_CS_0__GPIO7_9_PD,
	/* LCD_MODE */
	MX53_PAD_PATA_DATA5__GPIO2_5,
	/* LCD_PWR_EN Output*/
	MX53_PAD_EIM_D22__GPIO3_22,
	/* SD_PWR_EN Output*/
	MX53_PAD_PATA_DATA6__GPIO2_6,
	/* LED_PWM_OUT*/
	MX53_PAD_PATA_CS_1__GPIO7_10_PD,
	/* LCD_CABC_EN3 */
	MX53_PAD_PATA_DA_1__GPIO7_7_PD,
	/* LCD_SEL */
	MX53_PAD_PATA_DA_2__GPIO7_8_PD,
	/*LCD_UD*/
	MX53_PAD_PATA_DATA12__GPIO2_12_PD,
	/*LCD_LR*/
	MX53_PAD_PATA_DATA13__GPIO2_13_PD,
	/*LCD_CABC_EN1*/
	MX53_PAD_PATA_DATA14__GPIO2_14_PD,
	/*LCD_CABC_EN2*/
	MX53_PAD_PATA_DATA15__GPIO2_15_PD,
	/*LCD_BL_PWM Output*/
	MX53_PAD_GPIO_1__GPIO1_1,
	/*DSS_PCLK*/
	MX53_PAD_DI0_DISP_CLK__GPIO4_16_PD,
	/*DSS_DRDY*/
	MX53_PAD_DI0_PIN15__GPIO4_17_PD,
	/*DSS_HSYNC*/
	MX53_PAD_DI0_PIN2__GPIO4_18_PD,
	/*DSS_VSYNC*/
	MX53_PAD_DI0_PIN3__GPIO4_19_PD,
	/*DSS_DATA0:23*/
	MX53_PAD_DISP0_DAT0__GPIO4_21_PD,
	MX53_PAD_DISP0_DAT1__GPIO4_22_PD,
	MX53_PAD_DISP0_DAT2__GPIO4_23_PD,
	MX53_PAD_DISP0_DAT3__GPIO4_24_PD,
	MX53_PAD_DISP0_DAT4__GPIO4_25_PD,
	MX53_PAD_DISP0_DAT5__GPIO4_26_PD,
	MX53_PAD_DISP0_DAT6__GPIO4_27_PD,
	MX53_PAD_DISP0_DAT7__GPIO4_28_PD,
	MX53_PAD_DISP0_DAT8__GPIO4_29_PD,
	MX53_PAD_DISP0_DAT9__GPIO4_30_PD,
	MX53_PAD_DISP0_DAT10__GPIO4_31_PD,
	MX53_PAD_DISP0_DAT11__GPIO5_5_PD,
	MX53_PAD_DISP0_DAT12__GPIO5_6_PD,
	MX53_PAD_DISP0_DAT13__GPIO5_7_PD,
	MX53_PAD_DISP0_DAT14__GPIO5_8_PD,
	MX53_PAD_DISP0_DAT15__GPIO5_9_PD,
	MX53_PAD_DISP0_DAT16__GPIO5_10_PD,
	MX53_PAD_DISP0_DAT17__GPIO5_11_PD,
	MX53_PAD_DISP0_DAT18__GPIO5_12_PD,
	MX53_PAD_DISP0_DAT19__GPIO5_13_PD,
	MX53_PAD_DISP0_DAT20__GPIO5_14_PD,
	MX53_PAD_DISP0_DAT21__GPIO5_15_PD,
	MX53_PAD_DISP0_DAT22__GPIO5_16_PD,
	MX53_PAD_DISP0_DAT23__GPIO5_17_PD,

	/* LED*/
	/*KEY_FLASH_LED3_AP Output*/
	MX53_PAD_KEY_ROW4__GPIO4_15,

	/*CKIH_EN Output*/
	MX53_PAD_FEC_MDIO__GPIO1_22,
	
	/* MicroSD */

	/* UART1 */
        MX53_PAD_PATA_DIOW__GPIO6_17_PD,	//MX53_PAD_PATA_DIOW__UART1_TXD_MUX,
	MX53_PAD_PATA_DMACK__GPIO6_18_PD,  //MX53_PAD_PATA_DMACK__UART1_RXD_MUX,
	
	
	/*Modem*/
	/* MODEM_WAKEUP_IN Output*/
	MX53_PAD_FEC_RXD0__GPIO1_27,
	/* MODEM_WAKEUP_OUT */
	MX53_PAD_FEC_RXD1__GPIO1_26,
	/* MODEM_POWER_ON Output*/
	MX53_PAD_FEC_TXD0__GPIO1_30,

	
	/* misc */ 
	/* OTG_PWR_EN Output*/
	MX53_PAD_PATA_DATA4__GPIO2_4,
	/* OTG_PWR_FAULT */
	MX53_PAD_PATA_DMARQ__GPIO7_0,
	/* PMIC_INT */
	MX53_PAD_GPIO_16__GPIO7_11,
	/*CSPI1*/
	/*SPI1_CLK*/
	MX53_PAD_EIM_D16__GPIO3_16_PD,
	/*SPI1_MISO*/
	MX53_PAD_EIM_D17__GPIO3_17_PD,
	/*SPI1_MOSI*/
	MX53_PAD_EIM_D18__GPIO3_18_PD,
	/*SPI1_CS0*/
	MX53_PAD_EIM_EB2__GPIO2_30_PD,

	/*WDT_OUTPUT*/
	MX53_PAD_GPIO_9__GPIO1_9,
	/* GYRO_INT */
	MX53_PAD_EIM_D29__GPIO3_29,
	/* GYRO_DRDY */
	MX53_PAD_EIM_OE__GPIO2_25,
	/* COMPASS_INT */
	MX53_PAD_EIM_DA15__GPIO3_15,
	/* G_SENSOR_INT1 */
	MX53_PAD_NANDF_WE_B__GPIO6_12,
	/* G_SENSOR_INT2 */
	MX53_PAD_NANDF_RE_B__GPIO6_13,
	/*BTCFG10*/
	MX53_PAD_EIM_LBA__GPIO2_27,
	/*SPICS1 for Flash test pin*/
	MX53_PAD_EIM_D19__GPIO3_19_PD,
	/* Not used pin*/
	MX53_PAD_GPIO_10__GPIO4_0_PD,
	MX53_PAD_GPIO_11__GPIO4_1_PD,
	MX53_PAD_GPIO_12__GPIO4_2_PD,
	MX53_PAD_GPIO_13__GPIO4_3_PD,
	MX53_PAD_GPIO_14__GPIO4_4_PD,
	MX53_PAD_KEY_ROW1__GPIO4_9_PD,
	MX53_PAD_CSI0_DAT10__GPIO5_28_PD,
	MX53_PAD_CSI0_DAT11__GPIO5_29_PD,
	MX53_PAD_GPIO_19__GPIO4_5_PD,
	//MX53_PCBA_WLAN_CLK_REQ,


};
static unsigned int suspend_gpio_out2in[] = {
	/*
	 * Keep it for future possible modification.
	 */
};
/*
static unsigned int suspend_alt_req[] = {
	MX53_PCBA_UART1_TXD,
	MX53_PCBA_UART1_RXD,
	MX53_PCBA_CAM_MCLK,
	MX53_PCBA_AU_MCLK1,
	MX53_PCBA_GPS_UART2_TXD,
	MX53_PCBA_GPS_UART2_RXD,
	MX53_PCBA_WDT_OUTPUT,
	MX53_PCBA_SDIO1_CLK,
	MX53_PCBA_SDIO1_CMD,
	MX53_PCBA_SDIO1_DATA0,
	MX53_PCBA_SDIO1_DATA1,
	MX53_PCBA_SDIO1_DATA2,
	MX53_PCBA_SDIO1_DATA3,
	MX53_PCBA_W_SDIO_CLK,
	MX53_PCBA_W_SDIO_CMD,
	MX53_PCBA_W_SDIO_DATA0,
	MX53_PCBA_W_SDIO_DATA1,
	MX53_PCBA_W_SDIO_DATA2,
	MX53_PCBA_W_SDIO_DATA3,
	MX53_PCBA_DSS1_PCLK_BTCFG11,
	MX53_PCBA_DSS1_DAT12_BTCFG12,
	MX53_PCBA_DSS1_DAT13_BTCFG13,
	MX53_PCBA_DSS1_DAT14_BTCFG14,
	MX53_PCBA_DSS1_DAT15_BTCFG15,
	MX53_PCBA_DSS1_DAT16_BTCFG16,
	MX53_PCBA_DSS1_DAT17_BTCFG17,
	MX53_PCBA_DSS1_DAT18,
	MX53_PCBA_DSS1_DAT19,
	MX53_PCBA_SPI1_CLK,
	MX53_PCBA_SPI1_MISO,
	MX53_PCBA_BT_UART3_CTS,
	MX53_PCBA_BT_UART3_TXD,
	MX53_PCBA_BT_UART3_RXD,
	MX53_PCBA_DSS1_DAT22,
	MX53_PCBA_DSS1_DAT23,
	MX53_PCBA_DSS1_DAT21,
	MX53_PCBA_DSS1_DAT20,
	MX53_PCBA_DSS1_DAT9_BTCFG25,
	MX53_PCBA_DSS1_DAT8_BTCFG24,
	MX53_PCBA_DSS1_DAT7_BTCFG23,
	MX53_PCBA_DSS1_DAT6_BTCFG22,
	MX53_PCBA_DSS1_DAT5_BTCFG37,
	MX53_PCBA_DSS1_DAT4_BTCFG36,
	MX53_PCBA_DSS1_DAT3_BTCFG35,
	MX53_PCBA_DSS1_DAT2_BTCFG34,
	MX53_PCBA_DSS1_DAT1_BTCFG33,
	MX53_PCBA_DSS1_DAT0_BTCFG32,
	MX53_PCBA_DSS1_DRDY_BTCFG31,
	MX53_PCBA_DSS1_HSYNC,
	MX53_PCBA_DSS1_VSYNC,
	MX53_PCBA_DSS1_DAT11_BTCFG27,
	MX53_PCBA_DSS1_DAT10_BTCFG26,
	MX53_PCBA_SPI1_CS0,
	MX53_PCBA_BT_UART3_RTS,
	MX53_PCBA_BTCFG10,
	MX53_PCBA_AP_I2S_CLK,
	MX53_PCBA_AP_I2S_DOUT,
	MX53_PCBA_AP_I2S_SYNC,
	MX53_PCBA_AP_I2S_DIN,
	MX53_PCBA_CAM_DATA4,
	MX53_PCBA_CAM_DATA5,
	MX53_PCBA_CAM_DATA6,
	MX53_PCBA_CAM_DATA7,
	MX53_PCBA_CAM_DATA8,
	MX53_PCBA_CAM_DATA9,
	MX53_PCBA_CAM_DATA10,
	MX53_PCBA_CAM_DATA11,
	MX53_PCBA_CAM_VSYNC,
	MX53_PCBA_CAM_PCLK,
	MX53_PCBA_CAM_HSYNC,
	MX53_PCBA_DSS_HSYNC,
	MX53_PCBA_DSS_VSYNC,
	MX53_PCBA_DSS_DRDY,
	MX53_PCBA_DSS_DATA0,
	MX53_PCBA_DSS_DATA1,
	MX53_PCBA_DSS_DATA2,
	MX53_PCBA_DSS_DATA3,
	MX53_PCBA_DSS_DATA4,
	MX53_PCBA_DSS_DATA5,
	MX53_PCBA_DSS_DATA6,
	MX53_PCBA_DSS_DATA7,
	MX53_PCBA_DSS_DATA8,
	MX53_PCBA_DSS_DATA9,
	MX53_PCBA_DSS_DATA10,
	MX53_PCBA_DSS_DATA11,
	MX53_PCBA_DSS_DATA12,
	MX53_PCBA_DSS_DATA13,
	MX53_PCBA_DSS_DATA14,
	MX53_PCBA_DSS_DATA15,
	MX53_PCBA_DSS_DATA16,
	MX53_PCBA_DSS_DATA17,
	MX53_PCBA_DSS_DATA18,
	MX53_PCBA_DSS_DATA19,
	MX53_PCBA_DSS_DATA20,
	MX53_PCBA_DSS_DATA21,
	MX53_PCBA_DSS_DATA22,
	MX53_PCBA_DSS_DATA23,


};
*/
static iomux_v3_cfg_t suspend_exit_pads[ARRAY_SIZE(suspend_enter_pads)];
static void pcba_suspend_enter()
{
	iomux_v3_cfg_t *p = suspend_enter_pads;
	int i, ret;
	printk("[FSL] Entry suspend.\n");
	for (i = 0; i < ARRAY_SIZE(suspend_enter_pads); i++) {
		suspend_exit_pads[i] = *p;
		*p &= ~MUX_PAD_CTRL_MASK;
		p++;
	}
	mxc_iomux_v3_get_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));
	mxc_iomux_v3_setup_multiple_pads(suspend_enter_pads,
			ARRAY_SIZE(suspend_enter_pads));

	/*Config some output pins to lowlevel*/
	gpio_direction_output(MX53_PCBA_SD_PWR_EN, 0);/*SD_PWR_EN*/
}

static void pcba_suspend_exit()
{
	int i,ret;
	printk("[FSL] Exit suspend.\n");
	mxc_iomux_v3_setup_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));
	gpio_direction_output(MX53_PCBA_SD_PWR_EN, 1);/*SD_PWR_EN*/

}

static struct mxc_pm_platform_data pcba_pm_data = {
	.suspend_enter = pcba_suspend_enter,
	.suspend_exit = pcba_suspend_exit,
};

	/* LED*/
static struct fb_videomode video_modes[] = {
	 /* TODO:add fbmode here */
	{"AT070TN93", 57, 800, 480, 37037, 
	 40, 60, 
	 10, 10, 
	 20, 10,
	 FB_SYNC_CLK_LAT_FALL,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 "1280x720M@60", 60, 1280, 720, 13468,
	 220, 110,
	 20, 5,
	 40, 5,
	 FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	 FB_VMODE_NONINTERLACED,
	 0,},
};

static struct platform_pwm_backlight_data mxc_pwm_backlight_data = {
	.pwm_id = 1,
	.max_brightness = 255,
	.dft_brightness = 128,
	.pwm_period_ns = 50000,
};

extern void mx5_ipu_reset(void);
static struct mxc_ipu_config mxc_ipu_data = {
	.rev = 3,
	.reset = mx5_ipu_reset,
};

extern void mx5_vpu_reset(void);
static struct mxc_vpu_platform_data mxc_vpu_data = {
	.iram_enable = true,
	.iram_size = 0x14000,
	.reset = mx5_vpu_reset,
};

static struct mxc_spi_master mxcspi1_data = {
	.maxchipselect = 4,
	.spi_version = 23,
	.chipselect_active = NULL,
	.chipselect_inactive = NULL,
};

static struct mxc_dvfs_platform_data dvfs_core_data = {
	.reg_id = "SW1A",
	.clk1_id = "cpu_clk",
	.clk2_id = "gpc_dvfs_clk",
	.gpc_cntr_offset = MXC_GPC_CNTR_OFFSET,
	.gpc_vcr_offset = MXC_GPC_VCR_OFFSET,
	.ccm_cdcr_offset = MXC_CCM_CDCR_OFFSET,
	.ccm_cacrr_offset = MXC_CCM_CACRR_OFFSET,
	.ccm_cdhipr_offset = MXC_CCM_CDHIPR_OFFSET,
	.prediv_mask = 0x1F800,
	.prediv_offset = 11,
	.prediv_val = 3,
	.div3ck_mask = 0xE0000000,
	.div3ck_offset = 29,
	.div3ck_val = 2,
	.emac_val = 0x08,
	.upthr_val = 25,
	.dnthr_val = 9,
	.pncthr_val = 33,
	.upcnt_val = 10,
	.dncnt_val = 10,
	.delay_time = 30,
};

static struct mxc_bus_freq_platform_data bus_freq_data = {
	.gp_reg_id = "SW1A",
	.lp_reg_id = "SW2",
};

static struct tve_platform_data tve_data = {
	.dac_reg = "",
};

static struct ldb_platform_data ldb_data = {
	.ext_ref = 1,
	.boot_enable = MXC_LDBDI0,
};

static void mxc_iim_enable_fuse(void)
{
	u32 reg;

	if (!ccm_base)
		return;

	/* enable fuse blown */
	reg = readl(ccm_base + 0x64);
	reg |= 0x10;
	writel(reg, ccm_base + 0x64);
}

static void mxc_iim_disable_fuse(void)
{
	u32 reg;

	if (!ccm_base)
		return;
	/* enable fuse blown */
	reg = readl(ccm_base + 0x64);
	reg &= ~0x10;
	writel(reg, ccm_base + 0x64);
}

static struct mxc_iim_data iim_data = {
	.bank_start = MXC_IIM_MX53_BANK_START_ADDR,
	.bank_end   = MXC_IIM_MX53_BANK_END_ADDR,
	.enable_fuse = mxc_iim_enable_fuse,
	.disable_fuse = mxc_iim_disable_fuse,
};

static struct resource mxcfb_resources[] = {
	[0] = {
	       .flags = IORESOURCE_MEM,
	       },
};

static struct mxc_fb_platform_data fb_data[] = {
	{
	 .interface_pix_fmt = IPU_PIX_FMT_RGB24,
	 .mode_str = "AT070TN93",
	 .mode = video_modes,
	 .num_modes = ARRAY_SIZE(video_modes),
	},
	{
	 .interface_pix_fmt = IPU_PIX_FMT_RGB24,
	 .mode_str = "1280x720M-24@60",
	 .mode = video_modes,
	 .num_modes = ARRAY_SIZE(video_modes),
	 },
};

extern int primary_di;
static int __init mxc_init_fb(void)
{
	if (!machine_is_mx53_pcba())
		return 0;

	/*for pcba board, set default display as LDB*/
	if (primary_di < 0)
		primary_di = 0;

	if (primary_di) {
		printk(KERN_INFO "DI1 is primary\n");
		/* DI1 -> DP-BG channel: */
		mxc_fb_devices[1].num_resources = ARRAY_SIZE(mxcfb_resources);
		mxc_fb_devices[1].resource = mxcfb_resources;
		mxc_register_device(&mxc_fb_devices[1], &fb_data[1]);

		/* DI0 -> DC channel: */
		mxc_register_device(&mxc_fb_devices[0], &fb_data[0]);
	} else {
		printk(KERN_INFO "DI0 is primary\n");

		/* DI0 -> DP-BG channel: */
		mxc_fb_devices[0].num_resources = ARRAY_SIZE(mxcfb_resources);
		mxc_fb_devices[0].resource = mxcfb_resources;
		mxc_register_device(&mxc_fb_devices[0], &fb_data[0]);

		/* DI1 -> DC channel: */
		mxc_register_device(&mxc_fb_devices[1], &fb_data[1]);
	}

	/*
	 * DI0/1 DP-FG channel:
	 */
	mxc_register_device(&mxc_fb_devices[2], NULL);

	return 0;
}
device_initcall(mxc_init_fb);

static struct imxi2c_platform_data mxci2c_data = {
       .bitrate = 100000,
};

void camera1_suspend(void )
{
		gpio_set_value(MX53_PCBA_CAM1_PWR_DOWN, 1);
		gpio_direction_input(MX53_PCBA_CAMERA1_RST);
		gpio_direction_input(MX53_PCBA_CAM1_PWDN_VCM);
}

void camera1_resume(void )
{
		gpio_direction_output(MX53_PCBA_CAMERA1_RST, 1);
		gpio_set_value(MX53_PCBA_CAM1_PWR_DOWN, 0);
		gpio_set_value(MX53_PCBA_CAM1_PWDN_VCM, 0);
}

void camera2_suspend(void )
{
		gpio_set_value(MX53_PCBA_CAM2_PWR_DOWN, 1);
		gpio_direction_input(MX53_PCBA_CAM2_RESET);
}

void camera2_resume(void )
{
		gpio_direction_output(MX53_PCBA_CAM2_RESET, 1);
		gpio_set_value(MX53_PCBA_CAM2_PWR_DOWN, 0);
}

void camera1_powerdown(int down)
{
	if (down)
		gpio_set_value(MX53_PCBA_CAM1_PWR_DOWN, 1);
	else
		gpio_set_value(MX53_PCBA_CAM1_PWR_DOWN, 0);
}

void camera2_powerdown(int down)
{
	if (down)
		gpio_set_value(MX53_PCBA_CAM2_PWR_DOWN, 1);
	else
		gpio_set_value(MX53_PCBA_CAM2_PWR_DOWN, 0);
}

static struct mxc_camera_platform_data camera_data1 = {
	.analog_regulator = "",
	.core_regulator = "",
	.mclk = 24000000,
	.csi = 0,
	.pwdn = camera1_powerdown,
	.suspend = camera1_suspend,
	.resume = camera1_resume,
};

static struct mxc_camera_platform_data camera_data2 = {
	.analog_regulator = "",
	.core_regulator = "",
	.mclk = 24000000,
	.csi = 0,
	.pwdn = camera2_powerdown,
	.suspend = camera2_suspend,
	.resume = camera2_resume,
};

static struct wm8994_pdata wm8958_pdata = {
     .gpio_defaults =
     {
         [0] = WM8994_GPN_PD |WM8994_GPN_DB|WM8994_GP_FN_FLL1_OUT,
         [1] = WM8994_GPN_DIR|WM8994_GPN_DB|WM8994_GP_FN_GPIO,
         [2] = WM8994_GPN_DIR|WM8994_GPN_DB|WM8994_GP_FN_GPIO,
         [3] = WM8994_GPN_DIR|WM8994_GPN_DB|WM8994_GP_FN_GPIO,
         [4] = WM8994_GPN_DIR|WM8994_GPN_DB|WM8994_GP_FN_GPIO,
         [5] = WM8994_GP_FN_MICBIAS1_DET,
         [7] = WM8994_GPN_DB,
         [8] = WM8994_GPN_DIR|WM8994_GPN_DB,
         [9] = WM8994_GPN_DB,
         [10]= WM8994_GPN_DB,
     },
     .micdet_irq = gpio_to_irq(MX53_PCBA_AUD_REQ),
};


static int p1003_ts_hw_status(void)
{
	return gpio_get_value(MX53_PCBA_TOUCH_INT);
}

static struct p1003_ts_platform_data p1003_ts_data = {
	.hw_status = p1003_ts_hw_status,
};

static struct led_mc34708 leds_mc34708[] = {
	{
	 .name = "keypad",
	 .pwm_id = 0,
	 .max_brightness = LED_FULL,
	 .pwm_period_ns = 20000,
	},
	{
	 .name = "vibrator",
	 .pwm_id = 1,
	 .max_brightness = LED_FULL,
	 .pwm_period_ns = 20000,
	},
};

static struct led_mc34708_platform_data leds_mc34708_data = {
	.num_leds = ARRAY_SIZE(leds_mc34708),
	.leds = leds_mc34708,
};

static struct l3g4200d_gyr_platform_data l3g4200d_gyr_data = {
	.poll_interval = 50,
	.min_interval = 10,

	.fs_range = L3G4200D_GYR_FS_250DPS,

	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,

	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
};

static struct akm8975_platform_data akm_platform_data_8975 = {
	.gpio_DRDY = MX53_PCBA_COMPASS_INT, /*GYRO_DRDY*/
};

static struct goodix_i2c_platform_data goodix_data = {
	.gpio_irq = MX53_PCBA_TOUCH_INT,
	.gpio_shutdown = MX53_PCBA_TOUCH_RST,
};
/*
 * NOTE: Here exist 2 possible declarations, one for Huawei new IO board,
 *       the other is for Huawei old IO board.
 */
static u16 pcba_touchkey_martix[] = {
	#if defined(CONFIG_AT070TN93)
	KEY_MENU, KEY_HOME,KEY_BACK, KEY_SEARCH,
	#endif
	#if defined(CONFIG_AT070TN2_WSVGA)
	KEY_HOME, KEY_BACK, KEY_MENU, KEY_SEARCH,
	#endif
};

static struct ha2605_platform_data ha2605_keyboard_platdata = {
	.keycount = ARRAY_SIZE(pcba_touchkey_martix),
	.matrix = pcba_touchkey_martix,
};

static struct i2c_board_info mxc_i2c1_board_info[] __initdata = {
	{
	.type = "mma8451",
	.addr = 0x1C, /* gsensor, according to mma8451 driver, it seems no interrupt */
	 },
};

static struct i2c_board_info mxc_i2c2_board_info[] __initdata = {
	{
	.type = "p1003_fwv33",
	.addr = 0x41,
	.irq = gpio_to_irq(MX53_PCBA_TOUCH_INT),
	.platform_data = &p1003_ts_data,
	},
	{
	.type = "egalax_ts",
	.addr = 0x4,
	.irq = gpio_to_irq(MX53_PCBA_TOUCH_INT),
	},
	{
	.type = "Goodix-TS",
	.addr = 0x55,
	.irq = gpio_to_irq(MX53_PCBA_TOUCH_INT),
	.platform_data = &goodix_data,
	},
	{
	.type = "ha2605_touchkey",
	.addr = 0x62,
	.irq = gpio_to_irq(MX53_PCBA_TK_INT),
	.platform_data = &ha2605_keyboard_platdata,
	},
};

static struct i2c_board_info mxc_i2c3_board_info[] __initdata = {
	{
	.type = "mt9v114",
	.addr = 0x3d,
	.platform_data = (void *)&camera_data2,
	 },
	{
	.type = "mt9p111",
	.addr = 0x3c,
	.platform_data = (void *)&camera_data1,
	 },
	{
	I2C_BOARD_INFO("akm8975", 0x0E),
	.flags = I2C_CLIENT_WAKE,
	.platform_data = &akm_platform_data_8975,
	.irq = gpio_to_irq(MX53_PCBA_COMPASS_INT),/* E-Compass INT */
	},
	{
	.type = "l3g4200d_gyr",
	.addr = 0x68,
	.platform_data = &l3g4200d_gyr_data,
	.irq = gpio_to_irq(MX53_PCBA_GYRO_INT), /* GYRO_INT */
	},
	{
	.type = "wm8958",
	.addr = 0x1A, /* codec */
	.platform_data = &wm8958_pdata,
	 },
};


#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#define GPIO_BUTTON(gpio_num, ev_code, act_low, descr, wake)	\
{								\
	.gpio		= gpio_num,				\
	.type		= EV_KEY,				\
	.code		= ev_code,				\
	.active_low	= act_low,				\
	.desc		= "btn " descr,				\
	.wakeup		= wake,					\
}

static struct gpio_keys_button pcba_buttons[] = {
	GPIO_BUTTON(MX53_PCBA_KEY_VOL_UP, KEY_VOLUMEUP, 1, "volume-up", 0),
	GPIO_BUTTON(MX53_PCBA_KEY_VOL_DOWN, KEY_VOLUMEDOWN, 1, "volume-down", 0),
};

static struct gpio_keys_platform_data pcba_button_data = {
	.buttons	= pcba_buttons,
	.nbuttons	= ARRAY_SIZE(pcba_buttons),
};

static struct platform_device pcba_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources  = 0,
	.dev		= {
		.platform_data = &pcba_button_data,
	}
};

static void __init pcba_add_device_buttons(void)
{
	platform_device_register(&pcba_button_device);
}
#else
static void __init pcba_add_device_buttons(void) {}
#endif

static void my_mdelay(int ms)
{
	unsigned long timeout;
	timeout = jiffies + msecs_to_jiffies(ms);
	/* Wait for jiffies timeout  */
	while (1) {
		if (time_after(jiffies, timeout)) {
			break;
		}
		msleep(1);
	}
}

static void mx53_gpio_usbotg_driver_vbus(bool on)
{
	pmic_write_reg(REG_USB_TIMING, BITFVAL(USBtiming, 0x0), BITFMASK(USBtiming));
	if (on) {
			pmic_write_reg(REG_BATTERY_PROFILE, BITFVAL(CHREN, 0), BITFMASK(CHREN));
			pmic_write_reg(REG_USB_CTL, BITFVAL(ManualSW, 0) | BITFVAL(SWHOLD, 0) \
				| BITFVAL( DPSWITCHING, 2) | BITFVAL(DMSWITCHING, 2),BITFMASK(ManualSW)\
				| BITFMASK(SWHOLD)| BITFMASK(DPSWITCHING) | BITFMASK(DMSWITCHING));
			gpio_set_value(MX53_PCBA_USB_OTG_PWR_EN, 1);
			my_mdelay(800);
			pmic_write_reg(REG_USB_CTL, BITFVAL(ManualSW, 0) | BITFVAL(SWHOLD, 0) \
				| BITFVAL( DPSWITCHING, 1) | BITFVAL(DMSWITCHING, 1),BITFMASK(ManualSW)\
				| BITFMASK(SWHOLD) | BITFMASK(DPSWITCHING) | BITFMASK(DMSWITCHING));
	} else {
			pmic_write_reg(REG_USB_CTL, BITFVAL(ManualSW, 1), BITFMASK(ManualSW));
			gpio_set_value(MX53_PCBA_USB_OTG_PWR_EN, 0);
	}
}

static int sdhc_write_protect(struct device *dev)
{
	return 0;
}

static unsigned int sdhc_get_card_det_status(struct device *dev)
{
	int ret = 0;

	if (to_platform_device(dev)->id == 0)
		ret = gpio_get_value(MX53_PCBA_SD1_CD);

	return ret;
}
bool bcm4329_power_bt_on = false;
bool bcm4329_power_wifi_on = false;
static unsigned int sdhc_get_sdio_det_status(struct device *dev)
{
	return bcm4329_power_wifi_on ? 0 : 1;
}
static struct mxc_mmc_platform_data mmc1_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
	.power_mmc = NULL,
};

static struct mxc_mmc_platform_data mmc2_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 1,
	.status = sdhc_get_sdio_det_status,
	.clock_mmc = "esdhc_clk",
	.power_mmc = NULL,
};

static struct mxc_mmc_platform_data mmc3_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 1,
	.clock_mmc = "esdhc_clk",
};


/* TODO: add wm8958 codec paltform here */
static int mxc_wm8958_amp_enable(int enable)
{
	return 0;
}

static int headphone_det_status(void)
{
	return (gpio_get_value(MX53_PCBA_HEADSET_DECT) == 1);
}
static struct mxc_audio_platform_data wm8958_data;

static int mxc_wm8958_init(void)
{
	wm8958_data.sysclk = 25000000;	// Changed due to switch clock source. For axi_b_clk, it is 25000000; for osc_clk, it is 24000000.
	return 0;
}

static int mxc_wm8958_gpio_suspend(void)
{
	gpio_direction_output(MX53_PCBA_AU_LDO_EN, 0);
	return 0;
}

static int mxc_wm8958_gpio_resume(void)
{
	gpio_direction_output(MX53_PCBA_AU_LDO_EN, 1);
	return 0;
}

static struct mxc_audio_platform_data wm8958_data = {
	.ssi_num = 1,
	.src_port = 2,
	.ext_port = 3,
	.hp_irq     = gpio_to_irq(MX53_PCBA_HEADSET_DECT),
	.hp_status  = headphone_det_status,
	.amp_enable = mxc_wm8958_amp_enable,
	.init       = mxc_wm8958_init,
	.ext_ram_rx = 1,
	.ext_ram_tx = 0,
	.gpio_suspend = mxc_wm8958_gpio_suspend,
	.gpio_resume = mxc_wm8958_gpio_resume,
};

static struct platform_device mxc_wm8958_device = {
	.name = "imx-3stack-wm8994",
};

static struct android_pmem_platform_data android_pmem_data = {
	.name = "pmem_adsp",
	.size = SZ_32M,
};

static struct android_pmem_platform_data android_pmem_gpu_data = {
	.name = "pmem_gpu",
	.size = SZ_64M,
	.cached = 1,
};

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_all[] = {
	"rndis",
	"usb_mass_storage",
	"adb"
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x0c01,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= 0x0c02,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	{
		.product_id	= 0x0c10,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= 0x0c02,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
};

static struct usb_mass_storage_platform_data mass_storage_data = {
	.nluns		= 3,
	.vendor		= "Freescale",
	.product	= "MX53 PCBA Android",
	.release	= 0x0100,
};

static struct usb_ether_platform_data rndis_data = {
	.vendorID	= 0x15a2,
	.vendorDescr	= "Freescale",
};

static struct android_usb_platform_data android_usb_data = {
	.vendor_id      = 0x15a2,
	.product_id     = 0x0c01,
	.version        = 0x0100,
	.product_name   = "MX53 PCBA Android",
	.manufacturer_name = "Freescale",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct mxc_asrc_platform_data mxc_asrc_data = {
	.channel_bits = 4,
	.clk_map_ver = 2,
};

static struct mxc_spdif_platform_data mxc_spdif_data = {
	.spdif_tx = 1,
	.spdif_rx = 0,
	.spdif_clk_44100 = 0,	/* Souce from CKIH1 for 44.1K */
	/* Source from CCM spdif_clk (24M) for 48k and 32k
	 * It's not accurate
	 */
	.spdif_clk_48000 = 1,
	.spdif_clkid = 0,
	.spdif_clk = NULL,	/* spdif bus clk */
};

static struct mxc_audio_platform_data spdif_audio_data = {
	.ext_ram_tx = 1,
};

static struct platform_device mxc_spdif_audio_device = {
	.name = "imx-spdif-audio-device",
};

int mx53_pcba_bt_power_change(int status)
{
	if (1 == status)
	{		
        bcm4329_power_bt_on = true;
		gpio_request(MX53_PCBA_WLAN_VCC_EN, "wl-vcc-enable");
		gpio_direction_output(MX53_PCBA_WLAN_VCC_EN, status);
		gpio_free(MX53_PCBA_WLAN_VCC_EN);
		msleep(100);
		printk(KERN_INFO"mx53_pcba bt power on\r\n");
		gpio_request(MX53_PCBA_BT_ENABLE, "bt-reset");
		gpio_direction_output(MX53_PCBA_BT_ENABLE, status);
		gpio_free(MX53_PCBA_BT_ENABLE);
	}else if (0 == status)
	{
		bcm4329_power_bt_on = false;
		gpio_request(MX53_PCBA_BT_ENABLE, "bt-reset");
		gpio_direction_output(MX53_PCBA_BT_ENABLE, status);
		gpio_free(MX53_PCBA_BT_ENABLE);
		if (!bcm4329_power_wifi_on)
		{
			printk(KERN_INFO"mx53_pcba bt power off\r\n");
			gpio_request(MX53_PCBA_WLAN_VCC_EN, "wl-vcc-enable");
			gpio_direction_output(MX53_PCBA_WLAN_VCC_EN, status);
			gpio_free(MX53_PCBA_WLAN_VCC_EN);
		}
	}

	return 0;
}
int mx53_pcba_wifi_set_power(int val)
{
	if (1 == val)
	{
		bcm4329_power_wifi_on = true;
		gpio_request(MX53_PCBA_WLAN_VCC_EN, "wl-vcc-enable");
		gpio_direction_output(MX53_PCBA_WLAN_VCC_EN, val);
		gpio_free(MX53_PCBA_WLAN_VCC_EN);
		msleep(100);
		printk(KERN_INFO"mx53_pcba wifi power on\r\n");
		gpio_request(MX53_PCBA_WLAN_ENABLE, "wl-enable");
		gpio_direction_output(MX53_PCBA_WLAN_ENABLE, val);
		gpio_free(MX53_PCBA_WLAN_ENABLE);
	}else if (0 == val)
	{
		bcm4329_power_wifi_on = false;
#if 0
		//workaround system hang up when playback media files by MHL.
		gpio_request(MX53_PCBA_WLAN_ENABLE, "wl-enable");
		gpio_direction_output(MX53_PCBA_WLAN_ENABLE, val);
		gpio_free(MX53_PCBA_WLAN_ENABLE);
#endif
		msleep(100);
		if (!bcm4329_power_bt_on)
		{
			printk(KERN_INFO"mx53_pcba wifi power off\r\n");
			gpio_request(MX53_PCBA_WLAN_VCC_EN, "wl-vcc-enable");
			gpio_direction_output(MX53_PCBA_WLAN_VCC_EN, val);
			gpio_free(MX53_PCBA_WLAN_VCC_EN);
		}
	}
	return 0;
}
static int mx53_pcba_wifi_reset(int val)
{
	mx53_pcba_wifi_set_power(1-val);
	mdelay(6);
	mx53_pcba_wifi_set_power(val);
	return 0;
}
static int mx53_pcba_wifi_set_carddetect(int val)
{
        int wifi_bus_num = 1;
        printk(KERN_INFO"mx53 pcba enter %s=%d\r\n", __FUNCTION__, val);
        if (val != bcm4329_power_wifi_on)
	{
	    printk(KERN_INFO"pcba no need to detect and exit\r\n");
	    return 0;
        }
        mxc_mmc_force_detect(wifi_bus_num);
        mdelay(500);
        return 0;
}

static void * mx53_pcba_wifi_mem_prealloc(int section, unsigned long size)
{
	return 0;
}
static int mx53_pcba_wifi_get_mac_addr(unsigned char *buf)
{
	return 0;
}
static void * mx53_pcba_wifi_get_country_code(char *ccode)
{
	return 0;
}

static struct platform_device mxc_bt_rfkill = {
	.name = "mxc_bt_rfkill",
};

static struct mxc_bt_rfkill_platform_data mxc_bt_rfkill_data = {
	.power_change = mx53_pcba_bt_power_change,
};
static struct platform_device mxc_wifi_control = {
        .name = "bcm4329_wlan",
};

static struct wifi_platform_data wifi_control_data = {
        .set_power = mx53_pcba_wifi_set_power,
        .set_reset =  mx53_pcba_wifi_reset,
        .set_carddetect = mx53_pcba_wifi_set_carddetect,
        .mem_prealloc ="",// &mx53_pcba_wifi_mem_prealloc,
        .get_mac_addr = mx53_pcba_wifi_get_mac_addr,
        .get_country_code = mx53_pcba_wifi_get_country_code,
};
static void mxc_register_powerkey(pwrkey_callback pk_cb)
{
	pmic_event_callback_t power_key_event;

	power_key_event.param = (void *)1;
	power_key_event.func = (void *)pk_cb;
	pmic_event_subscribe(EVENT_PWRONI, power_key_event);

	power_key_event.param = (void *)3;
	pmic_event_subscribe(EVENT_PWRON3I, power_key_event);
}

static int mxc_pwrkey_getstatus(int id)
{
	int sense, off = 3;

	pmic_read_reg(REG_INT_SENSE1, &sense, 0xffffffff);
	switch (id) {
	case 2:
		off = 4;
		break;
	case 3:
		off = 2;
		break;
	}

	if (sense & (1 << off))
		return 0;

	return 1;
}

void huawei_mu509_poweron(void)
{
	gpio_set_value(MX53_PCBA_MODEM_PWR_ON,0);
	mdelay(330);
	gpio_set_value(MX53_PCBA_MODEM_PWR_ON,1);
}

void huawei_mu509_poweroff(void)
{
	gpio_set_value(MX53_PCBA_MODEM_PWR_ON,0);
	mdelay(3500);
	gpio_set_value(MX53_PCBA_MODEM_PWR_ON,1);
}

static struct power_key_platform_data pwrkey_data = {
	.key_value = KEY_F4,
	.register_pwrkey = mxc_register_powerkey,
	.get_key_status = mxc_pwrkey_getstatus,
};

static void __init mx53_pcba_io_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(mx53_pcba_pads,
					ARRAY_SIZE(mx53_pcba_pads));
	/* Misc */
	gpio_request(MX53_PCBA_POWER_ON_1V8_PE, "1v8-enable");
	gpio_direction_output(MX53_PCBA_POWER_ON_1V8_PE, 0);
	
	/* SD1 CD */
	gpio_request(MX53_PCBA_SD1_CD, "sd1-cd");
	gpio_direction_input(MX53_PCBA_SD1_CD);

	/* SD/eMMC power */
	gpio_request(MX53_PCBA_SD_PWR_EN, "sd-pwr");
	gpio_direction_output(MX53_PCBA_SD_PWR_EN, 1);

	/* Headset detect */
	gpio_request(MX53_PCBA_HEADSET_DECT, "headset-dect");
	gpio_direction_input(MX53_PCBA_HEADSET_DECT);

	/* USB PWR enable */
	gpio_request(MX53_PCBA_USB_OTG_PWR_EN, "usb-pwr");
	gpio_direction_output(MX53_PCBA_USB_OTG_PWR_EN, 0);
	/* USB PWR fault */
	gpio_request(MX53_PCBA_OTG_PWR_FAULT, "usb-pwr-fault");
	gpio_direction_input(MX53_PCBA_OTG_PWR_FAULT);

	/* Sii9232 MHL controller, no power-up sequence requirements from  */
	gpio_request(MX53_PCBA_MHL_3V3_ON, "mhl-3v3-pwr-on");
	gpio_direction_output(MX53_PCBA_MHL_3V3_ON, 0);
	gpio_request(MX53_PCBA_MHL_1V3_ON, "mhl-1v3-pwr-on");
	gpio_direction_output(MX53_PCBA_MHL_1V3_ON, 0);

	msleep(10);
	gpio_request(MX53_PCBA_MHL_RST_N, "mhl-reset");
	gpio_direction_output(MX53_PCBA_MHL_RST_N, 1);
	msleep(5);
	gpio_direction_output(MX53_PCBA_MHL_RST_N, 0);
	msleep(5);
	gpio_direction_output(MX53_PCBA_MHL_RST_N, 1);
	gpio_request(MX53_PCBA_MHL_INT, "mhl-intr");
	gpio_direction_input(MX53_PCBA_MHL_INT);
	gpio_request(MX53_PCBA_MHL_WAKE, "mhl-wakeup");
	gpio_direction_output(MX53_PCBA_MHL_WAKE, 1);

	gpio_request(MX53_PCBA_MHL_SW_I2C_SCL, "mhl-sw-i2c-scl");
	gpio_direction_output(MX53_PCBA_MHL_SW_I2C_SCL, 1);	// Because this GPIO pin is set to be open-drain mode in IOMUX config
	gpio_request(MX53_PCBA_MHL_SW_I2C_SDA, "mhl-sw-i2c-sda");
	gpio_direction_output(MX53_PCBA_MHL_SW_I2C_SDA, 1);	// Because this GPIO pin is set to be open-drain mode in IOMUX config


	/* LCD power enable */
	gpio_request(MX53_PCBA_LCD_PWR_EN, "lcd-pwr-en");
	gpio_direction_output(MX53_PCBA_LCD_PWR_EN, 1);

	#if defined(CONFIG_AT070TN93)
	/* backlight power */
	gpio_request(MX53_PCBA_BL_PWR_EN, "bl-pwr-en");
	gpio_direction_output(MX53_PCBA_BL_PWR_EN, 1);
	#endif

	#if defined(CONFIG_AT070TN2_WSVGA)
	/* LCD 6/8bits select, 1-6bit, 0-8bit */
	/* 8 bits mode */
	/* LVDS backlight power */
	gpio_request(MX53_PCBA_LCD_GPIO0, "lcd-gpio0-en");
	gpio_direction_output(MX53_PCBA_LCD_GPIO0, 1);		// Work in input mode for power saving
	gpio_request(MX53_PCBA_LCD_GPIO1, "lcd-gpio1-en");	// Work in input mode for power saving
	gpio_direction_input(MX53_PCBA_LCD_GPIO1);
	#endif

	gpio_request(MX53_PCBA_LCD_RESET, "lcd-id");
	gpio_direction_output(MX53_PCBA_LCD_RESET, 0);
	msleep(100);
	gpio_direction_output(MX53_PCBA_LCD_RESET, 1);	// Recover from LCD RESET
	/* LCD misc */
	gpio_request(MX53_PCBA_LCD_UD, "lcd-ud");
	gpio_direction_output(MX53_PCBA_LCD_UD, 0);
	gpio_request(MX53_PCBA_LCD_LR, "lcd-lr");
	gpio_direction_output(MX53_PCBA_LCD_LR, 1);
	/*
	 * Due to not used for LVDS panel, so set it to be input mode for power saving.
	 */
#if defined(CONFIG_AT070TN2_WSVGA)
	gpio_request(MX53_PCBA_LCD_CABC_EN1, "lcd-cabc-en1");
	gpio_direction_input(MX53_PCBA_LCD_CABC_EN1);
	gpio_request(MX53_PCBA_LCD_CABC_EN2, "lcd-cabc-en2");
	gpio_direction_input(MX53_PCBA_LCD_CABC_EN2);
	gpio_request(MX53_PCBA_LCD_SEL, "lcd_sel");
	gpio_direction_input(MX53_PCBA_LCD_SEL);
#endif
	#if defined(CONFIG_AT070TN93)
	/* LCD normal mode */
	gpio_request(MX53_PCBA_LCD_MODE, "lcd_mode");
	gpio_direction_output(MX53_PCBA_LCD_MODE, 1);
	#endif

	#if defined(CONFIG_AT070TN2_WSVGA)
	/* LCD 6/8bits select, 1-6bit, 0-8bit */
	/* 8 bits mode */
	gpio_request(MX53_PCBA_LCD_SEL, "lcd_sel");
	gpio_direction_output(MX53_PCBA_LCD_SEL, 0);
	#endif

	/* ecompass sensor intr */
	gpio_request(MX53_PCBA_COMPASS_INT, "ecompass int");
	gpio_direction_input(MX53_PCBA_COMPASS_INT);

	/* BT wakeup pin and host wake up */
	gpio_request(MX53_PCBA_BT_WAKE, "bt-wake");
	gpio_direction_output(MX53_PCBA_BT_WAKE, 0);
	gpio_request(MX53_PCBA_BT_HOST_WAKE, "bt-host-wake");
	gpio_direction_input(MX53_PCBA_BT_HOST_WAKE);

	/* WLAN clk req, enable, wake, host wake */
	gpio_request(MX53_PCBA_WLAN_VCC_EN, "wl-vcc-enable");
	gpio_direction_output(MX53_PCBA_WLAN_VCC_EN, 0);
	gpio_request(MX53_PCBA_WLAN_ENABLE, "wl-enable");
#if 0
	//workaround system hang up when playback media files by MHL.
	gpio_direction_output(MX53_PCBA_WLAN_ENABLE, 0);
#else
	gpio_direction_output(MX53_PCBA_WLAN_ENABLE, 1);
#endif
	gpio_request(MX53_PCBA_WLAN_WAKE_B, "wl-wake");
	gpio_direction_output(MX53_PCBA_WLAN_WAKE_B, 0);
	gpio_request(MX53_PCBA_WLAN_HOST_WAKE_B, "wl-host-wake");
	gpio_direction_input(MX53_PCBA_WLAN_HOST_WAKE_B);

	/* Touch related */
	gpio_request(MX53_PCBA_TOUCH_INT, "touch-att");
	gpio_direction_input(MX53_PCBA_TOUCH_INT);
	gpio_request(MX53_PCBA_TOUCH_RST, "touch-rst");
	gpio_direction_output(MX53_PCBA_TOUCH_RST, 1);
	/* Touch Key */
	gpio_request(MX53_PCBA_TK_INT, "touch-key");
	gpio_direction_input(MX53_PCBA_TK_INT);

	/* Gsensor intr */
	gpio_request(MX53_PCBA_GSENSOR_INT1, "gsensor-int1");
	gpio_direction_input(MX53_PCBA_GSENSOR_INT1);
	gpio_request(MX53_PCBA_GSENSOR_INT2, "gsensor-int1");
	gpio_direction_input(MX53_PCBA_GSENSOR_INT2);

	/* Camera 1 & 2 power down set for power turn on, leave reset in driver */
	gpio_request(MX53_PCBA_CAM1_PWR_DOWN, "cam1-pwdn");
	gpio_direction_output(MX53_PCBA_CAM1_PWR_DOWN, 0);
	gpio_request(MX53_PCBA_CAM2_PWR_DOWN, "cam2-pwdn");
	gpio_direction_output(MX53_PCBA_CAM2_PWR_DOWN, 0);
	gpio_request(MX53_PCBA_CAM1_PWDN_VCM, "cam1-pwdn-vcm");
	gpio_direction_output(MX53_PCBA_CAM1_PWDN_VCM, 1);

	/* Camera sensor id pin */
	gpio_request(MX53_PCBA_CAM1_ID0, "cam1-id0");
	gpio_direction_input(MX53_PCBA_CAM1_ID0);
	gpio_request(MX53_PCBA_CAM2_ID0, "cam2-id0");
	gpio_direction_input(MX53_PCBA_CAM2_ID0);
	/* Camera 2 reset */
	gpio_request(MX53_PCBA_CAM2_RESET, "cam2-reset");
	gpio_direction_output(MX53_PCBA_CAM2_RESET, 0);
	/* Camera1 power down */
	gpio_request(MX53_PCBA_CAM1_PWR_DOWN, "cam1-pwdn");
	gpio_direction_output(MX53_PCBA_CAM1_PWR_DOWN, 0);
	/* Camera reset */
	msleep(3);
	gpio_request(MX53_PCBA_CAMERA1_RST, "cam-reset");
	gpio_direction_output(MX53_PCBA_CAMERA1_RST, 1);
	msleep(2);
	gpio_set_value(MX53_PCBA_CAMERA1_RST, 0);
	msleep(2);
	gpio_set_value(MX53_PCBA_CAMERA1_RST, 1);
	msleep(2);
	/* Camera2 power down */
	gpio_request(MX53_PCBA_CAM2_PWR_DOWN, "cam2-pwdn");
	gpio_direction_output(MX53_PCBA_CAM2_PWR_DOWN, 0);
	/* flash led */
	gpio_request(MX53_PCBA_KEY_FLASH_LED3, "cam-flash");
	gpio_direction_output(MX53_PCBA_KEY_FLASH_LED3, 0);

	/* Gryo ready and intr */
	gpio_request(MX53_PCBA_GYRO_DRDY, "gyro-ready");
	gpio_direction_input(MX53_PCBA_GYRO_DRDY);
	gpio_request(MX53_PCBA_GYRO_INT, "gyro-int");
	gpio_direction_input(MX53_PCBA_GYRO_INT);

	/* make sure the TCXO_PWR_EN is set as high all the time by h/w connection */

    /* GPS onoff, tsync, modified according to CSR request 11/25/2011 */
	// gpio_request(MX53_PCBA_GPS_1V8_ON, "gps-1v8-on");
	// gpio_direction_output(MX53_PCBA_GPS_1V8_ON, 1);
    // gpio_request(MX53_PCBA_GPS_ONOFF, "gps-onoff");
    // gpio_direction_output(MX53_PCBA_GPS_ONOFF, 0);
    // gpio_request(MX53_PCBA_GPS_TSYNC, "gps-tsync");
    // gpio_direction_output(MX53_PCBA_GPS_TSYNC, 0);
    // gpio_request(MX53_PCBA_GPS_RESET, "gps-rst");
    // gpio_direction_output(MX53_PCBA_GPS_RESET, 0);
		
	/* MODEM wakeup and power */
	gpio_request(MX53_PCBA_MODEM_WAKEUP_IN, "modem-wakeup-in");
	gpio_direction_output(MX53_PCBA_MODEM_WAKEUP_IN, 0);
	gpio_request(MX53_PCBA_MODEM_WAKEUP_OUT, "modem-wakeup-out");
	gpio_direction_input(MX53_PCBA_MODEM_WAKEUP_OUT);
	gpio_request(MX53_PCBA_MODEM_PWR_ON, "modem-pwr");
	gpio_direction_output(MX53_PCBA_MODEM_PWR_ON, 0);
	gpio_request(MX53_PCBA_USIM_CDT, "sim-detect");
	gpio_direction_input(MX53_PCBA_USIM_CDT);
	
	/* AU LDO enable */
	gpio_request(MX53_PCBA_AU_LDO_EN, "au-leo-en");
	gpio_direction_output(MX53_PCBA_AU_LDO_EN, 1);

	/* audio codec REQ */
	gpio_request(MX53_PCBA_AUD_REQ, "aud-req");
	gpio_direction_input(MX53_PCBA_AUD_REQ);

	/* charging */
	gpio_request(MX53_PCBA_BTCFG10, "extra-charging");
	gpio_direction_output(MX53_PCBA_BTCFG10, 1);
	gpio_export(MX53_PCBA_BTCFG10, true);
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	mxc_ipu_data.di_clk[0] = clk_get(NULL, "ipu_di0_clk");
	mxc_ipu_data.di_clk[1] = clk_get(NULL, "ipu_di1_clk");
	mxc_ipu_data.csi_clk[0] = clk_get(NULL, "ssi_ext1_clk");
	mxc_spdif_data.spdif_core_clk = clk_get(NULL, "spdif_xtal_clk");
	clk_put(mxc_spdif_data.spdif_core_clk);

	mxcsdhc1_device.resource[2].start = gpio_to_irq(MX53_PCBA_SD1_CD);
	mxcsdhc1_device.resource[2].end = gpio_to_irq(MX53_PCBA_SD1_CD);

	mxc_cpu_common_init();
	mx53_pcba_io_init();

	/* power off by sending shutdown command to da9053*/
	mxc_register_device(&mxc_dma_device, NULL);
	mxc_register_device(&mxc_wdt_device, NULL);
	mxc_register_device(&mxcspi1_device, &mxcspi1_data);
	mxc_register_device(&mxci2c_devices[0], &mxci2c_data);
	mxc_register_device(&mxci2c_devices[1], &mxci2c_data);
	mxc_register_device(&mxci2c_devices[2], &mxci2c_data);
	mx53_pcba_init_mc34708();

	mxc_register_device(&mxc_rtc_device, NULL);
	mxc_register_device(&mxc_ipu_device, &mxc_ipu_data);
	mxc_register_device(&mxc_ldb_device, &ldb_data);
	mxc_register_device(&mxc_tve_device, &tve_data);
	mxc_register_device(&mxcvpu_device, &mxc_vpu_data);
	mxc_register_device(&gpu_device, &gpu_data);
	mxc_register_device(&mxcscc_device, NULL);
	mxc_register_device(&pm_device, &pcba_pm_data);/*add pcab_pm device*/
	mxc_register_device(&mxc_dvfs_core_device, &dvfs_core_data);
	mxc_register_device(&busfreq_device, &bus_freq_data);
	mxc_register_device(&mxc_iim_device, &iim_data);
	mxc_register_device(&mxc_pwm2_device, NULL);
	mxc_register_device(&mxc_pwm1_backlight_device,
			&mxc_pwm_backlight_data);
	mxc_register_device(&mxcsdhc3_device, &mmc3_data);
	mxc_register_device(&mxcsdhc1_device, &mmc1_data);
	mxc_register_device(&mxcsdhc2_device, &mmc2_data);
	mxc_register_device(&mxc_ssi1_device, NULL);
	mxc_register_device(&mxc_ssi2_device, NULL);
	mxc_register_device(&mxc_alsa_spdif_device, &mxc_spdif_data);
	mxc_register_device(&mxc_android_pmem_device, &android_pmem_data);
	mxc_register_device(&mxc_android_pmem_gpu_device,
				&android_pmem_gpu_data);
	mxc_register_device(&usb_mass_storage_device, &mass_storage_data);
	mxc_register_device(&usb_rndis_device, &rndis_data);
	mxc_register_device(&android_usb_device, &android_usb_data);

	mxc_register_device(&mxc_ptp_device, NULL);
	/* ASRC is only available for MX53 TO2.0 */
	if (mx53_revision() >= IMX_CHIP_REVISION_2_0) {
		mxc_asrc_data.asrc_core_clk = clk_get(NULL, "asrc_clk");
		clk_put(mxc_asrc_data.asrc_core_clk);
		mxc_asrc_data.asrc_audio_clk = clk_get(NULL, "asrc_serial_clk");
		clk_put(mxc_asrc_data.asrc_audio_clk);
		mxc_register_device(&mxc_asrc_device, &mxc_asrc_data);
	}

	i2c_register_board_info(0, mxc_i2c1_board_info,
				ARRAY_SIZE(mxc_i2c1_board_info));
	i2c_register_board_info(1, mxc_i2c2_board_info,
				ARRAY_SIZE(mxc_i2c2_board_info));
	i2c_register_board_info(2, mxc_i2c3_board_info,
				ARRAY_SIZE(mxc_i2c3_board_info));

	wm8958_data.ext_ram_clk = clk_get(NULL, "emi_fast_clk");
	clk_put(wm8958_data.ext_ram_clk);
	mxc_register_device(&mxc_wm8958_device, &wm8958_data);
	spdif_audio_data.ext_ram_clk = clk_get(NULL, "emi_fast_clk");
	clk_put(spdif_audio_data.ext_ram_clk);
	mxc_register_device(&mxc_spdif_audio_device, &spdif_audio_data);
	mx5_set_otghost_vbus_func(mx53_gpio_usbotg_driver_vbus);
	mx5_usb_dr_init();
	mx5_usbh1_init();
	mxc_register_device(&mxc_v4l2_device, NULL);
	mxc_register_device(&mxc_v4l2out_device, NULL);
	mxc_register_device(&mxc_bt_rfkill, &mxc_bt_rfkill_data);
	mxc_register_device(&mxc_wifi_control, &wifi_control_data);
	pcba_add_device_buttons();
	mxc_register_device(&mxc_powerkey_device, &pwrkey_data);
	mxc_register_device(&leds_mc34708_device, &leds_mc34708_data);
	huawei_mu509_poweron();
}

static void __init mx53_pcba_timer_init(void)
{
	struct clk *uart_clk;

	mx53_clocks_init(32768, 24000000, 22579200, 0);

	uart_clk = clk_get_sys("mxcintuart.0", NULL);
	early_console_setup(MX53_BASE_ADDR(UART1_BASE_ADDR), uart_clk);
}

static struct sys_timer mxc_timer = {
	.init	= mx53_pcba_timer_init,
};

static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	char *str;
	struct tag *t;
	struct tag *mem_tag = 0;
	int total_mem = SZ_1G;
	int left_mem = 0, avali_mem = 0;
	int gpu_mem = SZ_64M;
	int pmem_gpu_size = android_pmem_gpu_data.size;
	int pmem_adsp_size = android_pmem_data.size;

	mxc_set_cpu_type(MXC_CPU_MX53);

	/* get mem= and gpu_memory= from cmdline */
	for_each_tag(t, tags) {
		if (t->hdr.tag == ATAG_CMDLINE) {
			str = t->u.cmdline.cmdline;
			str = strstr(str, "mem=");
			if (str != NULL) {
				str += 4;
				avali_mem = memparse(str, &str);
			}

			str = t->u.cmdline.cmdline;
			str = strstr(str, "gpu_nommu");
			if (str != NULL)
				gpu_data.enable_mmu = 0;

			str = t->u.cmdline.cmdline;
			str = strstr(str, "gpu_memory=");
			if (str != NULL) {
				str += 11;
				gpu_mem = memparse(str, &str);
			}
			break;
		}
	}

	if (gpu_data.enable_mmu)
		gpu_mem = 0;

	/* get total memory from TAGS */
	for_each_tag(mem_tag, tags) {
		if (mem_tag->hdr.tag == ATAG_MEM) {
			total_mem = mem_tag->u.mem.size;
			left_mem = total_mem - gpu_mem
				- pmem_gpu_size - pmem_adsp_size;
			break;
		}
	}

	if (avali_mem > 0 && avali_mem < left_mem)
		left_mem = avali_mem;

	if (mem_tag) {
		android_pmem_data.start = mem_tag->u.mem.start
				+ left_mem + gpu_mem + pmem_gpu_size;
		android_pmem_gpu_data.start = mem_tag->u.mem.start
				+ left_mem + gpu_mem;
		mem_tag->u.mem.size = left_mem;

		/*reserve memory for gpu*/
		if (!gpu_data.enable_mmu) {
			gpu_device.resource[5].start =
				mem_tag->u.mem.start + left_mem;
			gpu_device.resource[5].end =
				gpu_device.resource[5].start + gpu_mem - 1;
		}
	}
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX53_PCBA data structure.
 */
MACHINE_START(MX53_PCBA, "Freescale MX53 PCBA Board")
	/* Maintainer: Freescale Semiconductor, Inc. */
	.fixup = fixup_mxc_board,
	.map_io = mx5_map_io,
	.init_irq = mx5_init_irq,
	.init_machine = mxc_board_init,
	.timer = &mxc_timer,
MACHINE_END
