/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_global_reg_dec                                             */
/*  File     : med_global_reg_dec.h                                           */
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

#ifndef __MED_GLOBAL_REG_DEC_H__
#define __MED_GLOBAL_REG_DEC_H__

/** @ingroup rdl
 *  @file med_global_reg_dec.h
 */
#define MED_GLOBAL_REG_DEC_PIPE_CONFIG                      0x00000000
#define MED_GLOBAL_REG_DEC_INPUT_PIPE                       0x00000004
#define MED_GLOBAL_REG_DEC_INPUT_PIXEL_SIZE                 0x00000008
#define MED_GLOBAL_REG_DEC_INPUT_MB_SIZE                    0x0000000C
#define MED_GLOBAL_REG_DEC_ECO_REGISTER                     0x00000010
#define MED_GLOBAL_REG_DEC_INTRA_DECODE_ENABLE              0x00000014
#define MED_GLOBAL_REG_DEC_HEVC_DBLK_CONTROL_STATUS         0x00000018
#define MED_GLOBAL_REG_DEC_HEVC_DBLK_BETA_TC_OFFSET_PARAMS  0x0000001C
#define MED_GLOBAL_REG_DEC_HEVC_DBLK_POC_IDS_LIST0_REG0     0x00000020
#define MED_GLOBAL_REG_DEC_HEVC_DBLK_POC_IDS_LIST0_REG1     0x00000024
#define MED_GLOBAL_REG_DEC_HEVC_DBLK_POC_IDS_LIST1_REG0     0x00000028
#define MED_GLOBAL_REG_DEC_HEVC_DBLK_POC_IDS_LIST1_REG1     0x0000002C
#define MED_GLOBAL_REG_DEC_SLICE_QP_Y                       0x00000030
#define MED_GLOBAL_REG_DEC_EC_SYNC_ENABLE                   0x00000034
#define MED_GLOBAL_REG_DEC_CHROMA_QP_OFFSET                 0x00000038
#define MED_GLOBAL_REG_DEC_CU_QP_DELTA                      0x0000003C
#define MED_GLOBAL_REG_DEC_SAO_CFG                          0x00000044
#define MED_GLOBAL_REG_DEC_REC_LUMA_F0                      0x00000048
#define MED_GLOBAL_REG_DEC_REC_CHROMA_F0                    0x0000004C
#define MED_GLOBAL_REG_DEC_REC_LUMA_F1                      0x00000050
#define MED_GLOBAL_REG_DEC_REC_CHROMA_F1                    0x00000054
#define MED_GLOBAL_REG_DEC_COL_MV                           0x00000058
#define MED_GLOBAL_REG_DEC_DMA_CFG                          0x0000005C
#define MED_GLOBAL_REG_DEC_HEVC_MVP_SLICE_CFG               0x00000060
#define MED_GLOBAL_REG_DEC_LAST_CTU_IN_SLICE_POS            0x00000064
#define MED_GLOBAL_REG_DEC_INTERRUPT_CTRL_ENBL_SET          0x00000070
#define MED_GLOBAL_REG_DEC_INTERRUPT_CTRL_ENBL_CLR          0x00000074
#define MED_GLOBAL_REG_DEC_INTERRUPT_CTRL_STAT_SET          0x00000078
#define MED_GLOBAL_REG_DEC_INTERRUPT_CTRL_STAT_CLR          0x0000007C
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_COL_BD_REG_0_1        0x00000080
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_COL_BD_REG_2_3        0x00000084
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_COL_BD_REG_4_5        0x00000088
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_COL_BD_REG_6_7        0x0000008C
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_COL_BD_REG_8_9        0x00000090
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_ROW_BD_REG_0_1        0x00000094
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_ROW_BD_REG_2_3        0x00000098
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_ROW_BD_REG_4_5        0x0000009C
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_ROW_BD_REG_6_7        0x000000A0
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_ROW_BD_REG_8_9        0x000000A4
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_ROW_BD_REG_10         0x000000A8
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_SLICE_CONFIG_0        0x000000AC
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_BLOCK_CONFIG_0        0x000000B0
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_LT                    0x000000B4
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC0                  0x000000B8
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC1                  0x000000BC
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC2                  0x000000C0
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC3                  0x000000C4
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC4                  0x000000C8
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC5                  0x000000CC
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC6                  0x000000D0
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC7                  0x000000D4
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC8                  0x000000D8
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC9                  0x000000DC
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC10                 0x000000E0
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC11                 0x000000E4
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC12                 0x000000E8
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC13                 0x000000EC
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC14                 0x000000F0
#define MED_GLOBAL_REG_DEC_INPUT_HEVC_POC                   0x000000F4

#endif
