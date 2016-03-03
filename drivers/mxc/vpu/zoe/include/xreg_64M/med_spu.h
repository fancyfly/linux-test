/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_spu                                                        */
/*  File     : med_spu.h                                                      */
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

#ifndef __MED_SPU_H__
#define __MED_SPU_H__

/** @ingroup rdl
 *  @file med_spu.h
 */
#define MED_SPU_BOOT_ROM                           0x00000000
#define MED_SPU_SECURITY_REG                       0x00010000
#define MED_SPU_SPU_CPU_REGS                       0x00020000
#define MED_SPU_OTP                                0x00030000
#define MED_SPU_RSA_ECC                            0x00040000
#define MED_SPU_SPU_SCRATCH_MEMORY                 0x00080000
#define MED_SPU_SEC_REG_SECURITY_CTRL_SEC_ENBL     0x000E0000
#define MED_SPU_SEC_REG_SECURITY_CTRL_SEC_STAT     0x000E0004
#define MED_SPU_SPU_CPU_SECURITY_CTRL_SEC_ENBL     0x000E0010
#define MED_SPU_SPU_CPU_SECURITY_CTRL_SEC_STAT     0x000E0014
#define MED_SPU_OTP_SECURITY_CTRL_SEC_ENBL         0x000E0020
#define MED_SPU_OTP_SECURITY_CTRL_SEC_STAT         0x000E0024
#define MED_SPU_RSA_ECC_SECURITY_CTRL_SEC_ENBL     0x000E0030
#define MED_SPU_RSA_ECC_SECURITY_CTRL_SEC_STAT     0x000E0034
#define MED_SPU_SCRATCH_SECURITY_CTRL_SEC_ENBL     0x000E0040
#define MED_SPU_SCRATCH_SECURITY_CTRL_SEC_STAT     0x000E0044
#define MED_SPU_BLK_CTRL_SECURITY_CTRL_SEC_ENBL    0x000E0050
#define MED_SPU_BLK_CTRL_SECURITY_CTRL_SEC_STAT    0x000E0054
#define MED_SPU_SPU_BLK_CTRL                       0x000F0000

#endif
