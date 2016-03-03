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
// zoe_util.h
//
// Description: 
//
//  handy utilities
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_UTIL_H__
#define __ZOE_UTIL_H__


#include "zoe_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//
#define Swap4Bytes(val) \
		((((val) & 0xff000000) >> 24) |\
		(((val) & 0x00ff0000) >>  8) |\
		(((val) & 0x0000ff00) <<  8) |\
		(((val) & 0x000000ff) << 24))

#define LittleToBigEndian32(val)    Swap4Bytes(val)
#define BigToLittleEndian32(val)    Swap4Bytes(val)


#define Swap2Bytes(val) \
		((((val) & 0xff00) >>  8) |\
		(((val) & 0x00ff) << 8))

#define LittleToBigEndian16(val)    Swap2Bytes(val)
#define BigToLittleEndian16(val)    Swap2Bytes(val)


/////////////////////////////////////////////////////////////////////////////
//
//
uint64_t util_get_system_time_ms(void);
uint64_t util_get_system_time_us(void);
void util_begin_timer(uint64_t *pTimeto);
zoe_bool_t util_timeout(uint64_t Timeto, 
				        uint32_t delay
				        );
uint32_t util_bytes_swap(uint8_t *pSource,  // Pointer to source data
					     uint8_t *pTarget,  // Pointer to hold the target data, NULL if inplace swapping is required
					     uint32_t size      // Size of buffers in bytes.
					     );
zoe_bool_t util_is_equal_guid(ZOE_REFGUID rguid1, 
						      ZOE_REFGUID rguid2
						      );
zoe_bool_t util_is_safe_to_call(void);
void * util_memcpy(void *dest, 
                   const void *src, 
                   uint32_t count
                   );
void * util_memset(void *dest, 
                   int c,
                   uint32_t count
                   );
#ifdef __cplusplus
}
#endif

#endif //__ZOE_UTIL_H__



