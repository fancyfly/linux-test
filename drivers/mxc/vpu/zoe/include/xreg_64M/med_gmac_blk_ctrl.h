/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_gmac_blk_ctrl                                              */
/*  File     : med_gmac_blk_ctrl.h                                            */
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

#ifndef __MED_GMAC_BLK_CTRL_H__
#define __MED_GMAC_BLK_CTRL_H__

/** @ingroup rdl
 *  @file med_gmac_blk_ctrl.h
 */
#define MED_GMAC_BLK_CTRL_GMAC_RESET_SET            0x00000000
#define MED_GMAC_BLK_CTRL_GMAC_RESET_CLR            0x00000004
#define MED_GMAC_BLK_CTRL_GMAC_RESET                0x00000008
#define MED_GMAC_BLK_CTRL_GMAC0_RESET_SET           0x00000010
#define MED_GMAC_BLK_CTRL_GMAC0_RESET_CLR           0x00000014
#define MED_GMAC_BLK_CTRL_GMAC0_RESET               0x00000018
#define MED_GMAC_BLK_CTRL_GMAC1_RESET_SET           0x00000020
#define MED_GMAC_BLK_CTRL_GMAC1_RESET_CLR           0x00000024
#define MED_GMAC_BLK_CTRL_GMAC1_RESET               0x00000028
#define MED_GMAC_BLK_CTRL_GMAC_CLOCK_ENABLE_SET     0x00000100
#define MED_GMAC_BLK_CTRL_GMAC_CLOCK_ENABLE_CLR     0x00000104
#define MED_GMAC_BLK_CTRL_GMAC_CLOCK_ENABLE         0x00000108
#define MED_GMAC_BLK_CTRL_GMAC0_CLOCK_ENABLE_SET    0x00000110
#define MED_GMAC_BLK_CTRL_GMAC0_CLOCK_ENABLE_CLR    0x00000114
#define MED_GMAC_BLK_CTRL_GMAC0_CLOCK_ENABLE        0x00000118
#define MED_GMAC_BLK_CTRL_GMAC1_CLOCK_ENABLE_SET    0x00000120
#define MED_GMAC_BLK_CTRL_GMAC1_CLOCK_ENABLE_CLR    0x00000124
#define MED_GMAC_BLK_CTRL_GMAC1_CLOCK_ENABLE        0x00000128
#define MED_GMAC_BLK_CTRL_BLK_CTRL_INT_ENBL_SET     0x00000200
#define MED_GMAC_BLK_CTRL_BLK_CTRL_INT_ENBL_CLR     0x00000204
#define MED_GMAC_BLK_CTRL_BLK_CTRL_INT_STAT_SET     0x00000208
#define MED_GMAC_BLK_CTRL_BLK_CTRL_INT_STAT_CLR     0x0000020C
#define MED_GMAC_BLK_CTRL_XREG_TIMEOUT_EN           0x00000300
#define MED_GMAC_BLK_CTRL_XREG_TIMEOUT_RELOAD       0x00000304
#define MED_GMAC_BLK_CTRL_XREG_TIMEOUT_COUNT        0x00000308
#define MED_GMAC_BLK_CTRL_XREG_TIMEOUT_STATUS       0x00000310
#define MED_GMAC_BLK_CTRL_XREG_TIMEOUT_ADDR         0x00000314
#define MED_GMAC_BLK_CTRL_XREG_COHERENCY_0          0x00000400
#define MED_GMAC_BLK_CTRL_XREG_COHERENCY_0_ADDR     0x00000404
#define MED_GMAC_BLK_CTRL_XREG_COHERENCY_1          0x00000410
#define MED_GMAC_BLK_CTRL_XREG_COHERENCY_1_ADDR     0x00000414
#define MED_GMAC_BLK_CTRL_FLOORPLAN_SECURITY_SEC_ENBL 0x00000500
#define MED_GMAC_BLK_CTRL_FLOORPLAN_SECURITY_SEC_STAT 0x00000504
#define MED_GMAC_BLK_CTRL_GMAC0_CLKDIV              0x00000600
#define MED_GMAC_BLK_CTRL_GMAC1_CLKDIV              0x00000604
#define MED_GMAC_BLK_CTRL_ECO_REG                   0x00000E00
#define MED_GMAC_BLK_CTRL_MEM_ADJ_OUT               0x00000E10
#define MED_GMAC_BLK_CTRL_GMAC0_PACKET              0x00001000
#define MED_GMAC_BLK_CTRL_GMAC1_PACKET              0x00001004
#define MED_GMAC_BLK_CTRL_GMAC0_AXI_MSTR_ENDIAN     0x00001008
#define MED_GMAC_BLK_CTRL_GMAC1_AXI_MSTR_ENDIAN     0x0000100C
#define MED_GMAC_BLK_CTRL_GMAC0_DMA_SLAVE_ERR_TAG   0x00001050
#define MED_GMAC_BLK_CTRL_GMAC1_DMA_SLAVE_ERR_TAG   0x00001054

#endif
