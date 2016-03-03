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
// ctask.h
//
// Description: 
//
//	Header file to define the task that runs the streams
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CTASK_H__
#define __CTASK_H__


#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_cthread.h"
#include "zoe_cfifo.h"
#include "cchannel.h"
#include "czvcodec.h"
#include "zoe_ipc_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

#define MAX_TASK_CHANNEL    ZV_CODEC_OPEN_END

// task state
//
typedef enum _TASK_STATE
{
    TASK_STATE_IDLE = 0,
    TASK_STATE_RUN,
    TASK_STATE_STOP
} TASK_STATE, *PTASK_STATE;


// module node
//
typedef struct _TASK_MODULE_NODE
{
    zoe_bool_t  valid;
    uint32_t    module;
    uint32_t    inst;
    uint32_t    cpu;
} TASK_MODULE_NODE, *PTASK_MODULE_NODE;


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __ZOE_MODULE_CORE_SOURCE_FWD_DEFINED__
#define __ZOE_MODULE_CORE_SOURCE_FWD_DEFINED__
typedef struct c_zoe_module_core_source c_zoe_module_core_source;
#endif //__ZOE_MODULE_CORE_SOURCE_FWD_DEFINED__

#ifndef __ZOE_MODULE_CORE_SINK_FWD_DEFINED__
#define __ZOE_MODULE_CORE_SINK_FWD_DEFINED__
typedef struct c_zoe_module_core_sink c_zoe_module_core_sink;
#endif //__ZOE_MODULE_CORE_SINK_FWD_DEFINED__

#ifndef __CTASK_FWD_DEFINED__
#define __CTASK_FWD_DEFINED__
typedef struct c_task c_task;
#endif // !__CTASK_FWD_DEFINED__

/////////////////////////////////////////////////////////////////////////////
//
//

struct c_task
{                        
	// public c_object
	//
	c_object                    m_Object;

	// c_task
	//
    zoe_errs_t (*alloc)(c_task *This, 
                        ZOE_OBJECT_HANDLE hTask
                        );
    zoe_errs_t (*release)(c_task *This);
	zoe_errs_t (*open)(c_task *This, 
					   ZV_CODEC_OPEN_TYPE dataType,
					   CHANNEL_DIRECTION direction,
					   c_channel *pChannel
					   );
	zoe_errs_t (*close)(c_task *This, 
						ZV_CODEC_OPEN_TYPE dataType,
                        zoe_bool_t critical
						);
	zoe_errs_t (*start)(c_task *This, 
						ZV_CODEC_OPEN_TYPE dataType
						);
	zoe_errs_t (*stop)(c_task *This, 
					   ZV_CODEC_OPEN_TYPE dataType
					   );
	zoe_errs_t (*acquire)(c_task *This, 
						  ZV_CODEC_OPEN_TYPE dataType
						  );
	zoe_errs_t (*pause)(c_task *This, 
						ZV_CODEC_OPEN_TYPE dataType
						);
	zoe_errs_t (*resume)(c_task *This, 
						 ZV_CODEC_OPEN_TYPE dataType
						 );
	zoe_bool_t (*cancel_buffer)(c_task *This, 
						        ZV_CODEC_OPEN_TYPE dataType,
							    PZV_BUFFER_DESCRIPTOR pBufDesc
							    );
	zoe_bool_t (*new_buffer)(c_task *This, 
						     ZV_CODEC_OPEN_TYPE dataType
						     );
	zoe_bool_t (*flush)(c_task *This, 
					    ZV_CODEC_OPEN_TYPE dataType
					    );
    zoe_errs_t (*get_fw_addr)(c_task *This,
						      ZV_CODEC_OPEN_TYPE data_type,
                              ZOE_IPC_CPU *p_cpu_id,
                              uint32_t *p_module,
                              uint32_t *p_inst
                              );

	c_zv_codec                  *m_pZVCodec;
    IZOEHALAPI                  *m_pHal;
    zoe_dbg_comp_id_t           m_dbgID;
	ZOE_OBJECT_HANDLE	        m_hTask;		// handle to this task
    ZV_CODEC_TASK_TYPE          m_type;         // task type

	// task state management
	//
	uint32_t		            m_dwSession;
	uint32_t		            m_dwStarted;
	uint32_t		            m_dwPaused;
	zoe_bool_t			        m_bAcquired;
	TASK_STATE			        m_State;
	zoe_errs_t			        m_Error;

    // source module
    //
    c_zoe_module_core_source    *m_p_zoe_module_core_source;
    uint32_t                    m_inst_src;
    zoe_bool_t                  m_src_valid;

    // sink module
    //
    c_zoe_module_core_sink      *m_p_zoe_module_core_sink;
    uint32_t                    m_inst_sink;
    zoe_bool_t                  m_sink_valid;

    // firmware modules
    //
    TASK_MODULE_NODE            m_module_viddec;

	// encoder settings
	//

	// encoder infomation
	//

	// decoder settings
	//

	// decoder information
	//

	// txcode settings
	//

    // txcode information
    //

};

/////////////////////////////////////////////////////////////////////////////
//
//

// helpers
//
TASK_STATE c_task_get_task_state(c_task *This);
zoe_bool_t c_task_is_opened(c_task *This);
zoe_bool_t c_task_is_started(c_task *This);

// constructor
//
c_task * c_task_constructor(c_task *pTask, 
						    c_object *pParent,
						    uint32_t dwAttributes,
						    uint32_t dwPriority,
                            ZV_CODEC_TASK_TYPE type,
						    c_zv_codec *pZVCodec,
                            IZOEHALAPI *pHal,
                            zoe_dbg_comp_id_t dbgID
						    );

// destructor
//
void c_task_destructor(c_task *This);


#ifdef __cplusplus
}
#endif

#endif //__CTASK_H__



