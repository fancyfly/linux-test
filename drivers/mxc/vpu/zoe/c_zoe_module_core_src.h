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
// c_zoe_module_core_src.h
//
// Description: 
//
//  ZOE streaming core source module
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __C_ZOE_MODULE_CORE_SRC_H__
#define __C_ZOE_MODULE_CORE_SRC_H__


#include "zoe_module_base.h"
#include "zoe_ipc_def.h"
#include "cchannel.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//
#ifndef __ZOE_MODULE_CORE_SOURCE_FWD_DEFINED__
#define __ZOE_MODULE_CORE_SOURCE_FWD_DEFINED__
typedef struct c_zoe_module_core_source c_zoe_module_core_source;
#endif //__ZOE_MODULE_CORE_SOURCE_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

#define ZOE_MODULE_CORE_SOURCE_NUM_INPUT    0
#define ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT   ZV_CODEC_DATA_NUM
#define ZOE_MODULE_CORE_SOURCE_NUM_EXTRA    ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT
#define ZOE_MODULE_CORE_SOURCE_TIMEOUT_US   -1


/////////////////////////////////////////////////////////////////////////////
//
//

// zoe core source module
//
struct c_zoe_module_core_source
{
    // c_zoe_module_base
    //
    c_zoe_module_base   m_base;

    // c_zoe_module_core_source
    //
    c_channel           *m_p_channels[ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT];
    zoe_sosal_obj_id_t  m_critical_section[ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT];
};


/////////////////////////////////////////////////////////////////////////////
//
//

// interfaces for local cpu
//
zoe_errs_t c_zoe_module_core_source_open(c_zoe_module_core_source *This,
							             ZV_CODEC_OPEN_TYPE data_type,
							             c_channel *p_channel
							             );
zoe_errs_t c_zoe_module_core_source_close(c_zoe_module_core_source *This,
							              ZV_CODEC_OPEN_TYPE data_type
							              );
zoe_errs_t c_zoe_module_core_source_cancel_buffer(c_zoe_module_core_source *This,
						                          ZV_CODEC_OPEN_TYPE data_type,
                                                  PZV_BUFFER_DESCRIPTOR pBufDesc
                                                  );
zoe_bool_t c_zoe_module_core_source_new_buffer(c_zoe_module_core_source *This,
						                       ZV_CODEC_OPEN_TYPE data_type
                                               );
int32_t c_zoe_module_core_source_get_port_id(ZV_CODEC_OPEN_TYPE data_type);
zoe_errs_t c_zoe_module_core_source_get_fw_addr(c_zoe_module_core_source *This,
						                        ZV_CODEC_OPEN_TYPE data_type,
                                                ZOE_IPC_CPU *p_cpu_id,
                                                uint32_t *p_module,
                                                uint32_t *p_inst
                                                );
#ifdef __cplusplus
}
#endif

#endif //#define __C_ZOE_MODULE_CORE_SRC_H__



