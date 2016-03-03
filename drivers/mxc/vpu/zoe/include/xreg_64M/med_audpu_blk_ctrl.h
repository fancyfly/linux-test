/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_audpu_blk_ctrl                                             */
/*  File     : med_audpu_blk_ctrl.h                                           */
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

#ifndef __MED_AUDPU_BLK_CTRL_H__
#define __MED_AUDPU_BLK_CTRL_H__

/** @ingroup rdl
 *  @file med_audpu_blk_ctrl.h
 */
#define MED_AUDPU_BLK_CTRL_AUDPU_RESET_SET                   0x00000000
#define MED_AUDPU_BLK_CTRL_AUDPU_RESET_CLR                   0x00000004
#define MED_AUDPU_BLK_CTRL_AUDPU_RESET                       0x00000008
#define MED_AUDPU_BLK_CTRL_AUDPU_CLKEN_SET                   0x00000100
#define MED_AUDPU_BLK_CTRL_AUDPU_CLKEN_CLR                   0x00000104
#define MED_AUDPU_BLK_CTRL_AUDPU_CLKEN                       0x00000108
#define MED_AUDPU_BLK_CTRL_BLK_CTRL_INT_ENBL_SET             0x00000200
#define MED_AUDPU_BLK_CTRL_BLK_CTRL_INT_ENBL_CLR             0x00000204
#define MED_AUDPU_BLK_CTRL_BLK_CTRL_INT_STAT_SET             0x00000208
#define MED_AUDPU_BLK_CTRL_BLK_CTRL_INT_STAT_CLR             0x0000020C
#define MED_AUDPU_BLK_CTRL_XREG_TIMEOUT_EN                   0x00000300
#define MED_AUDPU_BLK_CTRL_XREG_TIMEOUT_RELOAD               0x00000304
#define MED_AUDPU_BLK_CTRL_XREG_TIMEOUT_COUNT                0x00000308
#define MED_AUDPU_BLK_CTRL_XREG_TIMEOUT_STATUS               0x00000310
#define MED_AUDPU_BLK_CTRL_XREG_TIMEOUT_ADDR                 0x00000314
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_EN0      0x00000380
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_RELOAD0  0x00000384
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_COUNT0   0x00000388
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_STATUS0  0x00000390
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_ADDR0    0x00000394
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_EN1      0x000003A0
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_RELOAD1  0x000003A4
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_COUNT1   0x000003A8
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_STATUS1  0x000003B0
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_ADDR1    0x000003B4
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_EN2      0x000003C0
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_RELOAD2  0x000003C4
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_COUNT2   0x000003C8
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_STATUS2  0x000003D0
#define MED_AUDPU_BLK_CTRL_AUDPU_XREG_MASTERTIMEOUT_ADDR2    0x000003D4
#define MED_AUDPU_BLK_CTRL_XREG_COHERENCY_0                  0x00000400
#define MED_AUDPU_BLK_CTRL_XREG_COHERENCY_0_ADDR             0x00000404
#define MED_AUDPU_BLK_CTRL_XREG_COHERENCY_1                  0x00000410
#define MED_AUDPU_BLK_CTRL_XREG_COHERENCY_1_ADDR             0x00000414
#define MED_AUDPU_BLK_CTRL_XREG_COHERENCY_2                  0x00000420
#define MED_AUDPU_BLK_CTRL_XREG_COHERENCY_2_ADDR             0x00000424
#define MED_AUDPU_BLK_CTRL_FLOORPLAN_SECURITY_SEC_ENBL       0x00000500
#define MED_AUDPU_BLK_CTRL_FLOORPLAN_SECURITY_SEC_STAT       0x00000504
#define MED_AUDPU_BLK_CTRL_ECO_REG                           0x00000E00
#define MED_AUDPU_BLK_CTRL_MEM_ADJ_OUT                       0x00000E10

#endif
