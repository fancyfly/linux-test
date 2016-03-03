/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_me                                                         */
/*  File     : med_me.h                                                       */
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

#ifndef __MED_ME_H__
#define __MED_ME_H__

/** @ingroup rdl
 *  @file med_me.h
 */
#define MED_ME_RMEM_0                               0x00000000
#define MED_ME_RMEM_1                               0x00002000
#define MED_ME_RMEM_2                               0x00004000
#define MED_ME_SMEM                                 0x00006000
#define MED_ME_PG_RES_OUTPUT_PARAM                  0x00008000
#define MED_ME_PG_RES_OUTPUT_PREDICTOR              0x00008800
#define MED_ME_PG_RES_OUTPUT_RESIDUAL               0x00009000
#define MED_ME_MVRP_L0_L1_SCRATCH_DMEM              0x0000A000
#define MED_ME_MVRP_L0_L1_IMEM                      0x0000C000
#define MED_ME_MVRP_L0_L1_GPR                       0x0000E000
#define MED_ME_MVRP_L0_L1_PSR                       0x0000E100
#define MED_ME_MVRP_L0_L1_DBG                       0x0000E140
#define MED_ME_MVRP_L0_L1_BRT                       0x0000E180
#define MED_ME_ME_REGFILE                           0x00010000
#define MED_ME_ME_DMA0                              0x00010200
#define MED_ME_ME_DMA1                              0x00010280
#define MED_ME_ME_DMA2                              0x00010300
#define MED_ME_ME_DMA3                              0x00010880
#define MED_ME_FP_RESULTS                           0x00010900
#define MED_ME_SP_RESULTS                           0x00010A00
#define MED_ME_FILL_RESULTS                         0x00010B00
#define MED_ME_FP_REG                               0x00010C00
#define MED_ME_FP_SAD                               0x00010C80
#define MED_ME_LUMA_L0_REFIDX                       0x00010D00
#define MED_ME_CHROMA_L0_REFIDX                     0x00010D80
#define MED_ME_LUMA_L1_REFIDX                       0x00010E00
#define MED_ME_CHROMA_L1_REFIDX                     0x00010E80
#define MED_ME_MVR_RESULTS                          0x00010F00
#define MED_ME_COLOC_MEM                            0x00014000
#define MED_ME_AQ_MVRP_AQ_SCRATCH_DMEM              0x0001A000
#define MED_ME_AQ_MVRP_AQ_IMEM                      0x0001C000
#define MED_ME_AQ_MVRP_AQ_GPR                       0x0001E000
#define MED_ME_AQ_MVRP_AQ_PSR                       0x0001E100
#define MED_ME_AQ_MVRP_AQ_DBG                       0x0001E140
#define MED_ME_AQ_MVRP_AQ_BRT                       0x0001E180
#define MED_ME_GLOBALS                              0x0001FF00

#endif
