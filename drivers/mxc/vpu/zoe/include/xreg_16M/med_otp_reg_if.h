/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_otp_reg_if                                                 */
/*  File     : med_otp_reg_if.h                                               */
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

#ifndef __MED_OTP_REG_IF_H__
#define __MED_OTP_REG_IF_H__

/** @ingroup rdl
 *  @file med_otp_reg_if.h
 */
#define MED_OTP_REG_IF_OTP_START                0x00000000
#define MED_OTP_REG_IF_OTP_STATUS               0x00000004
#define MED_OTP_REG_IF_OTP_COMMAND              0x00000008
#define MED_OTP_REG_IF_OTP_ADDRESS              0x0000000C
#define MED_OTP_REG_IF_OTP_WRITE_DATA           0x00000010
#define MED_OTP_REG_IF_OTP_READ_DATA            0x00000014
#define MED_OTP_REG_IF_OTP_DEST_ADDRESS         0x00000018
#define MED_OTP_REG_IF_OTP_CONTROL              0x00000020
#define MED_OTP_REG_IF_OTP_SHA1_DATA            0x00000024
#define MED_OTP_REG_IF_OTP_SHA1_MSDATA          0x00000028
#define MED_OTP_REG_IF_MISC_STS                 0x00000030
#define MED_OTP_REG_IF_WR_BIT_ERR_STS_REG0      0x00000034
#define MED_OTP_REG_IF_WR_BIT_ERR_STS_REG1      0x00000038
#define MED_OTP_REG_IF_WR_BIT_ERR_STS_REG2      0x0000003C
#define MED_OTP_REG_IF_WR_BIT_ERR_STS_REG3      0x00000040
#define MED_OTP_REG_IF_ID_LOG                   0x00000050
#define MED_OTP_REG_IF_CMD_LOG                  0x00000054
#define MED_OTP_REG_IF_ADDR_LOG                 0x00000058
#define MED_OTP_REG_IF_OTP_INTR_ENBL_SET        0x00000070
#define MED_OTP_REG_IF_OTP_INTR_ENBL_CLR        0x00000074
#define MED_OTP_REG_IF_OTP_INTR_STAT_SET        0x00000078
#define MED_OTP_REG_IF_OTP_INTR_STAT_CLR        0x0000007C
#define MED_OTP_REG_IF_CTEXT0                   0x00000100
#define MED_OTP_REG_IF_CTEXT1                   0x00000104
#define MED_OTP_REG_IF_CTEXT2                   0x00000108
#define MED_OTP_REG_IF_CTEXT3                   0x0000010C
#define MED_OTP_REG_IF_PTEXT0                   0x00000200
#define MED_OTP_REG_IF_PTEXT1                   0x00000204
#define MED_OTP_REG_IF_PTEXT2                   0x00000208
#define MED_OTP_REG_IF_PTEXT3                   0x0000020C
#define MED_OTP_REG_IF_TIM_CSRST                0x00000300
#define MED_OTP_REG_IF_TIM_RW                   0x00000304
#define MED_OTP_REG_IF_TIM_RS                   0x00000308
#define MED_OTP_REG_IF_TIM_RD_ADDR              0x0000030C
#define MED_OTP_REG_IF_TIM_WR_AS                0x00000310
#define MED_OTP_REG_IF_TIM_WR_DS                0x00000314
#define MED_OTP_REG_IF_TIM_WR_WWL               0x00000318
#define MED_OTP_REG_IF_TIM_WR_DPD               0x0000031C
#define MED_OTP_REG_IF_TIM_WR_WWH               0x00000320
#define MED_OTP_REG_IF_TIM_WR_PW                0x00000324
#define MED_OTP_REG_IF_TIM_WR_CPPW              0x00000328
#define MED_OTP_REG_IF_TIM_WR_PGDIS             0x0000032C
#define MED_OTP_REG_IF_TIM_WR_PES               0x00000330
#define MED_OTP_REG_IF_TIM_WR_PEH               0x00000334
#define MED_OTP_REG_IF_TIM_WR_PGMDD             0x00000338
#define MED_OTP_REG_IF_TIM_WR_DLEH              0x0000033C
#define MED_OTP_REG_IF_TIM_WR_CPH               0x00000340
#define MED_OTP_REG_IF_TIM_WR_DLES              0x00000344
#define MED_OTP_REG_IF_TIM_WR_CPS               0x00000348
#define MED_OTP_REG_IF_TIM_WR_PGMAS             0x0000034C

#endif
