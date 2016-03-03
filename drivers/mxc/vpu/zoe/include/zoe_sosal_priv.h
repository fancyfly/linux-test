/*
 * Copyright (c) 2011-2015, Freescale Semiconductor, Inc.
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
/******************************************************************************
*                                                                             *
* All rights are reserved. This confidential and proprietary HDL/C/HVL soft   *
* description of a Hardware/Software component may be used only as authorized *
* by a licensing agreement from Zenverge Incorporated. In the event of        *
* publication, the following notice is applicable:                            *
*                                                                             *
*                  (C) COPYRIGHT 2011-2014 Zenverge Inc.                      *
*                          ALL RIGHTS RESERVED                                *
* The entire notice above must be reproduced on all authorized copies of this *
* code. Reproduction in whole or in part is prohibited without the prior      *
* written consent of the Zenverge Incorporated.                               *
* Zenverge Incorporated reserves the right to make changes without notice at  *
* any time. Also, Zenverge Incorporated makes no warranty, expressed, implied *
* or statutory, including but not limited to any implied warranty of          *
* merchantability or fitness for any particular purpose, or that the use will *
* not infringe any third party patent, copyright or trademark. Zenverge       *
* Incorporated will not be liable for any loss or damage arising from its use *
******************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//
// zoe_sosal_priv.h OS Abstraction Layer, private interface:
//                      software interrupts
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_SOSAL_PRIV_H__
#define __ZOE_SOSAL_PRIV_H__


#include "zoe_types.h"
#include "zoe_sosal.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/////////////////////////
// Memory heaps info
//

// Get heap information
typedef struct {
    uint32_t    blocksNum;
    uint32_t    usedBytes;
    uint32_t    holesNum;
    uint32_t    totAvail;
    uint32_t    largestAvail;
} zoe_sosal_memory_heap_info_t, *zoe_sosal_memory_heap_info_ptr_t;

zoe_bool_t zoe_sosal_memory_get_info (zoe_sosal_memory_pools_t pool, zoe_sosal_memory_heap_info_ptr_t heap_info_ptr);

typedef struct {
	zoe_bool_t			isHole;
	zoe_uintptr_t		address;
	uint32_t			bytesRequested;
	uint32_t			bytesUsed;
	zoe_sosal_obj_id_t	threadID;
	uint16_t			alignment;
} zoe_sosal_memory_block_info_t, *zoe_sosal_memory_block_info_ptr_t;


zoe_bool_t zoe_sosal_memory_blocks_info (zoe_sosal_memory_pools_t pool,
							zoe_sosal_memory_block_info_ptr_t block_info_array,
							uint32_t * block_num_ptr);

////////////////////////
// IPC interrupts
//
// One ISR handler can be installed per each of a range of available
// pairs of sources and destinations.
//

#define ZOE_SOSAL_ISR_SW_SELF (-1)

typedef enum {
    ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL = 0,
    ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER,
    ZOE_SOSAL_CHISEL_ISR_SW_SPU,
    ZOE_SOSAL_CHISEL_ISR_SW_DMAPU,
    ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0,
    ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1,
    ZOE_SOSAL_CHISEL_ISR_SW_EDPU,
    ZOE_SOSAL_CHISEL_ISR_SW_EEPU,
    ZOE_SOSAL_CHISEL_ISR_SW_MEPU,
    ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL,
    ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER,

    ZOE_SOSAL_CHISEL_ISR_SW_NUM
} zoe_sosal_chisel_isr_sw_numbers_t;

typedef enum {
    ZOE_SOSAL_DAWN_ISR_SW_HPU_KERNEL = 0,
    ZOE_SOSAL_DAWN_ISR_SW_HPU_USER,
    ZOE_SOSAL_DAWN_ISR_SW_SPU,
    ZOE_SOSAL_DAWN_ISR_SW_FWPU,
    ZOE_SOSAL_DAWN_ISR_SW_MEPU,
    ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL,
    ZOE_SOSAL_DAWN_ISR_SW_EXT_USER,

    ZOE_SOSAL_DAWN_ISR_SW_NUM
} zoe_sosal_dawn_isr_sw_numbers_t;

typedef enum {
    ZOE_SOSAL_MX6_ISR_SW_HPU_KERNEL = 0,
    ZOE_SOSAL_MX6_ISR_SW_HPU_USER,
    ZOE_SOSAL_MX6_ISR_SW_EXT_KERNEL,
    ZOE_SOSAL_MX6_ISR_SW_EXT_USER,

    ZOE_SOSAL_MX6_ISR_SW_NUM
} zoe_sosal_mx6_isr_sw_numbers_t;

typedef enum {
    ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL = 0,
    ZOE_SOSAL_MX8_ISR_SW_HPU_USER,
    ZOE_SOSAL_MX8_ISR_SW_FWPU,

    ZOE_SOSAL_MX8_ISR_SW_NUM
} zoe_sosal_mx8_isr_sw_numbers_t;

typedef zoe_int32_t zoe_sosal_isr_sw_numbers_t;

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
	#define ZOE_SOSAL_ISR_SW_NUM   ZOE_SOSAL_CHISEL_ISR_SW_NUM
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
	#define ZOE_SOSAL_ISR_SW_NUM   ZOE_SOSAL_DAWN_ISR_SW_NUM
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
	#define ZOE_SOSAL_ISR_SW_NUM   ZOE_SOSAL_MX6_ISR_SW_NUM
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
	#define ZOE_SOSAL_ISR_SW_NUM   ZOE_SOSAL_MX8_ISR_SW_NUM
#endif

// The prototype for a SW ISR handler
typedef void (*zoe_sosal_isr_sw_proc_t) (void * ctxt,
                                         zoe_sosal_isr_sw_numbers_t from_num);

// Return the current CPU's ISR number
zoe_sosal_isr_sw_numbers_t zoe_sosal_isr_sw_my_isr_num (void);

// Install a custom SW ISR handler (set 'proc' to NULL to uninstall)
zoe_errs_t zoe_sosal_isr_sw_handler_install (zoe_sosal_isr_sw_numbers_t from_num,
                                             zoe_sosal_isr_sw_proc_t proc,
                                             void * ctxt);

// Set the enable/disable state of a given SW ISR; the previous state is returned
// in *enable_ptr
zoe_errs_t zoe_sosal_isr_sw_enable_state_set (zoe_sosal_isr_sw_numbers_t from_num,
                                              zoe_bool_t * enable_ptr);

// Trigger a SW ISR; use ZOE_SOSAL_ISR_SW_SELF for the running CPU
zoe_errs_t zoe_sosal_isr_sw_trigger (zoe_sosal_isr_sw_numbers_t to_num);

// Clear a SW ISR
zoe_errs_t zoe_sosal_isr_sw_clear (zoe_sosal_isr_sw_numbers_t from_num);


// Check if running within an ISR handler
zoe_bool_t zoe_sosal_isr_in_isr (void);




////////////////////////
//
// Test/validation interface
//

// Issue an invalid instruction (and therefore cause an exception)
void zoe_sosal_test_invalid_instruction (void);



#ifdef __cplusplus
}
#endif //__cplusplus


#endif //__ZOE_SOSAL_PRIV_H__

