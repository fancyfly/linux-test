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
// czvcodec.h
//
// Description: 
//
//  This is the definations of the codec chip
// 
// Authors: (dt) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CZVCODEC_H__
#define __CZVCODEC_H__


#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_hal.h"
#include "zoe_sosal.h"
#include "zv_codec.h"
#include "zoe_cobject.h"
#include "zoe_cobjectmgr.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

enum CZVCODEC_POWERSTATE
{   
    ZVCODEC_PWR_STATE_D0	        = 0,	// on
    ZVCODEC_PWR_STATE_D1	        ,		// stand by
    ZVCODEC_PWR_STATE_D2	        ,		// hibernate
    ZVCODEC_PWR_STATE_D3			        // off
};



/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __CZVCODEC_FWD_DEFINED__
#define __CZVCODEC_FWD_DEFINED__
typedef struct c_zv_codec c_zv_codec;
#endif //__CZVCODEC_FWD_DEFINED__

struct c_zv_codec
{
	// public c_object
	//
	c_object			m_Object;

	// public i_zv_codec
	//
	i_zv_codec			m_iZVCodec;

	// c_zv_codec
	//
	ZV_DEVICE_CALLBACK	m_pDeviceCallback;				// codec client call back function
	zoe_void_ptr_t		m_callbackContext;				// codec client call back function context
	c_object_mgr		*m_pTaskMgr;	                // task manager
    c_object_mgr        *m_pChannelMgr;                 // channel manager
	uint32_t            m_State;                        // codec state
	uint32_t		    m_PowerState;                   // power state
    IZOEHALAPI          *m_pHal;                        // HAL pointer
    zoe_dbg_comp_id_t   m_dbgID;                        // debug ID
};


// constructor
//
c_zv_codec * c_zv_codec_constructor(c_zv_codec *pZVCodec,
								    c_object *pParent,
								    uint32_t dwAttributes,
                                    IZOEHALAPI *pHal,
                                    zoe_dbg_comp_id_t dbgID
								    );

// destructor
//
void c_zv_codec_destructor(c_zv_codec *This);

zoe_errs_t c_zv_codec_power_up(c_zv_codec *This);
zoe_errs_t c_zv_codec_power_down(c_zv_codec *This);

// DMA
//
zoe_errs_t c_zv_codec_dma_write(c_zv_codec *This, 
								zoe_dev_mem_t ulFwAddr,
								uint8_t * pHostAddr,
								uint32_t ulLength,
								zoe_bool_t bSwap,
								zoe_bool_t bSync,
								uint32_t ulXferMode,
                                zoe_sosal_obj_id_t evt
								);
zoe_errs_t c_zv_codec_dma_read(c_zv_codec *This, 
							   zoe_dev_mem_t ulFwAddr,
							   uint8_t * pHostAddr,
							   uint32_t ulLength,
							   zoe_bool_t bSwap,
							   zoe_bool_t bSync,
							   uint32_t ulXferMode,
                               zoe_sosal_obj_id_t evt
							   );

// helper
//
zoe_bool_t c_zv_codec_is_codec_idle(c_zv_codec *This);
zoe_bool_t c_zv_codec_is_codec_open(c_zv_codec *This);
void c_zv_codec_clr_codec_error(c_zv_codec *This);
zoe_bool_t c_zv_codec_is_codec_error(c_zv_codec *This);


#ifdef __cplusplus
}
#endif

#endif //__CZVCODEC_H__


