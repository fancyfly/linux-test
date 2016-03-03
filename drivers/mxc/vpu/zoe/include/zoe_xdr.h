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
// zoe_xdr.h
//
// Description: 
//
//   ZOE eXternal Data Representation interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_XDR_H__
#define __ZOE_XDR_H__

#include "zoe_types.h"

// ZOEXDR version
//
#define _ZOEXDR_VERSION             1


#pragma pack(1)

// ZOEXDR stream header
//
typedef struct tagZOEXDR_STREAM_HDR
{
    uint32_t    ulXDRVersion;       // ZOEXDR version
    uint32_t    ulAPIVersion;       // API version
    uint32_t    ulFunctionID;       // function ID
    uint32_t    ulDataLength;       // data length
} ZOEXDR_STREAM_HDR, *PZOEXDR_STREAM_HDR;

#pragma pack()

// ZOEXDR byte order
//
#define ZOEXDR_ENDIAN_NONE      0
#define ZOEXDR_ENDIAN_BIG       1
#define ZOEXDR_ENDIAN_LITTLE    2

// ZOEXDR network byte order
//
#define ZOEXDR_ENDIAN_NETWORK   ZOEXDR_ENDIAN_LITTLE


// ZOEXDR direction
//
#define _ZOEXDR_ENCODE          1   // host to network
#define _ZOEXDR_DECODE          2   // network to host

// ZOEXDR convertion filter
//
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int32_t  g_local_endian;

void ZOEXDR_init(void);


// ZOEXDR convertion filter
//
STATIC_INLINE int64_t ZOEXDR_int_64(int direction, int64_t value)
{
    if (ZOEXDR_ENDIAN_NETWORK == g_local_endian)
    {
        return (value);
    }
    else
    {
        return (((value & 0x00000000000000ffLL) << 56) | 
                ((value & 0x000000000000ff00LL) << 40) | 
                ((value & 0x0000000000ff0000LL) << 24) | 
                ((value & 0x00000000ff000000LL) << 8)  |
                ((value & 0x000000ff00000000LL) >> 8)  |
                ((value & 0x0000ff0000000000LL) >> 24) |
                ((value & 0x00ff000000000000LL) >> 40) |
                ((value & 0xff00000000000000LL) >> 56)
                );
    }
}

STATIC_INLINE int32_t ZOEXDR_int_32(int direction, int32_t value)
{
    if (ZOEXDR_ENDIAN_NETWORK == g_local_endian)
    {
        return (value);
    }
    else
    {
        return (((value & 0x000000ff) << 24) | 
                ((value & 0x0000ff00) << 8) | 
                ((value & 0x00ff0000) >> 8) | 
                ((value & 0xff000000) >> 24)
                );
    }
}

STATIC_INLINE int16_t ZOEXDR_int_16(int direction, int16_t value)
{
    if (ZOEXDR_ENDIAN_NETWORK == g_local_endian)
    {
        return (value);
    }
    else
    {
        return (((value & 0x00ff) << 8) | 
                ((value & 0xff00) >> 8)
                );
    }
}

STATIC_INLINE int ZOEXDR_int(int direction, int value)
{
    if (ZOEXDR_ENDIAN_NETWORK == g_local_endian)
    {
        return (value);
    }
    else
    {
        return (((value & 0x000000ff) << 24) | 
                ((value & 0x0000ff00) << 8) | 
                ((value & 0x00ff0000) >> 8) | 
                ((value & 0xff000000) >> 24)
                );
    }
}

#if !defined(ZOE_LINUXKER_BUILD) && !defined(ZOE_WINKER_BUILD)
STATIC_INLINE float ZOEXDR_float(int direction, float value)
{
    if (ZOEXDR_ENDIAN_NETWORK == g_local_endian)
    {
        return (value);
    }
    else
    {
        return ((float) ((((int64_t)value & 0x00000000000000ffLL) << 56) |
                (((int64_t)value & 0x000000000000ff00LL) << 40) | 
                (((int64_t)value & 0x0000000000ff0000LL) << 24) | 
                (((int64_t)value & 0x00000000ff000000LL) << 8)  |
                (((int64_t)value & 0x000000ff00000000LL) >> 8)  |
                (((int64_t)value & 0x0000ff0000000000LL) >> 24) |
                (((int64_t)value & 0x00ff000000000000LL) >> 40) |
                (((int64_t)value & 0xff00000000000000LL) >> 56)
                ));
    }
}

STATIC_INLINE double ZOEXDR_double(int direction, double value)
{
    if (ZOEXDR_ENDIAN_NETWORK == g_local_endian)
    {
        return (value);
    }
    else
    {
        return ((double) ((((int64_t)value & 0x00000000000000ffLL) << 56) |
                (((int64_t)value & 0x000000000000ff00LL) << 40) | 
                (((int64_t)value & 0x0000000000ff0000LL) << 24) | 
                (((int64_t)value & 0x00000000ff000000LL) << 8)  |
                (((int64_t)value & 0x000000ff00000000LL) >> 8)  |
                (((int64_t)value & 0x0000ff0000000000LL) >> 24) |
                (((int64_t)value & 0x00ff000000000000LL) >> 40) |
                (((int64_t)value & 0xff00000000000000LL) >> 56)
                ));
    }
}
#endif // !ZOE_LINUXKER_BUILD && !ZOE_WINKER_BUILD

#ifdef __cplusplus
}
#endif //__cplusplus

#define ZOEXDR_char(direction, value) value

#endif //__ZOE_XDR_H__
