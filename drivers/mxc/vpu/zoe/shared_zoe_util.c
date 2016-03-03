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
// zoe_util.c
//
// Description: 
//
//  handy utilities
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifdef ZOE_WINKER_BUILD
#include <wdm.h>
#endif //ZOE_WINKER_BUILD
#include "zoe_util.h"
#include "zoe_sosal.h"
#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#else //ZOE_LINUXKER_BUILD
#include <asm/div64.h>
#include <linux/string.h>
#endif //!ZOE_LINUXKER_BUILD

uint64_t util_get_system_time_ms(void) 
{
    zoe_sosal_ticks_t   ticks;
    uint64_t            ms;
    uint64_t            ticks_per_sec;

    ticks = zoe_sosal_time_sys_ticks();
    ticks_per_sec = zoe_sosal_time_ticks_per_second();
#ifdef ZOE_LINUXKER_BUILD
    ticks = ticks * 1000;
    do_div(ticks, ((uint32_t)ticks_per_sec));
    ms = ticks;
#else //!ZOE_LINUXKER_BUILD
    ms = (ticks * 1000) / ticks_per_sec;
#endif //ZOE_LINUXKER_BUILD
    return (ms);
}



uint64_t util_get_system_time_us(void) 
{
    zoe_sosal_ticks_t   ticks;
    uint64_t            us;
    uint64_t            ticks_per_sec;

    ticks = zoe_sosal_time_sys_ticks();
    ticks_per_sec = zoe_sosal_time_ticks_per_second();
#ifdef ZOE_LINUXKER_BUILD
    ticks = ticks * 1000000;
    do_div(ticks, ((uint32_t)ticks_per_sec));
    us = ticks;
#else //!ZOE_LINUXKER_BUILD    
    us = (ticks * 1000000) / ticks_per_sec;
#endif //ZOE_LINUXKER_BUILD
    return (us);
}



// in msec
void util_begin_timer(uint64_t *pTimeto)
{
    uint64_t    curTime;

    curTime = util_get_system_time_ms();
    *pTimeto += curTime;
}



zoe_bool_t util_timeout(uint64_t Timeto, 
				        uint32_t delay
				        )
{
    uint64_t	curTime;

    curTime = util_get_system_time_ms();

	if (delay && (curTime < Timeto))
    {
		zoe_sosal_thread_sleep_ms(delay);
    }

    return (curTime >= Timeto);
}



uint32_t util_bytes_swap(uint8_t *pSource,  // Pointer to source data
					     uint8_t *pTarget,  // Pointer to hold the target data, NULL if inplace swapping is required
					     uint32_t size      // Size of buffers in bytes.
					     )
{
    uint32_t    bytesSwapped = 0;
    uint32_t    remainder = 0;

	// Make it 4 bytes aligned.
	if ((size < sizeof(uint32_t)) || 
		(ZOE_NULL == pSource)
		)
	{
		return (bytesSwapped);
	}

    remainder = size % sizeof(uint32_t);
	size -= remainder;

	if ((pSource == pTarget) || 
		(ZOE_NULL == pTarget)
		)
	{
		while (bytesSwapped < size)
		{
            uint8_t ucTmp;
			ucTmp = *pSource;
			*pSource = *(pSource + 3);
			*(pSource + 3) = ucTmp;

			ucTmp = *(pSource + 1);
			*(pSource + 1) = *(pSource + 2);
			*(pSource + 2) = ucTmp;

            pSource += sizeof(uint32_t);
			bytesSwapped += sizeof(uint32_t);
		}
	}
	else
	{
		while (bytesSwapped < size)
		{
			*pTarget = *(pSource + 3);
			*(pTarget + 1) = *(pSource + 2);
			*(pTarget + 2) = *(pSource + 1);
			*(pTarget + 3) = *pSource;
			pSource += sizeof(uint32_t);
			pTarget += sizeof(uint32_t);
			bytesSwapped += sizeof(uint32_t);
		}
	}

	return (bytesSwapped);
}



zoe_bool_t util_is_equal_guid(ZOE_REFGUID rguid1, 
						      ZOE_REFGUID rguid2
						      )
{ 
    return ((zoe_bool_t)!memcmp(rguid1, 
							    rguid2, 
							    sizeof(ZOE_GUID)
							    )); 
}



zoe_bool_t util_is_safe_to_call(void)
{
#ifdef ZOE_WINKER_BUILD
	return (KeGetCurrentIrql() < DISPATCH_LEVEL);
#else //!ZOE_WINKER_BUILD
	return (ZOE_TRUE);
#endif //ZOE_WINKER_BUILD
}


void * util_memcpy(void *dest, 
                   const void *src, 
                   uint32_t count
                   )
{
    return (memcpy(dest, src, (size_t)count));
}


void * util_memset(void *dest, 
                   int c,
                   uint32_t count
                   )
{
    return (memset(dest, c, (size_t)count));
}





