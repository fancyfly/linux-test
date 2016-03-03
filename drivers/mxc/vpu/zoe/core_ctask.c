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
// ctask.c
//
// Description: 
//
//	implementation of the task that maintain the streaming
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "ctask.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD
#include "zoe_util.h"
#include "zoe_sosal.h"
#include "zoe_hal.h"
#include "zoe_objids.h"
#include "czvcodec.h"
#include "czvavlib.h"
#include "zoe_module_objids.h"
#include "zoe_module_base.h"
#include "zoe_module_mgr.h"
#include "zoe_module_mgr_intf_clnt.h"
#include "zoe_module_connection_intf_clnt.h"
#include "zoe_module_control_intf_clnt.h"
#include "c_zoe_module_core_src.h"
#include "c_zoe_module_core_sink.h"

//#define _TASK_DO_NOTHING
#if defined(CONFIG_ZV_HPU_EVK) && defined(CONFIG_HOST_PLATFORM_X86_LINUX)
#define TASK_USE_KER_VDEC
#endif //CONFIG_ZV_HPU_EVK && CONFIG_HOST_PLATFORM_X86_LINUX

/////////////////////////////////////////////////////////////////////////////
//
//

static zoe_bool_t c_task_init_tasks(c_task *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
	               "c_task_init_tasks()\n"
	               );

    // task state management
    //
	This->m_dwStarted = 0;
	This->m_dwPaused = 0;
	This->m_bAcquired = ZOE_FALSE;
	This->m_State = TASK_STATE_IDLE;
	This->m_Error = ZOE_ERRS_SUCCESS;
    This->m_src_valid = ZOE_FALSE;
    This->m_sink_valid = ZOE_FALSE;
	memset((void *)&This->m_module_viddec,
		   0,
		   sizeof(TASK_MODULE_NODE)
		   );
    This->m_module_viddec.module = ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VDEC_ADAPTOR);
#ifdef TASK_USE_KER_VDEC
    This->m_module_viddec.cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());
    //This->m_module_viddec.cpu = ZOE_IPC_EXT_USER;
#else //!TASK_USE_KER_VDEC
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
    This->m_module_viddec.cpu = ZOE_IPC_SPU;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
    This->m_module_viddec.cpu = ZOE_IPC_SPU;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
    This->m_module_viddec.cpu = ZOE_IPC_FWPU;
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP
#endif //TASK_USE_KER_VDEC
	return (ZOE_TRUE);
}



static void c_task_done_tasks(c_task *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
	               "c_task_done_tasks()\n"
	               );

	This->release(This);
}



static void c_task_reset_state(c_task *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
	               "c_task_reset_state(%d)\n"
	               );

    // task state management
    //
	This->m_dwStarted = 0;
	This->m_dwPaused = 0;
	This->m_bAcquired = ZOE_FALSE;
	This->m_State = TASK_STATE_IDLE;
	This->m_Error = ZOE_ERRS_SUCCESS;
}



static void c_task_load_default_settings(c_task *This)
{
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_task
//
static zoe_errs_t c_task_alloc(c_task *This, 
                               ZOE_OBJECT_HANDLE hTask
                               )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

    This->m_hTask = hTask;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_task_alloc() hTask(%d)\n",
				   hTask
				   );

	ENTER_CRITICAL(&This->m_Object)

	// load default settings
	//
	c_task_load_default_settings(This);

	// clear all the session information
	//
	This->m_dwSession = 0;

	// restore to default state
	//
	c_task_reset_state(This);

#ifndef _TASK_DO_NOTHING
    // create source
    //
    err = zoe_module_mgr_create_module_clnt(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE), 
                                            &This->m_inst_src, 
                                            cpu, 
                                            ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                            0
                                            );
    if (ZOE_FAIL(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_task_alloc() failed to create (OBJECT_ZOE_MODULE_CORE_SOURCE)! err(%d)\n",
                       err
					   );
        goto c_task_Alloc_exit;
    }
    else
    {
        This->m_src_valid = ZOE_TRUE;
        This->m_p_zoe_module_core_source = (c_zoe_module_core_source *)c_zoe_module_mgr_get_module_inst_data(c_zoe_module_mgr_get_module_mgr(), 
                                                                                                             ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE), 
                                                                                                             This->m_inst_src
                                                                                                             );
    }

#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
    This->m_sink_valid = ZOE_FALSE;
#else //!TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8
    // create sink
    //
    err = zoe_module_mgr_create_module_clnt(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK), 
                                            &This->m_inst_sink, 
                                            cpu, 
                                            ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                            0
                                            );
    if (ZOE_FAIL(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_task_alloc() failed to create (OBJECT_ZOE_MODULE_CORE_SINK)! err(%d)\n",
                       err
					   );
        goto c_task_Alloc_exit;
    }
    else
    {
        This->m_sink_valid = ZOE_TRUE;
        This->m_p_zoe_module_core_sink = (c_zoe_module_core_sink *)c_zoe_module_mgr_get_module_inst_data(c_zoe_module_mgr_get_module_mgr(), 
                                                                                                         ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK), 
                                                                                                         This->m_inst_sink
                                                                                                         );
    }
#endif //TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

#endif //!_TASK_DO_NOTHING


c_task_Alloc_exit:

#ifndef _TASK_DO_NOTHING
    if (ZOE_FAIL(err))
    {
        if (This->m_src_valid)
        {
            zoe_module_mgr_destroy_module_clnt(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE), 
                                               This->m_inst_src, 
                                               cpu, 
                                               ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                               0
                                               );
            This->m_src_valid = ZOE_FALSE;
            This->m_p_zoe_module_core_source = ZOE_NULL;
        }

        if (This->m_sink_valid)
        {
            zoe_module_mgr_destroy_module_clnt(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK), 
                                               This->m_inst_sink, 
                                               cpu, 
                                               ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                               0
                                               );
            This->m_sink_valid = ZOE_FALSE;
            This->m_p_zoe_module_core_sink = ZOE_NULL;
        }
    }
#endif //!_TASK_DO_NOTHING

	LEAVE_CRITICAL(&This->m_Object)

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_task_alloc()\n"
				   );
	return (err);
}



static zoe_errs_t c_task_release(c_task *This)
{
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());
	int         i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_task_release()\n"
                   );

	ENTER_CRITICAL(&This->m_Object)

    // close all the open channels
    //
	for (i = 0; i < MAX_TASK_CHANNEL; i++)
	{
		This->close(This,
					i,
                    ZOE_FALSE
					);
	}
#ifndef _TASK_DO_NOTHING
    // release all the streaming modules
    //
    if (This->m_src_valid)
    {
        zoe_module_mgr_destroy_module_clnt(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE), 
                                           This->m_inst_src, 
                                           cpu, 
                                           ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                           0
                                           );
        This->m_src_valid = ZOE_FALSE;
        This->m_p_zoe_module_core_source = ZOE_NULL;
    }

    if (This->m_sink_valid)
    {
        zoe_module_mgr_destroy_module_clnt(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK), 
                                           This->m_inst_sink, 
                                           cpu, 
                                           ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                           0
                                           );
        This->m_sink_valid = ZOE_FALSE;
        This->m_p_zoe_module_core_sink = ZOE_NULL;
    }

    if (This->m_module_viddec.valid)
    {
        zoe_module_mgr_destroy_module_clnt(This->m_module_viddec.module, 
                                           This->m_module_viddec.inst, 
                                           This->m_module_viddec.cpu, 
                                           ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                           0
                                           );
        This->m_module_viddec.valid = ZOE_FALSE;
    }
#endif //!_TASK_DO_NOTHING

	LEAVE_CRITICAL(&This->m_Object)

	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_task_open(c_task *This,
							  ZV_CODEC_OPEN_TYPE dataType,
							  CHANNEL_DIRECTION direction,
							  c_channel *pChannel
							  )
{
    zoe_errs_t                  err = ZOE_ERRS_SUCCESS;
    int32_t                     port;
    ZOE_MODULE_DATA_CONNECTOR   conn;
    uint32_t                    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_task_open() dataType(%d) dir(%d)\n",
				   dataType,
				   direction
				   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

#ifndef _TASK_DO_NOTHING

	ENTER_CRITICAL(&This->m_Object)

    // create viddec module
    //
    if (!This->m_module_viddec.valid)
    {
        err = zoe_module_mgr_create_module_clnt(This->m_module_viddec.module, 
                                                &This->m_module_viddec.inst, 
                                                This->m_module_viddec.cpu, 
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                                0
                                                );
        if (ZOE_FAIL(err))
        {
	        LEAVE_CRITICAL(&This->m_Object)
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
					       "c_task_open() failed to create %s! err(%d)\n",
                           zoe_module_get_name(This->m_module_viddec.module),
                           err
					       );
            goto c_task_open_exit;
        }
        else
        {
            This->m_module_viddec.valid = ZOE_TRUE;
        }
    }

	LEAVE_CRITICAL(&This->m_Object)

    switch (dataType)
    {
        case ZV_CODEC_VID_IN:
            // connect core_src -> viddec
            //
            port = c_zoe_module_core_source_get_port_id(dataType);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                // source output
                conn.addr.cpu = This->m_module_viddec.cpu;
                conn.addr.module = This->m_module_viddec.module;
                conn.addr.inst = This->m_module_viddec.inst;
                conn.selector = 1;

                err = zoe_module_base_port_set_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                    &conn,
                                                    cpu,
                                                    ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                    This->m_inst_src
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() failed to connect %s to %s! err(%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   err
					               );
                    return (err);
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
					               "c_task_open() connect %s sel(%d) to %s sel (%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   conn.selector,
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START
					               );
                }

                // viddec input
                conn.addr.cpu = cpu;
                conn.addr.module = ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE);
                conn.addr.inst = This->m_inst_src;
                conn.selector = port + ZOE_MODULE_DATA_SEL_PORT_START;

                err = zoe_module_base_port_set_clnt(1,
                                                    &conn,
                                                    This->m_module_viddec.cpu,
                                                    This->m_module_viddec.module,
                                                    This->m_module_viddec.inst
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() failed to connect %s to %s! err(%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
					               );
                    return (err);
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
					               "c_task_open() connect %s sel(%d) to %s sel(1)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   conn.selector,
                                   zoe_module_get_name(This->m_module_viddec.module)
					               );
                }

                // open the port
                err = c_zoe_module_core_source_open(This->m_p_zoe_module_core_source, 
                                                    dataType, 
                                                    pChannel
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() c_zoe_module_core_source_open failed! err(%d)\n",
                                   err
					               );
                    return (err);
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
					               "c_task_open() c_zoe_module_core_source_open dataType(%d)\n",
                                   dataType
					               );
                }
            }
            break;

        case ZV_CODEC_YUV_OUT:
#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            // connect core_src port to vid_dec input
            port = c_zoe_module_core_source_get_port_id(dataType);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                // source output
                conn.addr.cpu = This->m_module_viddec.cpu;
                conn.addr.module = This->m_module_viddec.module;
                conn.addr.inst = This->m_module_viddec.inst;
                conn.selector = 2;

                err = zoe_module_base_port_set_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                    &conn,
                                                    cpu,
                                                    ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                    This->m_inst_src
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() failed to connect %s to %s! err(%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   err
					               );
                    return (err);
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
					               "c_task_open() connect %s sel(%d) to %s sel (%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   conn.selector,
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START
					               );
                }

                // viddec input
                conn.addr.cpu = cpu;
                conn.addr.module = ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE);
                conn.addr.inst = This->m_inst_src;
                conn.selector = port + ZOE_MODULE_DATA_SEL_PORT_START;

                err = zoe_module_base_port_set_clnt(2,
                                                    &conn,
                                                    This->m_module_viddec.cpu,
                                                    This->m_module_viddec.module,
                                                    This->m_module_viddec.inst
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() failed to connect %s to %s! err(%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
					               );
                    return (err);
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
					               "c_task_open() connect %s sel(%d) to %s sel(2)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   conn.selector,
                                   zoe_module_get_name(This->m_module_viddec.module)
					               );
                }

                // open the port
                err = c_zoe_module_core_source_open(This->m_p_zoe_module_core_source, 
                                                    dataType, 
                                                    pChannel
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() c_zoe_module_core_source_open failed! err(%d)\n",
                                   err
					               );
                    return (err);
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   This->m_dbgID,
					               "c_task_open() c_zoe_module_core_source_open dataType(%d)\n",
                                   dataType
					               );
                }
            }
#else // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
            // connect vid_dec output to core_sink port
            port = c_zoe_module_core_sink_get_port_id(ZV_CODEC_YUV_OUT);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                // viddec output
                conn.addr.cpu = cpu;
                conn.addr.module = ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK);
                conn.addr.inst = This->m_inst_sink;
                conn.selector = port + ZOE_MODULE_DATA_SEL_PORT_START;

                err = zoe_module_base_port_set_clnt(2,
                                                    &conn,
                                                    This->m_module_viddec.cpu,
                                                    This->m_module_viddec.module,
                                                    This->m_module_viddec.inst
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() failed to connect %s to %s! err(%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
					               );
                    return (err);
                }

                // sink input
                conn.addr.cpu = This->m_module_viddec.cpu;
                conn.addr.module = This->m_module_viddec.module;
                conn.addr.inst = This->m_module_viddec.inst;
                conn.selector = 2;

                err = zoe_module_base_port_set_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                    &conn,
                                                    cpu,
                                                    ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                    This->m_inst_sink
                                                    );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() failed to connect %s to %s! err(%d)\n",
                                   zoe_module_get_name(conn.addr.module),
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                   err
					               );
                    return (err);
                }

                // open the port
                err = c_zoe_module_core_sink_open(This->m_p_zoe_module_core_sink, 
                                                  dataType, 
                                                  pChannel
                                                  );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
					               "c_task_open() c_zoe_module_core_sink_open failed! err(%d)\n",
                                   err
					               );
                    return (err);
                }
            }
#endif // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            break;
        default:
            break;
    }
#endif //!_TASK_DO_NOTHING

c_task_open_exit:
	// update session bitmap
	//
    if (ZOE_SUCCESS(err))
    {
	    ENTER_CRITICAL(&This->m_Object)
	    This->m_dwSession |= (1 << dataType);
	    LEAVE_CRITICAL(&This->m_Object)
    }
	return (err);
}



static zoe_errs_t c_task_close(c_task *This,
							   ZV_CODEC_OPEN_TYPE dataType,
                               zoe_bool_t critical
							   )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    int32_t     port;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_close() dataType(%d)\n",
                   dataType
                   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

#ifndef _TASK_DO_NOTHING

    switch (dataType)
    {
        case ZV_CODEC_VID_IN:
            // disconnect core_src -> vid_dec
            port = c_zoe_module_core_source_get_port_id(dataType);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                if (This->m_module_viddec.valid)
                {
                    // stop this viddec input port
                    err = zoe_module_base_stop_clnt(1,
                                                    This->m_module_viddec.cpu,
                                                    This->m_module_viddec.module,
                                                    This->m_module_viddec.inst
                                                    );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
				                       "c_task_close() failed to stop %s sel#1 err(%d)\n",
                                       zoe_module_get_name(This->m_module_viddec.module),
                                       err
				                       );
                    }
                }

                if (This->m_src_valid)
                {
                    // stop this source output port
                    err = zoe_module_base_stop_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                    cpu,
                                                    ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                    This->m_inst_src
                                                    );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to stop %s sel#(%d) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                       port + ZOE_MODULE_DATA_SEL_PORT_START,
                                       err
					                   );
                    }

                    // close the port
                    err = c_zoe_module_core_source_close(This->m_p_zoe_module_core_source, 
                                                         dataType
                                                         );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_open() c_zoe_module_core_source_close failed! err(%d)\n",
                                       err
					                   );
                    }

                    // disconnect source output
                    err = zoe_module_base_port_clear_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                          cpu,
                                                          ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                          This->m_inst_src
                                                          );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to disconnect %s err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                       err
					                   );
                    }
                }

                if (This->m_module_viddec.valid)
                {
                    // disconnect viddec input
                    err = zoe_module_base_port_clear_clnt(1,
                                                          This->m_module_viddec.cpu,
                                                          This->m_module_viddec.module,
                                                          This->m_module_viddec.inst
                                                          );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to disconnect %s err(%d)\n",
                                       zoe_module_get_name(This->m_module_viddec.module),
                                       err
					                   );
                    }
                }
            }
            break;

        case ZV_CODEC_YUV_OUT:
#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            port = c_zoe_module_core_source_get_port_id(dataType);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                if (This->m_module_viddec.valid)
                {
                    // stop this viddec input port
                    err = zoe_module_base_stop_clnt(2,
                                                    This->m_module_viddec.cpu,
                                                    This->m_module_viddec.module,
                                                    This->m_module_viddec.inst
                                                    );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
				                       "c_task_close() failed to stop %s sel#1 err(%d)\n",
                                       zoe_module_get_name(This->m_module_viddec.module),
                                       err
				                       );
                    }
                }

                if (This->m_src_valid)
                {
                    // stop this source output port
                    err = zoe_module_base_stop_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                    cpu,
                                                    ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                    This->m_inst_src
                                                    );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to stop %s sel#(%d) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                       port + ZOE_MODULE_DATA_SEL_PORT_START,
                                       err
					                   );
                    }

                    // close the port
                    err = c_zoe_module_core_source_close(This->m_p_zoe_module_core_source, 
                                                         dataType
                                                         );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_open() c_zoe_module_core_source_close failed! err(%d)\n",
                                       err
					                   );
                    }

                    // disconnect source output
                    err = zoe_module_base_port_clear_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                          cpu,
                                                          ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                          This->m_inst_src
                                                          );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to disconnect %s err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                       err
					                   );
                    }
                }

                if (This->m_module_viddec.valid)
                {
                    // disconnect viddec input
                    err = zoe_module_base_port_clear_clnt(2,
                                                          This->m_module_viddec.cpu,
                                                          This->m_module_viddec.module,
                                                          This->m_module_viddec.inst
                                                          );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to disconnect %s err(%d)\n",
                                       zoe_module_get_name(This->m_module_viddec.module),
                                       err
					                   );
                    }
                }
            }
#else // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
            port = c_zoe_module_core_sink_get_port_id(ZV_CODEC_YUV_OUT);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                if (This->m_sink_valid)
                {
                    // stop this sink input port
                    err = zoe_module_base_stop_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                    cpu,
                                                    ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                    This->m_inst_sink
                                                    );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to stop %s sel#(%d) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                       port + ZOE_MODULE_DATA_SEL_PORT_START,
                                       err
					                   );
                    }
                }

                if (This->m_module_viddec.valid)
                {
                    // stop this viddec output port
                    err = zoe_module_base_stop_clnt(2,
                                                    This->m_module_viddec.cpu,
                                                    This->m_module_viddec.module,
                                                    This->m_module_viddec.inst
                                                    );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to stop %s sel#2 err(%d)\n",
                                       zoe_module_get_name(This->m_module_viddec.module),
                                       err
					                   );
                    }
                }

                if (This->m_sink_valid)
                {
                    // close the port
                    err = c_zoe_module_core_sink_close(This->m_p_zoe_module_core_sink, 
                                                       dataType
                                                       );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() c_zoe_module_core_sink_close failed! err(%d)\n",
                                       err
					                   );
                    }
                }

                if (This->m_module_viddec.valid)
                {
                    // disconnect viddec output
                    err = zoe_module_base_port_clear_clnt(2,
                                                          This->m_module_viddec.cpu,
                                                          This->m_module_viddec.module,
                                                          This->m_module_viddec.inst
                                                          );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to disconnect %s err(%d)\n",
                                       zoe_module_get_name(This->m_module_viddec.module),
                                       err
					                   );
                    }
                }

                if (This->m_sink_valid)
                {
                    // disconnect sink input
                    err = zoe_module_base_port_clear_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                          cpu,
                                                          ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                          This->m_inst_sink
                                                          );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_close() failed to disconnect %s err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                       err
					                   );
                    }
                }
            }
#endif // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            break;
        default:
            break;
    }
#endif //!_TASK_DO_NOTHING

	// update session bitmap
	//
    if (critical)
    {
	    ENTER_CRITICAL(&This->m_Object)
    }
	This->m_dwSession &= ~(1 << dataType);

#ifndef _TASK_DO_NOTHING
    // destroy decoder module if the session is closed
    //
    if ((0 == This->m_dwSession) && 
        This->m_module_viddec.valid
        )
    {
        zoe_module_mgr_destroy_module_clnt(This->m_module_viddec.module, 
                                           This->m_module_viddec.inst, 
                                           This->m_module_viddec.cpu, 
                                           ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR), 
                                           0
                                           );
        This->m_module_viddec.valid = ZOE_FALSE;
    }
#endif //!_TASK_DO_NOTHING

    if (critical)
    {
	    LEAVE_CRITICAL(&This->m_Object)
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_task_start(c_task *This,
							   ZV_CODEC_OPEN_TYPE dataType
							   )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    int32_t     port;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_start() dataType(%d)\n",
				   dataType
				   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

#ifndef _TASK_DO_NOTHING
    // start streaming
    //
    switch (dataType)
    {
        case ZV_CODEC_VID_IN:

            if (This->m_module_viddec.valid)
            {
                // start viddec
                err = zoe_module_base_play_clnt(1,
                                                This->m_module_viddec.cpu,
                                                This->m_module_viddec.module,
                                                This->m_module_viddec.inst
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_start() failed to start %s err(%d)\n",
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
				                   );
                    return (err);
                }
            }

            if (This->m_src_valid)
            {
                port = c_zoe_module_core_source_get_port_id(dataType);
                // start source
                err = zoe_module_base_play_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                cpu,
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                This->m_inst_src
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_start() failed to start %s port (%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START,
                                   err
				                   );
                    return (err);
                }
            }
            break;

        case ZV_CODEC_YUV_OUT:
#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
            if (This->m_sink_valid)
            {
                port = c_zoe_module_core_sink_get_port_id(dataType);
                // start sink
                err = zoe_module_base_play_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                cpu,
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                This->m_inst_sink
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_start() failed to start %s port(%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START,
                                   err
				                   );
                    return (err);
                }
            }
#else // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            if (This->m_src_valid)
            {
                port = c_zoe_module_core_source_get_port_id(dataType);
                // start source
                err = zoe_module_base_play_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                cpu,
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                This->m_inst_src
                                                );
                if (ZOE_FAIL(err) &&
                    (ZOE_ERRS_INVALID != err)
                    )
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_start() failed to start %s port(%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START,
                                   err
				                   );
                    return (err);
                }
            }
#endif // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)

            if (This->m_module_viddec.valid)
            {
                // start viddec
                err = zoe_module_base_play_clnt(2,
                                                This->m_module_viddec.cpu,
                                                This->m_module_viddec.module,
                                                This->m_module_viddec.inst
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_start() failed to start %s port(2) err(%d)\n",
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
				                   );
                    return (err);
                }
            }
            break;

        default:
            break;
    }
#endif //!_TASK_DO_NOTHING

	if (ZOE_SUCCESS(err))
	{
	    ENTER_CRITICAL(&This->m_Object)
		This->m_dwStarted |= (1 << dataType);
	    LEAVE_CRITICAL(&This->m_Object)
	}
	return (err);
}



static zoe_errs_t c_task_stop(c_task *This,
							  ZV_CODEC_OPEN_TYPE dataType
							  )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    int32_t     port;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_stop() dataType(%d)\n",
                   dataType
                   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

#ifndef _TASK_DO_NOTHING
    // stop streaming
    //
    switch (dataType)
    {
        case ZV_CODEC_YUV_OUT:
#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
            if (This->m_sink_valid)
            {
                port = c_zoe_module_core_sink_get_port_id(dataType);
                // stop sink
                err = zoe_module_base_stop_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                cpu,
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                This->m_inst_sink
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_stop() failed to stop %s port(%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START,
                                   err
				                   );
                }
            }
#endif // !TASK_USE_KER_VDEC

            if (This->m_module_viddec.valid)
            {
                // stop viddec
                err = zoe_module_base_stop_clnt(2,
                                                This->m_module_viddec.cpu,
                                                This->m_module_viddec.module,
                                                This->m_module_viddec.inst
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_stop() failed to stop %s port(2) err(%d)\n",
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
				                   );
                }
            }

#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            if (This->m_src_valid)
            {
                port = c_zoe_module_core_source_get_port_id(dataType);
                // stop source
                err = zoe_module_base_stop_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                cpu,
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                This->m_inst_src
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_stop() failed to stop %s port(%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START,
                                   err
				                   );
                }
            }
#endif // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            break;

            // fall through
        case ZV_CODEC_VID_IN:
            if (This->m_module_viddec.valid)
            {
                // stop viddec
                err = zoe_module_base_stop_clnt(1,
                                                This->m_module_viddec.cpu,
                                                This->m_module_viddec.module,
                                                This->m_module_viddec.inst
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_stop() failed to stop %s port(1) err(%d)\n",
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
				                   );
                }
            }

            if (This->m_src_valid)
            {
                port = c_zoe_module_core_source_get_port_id(dataType);
                // stop source
                err = zoe_module_base_stop_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                cpu,
                                                ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                This->m_inst_src
                                                );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_stop() failed to stop %s port(%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   port + ZOE_MODULE_DATA_SEL_PORT_START,
                                   err
				                   );
                }
            }
            break;

        default:
            break;
    }
#endif //!_TASK_DO_NOTHING

	// update the bitmap
	//
	ENTER_CRITICAL(&This->m_Object)
	This->m_dwStarted &= ~(1 << dataType);
	LEAVE_CRITICAL(&This->m_Object)

	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
                       "c_task_stop() Failed!(%d) error(%d)\n",
                       err
                       );
	}
    // stop always succeed
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_task_acquire(c_task *This,
								 ZV_CODEC_OPEN_TYPE dataType
								 )
{
	zoe_errs_t  err = ZOE_ERRS_SUCCESS;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_acquire() dataType(%d)\n",
                   dataType
                   );

    if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

	ENTER_CRITICAL(&This->m_Object)
	if (!This->m_bAcquired)
	{
        This->m_bAcquired = ZOE_TRUE;
	}
	LEAVE_CRITICAL(&This->m_Object)

	return (err);
}



static zoe_errs_t c_task_pause(c_task *This,
							   ZV_CODEC_OPEN_TYPE dataType
							   )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    int32_t     port;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_pause() dataType(%d)\n",
                   dataType
                   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

#ifndef _TASK_DO_NOTHING
    // pause streaming
    //
    switch (dataType)
    {
        case ZV_CODEC_VID_IN:
        case ZV_CODEC_YUV_OUT:
#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
            if (This->m_sink_valid)
            {
                port = c_zoe_module_core_sink_get_port_id(dataType);
                // pause sink
                err = zoe_module_base_pause_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                 cpu,
                                                 ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                 This->m_inst_sink
                                                 );
                if (ZOE_FAIL(err) &&
                    (ZOE_ERRS_INVALID != err)
                    )
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_pause() failed to pause %s err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                   err
				                   );
                    return (err);
                }
            }
#endif // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)

            if (This->m_module_viddec.valid)
            {
                // pause viddec
                err = zoe_module_base_pause_clnt((ZV_CODEC_VID_IN == dataType) ? 1 : 2,
                                                 This->m_module_viddec.cpu,
                                                 This->m_module_viddec.module,
                                                 This->m_module_viddec.inst
                                                 );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_pause() failed to pause %s err(%d)\n",
                                   zoe_module_get_name(This->m_module_viddec.module),
                                   err
				                   );
                    return (err);
                }
            }

            if (This->m_src_valid)
            {
                port = c_zoe_module_core_source_get_port_id(dataType);
                // pause source
                err = zoe_module_base_pause_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                 cpu,
                                                 ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                 This->m_inst_src
                                                 );
                if (ZOE_FAIL(err) &&
                    (ZOE_ERRS_INVALID != err)
                    )
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_task_pause() failed to pause %s err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                   err
				                   );
                    return (err);
                }
            }
            break;

        default:
            break;
    }
#endif //!_TASK_DO_NOTHING

	ENTER_CRITICAL(&This->m_Object)
	if (ZOE_SUCCESS(err))
	{
        This->m_dwPaused |= (1 << dataType);
	}
	LEAVE_CRITICAL(&This->m_Object)

	return (err);
}



static zoe_errs_t c_task_resume(c_task *This,
							    ZV_CODEC_OPEN_TYPE dataType
							    )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    int32_t     port;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_resume() dataType(%d)\n",
				   dataType
				   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_ERRS_PARMS);
	}

	ENTER_CRITICAL(&This->m_Object)
	This->m_dwPaused &= ~(1 << dataType);
	if (0 == This->m_dwPaused)
	{
#ifndef _TASK_DO_NOTHING
		// resume streaming
		//
#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        if (This->m_sink_valid)
        {
            port = c_zoe_module_core_sink_get_port_id(dataType);
            // start sink
            err = zoe_module_base_play_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                            cpu,
                                            ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                            This->m_inst_sink
                                            );
            if (ZOE_FAIL(err) &&
                (ZOE_ERRS_INVALID != err)
                )
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_task_resume() failed to start %s err(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                               err
			                   );
                goto c_task_resume_exit;
            }
        }
#endif // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)

        if (This->m_module_viddec.valid)
        {
            // start viddec
            err = zoe_module_base_play_clnt((ZV_CODEC_VID_IN == dataType) ? 1 : 2,
                                            This->m_module_viddec.cpu,
                                            This->m_module_viddec.module,
                                            This->m_module_viddec.inst
                                            );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_task_resume() failed to start %s err(%d)\n",
                               zoe_module_get_name(This->m_module_viddec.module),
                               err
			                   );
                goto c_task_resume_exit;
            }
        }

        if (This->m_src_valid)
        {
            port = c_zoe_module_core_source_get_port_id(dataType);
            // start source
            err = zoe_module_base_play_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                            cpu,
                                            ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                            This->m_inst_src
                                            );
            if (ZOE_FAIL(err) &&
                (ZOE_ERRS_INVALID != err)
                )
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_task_resume() failed to start %s err(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                               err
			                   );
                goto c_task_resume_exit;
            }
        }
#endif //!_TASK_DO_NOTHING
    }

c_task_resume_exit:
	LEAVE_CRITICAL(&This->m_Object)
	return (err);
}



static zoe_bool_t c_task_cancel_buffer(c_task *This,
								       ZV_CODEC_OPEN_TYPE dataType,
								       PZV_BUFFER_DESCRIPTOR pBufDesc
								       )
{
    zoe_errs_t  err = ZOE_ERRS_FAIL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_cancel_buffer() dataType(%d) bufDesc(0x%x)\n",
                   dataType,
                   pBufDesc
                   );

#ifndef _TASK_DO_NOTHING
    switch (dataType)
    {
        case ZV_CODEC_VID_IN:
#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
        case ZV_CODEC_YUV_OUT:
#endif // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            if (This->m_src_valid)
            {
                err = c_zoe_module_core_source_cancel_buffer(This->m_p_zoe_module_core_source, 
                                                             dataType, 
                                                             pBufDesc
                                                             );
            }
            break;

#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        case ZV_CODEC_YUV_OUT:
            if (This->m_sink_valid)
            {
                err = c_zoe_module_core_sink_cancel_buffer(This->m_p_zoe_module_core_sink, 
                                                           dataType, 
                                                           pBufDesc
                                                           );
            }
            break;
#endif // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        default:
            break;
    }
#endif //!_TASK_DO_NOTHING
	return (ZOE_SUCCESS(err));
}



static zoe_bool_t c_task_new_buffer(c_task *This,
						            ZV_CODEC_OPEN_TYPE dataType
                                    )
{
    zoe_bool_t  b_ret = ZOE_TRUE;

#ifndef _TASK_DO_NOTHING
    switch (dataType)
    {
        case ZV_CODEC_VID_IN:
#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
        case ZV_CODEC_YUV_OUT:
#endif // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            if (This->m_src_valid)
            {
                b_ret = c_zoe_module_core_source_new_buffer(This->m_p_zoe_module_core_source, 
                                                            dataType
                                                            );
            }
            break;

#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        case ZV_CODEC_YUV_OUT:
            if (This->m_sink_valid)
            {
                b_ret = c_zoe_module_core_sink_new_buffer(This->m_p_zoe_module_core_sink, 
                                                          dataType
                                                          );
            }
            break;
#endif // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        default:
            break;
    }
#endif //!_TASK_DO_NOTHING
	return (b_ret);
}



static zoe_bool_t c_task_flush(c_task *This,
							   ZV_CODEC_OPEN_TYPE dataType
							   )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    int32_t     port;
    uint32_t    cpu = c_zoe_ipc_service_get_cpu_id(c_zoe_ipc_service_get_ipc_svc());

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "c_task_flush() dataType(%d)[\n",
                   dataType
                   );

	if (dataType > MAX_TASK_CHANNEL)
	{
		return (ZOE_FALSE);
	}

#ifndef _TASK_DO_NOTHING
    switch (dataType)
    {
        case ZV_CODEC_VID_IN:
#if defined(TASK_USE_KER_VDEC) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
        case ZV_CODEC_YUV_OUT:
#endif // TASK_USE_KER_VDEC || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            // flish the source
            port = c_zoe_module_core_source_get_port_id(dataType);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                if (This->m_src_valid)
                {
                    err = zoe_module_base_flush_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                     cpu,
                                                     ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),
                                                     This->m_inst_src
                                                     );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_flush() failed to flush %s sel#(%d) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE)),
                                       port + ZOE_MODULE_DATA_SEL_PORT_START,
                                       err
					                   );
                    }
                }
            }
            break;

#if !defined(TASK_USE_KER_VDEC) && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        case ZV_CODEC_YUV_OUT:
            // flush the sink
            port = c_zoe_module_core_sink_get_port_id(dataType);
            if (-1 == port)
            {
                return (ZOE_ERRS_INTERNAL);
            }
            else
            {
                if (This->m_sink_valid)
                {
                    err = zoe_module_base_flush_clnt(port + ZOE_MODULE_DATA_SEL_PORT_START,
                                                     cpu,
                                                     ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),
                                                     This->m_inst_sink
                                                     );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbgID,
					                   "c_task_flush() failed to flush %s sel#(%d) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK)),
                                       port + ZOE_MODULE_DATA_SEL_PORT_START,
                                       err
					                   );
                    }
                }
            }
            break;
#endif // !TASK_USE_KER_VDEC && (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)

        default:
            break;
    }
#endif //!_TASK_DO_NOTHING

    return (ZOE_SUCCESS(err));
}



static zoe_errs_t c_task_get_fw_addr(c_task *This,
						             ZV_CODEC_OPEN_TYPE dataType,
                                     ZOE_IPC_CPU *p_cpu_id,
                                     uint32_t *p_module,
                                     uint32_t *p_inst
                                     )
{
    zoe_errs_t  err;

    if (This->m_src_valid)
    {
        err = c_zoe_module_core_source_get_fw_addr(This->m_p_zoe_module_core_source, 
                                                   dataType,
                                                   p_cpu_id,
                                                   p_module,
                                                   p_inst
                                                   );
    }
    else
    {
        err = ZOE_ERRS_INVALID;
    }
    return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// helpers
//
TASK_STATE c_task_get_task_state(c_task *This)
{
	return (This->m_State);
}							  


zoe_bool_t c_task_is_opened(c_task *This)
{
	return (0 != This->m_dwSession);
}



zoe_bool_t c_task_is_started(c_task *This)
{
	return (c_task_is_opened(This) ? (0 != This->m_dwStarted) : ZOE_FALSE);
}



/////////////////////////////////////////////////////////////////////////////
//
//

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
						    )
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   dbgID,
				   "c_task_constructor()\n" 
				   );

	if (pTask)
	{
        // zero init ourselves
        //
        memset((void *)pTask, 
               0, 
               sizeof(c_task)
               );

		c_object_constructor(&pTask->m_Object, 
							 pParent, 
                             OBJECT_ZOE_TASK,
		  					 dwAttributes
		  					 );

		// c_task
        //
        pTask->m_type = type;

		// fill in the functions table
		pTask->alloc = c_task_alloc;
		pTask->release = c_task_release;
		pTask->open = c_task_open;
		pTask->close = c_task_close;
		pTask->start = c_task_start;
		pTask->stop = c_task_stop;
		pTask->acquire = c_task_acquire;
		pTask->pause = c_task_pause;
		pTask->resume = c_task_resume;
		pTask->cancel_buffer = c_task_cancel_buffer;
		pTask->new_buffer = c_task_new_buffer;
		pTask->flush = c_task_flush;
		pTask->get_fw_addr = c_task_get_fw_addr;

		// init other member variables
		pTask->m_pZVCodec = pZVCodec;
        pTask->m_pHal = pHal;
        pTask->m_dbgID = dbgID;

		if (!c_task_init_tasks(pTask))
		{
			c_task_destructor(pTask);
			return (ZOE_NULL);
		}
	}

	return (pTask);
}



// destructor
//
void c_task_destructor(c_task *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_task_destructor()\n" 
				   );

	c_task_done_tasks(This);

	// c_object
	//
	c_object_destructor(&This->m_Object);
}

