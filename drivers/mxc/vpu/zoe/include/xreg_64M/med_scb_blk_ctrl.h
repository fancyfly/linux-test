/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_scb_blk_ctrl                                               */
/*  File     : med_scb_blk_ctrl.h                                             */
/*                                                                            */
/*  Author   : reggen 1.55                                                    */
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

#ifndef __MED_SCB_BLK_CTRL_H__
#define __MED_SCB_BLK_CTRL_H__

/** @ingroup rdl
 *  @file med_scb_blk_ctrl.h
 */
#define MED_SCB_BLK_CTRL_MISC_CTRL_RESET_SET          0x00000000
#define MED_SCB_BLK_CTRL_MISC_CTRL_RESET_CLR          0x00000004
#define MED_SCB_BLK_CTRL_MISC_CTRL_RESET              0x00000008
#define MED_SCB_BLK_CTRL_INT_CTRL_RESET_SET           0x00000010
#define MED_SCB_BLK_CTRL_INT_CTRL_RESET_CLR           0x00000014
#define MED_SCB_BLK_CTRL_INT_CTRL_RESET               0x00000018
#define MED_SCB_BLK_CTRL_GLOBAL_CTRL_RESET_SET        0x00000020
#define MED_SCB_BLK_CTRL_GLOBAL_CTRL_RESET_CLR        0x00000024
#define MED_SCB_BLK_CTRL_GLOBAL_CTRL_RESET            0x00000028
#define MED_SCB_BLK_CTRL_FDMA_RESET_SET               0x00000030
#define MED_SCB_BLK_CTRL_FDMA_RESET_CLR               0x00000034
#define MED_SCB_BLK_CTRL_FDMA_RESET                   0x00000038
#define MED_SCB_BLK_CTRL_MDMA_RESET_SET               0x00000040
#define MED_SCB_BLK_CTRL_MDMA_RESET_CLR               0x00000044
#define MED_SCB_BLK_CTRL_MDMA_RESET                   0x00000048
#define MED_SCB_BLK_CTRL_PPM_RESET_SET                0x00000050
#define MED_SCB_BLK_CTRL_PPM_RESET_CLR                0x00000054
#define MED_SCB_BLK_CTRL_PPM_RESET                    0x00000058
#define MED_SCB_BLK_CTRL_CRYPTO_RESET_SET             0x00000060
#define MED_SCB_BLK_CTRL_CRYPTO_RESET_CLR             0x00000064
#define MED_SCB_BLK_CTRL_CRYPTO_RESET                 0x00000068
#define MED_SCB_BLK_CTRL_FWPU_RESET_SET               0x00000070
#define MED_SCB_BLK_CTRL_FWPU_RESET_CLR               0x00000074
#define MED_SCB_BLK_CTRL_FWPU_RESET                   0x00000078
#define MED_SCB_BLK_CTRL_GDMA_XMEM_RESET_SET          0x00000080
#define MED_SCB_BLK_CTRL_GDMA_XMEM_RESET_CLR          0x00000084
#define MED_SCB_BLK_CTRL_GDMA_XMEM_RESET              0x00000088
#define MED_SCB_BLK_CTRL_MISC_CTRL_CLK_EN_SET         0x00000100
#define MED_SCB_BLK_CTRL_MISC_CTRL_CLK_EN_CLR         0x00000104
#define MED_SCB_BLK_CTRL_MISC_CTRL_CLK_EN             0x00000108
#define MED_SCB_BLK_CTRL_INT_CTRL_CLK_EN_SET          0x00000110
#define MED_SCB_BLK_CTRL_INT_CTRL_CLK_EN_CLR          0x00000114
#define MED_SCB_BLK_CTRL_INT_CTRL_CLK_EN              0x00000118
#define MED_SCB_BLK_CTRL_GLOBAL_CTRL_CLK_EN_SET       0x00000120
#define MED_SCB_BLK_CTRL_GLOBAL_CTRL_CLK_EN_CLR       0x00000124
#define MED_SCB_BLK_CTRL_GLOBAL_CTRL_CLK_EN           0x00000128
#define MED_SCB_BLK_CTRL_FDMA_CLK_EN_SET              0x00000130
#define MED_SCB_BLK_CTRL_FDMA_CLK_EN_CLR              0x00000134
#define MED_SCB_BLK_CTRL_FDMA_CLK_EN                  0x00000138
#define MED_SCB_BLK_CTRL_MDMA_CLK_EN_SET              0x00000140
#define MED_SCB_BLK_CTRL_MDMA_CLK_EN_CLR              0x00000144
#define MED_SCB_BLK_CTRL_MDMA_CLK_EN                  0x00000148
#define MED_SCB_BLK_CTRL_PPM_CLK_EN_SET               0x00000150
#define MED_SCB_BLK_CTRL_PPM_CLK_EN_CLR               0x00000154
#define MED_SCB_BLK_CTRL_PPM_CLK_EN                   0x00000158
#define MED_SCB_BLK_CTRL_CRYPTO_CLK_EN_SET            0x00000160
#define MED_SCB_BLK_CTRL_CRYPTO_CLK_EN_CLR            0x00000164
#define MED_SCB_BLK_CTRL_CRYPTO_CLK_EN                0x00000168
#define MED_SCB_BLK_CTRL_FWPU_CLK_EN_SET              0x00000170
#define MED_SCB_BLK_CTRL_FWPU_CLK_EN_CLR              0x00000174
#define MED_SCB_BLK_CTRL_FWPU_CLK_EN                  0x00000178
#define MED_SCB_BLK_CTRL_GDMA_XMEM_CLK_EN_SET         0x00000180
#define MED_SCB_BLK_CTRL_GDMA_XMEM_CLK_EN_CLR         0x00000184
#define MED_SCB_BLK_CTRL_GDMA_XMEM_CLK_EN             0x00000188
#define MED_SCB_BLK_CTRL_BLK_CTRL_INT_ENBL_SET        0x00000200
#define MED_SCB_BLK_CTRL_BLK_CTRL_INT_ENBL_CLR        0x00000204
#define MED_SCB_BLK_CTRL_BLK_CTRL_INT_STAT_SET        0x00000208
#define MED_SCB_BLK_CTRL_BLK_CTRL_INT_STAT_CLR        0x0000020C
#define MED_SCB_BLK_CTRL_XREG_TIMEOUT_EN              0x00000300
#define MED_SCB_BLK_CTRL_XREG_TIMEOUT_RELOAD          0x00000304
#define MED_SCB_BLK_CTRL_XREG_TIMEOUT_COUNT           0x00000308
#define MED_SCB_BLK_CTRL_XREG_TIMEOUT_STATUS          0x00000310
#define MED_SCB_BLK_CTRL_XREG_TIMEOUT_ADDR            0x00000314
#define MED_SCB_BLK_CTRL_XREG_MASTERTIMEOUT_EN        0x00000380
#define MED_SCB_BLK_CTRL_XREG_MASTERTIMEOUT_RELOAD    0x00000384
#define MED_SCB_BLK_CTRL_XREG_MASTERTIMEOUT_COUNT     0x00000388
#define MED_SCB_BLK_CTRL_XREG_MASTERTIMEOUT_STATUS    0x00000390
#define MED_SCB_BLK_CTRL_XREG_MASTERTIMEOUT_ADDR      0x00000394
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_0             0x00000400
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_0_ADDR        0x00000404
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_1             0x00000410
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_1_ADDR        0x00000414
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_2             0x00000420
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_2_ADDR        0x00000424
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_3             0x00000430
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_3_ADDR        0x00000434
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_4             0x00000440
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_4_ADDR        0x00000444
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_5             0x00000450
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_5_ADDR        0x00000454
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_6             0x00000460
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_6_ADDR        0x00000464
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_7             0x00000470
#define MED_SCB_BLK_CTRL_XREG_COHERENCY_7_ADDR        0x00000474
#define MED_SCB_BLK_CTRL_FLOORPLAN_SECURITY_SEC_ENBL  0x00000500
#define MED_SCB_BLK_CTRL_FLOORPLAN_SECURITY_SEC_STAT  0x00000504
#define MED_SCB_BLK_CTRL_ECO_REG                      0x00000E00
#define MED_SCB_BLK_CTRL_MEM_ADJ_OUT                  0x00000E10

#endif
