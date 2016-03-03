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
// zv_devcntl.h
//
// Description: 
//
//   This is the definition of the zv codec chip control
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZV_DEVCNTL_H__
#define __ZV_DEVCNTL_H__

#include "zoe_types.h"

// Property set for ZV_CODEC_CONTROL
//

// {983F8E69-B424-4d49-B847-C8557311E713}
static const ZOE_GUID PROPSETID_ZV_CODEC_CONTROL = 
{ 0x983f8e69, 0xb424, 0x4d49, { 0xb8, 0x47, 0xc8, 0x55, 0x73, 0x11, 0xe7, 0x13 } };


typedef enum _PROPERTY_ZV_CODEC_CONTROL
{                                   
    // decoder control
    //
    ZV_CODEC_PROP_DEC_FMT               = 0,        // Get & Set
    ZV_CODEC_PROP_DEC_MODE              = 1,        // Get & Set
    ZV_CODEC_PROP_DEC_ORDER             = 2,        // Get & Set
    ZV_CODEC_PROP_DEC_YUV_FMT           = 3,        // Get
    ZV_CODEC_PROP_DEC_YUV_PIXEL_FMT     = 4,        // Set
    ZV_CODEC_PROP_DEC_MIN_FRAME_BUF     = 5,        // Get
    ZV_CODEC_PROP_DEC_VBV_SIZE          = 6,        // Get
    ZV_CODEC_PROP_DEC_CROP              = 7,        // Get

    // encoder control
    //

    // test control 
    //
	ZV_CODEC_PROP_TEST_CONTROL          = 80,		// Get & Set

} PROPERTY_ZV_CODEC_CONTROL;



#pragma pack(4)


// ZV_CODEC_PROP_TEST_CONTROL
//
typedef struct _ZV_CODEC_TEST_CONTROL
{
	uint32_t    t1;
	uint32_t    t2;
} ZV_CODEC_TEST_CONTROL, *PZV_CODEC_TEST_CONTROL;



#pragma pack()


#endif //__ZV_DEVCNTL_H__



