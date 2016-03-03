/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_ed_stats_regs                                              */
/*  File     : med_ed_stats_regs.h                                            */
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

#ifndef __MED_ED_STATS_REGS_H__
#define __MED_ED_STATS_REGS_H__

/** @ingroup rdl
 *  @file med_ed_stats_regs.h
 */
#define MED_ED_STATS_REGS_ED_RESET_STATS                       0x00000000
#define MED_ED_STATS_REGS_ED_INTRA_TOTAL                       0x00000008
#define MED_ED_STATS_REGS_ED_INTER_TOTAL                       0x00000010
#define MED_ED_STATS_REGS_ED_INTER_8X8                         0x00000018
#define MED_ED_STATS_REGS_ED_INTER_16X16                       0x00000020
#define MED_ED_STATS_REGS_ED_INTER_L0_REF_IDX_0                0x00000028
#define MED_ED_STATS_REGS_ED_INTER_L0_TOTAL                    0x00000030
#define MED_ED_STATS_REGS_ED_INTER_L1_REF_IDX_0                0x00000038
#define MED_ED_STATS_REGS_ED_INTER_L1_TOTAL                    0x00000040
#define MED_ED_STATS_REGS_ED_INTER_BI_REF_IDX_0_0              0x00000048
#define MED_ED_STATS_REGS_ED_INTER_BI_TOTAL                    0x00000050
#define MED_ED_STATS_REGS_ED_INTER_B_DIRECT_16X16              0x00000058
#define MED_ED_STATS_REGS_ED_INTER_SKIP                        0x00000060
#define MED_ED_STATS_REGS_ED_INTER_FIELD                       0x00000068
#define MED_ED_STATS_REGS_ED_INTRA_FIELD                       0x00000070
#define MED_ED_STATS_REGS_ED_INTRA_TOTAL_NZCOEFFICIENTS        0x00000078
#define MED_ED_STATS_REGS_ED_TOTAL_NZCOEFFICIENTS              0x00000080
#define MED_ED_STATS_REGS_ED_INTRA_TOTAL_LUMA_NZCOEFFICIENTS   0x00000088
#define MED_ED_STATS_REGS_ED_TOTAL_LUMA_NZCOEFFICIENTS         0x00000090
#define MED_ED_STATS_REGS_ED_SUM_QP                            0x00000098
#define MED_ED_STATS_REGS_ED_MAX_QP                            0x000000A0
#define MED_ED_STATS_REGS_ED_MIN_QP                            0x000000A8

#endif
