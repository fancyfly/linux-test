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
// zoe_ipc_srv.c
//
// Description: 
//
//   ZOE ipc service
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_hal.h"
#include "zoe_sosal.h"
#include "zoe_ipc_srv.h"
#include "zoe_objids.h"
#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#include <stdio.h>
#else //ZOE_LINUXKER_BUILD
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#endif //!ZOE_LINUXKER_BUILD
#include "zoe_common.h"
#include "zoe_xdr.h"

#ifdef ZOE_WINKER_BUILD
#include <wdm.h>
#endif //ZOE_WINKER_BUILD

#define IPC_DBG_MAILBOX

// temporary compile flags
//
//#define _NO_ISR_SUPPORT_YET
//#define _NO_XREG_ACCESS_IN_CONSTRUCTOR


#ifdef ZOE_TARGET_CHIP_VAR
#if (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define SCB_BLK_CTRL            (SCB_XREG_SLV_BASE + MED_SCB_SCB_BLK_CTRL)
#define INT_CTRL_XREG_SLV_BASE  (SCB_XREG_SLV_BASE + MED_SCB_INT_CTRL)
#define MDMA_XREG_SLV_BASE      (SCB_XREG_SLV_BASE + MED_SCB_MDMA)
#define MISC_XREG_SLV_BASE      (SCB_XREG_SLV_BASE + MED_SCB_MISC_CTRL)
#endif // 16MB
#endif //ZOE_TARGET_CHIP_VAR


/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct _ZOE_IPC_DISPATCHER
{
    uint32_t                intf;
    uint32_t                module;
    uint32_t                inst;
    void                    *pContext;
    ZOE_IPC_DISPATCH_FUNC   dispatch_func;
    zoe_bool_t              use_thread;
    uint32_t                priority;
    c_thread                *pThread;
} ZOE_IPC_DISPATCHER, *PZOE_IPC_DISPATCHER;



#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

// ZN200
//

static const uint32_t s_InterruptTable[ZOE_IPC_CPU_NUM] =
{
    ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL,
    ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER,
    ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL,
    ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER,
    ZOE_SOSAL_CHISEL_ISR_SW_SPU,
    ZOE_SOSAL_CHISEL_ISR_SW_EDPU,
    ZOE_SOSAL_CHISEL_ISR_SW_EEPU,
    ZOE_SOSAL_CHISEL_ISR_SW_MEPU,
    ZOE_SOSAL_CHISEL_ISR_SW_DMAPU,
    ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0,
    ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1
};


static const uint32_t s_Interrupt2CPU[ZOE_SOSAL_ISR_SW_NUM] = 
{
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,
    ZOE_IPC_SPU,
    ZOE_IPC_DMAPU,
    ZOE_IPC_AUD0_CPU,
    ZOE_IPC_AUD1_CPU,
    ZOE_IPC_EDPU,
    ZOE_IPC_EEPU,
    ZOE_IPC_MEPU,
    ZOE_IPC_EXT_KERNEL,
    ZOE_IPC_EXT_USER
};


static const uint32_t s_rxChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] = 
{   
    // [EK]                  [EU]                    [HK]                    [HU]                    [S]                    [ED]                    [EE]                    [ME]                    [DMA]                    [A0]                    [A1]
    {ZOE_IPC_CHANNEL_EK2EK,  ZOE_IPC_CHANNEL_EU2EK,  ZOE_IPC_CHANNEL_HK2EK,  ZOE_IPC_CHANNEL_HU2EK,  ZOE_IPC_CHANNEL_S2EK,  ZOE_IPC_CHANNEL_ED2EK,  ZOE_IPC_CHANNEL_EE2EK,  ZOE_IPC_CHANNEL_ME2EK,  ZOE_IPC_CHANNEL_DMA2EK,  ZOE_IPC_CHANNEL_A02EK,  ZOE_IPC_CHANNEL_A12EK},  // EXT_KER
    {ZOE_IPC_CHANNEL_EK2EU,  ZOE_IPC_CHANNEL_EU2EU,  ZOE_IPC_CHANNEL_HK2EU,  ZOE_IPC_CHANNEL_HU2EU,  ZOE_IPC_CHANNEL_S2EU,  ZOE_IPC_CHANNEL_ED2EU,  ZOE_IPC_CHANNEL_EE2EU,  ZOE_IPC_CHANNEL_ME2EU,  ZOE_IPC_CHANNEL_DMA2EU,  ZOE_IPC_CHANNEL_A02EU,  ZOE_IPC_CHANNEL_A12EU},  // EXT_USR
    {ZOE_IPC_CHANNEL_EK2HK,  ZOE_IPC_CHANNEL_EU2HK,  ZOE_IPC_CHANNEL_HK2HK,  ZOE_IPC_CHANNEL_HU2HK,  ZOE_IPC_CHANNEL_S2HK,  ZOE_IPC_CHANNEL_ED2HK,  ZOE_IPC_CHANNEL_EE2HK,  ZOE_IPC_CHANNEL_ME2HK,  ZOE_IPC_CHANNEL_DMA2HK,  ZOE_IPC_CHANNEL_A02HK,  ZOE_IPC_CHANNEL_A12HK},  // HPU_KER
    {ZOE_IPC_CHANNEL_EK2HU,  ZOE_IPC_CHANNEL_EU2HU,  ZOE_IPC_CHANNEL_HK2HU,  ZOE_IPC_CHANNEL_HU2HU,  ZOE_IPC_CHANNEL_S2HU,  ZOE_IPC_CHANNEL_ED2HU,  ZOE_IPC_CHANNEL_EE2HU,  ZOE_IPC_CHANNEL_ME2HU,  ZOE_IPC_CHANNEL_DMA2HU,  ZOE_IPC_CHANNEL_A02HU,  ZOE_IPC_CHANNEL_A12HU},  // HPU_USR
    {ZOE_IPC_CHANNEL_EK2S,   ZOE_IPC_CHANNEL_EU2S,   ZOE_IPC_CHANNEL_HK2S,   ZOE_IPC_CHANNEL_HU2S,   ZOE_IPC_CHANNEL_S2S,   ZOE_IPC_CHANNEL_ED2S,   ZOE_IPC_CHANNEL_EE2S,   ZOE_IPC_CHANNEL_ME2S,   ZOE_IPC_CHANNEL_DMA2S,   ZOE_IPC_CHANNEL_A02S,   ZOE_IPC_CHANNEL_A12S},   // SPU
    {ZOE_IPC_CHANNEL_EK2ED,  ZOE_IPC_CHANNEL_EU2ED,  ZOE_IPC_CHANNEL_HK2ED,  ZOE_IPC_CHANNEL_HU2ED,  ZOE_IPC_CHANNEL_S2ED,  ZOE_IPC_CHANNEL_ED2ED,  ZOE_IPC_CHANNEL_EE2ED,  ZOE_IPC_CHANNEL_ME2ED,  ZOE_IPC_CHANNEL_DMA2ED,  ZOE_IPC_CHANNEL_A02ED,  ZOE_IPC_CHANNEL_A12ED},  // EDPU
    {ZOE_IPC_CHANNEL_EK2EE,  ZOE_IPC_CHANNEL_EU2EE,  ZOE_IPC_CHANNEL_HK2EE,  ZOE_IPC_CHANNEL_HU2EE,  ZOE_IPC_CHANNEL_S2EE,  ZOE_IPC_CHANNEL_ED2EE,  ZOE_IPC_CHANNEL_EE2EE,  ZOE_IPC_CHANNEL_ME2EE,  ZOE_IPC_CHANNEL_DMA2EE,  ZOE_IPC_CHANNEL_A02EE,  ZOE_IPC_CHANNEL_A12EE},  // EEPU
    {ZOE_IPC_CHANNEL_EK2ME,  ZOE_IPC_CHANNEL_EU2ME,  ZOE_IPC_CHANNEL_HK2ME,  ZOE_IPC_CHANNEL_HU2ME,  ZOE_IPC_CHANNEL_S2ME,  ZOE_IPC_CHANNEL_ED2ME,  ZOE_IPC_CHANNEL_EE2ME,  ZOE_IPC_CHANNEL_ME2ME,  ZOE_IPC_CHANNEL_DMA2ME,  ZOE_IPC_CHANNEL_A02ME,  ZOE_IPC_CHANNEL_A12ME},  // MEPU
    {ZOE_IPC_CHANNEL_EK2DMA, ZOE_IPC_CHANNEL_EU2DMA, ZOE_IPC_CHANNEL_HK2DMA, ZOE_IPC_CHANNEL_HU2DMA, ZOE_IPC_CHANNEL_S2DMA, ZOE_IPC_CHANNEL_ED2DMA, ZOE_IPC_CHANNEL_EE2DMA, ZOE_IPC_CHANNEL_ME2DMA, ZOE_IPC_CHANNEL_DMA2DMA, ZOE_IPC_CHANNEL_A02DMA, ZOE_IPC_CHANNEL_A12DMA}, // DMAPU
    {ZOE_IPC_CHANNEL_EK2A0,  ZOE_IPC_CHANNEL_EU2A0,  ZOE_IPC_CHANNEL_HK2A0,  ZOE_IPC_CHANNEL_HU2A0,  ZOE_IPC_CHANNEL_S2A0,  ZOE_IPC_CHANNEL_ED2A0,  ZOE_IPC_CHANNEL_EE2A0,  ZOE_IPC_CHANNEL_ME2A0,  ZOE_IPC_CHANNEL_DMA2A0,  ZOE_IPC_CHANNEL_A02A0,  ZOE_IPC_CHANNEL_A12A0},  // AUD0
    {ZOE_IPC_CHANNEL_EK2A1,  ZOE_IPC_CHANNEL_EU2A1,  ZOE_IPC_CHANNEL_HK2A1,  ZOE_IPC_CHANNEL_HU2A1,  ZOE_IPC_CHANNEL_S2A1,  ZOE_IPC_CHANNEL_ED2A1,  ZOE_IPC_CHANNEL_EE2A1,  ZOE_IPC_CHANNEL_ME2A1,  ZOE_IPC_CHANNEL_DMA2A1,  ZOE_IPC_CHANNEL_A02A1,  ZOE_IPC_CHANNEL_A12A1}   // AUD1
};


static const uint32_t s_txChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] = 
{   
    // [EK]                   [EU]                     [HK]                   [HU]                   [S]                    [ED]                    [EE]                    [ME]                    [DMA]                    [A0]                    [A1]
    {ZOE_IPC_CHANNEL_EK2EK,   ZOE_IPC_CHANNEL_EK2EU,   ZOE_IPC_CHANNEL_EK2HK, ZOE_IPC_CHANNEL_EK2HU, ZOE_IPC_CHANNEL_EK2S,  ZOE_IPC_CHANNEL_EK2ED,  ZOE_IPC_CHANNEL_EK2EE,  ZOE_IPC_CHANNEL_EK2ME,  ZOE_IPC_CHANNEL_EK2DMA,  ZOE_IPC_CHANNEL_EK2A0,  ZOE_IPC_CHANNEL_EK2A1},  // EXT_KER
    {ZOE_IPC_CHANNEL_EU2EK,   ZOE_IPC_CHANNEL_EU2EU,   ZOE_IPC_CHANNEL_EU2HK, ZOE_IPC_CHANNEL_EU2HU, ZOE_IPC_CHANNEL_EU2S,  ZOE_IPC_CHANNEL_EU2ED,  ZOE_IPC_CHANNEL_EU2EE,  ZOE_IPC_CHANNEL_EU2ME,  ZOE_IPC_CHANNEL_EU2DMA,  ZOE_IPC_CHANNEL_EU2A0,  ZOE_IPC_CHANNEL_EU2A1},  // EXT_USR
    {ZOE_IPC_CHANNEL_HK2EK,   ZOE_IPC_CHANNEL_HK2EU,   ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HK2HU, ZOE_IPC_CHANNEL_HK2S,  ZOE_IPC_CHANNEL_HK2ED,  ZOE_IPC_CHANNEL_HK2EE,  ZOE_IPC_CHANNEL_HK2ME,  ZOE_IPC_CHANNEL_HK2DMA,  ZOE_IPC_CHANNEL_HK2A0,  ZOE_IPC_CHANNEL_HK2A1},  // HPU_KER
    {ZOE_IPC_CHANNEL_HU2EK,   ZOE_IPC_CHANNEL_HU2EU,   ZOE_IPC_CHANNEL_HU2HK, ZOE_IPC_CHANNEL_HU2HU, ZOE_IPC_CHANNEL_HU2S,  ZOE_IPC_CHANNEL_HU2ED,  ZOE_IPC_CHANNEL_HU2EE,  ZOE_IPC_CHANNEL_HU2ME,  ZOE_IPC_CHANNEL_HU2DMA,  ZOE_IPC_CHANNEL_HU2A0,  ZOE_IPC_CHANNEL_HU2A1},  // HPU_USR
    {ZOE_IPC_CHANNEL_S2EK,    ZOE_IPC_CHANNEL_S2EU,    ZOE_IPC_CHANNEL_S2HK,  ZOE_IPC_CHANNEL_S2HU,  ZOE_IPC_CHANNEL_S2S,   ZOE_IPC_CHANNEL_S2ED,   ZOE_IPC_CHANNEL_S2EE,   ZOE_IPC_CHANNEL_S2ME,   ZOE_IPC_CHANNEL_S2DMA,   ZOE_IPC_CHANNEL_S2A0,   ZOE_IPC_CHANNEL_S2A1},   // SPU
    {ZOE_IPC_CHANNEL_ED2EK,   ZOE_IPC_CHANNEL_ED2EU,   ZOE_IPC_CHANNEL_ED2HK, ZOE_IPC_CHANNEL_ED2HU, ZOE_IPC_CHANNEL_ED2S,  ZOE_IPC_CHANNEL_ED2ED,  ZOE_IPC_CHANNEL_ED2EE,  ZOE_IPC_CHANNEL_ED2ME,  ZOE_IPC_CHANNEL_ED2DMA,  ZOE_IPC_CHANNEL_ED2A0,  ZOE_IPC_CHANNEL_ED2A1},  // EDPU
    {ZOE_IPC_CHANNEL_EE2EK,   ZOE_IPC_CHANNEL_EE2EU,   ZOE_IPC_CHANNEL_EE2HK, ZOE_IPC_CHANNEL_EE2HU, ZOE_IPC_CHANNEL_EE2S,  ZOE_IPC_CHANNEL_EE2ED,  ZOE_IPC_CHANNEL_EE2EE,  ZOE_IPC_CHANNEL_EE2ME,  ZOE_IPC_CHANNEL_EE2DMA,  ZOE_IPC_CHANNEL_EE2A0,  ZOE_IPC_CHANNEL_EE2A1},  // EEPU
    {ZOE_IPC_CHANNEL_ME2EK,   ZOE_IPC_CHANNEL_ME2EU,   ZOE_IPC_CHANNEL_ME2HK, ZOE_IPC_CHANNEL_ME2HU, ZOE_IPC_CHANNEL_ME2S,  ZOE_IPC_CHANNEL_ME2ED,  ZOE_IPC_CHANNEL_ME2EE,  ZOE_IPC_CHANNEL_ME2ME,  ZOE_IPC_CHANNEL_ME2DMA,  ZOE_IPC_CHANNEL_ME2A0,  ZOE_IPC_CHANNEL_ME2A1},  // MEPU
    {ZOE_IPC_CHANNEL_DMA2EK,  ZOE_IPC_CHANNEL_DMA2EU,  ZOE_IPC_CHANNEL_DMA2HK,ZOE_IPC_CHANNEL_DMA2HU,ZOE_IPC_CHANNEL_DMA2S, ZOE_IPC_CHANNEL_DMA2ED, ZOE_IPC_CHANNEL_DMA2EE, ZOE_IPC_CHANNEL_DMA2ME, ZOE_IPC_CHANNEL_DMA2DMA, ZOE_IPC_CHANNEL_DMA2A0, ZOE_IPC_CHANNEL_DMA2A1}, // DMAPU
    {ZOE_IPC_CHANNEL_A02EK,   ZOE_IPC_CHANNEL_A02EU,   ZOE_IPC_CHANNEL_A02HK, ZOE_IPC_CHANNEL_A02HU, ZOE_IPC_CHANNEL_A02S,  ZOE_IPC_CHANNEL_A02ED,  ZOE_IPC_CHANNEL_A02EE,  ZOE_IPC_CHANNEL_A02ME,  ZOE_IPC_CHANNEL_A02DMA,  ZOE_IPC_CHANNEL_A02A0,  ZOE_IPC_CHANNEL_A02A1},  // AUD0
    {ZOE_IPC_CHANNEL_A12EK,   ZOE_IPC_CHANNEL_A12EU,   ZOE_IPC_CHANNEL_A12HK, ZOE_IPC_CHANNEL_A12HU, ZOE_IPC_CHANNEL_A12S,  ZOE_IPC_CHANNEL_A12ED,  ZOE_IPC_CHANNEL_A12EE,  ZOE_IPC_CHANNEL_A12ME,  ZOE_IPC_CHANNEL_A12DMA,  ZOE_IPC_CHANNEL_A12A0,  ZOE_IPC_CHANNEL_A12A1},  // AUD1   
};

static const char * s_ZoeIPCServiceThreadName[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM] =
{
    {"IPC_RX_EXT_KER",
    "IPC_RX_EXT_USR",
    "IPC_RX_HPU_KER",
    "IPC_RX_HPU_USR",
    "IPC_RX_SPU",
    "IPC_RX_EDPU",
    "IPC_RX_EEPU",
    "IPC_RX_MEPU",
    "IPC_RX_DMAPU",
    "IPC_RX_AUD0",
    "IPC_RX_AUD1"},
    {"IPC_TX_EXT_KER",
    "IPC_TX_EXT_USR",
    "IPC_TX_HPU_KER",
    "IPC_TX_HPU_USR",
    "IPC_TX_SPU",
    "IPC_TX_EDPU",
    "IPC_TX_EEPU",
    "IPC_TX_MEPU",
    "IPC_TX_DMAPU",
    "IPC_TX_AUD0",
    "IPC_TX_AUD1"}
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

// Dawn
//

static const uint32_t s_InterruptTable[ZOE_IPC_CPU_NUM] =
{
    ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL,
    ZOE_SOSAL_DAWN_ISR_SW_EXT_USER,
    ZOE_SOSAL_DAWN_ISR_SW_FWPU,
    ZOE_SOSAL_DAWN_ISR_SW_SPU,
    ZOE_SOSAL_DAWN_ISR_SW_HPU_KERNEL,
    ZOE_SOSAL_DAWN_ISR_SW_HPU_USER,
    ZOE_SOSAL_DAWN_ISR_SW_MEPU
};


static const uint32_t s_Interrupt2CPU[ZOE_SOSAL_ISR_SW_NUM] =
{
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,
    ZOE_IPC_SPU,
    ZOE_IPC_FWPU,
    ZOE_IPC_MEPU,
    ZOE_IPC_EXT_KERNEL,
    ZOE_IPC_EXT_USER
};


static const uint32_t s_rxChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] =
{   // [EK]                 [EU]                   [F]                   [S]                   [HK]                   [HU]                   [M]                
    {ZOE_IPC_CHANNEL_EK2EK, ZOE_IPC_CHANNEL_EU2EK, ZOE_IPC_CHANNEL_F2EK, ZOE_IPC_CHANNEL_S2EK, ZOE_IPC_CHANNEL_HK2EK, ZOE_IPC_CHANNEL_HU2EK, ZOE_IPC_CHANNEL_M2EK}, // Ext CPU Ker
    {ZOE_IPC_CHANNEL_EK2EU, ZOE_IPC_CHANNEL_EU2EU, ZOE_IPC_CHANNEL_F2EU, ZOE_IPC_CHANNEL_S2EU, ZOE_IPC_CHANNEL_HK2EU, ZOE_IPC_CHANNEL_HU2EU, ZOE_IPC_CHANNEL_M2EU}, // Ext CPU Usr
    {ZOE_IPC_CHANNEL_EK2F,  ZOE_IPC_CHANNEL_EU2F,  ZOE_IPC_CHANNEL_F2F,  ZOE_IPC_CHANNEL_S2F,  ZOE_IPC_CHANNEL_HK2F,  ZOE_IPC_CHANNEL_HU2F,  ZOE_IPC_CHANNEL_M2F},  // FW CPU
    {ZOE_IPC_CHANNEL_EK2S,  ZOE_IPC_CHANNEL_EU2S,  ZOE_IPC_CHANNEL_F2S,  ZOE_IPC_CHANNEL_S2S,  ZOE_IPC_CHANNEL_HK2S,  ZOE_IPC_CHANNEL_HU2S,  ZOE_IPC_CHANNEL_M2S},  // SPU
    {ZOE_IPC_CHANNEL_EK2HK, ZOE_IPC_CHANNEL_EU2HK, ZOE_IPC_CHANNEL_F2HK, ZOE_IPC_CHANNEL_S2HK, ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HU2HK, ZOE_IPC_CHANNEL_M2HK}, // App CPU Ker
    {ZOE_IPC_CHANNEL_EK2HU, ZOE_IPC_CHANNEL_EU2HU, ZOE_IPC_CHANNEL_F2HU, ZOE_IPC_CHANNEL_S2HU, ZOE_IPC_CHANNEL_HK2HU, ZOE_IPC_CHANNEL_HU2HU, ZOE_IPC_CHANNEL_M2HU}, // App CPU Usr
    {ZOE_IPC_CHANNEL_EK2M,  ZOE_IPC_CHANNEL_EU2M,  ZOE_IPC_CHANNEL_F2M,  ZOE_IPC_CHANNEL_S2M,  ZOE_IPC_CHANNEL_HK2M,  ZOE_IPC_CHANNEL_HU2M,  ZOE_IPC_CHANNEL_M2M}   // ME CPU
};


static const uint32_t s_txChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] =
{   // [EK]                 [EU]                   [F]                   [S]                   [HK]                   [HU]                   [M]                
    {ZOE_IPC_CHANNEL_EK2EK, ZOE_IPC_CHANNEL_EK2EU, ZOE_IPC_CHANNEL_EK2F, ZOE_IPC_CHANNEL_EK2S, ZOE_IPC_CHANNEL_EK2HK, ZOE_IPC_CHANNEL_EK2HU, ZOE_IPC_CHANNEL_EK2M}, // Ext CPU Ker
    {ZOE_IPC_CHANNEL_EU2EK, ZOE_IPC_CHANNEL_EU2EU, ZOE_IPC_CHANNEL_EU2F, ZOE_IPC_CHANNEL_EU2S, ZOE_IPC_CHANNEL_EU2HK, ZOE_IPC_CHANNEL_EU2HU, ZOE_IPC_CHANNEL_EU2M}, // Ext CPU Usr
    {ZOE_IPC_CHANNEL_F2EK,  ZOE_IPC_CHANNEL_F2EU,  ZOE_IPC_CHANNEL_F2F,  ZOE_IPC_CHANNEL_F2S,  ZOE_IPC_CHANNEL_F2HK,  ZOE_IPC_CHANNEL_F2HU,  ZOE_IPC_CHANNEL_F2M},  // FW CPU
    {ZOE_IPC_CHANNEL_S2EK,  ZOE_IPC_CHANNEL_S2EU,  ZOE_IPC_CHANNEL_S2F,  ZOE_IPC_CHANNEL_S2S,  ZOE_IPC_CHANNEL_S2HK,  ZOE_IPC_CHANNEL_S2HU,  ZOE_IPC_CHANNEL_S2M},  // SPU
    {ZOE_IPC_CHANNEL_HK2EK, ZOE_IPC_CHANNEL_HK2EU, ZOE_IPC_CHANNEL_HK2F, ZOE_IPC_CHANNEL_HK2S, ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HK2HU, ZOE_IPC_CHANNEL_HK2M}, // App CPU Ker
    {ZOE_IPC_CHANNEL_HU2EK, ZOE_IPC_CHANNEL_HU2EU, ZOE_IPC_CHANNEL_HU2F, ZOE_IPC_CHANNEL_HU2S, ZOE_IPC_CHANNEL_HU2HK, ZOE_IPC_CHANNEL_HU2HU, ZOE_IPC_CHANNEL_HU2M}, // App CPU Usr
    {ZOE_IPC_CHANNEL_M2EK,  ZOE_IPC_CHANNEL_M2EU,  ZOE_IPC_CHANNEL_M2F,  ZOE_IPC_CHANNEL_M2S,  ZOE_IPC_CHANNEL_M2HK,  ZOE_IPC_CHANNEL_M2HU,  ZOE_IPC_CHANNEL_M2M}   // ME CPU
};


static const char * s_ZoeIPCServiceThreadName[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM] =
{
    {"IPC_RX_EXT_KER",
    "IPC_RX_EXT_USR",
    "IPC_RX_FWPU",
    "IPC_RX_SPU",
    "IPC_RX_HPU_KER",
    "IPC_RX_HPU_USR",
    "IPC_RX_MEPU"},
    {"IPC_TX_EXT_KER",
    "IPC_TX_EXT_USR",
    "IPC_TX_FWPU",
    "IPC_TX_SPU",
    "IPC_TX_HPU_KER",
    "IPC_TX_HPU_USR",
    "IPC_TX_MEPU"}
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)

// iMX6
//

static const uint32_t s_InterruptTable[ZOE_IPC_CPU_NUM] =
{
    ZOE_SOSAL_MX6_ISR_SW_EXT_KERNEL,
    ZOE_SOSAL_MX6_ISR_SW_EXT_USER,
    ZOE_SOSAL_MX6_ISR_SW_HPU_KERNEL,
    ZOE_SOSAL_MX6_ISR_SW_HPU_USER
};


static const uint32_t s_Interrupt2CPU[ZOE_SOSAL_ISR_SW_NUM] =
{
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,
    ZOE_IPC_EXT_KERNEL,
    ZOE_IPC_EXT_USER
};


static const uint32_t s_rxChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] =
{   // [EK]                 [EU]                   [HK]                   [HU]
    {ZOE_IPC_CHANNEL_EK2EK, ZOE_IPC_CHANNEL_EU2EK, ZOE_IPC_CHANNEL_HK2EK, ZOE_IPC_CHANNEL_HU2EK}, // Ext CPU Ker
    {ZOE_IPC_CHANNEL_EK2EU, ZOE_IPC_CHANNEL_EU2EU, ZOE_IPC_CHANNEL_HK2EU, ZOE_IPC_CHANNEL_HU2EU}, // Ext CPU Usr
    {ZOE_IPC_CHANNEL_EK2HK, ZOE_IPC_CHANNEL_EU2HK, ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HU2HK}, // App CPU Ker
    {ZOE_IPC_CHANNEL_EK2HU, ZOE_IPC_CHANNEL_EU2HU, ZOE_IPC_CHANNEL_HK2HU, ZOE_IPC_CHANNEL_HU2HU}, // App CPU Usr
};


static const uint32_t s_txChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] =
{   // [EK]                 [EU]                   [HK]                   [HU]
    {ZOE_IPC_CHANNEL_EK2EK, ZOE_IPC_CHANNEL_EK2EU, ZOE_IPC_CHANNEL_EK2HK, ZOE_IPC_CHANNEL_EK2HU}, // Ext CPU Ker
    {ZOE_IPC_CHANNEL_EU2EK, ZOE_IPC_CHANNEL_EU2EU, ZOE_IPC_CHANNEL_EU2HK, ZOE_IPC_CHANNEL_EU2HU}, // Ext CPU Usr
    {ZOE_IPC_CHANNEL_HK2EK, ZOE_IPC_CHANNEL_HK2EU, ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HK2HU}, // App CPU Ker
    {ZOE_IPC_CHANNEL_HU2EK, ZOE_IPC_CHANNEL_HU2EU, ZOE_IPC_CHANNEL_HU2HK, ZOE_IPC_CHANNEL_HU2HU}, // App CPU Usr
};


static const char * s_ZoeIPCServiceThreadName[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM] =
{
    {"IPC_RX_EXT_KER",
    "IPC_RX_EXT_USR",
    "IPC_RX_HPU_KER",
    "IPC_RX_HPU_USR"},
    {"IPC_TX_EXT_KER",
    "IPC_TX_EXT_USR",
    "IPC_TX_HPU_KER",
    "IPC_TX_HPU_USR"}
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

// iMX8
//

static const uint32_t s_InterruptTable[ZOE_IPC_CPU_NUM] =
{
	ZOE_SOSAL_MX8_ISR_SW_FWPU,
    ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL,
    ZOE_SOSAL_MX8_ISR_SW_HPU_USER
};


static const uint32_t s_Interrupt2CPU[ZOE_SOSAL_ISR_SW_NUM] =
{
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,
	ZOE_IPC_FWPU
};


static const uint32_t s_rxChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] =
{   // [F]                 [HK]                   [HU]
    {ZOE_IPC_CHANNEL_F2F,  ZOE_IPC_CHANNEL_HK2F,  ZOE_IPC_CHANNEL_HU2F},  // FW CPU
    {ZOE_IPC_CHANNEL_F2HK, ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HU2HK}, // App CPU Ker
    {ZOE_IPC_CHANNEL_F2HU, ZOE_IPC_CHANNEL_HK2HU, ZOE_IPC_CHANNEL_HU2HU}  // App CPU Usr
};


static const uint32_t s_txChannelTable[ZOE_IPC_CPU_NUM][ZOE_IPC_CPU_NUM] =
{   // [F]                 [HK]                   [HU]
    {ZOE_IPC_CHANNEL_F2F,  ZOE_IPC_CHANNEL_F2HK,  ZOE_IPC_CHANNEL_F2HU},  // FW CPU
    {ZOE_IPC_CHANNEL_HK2F, ZOE_IPC_CHANNEL_HK2HK, ZOE_IPC_CHANNEL_HK2HU}, // App CPU Ker
    {ZOE_IPC_CHANNEL_HU2F, ZOE_IPC_CHANNEL_HU2HK, ZOE_IPC_CHANNEL_HU2HU}, // App CPU Usr
};


static const char * s_ZoeIPCServiceThreadName[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM] =
{
	    {"IPC_RX_FWPU",
	    "IPC_RX_HPU_KER",
	    "IPC_RX_HPU_USR"},
	    {"IPC_TX_FWPU",
	    "IPC_TX_HPU_KER",
	    "IPC_TX_HPU_USR"}
};


#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

static uint32_t s_CZoeIPCService_sessionID = 0;

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#define MISC_SCRATCH_MEMORY                         (MISC_XREG_SLV_BASE + MED_MISC_CTRL_MISC_CTRL_SCRATCH_MEMORY)
#else //ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8
#define MISC_SCRATCH_MEMORY                         (XREG_ARB_BASE_PHY + MISC_XREG_SLV_BASE + MED_MISC_CTRL_MISC_CTRL_SCRATCH_MEMORY)
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8

#if (ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS) || (ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS)
#define MBOX_READ(hal, addr, p_data)                {*p_data = *((volatile uint32_t *)addr);}
#define MBOX_WRITE(hal, addr, data)                 {*((volatile uint32_t *)addr) = data;}
#define MBOX_READ_EX(hal, addr, p_data, num_reg)    {memcpy(p_data, (void *)addr, (num_reg) << 2);}
#define MBOX_WRITE_EX(hal, addr, p_data, num_reg)   {memcpy((void *)addr, p_data, (num_reg) << 2);}
#else // ZOE_TARGET_OS != ZOE_TARGET_OS_ZEOS && ZOE_TARGET_OS != ZOE_TARGET_OS_FREERTOS
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#ifdef ZOE_LINUXKER_BUILD
#define MBOX_READ(hal, addr, p_data)                {*p_data = *((volatile uint32_t *)addr);}
#define MBOX_WRITE(hal, addr, data)                 {*((volatile uint32_t *)addr) = data;}
#define MBOX_READ_EX(hal, addr, p_data, num_reg)    {memcpy(p_data, (void *)addr, (num_reg) << 2);}
#define MBOX_WRITE_EX(hal, addr, p_data, num_reg)   {memcpy((void *)addr, p_data, (num_reg) << 2);}
#else // !ZOE_LINUXKER_BUILD
#define MBOX_READ(hal, addr, p_data)                ZOEHAL_MEM_READ(hal, (zoe_dev_mem_t)(addr), (uint32_t *)(p_data), ZOE_FALSE)
#define MBOX_WRITE(hal, addr, data)                 ZOEHAL_MEM_WRITE(hal, (zoe_dev_mem_t)(addr), data, ZOE_FALSE)
#define MBOX_READ_EX(hal, addr, p_data, num_reg)    ZOEHAL_MEM_READ_EX(hal, (zoe_dev_mem_t)(addr), (uint8_t *)(p_data), (num_reg) << 2, ZOE_FALSE)
#define MBOX_WRITE_EX(hal, addr, p_data, num_reg)   ZOEHAL_MEM_WRITE_EX(hal, (zoe_dev_mem_t)(addr), (uint8_t *)(p_data), (num_reg) << 2, ZOE_FALSE)
#endif // ZOE_LINUXKER_BUILD
#else // ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8
#define MBOX_READ(hal, addr, p_data)                ZOEHAL_MEM_READ(hal, (zoe_dev_mem_t)(addr), (uint32_t *)(p_data), ZOE_FALSE)
#define MBOX_WRITE(hal, addr, data)                 ZOEHAL_MEM_WRITE(hal, (zoe_dev_mem_t)(addr), data, ZOE_FALSE)
#define MBOX_READ_EX(hal, addr, p_data, num_reg)    ZOEHAL_MEM_READ_EX(hal, (zoe_dev_mem_t)(addr), (uint8_t *)(p_data), (num_reg) << 2, ZOE_FALSE)
#define MBOX_WRITE_EX(hal, addr, p_data, num_reg)   ZOEHAL_MEM_WRITE_EX(hal, (zoe_dev_mem_t)(addr), (uint8_t *)(p_data), (num_reg) << 2, ZOE_FALSE)
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8
#endif //ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS || ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS


/////////////////////////////////////////////////////////////////////////////
//
//

// custom queue function
//

#ifdef ZOE_DEBUG
static void c_queue_DumpMessageIDs(c_queue *This)
{
    QUEUE_ENTRY         *pthis_queue;
    PZOE_IPC_ENTRY      pMsgEntry;
    PZOE_IPC_MSG_HDR    pMsgHdr;

    ENTER_CRITICAL(&This->m_Object)

    pthis_queue = This->m_Queue.pHead;

    while (pthis_queue)
    {
        pMsgEntry = (PZOE_IPC_ENTRY)pthis_queue->Data;
	    pMsgHdr = &pMsgEntry->msg.hdr;

	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "Srv intf(%d) mod(%d) inst(%d) msg(0x%x) id(%d) cxt(0x%08x)\r\n",
                       ZOE_IPC_MSG_INTF_GET(pMsgHdr->msg),
                       ZOE_IPC_MSG_MOD_GET(pMsgHdr->msg),
                       ZOE_IPC_MSG_INST_GET(pMsgHdr->msg),
                       ZOE_IPC_MSG_MSG_GET(pMsgHdr->msg),
                       ZOE_IPC_STS_ID_GET(pMsgHdr->status),
                       pMsgHdr->context.Reg.context
		               );
        pthis_queue = pthis_queue->pNext;
    }

    LEAVE_CRITICAL(&This->m_Object)
}
#else //!ZOE_DEBUG
#define c_queue_DumpMessageIDs(This)
#endif //ZOE_DEBUG



// return the entry with matching message ID
//
static QUEUE_ENTRY* c_queue_GetEntryByMessageID(c_queue *This, 
                                               uint32_t ID,
                                               uint32_t msg
                                               )
{
    QUEUE_ENTRY         *pthis_queue;
    QUEUE_ENTRY         *pprev_queue;
    PZOE_IPC_ENTRY      pMsgEntry;
    PZOE_IPC_MSG_HDR    pMsgHdr;

    ENTER_CRITICAL(&This->m_Object)

    pthis_queue = This->m_Queue.pHead;
    pprev_queue = ZOE_NULL;

    while (pthis_queue)
    {
        pMsgEntry = (PZOE_IPC_ENTRY)pthis_queue->Data;
	    pMsgHdr = &pMsgEntry->msg.hdr;

        if ((ZOE_IPC_STS_ID_GET(pMsgHdr->status) == ID) &&
            (ZOE_IPC_MESSAGE_REQ(pMsgHdr->msg) == ZOE_IPC_MESSAGE_REQ(msg))
            )
	    {
            if (!pprev_queue)
            {
                This->m_Queue.pHead = pthis_queue->pNext;
                if (!This->m_Queue.pHead)
                {
                    This->m_Queue.pTail = This->m_Queue.pHead;
                }
            }
            else
            {
                pprev_queue->pNext = pthis_queue->pNext;
                if (!pthis_queue->pNext)
                {
                    This->m_Queue.pTail = pprev_queue;
                }
            }
            This->m_dwNbInQueue--;
            break;
        }
        pprev_queue = pthis_queue;
        pthis_queue = pthis_queue->pNext;
    }

    LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}


/////////////////////////////////////////////////////////////////////////////
//
//

#define THREAD_CMD_IPC_INTERFACE    1

// interface thread command handler
//
static void CZoeIPCService_InterfaceDoCommand(c_thread *This, 
                                              THREAD_CMD *pCmd
                                              )
{
    if (pCmd && 
        (THREAD_CMD_IPC_INTERFACE == pCmd->dwCmdCode)
        )
    {
        CZoeIPCService          *This = (CZoeIPCService *)pCmd->pContext;
        uint32_t            cpu_id = pCmd->dwParam[0];
        ZOE_IPC_DISPATCH_FUNC   dispatch_func = (ZOE_IPC_DISPATCH_FUNC)pCmd->pParam[0];
        void                    *dispatch_context = pCmd->pParam[1];
        QUEUE_ENTRY             *pEntry = (QUEUE_ENTRY * )pCmd->pParam[2];
        PZOE_IPC_ENTRY          pMsgEntry;
        PZOE_IPC_MSG            pMsg;

        if (pEntry)
        {
            pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;
            pMsg = &pMsgEntry->msg;

            // call interface dispatch
            //
            if (dispatch_func)
            {
                (*dispatch_func)(dispatch_context, 
                                 (ZOE_IPC_CPU) cpu_id,
                                 pMsg
								 );
            }

            // clear the entry data
            //
            memset(pMsg, 
                   0, 
                   sizeof(ZOE_IPC_MSG)
                   );

            // move to free queue
            //
            c_queue_add_entry(This->m_pFreeQueue,
                              pEntry
                              );
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//

// IPC dispatch helpers
//
static zoe_errs_t CZoeIPCService_UnsolicitCommand(CZoeIPCService *This, 
                                                  ZOE_IPC_CPU cpu_id,
                                                  PZOE_IPC_MSG pMsg
                                                  )
{
    if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status))
    {
        ZOE_IPC_MSG resp;

        // return error
        //
        resp.param[0] = ZOE_ERRS_NOTSUPP;

        // prepare response header
        //
        ZOE_IPC_PREPARE_RESPONSE(&pMsg->hdr, &resp.hdr, 1);
                         
        // post response
        //
        return (c_zoe_ipc_service_post_message(This, 
                                               cpu_id, 
                                               &resp,
                                               ZOE_NULL
                                               ));
    }
    else
    {
        return (ZOE_ERRS_SUCCESS);
    }
}



static zoe_bool_t CZoeIPCService_FindEntry(CZoeIPCService *This,
                                           uint32_t intf,
                                           uint32_t module,
                                           uint32_t inst,
                                           PZOE_IPC_DISPATCHER pDispatcher
                                           )
{
    zoe_bool_t                  bRet = ZOE_FALSE;
    PZOE_IPC_DISPATCH_INTF      pIntf;
    PZOE_IPC_DISPATCH_MODULE    pModule;

    ENTER_CRITICAL(&This->m_Object)

    pIntf = &This->m_IPCDispatchTable[intf];
    pModule = pIntf->pModuleHead;

    while (pModule)
    {
        if (pModule->module == module)
        {
            if (pModule->inst_set[inst])
            {
                pDispatcher->intf = intf;
                pDispatcher->module = module;
                pDispatcher->inst = inst;
                pDispatcher->pContext = pModule->context[inst];
                pDispatcher->dispatch_func = pModule->dispatch_func;
                pDispatcher->pThread = pModule->thread[inst];
                bRet = ZOE_TRUE;
                break;
            }
        }
        // move to the next one
        pModule = pModule->pNext;
    }

    LEAVE_CRITICAL(&This->m_Object)
    return (bRet);
}



static zoe_bool_t CZoeIPCService_AddEntry(CZoeIPCService *This,
                                          PZOE_IPC_DISPATCHER pDispatcher
                                          )
{
    zoe_bool_t                  bRet = ZOE_FALSE;
    PZOE_IPC_DISPATCH_INTF      pIntf;
    PZOE_IPC_DISPATCH_MODULE    pModule, pModulePrev;
    c_thread                    *pThread = ZOE_NULL;
    char                        name[ZOE_SOSAL_OBJNAME_MAXLEN] = {'\0'};

    // validate parameters
    //
    if (!pDispatcher ||
        (pDispatcher->intf >= ZOE_IPC_MAX_INTF) ||
        (pDispatcher->module >= ZOE_IPC_MAX_MODULE) ||
        (pDispatcher->inst >= ZOE_IPC_MAX_INST)
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "Invalid parameter (0x%x), intf(%d) module(%d) inst(%d)\n",
                       pDispatcher,
                       pDispatcher ? pDispatcher->intf : 0,
                       pDispatcher ? pDispatcher->module : 0,
                       pDispatcher ? pDispatcher->inst : 0
		               );
        return (bRet);
    }

    // create thread for this interface
    //
    if (pDispatcher->use_thread)
    {
        pThread = (c_thread *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                     sizeof(c_thread), 
                                                    0
                                                     );
        if (!pThread)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
		                   "zoe_sosal_memory_alloc() failed!\n"
		                   );
            // fail to allocate memory
            //
            return (bRet);
        }

        // thread name
        sprintf(name, 
			    "if_%d_md_%d(%d)",
                pDispatcher->intf,
                pDispatcher->module,
                pDispatcher->inst
			    );

        pDispatcher->pThread = c_thread_constructor(pThread, 
                                                    name,
                                                    (zoe_void_ptr_t)This,
                                                    pDispatcher->priority,
                                                    4096,
                                                    0,
#ifdef ZOE_LINUXKER_BUILD
                                                    -1,//LINUX_KER_THREAD_DEF_TIMEOUT,
#else //!ZOE_LINUXKER_BUILD
                                                    -1,          // no timeout
#endif //ZOE_LINUXKER_BUILD
                                                    ZOE_TRUE,    // remove user space mapping
                                                    This->m_dbgID
                                                    );
		if (!pDispatcher->pThread)
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
		                   "CZoeIPCService_AddEntry c_thread_constructor() FAILED\n"
		                   );
            zoe_sosal_memory_free((void *)pThread);
            return (bRet);
		}

        // override docommand
        pDispatcher->pThread->m_do_command = CZoeIPCService_InterfaceDoCommand;
    }

    ENTER_CRITICAL(&This->m_Object)

    pIntf = &This->m_IPCDispatchTable[pDispatcher->intf];
    pModule = pModulePrev = pIntf->pModuleHead;

    while (pModule)
    {
        if (pDispatcher->module == pModule->module)
        {
            if (pDispatcher->dispatch_func == pModule->dispatch_func)
            {
                if (pModule->inst_set[pDispatcher->inst])
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
		                           "register intf(%d) module(%d) inst(%d) registered already!\n",
                                   pDispatcher->intf,
                                   pDispatcher->module,
                                   pDispatcher->inst
		                           );
                    goto CZoeIPCService_AddEntry_Exit;
                }
                else
                {
                    pModule->inst_set[pDispatcher->inst] = ZOE_TRUE;
                    pModule->context[pDispatcher->inst] = pDispatcher->pContext;
                    pModule->thread[pDispatcher->inst] = pDispatcher->pThread;
                    pModule->inst_num++;
                    bRet = ZOE_TRUE;
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
		                           "register intf(%d) for existing module(%d) inst(%d)\n",
                                   pDispatcher->intf,
                                   pDispatcher->module,
                                   pDispatcher->inst
		                           );
                    break;
                }
            }
            else
            {
                // cannot have same module with different dispatch function
                //
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
		                       "Can not register different dispatch for the same module/intf!\n"
		                       );
                goto CZoeIPCService_AddEntry_Exit;
            }
        }
        // move to the next one
        pModulePrev = pModule;
        pModule = pModule->pNext;
    }

    if (!pModule)
    {
        // add a new module
        //
        pModule = (PZOE_IPC_DISPATCH_MODULE)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                   sizeof(ZOE_IPC_DISPATCH_MODULE), 
                                                                   0
                                                                   );
        if (!pModule)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
		                   "zoe_sosal_memory_alloc() failed!\n"
		                   );
            // fail to allocate memory
            //
            goto CZoeIPCService_AddEntry_Exit;
        }

        // init the entry
        //
        memset(pModule, 
               0, 
               sizeof(ZOE_IPC_DISPATCH_MODULE)
               );
        pModule->module = pDispatcher->module;
        pModule->dispatch_func = pDispatcher->dispatch_func;
        pModule->inst_set[pDispatcher->inst] = ZOE_TRUE;
        pModule->context[pDispatcher->inst] = pDispatcher->pContext;
        pModule->thread[pDispatcher->inst] = pDispatcher->pThread;
        pModule->inst_num = 1;

	    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       This->m_dbgID,
	                   "register %s intf(%d) add module(%d) inst(%d) dispatch(0x%x) cnxt(0x%x)\n",
                       pModulePrev ? "existing" : "new",
                       pDispatcher->intf,
                       pDispatcher->module,
                       pDispatcher->inst,
                       pDispatcher->dispatch_func,
                       pDispatcher->pContext
	                   );
        if (!pModulePrev)
        {
            pIntf->pModuleHead = pModule;
        }
        else
        {
            pModulePrev->pNext = pModule;
        }
    }

    // create the interface thread
    //
    if (pDispatcher->pThread)
    {
        bRet = c_thread_thread_init(pDispatcher->pThread);
    }
    else
    {
        bRet = ZOE_TRUE;
    }

CZoeIPCService_AddEntry_Exit:

    LEAVE_CRITICAL(&This->m_Object)
    if (!bRet && pDispatcher->pThread)
    {
	    c_thread_destructor(pDispatcher->pThread);
        zoe_sosal_memory_free((void *)pDispatcher->pThread);
    }
    return (bRet);
}



static zoe_bool_t CZoeIPCService_RemoveEntry(CZoeIPCService *This,
                                             uint32_t intf,
                                             uint32_t module,
                                             uint32_t inst
                                             )
{
    zoe_bool_t                  bRet = ZOE_FALSE;
    PZOE_IPC_DISPATCH_INTF      pIntf;
    PZOE_IPC_DISPATCH_MODULE    pModule, pModulePrev;

    if ((intf >= ZOE_IPC_MAX_INTF) ||
        (module >= ZOE_IPC_MAX_MODULE) ||
        (inst >= ZOE_IPC_MAX_INST)
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "Invalid parameter intf(%d) module(%d) inst(%d)\n",
                       intf,
                       module,
                       inst
		               );
        return (bRet);
    }

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
	               "unregister intf(%d) module(%d) inst(%d)\n",
                   intf,
                   module,
                   inst
	               );

    ENTER_CRITICAL(&This->m_Object)

    pIntf = &This->m_IPCDispatchTable[intf];
    pModule = pModulePrev = pIntf->pModuleHead;

    while (pModule)
    {
        if (pModule->module == module)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           This->m_dbgID,
	                       "unregister intf(%d) module(%d) inst(%d) succeed\n",
                           intf,
                           module,
                           inst
	                       );
            if (pModule->inst_set[inst])
            {
                if (pModule->thread[inst])
                {
                    c_thread_thread_done(pModule->thread[inst]);
                    c_thread_destructor(pModule->thread[inst]);
                    zoe_sosal_memory_free(pModule->thread[inst]);
                    pModule->thread[inst] = ZOE_NULL;
                }
                pModule->inst_set[inst] = ZOE_FALSE;
                pModule->context[inst] = ZOE_NULL;
                if (pModule->inst_num > 0)
                {
                    pModule->inst_num--;
                }
                bRet = ZOE_TRUE;
            }
            break;
        }
        // move to the next one
        pModulePrev = pModule;
        pModule = pModule->pNext;
    }

    if (bRet &&
        (0 == pModule->inst_num)
        )
    {
        if (pModule != pIntf->pModuleHead)
        {
            pModulePrev->pNext = pModule->pNext;
        }
        else
        {
            pIntf->pModuleHead = pModule->pNext;
        }
        zoe_sosal_memory_free(pModule);
    }

    LEAVE_CRITICAL(&This->m_Object)
    return (bRet);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// interrupt service routine
//
void CZoeIPCService_ISR(void * ctxt, 
                        zoe_sosal_isr_sw_numbers_t from_num
                        )
{
    CZoeIPCService   *This = (CZoeIPCService *)ctxt;

    if (This && (from_num < ZOE_SOSAL_ISR_SW_NUM))
    {
        uint32_t   	rd;
        uint32_t	wr;
        uint32_t    cpu_id;
        uint32_t    rx_ch;
        uint32_t    tx_ch;


        // clear the interrupt
        //
        ZOEHAL_ISR_SW_CLEAR(This->m_pHal,
                            from_num
                            );
        // find the cpu that generates this interrupt 
        //
        cpu_id = s_Interrupt2CPU[from_num];

        // get the rx channel and tx channel in the mailboxes
        //
        rx_ch = This->m_rxChannels[cpu_id];
        tx_ch = This->m_txChannels[cpu_id];

        // check incoming message
        //
        MBOX_READ(This->m_pHal, REG_IPC_MB_HDR_RD(This->m_MBAddrs[rx_ch]), &rd);
        MBOX_READ(This->m_pHal, REG_IPC_MB_HDR_WR(This->m_MBAddrs[rx_ch]), &wr);

        // convert MB to local endianness
        //
        rd = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)rd);
        wr = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)wr);

        // check mailbox empty
        //
        if (ZOE_IPC_MB_CNT_GET(wr) != ZOE_IPC_MB_CNT_GET(rd))
        {
            // trigger rx event
            //
            zoe_sosal_event_set(This->m_EvtMsg[ZOE_IPC_DIR_RX][cpu_id]);
        }

        // check outgoing message
        //
        if (!c_queue_is_empty(This->m_pTxQueue[cpu_id]))
        {
            MBOX_READ(This->m_pHal, REG_IPC_MB_HDR_RD(This->m_MBAddrs[tx_ch]), &rd);
            MBOX_READ(This->m_pHal, REG_IPC_MB_HDR_WR(This->m_MBAddrs[tx_ch]), &wr);

            // convert MB to local endianness
            //
            rd = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)rd);
            wr = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)wr);

            // check mailbox full
            if ((ZOE_IPC_MB_PTR_GET(wr) != ZOE_IPC_MB_PTR_GET(rd)) || 
                (ZOE_IPC_MB_CNT_GET(wr) == ZOE_IPC_MB_CNT_GET(rd))
                )
            {
                // trigger tx event
                //
                zoe_sosal_event_set(This->m_EvtMsg[ZOE_IPC_DIR_TX][cpu_id]);
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//

// tx helper
//
static zoe_bool_t CZoeIPCService_TX(CZoeIPCService *This,
                                    uint32_t cpu_id
                                    )
{
    uint32_t        tx_ch = This->m_txChannels[cpu_id];
    zoe_bool_t      xfered = ZOE_FALSE;
    zoe_bool_t      pending = ZOE_FALSE;
    QUEUE_ENTRY     *pEntry;
    PZOE_IPC_ENTRY  pMsgEntry;
    PZOE_IPC_MSG    pMsg;
    ZOE_IPC_MB_HDR  hdr;
    ZOE_IPC_MSG_HDR msg_hdr;

    while (!c_queue_is_empty(This->m_pTxQueue[cpu_id]))
    {
        pending = ZOE_TRUE;

        // read the mailbox header
        //
        MBOX_READ_EX(This->m_pHal, REG_IPC_MB_BASE(This->m_MBAddrs[tx_ch]), &hdr, 3);

        // convert MB to local endianness
        //
        hdr.open = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)hdr.open);
        hdr.rd = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)hdr.rd);
        hdr.wr = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)hdr.wr);

        // flush the tx queue if the rx channel for the target cpu is not opened
        //
        if (ZOE_IPC_MB_HDR_OPEN_SIGNATURE != hdr.open)
        {
        }

        // break out if mailbox is full
        //
        if ((ZOE_IPC_MB_PTR_GET(hdr.wr) == ZOE_IPC_MB_PTR_GET(hdr.rd)) && 
            (ZOE_IPC_MB_CNT_GET(hdr.wr) != ZOE_IPC_MB_CNT_GET(hdr.rd))
            )
        {
            break;
        }

        // take the request out of the tx queue
        //
        pEntry = c_queue_get_one_entry(This->m_pTxQueue[cpu_id]);
        pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;
        pMsg = &pMsgEntry->msg;

        // convert message header to network endianness, make sure we don't do it in place because the 
        // message is saved later on to match response
        //
        msg_hdr.msg = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)pMsg->hdr.msg);
        msg_hdr.status = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)pMsg->hdr.status);

        // write the message to the mailbox
        //
        MBOX_WRITE_EX(This->m_pHal, REG_IPC_MSG(This->m_MBAddrs[tx_ch], ZOE_IPC_MB_PTR_GET(hdr.wr)), &msg_hdr, 2);
        MBOX_WRITE_EX(This->m_pHal, REG_IPC_CONTEXT1(This->m_MBAddrs[tx_ch], ZOE_IPC_MB_PTR_GET(hdr.wr)), ((char *)pMsg + REG_IPC_CONTEXT1_OFFSET), sizeof(ZOE_IPC_CALLER_CONTEXT) + ZOE_IPC_STS_SIZE_GET(pMsg->hdr.status));

        // move it to either the free queue or the wait queue based on the rsvp flag
        //
        if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status))
        {
            // move the entry to the wait response queue
            //
            c_queue_add_entry(This->m_pWaitRespQueue[cpu_id],
                              pEntry
                              );
        }
        else
        {
            // clear the entry data
            //
            memset(pMsg, 
                   0, 
                   sizeof(ZOE_IPC_MSG)
                   );

            // move the entry back to free queue
            //
            c_queue_add_entry(This->m_pFreeQueue,
                              pEntry
                              );
        }

        // update write pointer and count
        //
        {
            uint32_t    ptr, cnt;
            ptr = ZOE_IPC_MB_PTR_GET(hdr.wr) + 1;
            if (ptr >= ZOE_IPC_MSG_PER_MB)
            {
                ptr = 0;
            }
            cnt = ZOE_IPC_MB_CNT_GET(hdr.wr) + 1;
            if (cnt > ZOE_IPC_MB_CNT_MAX)
            {
                cnt = 0;
            }
            hdr.wr = ZOE_IPC_MB_PTR_SET(ptr) | ZOE_IPC_MB_CNT_SET(cnt);
        }
        hdr.wr = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)hdr.wr);
        MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_WR(This->m_MBAddrs[tx_ch]), hdr.wr);

        // set the xfered flag
        //
        xfered = ZOE_TRUE;
    }

    if (xfered)
    {
#ifdef _NO_ISR_SUPPORT_YET
        // make it happen
        CZoeIPCService_ISR(This, s_InterruptTable[This->m_cpuID]);
        ZOEHAL_ISR_SW_TRIGGER(This->m_pHal,
                              s_InterruptTable[cpu_id],
                              s_InterruptTable[This->m_cpuID]
                              );

#else //!_NO_ISR_SUPPORT_YET
        // interrupt the target cpu
        //
        ZOEHAL_ISR_SW_TRIGGER(This->m_pHal,
                              s_InterruptTable[cpu_id],
                              s_InterruptTable[This->m_cpuID]
                              );
#endif //_NO_ISR_SUPPORT_YET
    }

    return (!pending || xfered);
}



// rx helper
//
static void CZoeIPCService_RX(CZoeIPCService *This,
                              uint32_t cpu_id
                              )
{
    uint32_t        rx_ch = This->m_rxChannels[cpu_id];
    zoe_bool_t      xfered = ZOE_FALSE;
    QUEUE_ENTRY     *pEntry;
    PZOE_IPC_ENTRY  pMsgEntry;
    PZOE_IPC_MSG    pMsg;
    ZOE_IPC_MB_HDR  hdr;

    while (ZOE_TRUE)
    {
        // read the mailbox header
        //
        MBOX_READ_EX(This->m_pHal, REG_IPC_MB_BASE(This->m_MBAddrs[rx_ch]), &hdr, 3);

        // convert MB to local endianness
        //
        hdr.open = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)hdr.open);
        hdr.rd = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)hdr.rd);
        hdr.wr = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)hdr.wr);

        // breakout if mailbox is empty
        //
        if (ZOE_IPC_MB_CNT_GET(hdr.wr) == ZOE_IPC_MB_CNT_GET(hdr.rd))
        {
            break;
        }

        // get a free message entry
        //
        pEntry = c_queue_get_one_entry(This->m_pFreeQueue);
        if (pEntry)
        {
            // save the mailbox content
            //
            pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;
            pMsg = &pMsgEntry->msg;

            // read message header
            MBOX_READ_EX(This->m_pHal, REG_IPC_MSG(This->m_MBAddrs[rx_ch], ZOE_IPC_MB_PTR_GET(hdr.rd)), pMsg, ZOE_IPC_MESSAGE_MSG_HDR_SIZE);

            // convert message header to local endianness
            pMsg->hdr.msg = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pMsg->hdr.msg);
            pMsg->hdr.status = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pMsg->hdr.status);

            // read message parameters
            if (0 != ZOE_IPC_STS_SIZE_GET(pMsg->hdr.status))
            {
                MBOX_READ_EX(This->m_pHal, REG_IPC_PARAM_1(This->m_MBAddrs[rx_ch], ZOE_IPC_MB_PTR_GET(hdr.rd)), &pMsg->param[0], ZOE_IPC_STS_SIZE_GET(pMsg->hdr.status));
            }

            // update read pointer and count
            //
            {
                uint32_t    ptr, cnt;

                ptr = ZOE_IPC_MB_PTR_GET(hdr.rd) + 1;
                if (ptr >= ZOE_IPC_MSG_PER_MB)
                {
                    ptr = 0;
                }
                cnt = ZOE_IPC_MB_CNT_GET(hdr.rd) + 1;
                if (cnt > ZOE_IPC_MB_CNT_MAX)
                {
                    cnt = 0;
                }
                hdr.rd = ZOE_IPC_MB_PTR_SET(ptr) | ZOE_IPC_MB_CNT_SET(cnt);
            }
            hdr.rd = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)hdr.rd);
            MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_RD(This->m_MBAddrs[rx_ch]), hdr.rd);

            // set the xfered flag
            //
            xfered = ZOE_TRUE;

            // put the entry to the rx queue
            //
            c_queue_add_entry(This->m_pRxQueue[cpu_id],
                              pEntry
                              );
        }
        else
        {
            break;
        }
    }

    if (xfered)
    {
#ifdef _NO_ISR_SUPPORT_YET
        // make it happen
        CZoeIPCService_ISR(This, s_InterruptTable[This->m_cpuID]);
        // interrupt the target cpu
        //
        ZOEHAL_ISR_SW_TRIGGER(This->m_pHal,
                              s_InterruptTable[cpu_id],
                              s_InterruptTable[This->m_cpuID]
                              );
#else //!_NO_ISR_SUPPORT_YET
        // interrupt the target cpu
        //
        ZOEHAL_ISR_SW_TRIGGER(This->m_pHal,
                              s_InterruptTable[cpu_id],
                              s_InterruptTable[This->m_cpuID]
                              );
#endif //_NO_ISR_SUPPORT_YET
    }
}


 
/////////////////////////////////////////////////////////////////////////////
//
//

// c_thread
//
static void CZoeIPCService_InitRxEvents(c_thread *This_p, 
                                      zoe_sosal_event_wait_t *pWait
                                      )
{
    PZOE_IPC_SRV_THREAD_CNXT    pCnxt = (PZOE_IPC_SRV_THREAD_CNXT)This_p->m_context;
    CZoeIPCService              *This = pCnxt->pZoeIPCService;
    uint32_t                    cpu_id = pCnxt->cpu_id;

    pWait->event_id = This->m_EvtMsg[ZOE_IPC_DIR_RX][cpu_id];
    pWait->fired = ZOE_FALSE;
}



static void CZoeIPCService_InitTxEvents(c_thread *This_p, 
                                      zoe_sosal_event_wait_t *pWait
                                      )
{
    PZOE_IPC_SRV_THREAD_CNXT    pCnxt = (PZOE_IPC_SRV_THREAD_CNXT)This_p->m_context;
    CZoeIPCService              *This = pCnxt->pZoeIPCService;
    uint32_t                    cpu_id = pCnxt->cpu_id;

    pWait->event_id = This->m_EvtMsg[ZOE_IPC_DIR_TX][cpu_id];
    pWait->fired = ZOE_FALSE;
}



static void CZoeIPCService_HandleRxEvents(c_thread *This_p, 
                                          zoe_sosal_event_wait_t *pWait
                                          )
{
    PZOE_IPC_SRV_THREAD_CNXT    pCnxt = (PZOE_IPC_SRV_THREAD_CNXT)This_p->m_context;
    CZoeIPCService              *This = pCnxt->pZoeIPCService;
    uint32_t                    cpu_id = pCnxt->cpu_id;
    QUEUE_ENTRY                 *pEntry;
    PZOE_IPC_ENTRY              pMsgEntry;
    PZOE_IPC_MSG                pMsg;
    zoe_bool_t                  free_entry;

    // rx thread
    //
    CZoeIPCService_RX(This, cpu_id);

    // walk through the rx queue
    //
    while (ZOE_NULL != (pEntry = c_queue_get_one_entry(This->m_pRxQueue[cpu_id])))
    {
        pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;
        pMsg = &pMsgEntry->msg;

        free_entry = ZOE_TRUE;

        if (0 == ZOE_IPC_STS_REQ_GET(pMsg->hdr.status))
        {
            // response
            //
            if (s_CZoeIPCService_sessionID != ZOE_IPC_STS_SESSION_GET(pMsg->hdr.status))
            {
                // from a stale session
                //
	            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
			                   "RESP from staled session(%d) current(%d) msg id(%d)\n",
                               ZOE_IPC_STS_SESSION_GET(pMsg->hdr.status),
                               s_CZoeIPCService_sessionID,
                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status)
			                   );
            }
            else
            {
                zoe_sosal_obj_id_t	context = pMsg->hdr.context.Reg.context;
                QUEUE_ENTRY     	*pEntryWait;

                // take the matching entry out of the wait response queue
                //
                pEntryWait = c_queue_GetEntryByMessageID(This->m_pWaitRespQueue[cpu_id], 
                                                        ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                                        pMsg->hdr.msg
                                                        );
                if (pEntryWait)
                {
                    pMsgEntry = (PZOE_IPC_ENTRY)pEntryWait->Data;

                    if (pMsgEntry->pOverlapped)
                    {
                        // copy the response to the overlapped structure
                        //
                        ZOE_IPC_OVERLAPPED_CPY(pMsgEntry->pOverlapped, pMsg);

                        // set the event
                        if (context)
                        {
                            if (pMsgEntry->evt != context)
                            {
	                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                               This->m_dbgID,
			                                   "RESP msg_id(%d) evt(0x%08x) != context(%08x)\r\n",
                                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                               pMsgEntry->evt,
                                               context
			                                   );
                            }
                            if (pMsgEntry->msg.hdr.context.Reg.context != context)
                            {
	                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                               This->m_dbgID,
			                                   "RESP msg_id(%d) msg context(0x%08x) != context(%08x)\r\n",
                                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                               pMsgEntry->msg.hdr.context.Reg.context,
                                               context
			                                   );
                            }
                            zoe_sosal_event_set(context);
                        }

                        // clear the overlapped pointer
                        //
                        pMsgEntry->pOverlapped = ZOE_NULL;

                        // clear the entry data
                        //
                        memset((void *)&pMsgEntry->msg, 
                               0, 
                               sizeof(ZOE_IPC_MSG)
                               );
                        // move to free queue
                        //
                        c_queue_add_entry(This->m_pFreeQueue,
                                          pEntryWait
                                          );
                    }
                    else
                    {
                        if (context)
                        {
                            if (pMsgEntry->evt != context)
                            {
	                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                               This->m_dbgID,
			                                   "RESP msg_id(%d) evt(0x%08x) != context(%08x)\r\n",
                                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                               pMsgEntry->evt,
                                               context
			                                   );
                            }
                            if (pMsgEntry->msg.hdr.context.Reg.context != context)
                            {
	                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                               This->m_dbgID,
			                                   "RESP msg_id(%d) msg context(0x%08x) != context(%08x)\r\n",
                                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                               pMsgEntry->msg.hdr.context.Reg.context,
                                               context
			                                   );
                            }
                        }

                        // copy the response
                        //
                        ZOE_IPC_MSG_CPY(&pMsgEntry->msg, pMsg);

                        // move the entry to the response queue
                        //
                        c_queue_add_entry(This->m_pResponseQueue[cpu_id],
                                          pEntryWait
                                          );

                        // set the event
                        if (context)
                        {
                            zoe_sosal_event_set(context);
                        }
                    }
                }
                else
                {
                    // request cancelled
	                zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
			                       "Req ID(%d) cancelled\n",
                                   ZOE_IPC_STS_ID_GET(pMsg->hdr.status)
			                       );
                }
            }
        }
        else
        {
            // request
            //
            ZOE_IPC_DISPATCHER  dispatcher;

            memset(&dispatcher, 
                   0, 
                   sizeof(ZOE_IPC_DISPATCHER)
                   );

            // call the API dispatcher
            //
            if (CZoeIPCService_FindEntry(This, 
                                         ZOE_IPC_MSG_INTF_GET(pMsg->hdr.msg), 
                                         ZOE_IPC_MSG_MOD_GET(pMsg->hdr.msg), 
                                         ZOE_IPC_MSG_INST_GET(pMsg->hdr.msg), 
                                         &dispatcher
                                         ))
            {
                if (dispatcher.pThread)
                {
                    zoe_errs_t  err;
                    THREAD_CMD  cmd;

                    memset(&cmd, 0, sizeof(cmd));

	                cmd.pContext = (zoe_void_ptr_t)This;
	                cmd.dwCmdCode = THREAD_CMD_IPC_INTERFACE;
                    cmd.dwParam[0] = cpu_id; 
	                cmd.pParam[0] = (zoe_void_ptr_t)dispatcher.dispatch_func;
	                cmd.pParam[1] = (zoe_void_ptr_t)dispatcher.pContext;
	                cmd.pParam[2] = (zoe_void_ptr_t)pEntry;

                    err = c_thread_set_command(dispatcher.pThread, 
                                               &cmd
                                               );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                       This->m_dbgID,
			                           "c_thread_set_command() failed(%d) for intf(%d) module(%d) inst(%d)\n",
                                       err,
                                       ZOE_IPC_MSG_INTF_GET(pMsg->hdr.msg), 
                                       ZOE_IPC_MSG_MOD_GET(pMsg->hdr.msg), 
                                       ZOE_IPC_MSG_INST_GET(pMsg->hdr.msg)
			                           );
                        // jump the line
                        (*dispatcher.dispatch_func)(dispatcher.pContext, 
                                                    (ZOE_IPC_CPU) cpu_id,
                                                    pMsg
													);
                    }
                    else
                    {
                        free_entry = ZOE_FALSE;
                    }
                }
                else
                {
                    (*dispatcher.dispatch_func)(dispatcher.pContext, 
                                                (ZOE_IPC_CPU) cpu_id,
                                                pMsg
                                                );
                }
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
			                   "REQ intf(%d) module(%d) inst(%d) not handled!\n",
                               ZOE_IPC_MSG_INTF_GET(pMsg->hdr.msg), 
                               ZOE_IPC_MSG_MOD_GET(pMsg->hdr.msg), 
                               ZOE_IPC_MSG_INST_GET(pMsg->hdr.msg)
			                   );
                CZoeIPCService_UnsolicitCommand(This, 
                                                (ZOE_IPC_CPU) cpu_id,
                                                pMsg
                                                );
            }
        }

        if (free_entry)
        {
            // clear the entry data
            //
            memset(pMsg, 
                   0, 
                   sizeof(ZOE_IPC_MSG)
                   );
            // move to free queue
            //
            c_queue_add_entry(This->m_pFreeQueue,
                              pEntry
                              );
        }
    }
}




static void CZoeIPCService_HandleTxEvents(c_thread *This_p, 
                                          zoe_sosal_event_wait_t *pWait
                                          )
{
    PZOE_IPC_SRV_THREAD_CNXT    pCnxt = (PZOE_IPC_SRV_THREAD_CNXT)This_p->m_context;
    CZoeIPCService              *This = pCnxt->pZoeIPCService;
    uint32_t                    cpu_id = pCnxt->cpu_id;

    // tx thread
    //
    CZoeIPCService_TX(This, cpu_id);
}



static void CZoeIPCService_HandleRxTimeout(c_thread *This_p)
{
    CZoeIPCService_HandleRxEvents(This_p, ZOE_NULL);
}



static void CZoeIPCService_HandleTxTimeout(c_thread *This_p)
{
    CZoeIPCService_HandleTxEvents(This_p, ZOE_NULL);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// CZoeIPCService
//
CZoeIPCService * c_zoe_ipc_service_constructor(CZoeIPCService *pZoeIPCService,
							                   c_object *pParent, 
							                   uint32_t dwAttributes,
                                               IZOEHALAPI *pHal,
                                               zoe_dbg_comp_id_t dbgID,
                                               zoe_sosal_isr_sw_numbers_t isr,
                                               void *device_object // only valid in Linux kernel
							                   )
{
    if (pZoeIPCService)
    {
        zoe_errs_t  err;
        c_thread    *pThread;
        uint32_t    maxpriorities;
        uint32_t    priority;
        c_queue     *pQueue;
        int         i, j;

        // init XDR
        //
        ZOEXDR_init();

        // zero init ourselves
        //
        memset((void *)pZoeIPCService, 
               0, 
               sizeof(CZoeIPCService)
               );

        // c_object
        //
        c_object_constructor(&pZoeIPCService->m_Object, 
                             pParent, 
                             OBJECT_ZOE_IPC_SRV,
                             dwAttributes
                             );

        // c_thread
        //
        maxpriorities = zoe_sosal_thread_maxpriorities_get();
        priority = maxpriorities - 1;

        for (i = 0; i < NUM_ZOE_IPC_DIR; i++)
        {
            for (j = 0; j < ZOE_IPC_CPU_NUM; j++)
            {
                pZoeIPCService->m_ThreadCnxt[i][j].cpu_id = (ZOE_IPC_CPU)j;
                pZoeIPCService->m_ThreadCnxt[i][j].pZoeIPCService = pZoeIPCService;

                pThread = c_thread_constructor(&pZoeIPCService->m_Thread[i][j], 
                                               (char *)s_ZoeIPCServiceThreadName[i][j],
                                               (zoe_void_ptr_t)&pZoeIPCService->m_ThreadCnxt[i][j],
                                               priority,
                                               4096,
                                               1,
#ifdef ZOE_LINUXKER_BUILD
#ifdef CONFIG_HOST_PLATFORM_ARM64
                                               LINUX_KER_THREAD_DEF_TIMEOUT,
#else //!CONFIG_HOST_PLATFORM_ARM64
                                               -1,
#endif //CONFIG_HOST_PLATFORM_ARM64

#else //!ZOE_LINUXKER_BUILD
                                               -1,      // no timeout
#endif //ZOE_LINUXKER_BUILD
                                               ZOE_TRUE, // remove user space mapping
                                               dbgID
                                               );
		        if (!pThread)
		        {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   dbgID,
				                   "c_zoe_ipc_service_constructor c_thread_constructor() FAILED\n"
				                   );
                    c_zoe_ipc_service_destructor(pZoeIPCService);
                    return (ZOE_NULL);
		        }

                pZoeIPCService->m_Thread[i][j].m_init_events = (ZOE_IPC_DIR_TX == i) ? CZoeIPCService_InitTxEvents : CZoeIPCService_InitRxEvents;
                pZoeIPCService->m_Thread[i][j].m_handle_events = (ZOE_IPC_DIR_TX == i) ? CZoeIPCService_HandleTxEvents : CZoeIPCService_HandleRxEvents;
                pZoeIPCService->m_Thread[i][j].m_handle_timeout = (ZOE_IPC_DIR_TX == i) ? CZoeIPCService_HandleTxTimeout : CZoeIPCService_HandleRxTimeout;

                // thread events
                err = zoe_sosal_event_create(ZOE_NULL, 
                                             &pZoeIPCService->m_EvtMsg[i][j]
                                             );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   dbgID,
			                       "c_zoe_ipc_service_constructor zoe_sosal_event_create() FAILED\n"
			                       );
                    c_zoe_ipc_service_destructor(pZoeIPCService);
                    return (ZOE_NULL);
                }
            }
        }

        // CZoeIPCService
        //
        pZoeIPCService->m_pHal = pHal;
        pZoeIPCService->m_dbgID = dbgID;
        pZoeIPCService->m_cpuID = (ZOE_IPC_CPU) s_Interrupt2CPU[isr];
        pZoeIPCService->m_msgID = 0;
        pZoeIPCService->m_device_object = device_object;

        // advance session ID
        //
        s_CZoeIPCService_sessionID++;
        if (s_CZoeIPCService_sessionID >= ZOE_IPC_SESSION_MAX)
        {
            s_CZoeIPCService_sessionID = 0;
        }

        // setup rx/tx mailbox channel mappings
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            pZoeIPCService->m_rxChannels[i] = s_rxChannelTable[pZoeIPCService->m_cpuID][i];
            pZoeIPCService->m_txChannels[i] = s_txChannelTable[pZoeIPCService->m_cpuID][i];
        }

        // setup mailbox location
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
        // for MX8 the mailbox is allocated by the host driver
        //        
        if (ZOE_IPC_HPU_KERNEL == pZoeIPCService->m_cpuID)
        {
            void        *test_ptr = ZOE_NULL;
            uint32_t    mb_checksum, mb;
#ifdef ZOE_LINUXKER_BUILD
            pZoeIPCService->m_mb_space_vir = dma_alloc_coherent(
#if defined(CONFIG_ZV_HPU_EVK) && defined(CONFIG_HOST_PLATFORM_X86_LINUX)
                                                                0,
#else //!CONFIG_ZV_HPU_EVK || !CONFIG_HOST_PLATFORM_X86_LINUX
                                                                (struct device *)pZoeIPCService->m_device_object, 
#endif //CONFIG_ZV_HPU_EVK && CONFIG_HOST_PLATFORM_X86_LINUX
                                                                ZOE_IPC_MB_SIZE_TOTAL, 
                                                                (dma_addr_t *)&pZoeIPCService->m_mb_space_phy,
						                                        GFP_KERNEL | GFP_DMA32
                                                                );
            if (!pZoeIPCService->m_mb_space_vir)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
                               "%s() dma_alloc_coherent failed!!!\n",
                               __FUNCTION__
			                   );

                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }
            // test test
            test_ptr = phys_to_virt(pZoeIPCService->m_mb_space_phy);
#endif //ZOE_LINUXKER_BUILD
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
                           "%s() dma_alloc_coherent phy(%p) vir(%p) test_ptr(%p)\n",
                           __FUNCTION__,
                           (void *)pZoeIPCService->m_mb_space_phy,
                           pZoeIPCService->m_mb_space_vir,
                           test_ptr
			               );
            mb = (uint32_t)pZoeIPCService->m_mb_space_phy;
            mb_checksum = ~mb;
            // save the mailbox location in the misc scratch register
            ZOEHAL_REG_WRITE(pHal, MISC_SCRATCH_MEMORY, mb);
            ZOEHAL_REG_WRITE(pHal, MISC_SCRATCH_MEMORY + 4, mb_checksum);
        }
        else
        {
            uint32_t    mb_checksum, mb;
            // read the mailbox address from the scratch memory
            ZOEHAL_REG_READ(pHal, MISC_SCRATCH_MEMORY, (uint32_t *)&mb);
            ZOEHAL_REG_READ(pHal, MISC_SCRATCH_MEMORY + 4, (uint32_t *)&mb_checksum);
            if (mb == ~mb_checksum)
            {
                pZoeIPCService->m_mb_space_phy = (zoe_dev_mem_t)mb;
#if (ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS) || (ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS)
                pZoeIPCService->m_mb_space_vir = zoe_sosal_memory_get_vir((void *)(zoe_uintptr_t)pZoeIPCService->m_mb_space_phy);
#else // ZOE_TARGET_OS != ZOE_TARGET_OS_ZEOS && ZOE_TARGET_OS != ZOE_TARGET_OS_FREERTOS
#if (ZOE_TARGET_OS == ZOE_TARGET_OS_LINUX_USR)
            // use physical address for mailbox
                pZoeIPCService->m_mb_space_vir = (void *)pZoeIPCService->m_mb_space_phy;
#endif // ZOE_TARGET_OS == ZOE_TARGET_OS_LINUX_USR
#endif // ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS || ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS
            }
            else
            {
                pZoeIPCService->m_mb_space_phy = 0;
                pZoeIPCService->m_mb_space_vir = 0;
            }
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
			               "%s() got mailbox phy(%p) vir(%p) mb(%x) mb_checksum(%x)\n",
                           __FUNCTION__,
                           (void *)pZoeIPCService->m_mb_space_phy,
                           pZoeIPCService->m_mb_space_vir,
                           mb,
                           mb_checksum
			               );
        }

#else // ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8
        // for non MX8 platform the mailbox is on common memory region in the first MB
        //        
        pZoeIPCService->m_mb_space_phy = ZOE_COMMON_IPC_PRIVATE_AREA_START;
#if (ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS) || (ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS)
        pZoeIPCService->m_mb_space_vir = zoe_sosal_memory_get_vir((void *)(zoe_uintptr_t)pZoeIPCService->m_mb_space_phy);
#else // ZOE_TARGET_OS != ZOE_TARGET_OS_ZEOS && ZOE_TARGET_OS != ZOE_TARGET_OS_FREERTOS
        // use physical address for mailbox
        pZoeIPCService->m_mb_space_vir = (void *)pZoeIPCService->m_mb_space_phy;
#endif // ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS || ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS

#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8

        for (i = 0; i < ZOE_IPC_CHANNEL_NUM; i++)
        {
            pZoeIPCService->m_MBAddrs[i] = (zoe_uintptr_t)(pZoeIPCService->m_mb_space_vir + (i * ZOE_IPC_MB_SIZE));
#ifdef IPC_DBG_MAILBOX
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
                           "%s() MB(%d) phys(%p) vir(%p)\n",
                           __FUNCTION__,
                           i,
                           (void *)(pZoeIPCService->m_mb_space_phy + (i * ZOE_IPC_MB_SIZE)),
                           (void *)pZoeIPCService->m_MBAddrs[i]
			               );
#endif //IPC_DBG_MAILBOX
        }

        // close mailboxes
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            MBOX_WRITE(pZoeIPCService->m_pHal, REG_IPC_MB_HDR_OPEN(pZoeIPCService->m_MBAddrs[pZoeIPCService->m_rxChannels[i]]), 0);
        }

        // reset mailboxes
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            MBOX_WRITE(pZoeIPCService->m_pHal, REG_IPC_MB_HDR_RD(pZoeIPCService->m_MBAddrs[pZoeIPCService->m_rxChannels[i]]), 0);
            MBOX_WRITE(pZoeIPCService->m_pHal, REG_IPC_MB_HDR_WR(pZoeIPCService->m_MBAddrs[pZoeIPCService->m_rxChannels[i]]), 0);
        }

        // free queue
        pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                  sizeof(c_queue), 
                                                  0
                                                  );
        if (!pQueue)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
			               "c_zoe_ipc_service_constructor zoe_sosal_memory_alloc() FAILED\n"
			               );
            c_zoe_ipc_service_destructor(pZoeIPCService);
            return (ZOE_NULL);
        }

        pZoeIPCService->m_pFreeQueue = c_queue_constructor(pQueue,
                                                           &pZoeIPCService->m_Object, 
                                                           OBJECT_CRITICAL_HEAVY,
                                                           ZOE_NULL,
                                                           ZOE_NULL,
                                                           dbgID
                                                           );
        if (!pZoeIPCService->m_pFreeQueue)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
			               "c_zoe_ipc_service_constructor c_queue_constructor() FAILED\n"
			               );
            zoe_sosal_memory_free((void *)pQueue);
            c_zoe_ipc_service_destructor(pZoeIPCService);
            return (ZOE_NULL);
        }

        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            // rx queue
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                      sizeof(c_queue), 
                                                      0
                                                      );
            if (!pQueue)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor zoe_sosal_memory_alloc() FAILED\n"
			                   );
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            pZoeIPCService->m_pRxQueue[i] = c_queue_constructor(pQueue,
                                                                &pZoeIPCService->m_Object, 
                                                                OBJECT_CRITICAL_HEAVY,
                                                                ZOE_NULL,
                                                                ZOE_NULL,
                                                                dbgID
                                                                );
            if (!pZoeIPCService->m_pRxQueue[i])
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor c_queue_constructor() FAILED\n"
			                   );
                zoe_sosal_memory_free((void *)pQueue);
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            // tx queue
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                      sizeof(c_queue), 
                                                      0
                                                      );
            if (!pQueue)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor zoe_sosal_memory_alloc() FAILED\n"
			                   );
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            pZoeIPCService->m_pTxQueue[i] = c_queue_constructor(pQueue,
                                                                &pZoeIPCService->m_Object, 
                                                                OBJECT_CRITICAL_HEAVY,
                                                                ZOE_NULL,
                                                                ZOE_NULL,
                                                                dbgID
                                                                );
            if (!pZoeIPCService->m_pTxQueue[i])
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor c_queue_constructor() FAILED\n"
			                   );
                zoe_sosal_memory_free((void *)pQueue);
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            // wait resp queue
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                      sizeof(c_queue), 
                                                      0
                                                      );
            if (!pQueue)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor zoe_sosal_memory_alloc() FAILED\n"
			                   );
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            pZoeIPCService->m_pWaitRespQueue[i] = c_queue_constructor(pQueue,
                                                                      &pZoeIPCService->m_Object, 
                                                                      OBJECT_CRITICAL_HEAVY,
                                                                      ZOE_NULL,
                                                                      ZOE_NULL,
                                                                      dbgID
                                                                      );
            if (!pZoeIPCService->m_pWaitRespQueue[i])
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor c_queue_constructor() FAILED\n"
			                   );
                zoe_sosal_memory_free((void *)pQueue);
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            // resp queue
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                      sizeof(c_queue), 
                                                      0
                                                      );
            if (!pQueue)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor zoe_sosal_memory_alloc() FAILED\n"
			                   );
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }

            pZoeIPCService->m_pResponseQueue[i] = c_queue_constructor(pQueue,
                                                                      &pZoeIPCService->m_Object, 
                                                                      OBJECT_CRITICAL_HEAVY,
                                                                      ZOE_NULL,
                                                                      ZOE_NULL,
                                                                      dbgID
                                                                      );
            if (!pZoeIPCService->m_pResponseQueue[i])
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor c_queue_constructor() FAILED\n"
			                   );
                zoe_sosal_memory_free((void *)pQueue);
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }
        }

        // initialize free queue
        //
	    for (i = 0; i < ZOE_IPC_MAX_MSG_ENTRIES; i++)
	    {
            // create event per entry
            err = zoe_sosal_event_create(ZOE_NULL,
                                         &pZoeIPCService->m_Msgs[i].evt
                                         );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbgID,
			                   "c_zoe_ipc_service_constructor zoe_sosal_event_create(%d) FAILED\n",
                               i
			                   );
                c_zoe_ipc_service_destructor(pZoeIPCService);
                return (ZOE_NULL);
            }
            pZoeIPCService->m_Msgs[i].pOverlapped = ZOE_NULL;
		    pZoeIPCService->m_Entries[i].Data = (zoe_void_ptr_t)&pZoeIPCService->m_Msgs[i];

		    c_queue_add_entry(pZoeIPCService->m_pFreeQueue, 
						      &pZoeIPCService->m_Entries[i]
						      );
	    }

        // create the ipc service threads
        //
        for (i = 0; i < NUM_ZOE_IPC_DIR; i++)
        {
            for (j = 0; j < ZOE_IPC_CPU_NUM; j++)
            {
		        if (!c_thread_thread_init(&pZoeIPCService->m_Thread[i][j]))
		        {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   dbgID,
				                   "c_zoe_ipc_service_constructor c_thread_thread_init(%d:%d) FAILED\n",
                                   i,
                                   j
				                   );
                    c_zoe_ipc_service_destructor(pZoeIPCService);
                    return (ZOE_NULL);
		        }
            }
        }
    }

    return (pZoeIPCService);
}



void c_zoe_ipc_service_destructor(CZoeIPCService *This)
{
    int i, j;

    // close mailboxes
    //
    c_zoe_ipc_service_close(This);

	// destroy the ipc service threads
	//
    for (i = 0; i < NUM_ZOE_IPC_DIR; i++)
    {
        for (j = 0; j < ZOE_IPC_CPU_NUM; j++)
        {
            c_thread_thread_done(&This->m_Thread[i][j]);
        }
    }

    // delete queues
    //
    for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
    {
        if (This->m_pRxQueue[i])
        {
            c_queue_destructor(This->m_pRxQueue[i]);
            zoe_sosal_memory_free(This->m_pRxQueue[i]);
            This->m_pRxQueue[i] = ZOE_NULL;
        }

        if (This->m_pTxQueue[i])
        {
            c_queue_destructor(This->m_pTxQueue[i]);
            zoe_sosal_memory_free(This->m_pTxQueue[i]);
            This->m_pTxQueue[i] = ZOE_NULL;
        }

        if (This->m_pWaitRespQueue[i])
        {
            c_queue_destructor(This->m_pWaitRespQueue[i]);
            zoe_sosal_memory_free(This->m_pWaitRespQueue[i]);
            This->m_pWaitRespQueue[i] = ZOE_NULL;
        }

        if (This->m_pResponseQueue[i])
        {
            c_queue_destructor(This->m_pResponseQueue[i]);
            zoe_sosal_memory_free(This->m_pResponseQueue[i]);
            This->m_pResponseQueue[i] = ZOE_NULL;
        }
    }

    // delete free queue
    //
    if (This->m_pFreeQueue)
    {
        c_queue_destructor(This->m_pFreeQueue);
        zoe_sosal_memory_free(This->m_pFreeQueue);
        This->m_pFreeQueue = ZOE_NULL;
    }

    // delete thread events
    //
    for (i = 0; i < NUM_ZOE_IPC_DIR; i++)
    {
        for (j = 0; j < ZOE_IPC_CPU_NUM; j++)
        {
            if (This->m_EvtMsg[i][j])
            {
                zoe_sosal_event_delete(This->m_EvtMsg[i][j]);
                This->m_EvtMsg[i][j] = ZOE_NULL;
            }
        }
    }

    // delete per message entry event
    //
    for (i = 0; i < ZOE_IPC_MAX_MSG_ENTRIES; i++)
    {
        if (This->m_Msgs[i].evt)
        {
            zoe_sosal_event_delete(This->m_Msgs[i].evt);
            This->m_Msgs[i].evt = ZOE_NULL;
        }
    }

    // delete all dispatch 
    //
    for (i = 0; i < ZOE_IPC_MAX_INTF; i++)
    {
        PZOE_IPC_DISPATCH_INTF      pIntf = &This->m_IPCDispatchTable[i];
        PZOE_IPC_DISPATCH_MODULE    pModule = pIntf->pModuleHead;
        PZOE_IPC_DISPATCH_MODULE    pTemp;        

        while (pModule)
        {
            pTemp = pModule;

            // move to the next one
            pModule = pModule->pNext;

            // free the storage
            zoe_sosal_memory_free(pTemp);
        }
    }

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
    if (ZOE_IPC_HPU_KERNEL == This->m_cpuID)
    {
	    if (This->m_mb_space_vir) 
	    {
#ifdef ZOE_LINUXKER_BUILD
		    dma_free_coherent(0,
						      ZOE_IPC_MB_SIZE_TOTAL, 
						      This->m_mb_space_vir, 
						      This->m_mb_space_phy
						      );
#endif //ZOE_LINUXKER_BUILD
		    This->m_mb_space_vir = ZOE_NULL;
            This->m_mb_space_phy = 0;
        }
    }
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8

    // c_thread
    //
    for (i = 0; i < NUM_ZOE_IPC_DIR; i++)
    {
        for (j = 0; j < ZOE_IPC_CPU_NUM; j++)
        {
	        c_thread_destructor(&This->m_Thread[i][j]);
        }
    }

	// c_object
	//
	c_object_destructor(&This->m_Object);
}



// zoe ipc open
//
zoe_errs_t c_zoe_ipc_service_open(CZoeIPCService *This)
{
    if (This)
    {
        zoe_errs_t  err;
        zoe_bool_t  enableIsr;
        int         i;

        // set the proxy, this function only has effect in the linux user mode
        //
        err = ZOEHAL_SET_PROXY(This->m_pHal);
        if (ZOE_FAIL(err))
        {
            if ((ZOE_ERRS_NOTSUPP != err) &&
                (ZOE_ERRS_NOTIMPL != err)
                )
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
		                       "ZOEHAL_SET_PROXY failed(%d)\n",
                               err
		                       );
                return (err);
            }
        }

        // close mailboxes
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[This->m_rxChannels[i]]), 0);
        }

        // reset mailboxes
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_RD(This->m_MBAddrs[This->m_rxChannels[i]]), 0);
            MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_WR(This->m_MBAddrs[This->m_rxChannels[i]]), 0);
        }

        // register interrupts
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
#ifdef ZOE_IPC_NO_USER_ISR
            if ((ZOE_IPC_EXT_USER == This->m_cpuID) ||
                (ZOE_IPC_HPU_USER == This->m_cpuID) ||
                ((ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER != s_InterruptTable[i]) &&
                (ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER != s_InterruptTable[i]))
                )
            {
#endif //ZOE_IPC_NO_USER_ISR
                err = ZOEHAL_ISR_SW_HANDLER_INSTALL(This->m_pHal,
                                                    s_InterruptTable[i], 
                                                    CZoeIPCService_ISR, 
                                                    (void *)This
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
			                       "c_zoe_ipc_service_open zoe_sosal_isr_handler_install() FAILED(%d)\n",
                                   err
			                       );
                    return (err);
                }
#ifdef ZOE_IPC_NO_USER_ISR
            }
#endif //ZOE_IPC_NO_USER_ISR
        }

        // enable interrupt
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
#ifdef ZOE_IPC_NO_USER_ISR
            if ((ZOE_IPC_EXT_USER == This->m_cpuID) ||
                (ZOE_IPC_HPU_USER == This->m_cpuID) ||
                ((ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER != s_InterruptTable[i]) &&
                (ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER != s_InterruptTable[i]))
                )
            {
#endif //ZOE_IPC_NO_USER_ISR
                enableIsr = ZOE_TRUE;
                err = ZOEHAL_ISR_SW_ENABLE_STATE_SET(This->m_pHal,
                                                     s_InterruptTable[i], 
                                                     &enableIsr
                                                     );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
			                       "c_zoe_ipc_service_open zoe_sosal_isr_state_set() FAILED(%d)\n",
                                   err
			                       );
                    return (err);
                }
#ifdef ZOE_IPC_NO_USER_ISR
            }
#endif //ZOE_IPC_NO_USER_ISR
        }

        // open rx mailboxes
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[This->m_rxChannels[i]]), (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, ZOE_IPC_MB_HDR_OPEN_SIGNATURE));
        }

        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



// zoe ipc close
//
zoe_errs_t c_zoe_ipc_service_close(CZoeIPCService *This)
{
    if (This)
    {
        zoe_errs_t  err;
        zoe_bool_t  enableIsr;
        int         i;

        // disable interrupts
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
#ifdef ZOE_IPC_NO_USER_ISR
            if ((ZOE_IPC_EXT_USER == This->m_cpuID) ||
                (ZOE_IPC_HPU_USER == This->m_cpuID) ||
                ((ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER != s_InterruptTable[i]) &&
                (ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER != s_InterruptTable[i]))
                )
            {
#endif //ZOE_IPC_NO_USER_ISR
                enableIsr = ZOE_FALSE;
                ZOEHAL_ISR_SW_ENABLE_STATE_SET(This->m_pHal,
                                               s_InterruptTable[i], 
                                               &enableIsr
                                               );
#ifdef ZOE_IPC_NO_USER_ISR
            }
#endif //ZOE_IPC_NO_USER_ISR
        }

        // unregister interrupts
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
#ifdef ZOE_IPC_NO_USER_ISR
            if ((ZOE_IPC_EXT_USER == This->m_cpuID) ||
                (ZOE_IPC_HPU_USER == This->m_cpuID) ||
                ((ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER != s_InterruptTable[i]) &&
                (ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER != s_InterruptTable[i]))
                )
            {
#endif //ZOE_IPC_NO_USER_ISR
                ZOEHAL_ISR_SW_HANDLER_INSTALL(This->m_pHal,
                                              s_InterruptTable[i],
                                              ZOE_NULL, 
                                              ZOE_NULL
                                              );
#ifdef ZOE_IPC_NO_USER_ISR
            }
#endif //ZOE_IPC_NO_USER_ISR
        }

        // close mailboxes
        //
        for (i = 0; i < ZOE_IPC_CPU_NUM; i++)
        {
            MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[This->m_rxChannels[i]]), 0);
        }

        // terminate the proxy, this function only has effect in the linux user mode
        //
        err = ZOEHAL_TERM_PROXY(This->m_pHal);
        if (ZOE_FAIL(err))
        {
            if ((ZOE_ERRS_NOTSUPP != err) &&
                (ZOE_ERRS_NOTIMPL != err)
                )
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
		                       "ZOEHAL_TERM_PROXY failed(%d)\n",
                               err
		                       );
            }
        }
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



// zoe ipc service send message to cpu
//
zoe_errs_t c_zoe_ipc_service_send_message(CZoeIPCService *This, 
                                          ZOE_IPC_CPU cpu_id,
                                          PZOE_IPC_MSG pMsg,
                                          PZOE_IPC_MSG pResp,
                                          int32_t milliseconds
                                          )
{
    QUEUE_ENTRY         *pEntry;
    PZOE_IPC_ENTRY      pMsgEntry;
    zoe_sosal_obj_id_t  evt = ZOE_NULL;
    zoe_errs_t          err = ZOE_ERRS_SUCCESS;

    // validate parameters
    //
    if (!pMsg || (cpu_id >= ZOE_IPC_CPU_NUM))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "c_zoe_ipc_service_send_message() pMsg(0x%x) cpu_id(%d) FAILED\n",
                       pMsg,
                       cpu_id
			           );
        return (ZOE_ERRS_PARMS);
    }

    if (!ZOE_IPC_STS_REQ_GET(pMsg->hdr.status) && 
        ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status)
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "c_zoe_ipc_service_send_message() !ZOE_IPC_STS_REQ_GET(%d) ZOE_IPC_STS_RSVP_GET(%d) FAILED\n",
                       ZOE_IPC_STS_REQ_GET(pMsg->hdr.status),
                       ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status)
			           );
        return (ZOE_ERRS_PARMS);
    }

    if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status) && !pResp)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "c_zoe_ipc_service_send_message() ZOE_IPC_STS_RSVP_GET(%d) !pResp(0x%x) FAILED\n",
                       ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status),
                       pResp
			           );
        return (ZOE_ERRS_PARMS);
    }

    // get a free message entry
    //
    pEntry = c_queue_get_one_entry(This->m_pFreeQueue);

    if (pEntry)
    {
        pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;

        if (ZOE_IPC_STS_REQ_GET(pMsg->hdr.status))
        {
            // use the event in the entry if we need response
            //
            if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status))
            {
                // clear the event
                zoe_sosal_event_clear(pMsgEntry->evt);
                // set the event
                evt = pMsg->hdr.context.Reg.context = pMsgEntry->evt;
            }
            else
            {
                pMsg->hdr.context.Reg.context = ZOE_NULL;
            }

            // set message ID
            //
            ENTER_CRITICAL(&This->m_Object)            
            pMsg->hdr.status = ZOE_IPC_STS_ID_CLR(pMsg->hdr.status) | ZOE_IPC_STS_ID_SET(This->m_msgID++);
	        LEAVE_CRITICAL(&This->m_Object)

            // set session ID
            //
            pMsg->hdr.status = ZOE_IPC_STS_SESSION_CLR(pMsg->hdr.status) | ZOE_IPC_STS_SESSION_SET(s_CZoeIPCService_sessionID);
        }

        // clear the overlap pointer
        //
        pMsgEntry->pOverlapped = ZOE_NULL;

        // copy the message
        //
        ZOE_IPC_MSG_CPY(&pMsgEntry->msg, pMsg);

        // move the entry to the tx queue for that CPU
        //
        c_queue_add_entry(This->m_pTxQueue[cpu_id],
                          pEntry
                          );
        // trigger the event
        //
        zoe_sosal_event_set(This->m_EvtMsg[ZOE_IPC_DIR_TX][cpu_id]);
    }
    else
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "c_zoe_ipc_service_send_message() c_queue_get_one_entry() FAILED\n"
			           );
        return (ZOE_ERRS_BUSY);
    }

    if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status))
    {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
        milliseconds *= 8;
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN

        // wait for the response
        //
	    err = zoe_sosal_event_wait(evt, 
                                   milliseconds * 1000
                                   );
        if (ZOE_SUCCESS(err))
        {
            // find the response in the response queue 
            //
            pEntry = c_queue_GetEntryByMessageID(This->m_pResponseQueue[cpu_id], 
                                                ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                                pMsg->hdr.msg
                                                );
            if (pEntry)
            {
                pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;

                // copy the response to the caller buffer
                //
                ZOE_IPC_MSG_CPY(pResp, &pMsgEntry->msg);

                // clear the entry message
                //
                memset(&pMsgEntry->msg, 
                       0, 
                       sizeof(ZOE_IPC_MSG)
                       );

                // move it back to the free queue
                //
                c_queue_add_entry(This->m_pFreeQueue,
                                  pEntry
                                  );
            }
            else
            {
                err = ZOE_ERRS_NOTFOUND;

	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zoe_ipc_service_send_message() c_queue_GetEntryByMessageID() msg_id(%d) FAILED\n",
                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status)
			                   );
                c_queue_DumpMessageIDs(This->m_pResponseQueue[cpu_id]);
            }
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
			               "zoe_sosal_event_wait FAILED(%d) msg_id(%d) evt(0x%016llx) cxt(0x%016llx) entry(0x%016llx)\r\n",
                           err,
                           ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                           evt,
                           pMsg->hdr.context.Reg.context,
                           pMsgEntry->evt
			               );
            // wait failed, find this entry in queues and recycle it
            //
            pEntry = c_queue_GetEntryByMessageID(This->m_pTxQueue[cpu_id], 
                                                ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                                pMsg->hdr.msg
                                                );
            if (!pEntry)
            {
                pEntry = c_queue_GetEntryByMessageID(This->m_pWaitRespQueue[cpu_id], 
                                                    ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                                    pMsg->hdr.msg
                                                    );
            }
            if (!pEntry)
            {
                pEntry = c_queue_GetEntryByMessageID(This->m_pResponseQueue[cpu_id], 
                                                    ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                                                    pMsg->hdr.msg
                                                    );
            }

            if (pEntry)
            {
                pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;

                // clear the entry data
                //
                memset((void *)&pMsgEntry->msg, 
                       0, 
                       sizeof(ZOE_IPC_MSG)
                       );

                // move it back to the free queue
                //
                c_queue_add_entry(This->m_pFreeQueue,
                                  pEntry
                                  );
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "wait (%x) failed(%d) (%d:%x) and entry not found!!!\n",
                               evt, 
                               err, 
                               ZOE_IPC_STS_ID_GET(pMsg->hdr.status),
                               ZOE_IPC_MSG_MSG_GET(pMsg->hdr.msg)
			                   );
            }
        }
    }

    return (err);
}



// zoe ipc service post message to cpu
//
zoe_errs_t c_zoe_ipc_service_post_message(CZoeIPCService *This, 
                                          ZOE_IPC_CPU cpu_id,
                                          PZOE_IPC_MSG pMsg,
                                          PZOE_IPC_OVERLAPPED pOverlapped
                                          )
{
    QUEUE_ENTRY     *pEntry;
    PZOE_IPC_ENTRY  pMsgEntry;
    zoe_errs_t      err = ZOE_ERRS_SUCCESS;

    // validate parameters
    //
    if (!pMsg || (cpu_id >= ZOE_IPC_CPU_NUM))
    {
        return (ZOE_ERRS_PARMS);
    }

    if (!ZOE_IPC_STS_REQ_GET(pMsg->hdr.status) && 
        ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status)
        )
    {
        return (ZOE_ERRS_PARMS);
    }

    if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status) && !pOverlapped)
    {
        return (ZOE_ERRS_PARMS);
    }

    // get a free message entry
    //
    pEntry = c_queue_get_one_entry(This->m_pFreeQueue);

    if (pEntry)
    {
        pMsgEntry = (PZOE_IPC_ENTRY)pEntry->Data;

        if (ZOE_IPC_STS_REQ_GET(pMsg->hdr.status))
        {
            // use the event in the user supplied overlapped structure if we need response
            //
            if (ZOE_IPC_STS_RSVP_GET(pMsg->hdr.status))
            {
                // clear the event
                zoe_sosal_event_clear(pOverlapped->hEvent);
                // save the event
                pMsg->hdr.context.Reg.context = pOverlapped->hEvent;
                // save the user supplied overlapped structure
                pMsgEntry->pOverlapped = pOverlapped;
            }
            else
            {
                pMsg->hdr.context.Reg.context = ZOE_NULL;
                pMsgEntry->pOverlapped = ZOE_NULL;
            }

            // set message ID
            //
            ENTER_CRITICAL(&This->m_Object)            
            pMsg->hdr.status = ZOE_IPC_STS_ID_CLR(pMsg->hdr.status) | ZOE_IPC_STS_ID_SET(This->m_msgID++);
	        LEAVE_CRITICAL(&This->m_Object)

            // set session ID
            //
            pMsg->hdr.status = ZOE_IPC_STS_SESSION_CLR(pMsg->hdr.status) | ZOE_IPC_STS_SESSION_SET(s_CZoeIPCService_sessionID);
        }

        // copy the message
        //
        ZOE_IPC_MSG_CPY(&pMsgEntry->msg, pMsg);
        
        // move the entry to the tx queue for that CPU
        //
        c_queue_add_entry(This->m_pTxQueue[cpu_id],
                          pEntry
                          );
        // trigger the event
        //
        zoe_sosal_event_set(This->m_EvtMsg[ZOE_IPC_DIR_TX][cpu_id]);
    }
    else
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "c_zoe_ipc_service_post_message() c_queue_get_one_entry() FAILED\n"
			           );

        c_zoe_ipc_service_debug_queues(This, 
                                       cpu_id
                                       );
        err = ZOE_ERRS_BUSY;
    }

    return (err);
}



// zoe ipc register interface
//
zoe_errs_t c_zoe_ipc_service_register_interface(CZoeIPCService *This, 
                                                uint32_t intf,
                                                uint32_t module,
                                                uint32_t inst,
                                                void *pContext,
                                                ZOE_IPC_DISPATCH_FUNC dispatch_func,
                                                zoe_bool_t use_thread,
                                                uint32_t priority
                                                )
{

    ZOE_IPC_DISPATCHER  dispatcher;
    
    if (!dispatch_func)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "Invalid parameter dispatch_func(0x%x)\n",
                       dispatch_func
		               );
        return (ZOE_ERRS_PARMS);
    }
    dispatcher.intf = intf;
    dispatcher.module = module;
    dispatcher.inst = inst;
    dispatcher.pContext = pContext;
    dispatcher.dispatch_func = dispatch_func;
    dispatcher.pThread = ZOE_NULL;
    dispatcher.use_thread = use_thread;
    dispatcher.priority = priority;

    if (CZoeIPCService_AddEntry(This, 
                                &dispatcher
                                ))
    {
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_INVALID);
    }
}



// zoe ipc unregister interface
//
zoe_errs_t c_zoe_ipc_service_unregister_interface(CZoeIPCService *This, 
                                                  uint32_t intf,
                                                  uint32_t module,
                                                  uint32_t inst
                                                  )
{
    if (CZoeIPCService_RemoveEntry(This, intf, module, inst))
    {
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}




// zoe ipc debug queues
//
void c_zoe_ipc_service_debug_queues(CZoeIPCService *This, 
                                    ZOE_IPC_CPU cpu_id
                                    )
{
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
	               "CPU id(%d)\r\n",
                   cpu_id
                   );

    // free queue
    //
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
	               "FREE => %d\r\n",
                   c_queue_get_queue_level(This->m_pFreeQueue)
	               );

    if ((cpu_id < ZOE_IPC_CPU_NUM) && 
        (cpu_id >= 0)
        )
    {
        // rx queue
        //
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "RX => %d\r\n",
                       c_queue_get_queue_level(This->m_pRxQueue[cpu_id])
                       );
        c_queue_DumpMessageIDs(This->m_pRxQueue[cpu_id]);

        // tx queue
        //
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "TX => %d\r\n",
                       c_queue_get_queue_level(This->m_pTxQueue[cpu_id])
                       );
        c_queue_DumpMessageIDs(This->m_pTxQueue[cpu_id]);

        // wait resp queue
        //
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "WAIT => %d\r\n",
                       c_queue_get_queue_level(This->m_pWaitRespQueue[cpu_id])
                       );
        c_queue_DumpMessageIDs(This->m_pWaitRespQueue[cpu_id]);

        // resp queue
        //
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "RESP => %d\r\n",
                       c_queue_get_queue_level(This->m_pResponseQueue[cpu_id])
                       );
        c_queue_DumpMessageIDs(This->m_pResponseQueue[cpu_id]);
    }
}


// get CPU id from interrupt number
//
ZOE_IPC_CPU c_zoe_ipc_service_get_cpu_from_interrupt(zoe_sosal_isr_sw_numbers_t isr)
{
    return ((ZOE_IPC_CPU) s_Interrupt2CPU[isr]);
}



// get per instance data for interface
//
void * c_zoe_ipc_service_get_interface_context(CZoeIPCService *This,
                                               uint32_t intf,
                                               uint32_t module,
                                               uint32_t inst
                                               )
{
    PZOE_IPC_DISPATCH_INTF      pIntf;
    PZOE_IPC_DISPATCH_MODULE    pModule;
    void                        *pContext = ZOE_NULL;

    ENTER_CRITICAL(&This->m_Object)

    pIntf = &This->m_IPCDispatchTable[intf];
    pModule = pIntf->pModuleHead;

    while (pModule)
    {
        if (pModule->module == module)
        {
            if (pModule->inst_set[inst])
            {
                pContext = pModule->context[inst];
                break;
            }
        }
        // move to the next one
        pModule = pModule->pNext;
    }

    LEAVE_CRITICAL(&This->m_Object)
    return (pContext);
}



// check target CPU mailbox open
//
zoe_bool_t c_zoe_ipc_service_target_opened(CZoeIPCService *This, 
                                           uint32_t cpu_id
                                           )
{
    uint32_t    tx_ch = 0;
    uint32_t    _open = 0;

    if (cpu_id < ZOE_IPC_CPU_NUM)
    {
        tx_ch = This->m_txChannels[cpu_id];
    }

    if ((cpu_id >= ZOE_IPC_CPU_NUM) ||
        (tx_ch >= ZOE_IPC_CHANNEL_NUM)
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "%s() cpu_id(%d) tx_ch(%d)\r\n",
                       __FUNCTION__,
                       cpu_id,
                       tx_ch
	                   );
        return (ZOE_FALSE);
    }
    else
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "%s() MBOX_READ mb_size(%d) tx_ch(%d) (%x) [\n",
                       __FUNCTION__,
                       ZOE_IPC_MB_SIZE,
                       tx_ch,
                       REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[tx_ch])
	                   );
        MBOX_READ(This->m_pHal, REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[tx_ch]), &_open);
        _open = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, _open);
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "%s() MBOX_READ(%x) _open(%x)]\n",
                       __FUNCTION__,
                       REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[tx_ch]),
                       _open
	                   );
        return (_open == ZOE_IPC_MB_HDR_OPEN_SIGNATURE);
    }
}



// clear target CPU mailbox open
//
void c_zoe_ipc_service_target_open_clr(CZoeIPCService *This, 
                                       uint32_t cpu_id
                                       )
{
    uint32_t    tx_ch = 0;

    if (cpu_id < ZOE_IPC_CPU_NUM)
    {
        tx_ch = This->m_txChannels[cpu_id];
    }

    if ((cpu_id >= ZOE_IPC_CPU_NUM) ||
        (tx_ch >= ZOE_IPC_CHANNEL_NUM)
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
	                   "%s() cpu_id(%d) tx_ch(%d)\r\n",
                       __FUNCTION__,
                       cpu_id,
                       tx_ch
	                   );
    }
    else
    {
        MBOX_WRITE(This->m_pHal, REG_IPC_MB_HDR_OPEN(This->m_MBAddrs[tx_ch]), 0);
    }
}






