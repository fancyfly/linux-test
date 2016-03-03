/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_misc_ctrl_reg                                              */
/*  File     : med_misc_ctrl_reg.h                                            */
/*                                                                            */
/*  Author   : reggen 1.56                                                    */
/*  Created  :                                                                */
/* -------------------------------------------------------------------------- */
/*  Comments :      !! AUTO-GENERATED-FILE - DO NOT EDIT !!                   */
/*                                                                            */
/*  COPYRIGHT:     Zenverge Inc. Proprietary and Confidential                 */
/*                      Copyright (c) 2015, Zenverge Inc.                     */
/*                            All rights reserved.                            */
/*                                                                            */
/*         Permission to use, modify or copy this code is prohibited          */
/*           without the express written consent of Zenverge Inc.             */
/*                                                                            */
/* ========================================================================== */

#ifndef __MED_MISC_CTRL_REG_H__
#define __MED_MISC_CTRL_REG_H__

/** @ingroup rdl
 *  @file med_misc_ctrl_reg.h
 */
#define MED_MISC_CTRL_REG_SCRATCH_0                   0x00000004
#define MED_MISC_CTRL_REG_SCRATCH_1                   0x00000008
#define MED_MISC_CTRL_REG_SCRATCH_2                   0x0000000C
#define MED_MISC_CTRL_REG_SCRATCH_3                   0x00000010
#define MED_MISC_CTRL_REG_CHIP_ID                     0x000000F0
#define MED_MISC_CTRL_REG_TAG_VERSION                 0x000000F4
#define MED_MISC_CTRL_REG_FPGA_IDENTIFIER             0x000000F8
#define MED_MISC_CTRL_REG_CPU_CROSS_TRIG              0x00000100
#define MED_MISC_CTRL_REG_CPU_TRACE_CTRL              0x00000104
#define MED_MISC_CTRL_REG_ECO_REG                     0x00000234
#define MED_MISC_CTRL_REG_DRV_STR_MCARD               0x00000280
#define MED_MISC_CTRL_REG_DRV_STR_SPI                 0x00000284
#define MED_MISC_CTRL_REG_DRV_STR_GPIO                0x00000288
#define MED_MISC_CTRL_REG_DRV_STR_GPIO_4_6            0x0000028C
#define MED_MISC_CTRL_REG_DRV_STR_I2C0                0x00000290
#define MED_MISC_CTRL_REG_DRV_STR_I2C1                0x00000294
#define MED_MISC_CTRL_REG_DRV_STR_UART0               0x00000298
#define MED_MISC_CTRL_REG_DRV_STR_UART1               0x0000029C
#define MED_MISC_CTRL_REG_DRV_STR_UART2               0x000002A0
#define MED_MISC_CTRL_REG_DRV_STR_MMC_PTS2            0x000002A4
#define MED_MISC_CTRL_REG_DRV_STR_NAND_MMC            0x000002A8
#define MED_MISC_CTRL_REG_DRV_STR_JTAG                0x000002AC
#define MED_MISC_CTRL_REG_DRV_STR_MISC                0x000002B0
#define MED_MISC_CTRL_REG_DRV_STR_RGMII0              0x000002B4
#define MED_MISC_CTRL_REG_DRV_STR_RGMII1_PTS3         0x000002B8
#define MED_MISC_CTRL_REG_DRV_STR_RGMII_REFCLK        0x000002BC
#define MED_MISC_CTRL_REG_DRV_STR_MOCA                0x000002C0
#define MED_MISC_CTRL_REG_DRV_STR_MOCA_AFE_CLK        0x000002C4
#define MED_MISC_CTRL_REG_DRV_STR_MTR                 0x000002C8
#define MED_MISC_CTRL_REG_DRV_STR_PTS0                0x000002CC
#define MED_MISC_CTRL_REG_DRV_STR_PTS1                0x000002D0
#define MED_MISC_CTRL_REG_DRV_STR_STS0                0x000002D4
#define MED_MISC_CTRL_REG_DRV_STR_STS1                0x000002D8
#define MED_MISC_CTRL_REG_DRV_STR_STS2                0x000002DC
#define MED_MISC_CTRL_REG_DRV_STR_STS3                0x000002E0
#define MED_MISC_CTRL_REG_DRV_STR_STS4                0x000002E4
#define MED_MISC_CTRL_REG_DRV_STR_STS5                0x000002E8
#define MED_MISC_CTRL_REG_DRV_STR_STS6                0x000002EC
#define MED_MISC_CTRL_REG_DRV_STR_STS7                0x000002F0
#define MED_MISC_CTRL_REG_DRV_STR_CLOCK_OUT           0x000002F4
#define MED_MISC_CTRL_REG_DRV_STR_PCIE_REFCLK_OUT     0x000002F8
#define MED_MISC_CTRL_REG_SLEW_RATE_MCARD             0x00000300
#define MED_MISC_CTRL_REG_SLEW_RATE_SPI               0x00000304
#define MED_MISC_CTRL_REG_SLEW_RATE_GPIO              0x00000308
#define MED_MISC_CTRL_REG_SLEW_RATE_GPIO_4_6          0x0000030C
#define MED_MISC_CTRL_REG_SLEW_RATE_I2C0              0x00000310
#define MED_MISC_CTRL_REG_SLEW_RATE_I2C1              0x00000314
#define MED_MISC_CTRL_REG_SLEW_RATE_UART0             0x00000318
#define MED_MISC_CTRL_REG_SLEW_RATE_UART1             0x0000031C
#define MED_MISC_CTRL_REG_SLEW_RATE_UART2             0x00000320
#define MED_MISC_CTRL_REG_SLEW_RATE_MMC_PTS2          0x00000324
#define MED_MISC_CTRL_REG_SLEW_RATE_NAND_MMC          0x00000328
#define MED_MISC_CTRL_REG_SLEW_RATE_JTAG              0x0000032C
#define MED_MISC_CTRL_REG_SLEW_RATE_MISC              0x00000330
#define MED_MISC_CTRL_REG_SLEW_RATE_RGMII0            0x00000334
#define MED_MISC_CTRL_REG_SLEW_RATE_RGMII1_PTS3       0x00000338
#define MED_MISC_CTRL_REG_SLEW_RATE_RGMII_REFCLK      0x0000033C
#define MED_MISC_CTRL_REG_SLEW_RATE_MOCA              0x00000340
#define MED_MISC_CTRL_REG_SLEW_RATE_MOCA_AFE_CLK      0x00000344
#define MED_MISC_CTRL_REG_SLEW_RATE_MTR               0x00000348
#define MED_MISC_CTRL_REG_SLEW_RATE_PTS0              0x0000034C
#define MED_MISC_CTRL_REG_SLEW_RATE_PTS1              0x00000350
#define MED_MISC_CTRL_REG_SLEW_RATE_STS0              0x00000354
#define MED_MISC_CTRL_REG_SLEW_RATE_STS1              0x00000358
#define MED_MISC_CTRL_REG_SLEW_RATE_STS2              0x0000035C
#define MED_MISC_CTRL_REG_SLEW_RATE_STS3              0x00000360
#define MED_MISC_CTRL_REG_SLEW_RATE_STS4              0x00000364
#define MED_MISC_CTRL_REG_SLEW_RATE_STS5              0x00000368
#define MED_MISC_CTRL_REG_SLEW_RATE_STS6              0x0000036C
#define MED_MISC_CTRL_REG_SLEW_RATE_STS7              0x00000370
#define MED_MISC_CTRL_REG_SLEW_RATE_CLOCK_OUT         0x00000374
#define MED_MISC_CTRL_REG_SLEW_RATE_PCIE_REFCLK_OUT   0x00000378

#endif
