/*
 * Copyright (c) 2014-2015, Freescale Semiconductor, Inc.
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
*                        ALL RIGHTS RESERVED                                  *
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
// zoe sosal memory management entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Nov 8, 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#include <stdio.h>
#endif

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"

#include "os_common.h"
#include "os_stubs.h"

#define CL_MASK ((1<<OSMemoryCachelineLog2())-1)

ZOE_DBG_COMP_EXT(ZoeSosalDbgCompID);



////////////////////////
//  Memory management
//

// Allocate a block of memory with a given alignment (number of least
// significant bits set to 0 in the returned address).
// The address returned is for cached access.
void *
zoe_sosal_memory_alloc (zoe_sosal_memory_pools_t pool, uint32_t size, uint32_t alignment)
{
	return (OSMemoryAlloc(pool,size,alignment));
}


void *
zoe_sosal_memory_local_alloc (uint32_t size)
{
	return (OSMemoryAlloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,size,0));
}


void *
zoe_sosal_memory_internal_alloc (uint32_t size)
{
	return (OSMemoryAlloc(ZOE_SOSAL_MEMORY_POOLS_INTERNAL,size,0));
}


void *
zoe_sosal_memory_shared_alloc (uint32_t size)
{
	return (OSMemoryAlloc(ZOE_SOSAL_MEMORY_POOLS_SHARED,size,0));
}


// Free a previously allocated memory block
void
zoe_sosal_memory_free (void * mem_ptr)
{
    if (mem_ptr != ZOE_NULL) {
        OSMemoryFree(mem_ptr);
    }
}


// Return the cached pointer for a given non-cached memory block
void *
zoe_sosal_memory_cached (void * noncached_ptr)
{
    return (OSMemoryCached(noncached_ptr));
}


// Return the non-cached pointer for a given cached memory block
void *
zoe_sosal_memory_noncached (void * cached_ptr)
{
    return (OSMemoryNoncached(cached_ptr));
}


// Return the log2 of the cacheline size (e.g. return 5 for 32-byte lines)
uint32_t
zoe_sosal_memory_cacheline_log2 (void)
{
    return (OSMemoryCachelineLog2());
}


// Clean (flush) the cache over a given memory area; address must be cache
// aligned, 'lines' cache lines will be cleaned 
zoe_errs_t
zoe_sosal_memory_cache_clean (void * start_ptr, uint32_t lines)
{
    zoe_errs_t      rv = ZOE_ERRS_PARMS;
    uint32_t  	    addr = (uint32_t) ((uintptr_t)start_ptr);

    if ((addr & CL_MASK) == 0) {
        rv = OSMemoryCacheClean(start_ptr,lines);
    }

    return (rv);
}


// Invalidate the cache over a given memory area; address must be cache-aligned
// lines cache lines will be invalidated
zoe_errs_t
zoe_sosal_memory_cache_inval (void * start_ptr, uint32_t lines)
{
    zoe_errs_t      rv = ZOE_ERRS_PARMS;
    uint32_t    	addr = (uint32_t) ((uintptr_t)start_ptr);

    if ((addr & CL_MASK) == 0) {
        rv = OSMemoryCacheInval(start_ptr,lines);
    }

    return (rv);
}


// Return the physical address for a given virtual memory address
void *
zoe_sosal_memory_get_phy (void * vir_ptr)
{
	return (OSMemoryGetPhy(vir_ptr));
}


// Return the virtual address for a given physical memory address
void *
zoe_sosal_memory_get_vir (void * phy_ptr)
{
	return (OSMemoryGetVir(phy_ptr));
}


// Return whether the virtual address is valid or not
zoe_bool_t
zoe_sosal_memory_is_valid_vir (void * vir_ptr)
{
	return (OSMemoryIsValidVir(vir_ptr));
}


