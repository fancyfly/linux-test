/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DT_BINDINGS_CLOCK_IMX8DV_H
#define __DT_BINDINGS_CLOCK_IMX8DV_H

#define IMX8DV_CLK_DUMMY		0
/* LSIO CLOCKS. */
#define IMX8DV_I2C0_DIV			1
#define IMX8DV_I2C1_DIV			2
#define IMX8DV_I2C2_DIV			3
#define IMX8DV_I2C3_DIV			4
#define IMX8DV_PWM0_DIV			5
#define IMX8DV_PWM1_DIV			6
#define IMX8DV_PWM2_DIV			7
#define IMX8DV_PWM3_DIV			8
#define IMX8DV_PWM4_DIV			9
#define IMX8DV_PWM5_DIV			10
#define IMX8DV_PWM6_DIV			11
#define IMX8DV_PWM7_DIV			12
#define IMX8DV_GPT0_DIV			13
#define IMX8DV_GPT1_DIV			14
#define IMX8DV_GPT2_DIV			15
#define IMX8DV_GPT3_DIV			16
#define IMX8DV_GPT4_DIV			17
#define IMX8DV_I2C0_DIV_CLK		18
#define IMX8DV_I2C1_DIV_CLK		19
#define IMX8DV_I2C2_DIV_CLK		20
#define IMX8DV_I2C3_DIV_CLK		21
#define IMX8DV_I2C0_CLK			22
#define IMX8DV_I2C1_CLK			23
#define IMX8DV_I2C2_CLK			24
#define IMX8DV_I2C3_CLK			25
#define IMX8DV_PWM0_CLK			26
#define IMX8DV_PWM1_CLK			27
#define IMX8DV_PWM2_CLK			28
#define IMX8DV_PWM3_CLK			29
#define IMX8DV_PWM4_CLK			30
#define IMX8DV_PWM5_CLK			31
#define IMX8DV_PWM6_CLK			32
#define IMX8DV_PWM7_CLK			33
#define IMX8DV_GPT0_CLK			34
#define IMX8DV_GPT1_CLK			35
#define IMX8DV_GPT2_CLK			36
#define IMX8DV_GPT3_CLK			37
#define IMX8DV_GPT4_CLK			38
#define IMX8DV_KPP_CLK			39
#define IMX8DV_SSI_CLK			40
/* MegaWrap Clocks. */
#define IMX8DV_SDHC_BUS			41
#define IMX8DV_ENET_BUS			42
#define IMX8DV_ANATOP_BUS		43
#define IMX8DV_SDHC0_DIV 		44
#define IMX8DV_SDHC1_DIV 		45
#define IMX8DV_SDHC2_DIV 		46
#define IMX8DV_ENET0_DIV 		47
#define IMX8DV_ENET1_DIV 		48
#define IMX8DV_GPU0_CORE_DIV	49
#define IMX8DV_GPU0_SHADER_DIV	50
#define IMX8DV_GPU1_CORE_DIV	51
#define IMX8DV_GPU1_SHADER_DIV	52

#define IMX8DV_SDHC_BUS_CLK 	53
#define IMX8DV_ENET_BUS_CLK 	54
#define IMX8DV_ANATOP_BUS_CLK 	55
#define IMX8DV_SDHC0_CLK 		56
#define IMX8DV_SDHC1_CLK 		57
#define IMX8DV_SDHC2_CLK 		58
#define IMX8DV_ENET0_CLK 		59
#define IMX8DV_ENET1_CLK 		60
#define IMX8DV_ENET0_TIME_CLK 	61
#define IMX8DV_ENET1_TIME_CLK 	62
#define IMX8DV_GPU0_CORE_CLK	62
#define IMX8DV_GPU0_SHADER_CLK	63
#define IMX8DV_GPU1_CORE_CLK	63
#define IMX8DV_GPU1_SHADER_CLK	64

#define IMX8DV_CLK_END 			65

#endif /* __DT_BINDINGS_CLOCK_IMX8DV_H */
