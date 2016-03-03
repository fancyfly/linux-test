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
// zv_busintf.h
//
// Description: 
//
//   ZOE common bus call back interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZV_BUSINTF_H__
#define __ZV_BUSINTF_H__


#include "zoe_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

// DMA swap type
//
#define DMA_DATA_SWAP_BYTE              0x00000001
#define DMA_DATA_SWAP_WORD              0x00000002
#define DMA_DATA_SWAP_DWORD             0x00000004


// bus driver callback fuction type
//
typedef zoe_errs_t (*ZV_BUSINTF_CALLBACK)(zoe_void_ptr_t context, uint32_t cmd, zoe_void_ptr_t param1, uint32_t param2, uint32_t param3);

// command code
//
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)


#define ZV_BUSINTF_INT_CPU_FROM_EXT     0x00000001
#define ZV_BUSINTF_INT_CPU_FROM_DMAPU   0x00000002
#define ZV_BUSINTF_INT_CPU_FROM_AUD0    0x00000004
#define ZV_BUSINTF_INT_CPU_FROM_AUD1    0x00000008
#define ZV_BUSINTF_INT_CPU_FROM_EDPU    0x00000010
#define ZV_BUSINTF_INT_CPU_FROM_EEPU    0x00000020
#define ZV_BUSINTF_INT_CPU_FROM_MEPU    0x00000040
#define ZV_BUSINTF_INT_CPU_FROM_SPU     0x00000080
#define ZV_BUSINTF_INT_CPU_FROM_HPU     0x00000100

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

#define ZV_BUSINTF_INT_CPU_FROM_EXT     0x00000001
#define ZV_BUSINTF_INT_CPU_FROM_VID     0x00000002
#define ZV_BUSINTF_INT_CPU_FROM_AUD0    0x00000004
#define ZV_BUSINTF_INT_CPU_FROM_AUD1    0x00000008
#define ZV_BUSINTF_INT_CPU_FROM_AUD2    0x00000010
#define ZV_BUSINTF_INT_CPU_FROM_SPU     0x00000020
#define ZV_BUSINTF_INT_CPU_FROM_HPU     0x00000040
#define ZV_BUSINTF_INT_CPU_FROM_MEPU    0x00000080

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)

#define ZV_BUSINTF_INT_CPU_FROM_EXT     0x00000001
#define ZV_BUSINTF_INT_CPU_FROM_HPU     0x00000002

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

#define ZV_BUSINTF_INT_CPU_FROM_VID     0x00000002
#define ZV_BUSINTF_INT_CPU_FROM_HPU     0x00000040

#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

#define ZV_BUSINTF_DMA_DONE_READ        0x00010000
#define ZV_BUSINTF_DMA_DONE_WRITE       0x00020000

#define ZV_BUSINTF_INT_CPU_MASK         0x00000FFF
#define ZV_BUSINTF_DMA_DONE_MASK        0x000F0000

// param
//

// ZV_BUSINTF_DMA_DONE_READ
//
// param1 : complete event
// param2 : io status
// param3 : data transferred

// ZV_BUSINTF_DMA_DONE_WRITE
//
// param1 : complete event
// param2 : io status
// param3 : data transferred



#ifdef __cplusplus
}
#endif

#endif //__ZV_BUSINTF_H__


