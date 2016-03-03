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
// zv_devdiag.h
//
// Description: 
//
//   This is the definition of the zv codec diagnostic interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZV_DEVDIAG_H__
#define __ZV_DEVDIAG_H__


#include "zoe_types.h"


// {AC130D8F-593E-41c4-B966-F033FDCA6844}
static const ZOE_GUID PROPSETID_ZV_CODEC_DIAG = 
{ 0xac130d8f, 0x593e, 0x41c4, { 0xb9, 0x66, 0xf0, 0x33, 0xfd, 0xca, 0x68, 0x44 } };


typedef enum _PROPERTY_ZV_CODEC_DIAGNOSTIC
{
    ZV_CODEC_DIAG_REG_READ				    = 5,
    ZV_CODEC_DIAG_REG_WRITE				    = 6,
    ZV_CODEC_DIAG_REG_READ_EX			    = 7,
    ZV_CODEC_DIAG_REG_WRITE_EX			    = 8,

    ZV_CODEC_DIAG_MEM_READ				    = 10,
    ZV_CODEC_DIAG_MEM_WRITE				    = 11,

	ZV_CODEC_DIAG_DMA_READ				    = 14,
	ZV_CODEC_DIAG_DMA_WRITE				    = 15,
	ZV_CODEC_DIAG_DMA_READ_PTR			    = 16,
	ZV_CODEC_DIAG_DMA_WRITE_PTR			    = 17,

    ZV_CODEC_DIAG_WAIT_ISR                  = 20,
    ZV_CODEC_DIAG_ENABLE_WAIT_ISR           = 21,
    ZV_CODEC_DIAG_DISABLE_WAIT_ISR          = 22,
    ZV_CODEC_DIAG_SET_ISR                   = 23,

    ZV_CODEC_DIAG_IPC_REG                   = 30,
    ZV_CODEC_DIAG_IPC_TEST                  = 31,

    ZV_CODEC_DIAG_CHIP_VERSION              = 40

} PROPERTY_ZV_CODEC_DIAGNOSTIC;


#define ZV_MAX_MEMORY_SIZE		            512
#define ZV_MAX_REGISTER_DWORDS		        64
#define ZV_MAX_DMA_SIZE				        (64 * 1024)

#pragma pack(4)

typedef struct _ZV_CODEC_DIAG_REGISTER_STRUCT
{
    uint32_t    address;
    uint32_t    value;
} ZV_CODEC_DIAG_REGISTER_STRUCT, *PZV_CODEC_DIAG_REGISTER_STRUCT;


typedef struct _ZV_CODEC_DIAG_REGISTER_STRUCT_EX
{
    uint32_t	address;
    uint32_t	num_reg;
    uint32_t	value[ZV_MAX_REGISTER_DWORDS];
} ZV_CODEC_DIAG_REGISTER_STRUCT_EX, *PZV_CODEC_DIAG_REGISTER_STRUCT_EX;


typedef struct _ZV_CODEC_DIAG_MEMORY_STRUCT
{
    zoe_dev_mem_t	address;
    uint32_t	    size;
    uint8_t	        data[ZV_MAX_MEMORY_SIZE];
} ZV_CODEC_DIAG_MEMORY_STRUCT, *PZV_CODEC_DIAG_MEMORY_STRUCT;


typedef struct _ZV_CODEC_DIAG_DMA_STRUCT
{
	zoe_dev_mem_t	deviceAddress;
	uint32_t	    size;
	uint8_t	        data[ZV_MAX_DMA_SIZE];
    zoe_bool_t      bSwap;
} ZV_CODEC_DIAG_DMA_STRUCT, *PZV_CODEC_DIAG_DMA_STRUCT;


typedef struct _ZV_CODEC_DIAG_DMA_PTR_STRUCT
{
	zoe_dev_mem_t	deviceAddress;
	uint32_t	    size;
	uint8_t	        *pData;	    // caller has to make sure the memory 
						        // is locked down and accessable by the 
						        // codec library
    uint32_t        ulXferMode;
    zoe_bool_t      bSwap;
} ZV_CODEC_DIAG_DMA_PTR_STRUCT, *PZV_CODEC_DIAG_DMA_PTR_STRUCT;


typedef struct _ZV_CODEC_DIAG_WAIT_ISR_STRUCT
{
    uint32_t        from_cpu_num;
    uint32_t        timeout_ms;
} ZV_CODEC_DIAG_WAIT_ISR_STRUCT, *PZV_CODEC_DIAG_WAIT_ISR_STRUCT;


typedef struct _ZV_CODEC_DIAG_ENABLE_WAIT_ISR_STRUCT
{
    uint32_t        from_cpu_num;
} ZV_CODEC_DIAG_ENABLE_WAIT_ISR_STRUCT, *PZV_CODEC_DIAG_ENABLE_WAIT_ISR_STRUCT;


typedef struct _ZV_CODEC_DIAG_DISABLE_WAIT_ISR_STRUCT
{
    uint32_t        from_cpu_num;
} ZV_CODEC_DIAG_DISABLE_WAIT_ISR_STRUCT, *PZV_CODEC_DIAG_DISABLE_WAIT_ISR_STRUCT;


typedef struct _ZV_CODEC_DIAG_SET_ISR_STRUCT
{
    uint32_t        to_cpu_num;
    uint32_t        from_cpu_num;
} ZV_CODEC_DIAG_SET_ISR_STRUCT, *PZV_CODEC_DIAG_SET_ISR_STRUCT;


typedef struct _ZV_CODEC_DIAG_IPC_REG_STRUCT
{
    uint32_t        reg;
} ZV_CODEC_DIAG_IPC_REG_STRUCT, *PZV_CODEC_DIAG_IPC_REG_STRUCT;


typedef struct _ZV_CODEC_DIAG_IPC_TEST_STRUCT
{
    uint32_t        to_cpu;
    uint32_t        iteration;
    uint32_t        second;
} ZV_CODEC_DIAG_IPC_TEST_STRUCT, *PZV_CODEC_DIAG_IPC_TEST_STRUCT;


typedef struct _ZV_CODEC_DIAG_CHIP_VERSION_STRUCT
{
    uint32_t        chip;
} ZV_CODEC_DIAG_CHIP_VERSION_STRUCT, *PZV_CODEC_DIAG_CHIP_VERSION_STRUCT;


#define ZV_CODEC_DIAG_CHIP_VERSION_CHISEL   1
#define ZV_CODEC_DIAG_CHIP_VERSION_CAFE     2
#define ZV_CODEC_DIAG_CHIP_VERSION_iMX8     3

#pragma pack()

#endif //ZV_DEVDIAG_H


