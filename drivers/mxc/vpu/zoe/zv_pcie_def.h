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
// zv_pcie_def.h
//
// Description: 
//
//  ZV PCIe registers/hardware definition
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////



#ifndef __ZV_PCIE_DEF_H__
#define __ZV_PCIE_DEF_H__

#include "zoe_types.h"
#include "zoe_xreg.h"

#define ZV_DMA_MAX_LEN                          0x1000000



// ZN200 BARS
//
enum
{
    ZVPCIE_BAR_XMEM = 0,
    ZVPCIE_BAR_XREG,
    ZVPCIE_BAR_XWND,
    ZVPCIE_BAR_MAX
};


#pragma pack(1)

#ifdef ZOE_TARGET_CHIP_VAR
#if (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define SCB_BLK_CTRL            (SCB_XREG_SLV_BASE + MED_SCB_SCB_BLK_CTRL)
#define INT_CTRL_XREG_SLV_BASE  (SCB_XREG_SLV_BASE + MED_SCB_INT_CTRL)
#define MDMA_XREG_SLV_BASE      (SCB_XREG_SLV_BASE + MED_SCB_MDMA)
#endif // 16MB
#endif //ZOE_TARGET_CHIP_VAR


#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

#define ZV_SERIALIZE_DMA

// Vendor and Device IDs
//
#define ZV_PCI_VENDOR_ID                        0x1A33
#define ZV_PCI_DEVICE_ID                        0x8202


#define XREG_BASE                               XREG_ARB_BASE_PHY

#define CLK_RST_XREG_SLV_BASE	                (XREG_BASE + CLKRST_XREG_SLV_BASE)
#define SCB_CTRL_XREG_SLV_BASE	                (XREG_BASE + SCB_BLK_CTRL)
#define INTERRUPT_CTRL_BASE_ADDRESS				(XREG_BASE + INT_CTRL_XREG_SLV_BASE)
#define MDMA_XREG_SLV_BASE_ADDRESS				(XREG_BASE + MDMA_XREG_SLV_BASE)
#define PCIE_DMA_REG_ACCESS_BASE_ADDRESS		(XREG_BASE + PCIE_XREG_SLV_BASE + MED_PCIE_CHANNEL_0_MEMORY_ADDRESS)        // 0x010C0000
#define PCIE_WND_REG_ACCESS_BASE_ADDRESS		(XREG_BASE + PCIE_XREG_SLV_BASE + MED_PCIE_DO_NOT_USE_WINDOW_LOWER_ADDR)    // 0x01080000

#if defined(ZOE_TARGET_CHIP_VAR) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define MDMA_LIST_OFFSET			            MDMA_XREG_SLV_BASE_ADDRESS
#define MDMA_INT_STAT_OFFSET                    (MDMA_XREG_SLV_BASE_ADDRESS + 0x3800)
#else // 64MB
#define MDMA_LIST_OFFSET			            (MDMA_XREG_SLV_BASE_ADDRESS + 0x40000)
#define MDMA_INT_STAT_OFFSET                    (MDMA_XREG_SLV_BASE_ADDRESS + 0xF0000)
#endif // 16MB



#include "zv_dma_def.h"


#define ZV_DMA_READ_LIST_ADDR                   0x70000
#define ZV_DMA_WRITE_LIST_ADDR                  (ZV_DMA_READ_LIST_ADDR + ZV_DMA_MAX_DESCR_LIST_SIZE)

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

#undef ZV_SERIALIZE_DMA

// Vendor and Device IDs
//
#define ZV_PCI_VENDOR_ID                        0x1A33
#define ZV_PCI_DEVICE_ID                        0x8201


// This is the hypothetical hardware DMA descriptor
//
typedef struct PED_DMA_DESCRIPTOR
{
    uint32_t    Control;
    uint32_t    ByteCount;
    uint64_t    SystemAddress;
    uint32_t    DeviceAddress;
    uint64_t    NextDescriptor;
} PED_DMA_DESCRIPTOR, *PPED_DMA_DESCRIPTOR;

#define ZV_DMA_MAX_NUM_DESCR                    1


#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP


#pragma pack()


// DMA channel direction
enum
{
    ZVPCIE_DIR_READ = 0,    // zv device to host
    ZVPCIE_DIR_WRITE,	    // host to zv device
    ZVPCIE_DIR_MAX
};


#define XMEM_WND_BASE                           0           // ddr 0
#define XMEM_WND_MASK                           0xFF000000  // 16 Megabytes
#define XMEM_WND_SPAN                           ~XMEM_WND_MASK

#define XREG_WND_BASE                           0x71000000  // PCIe/DMA base
#define XREG_WND_MASK                           0xFFFF0000  // 64 Kilobytes
#define XREG_WND_SPAN                           ~XREG_WND_MASK



// BAR[ZVPCIE_BAR_XWND] offsets
//
#define XMEM_WINDOW_BASE_OFFSET                 0x00
#define XMEM_WINDOW_BASE_MASK_OFFSET            0x04
#define XREG_WINDOW_BASE_OFFSET                 0x08
#define XREG_WINDOW_BASE_MASK_OFFSET            0x0C
#define OK_TO_PGM_XMEM_WND_OFFSET               0x10
#define OK_TO_PGM_XREG_WND_OFFSET               0x14
#define PCIE_XREG_BUS_RESET_OFFSET              0x80

#define PGM_WND_TIMEOUT                         256

// DMA related stuffs
//
#define DIRECTION_DDR_TO_PCIEBUS	            0
#define DIRECTION_PCIEBUS_TO_DDR	            1

#define BURST_SIZE(width)	                    (width << 8)
#define PCIE_ADDR_INCR		                    (1 << 4) 
#define START_DMA			                    1
#define STOP_DMA			                    0



#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

#define ZV_PCIE_IOL_0                           (1 << 0)    // CPU interrupts
#define ZV_PCIE_IOL_1                           (1 << 1)    // PCIe interrupt
#define ZV_PCIE_INTERRUPT_MASK                  (ZV_PCIE_IOL_0 | ZV_PCIE_IOL_1)                                                  

#define CPU_MASK			                    ZV_PCIE_IOL_0
#define PCIE_MASK                               ZV_PCIE_IOL_1

#define MAX_CPU			                        8


#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

#define ZV_PCIE_INTERRUPT_HW4                   (1 << 20)   // CPU interrupts
#define ZV_PCIE_INTERRUPT_HW6                   (1 << 22)   // PCIe interrupt
#define ZV_PCIE_INTERRUPT_MASK                  (ZV_PCIE_INTERRUPT_HW4 | ZV_PCIE_INTERRUPT_HW6)                                                  

#define CPU_MASK			                    ZV_PCIE_INTERRUPT_HW4
#define PCIE_MASK                               ZV_PCIE_INTERRUPT_HW6

#define MAX_CPU			                        9

// ZN200 register definition
//
#define XREG_BASE                               0x70000000

#define CLK_RST_XREG_SLV_BASE	                (XREG_BASE + 0x01800000)
#define INTERRUPT_CTRL_BASE_ADDRESS				(XREG_BASE + 0x01300000)

#define CLK_RST_SOFT_RST_EXT_OFFSET             0x1014
#define CLK_RST_CLK_ENA_SET_OFFSET	            0x1800

#define CLKRST_SOFT_RST_EXT_INT_CTL_RST         (1 << 1)
#define CLKRST_CLK_ENABLE_INT_CTL               (1 << 11)


// DMA register base
//
#define PCIE_DMA_REG_ACCESS_BASE_ADDRESS		0x710C0000
#define PCIE_WND_REG_ACCESS_BASE_ADDRESS		0x71080000

// All CPU interrupts
//
#define REG4_INT_ALL_OFFSET                     0x4F00
#define REG4_INT_CPU_START                      7
#define REG4_INT_CPU_MASK                       0x0000FF80
#define REG4_INT_OFFSET(x)                      (0x4000 + (4 * ((x) + REG4_INT_CPU_START)))
#define REG4_INT_ENABLE(x)                      (0x4080 + (4 * ((x) + REG4_INT_CPU_START)))
#define PED_IRQ_CPU_INT(x)                      (0x00000001 << ((x) + REG4_INT_CPU_START))

// HPU interrupt
//
#define REG4_INT7_OFFSET						0x401C
#define REG4_ENABLE7_OFFSET					    (REG4_INT7_OFFSET + 0x80)

// SPU interrupt
//
#define REG4_INT8_OFFSET						0x4020
#define REG4_ENABLE8_OFFSET					    (REG4_INT8_OFFSET + 0x80)

// DMAPU interrupt
//
#define REG4_INT9_OFFSET						0x4024
#define REG4_ENABLE9_OFFSET					    (REG4_INT9_OFFSET + 0x80)

// AUD0PU interrupt
//
#define REG4_INT10_OFFSET						0x4028
#define REG4_ENABLE10_OFFSET				    (REG4_INT10_OFFSET + 0x80)

// AUD1PU interrupt
//
#define REG4_INT11_OFFSET						0x402C
#define REG4_ENABLE11_OFFSET				    (REG4_INT11_OFFSET + 0x80)

// EDPU interrupt
//
#define REG4_INT12_OFFSET						0x4030
#define REG4_ENABLE12_OFFSET				    (REG4_INT12_OFFSET + 0x80)

// EEPU interrupt
//
#define REG4_INT13_OFFSET						0x4034
#define REG4_ENABLE13_OFFSET				    (REG4_INT13_OFFSET + 0x80)

// MEPU interrupt
//
#define REG4_INT14_OFFSET						0x4038
#define REG4_ENABLE14_OFFSET				    (REG4_INT14_OFFSET + 0x80)

// EXT interrupt
//
#define REG4_INT15_OFFSET						0x403C
#define REG4_ENABLE15_OFFSET				    (REG4_INT14_OFFSET + 0x80)

// PCIe interrupt
#define REG6_INT13_OFFSET						0x6034
#define REG6_ENABLE13_OFFSET					(REG6_INT13_OFFSET + 0x80)
#define REG6_PCIE_READ_OFFSET					0x6500

#define PED_IRQ_DMA_COMPLETE(x)                 (0x00000001 << (x))

#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

// WND offsets
//
#define UPPER_ADDR_REG_OFFSET					0x0008
#define OUTSTANDING_COMPLETIONS_REG_OFFSET		0x0018
#define DMA_CHANNEL_COMPLETE_REG_OFFSET			0x0180 
#define INTERRUPT_OUT_PCIE_REG_OFFSET			0x0114

// DMA offsets
//
#define CHANNEL0_REG_OFFSET						0x0
#define CHANNEL1_REG_OFFSET						0x100

#define DMA_G0_REG_OFFSET						0x0000
#define DMA_CONTROL_REG_OFFSET					0x0004
#define TRANSFER_SIZE_REG_OFFSET				0x0008
#define MEMORY_ADDRESS_REG_OFFSET				0x000C
#define PCIE_ADDRESS_REG_OFFSET					0x0010
#define DMA_DONE_REG_OFFSET						0x0014
#define FLUSH_REG_OFFSET						0x0018
#define BYTES_CONSUMED_REG_OFFSET				0x001C
#define PCIE_ADDRESS_UPPER_REG_OFFSET			0x0020

// DMA Interrupts
#define DMA_INTERRUPT_ENABLE_REG_OFFSET			0x280
#define LEGACY_INTERRUPT_IN_ENABLE_OFFSET		0x70
#define MSI_INTERRUPT_IN_ENABLE_OFFSET			0x60
#define INTERRUPT_IN_STATUS_OFFSET				0x64
#define MSI_ENABLE_OFFSET						0x68
#define SYNOPSIS_CORE_CFG_MSI_EN_STATUS         0x6c

#define DMA_INTERRUPT_STATUS_REG_OFFSET			0x200
#define DMA_CHANNEL0_INTERRUPT_CLEAR_REG_OFFSET	0x200
#define DMA_CHANNEL1_INTERRUPT_CLEAR_REG_OFFSET 0x204

#define TIME_OUT_SHIFT							16
#define HOST_CYCLE_FOR_PCI_READ_COMPLETION      0x400

#endif //__ZV_PCIE_DEF_H__

