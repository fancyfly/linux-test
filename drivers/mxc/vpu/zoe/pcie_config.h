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
// pcie_config.h
//
// Description: 
//
//  PCIe device configuration defination
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __PCIE_CONFIG_H__
#define __PCIE_CONFIG_H__

#include "zoe_types.h"


// Bar Types
typedef enum _PCI_BAR_TYPE
{
    RegisterMemBarType  = 0, // Memory Bar with Registers
    RamMemBarType       = 1, // Memory Bar with RAM
    RegisterIoBarType   = 2, // IO Bar with Registers
    DisabledBarType     = 3  // Disabled Bar
} PCI_BAR_TYPE, *PPCI_BAR_TYPE;


#define transSize8Bit               0x01    // 8 Bit transfer
#define transSize16Bit              0x02    // 16 Bit transfer
#define transSize32Bit              0x04    // 32 Bit transfer
#define transSize64Bit              0x08    // 64 Bit transfer (not currently supported)
#define transSize32BitDma           0x10    // 32 Bit transfer
#define transSize64BitDma           0x20    // 64 Bit transfer

#define PCI_TRANSFER_TYPES          (transSize8Bit | transSize16Bit | transSize32Bit)

#define PCI_MAX_BARS                6
#define PCI_DMA_CACHE_ENABLED       FALSE


#define PCI_BAR_TYPE_MASK           0x1
#define PCI_BAR_TYPE_MEM            0x0
#define PCI_BAR_TYPE_IO             0x1
#define PCI_BAR_MEM_ADDR_MASK       0x6
#define PCI_BAR_MEM_ADDR_32         0x0
#define PCI_BAR_MEM_ADDR_1M         0x2
#define PCI_BAR_MEM_ADDR_64         0x4
#define PCI_BAR_MEM_CACHE_MASK      0x8
#define PCI_BAR_MEM_CACHABLE        0x8

#define PCI_BAR_MEM_MASK            (~0x0F)
#define PCI_BAR_IO_MASK             (~0x03)
#define IS_IO_BAR(x)                (((x) & PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_IO)
#define IS_MEMORY_BAR(x)            (((x) & PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_MEM)
#define IS_MEMORY_32BIT(x)          (((x) & PCI_BAR_MEM_ADDR_MASK) == PCI_BAR_MEM_ADDR_32)
#define IS_MEMORY_64BIT(x)          (((x) & PCI_BAR_MEM_ADDR_MASK) == PCI_BAR_MEM_ADDR_64)
#define IS_MEMORY_BELOW1M(x)        (((x) & PCI_BAR_MEM_ADDR_MASK) == PCI_BAR_MEM_ADDR_1M)
#define IS_MEMORY_CACHABLE(x)       (((x) & PCI_BAR_MEM_CACHE_MASK) == PCI_BAR_MEM_CACHABLE)

#define PCI_TYPE0_ADDRESS_CNT       6   // TYPE-0 configuration header values
#define PCI_DEVICE_SPECIFIC_SIZE    192 // Device specific bytes [64..256]


#pragma pack(1)

// PCI Configuration Header
//
typedef struct _PCI_CONFIG_HEADER
{
    // The following fields contain the first 64 bytes of PCI configuration space.
    uint16_t    VendorID;
    uint16_t    DeviceID;
    uint16_t    Command;
    uint16_t    Status;
    uint8_t     RevisionID;
    uint8_t     ProgIf;
    uint8_t     SubClass;
    uint8_t     BaseClass;
    uint8_t     CacheLineSize;
    uint8_t     LatencyTimer;
    uint8_t     HeaderType;
    uint8_t     BIST;
    uint32_t    RawBaseAddresses[PCI_TYPE0_ADDRESS_CNT];
    uint32_t    CardBusCISPtr;
    uint16_t    SubsystemVendorID;
    uint16_t    SubsystemID;
    uint32_t    ROMBaseAddress;
    uint8_t     CapabilitiesPtr;
    uint8_t     Reserved1[3];
    uint32_t    Reserved2[1];
    uint8_t     InterruptLine;
    uint8_t     InterruptPin;
    uint8_t     MinimumGrant;
    uint8_t     MaximumLatency;
    uint8_t     DeviceSpecific[PCI_DEVICE_SPECIFIC_SIZE];

    // The following fields are decoded values from the PCI configuration space.
    // Make sure these start on an 8-byte boundary.
    uint64_t    BaseAddresses[PCI_MAX_BARS];
    uint64_t    BaseSizes[PCI_MAX_BARS];
    PCI_BAR_TYPE    BarType[PCI_MAX_BARS];
    uint16_t    BarTransferTypes[PCI_MAX_BARS];
    uint32_t    AddressSize;
} PCI_CONFIG_HEADER, *PPCI_CONFIG_HEADER;

#pragma pack()

#endif //__PCIE_CONFIG_H__



