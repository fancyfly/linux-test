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
// zv_genericdevice.h
//
// Description: 
//
//   This defines the generic device
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZV_GENERICDEVICE_H__
#define __ZV_GENERICDEVICE_H__


#include "zoe_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

// callback fuction type
//
typedef zoe_errs_t (*ZV_DEVICE_CALLBACK)(zoe_void_ptr_t Context, uint32_t dwCode, zoe_void_ptr_t pParam);


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __IZV_GENERICDEVICE_FWD_DEFINED__
#define __IZV_GENERICDEVICE_FWD_DEFINED__
typedef struct i_zv_generic_device i_zv_generic_device;
#endif //__IZV_GENERICDEVICE_FWD_DEFINED__

    struct i_zv_generic_device
    {
		zoe_errs_t (*init_device)(i_zv_generic_device *This, 
								  ZV_DEVICE_CALLBACK pFuncCallback,
								  zoe_void_ptr_t context,
								  zoe_void_ptr_t pInitData
								  );
		zoe_errs_t (*release)(i_zv_generic_device *This);
		zoe_errs_t (*reset)(i_zv_generic_device *This);
		zoe_errs_t (*set)(i_zv_generic_device *This, 
						  ZOE_REFGUID guid,
						  ZOE_OBJECT_HANDLE hIndex,
						  uint32_t dwCode,
						  zoe_void_ptr_t pSettings,
						  zoe_void_ptr_t pExtra,
						  uint32_t dwSize
						  );
		zoe_errs_t (*get)(i_zv_generic_device *This, 
						  ZOE_REFGUID guid,
						  ZOE_OBJECT_HANDLE hIndex,
						  uint32_t dwCode,
						  zoe_void_ptr_t pSettings,
						  zoe_void_ptr_t pExtra,
						  uint32_t * pdwSizeGot
						  );
    };

#define DECLARE_IZVGENERICDEVICE(Type) \
		zoe_errs_t (*init_device)(Type *This,\
								  ZV_DEVICE_CALLBACK pFuncCallback,\
								  zoe_void_ptr_t context,\
								  zoe_void_ptr_t pInitData\
								  );\
		zoe_errs_t (*release)(Type *This);\
		zoe_errs_t (*reset)(Type *This);\
		zoe_errs_t (*set)(Type *This,\
						  ZOE_REFGUID guid,\
						  ZOE_OBJECT_HANDLE hIndex,\
						  uint32_t dwCode,\
						  zoe_void_ptr_t pInput,\
						  zoe_void_ptr_t pOutput,\
						  uint32_t dwSize\
						  );\
		zoe_errs_t (*get)(Type *This,\
						  ZOE_REFGUID guid,\
						  ZOE_OBJECT_HANDLE hIndex,\
						  uint32_t dwCode,\
						  zoe_void_ptr_t pInput,\
						  zoe_void_ptr_t pOutput,\
						  uint32_t * pdwSizeGot\
						  );


/////////////////////////////////////////////////////////////////////////////
//
//
#define i_zv_generic_device_init_device(This, pFuncCallback, context, pInitData) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->init_device(This, pFuncCallback, context, pInitData)))

#define i_zv_generic_device_release(This) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->release(This)))

#define i_zv_generic_device_reset(This) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->reset(This)))

#define i_zv_generic_device_set(This, guid, hIndex, dwCode, pInput, pOutput, dwSize) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->set(This, guid, hIndex, dwCode, pInput, pOutput, dwSize)))

#define i_zv_generic_device_get(This, guid, hIndex, dwCode, pInput, pOutput, pdwSizeGot) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->get(This, guid, hIndex, dwCode, pInput, pOutput, pdwSizeGot)))


#ifdef __cplusplus
}
#endif

#endif //__ZV_GENERICDEVICE_H__

