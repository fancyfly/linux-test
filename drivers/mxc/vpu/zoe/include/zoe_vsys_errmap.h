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
// zoe_vsys_errmap.h
//
// Description: 
//
//  error code mapping between ZOE and video sub system
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_VSYS_ERRMAP_H__
#define __ZOE_VSYS_ERRMAP_H__

#include "zoe_types.h"
#include "vpu_if.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


STATIC_INLINE vpu_ret_t zoe_2_vsys_status(zoe_errs_t err)
{
	switch (err)
	{
		case ZOE_ERRS_SUCCESS:  return (VPU_RET_OK);
		case ZOE_ERRS_PENDING:  return (VPU_RET_OK);
		case ZOE_ERRS_TIMEOUT:  return (VPU_RET_NONFATAL);
		case ZOE_ERRS_NOMEMORY: return (VPU_RET_OUT_OF_MEMORY);
		case ZOE_ERRS_INVALID:	return (VPU_RET_PRECONDITION);
		case ZOE_ERRS_PARMS:    return (VPU_RET_INVALID_PARAM);
		case ZOE_ERRS_NOTFOUND: return (VPU_RET_NONFATAL);
		case ZOE_ERRS_FAIL:     return (VPU_RET_ERROR);
		case ZOE_ERRS_NOTIMPL:  return (VPU_RET_NOT_SUPPORTED);
		case ZOE_ERRS_NOTSUPP:  return (VPU_RET_NOT_SUPPORTED);
        case ZOE_ERRS_BUSY:     return (VPU_RET_BUFFER_IN_USE);
		case ZOE_ERRS_CANCELLED:return (VPU_RET_INVALID_BUFFER);
        case ZOE_ERRS_AGAIN:    return (VPU_RET_OUT_OF_BUFFERS); 
        case ZOE_ERRS_INTERNAL: return (VPU_RET_ERROR);
		default:                return (VPU_RET_ERROR);
	}
}



STATIC_INLINE zoe_errs_t vsys_2_zoe_status(vpu_ret_t ret)
{
	switch (ret)
	{
        case VPU_RET_OK:               return (ZOE_ERRS_SUCCESS);
        case VPU_RET_ERROR:            return (ZOE_ERRS_FAIL);
        case VPU_RET_OUT_OF_MEMORY:    return (ZOE_ERRS_NOMEMORY);
        case VPU_RET_OUT_OF_BUFFERS:   return (ZOE_ERRS_AGAIN);
        case VPU_RET_NONFATAL:         return (ZOE_ERRS_FAIL);
        case VPU_RET_FATAL:            return (ZOE_ERRS_FAIL);
        case VPU_RET_INVALID_PARAM:    return (ZOE_ERRS_PARMS);
        case VPU_RET_INVALID_HANDLE:   return (ZOE_ERRS_PARMS);
        case VPU_RET_INVALID_BUFFER:   return (ZOE_ERRS_CANCELLED);
        case VPU_RET_BUFFER_IN_USE:    return (ZOE_ERRS_BUSY);
        case VPU_RET_NOT_SUPPORTED:    return (ZOE_ERRS_NOTSUPP);
        case VPU_RET_PRECONDITION:     return (ZOE_ERRS_INVALID);
		default:                       return (ZOE_ERRS_FAIL);
	}
}


#ifdef __cplusplus
}
#endif //__cplusplus


#endif //__ZOE_VSYS_ERRMAP_H__

