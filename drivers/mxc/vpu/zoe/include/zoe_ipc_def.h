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
// zoe_ipc_def.h
//
// Description: 
//
//  ZOE ipc common definition
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_IPC_DEF_H__
#define __ZOE_IPC_DEF_H__


#include "zoe_types.h"
#include "zoe_sosal.h"
#include "zoe_sosal_priv.h"
#include "zoe_xreg.h"

#ifdef __cplusplus
extern "C" {
#endif


#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

/////////////////////////////////////////////////////////////////////////////
//
//

// there are nine CPU clusters, so the total number of channels are 110 + 11
// (N) * (N - 1) + N loop back channels
//
// 0: [EK] external CPU kernel
// 1: [EU] external CPU user
// 2: [HK] application CPU kernel
// 3: [HU] application CPU user
// 4: [S] security CPU
// 5: [ED] EDPU
// 6: [EE] EEPU
// 7: [ME] MEPU
// 8: [DMA] DMAPU
// 9: [A0] audio CPU(0)
// 10: [A1] audio CPU(1)
//
typedef enum _ZOE_IPC_CPU
{
    ZOE_IPC_NULL_CPU = -1,
    ZOE_IPC_EXT_KERNEL = 0,
    ZOE_IPC_EXT_USER,
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,
    ZOE_IPC_SPU,
    ZOE_IPC_EDPU,
    ZOE_IPC_EEPU,
    ZOE_IPC_MEPU,
    ZOE_IPC_DMAPU,
    ZOE_IPC_AUD0_CPU,
    ZOE_IPC_AUD1_CPU,

    ZOE_IPC_CPU_END

} ZOE_IPC_CPU, *PZOE_IPC_CPU;



typedef enum _ZOE_IPC_CHANNEL
{
    ZOE_IPC_CHANNEL_NULL = -1,

    ZOE_IPC_CHANNEL_EK2EK = 0,  // EK2EK loopback
    ZOE_IPC_CHANNEL_EU2EK,      // EU2EK
    ZOE_IPC_CHANNEL_HK2EK,      // HK2EK
    ZOE_IPC_CHANNEL_HU2EK,      // HU2EK
    ZOE_IPC_CHANNEL_S2EK,       // S2EK
    ZOE_IPC_CHANNEL_ED2EK,      // ED2EK
    ZOE_IPC_CHANNEL_EE2EK,      // EE2EK
    ZOE_IPC_CHANNEL_ME2EK,      // ME2EK
    ZOE_IPC_CHANNEL_DMA2EK,     // DMA2EK
    ZOE_IPC_CHANNEL_A02EK,      // A02EK
    ZOE_IPC_CHANNEL_A12EK,      // A12EK

    ZOE_IPC_CHANNEL_EK2EU,      // EK2EU
    ZOE_IPC_CHANNEL_EU2EU,      // EU2EU loopback 
    ZOE_IPC_CHANNEL_HK2EU,      // HK2EU
    ZOE_IPC_CHANNEL_HU2EU,      // HU2EU
    ZOE_IPC_CHANNEL_S2EU,       // S2EU
    ZOE_IPC_CHANNEL_ED2EU,      // ED2EU
    ZOE_IPC_CHANNEL_EE2EU,      // EE2EU
    ZOE_IPC_CHANNEL_ME2EU,      // ME2EU
    ZOE_IPC_CHANNEL_DMA2EU,     // DMA2EU
    ZOE_IPC_CHANNEL_A02EU,      // A02EU
    ZOE_IPC_CHANNEL_A12EU,      // A12EU

    ZOE_IPC_CHANNEL_EK2HK,      // EK2HK
    ZOE_IPC_CHANNEL_EU2HK,      // EU2HK
    ZOE_IPC_CHANNEL_HK2HK,      // HK2HK loopback
    ZOE_IPC_CHANNEL_HU2HK,      // HU2HK
    ZOE_IPC_CHANNEL_S2HK,       // S2HK
    ZOE_IPC_CHANNEL_ED2HK,      // ED2HK
    ZOE_IPC_CHANNEL_EE2HK,      // EE2HK
    ZOE_IPC_CHANNEL_ME2HK,      // ME2HK
    ZOE_IPC_CHANNEL_DMA2HK,     // DMA2HK
    ZOE_IPC_CHANNEL_A02HK,      // A02HK
    ZOE_IPC_CHANNEL_A12HK,      // A12HK

    ZOE_IPC_CHANNEL_EK2HU,      // EK2HU
    ZOE_IPC_CHANNEL_EU2HU,      // EU2HU
    ZOE_IPC_CHANNEL_HK2HU,      // HK2HU
    ZOE_IPC_CHANNEL_HU2HU,      // HU2HU loopback
    ZOE_IPC_CHANNEL_S2HU,       // S2HU
    ZOE_IPC_CHANNEL_ED2HU,      // ED2HU
    ZOE_IPC_CHANNEL_EE2HU,      // EE2HU
    ZOE_IPC_CHANNEL_ME2HU,      // ME2HU
    ZOE_IPC_CHANNEL_DMA2HU,     // DMA2HU
    ZOE_IPC_CHANNEL_A02HU,      // A02HU
    ZOE_IPC_CHANNEL_A12HU,      // A12HU

    ZOE_IPC_CHANNEL_EK2S,       // EK2S
    ZOE_IPC_CHANNEL_EU2S,       // EU2S
    ZOE_IPC_CHANNEL_HK2S,       // HK2S
    ZOE_IPC_CHANNEL_HU2S,       // HU2S
    ZOE_IPC_CHANNEL_S2S,        // S2S loopback
    ZOE_IPC_CHANNEL_ED2S,       // ED2S
    ZOE_IPC_CHANNEL_EE2S,       // EE2S
    ZOE_IPC_CHANNEL_ME2S,       // ME2S
    ZOE_IPC_CHANNEL_DMA2S,      // DMA2S
    ZOE_IPC_CHANNEL_A02S,       // A02S
    ZOE_IPC_CHANNEL_A12S,       // A12S

    ZOE_IPC_CHANNEL_EK2ED,      // EK2ED
    ZOE_IPC_CHANNEL_EU2ED,      // EU2ED
    ZOE_IPC_CHANNEL_HK2ED,      // HK2ED
    ZOE_IPC_CHANNEL_HU2ED,      // HU2ED
    ZOE_IPC_CHANNEL_S2ED,       // S2ED
    ZOE_IPC_CHANNEL_ED2ED,      // ED2ED loopback
    ZOE_IPC_CHANNEL_EE2ED,      // EE2ED
    ZOE_IPC_CHANNEL_ME2ED,      // ME2ED
    ZOE_IPC_CHANNEL_DMA2ED,     // DMA2ED
    ZOE_IPC_CHANNEL_A02ED,      // A02ED
    ZOE_IPC_CHANNEL_A12ED,      // A12ED

    ZOE_IPC_CHANNEL_EK2EE,      // EK2EE
    ZOE_IPC_CHANNEL_EU2EE,      // EU2EE
    ZOE_IPC_CHANNEL_HK2EE,      // HK2EE
    ZOE_IPC_CHANNEL_HU2EE,      // HU2EE
    ZOE_IPC_CHANNEL_S2EE,       // S2EE
    ZOE_IPC_CHANNEL_ED2EE,      // ED2EE
    ZOE_IPC_CHANNEL_EE2EE,      // EE2EE loopback
    ZOE_IPC_CHANNEL_ME2EE,      // ME2EE
    ZOE_IPC_CHANNEL_DMA2EE,     // DMA2EE
    ZOE_IPC_CHANNEL_A02EE,      // A02EE
    ZOE_IPC_CHANNEL_A12EE,      // A12EE

    ZOE_IPC_CHANNEL_EK2ME,      // EK2ME
    ZOE_IPC_CHANNEL_EU2ME,      // EU2ME
    ZOE_IPC_CHANNEL_HK2ME,      // HK2ME
    ZOE_IPC_CHANNEL_HU2ME,      // HU2ME
    ZOE_IPC_CHANNEL_S2ME,       // S2ME
    ZOE_IPC_CHANNEL_ED2ME,      // ED2ME
    ZOE_IPC_CHANNEL_EE2ME,      // EE2ME
    ZOE_IPC_CHANNEL_ME2ME,      // ME2ME loopback
    ZOE_IPC_CHANNEL_DMA2ME,     // DMA2ME
    ZOE_IPC_CHANNEL_A02ME,      // A02ME
    ZOE_IPC_CHANNEL_A12ME,      // A12ME

    ZOE_IPC_CHANNEL_EK2DMA,     // EK2DMA
    ZOE_IPC_CHANNEL_EU2DMA,     // EU2DMA
    ZOE_IPC_CHANNEL_HK2DMA,     // HK2DMA
    ZOE_IPC_CHANNEL_HU2DMA,     // HU2DMA
    ZOE_IPC_CHANNEL_S2DMA,      // S2DMA
    ZOE_IPC_CHANNEL_ED2DMA,     // ED2DMA
    ZOE_IPC_CHANNEL_EE2DMA,     // EE2DMA
    ZOE_IPC_CHANNEL_ME2DMA,     // ME2DMA
    ZOE_IPC_CHANNEL_DMA2DMA,    // DMA2DMA loopback
    ZOE_IPC_CHANNEL_A02DMA,     // A02DMA
    ZOE_IPC_CHANNEL_A12DMA,     // A12DMA

    ZOE_IPC_CHANNEL_EK2A0,      // EK2A0
    ZOE_IPC_CHANNEL_EU2A0,      // EU2A0
    ZOE_IPC_CHANNEL_HK2A0,      // HK2A0
    ZOE_IPC_CHANNEL_HU2A0,      // HU2A0
    ZOE_IPC_CHANNEL_S2A0,       // S2A0
    ZOE_IPC_CHANNEL_ED2A0,      // ED2A0
    ZOE_IPC_CHANNEL_EE2A0,      // EE2A0
    ZOE_IPC_CHANNEL_ME2A0,      // ME2A0
    ZOE_IPC_CHANNEL_DMA2A0,     // DMA2A0
    ZOE_IPC_CHANNEL_A02A0,      // A02A0 loopback
    ZOE_IPC_CHANNEL_A12A0,      // A12A0

    ZOE_IPC_CHANNEL_EK2A1,      // EK2A1
    ZOE_IPC_CHANNEL_EU2A1,      // EU2A1
    ZOE_IPC_CHANNEL_HK2A1,      // HK2A1
    ZOE_IPC_CHANNEL_HU2A1,      // HU2A1
    ZOE_IPC_CHANNEL_S2A1,       // S2A1
    ZOE_IPC_CHANNEL_ED2A1,      // ED2A1
    ZOE_IPC_CHANNEL_EE2A1,      // EE2A1
    ZOE_IPC_CHANNEL_ME2A1,      // ME2A1
    ZOE_IPC_CHANNEL_DMA2A1,     // DMA2A1
    ZOE_IPC_CHANNEL_A02A1,      // A02A1
    ZOE_IPC_CHANNEL_A12A1,      // A12A1 llopback

    ZOE_IPC_CHANNEL_END

} ZOE_IPC_CHANNEL, *PZOE_IPC_CHANNEL;


#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

/////////////////////////////////////////////////////////////////////////////
//
//

// there are 7 CPU clusters, so the total number of channels are 42 + 7
// (N) * (N - 1) + N loop back channels
//
// 0: [EK] external CPU kernel
// 1: [EU] external CPU user
// 2: [F] firmware CPU
// 3: [S] security CPU
// 4: [HK] application CPU kernel
// 5: [HU] application CPU user
// 6: [M] mepu
//
typedef enum _ZOE_IPC_CPU
{
    ZOE_IPC_NULL_CPU = -1,
    ZOE_IPC_EXT_KERNEL = 0,
    ZOE_IPC_EXT_USER,
    ZOE_IPC_FWPU,
    ZOE_IPC_SPU,
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,
    ZOE_IPC_MEPU,

    ZOE_IPC_CPU_END

} ZOE_IPC_CPU, *PZOE_IPC_CPU;


typedef enum _ZOE_IPC_CHANNEL
{
    ZOE_IPC_CHANNEL_NULL = -1,

    ZOE_IPC_CHANNEL_EK2EK = 0,  // EK2EK loopback
    ZOE_IPC_CHANNEL_EU2EK,      // E2EK
    ZOE_IPC_CHANNEL_F2EK,       // F2EK
    ZOE_IPC_CHANNEL_S2EK,       // S2EK
    ZOE_IPC_CHANNEL_HK2EK,      // H2EK
    ZOE_IPC_CHANNEL_HU2EK,      // H2EK
    ZOE_IPC_CHANNEL_M2EK,       // M2EK

    ZOE_IPC_CHANNEL_EK2EU,      // EK2EU
    ZOE_IPC_CHANNEL_EU2EU,      // EU2EU loopback
    ZOE_IPC_CHANNEL_F2EU,       // F2EU
    ZOE_IPC_CHANNEL_S2EU,       // S2EU
    ZOE_IPC_CHANNEL_HK2EU,      // H2EU
    ZOE_IPC_CHANNEL_HU2EU,      // H2EU
    ZOE_IPC_CHANNEL_M2EU,       // M2EU

    ZOE_IPC_CHANNEL_EK2F,       // EK2F
    ZOE_IPC_CHANNEL_EU2F,       // EU2F
    ZOE_IPC_CHANNEL_F2F,        // F2F loopback
    ZOE_IPC_CHANNEL_S2F,        // S2F
    ZOE_IPC_CHANNEL_HK2F,       // HK2F
    ZOE_IPC_CHANNEL_HU2F,       // HU2F
    ZOE_IPC_CHANNEL_M2F,        // M2F

    ZOE_IPC_CHANNEL_EK2S,       // EK2S
    ZOE_IPC_CHANNEL_EU2S,       // EU2S
    ZOE_IPC_CHANNEL_F2S,        // F2S
    ZOE_IPC_CHANNEL_S2S,        // S2S loopback
    ZOE_IPC_CHANNEL_HK2S,       // HK2S
    ZOE_IPC_CHANNEL_HU2S,       // HU2S
    ZOE_IPC_CHANNEL_M2S,        // M2S

    ZOE_IPC_CHANNEL_EK2HK,      // EK2HK
    ZOE_IPC_CHANNEL_EU2HK,      // EU2HK
    ZOE_IPC_CHANNEL_F2HK,       // F2HK
    ZOE_IPC_CHANNEL_S2HK,       // S2HK
    ZOE_IPC_CHANNEL_HK2HK,      // HK2HK loopback
    ZOE_IPC_CHANNEL_HU2HK,      // HU2HK
    ZOE_IPC_CHANNEL_M2HK,       // M2HK

    ZOE_IPC_CHANNEL_EK2HU,      // EK2H
    ZOE_IPC_CHANNEL_EU2HU,      // EU2H
    ZOE_IPC_CHANNEL_F2HU,       // F2H
    ZOE_IPC_CHANNEL_S2HU,       // S2H
    ZOE_IPC_CHANNEL_HK2HU,      // HK2HK
    ZOE_IPC_CHANNEL_HU2HU,      // HU2HU loopback
    ZOE_IPC_CHANNEL_M2HU,       // M2H

    ZOE_IPC_CHANNEL_EK2M,       // EK2M
    ZOE_IPC_CHANNEL_EU2M,       // EU2M
    ZOE_IPC_CHANNEL_F2M,        // F2M
    ZOE_IPC_CHANNEL_S2M,        // S2M
    ZOE_IPC_CHANNEL_HK2M,       // H2M
    ZOE_IPC_CHANNEL_HU2M,       // H2M
    ZOE_IPC_CHANNEL_M2M,        // M2M loopback

    ZOE_IPC_CHANNEL_END

} ZOE_IPC_CHANNEL, *PZOE_IPC_CHANNEL;

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)

// there are 4 CPU clusters, so the total number of channels are 12 + 4
// (N) * (N - 1) + N loop back channels
//
// 0: [EK] external CPU kernel
// 1: [EU] external CPU user
// 2: [HK] application CPU kernel
// 3: [HU] application CPU user
//
typedef enum _ZOE_IPC_CPU
{
    ZOE_IPC_NULL_CPU = -1,
    ZOE_IPC_EXT_KERNEL = 0,
    ZOE_IPC_EXT_USER,
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,

    ZOE_IPC_CPU_END
} ZOE_IPC_CPU, *PZOE_IPC_CPU;


typedef enum _ZOE_IPC_CHANNEL
{
    ZOE_IPC_CHANNEL_NULL = -1,

    ZOE_IPC_CHANNEL_EK2EK = 0,  // EK2EK loopback
    ZOE_IPC_CHANNEL_EU2EK,      // E2EK
    ZOE_IPC_CHANNEL_HK2EK,      // H2EK
    ZOE_IPC_CHANNEL_HU2EK,      // H2EK

    ZOE_IPC_CHANNEL_EK2EU,      // EK2EU
    ZOE_IPC_CHANNEL_EU2EU,      // EU2EU loopback
    ZOE_IPC_CHANNEL_HK2EU,      // H2EU
    ZOE_IPC_CHANNEL_HU2EU,      // H2EU

    ZOE_IPC_CHANNEL_EK2HK,      // EK2HK
    ZOE_IPC_CHANNEL_EU2HK,      // EU2HK
    ZOE_IPC_CHANNEL_HK2HK,      // HK2HK loopback
    ZOE_IPC_CHANNEL_HU2HK,      // HU2HK

    ZOE_IPC_CHANNEL_EK2HU,      // EK2H
    ZOE_IPC_CHANNEL_EU2HU,      // EU2H
    ZOE_IPC_CHANNEL_HK2HU,      // HK2HK
    ZOE_IPC_CHANNEL_HU2HU,      // HU2HU loopback

    ZOE_IPC_CHANNEL_END

} ZOE_IPC_CHANNEL, *PZOE_IPC_CHANNEL;

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

// there are 5 CPU clusters, so the total number of channels are 20 + 5
// (N) * (N - 1) + N loop back channels
//
// 0: [F] firmware CPU
// 1: [HK] application CPU kernel
// 2: [HU] application CPU user
//
typedef enum _ZOE_IPC_CPU
{
    ZOE_IPC_NULL_CPU = -1,
    ZOE_IPC_FWPU = 0,
    ZOE_IPC_HPU_KERNEL,
    ZOE_IPC_HPU_USER,

    ZOE_IPC_CPU_END
} ZOE_IPC_CPU, *PZOE_IPC_CPU;


typedef enum _ZOE_IPC_CHANNEL
{
    ZOE_IPC_CHANNEL_NULL = -1,

    ZOE_IPC_CHANNEL_F2F,        // F2F loopback
    ZOE_IPC_CHANNEL_HK2F,       // HK2F
    ZOE_IPC_CHANNEL_HU2F,       // HU2F

    ZOE_IPC_CHANNEL_F2HK,       // F2HK
    ZOE_IPC_CHANNEL_HK2HK,      // HK2HK loopback
    ZOE_IPC_CHANNEL_HU2HK,      // HU2HK

    ZOE_IPC_CHANNEL_F2HU,       // F2H
    ZOE_IPC_CHANNEL_HK2HU,      // HK2HK
    ZOE_IPC_CHANNEL_HU2HU,      // HU2HU loopback

    ZOE_IPC_CHANNEL_END

} ZOE_IPC_CHANNEL, *PZOE_IPC_CHANNEL;

#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP


#define ZOE_IPC_CPU_NUM                 ZOE_IPC_CPU_END
#define ZOE_IPC_CHANNEL_NUM             ZOE_IPC_CHANNEL_END

// totoal ddr used by ipc mailboxes is
// ZOE_IPC_MB_SIZE * ZOE_IPC_CHANNEL_NUM
//
#define ZOE_IPC_MSG_PER_MB              (24)    // number of messages per mailbox
#define ZOE_IPC_MSG_PARAM_SIZE_WORD     (63)    // parameter size in word

#define ZOE_IPC_MB_HDR_SIZE_WORD        (3)     // mailbox header size in word
#define ZOE_IPC_HDR_SIZE_WORD           (4)     // message header size in word
#define ZOE_IPC_MB_HDR_SIZE             (ZOE_IPC_MB_HDR_SIZE_WORD * sizeof(uint32_t))
#define ZOE_IPC_MSG_SIZE                ((ZOE_IPC_HDR_SIZE_WORD + ZOE_IPC_MSG_PARAM_SIZE_WORD) * sizeof(uint32_t))
#define ZOE_IPC_MSG_PARAM_SIZE          (ZOE_IPC_MSG_PARAM_SIZE_WORD * sizeof(uint32_t))
#define ZOE_IPC_MB_SIZE                 ((ZOE_IPC_MSG_SIZE * ZOE_IPC_MSG_PER_MB) + ZOE_IPC_MB_HDR_SIZE)
#define ZOE_IPC_MB_SIZE_TOTAL           (ZOE_IPC_MB_SIZE * ZOE_IPC_CHANNEL_NUM)

#define REG_IPC_MB_HDR_OPEN(mb)         ((zoe_uintptr_t)mb)
#define REG_IPC_MB_HDR_RD(mb)           (((zoe_uintptr_t)mb) + 0x04)
#define REG_IPC_MB_HDR_WR(mb)           (((zoe_uintptr_t)mb) + 0x08)

#define REG_IPC_MSG(mb, id)             (((zoe_uintptr_t)mb) + (id * ZOE_IPC_MSG_SIZE) + 0x0C)
#define REG_IPC_MSG_STATUS(mb, id)      (((zoe_uintptr_t)mb) + (id * ZOE_IPC_MSG_SIZE) + 0x10)
#define REG_IPC_CONTEXT1(mb, id)        (((zoe_uintptr_t)mb) + (id * ZOE_IPC_MSG_SIZE) + 0x14)
#define REG_IPC_CONTEXT2(mb, id)        (((zoe_uintptr_t)mb) + (id * ZOE_IPC_MSG_SIZE) + 0x18)
#define REG_IPC_PARAM_1(mb, id)         (((zoe_uintptr_t)mb) + (id * ZOE_IPC_MSG_SIZE) + 0x1C)

#define REG_IPC_MB_BASE(mb)             (mb)
#define REG_IPC_MSG_BASE(mb, id)        (((zoe_uintptr_t)mb) + (id * ZOE_IPC_MSG_SIZE) + ZOE_IPC_MB_HDR_SIZE)

#define REG_IPC_MSG_OFFSET              0
#define REG_IPC_MSG_STATUS_OFFSET       0x04
#define REG_IPC_CONTEXT1_OFFSET         0x08
#define REG_IPC_CONTEXT2_OFFSET         0x0C
#define REG_IPC_PARAM_1_OFFSET          0x10
#define REG_IPC_PARAM_X_OFFSET(x)       (0x0C + (4 * x))    // start from 1


/////////////////////////////////////////////////////////////////////////////
//
//
#pragma pack(4)

#define ZOE_IPC_MB_HDR_OPEN_SIGNATURE   0xABBACAFE


typedef union _ZOE_IPC_MB_HDR_RD
{
    uint32_t        Read; 
    struct
    {
        uint32_t    rd_ptr		: 10;	// bit 0 - 9
        uint32_t    rd_cnt		: 22;	// bit 10 - 31
	} Reg;
} ZOE_IPC_MB_HDR_RD, *PZOE_IPC_MB_HDR_RD;


typedef union _ZOE_IPC_MB_HDR_WR
{
    uint32_t        Read; 
    struct
    {
        uint32_t    wr_ptr		: 10;	// bit 0 - 9
        uint32_t    wr_cnt		: 22;	// bit 10 - 31
	} Reg;
} ZOE_IPC_MB_HDR_WR, *PZOE_IPC_MB_HDR_WR;


#define ZOE_IPC_MB_CNT_MAX              0x3FFFFF

#define ZOE_IPC_MB_PTR_SET(x)           ((x) & 0x3FF)
#define ZOE_IPC_MB_CNT_SET(x)           (((x) & 0x3FFFFF) << 10)
#define ZOE_IPC_MB_PTR_GET(x)           ((x) & 0x3FF)
#define ZOE_IPC_MB_CNT_GET(x)           (((x) >> 10) & 0x3FFFFF)


typedef struct _ZOE_IPC_MB_HDR
{
    uint32_t    open;
    uint32_t    rd;
    uint32_t    wr;
} ZOE_IPC_MB_HDR, *PZOE_IPC_MB_HDR;


typedef union _ZOE_IPC_MESSAGE
{
    uint32_t        Read; 
    struct
    {
        uint32_t    message		: 10;	// bit 0 - 9
        uint32_t    intf		: 9;	// bit 10 - 18
        uint32_t    module	    : 9;	// bit 19 - 27
        uint32_t    inst		: 4;	// bit 28 - 31
	} Reg;
} ZOE_IPC_MESSAGE, *PZOE_IPC_MESSAGE;


#define ZOE_IPC_MAX_INTF                (1 << 9)
#define ZOE_IPC_MAX_MODULE              (1 << 9)
#define ZOE_IPC_MAX_INST                (1 << 4)

#define ZOE_IPC_MSG_MSG_SET(x)          ((x) & 0x3FF)
#define ZOE_IPC_MSG_INTF_SET(x)         (((x) & 0x1FF) << 10)
#define ZOE_IPC_MSG_MOD_SET(x)          (((x) & 0x1FF) << 19)
#define ZOE_IPC_MSG_INST_SET(x)         (((x) & 0xF) << 28)
#define ZOE_IPC_MSG_MSG_GET(x)          ((x) & 0x3FF)
#define ZOE_IPC_MSG_INTF_GET(x)         (((x) >> 10) & 0x1FF)
#define ZOE_IPC_MSG_MOD_GET(x)          (((x) >> 19) & 0x1FF)
#define ZOE_IPC_MSG_INST_GET(x)         (((x) >> 28) & 0xF)

// module

// message

// response
#define ZOE_IPC_MESSAGE_RESP_GENERIC    0x200  // bit 10
#define ZOE_IPC_MESSAGE_RESP(x)         ((x) | ZOE_IPC_MESSAGE_RESP_GENERIC)
#define ZOE_IPC_MESSAGE_REQ(x)          ((x) & ~ZOE_IPC_MESSAGE_RESP_GENERIC)



typedef union _ZOE_IPC_MESSAGE_STATUS
{
    uint32_t        Read;
    struct
    {
        uint32_t	rsvp	    : 1;	// bit 0
        uint32_t	req	        : 1;	// bit 1
        uint32_t    session     : 5;	// bit 2 - 6
        uint32_t    msg_size	: 9;	// bit 7 - 15
        uint32_t    msg_id	    : 16;	// bit 16 - 31
	} Reg;
} ZOE_IPC_MESSAGE_STATUS, *PZOE_IPC_MESSAGE_STATUS;

// session
#define ZOE_IPC_SESSION_MAX (1 << 5)

#define ZOE_IPC_STS_RSVP_SET(x)         ((x) & 0x1)
#define ZOE_IPC_STS_REQ_SET(x)          (((x) & 0x1) << 1)
#define ZOE_IPC_STS_SESSION_SET(x)      (((x) & 0x1F) << 2)
#define ZOE_IPC_STS_SIZE_SET(x)         (((x) & 0x1FF) << 7)
#define ZOE_IPC_STS_ID_SET(x)           (((x) & 0xFFFF) << 16)
#define ZOE_IPC_STS_RSVP_GET(x)         ((x) & 0x1)
#define ZOE_IPC_STS_REQ_GET(x)          (((x) >> 1) & 0x1)
#define ZOE_IPC_STS_SESSION_GET(x)      (((x) >> 2) & 0x1F)
#define ZOE_IPC_STS_SIZE_GET(x)         (((x) >> 7) & 0x1FF)
#define ZOE_IPC_STS_ID_GET(x)           (((x) >> 16) & 0xFFFF)

#define ZOE_IPC_STS_SESSION_CLR(x)      ((x) & ~0x7C)
#define ZOE_IPC_STS_ID_CLR(x)           ((x) & ~0xFFFF0000)

typedef union _ZOE_IPC_CALLER_CONTEXT
{
    uint32_t        Read[2];
    struct
    {
        zoe_sosal_obj_id_t  context;
	} Reg;
} ZOE_IPC_CALLER_CONTEXT, *PZOE_IPC_CALLER_CONTEXT;



typedef struct _ZOE_IPC_MSG_HDR
{
    uint32_t                msg;
    uint32_t                status;
    ZOE_IPC_CALLER_CONTEXT  context;
} ZOE_IPC_MSG_HDR, *PZOE_IPC_MSG_HDR;


#define ZOE_IPC_MESSAGE_MSG_HDR_SIZE    (sizeof(ZOE_IPC_MSG_HDR) >> 2)


typedef struct _ZOE_IPC_MSG
{
    ZOE_IPC_MSG_HDR hdr;
    uint32_t        param[ZOE_IPC_MSG_PARAM_SIZE_WORD];
} ZOE_IPC_MSG, *PZOE_IPC_MSG;



/////////////////////////////////////////////////////////////////////////////
//
//

// overlapped I/O
//
typedef struct _ZOE_IPC_OVERLAPPED
{
    zoe_sosal_obj_id_t  hEvent;

    union 
    {
        uint32_t    param[ZOE_IPC_MSG_PARAM_SIZE_WORD];
    } Resp;

} ZOE_IPC_OVERLAPPED, *PZOE_IPC_OVERLAPPED;


#pragma pack()


/////////////////////////////////////////////////////////////////////////////
//
//

STATIC_INLINE void ZOE_IPC_PREPARE_REQUEST(PZOE_IPC_MSG_HDR pReq, uint32_t message, uint32_t intf, uint32_t module, uint32_t inst, uint32_t rsvp, uint32_t msg_size)
{
    (pReq)->msg = ZOE_IPC_MSG_MSG_SET(message) | ZOE_IPC_MSG_INTF_SET(intf) | ZOE_IPC_MSG_MOD_SET(module) | ZOE_IPC_MSG_INST_SET(inst);
    (pReq)->status = ZOE_IPC_STS_RSVP_SET(rsvp) | ZOE_IPC_STS_REQ_SET(1) | ZOE_IPC_STS_SIZE_SET(msg_size);
    (pReq)->context.Read[0] = 0;
    (pReq)->context.Read[1] = 0;
}

STATIC_INLINE void ZOE_IPC_PREPARE_RESPONSE(PZOE_IPC_MSG_HDR pReq, PZOE_IPC_MSG_HDR pResp, uint32_t msg_size)
{
    (pResp)->msg = ZOE_IPC_MSG_MSG_SET(ZOE_IPC_MESSAGE_RESP(ZOE_IPC_MSG_MSG_GET((pReq)->msg))) | 
                   ZOE_IPC_MSG_INTF_SET(ZOE_IPC_MSG_INTF_GET((pReq)->msg)) |
                   ZOE_IPC_MSG_MOD_SET(ZOE_IPC_MSG_MOD_GET((pReq)->msg)) |
                   ZOE_IPC_MSG_INST_SET(ZOE_IPC_MSG_INST_GET((pReq)->msg))
                   ;
    (pResp)->status = ZOE_IPC_STS_SESSION_SET(ZOE_IPC_STS_SESSION_GET((pReq)->status)) |
                      ZOE_IPC_STS_SIZE_SET(msg_size) |
                      ZOE_IPC_STS_ID_SET(ZOE_IPC_STS_ID_GET((pReq)->status));
    (pResp)->context.Read[0] = (pReq)->context.Read[0];
    (pResp)->context.Read[1] = (pReq)->context.Read[1];
}

STATIC_INLINE void ZOE_IPC_MSG_CPY(PZOE_IPC_MSG pDest, PZOE_IPC_MSG pSrc)
{
    uint32_t    i;
    // header
    (pDest)->hdr.msg = (pSrc)->hdr.msg;
    (pDest)->hdr.status = (pSrc)->hdr.status;
    (pDest)->hdr.context.Read[0] = (pSrc)->hdr.context.Read[0];
    (pDest)->hdr.context.Read[1] = (pSrc)->hdr.context.Read[1];
    // parameters
    for (i = 0; i < ZOE_IPC_STS_SIZE_GET((pSrc)->hdr.status); i++)
    {
        (pDest)->param[i] = (pSrc)->param[i];
    }
}

STATIC_INLINE void ZOE_IPC_OVERLAPPED_CPY(PZOE_IPC_OVERLAPPED pDest, PZOE_IPC_MSG pSrc)
{
    uint32_t    i;
    // parameters
    for (i = 0; i < ZOE_IPC_STS_SIZE_GET((pSrc)->hdr.status); i++)
    {
        (pDest)->Resp.param[i] = (pSrc)->param[i];
    }
}


#ifdef __cplusplus
}
#endif

#endif //__ZOE_IPC_DEF_H__



