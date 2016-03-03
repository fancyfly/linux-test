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
// zv_avlib.h
//
// Description: 
//
//   Header file for vpu streaming core library interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZV_AVLIB_H__
#define __ZV_AVLIB_H__


/////////////////////////////////////////////////////////////////////////////
//
//

#include "zoe_types.h"
#include "zv_genericdevice.h"
#include "zv_codec.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

// Maximum queue depth, need to be cohesive between the library
// and the client.
//

#define ZV_AVLIB_MAX_DATA_ENTRIES   256



/////////////////////////////////////////////////////////////////////////////
//
//

typedef enum _ZV_AVLIB_IMAGE_TYPE
{
    ZVAVLIB_IMAGE_BOOT_CODE = 0,
    ZVAVLIB_IMAGE_CUSTOM_BOOT,
    ZVAVLIB_IMAGE_SFW,
    ZVAVLIB_IMAGE_FW,
    ZVAVLIB_IMAGE_MAX
} ZV_AVLIB_IMAGE_TYPE, *PZV_AVLIB_IMAGE_TYPE;



// interface specific initialization data
//
typedef struct _ZV_AVLIB_INITDATA
{
	// zvavlib portion
	//
	zoe_bool_t				bSecureMode;
	zoe_bool_t				bDownloadFW[ZVAVLIB_IMAGE_MAX];
	uint8_t			        *pFW[ZVAVLIB_IMAGE_MAX];
	uint32_t			    FWSize[ZVAVLIB_IMAGE_MAX];

	// zvcodec portion
	//
	ZVCODEC_INITDATA		codecInitData;

} ZV_AVLIB_INITDATA, *PZV_AVLIB_INITDATA;




/////////////////////////////////////////////////////////////////////////////
//
//

// property supported by this interface
//



/////////////////////////////////////////////////////////////////////////////
//
//


// codec cllback command code
//



/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __ICODECLIB_FWD_DEFINED__
#define __ICODECLIB_FWD_DEFINED__
typedef struct i_zv_av_lib i_zv_av_lib;
#endif //__ICODECLIB_FWD_DEFINED__

    struct i_zv_av_lib
    {
		// i_zv_generic_device
		DECLARE_IZVGENERICDEVICE(i_zv_av_lib)

		// i_zv_av_lib
		zoe_errs_t (*get_codec)(i_zv_av_lib *This, 
							   i_zv_codec **ppCodec
							   );
		zoe_errs_t (*disable)(i_zv_av_lib *This);
		zoe_errs_t (*enable)(i_zv_av_lib *This);
    };



/////////////////////////////////////////////////////////////////////////////
//
//

// Codec Library entry points
//
zoe_errs_t zv_init_av_library(zoe_void_ptr_t pContext1,	//PDEVICE_OBJECT
						      zoe_void_ptr_t pContext2,	//PDEVICE_OBJECT
						      PZV_AVLIB_INITDATA pInitData,
						      i_zv_av_lib **pp_i_zv_av_lib
						      );
zoe_errs_t zv_done_av_library(i_zv_av_lib *p_i_zv_av_lib);


/////////////////////////////////////////////////////////////////////////////
//
//

#define i_zv_av_lib_get_codec(This, ppCodec) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->get_codec(This, ppCodec)))

#define i_zv_av_lib_disable(This) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->disable(This)))

#define i_zv_av_lib_enable(This) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->enable(This)))

#ifdef __cplusplus
}
#endif

#endif //__ZV_AVLIB_H__





