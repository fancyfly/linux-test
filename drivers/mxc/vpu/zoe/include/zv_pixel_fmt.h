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
// zv_pixel_fmt.h
//
// Description: 
//
//  VPU fourcc formats
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZV_PIXEL_FMT_H__
#define __ZV_PIXEL_FMT_H__


#define VPU_PIX_FMT_AVS     v4l2_fourcc('A', 'V', 'S', '0') // AVS video
#define VPU_PIX_FMT_ASP     v4l2_fourcc('A', 'S', 'P', '0') // MPEG4 ASP video
#define VPU_PIX_FMT_RV8     v4l2_fourcc('R', 'V', '8', '0') // RV8 video
#define VPU_PIX_FMT_RV9     v4l2_fourcc('R', 'V', '9', '0') // RV9 video
#define VPU_PIX_FMT_VP6     v4l2_fourcc('V', 'P', '6', '0') // VP6 video
#define VPU_PIX_FMT_VP7     v4l2_fourcc('V', 'P', '7', '0') // VP7 video
#define VPU_PIX_FMT_SPK     v4l2_fourcc('S', 'P', 'K', '0') // VP6 video
#define VPU_PIX_FMT_HEVC    v4l2_fourcc('H', 'E', 'V', 'C') // H.265 HEVC video
#define VPU_PIX_FMT_VP9     v4l2_fourcc('V', 'P', '9', '0') // VP9 video
#define VPU_PIX_FMT_LOGO    v4l2_fourcc('L', 'O', 'G', 'O') // logo


#endif //__ZV_PIXEL_FMT_H__





