/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_int_ctrl_isr_bank                                          */
/*  File     : med_int_ctrl_isr_bank.h                                        */
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

#ifndef __MED_INT_CTRL_ISR_BANK_H__
#define __MED_INT_CTRL_ISR_BANK_H__

/** @ingroup rdl
 *  @file med_int_ctrl_isr_bank.h
 */
#define MED_INT_CTRL_ISR_BANK_ISR_SET        0x00000000
#define MED_INT_CTRL_ISR_BANK_ISR_CLR        0x00000004
#define MED_INT_CTRL_ISR_BANK_INT_EN_SET     0x00000008
#define MED_INT_CTRL_ISR_BANK_INT_EN_CLR     0x0000000C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS00  0x00000100
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS01  0x00000104
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS02  0x00000108
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS03  0x0000010C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS04  0x00000110
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS05  0x00000114
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS06  0x00000118
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS07  0x0000011C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS08  0x00000120
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS09  0x00000124
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS10  0x00000128
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS11  0x0000012C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS12  0x00000130
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS13  0x00000134
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS14  0x00000138
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS15  0x0000013C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS16  0x00000140
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS17  0x00000144
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS18  0x00000148
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS19  0x0000014C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS20  0x00000150
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS21  0x00000154
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS22  0x00000158
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS23  0x0000015C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS24  0x00000160
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS25  0x00000164
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS26  0x00000168
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS27  0x0000016C
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS28  0x00000170
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS29  0x00000174
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS30  0x00000178
#define MED_INT_CTRL_ISR_BANK_CONTROL_STS31  0x0000017C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL00 0x00000200
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL01 0x00000204
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL02 0x00000208
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL03 0x0000020C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL04 0x00000210
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL05 0x00000214
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL06 0x00000218
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL07 0x0000021C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL08 0x00000220
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL09 0x00000224
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL10 0x00000228
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL11 0x0000022C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL12 0x00000230
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL13 0x00000234
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL14 0x00000238
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL15 0x0000023C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL16 0x00000240
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL17 0x00000244
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL18 0x00000248
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL19 0x0000024C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL20 0x00000250
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL21 0x00000254
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL22 0x00000258
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL23 0x0000025C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL24 0x00000260
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL25 0x00000264
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL26 0x00000268
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL27 0x0000026C
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL28 0x00000270
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL29 0x00000274
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL30 0x00000278
#define MED_INT_CTRL_ISR_BANK_INTS_ENFOR_IOL31 0x0000027C

#endif
