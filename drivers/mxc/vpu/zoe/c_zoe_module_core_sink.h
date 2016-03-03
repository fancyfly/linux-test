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
// c_zoe_module_core_sink.h
//
// Description: 
//
//  ZOE streaming core sink module
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __C_ZOE_MODULE_CORE_SINK_H__
#define __C_ZOE_MODULE_CORE_SINK_H__


#include "zoe_module_base.h"
#include "cchannel.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __ZOE_MODULE_CORE_SINK_FWD_DEFINED__
#define __ZOE_MODULE_CORE_SINK_FWD_DEFINED__
typedef struct c_zoe_module_core_sink c_zoe_module_core_sink;
#endif //__ZOE_MODULE_CORE_SINK_FWD_DEFINED__

/////////////////////////////////////////////////////////////////////////////
//
//

#define ZOE_MODULE_CORE_SINK_NUM_INPUT      ZV_CODEC_OUT_NUM
#define ZOE_MODULE_CORE_SINK_NUM_OUTPUT     0
#define ZOE_MODULE_CORE_SINK_NUM_EXTRA      0
#define ZOE_MODULE_CORE_SINK_TIMEOUT_US     -1

/////////////////////////////////////////////////////////////////////////////
//
//

// firmware buffer
//
typedef struct _FW_BUFFER
{
    // valid flag
    zoe_bool_t              valid;

    // release pointer and size
    zoe_dev_mem_t           rel_buf_ptr[3];
    uint32_t                rel_size[3];

	// status of the firmware buffer
	//
	zoe_errs_t              err;

	// buffer descriptor for this firmware buffer
	//
    ZOE_BUFFER_DESCRIPTOR   buf_desc;   // pending firmware buffer

} FW_BUFFER, *PFW_BUFFER;


// user buffer
//
typedef struct _USER_BUFFER
{
    uint8_t                 *p_buffer[3];
    uint32_t                length[3];

	// status of the user buffer
	//
	zoe_errs_t              err;

	// buffer descriptor for this user buffer
	//
    PZV_BUFFER_DESCRIPTOR   p_buf_desc;

} USER_BUFFER, *PUSER_BUFFER;


// task channel data
//
typedef struct _CHANNEL_DATA
{
    zoe_sosal_obj_id_t  m_critical_section;
    USER_BUFFER         m_user_buffer;  // pending user buffer
    FW_BUFFER           m_fw_buffer;    // pending firmware buffer
    c_channel           *m_p_channel;   // channel
} CHANNEL_DATA, *PCHANNEL_DATA;


/////////////////////////////////////////////////////////////////////////////
//
//

// zoe sink module
//
struct c_zoe_module_core_sink
{
    // c_zoe_module_base
    //
    c_zoe_module_base   m_base;

    // c_zoe_module_core_sink
    //
    CHANNEL_DATA        m_channels[ZOE_MODULE_CORE_SINK_NUM_INPUT];
};


/////////////////////////////////////////////////////////////////////////////
//
//

// interfaces for local cpu
//
zoe_errs_t c_zoe_module_core_sink_open(c_zoe_module_core_sink *This,
							           ZV_CODEC_OPEN_TYPE data_type,
							           c_channel *p_channel
							           );
zoe_errs_t c_zoe_module_core_sink_close(c_zoe_module_core_sink *This,
							            ZV_CODEC_OPEN_TYPE data_type
							            );
zoe_errs_t c_zoe_module_core_sink_cancel_buffer(c_zoe_module_core_sink *This,
						                        ZV_CODEC_OPEN_TYPE data_type,
                                                PZV_BUFFER_DESCRIPTOR pBufDesc
                                                );
zoe_bool_t c_zoe_module_core_sink_new_buffer(c_zoe_module_core_sink *This,
						                     ZV_CODEC_OPEN_TYPE data_type
                                             );
int32_t c_zoe_module_core_sink_get_port_id(ZV_CODEC_OPEN_TYPE data_type);

#ifdef __cplusplus
}
#endif

#endif //#define __C_ZOE_MODULE_CORE_SINK_H__



