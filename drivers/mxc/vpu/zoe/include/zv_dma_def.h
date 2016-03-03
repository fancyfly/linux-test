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
// zv_dma_def.h
//
// Description: 
//
//  VPU MDMA definition
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////



#ifndef __ZV_DMA_DEF_H__
#define __ZV_DMA_DEF_H__

#include "zoe_types.h"

//define MDMA_USE_1_DESC

#define ZV_DMA_MAX_LEN                          0x1000000

#define DMA_CH_CTRL_DIRECTION(x)                ((x) << 0)
#define DMA_CH_CTRL_INC_ADDR(x)                 ((x) << 4)
#define DMA_CH_CTRL_BURST_SIZE(x)               ((x) << 8)
#define DMA_CH_CTRL_XREG_RD_MODE(x)             ((x) << 11)
#define DMA_CH_CTRL_STREAM_MODE(x)              ((x) << 12)
#define DMA_CH_CTRL_ENDIAN_SWAP(x)              ((x) << 16)
#define DMA_CH_CTRL_BLOCKING_DESCR(x)           ((x) << 23)
#define DMA_CH_CTRL_EN_64BIT_ADDR(x)            ((x) << 24)
#define DMA_CH_CTRL_SEC_ID(x)                   ((x) << 25)
#define DMA_CH_CTRL_DESCR_INT_EN(x)             ((x) << 30)
#define DMA_CH_CTRL_LAST_DESCR(x)               ((x) << 31)

// direction
#define ZV_DMA_DIR_XREG_2_XREG                  0
#define ZV_DMA_DIR_XREG_2_DDR                   1
#define ZV_DMA_DIR_DDR_2_XREG                   2
#define ZV_DMA_DIR_DDR_2_DDR                    3

// burst_size
#define ZV_DMA_BURST_8_BYTES                    0
#define ZV_DMA_BURST_16_BYTES                   1
#define ZV_DMA_BURST_32_BYTES                   2
#define ZV_DMA_BURST_64_BYTES                   3
#define ZV_DMA_BURST_128_BYTES                  4
#define ZV_DMA_BURST_256_BYTES                  5

// endian_swap
#define ZV_DMA_SWAP_NONE                        0
#define ZV_DMA_SWAP_16BIT                       0x6
#define ZV_DMA_SWAP_32BIT                       0x5
#define ZV_DMA_SWAP_64BIT                       0x1


#pragma pack(4)

// This is the VPU MDMA descriptor
//
typedef struct PED_DMA_DESCRIPTOR
{
    uint32_t    ddr_addr;
    uint32_t    io_addr;
    uint32_t    control;
    uint32_t    xfer_size;
    uint32_t    upper_io_addr;
} PED_DMA_DESCRIPTOR, *PPED_DMA_DESCRIPTOR;


typedef struct PED_DMA32_DESCRIPTOR
{
    uint32_t    ddr_addr;
    uint32_t    io_addr;
    uint32_t    control;
    uint32_t    xfer_size;
} PED_DMA32_DESCRIPTOR, *PPED_DMA32_DESCRIPTOR;

#pragma pack()

#define ZV_DMA_MAX_DESCR_LIST_SIZE              0x4000
#ifdef MDMA_USE_1_DESC
#define ZV_DMA_MAX_NUM_DESCR                    1
#else //MDMA_USE_1_DESC
#define ZV_DMA_MAX_NUM_DESCR                    ZOE_MIN(128, ((ZV_DMA_MAX_DESCR_LIST_SIZE >> 1) / sizeof(PED_DMA_DESCRIPTOR)))
#endif //MDMA_USE_1_DESC


// VPU register definition
//

#define CLK_RST_SOFT_RST_EXT_OFFSET             0x0000
#define CLK_RST_CLK_ENA_SET_OFFSET	            0x0800

#define CLKRST_SOFT_RST_EXT_INT_CTL_RST         (1 << 7)
#define CLKRST_CLK_ENABLE_INT_CTL               (1 << 7)

#define SCB_CTRL_INT_CTRL_RST_SET_OFFSET        0x0010
#define SCB_CTRL_INT_CTRL_CLK_ENA_SET_OFFSET    0x0110

#define SCB_CTRL_INT_CTRL_RST                   (1 << 0)
#define SCB_CTRL_INT_CTRL_CLK                   (1 << 0)

#define SCB_CTRL_MISC_RST_SET_OFFSET            0x0000
#define SCB_CTRL_MISC_CLK_ENA_SET_OFFSET        0x0100

#define SCB_CTRL_MISC_RST                       0x3F
#define SCB_CTRL_MISC_CLK                       0x3F

#define SCB_CTRL_MDMA_RST_SET_OFFSET            0x0040
#define SCB_CTRL_MDMA_CLK_ENA_SET_OFFSET        0x0140

#define SCB_CTRL_MDMA_RST                       ((1 << 12) | (1 << 14) | (1 << 15) | (1 << 31))
#define SCB_CTRL_MDMA_CLK                       ((1 << 12) | (1 << 14) | (1 << 15) | (1 << 31))

#define SCB_CTRL_GDMA_RST_SET_OFFSET            0x0080
#define SCB_CTRL_GDMA_CLK_ENA_SET_OFFSET        0x0180

#define SCB_CTRL_GDMA_RST                       (1 << 0)
#define SCB_CTRL_GDMA_CLK                       (1 << 0)

#define INT_CTRL_IOL_BANK0_OFFSET               0x8000
#define INT_CTRL_IOL_BANK30_OFFSET              0x8780
#define INT_CTRL_IOL_BANK31_OFFSET              0x87C0

#define INT_CTRL_ISR_BANK0_OFFSET               0x0
#define INT_CTRL_ISR_BANK6_OFFSET               0x1800

// software interrupt
//
// HPU
#define INT_CTRL_CPU2HPU_STS_SET_OFFSET         (INT_CTRL_IOL_BANK0_OFFSET + 0)
#define INT_CTRL_CPU2HPU_STS_CLR_OFFSET         (INT_CTRL_IOL_BANK0_OFFSET + 4)
#define INT_CTRL_CPU2HPU_ENA_SET_OFFSET         (INT_CTRL_IOL_BANK0_OFFSET + 8)
#define INT_CTRL_CPU2HPU_ENA_CLR_OFFSET         (INT_CTRL_IOL_BANK0_OFFSET + 0xC)
#define INT_CTRL_CPU2HPU_IBER_SET_OFFSET        (INT_CTRL_IOL_BANK0_OFFSET + 0x10)
#define INT_CTRL_CPU2HPU_IBER_CLR_OFFSET        (INT_CTRL_IOL_BANK0_OFFSET + 0x14)
#define INT_CTRL_CPU2HPU_IBSR_OFFSET            (INT_CTRL_IOL_BANK0_OFFSET + 0x18)
// PCIe
#define INT_CTRL_CPU2CPU_STS_SET_OFFSET         (INT_CTRL_IOL_BANK30_OFFSET + 0)    
#define INT_CTRL_CPU2CPU_STS_CLR_OFFSET         (INT_CTRL_IOL_BANK30_OFFSET + 4)
#define INT_CTRL_CPU2CPU_ENA_SET_OFFSET         (INT_CTRL_IOL_BANK30_OFFSET + 8)
#define INT_CTRL_CPU2CPU_ENA_CLR_OFFSET         (INT_CTRL_IOL_BANK30_OFFSET + 0xC)
#define INT_CTRL_CPU2CPU_IBER_SET_OFFSET        (INT_CTRL_IOL_BANK30_OFFSET + 0x10)
#define INT_CTRL_CPU2CPU_IBER_CLR_OFFSET        (INT_CTRL_IOL_BANK30_OFFSET + 0x14)
#define INT_CTRL_CPU2CPU_IBSR_OFFSET            (INT_CTRL_IOL_BANK30_OFFSET + 0x18)

// CPU bitmask
#define HPU_INT_MASK                            (1 << 0)
#define FPU_INT_MASK                            (1 << 1)
#define SPU_INT_MASK                            (1 << 2)
#define MEPU_INT_MASK                           (1 << 3)
#define AUD0_INT_MASK                           (1 << 4)
#define AUD1_INT_MASK                           (1 << 5)
#define AUD2_INT_MASK                           (1 << 6)
#define FUSION_INT_MASK                         (1 << 7)
#define PCIE_INT_MASK                           (1 << 8)
#define MDMA_INT_MASK                           (1 << 9)

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

#define MAX_CPU			                        8

#define CPU_ALL_INT_MASK                        (HPU_INT_MASK | FPU_INT_MASK | SPU_INT_MASK | MEPU_INT_MASK | AUD0_INT_MASK | AUD1_INT_MASK | AUD2_INT_MASK | PCIE_INT_MASK)

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

#define MAX_CPU			                        2

#define CPU_ALL_INT_MASK                        (HPU_INT_MASK | FPU_INT_MASK)

#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

#define INT_CTRL_ISR_ISR_SET_PCIE_OFFSET        (INT_CTRL_ISR_BANK0_OFFSET + 0)
#define INT_CTRL_ISR_ISR_CLR_PCIE_OFFSET        (INT_CTRL_ISR_BANK0_OFFSET + 4)
#define INT_CTRL_ISR_ISR_INT_EN_PCIE_SET_OFFSET (INT_CTRL_ISR_BANK0_OFFSET + 8)
#define INT_CTRL_ISR_ISR_INT_EN_PCIE_CLR_OFFSET (INT_CTRL_ISR_BANK0_OFFSET + 0x0C)
#define INT_CTRL_ISR_ISR_SET_HPU_OFFSET         INT_CTRL_ISR_ISR_SET_PCIE_OFFSET
#define INT_CTRL_ISR_ISR_CLR_HPU_OFFSET         INT_CTRL_ISR_ISR_CLR_PCIE_OFFSET
#define INT_CTRL_ISR_ISR_INT_EN_HPU_SET_OFFSET  INT_CTRL_ISR_ISR_INT_EN_PCIE_SET_OFFSET
#define INT_CTRL_ISR_ISR_INT_EN_HPU_CLR_OFFSET  INT_CTRL_ISR_ISR_INT_EN_PCIE_CLR_OFFSET
#define INT_CTRL_ISR_CTRL_STS0_HPU_OFFSET       (INT_CTRL_ISR_BANK0_OFFSET + 0x100)
#define INT_CTRL_ISR_CTRL_STS8_PCIE_OFFSET      (INT_CTRL_ISR_BANK0_OFFSET + 0x120)

#define INT_CTRL_IOL_ISR_BANK0_MASK             (1 << 0)
#define INT_CTRL_IOL_ISR_CPU2CPU_MASK           (1 << 31)
#define INT_CTRL_IOL_IBER_MASK                  (INT_CTRL_IOL_ISR_BANK0_MASK | INT_CTRL_IOL_ISR_CPU2CPU_MASK) 
#define PCIE_ISR_INT_MASK                       (1 << 8)
#define HPU_ISR_INT_MASK                        (1 << 8)

// hardware interrupt
//
// HPU
#define INT_CTRL_IOL_IBER_HPU_SET_OFFSET        (INT_CTRL_IOL_BANK0_OFFSET + 0x10)
#define INT_CTRL_IOL_IBER_HPU_CLR_OFFSET        (INT_CTRL_IOL_BANK0_OFFSET + 0x14)
#define INT_CTRL_IOL_IBSR_HPU_OFFSET            (INT_CTRL_IOL_BANK0_OFFSET + 0x18)
// PCIe
#define INT_CTRL_IOL_IBER_SET_OFFSET            (INT_CTRL_IOL_BANK31_OFFSET + 0x10)
#define INT_CTRL_IOL_IBER_CLR_OFFSET            (INT_CTRL_IOL_BANK31_OFFSET + 0x14)
#define INT_CTRL_IOL_IBSR_OFFSET                (INT_CTRL_IOL_BANK31_OFFSET + 0x18)

#define INT_CTRL_IOL_ISR_BANK6_MASK             (1 << 6)

#define INT_CTRL_ISR_ISR_SET_OFFSET             (INT_CTRL_ISR_BANK6_OFFSET + 0)
#define INT_CTRL_ISR_ISR_CLR_OFFSET             (INT_CTRL_ISR_BANK6_OFFSET + 4)
#define INT_CTRL_ISR_ISR_INT_EN_SET_OFFSET      (INT_CTRL_ISR_BANK6_OFFSET + 8)
#define INT_CTRL_ISR_ISR_INT_EN_CLR_OFFSET      (INT_CTRL_ISR_BANK6_OFFSET + 0x0C)
#define INT_CTRL_ISR_CTRL_STS12_OFFSET          (INT_CTRL_ISR_BANK6_OFFSET + 0x130) // MDMA int 12 HPU
#define INT_CTRL_ISR_CTRL_STS14_OFFSET          (INT_CTRL_ISR_BANK6_OFFSET + 0x138) // MDMA int 14 PCIe read
#define INT_CTRL_ISR_CTRL_STS15_OFFSET          (INT_CTRL_ISR_BANK6_OFFSET + 0x13C) // MDMA int 15 PCIe write

#if defined(ZOE_TARGET_CHIP_VAR) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define MDMA_LIST_HPU_OFFSET		            (0xF40)     // list 61
#define MDMA_LIST_READ_OFFSET			        (0xF80)     // list 62 PCIe
#define MDMA_LIST_WRITE_OFFSET			        (0xFC0)     // list 63 PCIe
#else // 64MB
#define MDMA_LIST_HPU_OFFSET		            (0x3D00)    // list 61
#define MDMA_LIST_READ_OFFSET			        (0x3E00)    // list 62 PCIe
#define MDMA_LIST_WRITE_OFFSET			        (0x3F00)    // list 63 PCIe
#endif // 16MB

#define MDMA_LIST_REG_LS_BASE_PTR_OFFSET        0x0000
#define MDMA_LIST_REG_LS_CTRL1_OFFSET           0x0008
#define MDMA_LIST_REG_LS_CTRL2_OFFSET           0x000C
#define MDMA_LIST_REG_LS_WRITE_PTR_OFFSET       0x0010
#define MDMA_LIST_REG_LS_STAT_OFFSET            0x0014

// MDMA_LIST_REG_LS_CTRL1_OFFSET
#define MDMA_LIST_CTRL1_GO(x)                   ((x) << 0)
#define MDMA_LIST_CTRL1_PAUSE(x)                ((x) << 1)
#define MDMA_LIST_CTRL1_RELEASE(x)              ((x) << 2)

// MDMA_LIST_REG_LS_CTRL2_OFFSET
#define MDMA_LIST_CTRL2_GOBYINT_EN(x)           ((x) << 0)
#define MDMA_LIST_CTRL2_INT_EN(x)               ((x) << 1)
#define MDMA_LIST_CTRL2_PRI(x)                  ((x) << 5)
#define MDMA_LIST_CTRL2_OUT_INT_LINE(x)         ((x) << 11)
#define MDMA_LIST_CTRL2_EN_64BIT(x)             ((x) << 16)

#define MDMA_INT_STAT_SET_OFFSET                0x0000
#define MDMA_INT_STAT_CLR_OFFSET                0x0004
#define MDMA_INT_EN_SET_OFFSET                  0x0010
#define MDMA_INT_EN_CLR_OFFSET                  0x0014
#define MDMA_INT_LIST_DONE_INT_STS_SET_OFFSET   0x0040
#define MDMA_INT_LIST_DONE_INT_STS_CLR_OFFSET   0x0044
#define MDMA_INT_LIST_DONE_INT_EN_SET_OFFSET    0x0050
#define MDMA_INT_LIST_DONE_INT_EN_CLR_OFFSET    0x0054

// dma bitmask
#define DMA_HPU_INT_MASK                        (1 << 12)
#define DMA_READ_INT_MASK                       (1 << 14)
#define DMA_WRITE_INT_MASK                      (1 << 15)

#define DMA_HPU_LIST_MASK                       (1 << 29)
#define DMA_READ_LIST_MASK                      (1 << 30)
#define DMA_WRITE_LIST_MASK                     (1 << 31)

#define PED_IRQ_DMA_COMPLETE(x)                 (0x00000001 << ((x) + 30))


#endif //__ZV_DMA_DEF_H__

