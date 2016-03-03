/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_hpu_cpu_regs                                               */
/*  File     : med_hpu_cpu_regs.h                                             */
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

#ifndef __MED_HPU_CPU_REGS_H__
#define __MED_HPU_CPU_REGS_H__

/** @ingroup rdl
 *  @file med_hpu_cpu_regs.h
 */
#define MED_HPU_CPU_REGS_XMEM_OVERRIDE_ADDR           0x00000000
#define MED_HPU_CPU_REGS_SET_XMEM_TAG                 0x00000300
#define MED_HPU_CPU_REGS_OUTSTANDING_XMEM_WR_COUNT    0x00000304
#define MED_HPU_CPU_REGS_ECO_REGISTER                 0x00000308
#define MED_HPU_CPU_REGS_POWER_UP_CONTROL_SET         0x00000310
#define MED_HPU_CPU_REGS_POWER_UP_CONTROL_CLR         0x00000314
#define MED_HPU_CPU_REGS_CORE0_COLD_PWR_STATE         0x00000318
#define MED_HPU_CPU_REGS_CORE1_COLD_PWR_STATE         0x0000031C
#define MED_HPU_CPU_REGS_MIPS_POWER_UP_STATUS         0x00000320
#define MED_HPU_CPU_REGS_ENABLE_NMI_FROM_IOL          0x00000324
#define MED_HPU_CPU_REGS_TRACE_PORT_CTRL              0x00000380
#define MED_HPU_CPU_REGS_HPU_CPU_CONTROL_SET          0x00001000
#define MED_HPU_CPU_REGS_HPU_CPU_CONTROL_CLR          0x00001004
#define MED_HPU_CPU_REGS_CORE0_CPU_CONTROL_SET        0x00001010
#define MED_HPU_CPU_REGS_CORE0_CPU_CONTROL_CLR        0x00001014
#define MED_HPU_CPU_REGS_CORE1_CPU_CONTROL_SET        0x00001020
#define MED_HPU_CPU_REGS_CORE1_CPU_CONTROL_CLR        0x00001024
#define MED_HPU_CPU_REGS_HPU_CPU_STATUS               0x00001030
#define MED_HPU_CPU_REGS_CORE0_CPU_STATUS             0x00001034
#define MED_HPU_CPU_REGS_CORE1_CPU_STATUS             0x00001038
#define MED_HPU_CPU_REGS_IOCU_XMEM_PRIORITY           0x00008000
#define MED_HPU_CPU_REGS_IOCU_XREG_PRIORITY           0x00008004
#define MED_HPU_CPU_REGS_XREG_REQ_INFO                0x00008020
#define MED_HPU_CPU_REGS_XMEM_GMAC_REQ_INFO           0x00008024
#define MED_HPU_CPU_REGS_XMEM_PCIE_REQ_INFO           0x0000802C
#define MED_HPU_CPU_REGS_XMEM_USB_REQ_INFO            0x00008030
#define MED_HPU_CPU_REGS_XREG_UPPER_ADDR_BITS_REG     0x00008034
#define MED_HPU_CPU_REGS_XREG_IOCU_REQ_INFO_REG       0x00008338
#define MED_HPU_CPU_REGS_SI0_REG                      0x0000833C
#define MED_HPU_CPU_REGS_SI1_REG                      0x00008340
#define MED_HPU_CPU_REGS_SI_CMP_IOC                   0x00008344
#define MED_HPU_CPU_REGS_HPU_XMEM_PRIORITY            0x00008348
#define MED_HPU_CPU_REGS_CORE0_PM_SYS_SIZE            0x00008350
#define MED_HPU_CPU_REGS_CORE0_PM_SYS_AVAIL           0x00008354
#define MED_HPU_CPU_REGS_CORE1_PM_SYS_SIZE            0x00008358
#define MED_HPU_CPU_REGS_CORE1_PM_SYS_AVAIL           0x0000835C

#endif
