/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_ee_xreg_regfile                                            */
/*  File     : med_ee_xreg_regfile.h                                          */
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

#ifndef __MED_EE_XREG_REGFILE_H__
#define __MED_EE_XREG_REGFILE_H__

/** @ingroup rdl
 *  @file med_ee_xreg_regfile.h
 */
#define MED_EE_XREG_REGFILE_CABAC_INT                                  0x00000000
#define MED_EE_XREG_REGFILE_CABAC_CMD                                  0x00000004
#define MED_EE_XREG_REGFILE_CABAC_SLICE                                0x00000008
#define MED_EE_XREG_REGFILE_CABAC_MB                                   0x0000000C
#define MED_EE_XREG_REGFILE_CABAC4_SUBMB                               0x00000010
#define MED_EE_XREG_REGFILE_CABAC_REFIDX_NUM                           0x00000014
#define MED_EE_XREG_REGFILE_CABAC_MB_ADDR                              0x00000018
#define MED_EE_XREG_REGFILE_CABAC_MBCTX                                0x0000001C
#define MED_EE_XREG_REGFILE_CABAC_I4X4PM0                              0x00000020
#define MED_EE_XREG_REGFILE_CABAC_I4X4PM1                              0x00000024
#define MED_EE_XREG_REGFILE_CABAC_I4X4PM2                              0x00000028
#define MED_EE_XREG_REGFILE_CABAC_I4X4PM3                              0x0000002C
#define MED_EE_XREG_REGFILE_CABAC_I8X8PM                               0x00000030
#define MED_EE_XREG_REGFILE_CABAC_REFL00                               0x00000034
#define MED_EE_XREG_REGFILE_CABAC_REFL01                               0x00000038
#define MED_EE_XREG_REGFILE_CABAC_REFL10                               0x0000003C
#define MED_EE_XREG_REGFILE_CABAC_REFL11                               0x00000040
#define MED_EE_XREG_REGFILE_CABAC_MVDL0_0                              0x00000044
#define MED_EE_XREG_REGFILE_CABAC_MVDL0_1                              0x00000048
#define MED_EE_XREG_REGFILE_CABAC_MVDL0_2                              0x0000004C
#define MED_EE_XREG_REGFILE_CABAC_MVDL0_3                              0x00000050
#define MED_EE_XREG_REGFILE_CABAC_MVDL1_0                              0x00000054
#define MED_EE_XREG_REGFILE_CABAC_MVDL1_1                              0x00000058
#define MED_EE_XREG_REGFILE_CABAC_MVDL1_2                              0x0000005C
#define MED_EE_XREG_REGFILE_CABAC_MVDL1_3                              0x00000060
#define MED_EE_XREG_REGFILE_DBG_MB_STOP                                0x00000068
#define MED_EE_XREG_REGFILE_CABAC_EXTREME_ONES                         0x0000006C
#define MED_EE_XREG_REGFILE_CABAC_EXTREME_ZEROES                       0x00000070
#define MED_EE_XREG_REGFILE_CABAC_BIT_OVRFLOW_FATAL                    0x00000074
#define MED_EE_XREG_REGFILE_CABAC_INTRA_16X16_LUMA_COEFF_COUNT         0x00000080
#define MED_EE_XREG_REGFILE_CABAC_INTRA_8X8_LUMA_COEFF_COUNT           0x00000084
#define MED_EE_XREG_REGFILE_CABAC_INTRA_4X4_LUMA_COEFF_COUNT           0x00000088
#define MED_EE_XREG_REGFILE_CABAC_INTRA_CHROMA_COEFF_COUNT             0x0000008C
#define MED_EE_XREG_REGFILE_CABAC_INTER_LUMA_COEFF_COUNT               0x00000090
#define MED_EE_XREG_REGFILE_CABAC_INTER_LUMA_8X8_TRANSFORM_COEFF_COUNT 0x00000094
#define MED_EE_XREG_REGFILE_CABAC_INTER_CHROMA_COEFF_COUNT             0x00000098
#define MED_EE_XREG_REGFILE_CABAC_MVD_L0_COUNT                         0x0000009C
#define MED_EE_XREG_REGFILE_CABAC_MVD_L1_COUNT                         0x000000A0
#define MED_EE_XREG_REGFILE_CABAC_REF_IDX_L0_COUNT                     0x000000A4
#define MED_EE_XREG_REGFILE_CABAC_REF_IDX_L1_COUNT                     0x000000A8
#define MED_EE_XREG_REGFILE_CABAC_INTER_TOTAL_BITS_COUNT               0x000000AC
#define MED_EE_XREG_REGFILE_CABAC_INTRA_TOTAL_BITS_COUNT               0x000000B0
#define MED_EE_XREG_REGFILE_CABAC_SLICE_DATA_COUNT                     0x000000B4
#define MED_EE_XREG_REGFILE_ENT_ENC_DBG                                0x00000400
#define MED_EE_XREG_REGFILE_EE_FDMA_PUTBITS                            0x00000404
#define MED_EE_XREG_REGFILE_EE_PUTBITS_SEL                             0x0000040C
#define MED_EE_XREG_REGFILE_EE_FIFO_CTRL                               0x00000410

#endif
