/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_quant_xreg                                                 */
/*  File     : med_quant_xreg.h                                               */
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

#ifndef __MED_QUANT_XREG_H__
#define __MED_QUANT_XREG_H__

/** @ingroup rdl
 *  @file med_quant_xreg.h
 */
#define MED_QUANT_XREG_Q_4X4_LEVELSCALE                0x00000000
#define MED_QUANT_XREG_Q_4X4_EXPECTED_DISTORTION       0x00002000
#define MED_QUANT_XREG_Q_8X8_LEVELSCALE                0x00004000
#define MED_QUANT_XREG_Q_8X8_EXPECTED_DISTORTION       0x00006000
#define MED_QUANT_XREG_Q_4X4_LEVELOFFSET               0x00008000
#define MED_QUANT_XREG_Q_8X8_LEVELOFFSET               0x0000C000
#define MED_QUANT_XREG_IQ_INTRA_WEIGHT_MATRIX          0x00010000
#define MED_QUANT_XREG_IQ_INTER_WEIGHT_MATRIX          0x00014000
#define MED_QUANT_XREG_MODE_QP_SCALE_TBL_A1            0x00018000
#define MED_QUANT_XREG_QP_SLICE                        0x0001C000
#define MED_QUANT_XREG_MODE_DECISION_CTRL              0x0001C004
#define MED_QUANT_XREG_X_SCALE                         0x0001C008
#define MED_QUANT_XREG_DISABLE_INTRA                   0x0001C00C
#define MED_QUANT_XREG_DISABLE_INTER_4X4               0x0001C010
#define MED_QUANT_XREG_DISABLE_INTER_8X8               0x0001C014
#define MED_QUANT_XREG_INTRA_NON_ZERO                  0x0001C018
#define MED_QUANT_XREG_INTRA_PRED_MODE_OFFSET          0x0001C01C
#define MED_QUANT_XREG_INTRA_PRED_MODE                 0x0001C020
#define MED_QUANT_XREG_INTER_4X4_NON_ZERO              0x0001C024
#define MED_QUANT_XREG_INTER_8X8_NON_ZERO              0x0001C02C
#define MED_QUANT_XREG_FINAL_INTER                     0x0001C038
#define MED_QUANT_XREG_FINAL_INTRA                     0x0001C03C
#define MED_QUANT_XREG_DEBUG_ENC_MB_STOP               0x0001C040
#define MED_QUANT_XREG_DEBUG_CURRENT_MB                0x0001C044
#define MED_QUANT_XREG_DEBUG_STATUS                    0x0001C048
#define MED_QUANT_XREG_QUANT_LIMIT                     0x0001C04C
#define MED_QUANT_XREG_MODE_A2                         0x0001C050
#define MED_QUANT_XREG_NONZERO_ACCUM_INTRA             0x0001C054
#define MED_QUANT_XREG_NONZERO_ACCUM_INTER_4X4         0x0001C058
#define MED_QUANT_XREG_NONZERO_ACCUM_INTER_8X8         0x0001C05C
#define MED_QUANT_XREG_DISTORTION_ACCUM_INTRA          0x0001C060
#define MED_QUANT_XREG_DISTORTION_ACCUM_TOTAL          0x0001C064
#define MED_QUANT_XREG_NORM_ADJ_4X4_Y_ROW0             0x0001C070
#define MED_QUANT_XREG_NORM_ADJ_4X4_Y_ROW1             0x0001C074
#define MED_QUANT_XREG_NORM_ADJ_4X4_Y_ROW2             0x0001C078
#define MED_QUANT_XREG_NORM_ADJ_4X4_Y_ROW3             0x0001C07C
#define MED_QUANT_XREG_NORM_ADJ_4X4_Y_ROW4             0x0001C080
#define MED_QUANT_XREG_NORM_ADJ_4X4_U_ROW0             0x0001C090
#define MED_QUANT_XREG_NORM_ADJ_4X4_U_ROW1             0x0001C094
#define MED_QUANT_XREG_NORM_ADJ_4X4_U_ROW2             0x0001C098
#define MED_QUANT_XREG_NORM_ADJ_4X4_U_ROW3             0x0001C09C
#define MED_QUANT_XREG_NORM_ADJ_4X4_U_ROW4             0x0001C0A0
#define MED_QUANT_XREG_NORM_ADJ_4X4_V_ROW0             0x0001C0B0
#define MED_QUANT_XREG_NORM_ADJ_4X4_V_ROW1             0x0001C0B4
#define MED_QUANT_XREG_NORM_ADJ_4X4_V_ROW2             0x0001C0B8
#define MED_QUANT_XREG_NORM_ADJ_4X4_V_ROW3             0x0001C0BC
#define MED_QUANT_XREG_NORM_ADJ_4X4_V_ROW4             0x0001C0C0
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW0_LFT           0x0001C0D0
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW0_RGT           0x0001C0D4
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW1_LFT           0x0001C0D8
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW1_RGT           0x0001C0DC
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW2_LFT           0x0001C0E0
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW2_RGT           0x0001C0E4
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW3_LFT           0x0001C0E8
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW3_RGT           0x0001C0EC
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW4_LFT           0x0001C0F0
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW4_RGT           0x0001C0F4
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW5_LFT           0x0001C0F8
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW5_RGT           0x0001C0FC
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW6_LFT           0x0001C100
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW6_RGT           0x0001C104
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW7_LFT           0x0001C108
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW7_RGT           0x0001C10C
#define MED_QUANT_XREG_NORM_ADJ_8X8_ROW8               0x0001C110
#define MED_QUANT_XREG_CLIP_2063_ENABLE                0x0001C120
#define MED_QUANT_XREG_CLIP_2063_COUNT                 0x0001C124
#define MED_QUANT_XREG_STAT_INTRA_TOTAL_FIELD          0x0001C128
#define MED_QUANT_XREG_STAT_INTRA_4X4_8X8              0x0001C12C
#define MED_QUANT_XREG_STAT_INTER_TOTAL_FIELD          0x0001C130
#define MED_QUANT_XREG_STAT_INTER_XFORM                0x0001C134
#define MED_QUANT_XREG_STAT_INTER_8X8_16X16            0x0001C138
#define MED_QUANT_XREG_STAT_INTER_SKIPPED_BDIRECT      0x0001C13C
#define MED_QUANT_XREG_STAT_INTER_L0                   0x0001C140
#define MED_QUANT_XREG_STAT_INTER_L1                   0x0001C144
#define MED_QUANT_XREG_STAT_INTER_BI                   0x0001C148
#define MED_QUANT_XREG_STAT_CTRL                       0x0001C14C
#define MED_QUANT_XREG_GLOBALS                         0x0001FF00

#endif
