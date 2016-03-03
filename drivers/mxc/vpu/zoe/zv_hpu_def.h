/*
 * Copyright (c) 2012-2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


///////////////////////////////////////////////////////////////////////////////
//
// zv_hpu_def.h
//
// Description: 
//
//  ZV hpu bus registers/hardware definition
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////



#ifndef __ZV_HPU_DEF_H__
#define __ZV_HPU_DEF_H__

#include "zoe_types.h"
#include "zoe_xreg.h"

#ifdef ZOE_TARGET_CHIP_VAR
#if (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define SCB_BLK_CTRL            (SCB_XREG_SLV_BASE + MED_SCB_SCB_BLK_CTRL)
#define INT_CTRL_XREG_SLV_BASE  (SCB_XREG_SLV_BASE + MED_SCB_INT_CTRL)
#define MDMA_XREG_SLV_BASE      (SCB_XREG_SLV_BASE + MED_SCB_MDMA)
#endif // 16MB
#endif //ZOE_TARGET_CHIP_VAR


#define VPU_CLK_RST_XREG_SLV_BASE           (CLKRST_XREG_SLV_BASE)
#define VPU_SCB_CTRL_XREG_SLV_BASE	        (SCB_BLK_CTRL)
#define VPU_INTERRUPT_CTRL_BASE_ADDRESS		(INT_CTRL_XREG_SLV_BASE)
#define VPU_MDMA_XREG_SLV_BASE_ADDRESS		(MDMA_XREG_SLV_BASE)

#if defined(ZOE_TARGET_CHIP_VAR) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define VPU_MDMA_LIST_OFFSET	            VPU_MDMA_XREG_SLV_BASE_ADDRESS
#define VPU_MDMA_INT_STAT_OFFSET            (VPU_MDMA_XREG_SLV_BASE_ADDRESS + 0x3800)
#else // 64MB
#define VPU_MDMA_LIST_OFFSET	            (VPU_MDMA_XREG_SLV_BASE_ADDRESS + 0x40000)
#define VPU_MDMA_INT_STAT_OFFSET            (VPU_MDMA_XREG_SLV_BASE_ADDRESS + 0xF0000)
#endif // 16MB


#include "zv_dma_def.h"

// DMA channel direction
enum
{
    ZVDMA_DIR_READ = 0, // zv device to host
    ZVDMA_DIR_WRITE,	// host to zv device
    ZVDMA_DIR_MAX
};

#endif //__ZV_HPU_DEF_H__

