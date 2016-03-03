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
// zoe_module_base.c
//
// Description: 
//
//   ZOE base module
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_module_base.h"
#include "zoe_sosal.h"
#include "zoe_ipc_srv.h"
#include "zoe_module_connection_intf_srv.h"
#include "zoe_module_data_intf_srv.h"
#include "zoe_module_control_intf_srv.h"
#include "zoe_module_data_intf_clnt.h"
#include "zoe_module_connection_intf_clnt.h"
#include "zoe_module_objids.h"
#include "zoe_module_mgr.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#include <stdio.h>
#endif //ZOE_LINUXKER_BUILD

/////////////////////////////////////////////////////////////////////////////
//
//


//#define _DEBUG_BUFFER_STATS


/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_port functions
//
static zoe_errs_t zoe_port_play(c_zoe_port *This)
{
    if (This->m_port_set)
    {
        This->m_state = ZOE_STATE_PLAYING;
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_INVALID);
    }
}



static zoe_errs_t zoe_port_flush(c_zoe_port *This)
{
    ZOE_BUFFER_DESCRIPTOR   buf_desc;
    c_fifo                  *p_fifo;
    zoe_errs_t              err, last_err;
    uint32_t                i, nb_buf;
    zoe_dev_mem_t           dev_mem_yuv[3];
    uint32_t                size_yuv[3];

    if (ZOE_STATE_PLAYING != This->m_state)
    {
        last_err = ZOE_ERRS_SUCCESS;

	    if (This->m_p_input_bufdesc_fifo)
	    {
            p_fifo = This->m_p_input_bufdesc_fifo;

            while (c_fifo_get_fifo(p_fifo,
                                   &buf_desc
                                   ))
            {
                if (buf_desc.info.flags & ZOE_BUF_DESC_FLAGS_SEP_CHROMA)
                {
                    size_yuv[0] = buf_desc.buffers[ZOE_BUF_LUMA].valid_size;
                    size_yuv[1] = buf_desc.buffers[ZOE_BUF_CHROMA].valid_size;
                    dev_mem_yuv[0] = buf_desc.buffers[ZOE_BUF_LUMA].buf_ptr;
                    dev_mem_yuv[1] = buf_desc.buffers[ZOE_BUF_CHROMA].buf_ptr;

                    err = zoe_module_release_yuv_buffer_clnt(buf_desc.owner.selector,
                                                             2,
                                                             dev_mem_yuv,
                                                             size_yuv,
                                                             (ZOE_IPC_CPU)buf_desc.owner.addr.cpu,
                                                             buf_desc.owner.addr.module,
                                                             buf_desc.owner.addr.inst
                                                             );
                    if (ZOE_FAIL(err))
                    {
                        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_p_base_module->m_dbg_id,
                                       "zoe_port_flush(%s) - zoe_module_release_yuv_buffer_clnt(%s) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_p_base_module->m_object))),
                                       err
	                                   );
                    }
                }
                else
                {
                    nb_buf = (buf_desc.info.flags & ZOE_BUF_DESC_FLAGS_META) ? 2 : 1;

                    for (i = 0; i < nb_buf; i++)
                    {
#ifdef _DEBUG_BUFFER_STATS
                        // buffer stats
                        MODULE_PRINTF("\r\n%s: port(%d) : (%d:%s:%d:%d) %d:%d\r\n",
                                      zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_p_base_module->m_object))),
                                      This->m_id,
                                      (ZOE_IPC_CPU)buf_desc.owner.addr.cpu,
                                      zoe_module_get_name(buf_desc.owner.addr.module),
                                      buf_desc.owner.addr.inst,
                                      buf_desc.owner.selector,
                                      i,
                                      buf_desc.buffers[i].valid_size
                                      );
#endif //_DEBUG_BUFFER_STATS

                        // call release buffer functions
                        err = zoe_module_release_buffer_clnt(buf_desc.owner.selector,
                                                             i,
                                                             buf_desc.buffers[i].buf_ptr,
                                                             buf_desc.buffers[i].valid_size,
                                                             (ZOE_IPC_CPU)buf_desc.owner.addr.cpu,
                                                             buf_desc.owner.addr.module,
                                                             buf_desc.owner.addr.inst
                                                             );
                        if (ZOE_FAIL(err))
                        {
	                        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                           This->m_p_base_module->m_dbg_id,
		                                   "zoe_port_flush(%s) zoe_module_release_buffer_clnt() FAILED(%d)\n",
                                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_p_base_module->m_object))),
                                           err
		                                   );
                            last_err = err;
                        }
                    }
                }
            }
	    }

        // flush data fifos
        //
        for (i = 0; i < 2; i++)
        {
            if (This->m_p_data_fifo[i])
            {
#ifdef _DEBUG_BUFFER_STATS
                // buffer stats
                MODULE_PRINTF("\r\n%s: port(%d) fifo(%d) : dfifo(%d)\r\n", 
                              zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_p_base_module->m_object))),
                              This->m_id,
                              i,
                              c_data_fifo_get_fifo_level(This->m_p_data_fifo[i])
                              );
#endif //_DEBUG_BUFFER_STATS
                c_data_fifo_flush(This->m_p_data_fifo[i]);
            }
        }

        return (last_err);
    }
    else
    {
        return (ZOE_ERRS_INVALID);
    }
}



static zoe_errs_t zoe_port_stop(c_zoe_port *This)
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;

    if ((This->m_port_set) &&
        (ZOE_STATE_STOPPED != This->m_state)
        )
    {
        // change state now
        This->m_state = ZOE_STATE_STOPPED;

        if (ZOE_MODULE_DATA_OUT == This->m_port_dir)
        {
            // set output port event to unblock the write
            zoe_sosal_event_set(This->m_evt_port);
        }
        else
        {
            if (This->m_p_base_module->m_create_module_thread)
            {
                THREAD_CMD  cmd;

                // send an async command to the thread to flush the input port
                memset(&cmd, 0, sizeof(cmd));

	            cmd.dwCmdCode = ZOE_BASE_MODULE_THREAD_CMD_STOP_PORT;
                cmd.dwParam[0] = This->m_id; 
                cmd.evtCmdAck = This->m_p_base_module->m_evt_ack_state;
                cmd.fBlocking = ZOE_TRUE;

                err = c_thread_set_command(&This->m_p_base_module->m_thread, 
                                           &cmd
                                           );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_p_base_module->m_dbg_id,
	                               "zoe_port_stop(%s) (%d) c_thread_set_command() failed(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_p_base_module->m_object))),
                                   This->m_id,
                                   err
	                               );
                }
            }
            else
            {
                err = zoe_port_flush(This);
            }
        }
    }
    return (err);
}



static zoe_errs_t zoe_port_pause(c_zoe_port *This)
{
    if ((ZOE_STATE_STOPPED == This->m_state) ||
        (!This->m_port_set)
        )
    {
        return (ZOE_ERRS_INVALID);
    }
    else
    {
        This->m_state = ZOE_STATE_PAUSED;
        return (ZOE_ERRS_SUCCESS);
    }
}



static zoe_state_t zoe_port_get_state(c_zoe_port *This)
{
    return (This->m_state);
}



static zoe_errs_t zoe_port_write(c_zoe_port *This,
                                 ZOE_BUFFER_DESCRIPTOR *p_buf_desc
                                 )
{
    if (ZOE_STATE_STOPPED == This->m_state)
    {
        return (ZOE_ERRS_INVALID);
    }

    if (!This->m_p_input_bufdesc_fifo)
    {
        return (ZOE_ERRS_INTERNAL);
    }
    if (c_fifo_set_fifo(This->m_p_input_bufdesc_fifo, 
                        p_buf_desc
                        ))
    {
        zoe_sosal_event_set(This->m_evt_port);
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_NOMEMORY);
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_base private functions
//
static void zoe_module_base_get_memory_type(c_zoe_module_base *This)
{
    switch (This->m_cpu)
    {
        case ZOE_IPC_HPU_KERNEL:
            This->m_mem_type = ZOE_MODULE_MEM_TYPE_SOC;
            This->m_mem_mapping_flag = ZOE_BUF_DESC_FLAGS_MAP_KERNEL; 
            break;
        case ZOE_IPC_HPU_USER:
            This->m_mem_type = ZOE_MODULE_MEM_TYPE_SOC;
            This->m_mem_mapping_flag = ZOE_BUF_DESC_FLAGS_MAP_USER; 
            break;
#if (ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8)
        case ZOE_IPC_EXT_KERNEL:
            This->m_mem_type = ZOE_MODULE_MEM_TYPE_EXTERNAL;
            This->m_mem_mapping_flag = ZOE_BUF_DESC_FLAGS_MAP_KERNEL;
            break;
        case ZOE_IPC_EXT_USER:
            This->m_mem_type = ZOE_MODULE_MEM_TYPE_EXTERNAL;
            This->m_mem_mapping_flag = ZOE_BUF_DESC_FLAGS_MAP_USER; 
            break;
#endif // ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8
        default:
            This->m_mem_type = ZOE_MODULE_MEM_TYPE_INTERNAL;
            This->m_mem_mapping_flag = ZOE_BUF_DESC_FLAGS_MAP_RTOS; 
            break;
    }
}



static zoe_errs_t zoe_module_base_write_to_port_copy(c_zoe_module_base *This,
                                                     uint32_t port_index,
                                                     ZOE_BUFFER_DESCRIPTOR *p_buf_desc,
                                                     uint32_t req_flag,
                                                     uint32_t written[2],
                                                     zoe_void_ptr_t p_private[2]
                                                     )
{
    zoe_errs_t              err = ZOE_ERRS_INVALID, err1;
    uint32_t                alloc, remain, copied = 0;
    zoe_dev_mem_t           dev_mem;
    zoe_dev_mem_t           meta_mem = ZOE_NULL;
    uint32_t                meta_size = 0;
    zoe_dev_mem_t           dev_mem_yuv[3];
    uint32_t                size_yuv[3];
    ZOE_BUFFER_DESCRIPTOR   buf_desc;
    c_zoe_port              *port = &This->m_ports[port_index];
    zoe_bool_t              done = ZOE_FALSE;
    uint32_t                i;

    if ((p_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_META) &&
        (p_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_SEP_CHROMA)
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbg_id,
		               "write_to_port_copy(%s) invalid flags(0x%x)!!!\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                       p_buf_desc->info.flags
		               );
        return (ZOE_ERRS_INVALID);
    }

    // set buffer ownership
    buf_desc.owner.selector = port->m_port_conn.selector;
    buf_desc.owner.addr.cpu = port->m_port_conn.addr.cpu;
    buf_desc.owner.addr.module = port->m_port_conn.addr.module;
    buf_desc.owner.addr.inst = port->m_port_conn.addr.inst;

    // copy other buffer descriptor attributes
    buf_desc.info.pts = p_buf_desc->info.pts;
    buf_desc.info.dts = p_buf_desc->info.dts;
    buf_desc.info.flags = p_buf_desc->info.flags;
    
    if (p_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_SEP_CHROMA)
    {
        // yuv multi-plane buffer
        //
        size_yuv[0] = p_buf_desc->buffers[ZOE_BUF_LUMA].valid_size;
        size_yuv[1] = p_buf_desc->buffers[ZOE_BUF_CHROMA].valid_size;

        while (ZOE_STATE_STOPPED != port->m_state)
        {
            // allocate buffer from the down stream port
            err = zoe_module_allocate_yuv_buffer_clnt(port->m_port_conn.selector,
                                                      2,
                                                      size_yuv,
                                                      dev_mem_yuv,
                                                      port->m_port_conn.addr.cpu,
                                                      port->m_port_conn.addr.module,
                                                      port->m_port_conn.addr.inst
                                                      );
            if (ZOE_SUCCESS(err))
            {
                // copy data 
                for (i = 0; i < 2; i++)
                {
                    buf_desc.buffers[i].buf_ptr = dev_mem_yuv[i];
                    buf_desc.buffers[i].size = buf_desc.buffers[i].valid_size = size_yuv[i];

                    err = ZOEHAL_DMA_WRITE(This->m_p_hal, 
                                           dev_mem_yuv[i], 
                                           (uint8_t *)((zoe_uintptr_t)p_buf_desc->buffers[i].buf_ptr + p_buf_desc->buffers[i].offset), 
                                           size_yuv[i], 
                                           p_buf_desc->info.flags & DMA_BUFFER_MODE_MASK, 
                                           ZOE_FALSE, 
                                           port->m_evt_dma,
                                           p_private[i]
                                           );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "write_to_port_copy ZOEHAL_DMA_WRITE(%s) failed(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
		                               );
                        break;
                    }
                    else
                    {
                        if (port->m_evt_dma)
                        {
                            err = zoe_sosal_event_wait(port->m_evt_dma, 
                                                       2500000  // 2.5 second
                                                       );
	                        if (ZOE_FAIL(err))
                            {
	                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                               This->m_dbg_id,
		                                       "zoe_sosal_event_wait(%s on m_evt_dma) failed(%d)!\n",
                                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                               err
		                                       );
                                break;
                            }
                        }
                    }
                }

                // write to downstream port
                if (ZOE_SUCCESS(err))
                {
                    // call data input write function
                    err = zoe_module_write_clnt(port->m_port_conn.selector,
                                                &buf_desc,
                                                port->m_port_conn.addr.cpu,
                                                port->m_port_conn.addr.module,
                                                port->m_port_conn.addr.inst
                                                );
                }

                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
		                           "write_to_port_copy zoe_module_write_clnt(%s) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                   err
		                           );
                    // release destination buffer
                    err1 = zoe_module_release_yuv_buffer_clnt(port->m_port_conn.selector,
                                                              2,
                                                              dev_mem_yuv,
                                                              size_yuv,
                                                              port->m_port_conn.addr.cpu,
                                                              port->m_port_conn.addr.module,
                                                              port->m_port_conn.addr.inst
                                                              );
                    if (ZOE_FAIL(err1))
                    {
                        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
                                       "write_to_port_copy - zoe_module_release_yuv_buffer_clnt(%s) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err1
	                                   );
                    }
                }
                else
                {
                    written[0] = size_yuv[0];
                    written[1] = size_yuv[1];

                    // release source buffer
                    dev_mem_yuv[0] = p_buf_desc->buffers[ZOE_BUF_LUMA].buf_ptr + p_buf_desc->buffers[ZOE_BUF_LUMA].offset;
                    dev_mem_yuv[1] = p_buf_desc->buffers[ZOE_BUF_CHROMA].buf_ptr + p_buf_desc->buffers[ZOE_BUF_CHROMA].offset;

                    err = zoe_module_release_yuv_buffer_clnt(p_buf_desc->owner.selector,
                                                             2,
                                                             dev_mem_yuv,
                                                             size_yuv,
                                                             p_buf_desc->owner.addr.cpu,
                                                             p_buf_desc->owner.addr.module,
                                                             p_buf_desc->owner.addr.inst
                                                             );
                    if (ZOE_FAIL(err))
                    {
                        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
                                       "write_to_port_copy - zoe_module_release_buffer_clnt(%s) source err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
	                                   );
                    }
                }
            }
            else
            {
                // wait if this is a blocking request
                if ((ZOE_ERRS_NOMEMORY == err) && 
                    (req_flag & ZOE_MODULE_DATA_REQ_BLOCKING)
                    )
                {
                    // wait on port event(buffer available)
                    err = zoe_sosal_event_wait(port->m_evt_port, 
                                               -1
                                               );
	                if (ZOE_SUCCESS(err) &&
                        (ZOE_STATE_STOPPED == port->m_state)
                        )
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "write_to_port_copy STOP!\n"
		                               );
                        // return error if we are stopping
                        err = ZOE_ERRS_CANCELLED;
                    }
                }

                // bail out if failed
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
		                           "write_to_port_copy zoe_module_allocate_buffer_clnt(%s) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                   err
		                           );
                    return (err);
                }
            }
        }
    }
    else
    {
        // compressed data
        //

        // must write out all meta data in one shot
        //
        if (p_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_META)
        {
            buf_desc.buffers[ZOE_BUF_META].offset = 0;

            if (ZOE_STATE_STOPPED != port->m_state)
            {
                // allocate buffer from the down stream port, 
                // must succeed with the exact size allocated for meta data
                err = zoe_module_allocate_buffer_clnt(port->m_port_conn.selector,
                                                      ZOE_BUF_META,
                                                      p_buf_desc->buffers[ZOE_BUF_META].valid_size,
                                                      &meta_mem,
                                                      &meta_size,
                                                      port->m_port_conn.addr.cpu,
                                                      port->m_port_conn.addr.module,
                                                      port->m_port_conn.addr.inst
                                                      );
                if (ZOE_SUCCESS(err) &&
                    (meta_size == p_buf_desc->buffers[ZOE_BUF_META].valid_size)
                    )
                {
                    err = ZOEHAL_MEM_WRITE_EX(This->m_p_hal, 
                                              meta_mem, 
                                              (uint8_t *)((zoe_uintptr_t)p_buf_desc->buffers[ZOE_BUF_META].buf_ptr_va + p_buf_desc->buffers[ZOE_BUF_META].offset), 
                                              meta_size, 
                                              ZOE_TRUE
                                              );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "write_to_port_copy ZOEHAL_MEM_WRITE_EX(%s) meta failed(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
		                               );
                    }
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
		                           "write_to_port_copy zoe_module_allocate_buffer_clnt(%s) meta err(%d) alloc(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                   err,
                                   meta_size
		                           );
                }

                if (ZOE_FAIL(err) || 
                    (meta_size != p_buf_desc->buffers[ZOE_BUF_META].valid_size)
                    )
                {
                    if (meta_size)
                    {
                        zoe_errs_t err1;

                        // release this buffer now
                        //
                        err1 = zoe_module_release_buffer_clnt(port->m_port_conn.selector,
                                                              ZOE_BUF_META,
                                                              meta_mem,
                                                              meta_size,
                                                              port->m_port_conn.addr.cpu,
                                                              port->m_port_conn.addr.module,
                                                              port->m_port_conn.addr.inst
                                                              );
                        if (ZOE_FAIL(err1))
                        {
                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                           This->m_dbg_id,
                                           "write_to_port_copy - zoe_module_release_buffer_clnt(%s) meta err(%d)\n",
                                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                           err1
	                                       );
                        }
                    }
                    return (err);
                }
                else
                {
                    buf_desc.buffers[ZOE_BUF_META].buf_ptr = meta_mem;
                    buf_desc.buffers[ZOE_BUF_META].size = buf_desc.buffers[ZOE_BUF_META].valid_size = meta_size;
                }
            }

            written[1] = meta_size;
        }

        // primary data
        //
        remain = p_buf_desc->buffers[ZOE_BUF_DATA].valid_size;

        // buffer offset is always zero for copy
        buf_desc.buffers[ZOE_BUF_DATA].offset = 0;


        while ((ZOE_STATE_STOPPED != port->m_state) &&
               !done
               )
        {
            alloc = 0;
            // allocate buffer from the down stream port
            err = zoe_module_allocate_buffer_clnt(port->m_port_conn.selector,
                                                  ZOE_BUF_DATA,
                                                  remain,
                                                  &dev_mem,
                                                  &alloc,
                                                  port->m_port_conn.addr.cpu,
                                                  port->m_port_conn.addr.module,
                                                  port->m_port_conn.addr.inst
                                                  );
            // wait if this is a blocking request
            if ((ZOE_ERRS_NOMEMORY == err) && 
                (req_flag & ZOE_MODULE_DATA_REQ_BLOCKING)
                )
            {
                // wait on port event(buffer available)
                err = zoe_sosal_event_wait(port->m_evt_port, 
                                           -1
                                           );
	            if (ZOE_SUCCESS(err) &&
                    (ZOE_STATE_STOPPED == port->m_state)
                    )
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
		                           "write_to_port_copy STOP!\n"
		                           );
                    // return error if we are stopping
                    err = ZOE_ERRS_CANCELLED;
                }
            }

            // bail out if failed
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
		                       "write_to_port_copy zoe_module_allocate_buffer_clnt(%s) err(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               err
		                       );
                done = ZOE_TRUE;
            }
            else
            {
                // buffer allocated
                if (alloc)
                {
#ifndef USE_MEMWR_4_DMA
                    // dma data
                    err = ZOEHAL_DMA_WRITE(This->m_p_hal, 
                                           dev_mem, 
                                           (uint8_t *)((zoe_uintptr_t)p_buf_desc->buffers[ZOE_BUF_DATA].buf_ptr + p_buf_desc->buffers[ZOE_BUF_DATA].offset + copied), 
                                           alloc, 
                                           p_buf_desc->info.flags & DMA_BUFFER_MODE_MASK, 
                                           ZOE_FALSE, 
                                           port->m_evt_dma,
                                           p_private[0]
                                           );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "write_to_port_copy ZOEHAL_DMA_WRITE(%s) failed(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
		                               );
                    }
                    else
                    {
                        if (port->m_evt_dma)
                        {
                            err = zoe_sosal_event_wait(port->m_evt_dma, 
                                                       2500000  // 2.5 second
                                                       );
	                        if (ZOE_FAIL(err))
                            {
	                            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                               This->m_dbg_id,
		                                       "zoe_sosal_event_wait(%s on m_evt_dma) failed(%d)!\n",
                                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                               err
		                                       );
                            }
                        }
                    }
#else //USE_MEMWR_4_DMA
                    // pio data
                    //
                    err = ZOEHAL_MEM_WRITE_EX(This->m_p_hal, 
                                              (uint32_t)(dev_mem & 0xFFFFFFFF), 
                                              (uint8_t *)(p_buf_desc->buffers[ZOE_BUF_DATA].buf_ptr_va + p_buf_desc->buffers[ZOE_BUF_DATA].offset + copied), 
                                              alloc, 
                                              ZOE_TRUE
                                              );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "write_to_port_copy ZOEHAL_MEM_WRITE_EX(%s) failed(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
		                               );
                    }
#endif // !USE_MEMWR_4_DMA
                    if (ZOE_SUCCESS(err))
                    {
                        // fill in the rest of the buffer descriptor fields            
                        buf_desc.buffers[ZOE_BUF_DATA].buf_ptr = dev_mem;
                        buf_desc.buffers[ZOE_BUF_DATA].size = buf_desc.buffers[ZOE_BUF_DATA].valid_size = alloc;

                        // call data input write function
                        err = zoe_module_write_clnt(port->m_port_conn.selector,
                                                    &buf_desc,
                                                    port->m_port_conn.addr.cpu,
                                                    port->m_port_conn.addr.module,
                                                    port->m_port_conn.addr.inst
                                                    );
                    }
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "write_to_port_copy zoe_module_write_clnt(%s) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
		                               );
                        done = ZOE_TRUE;
                    }
                    else
                    {
                        // clear the meta data flag
                        if (buf_desc.info.flags & ZOE_BUF_DESC_FLAGS_META)
                        {
                            buf_desc.info.flags &= ~ZOE_BUF_DESC_FLAGS_META;
                        }
                        // update copied
                        remain -= alloc;
                        copied += alloc;
                    }
                }
            }

            if (ZOE_FAIL(err))
            {
                if (alloc)
                {
                    zoe_errs_t err1;

                    // release this buffer now
                    //
                    err1 = zoe_module_release_buffer_clnt(port->m_port_conn.selector,
                                                          ZOE_BUF_DATA,
                                                          dev_mem,
                                                          alloc,
                                                          port->m_port_conn.addr.cpu,
                                                          port->m_port_conn.addr.module,
                                                          port->m_port_conn.addr.inst
                                                          );
                    if (ZOE_FAIL(err1))
                    {
                        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
                                       "write_to_port_copy - zoe_module_release_buffer_clnt(%s) err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err1
	                                   );
                    }
                }

                if (meta_size && 
                    (buf_desc.info.flags & ZOE_BUF_DESC_FLAGS_META)
                    )
                {
                    zoe_errs_t err1;

                    // release this buffer now
                    //
                    err1 = zoe_module_release_buffer_clnt(port->m_port_conn.selector,
                                                          ZOE_BUF_META,
                                                          meta_mem,
                                                          meta_size,
                                                          port->m_port_conn.addr.cpu,
                                                          port->m_port_conn.addr.module,
                                                          port->m_port_conn.addr.inst
                                                          );
                    if (ZOE_FAIL(err1))
                    {
                        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
                                       "write_to_port_copy - zoe_module_release_buffer_clnt(%s) meta err(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err1
	                                   );
                    }
                }
            }

            // get out of the loop if we are either non-blocking or all
            // the data has been written
            if ((0 == (req_flag & ZOE_MODULE_DATA_REQ_BLOCKING)) ||
                (0 == remain)
                )
            {
                done = ZOE_TRUE;
            }
        }

        // release source buffer
        //
        if (copied)
        {
            err = zoe_module_release_buffer_clnt(p_buf_desc->owner.selector,
                                                 ZOE_BUF_DATA,
                                                 p_buf_desc->buffers[ZOE_BUF_DATA].buf_ptr + p_buf_desc->buffers[ZOE_BUF_DATA].offset,
                                                 copied,
                                                 p_buf_desc->owner.addr.cpu,
                                                 p_buf_desc->owner.addr.module,
                                                 p_buf_desc->owner.addr.inst
                                                 );
            if (ZOE_FAIL(err))
            {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
                               "write_to_port_copy - zoe_module_release_buffer_clnt(%s) source err(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               err
	                           );
            }

            if (p_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_META)
            {
                err = zoe_module_release_buffer_clnt(p_buf_desc->owner.selector,
                                                     ZOE_BUF_META,
                                                     p_buf_desc->buffers[ZOE_BUF_META].buf_ptr + p_buf_desc->buffers[ZOE_BUF_META].offset,
                                                     meta_size,
                                                     p_buf_desc->owner.addr.cpu,
                                                     p_buf_desc->owner.addr.module,
                                                     p_buf_desc->owner.addr.inst
                                                     );
                if (ZOE_FAIL(err))
                {
                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
                                   "write_to_port_copy - zoe_module_release_buffer_clnt(%s) meta source err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                   err
	                               );
                }
            }
        }
        written[0] = copied;
    }
    return (err);
}



static zoe_errs_t zoe_module_base_write_to_port_in_place(c_zoe_module_base *This,
                                                         uint32_t port_index,
                                                         ZOE_BUFFER_DESCRIPTOR *p_buf_desc,
                                                         uint32_t req_flag,
                                                         uint32_t written[2],
                                                         zoe_void_ptr_t p_private[2]
                                                         )
{
    zoe_errs_t  err;
    c_zoe_port  *port = &This->m_ports[port_index];

    if (ZOE_STATE_STOPPED == port->m_state)
    {
        err = ZOE_ERRS_INVALID;
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbg_id,
                       "%s STOPED \n",
                       __FUNCTION__
	                   );
    }
    else
    {
        err = zoe_module_write_clnt(port->m_port_conn.selector,
                                    p_buf_desc,
                                    port->m_port_conn.addr.cpu,
                                    port->m_port_conn.addr.module,
                                    port->m_port_conn.addr.inst
                                    );
        if (ZOE_FAIL(err))
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
                           "%s zoe_module_write_clnt() failed err(%d)\n",
                           __FUNCTION__,
                           err
	                       );
        }
    }

    written[0] = ZOE_SUCCESS(err) ? p_buf_desc->buffers[ZOE_BUF_DATA].valid_size : 0;
    if (buf_desc_2_bufs(p_buf_desc))
    {
        written[1] = ZOE_SUCCESS(err) ? p_buf_desc->buffers[ZOE_BUF_META].valid_size : 0;
    }
    return (err);
}



static zoe_errs_t zoe_module_base_write_to_port_null(c_zoe_module_base *This,
                                                     uint32_t port_index,
                                                     ZOE_BUFFER_DESCRIPTOR *p_buf_desc,
                                                     uint32_t req_flag,
                                                     uint32_t written[2],
                                                     zoe_void_ptr_t p_private[2]
                                                     )
{
    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbg_id,
                   "%s ?? \n",
                   __FUNCTION__
	               );
    return (ZOE_ERRS_INVALID);    
}



static zoe_errs_t zoe_module_base_write_to_port_set_method(c_zoe_module_base *This,
                                                           uint32_t port_index
                                                           )
{
    zoe_errs_t      err = ZOE_ERRS_SUCCESS;
    c_zoe_port      *port = &This->m_ports[port_index];

    if ((This->m_mem_type == port->m_down_stream_mem_type) ||
        ((This->m_mem_type == ZOE_MODULE_MEM_TYPE_SOC) &&
         (port->m_down_stream_mem_type == ZOE_MODULE_MEM_TYPE_INTERNAL))
        )
    {
        switch (port->m_down_stream_mem_usage)
        {
            case ZOE_MODULE_MEM_USAGE_COPY:
                port->m_p_write_2_port_func = zoe_module_base_write_to_port_copy;
                break;
            case ZOE_MODULE_MEM_USAGE_IN_PLACE:
                port->m_p_write_2_port_func = zoe_module_base_write_to_port_in_place;
                break;
            default:
                err = ZOE_ERRS_INVALID;
                break;
        }
    }
    else
    {
        switch (This->m_mem_type)
        {
            case ZOE_MODULE_MEM_TYPE_INTERNAL:
                switch (port->m_down_stream_mem_type)
                {
                    case ZOE_MODULE_MEM_TYPE_EXTERNAL:
                    case ZOE_MODULE_MEM_TYPE_SOC:
                        // in place
                        if (ZOE_MODULE_MEM_USAGE_IN_PLACE == port->m_down_stream_mem_usage)
                        {
                            port->m_p_write_2_port_func = zoe_module_base_write_to_port_in_place;
                        }
                        else
                        {
                            err = ZOE_ERRS_INVALID;
                        }
                        break;
                    default:
                        err = ZOE_ERRS_INVALID;
                        break;
                }
                break;
            case ZOE_MODULE_MEM_TYPE_EXTERNAL:
                switch (port->m_down_stream_mem_type)
                {
                    case ZOE_MODULE_MEM_TYPE_INTERNAL:
                        // copy
                        if (ZOE_MODULE_MEM_USAGE_COPY == port->m_down_stream_mem_usage)
                        {
                            port->m_p_write_2_port_func = zoe_module_base_write_to_port_copy;
                        }
                        else
                        {
                            err = ZOE_ERRS_INVALID;
                        }
                        break;
                    case ZOE_MODULE_MEM_TYPE_SOC:
                    default:
                        err = ZOE_ERRS_INVALID;
                        break;
                }
                break;
            case ZOE_MODULE_MEM_TYPE_SOC:
            default:
                err = ZOE_ERRS_INVALID;
                break;
        }
    }
    return (err);
}



// data process routine, leave buffer descriptor in the fifo
static zoe_errs_t zoe_module_base_processes(c_zoe_module_base *This, 
                                            uint32_t port_index
                                            )
{
    c_fifo                  *p_fifo;
    ZOE_BUFFER_DESCRIPTOR   buf_desc;
    zoe_errs_t              err = ZOE_ERRS_PARMS;

    p_fifo = This->m_ports[port_index].m_p_input_bufdesc_fifo;
    if (p_fifo)
    {
        while (c_fifo_get_fifo(p_fifo,
                               &buf_desc
                               ))
        {
            // process the data
            err = This->process(This,
                                port_index, 
                                &buf_desc
                                );                   
            if (ZOE_FAIL(err))
            {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
                               "zoe_module_base_processes(%s) - port(%d) err(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               port_index,
                               err
		                       );
                break;
            }
        }
    }
    return (err);
}

// data process routine, take one buffer descriptor out and pass to this function
//
static zoe_errs_t zoe_module_base_process(c_zoe_module_base *This, 
                                          uint32_t port_index, 
                                          ZOE_BUFFER_DESCRIPTOR *p_buf_desc
                                          )
{
    zoe_errs_t  err;
    uint32_t    i, nb_buf;

    nb_buf = (p_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_META) ? 2 : 1;

    for (i = 0; i < nb_buf; i++)
    {
        // release the buffer
        err = zoe_module_release_buffer_clnt(p_buf_desc->owner.selector,
                                             i,
                                             p_buf_desc->buffers[i].buf_ptr,
                                             p_buf_desc->buffers[i].valid_size,
                                             (ZOE_IPC_CPU)p_buf_desc->owner.addr.cpu,
                                             p_buf_desc->owner.addr.module,
                                             p_buf_desc->owner.addr.inst
                                             );
        if (ZOE_FAIL(err))
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
                           "zoe_module_base_process(%s) - port(%d) zoe_module_release_buffer_clnt err(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           port_index,
                           err
	                       );
        }
    }
    return (err);
}



// extra event notification
//
static zoe_errs_t zoe_module_base_notify(c_zoe_module_base *This, 
                                         uint32_t evt
                                         )
{
    return (ZOE_ERRS_SUCCESS);
}



// state transition handler, executed from the calling thread
//
static zoe_errs_t zoe_module_base_do_play(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_base_do_stop(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_base_do_pause(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



// state transition handler, synchronized with the main thread
//
static zoe_errs_t zoe_module_base_do_play_cmd(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_base_do_stop_cmd(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_base_do_pause_cmd(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



// unblock function for stop
static zoe_errs_t zoe_module_base_unblock(c_zoe_module_base *This)
{
    return (ZOE_ERRS_SUCCESS);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// module control interface API
//
static zoe_errs_t zoe_module_base_play(c_zoe_module_base *This,
                                       uint32_t sel
                                       )
{
    zoe_errs_t  err;
    THREAD_CMD  cmd;
    uint32_t    i;

    ENTER_CRITICAL(&This->m_object)

    // check if this is for individual port
    //
    if ((sel >= ZOE_MODULE_DATA_SEL_PORT_START) &&
        (sel < (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs))
        )
    {
        c_zoe_port *port = &This->m_ports[sel - ZOE_MODULE_DATA_SEL_PORT_START];
        err = zoe_port_play(port);
        if (ZOE_FAIL(err))
        {
            LEAVE_CRITICAL(&This->m_object)
            return (err);
        }
    }
    else
    {
        // for the entire module
        // set all ports to playing to allow data flow
        for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
        {
            if (This->m_ports[i].m_port_set)
            {
                This->m_ports[i].m_state = ZOE_STATE_PLAYING;
            }
        }
    }

    if (ZOE_STATE_PLAYING == This->m_state)
    {
        LEAVE_CRITICAL(&This->m_object)
        return (ZOE_ERRS_SUCCESS);
    }

    // set the target state to playing
    This->m_state_to = ZOE_STATE_PLAYING;

    // call the play handler from the calling thread
    err = This->do_play(This);
    if (ZOE_FAIL(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbg_id,
	                   "zoe_module_base_play(%d) do_play() failed(%d)\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                       err
	                   );
    }
    else
    {
        if (This->m_create_module_thread)
        {
            // set the command to the thread
            memset(&cmd, 0, sizeof(cmd));

            This->m_err_cmd_play = ZOE_ERRS_SUCCESS;

	        cmd.dwCmdCode = ZOE_BASE_MODULE_THREAD_CMD_SET_STATE;
            cmd.dwParam[0] = ZOE_STATE_PLAYING; 
            cmd.evtCmdAck = This->m_evt_ack_state;
            cmd.pdwError = &This->m_err_cmd_play;
            cmd.fBlocking = ZOE_TRUE;

            err = c_thread_set_command(&This->m_thread, 
                                       &cmd
                                       );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
	                           "zoe_module_base_play(%d) c_thread_set_command() failed(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               err
	                           );
            }
        }
        else
        {
            err = This->do_play_cmd(This);
        }
    }

    if (ZOE_FAIL(err))
    {
        // restore to previous state if play failed
        for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
        {
            if (This->m_ports[i].m_port_set)
            {
                This->m_ports[i].m_state = This->m_state;
            }
        }
        This->m_state_to = This->m_state;
    }
    else
    {
        This->m_state = ZOE_STATE_PLAYING;
    }
    LEAVE_CRITICAL(&This->m_object)
    return (err);
}



static zoe_errs_t zoe_module_base_stop(c_zoe_module_base *This, 
                                       uint32_t sel
                                       )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    THREAD_CMD  cmd;
    c_zoe_port  *port;
    uint32_t    i;
    zoe_bool_t  do_stop = ZOE_TRUE;

    ENTER_CRITICAL(&This->m_object)

    // check if this is for individual port
    //
    if ((sel >= ZOE_MODULE_DATA_SEL_PORT_START) &&
        (sel < (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs))
        )
    {
        c_zoe_port *port = &This->m_ports[sel - ZOE_MODULE_DATA_SEL_PORT_START];
        err = zoe_port_stop(port);
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
	                       "zoe_module_base_stop(%s) zoe_port_stop() failed(%d)!!!\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           err
	                       );
#if 0
            LEAVE_CRITICAL(&This->m_object)
            return (err);
#else
            // we need to move on anyway
            err = ZOE_ERRS_SUCCESS;
#endif
        }
    }
    else
    {
        // for the entire module
        // change all the port state to stopped to stop data flow
        for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
        {
            This->m_ports[i].m_state = ZOE_STATE_STOPPED;
        }
    }

    if (ZOE_STATE_STOPPED == This->m_state)
    {
        LEAVE_CRITICAL(&This->m_object)
        return (ZOE_ERRS_SUCCESS);
    }

    for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
    {
        if (ZOE_STATE_STOPPED != This->m_ports[i].m_state)
        {
            do_stop = ZOE_FALSE;
        }
    }

    if (do_stop)
    {
        // set target state to stop
        This->m_state_to = ZOE_STATE_STOPPED;

        // call the play handler from the calling thread
        err = This->do_stop(This);
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
	                       "zoe_module_base_stop(%d) do_stop() failed(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           err
	                       );
        }

        // set output port events to unblock them
        for (i = 0; i < This->m_num_outputs; i++)
        {
            port = &This->m_ports[This->m_num_inputs + i];

            if (port->m_evt_port)
            {
                zoe_sosal_event_set(port->m_evt_port);
            }
        }

        // call unblock function now so thread can go back to the main handler
        This->unblock(This);

        if (This->m_create_module_thread)
        {
            // set the command to the thread
            memset(&cmd, 0, sizeof(cmd));

            This->m_err_cmd_stop = ZOE_ERRS_SUCCESS;

    	    cmd.dwCmdCode = ZOE_BASE_MODULE_THREAD_CMD_SET_STATE;
            cmd.dwParam[0] = ZOE_STATE_STOPPED; 
            cmd.evtCmdAck = This->m_evt_ack_state;
            cmd.pdwError = &This->m_err_cmd_stop;
            cmd.fBlocking = ZOE_TRUE;

            err = c_thread_set_command(&This->m_thread, 
                                       &cmd
                                       );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
	                           "zoe_module_base_stop(%s) c_thread_set_command() failed(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               err
	                           );
            }
        }
        else
        {
            err = This->do_stop_cmd(This);
        }

        // change the state to stopped regardless
        This->m_state = ZOE_STATE_STOPPED;
    }

    LEAVE_CRITICAL(&This->m_object)

    if (ZOE_STATE_STOPPED == This->m_state)
    {
        This->flush(This, 
                    ZOE_MODULE_DATA_SEL_MODULE
                    );
    }
    return (err);
}



zoe_errs_t zoe_module_base_flush(c_zoe_module_base *This, 
                                 uint32_t sel
                                 )
{
    c_zoe_port  *port;
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    uint32_t    i;

    ENTER_CRITICAL(&This->m_object)

    // check if this is for individual port
    //
    if ((sel >= ZOE_MODULE_DATA_SEL_PORT_START) &&
        (sel < (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs))
        )
    {
        c_zoe_port *port = &This->m_ports[sel - ZOE_MODULE_DATA_SEL_PORT_START];
        err = zoe_port_flush(port);
        LEAVE_CRITICAL(&This->m_object)
        return (err);
    }

    // for the entire module
    //
    if (ZOE_STATE_PLAYING == This->m_state)
    {
        LEAVE_CRITICAL(&This->m_object)
        return (ZOE_ERRS_INVALID);
    }

    This->m_flushing = ZOE_TRUE;
    for (i = 0; i < This->m_num_inputs; i++)
    {
        port = &This->m_ports[i];
        err = zoe_port_flush(port);
    }
    This->m_flushing = ZOE_FALSE;

    LEAVE_CRITICAL(&This->m_object)
    return (err);
}



static zoe_errs_t zoe_module_base_pause(c_zoe_module_base *This, 
                                        uint32_t sel
                                        )
{
    zoe_errs_t  err;
    THREAD_CMD  cmd;
    uint32_t    i;

    ENTER_CRITICAL(&This->m_object)

    // check if this is for individual port
    //
    if ((sel >= ZOE_MODULE_DATA_SEL_PORT_START) &&
        (sel < (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs))
        )
    {
        c_zoe_port *port = &This->m_ports[sel - ZOE_MODULE_DATA_SEL_PORT_START];
        err = zoe_port_pause(port);
        LEAVE_CRITICAL(&This->m_object)
        return (err);
    }

    // for the entire module
    //
    if (ZOE_STATE_PAUSED == This->m_state)
    {
        LEAVE_CRITICAL(&This->m_object)
        return (ZOE_ERRS_SUCCESS);
    }
    else if (ZOE_STATE_STOPPED == This->m_state)
    {
        LEAVE_CRITICAL(&This->m_object)
        return (ZOE_ERRS_INVALID);
    }

    // set target state to pause
    This->m_state_to = ZOE_STATE_PAUSED;

    // change all the port state to paused
    for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
    {
        if (This->m_ports[i].m_port_set)
        {
            This->m_ports[i].m_state = ZOE_STATE_PAUSED;
        }
    }

    // call the pause handler from the calling thread
    err = This->do_pause(This);
    if (ZOE_FAIL(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbg_id,
	                   "zoe_module_base_pause(%d) do_pause() failed(%d)\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                       err
	                   );
    }
    else
    {
        if (This->m_create_module_thread)
        {
            // set the command to the thread
            memset(&cmd, 0, sizeof(cmd));

            This->m_err_cmd_pause = ZOE_ERRS_SUCCESS;

	        cmd.dwCmdCode = ZOE_BASE_MODULE_THREAD_CMD_SET_STATE;
            cmd.dwParam[0] = ZOE_STATE_PAUSED; 
            cmd.evtCmdAck = This->m_evt_ack_state;
            cmd.pdwError = &This->m_err_cmd_pause;
            cmd.fBlocking = ZOE_TRUE;

            err = c_thread_set_command(&This->m_thread, 
                                       &cmd
                                       );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
	                           "zoe_module_base_pause(%s) c_thread_set_command() failed(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               err
	                           );
            }
        }
        else
        {
            err = This->do_pause_cmd(This);
        }
    }

    if (ZOE_FAIL(err))
    {
        // restore to previous state if failed
        for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
        {
            if (This->m_ports[i].m_port_set)
            {
                This->m_ports[i].m_state = This->m_state;
            }
        }
        This->m_state_to = This->m_state;
    }
    else
    {
        This->m_state = ZOE_STATE_PAUSED;
    }
    LEAVE_CRITICAL(&This->m_object)
    return (err);
}



static zoe_state_t zoe_module_base_get_state(c_zoe_module_base *This, 
                                             uint32_t sel
                                             )
{
    if (ZOE_MODULE_DATA_SEL_MODULE == sel)
    {
        return (This->m_state);
    }
    else
    {
        sel -= ZOE_MODULE_DATA_SEL_PORT_START;
        if (sel >= (This->m_num_inputs + This->m_num_outputs))
        {
            return (ZOE_STATE_STOPPED);
        }
        else
        {
            return (zoe_port_get_state(&This->m_ports[sel]));
        }
    }
}



static zoe_state_t zoe_module_base_evt_notify(c_zoe_module_base *This, 
                                              uint32_t sel,
                                              uint32_t evt,
                                              uint32_t evt_data[4]
                                              )
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbg_id,
	               "zoe_module_base_evt_notify(%s) evt(%d)\n",
                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                   evt
	               );
    return (ZOE_ERRS_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// module data interface API
//
static zoe_errs_t zoe_module_base_allocate_buffer(c_zoe_module_base *This, 
                                                  uint32_t sel, 
                                                  uint32_t buf_sel,
                                                  uint32_t size, 
                                                  zoe_dev_mem_t *p_dev_mem, 
                                                  uint32_t *p_size_got
                                                  )
{
    c_zoe_port  *port;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (buf_sel >= 2)
        )
    {
        return (ZOE_ERRS_NOTIMPL);
    }
    if (!p_dev_mem ||
        !p_size_got
        )
    {
        return (ZOE_ERRS_PARMS);
    }    
    if ((ZOE_STATE_STOPPED == This->m_state) ||
        This->m_flushing
        )
    {
        return (ZOE_ERRS_INVALID);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];
    if (!port)
    {
        return (ZOE_ERRS_NOTFOUND);
    }
    if (ZOE_STATE_STOPPED == port->m_state)
    {
        return (ZOE_ERRS_INVALID);
    }
    if (port->m_p_data_fifo[buf_sel])
    {
        *p_dev_mem = (zoe_dev_mem_t)((zoe_uintptr_t)c_data_fifo_allocate_buffer(port->m_p_data_fifo[buf_sel], 
                                                                                size, 
                                                                                p_size_got
                                                                                ));
        return ((0 != *p_size_got) ? ZOE_ERRS_SUCCESS : ZOE_ERRS_NOMEMORY);
    }
    else
    {
        return (ZOE_ERRS_NOTFOUND);
    }
}



zoe_errs_t zoe_module_base_release_buffer(c_zoe_module_base *This, 
                                          uint32_t sel, 
                                          uint32_t buf_sel,
                                          zoe_dev_mem_t dev_mem,
                                          uint32_t size
                                          )
{
    c_zoe_port  *port;
    uint32_t    i;
    c_fifo      *p_fifo;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (buf_sel >= 2)
        ) 
    {
        return (ZOE_ERRS_NOTIMPL);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];

    if (!port->m_p_data_fifo[buf_sel])
    {
        return (ZOE_ERRS_NOTFOUND);
    }

    if (c_data_fifo_release_buffer(port->m_p_data_fifo[buf_sel], 
                                   (uint8_t *)((zoe_uintptr_t)dev_mem),
                                   size
                                   ))
    {
        zoe_errs_t  err;

//        printf("%s_REL_%d\r\n", zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))), c_data_fifo_get_fifo_level(port->m_p_input_data_fifo));

        // notify upstream port buffer is available
        if (ZOE_MODULE_DATA_IN == port->m_port_dir)
        {
            err = zoe_module_buffer_available_clnt(port->m_port_conn.selector,
                                                   buf_sel,                                                
                                                   port->m_port_conn.addr.cpu,
                                                   port->m_port_conn.addr.module,
                                                   port->m_port_conn.addr.inst                                               
                                                   );
            if (ZOE_FAIL(err))
            {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
			                   "zoe_module_base_release_buffer(%s) - zoe_module_buffer_available_clnt buf_sel(%d) failed(%d)!\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               buf_sel,
                               err
					           );
            }
        }
        else
        {
            if (ZOE_BUF_DATA == buf_sel)
            {
                // peek input fifo and set the input event to get the streaming moving
                // if the fifo is not empty
                //
                for (i = 0; i < This->m_num_inputs; i++)
                {
                    p_fifo = This->m_ports[i].m_p_input_bufdesc_fifo;
                    if (p_fifo && 
                        (0 != c_fifo_get_fifo_level(p_fifo))
                        )
                    {
                        zoe_sosal_event_set(This->m_ports[i].m_evt_port);
                    }
                }
            }
        }
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_FAIL);
    }
}




static zoe_errs_t zoe_module_base_release_buffer_with_info(c_zoe_module_base *This, 
                                                           uint32_t sel, 
                                                           uint32_t buf_sel,
                                                           zoe_dev_mem_t dev_mem,
                                                           uint32_t size,
                                                           ZOE_BUFFER_INFO* buf_info
                                                           )
{
    return (This->release_buffer(This, 
                                 sel, 
                                 buf_sel, 
                                 dev_mem, 
                                 size
                                 ));
}




static zoe_errs_t zoe_module_base_allocate_yuv_buffer(c_zoe_module_base *This, 
                                                      uint32_t sel, 
                                                      uint32_t num_planes,
                                                      uint32_t size[3], 
                                                      zoe_dev_mem_t dev_mem[3]
                                                      )
{
    return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t zoe_module_base_release_yuv_buffer(c_zoe_module_base *This, 
                                                     uint32_t sel, 
                                                     uint32_t num_planes,
                                                     zoe_dev_mem_t dev_mem[3],
                                                     uint32_t size[3]
                                                     )
{
    return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t zoe_module_base_release_yuv_buffer_with_info(c_zoe_module_base *This, 
                                                               uint32_t sel, 
                                                               uint32_t num_planes,
                                                               zoe_dev_mem_t dev_mem[3],
                                                               uint32_t size[3],
                                                               ZOE_BUFFER_INFO* buf_info
                                                               )
{
    return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t zoe_module_base_get_mem_type(c_zoe_module_base *This, 
                                               uint32_t sel, 
                                               uint32_t *p_mem_type
                                               )
{
    if (!p_mem_type)
    {
        return (ZOE_ERRS_PARMS);
    }    
    *p_mem_type = This->m_mem_type;
    return (ZOE_ERRS_SUCCESS);    
}



static zoe_errs_t zoe_module_base_get_mem_usage(c_zoe_module_base *This, 
                                                uint32_t sel, 
                                                uint32_t *p_mem_usage
                                                )
{
    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START)
        )
    {
        return (ZOE_ERRS_NOTIMPL);
    }
    if (!p_mem_usage)
    {
        return (ZOE_ERRS_PARMS);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    *p_mem_usage = This->m_ports[sel].m_input_port_mem_usage;
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_base_write(c_zoe_module_base *This, 
                                        uint32_t sel, 
                                        ZOE_BUFFER_DESCRIPTOR *p_buf_desc
                                        )
{
    c_zoe_port  *port;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START)
        )
    {
        return (ZOE_ERRS_NOTIMPL);
    }
    if (!p_buf_desc)
    {
        return (ZOE_ERRS_PARMS);
    }
    if ((ZOE_STATE_STOPPED == This->m_state) ||
        This->m_flushing
        )
    {
        return (ZOE_ERRS_INVALID);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];

    return (zoe_port_write(port, 
                           p_buf_desc
                           ));
}



static zoe_errs_t zoe_module_base_buffer_available(c_zoe_module_base *This, 
                                                   uint32_t sel,
                                                   uint32_t buf_sel
                                                   )
{
    c_fifo      *p_fifo;
    uint32_t    i;

    if (sel < (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs))
    {
        return (ZOE_ERRS_NOTIMPL);
    }
    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        (buf_sel >= 2)
        )
    {
        return (ZOE_ERRS_PARMS);
    }
//    printf("%s_BA\r\n", zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))));

    // unblock the output port
    //
    if (ZOE_BUF_DATA == buf_sel)
    {
        sel -= ZOE_MODULE_DATA_SEL_PORT_START;
        zoe_sosal_event_set(This->m_ports[sel].m_evt_port);

        // peek input fifo and set the input event to get the streaming moving
        // if the fifo is not empty
        //
        for (i = 0; i < This->m_num_inputs; i++)
        {
            p_fifo = This->m_ports[i].m_p_input_bufdesc_fifo;
            if (p_fifo && 
                (0 != c_fifo_get_fifo_level(p_fifo))
                )
            {
                zoe_sosal_event_set(This->m_ports[i].m_evt_port);
            }
        }
    }

    return (ZOE_ERRS_SUCCESS);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// module connection API
//
uint32_t zoe_module_base_get_num_ports(c_zoe_module_base *This) 
{
    return (This->m_num_inputs + This->m_num_outputs);
}



zoe_errs_t zoe_module_base_get_port_selector_from_index(c_zoe_module_base *This, 
                                                        uint32_t index, 
                                                        uint32_t *p_sel
                                                        )
{
    if ((index >= (This->m_num_inputs + This->m_num_outputs)) ||
        !p_sel
        )
    {
        return (ZOE_ERRS_PARMS);
    }
    *p_sel = ZOE_MODULE_DATA_SEL_PORT_START + index;
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t zoe_module_base_get_port_dir(c_zoe_module_base *This, 
                                        uint32_t sel, 
                                        uint32_t *p_dir
                                        ) 
{
    if ((sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        !p_dir
        )
    {
        return (ZOE_ERRS_PARMS);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    *p_dir = This->m_ports[sel].m_port_dir;
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t zoe_module_base_get_port_type(c_zoe_module_base *This, 
                                         uint32_t sel, 
                                         uint32_t *p_type, 
                                         uint32_t *p_sub_type
                                         ) 
{
    c_zoe_port  *port;

    if ((sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        !p_type ||
        !p_sub_type
        )
    {
        return (ZOE_ERRS_PARMS);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];

    *p_type = port->m_port_type;
    *p_sub_type = port->m_port_sub_type;
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t zoe_module_base_is_port_set(c_zoe_module_base *This, 
                                       uint32_t sel, 
                                       zoe_bool_t *p_set
                                       ) 
{
    if ((sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        !p_set
        )
    {
        return (ZOE_ERRS_PARMS);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    *p_set = This->m_ports[sel].m_port_set;
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t zoe_module_base_port_set(c_zoe_module_base *This, 
                                    uint32_t sel, 
                                    ZOE_MODULE_DATA_CONNECTOR *p_port
                                    ) 
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    c_zoe_port  *port;
    uint32_t    type, sub_type;
    uint32_t    i;

    if ((sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        !p_port
        )
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbg_id,
		               "zoe_module_base_port_set(%s) sel(%d) p_port(0x%x) FAILED\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                       sel,
                       p_port
		               );
        return (ZOE_ERRS_PARMS);
    }

    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];

    // cannot connect if port is not stopped
    //
    if (ZOE_STATE_STOPPED != port->m_state)
    {
        return (ZOE_ERRS_INVALID);
    }

    // cannot double park
    //
    if (port->m_port_set)
    {
        return (ZOE_ERRS_INVALID);
    }

    // check data compatibility
    //
    if ((port->m_port_type != ZOE_MODULE_DATA_TYPE_NONE) ||
        (port->m_port_sub_type != ZOE_MODULE_DATA_SUB_TYPE_NONE)
        )
    {
        err = zoe_module_base_get_port_type_clnt(p_port->selector, 
                                                 &type, 
                                                 &sub_type,
                                                 p_port->addr.cpu,
                                                 p_port->addr.module,
                                                 p_port->addr.inst
                                                 );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "zoe_module_base_port_set(%s) zoe_module_base_get_port_type_clnt() FAILED(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           err
		                   );
            return (err);
        }

        // check type
        if ((port->m_port_type != ZOE_MODULE_DATA_TYPE_NONE) &&
            (type != ZOE_MODULE_DATA_TYPE_NONE) &&
            (port->m_port_type != type)
            )
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "zoe_module_base_port_set(%s) type(%d) mismatch(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           port->m_port_type,
                           type                           
		                   );
            return (ZOE_ERRS_INVALID);
        }

        // check sub type
        if ((port->m_port_sub_type != ZOE_MODULE_DATA_SUB_TYPE_NONE) &&
            (sub_type != ZOE_MODULE_DATA_SUB_TYPE_NONE) &&
            (port->m_port_sub_type != sub_type)
            )
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "zoe_module_base_port_set(%s) sub_type(%d) mismatch(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           port->m_port_sub_type,
                           sub_type
		                   );
            return (ZOE_ERRS_INVALID);
        }
    }

    if (sel >= This->m_num_inputs)
    {
        // get down stream memory usage and memory type for output ports
        //
        err = zoe_module_get_mem_type_clnt(p_port->selector,
                                           &port->m_down_stream_mem_type,
                                           p_port->addr.cpu,
                                           p_port->addr.module,
                                           p_port->addr.inst
                                           );
        err = ZOE_FAIL(err) ? err : zoe_module_get_mem_usage_clnt(p_port->selector,
                                                                  &port->m_down_stream_mem_usage,
                                                                  p_port->addr.cpu,
                                                                  p_port->addr.module,
                                                                  p_port->addr.inst
                                                                  );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "zoe_module_base_port_set(%s) zoe_module_get_mem_xxx_clnt() FAILED(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                           err
		                   );
        }
        else
        {
            // set write to down stream port method
            err = zoe_module_base_write_to_port_set_method(This, 
                                                           sel
                                                           );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
		                       "zoe_module_base_port_set(%s) zoe_module_base_write_to_port_set_method() FAILED(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                               err
		                       );
            }
            else
            {
                if (ZOE_MODULE_MEM_USAGE_COPY == port->m_down_stream_mem_usage)
                {
                    // create dma completion event if downstream memory usage is copy
                    err = zoe_sosal_event_create(ZOE_NULL, 
                                                 &port->m_evt_dma
                                                 );
                    if (ZOE_FAIL(err))
                    {
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "zoe_module_base_port_set(%s) zoe_sosal_event_create() FAILED(%d)\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                       err
		                               );
                    }
                }
            }
        }
    }
    else
    {
        if (ZOE_MODULE_MEM_USAGE_IN_PLACE == port->m_input_port_mem_usage)
        {            
            if (port->m_need_dma)
            {
                // create dma completion event if input requires it
                err = zoe_sosal_event_create(ZOE_NULL, 
                                             &port->m_evt_dma
                                             );
                if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
		                           "zoe_module_base_port_set(%s) zoe_sosal_event_create() FAILED(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                   err
		                           );
                }
            }
        }
    }

    // create data fifos if requested
    //
    for (i = 0; i < 2; i++)
    {
        c_data_fifo *p_fifo;

        if (0 != port->m_port_fifo_size[i])
        {
            // allocate memory for the fifo
            p_fifo = (c_data_fifo *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                           sizeof(c_data_fifo), 
                                                           0
                                                           );
		    if (p_fifo)
		    {
                port->m_p_data_fifo[i] = c_data_fifo_constructor(p_fifo,
                                                                 &This->m_object, 
                                                                 OBJECT_CRITICAL_HEAVY,                                                                              
                                                                 port->m_port_fifo_size[i],
                                                                 This->m_dbg_id
                                                                 );
                if (!port->m_p_data_fifo[i])
		        {
		            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
			                       "zoe_module_base_port_set(%s) - c_data_fifo_constructor failed!\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object)))
				                   );
                    zoe_sosal_memory_free((void *)p_fifo);
                    err = ZOE_ERRS_NOMEMORY;
                    break;
		        }
		    }
            else
		    {                                       
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
		                       "zoe_module_base_port_set(%s) - zoe_sosal_memory_alloc local failed!\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object)))
				               );
                err = ZOE_ERRS_NOMEMORY;
                break;
		    }
        }
    }


    if (ZOE_SUCCESS(err))
    {
        memcpy(&port->m_port_conn, p_port, sizeof(ZOE_MODULE_DATA_CONNECTOR));
        port->m_port_set = ZOE_TRUE;
    }
    else
    {
        if (port->m_evt_dma)
        {
            zoe_sosal_event_delete(port->m_evt_dma);
            port->m_evt_dma = ZOE_NULL;
        }

        for (i = 0; i < 2; i++)
        {
            if (port->m_p_data_fifo[i])
            {
                c_data_fifo_destructor(port->m_p_data_fifo[i]);
                zoe_sosal_memory_free((void *)port->m_p_data_fifo[i]);
		        port->m_p_data_fifo[i] = ZOE_NULL;
            }
        }
    }

    return (err);
}



zoe_errs_t zoe_module_base_port_clear(c_zoe_module_base *This, 
                                      uint32_t sel
                                      )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    c_zoe_port  *port;
    uint32_t    i;

    if ((sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs))
        )
    {
        return (ZOE_ERRS_PARMS);
    }

    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];

    if (ZOE_STATE_STOPPED != port->m_state)
    {
        return (ZOE_ERRS_INVALID);
    }

    if (sel >= This->m_num_inputs)
    {
        // output
        // reset output to default
        port->m_down_stream_mem_type = ZOE_MODULE_MEM_TYPE_INTERNAL;
        port->m_down_stream_mem_usage = ZOE_MODULE_MEM_USAGE_COPY;
        port->m_p_write_2_port_func = zoe_module_base_write_to_port_null;
    }

    // delete data fifos
    for (i = 0; i < 2; i++)
    {
        if (port->m_p_data_fifo[i])
        {
            c_data_fifo_destructor(port->m_p_data_fifo[i]);
            zoe_sosal_memory_free((void *)port->m_p_data_fifo[i]);
		    port->m_p_data_fifo[i] = ZOE_NULL;
        }
    }

    // delete dma event
    if (port->m_evt_dma)
    {
        zoe_sosal_event_delete(port->m_evt_dma);
        port->m_evt_dma = ZOE_NULL;
    }


    memset(&port->m_port_conn, 0, sizeof(ZOE_MODULE_DATA_CONNECTOR));
    port->m_port_set = ZOE_FALSE;

    return (err);
}



zoe_errs_t zoe_module_base_port_get(c_zoe_module_base *This, 
                                    uint32_t sel, 
                                    ZOE_MODULE_DATA_CONNECTOR *p_port
                                    ) 
{
    c_zoe_port  *port;

    if ((sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (sel >= (ZOE_MODULE_DATA_SEL_PORT_START + This->m_num_inputs + This->m_num_outputs)) ||
        !p_port
        )
    {
        return (ZOE_ERRS_PARMS);
    }
    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This->m_ports[sel];

    if (!port->m_port_set)
    {
        return (ZOE_ERRS_NOTFOUND);
    }
    memcpy(p_port, &port->m_port_conn, sizeof(ZOE_MODULE_DATA_CONNECTOR));
    return (ZOE_ERRS_SUCCESS);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_thread
//
static void zoe_module_base_init_thread_events(c_thread *This_p, 
                                               zoe_sosal_event_wait_t *p_wait
                                               )
{
    c_zoe_module_base   *This = (c_zoe_module_base *)This_p->m_context;
    uint32_t            i;

    // wait on input port data availavle
    //
    for (i = 0; i < This->m_num_inputs; i++)
    {
        p_wait[i].event_id = This->m_ports[i].m_evt_port;
        p_wait[i].fired = ZOE_FALSE;
    }

    // plus any additional module specific events 
    //
    for (i = 0; i < This->m_num_extra_events; i++)
    {
        p_wait[This->m_num_inputs + i].event_id = This->m_evt_extra[i];
        p_wait[This->m_num_inputs + i].fired = ZOE_FALSE;
    }
}



static void zoe_module_base_handle_events(c_thread *This_p, 
                                          zoe_sosal_event_wait_t *p_wait
                                          )
{
    c_zoe_module_base   *This = (c_zoe_module_base *)This_p->m_context;
    uint32_t            i;
    zoe_errs_t          err;

    if (ZOE_STATE_STOPPED != This->m_state)
    {
        // input port event
        //
        for (i = 0; i < This->m_num_inputs; i++)
        {
            if (p_wait[i].fired)
            {
                err = This->processes(This, 
                                      i
                                      );
                if (ZOE_FAIL(err))
                {
                    if (ZOE_ERRS_CANCELLED == err)
                    {
                        // stopped, exit handle_event and go back to top of the thread proc
                        return;                        
                    }
                }
            }
        }

        // extra events
        //
        for (i = 0; i < This->m_num_extra_events; i++)
        {
            if (p_wait[This->m_num_inputs + i].fired)
            {
                err = This->notify(This,
                                   i
                                   );
                if (ZOE_FAIL(err))
                {
                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
                                   "zoe_module_base_handle_events(%s) - extra(%d) err(%d)\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                                   i,
                                   err
			                       );
                    if (ZOE_ERRS_CANCELLED == err)
                    {
                        return;                        
                    }
                }
            }
        }
    }
}



static void zoe_module_base_do_command(c_thread *This_p, 
                                       THREAD_CMD *p_cmd
                                       )
{
    c_zoe_module_base   *This = (c_zoe_module_base *)This_p->m_context;

    switch(p_cmd->dwCmdCode)
    {
        case ZOE_BASE_MODULE_THREAD_CMD_SET_STATE:
            switch (p_cmd->dwParam[0])
            {
                case ZOE_STATE_PLAYING:
                    *p_cmd->pdwError = This->do_play_cmd(This);
                    break;
                case ZOE_STATE_STOPPED:
                    *p_cmd->pdwError = This->do_stop_cmd(This);
                    break;
                case ZOE_STATE_PAUSED:
                    *p_cmd->pdwError = This->do_pause_cmd(This);
                    break;
                default:
                    break;
            }
            break;

        case ZOE_BASE_MODULE_THREAD_CMD_STOP_PORT:
            if (p_cmd->dwParam[0] < This->m_num_inputs)
            {
                //ENTER_CRITICAL(&This->m_object)
                MODULE_PRINTF("\r\n%s flush port(%d)\r\n", 
                              zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_object))),
                              (int)p_cmd->dwParam[0]
                              );
                zoe_port_flush(&This->m_ports[p_cmd->dwParam[0]]);
                //LEAVE_CRITICAL(&This->m_object)
            }
            break;

        default:
            break;
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_base
//
c_zoe_module_base * c_zoe_module_base_constructor(c_zoe_module_base *p_zoe_module_base,
							                      c_object *p_parent, 
                                                  uint32_t who_am_i,
							                      uint32_t attributes,
                                                  IZOEHALAPI *p_hal,
                                                  zoe_dbg_comp_id_t dbg_id,
                                                  uint32_t cpu,
                                                  uint32_t inst,
                                                  uint32_t num_inputs,
                                                  uint32_t num_outputs,
                                                  uint32_t num_extra_events,
                                                  int32_t timeout_us,
                                                  uint32_t priority,
                                                  uint32_t stack_words,
                                                  void * p_module,
                                                  char *p_thread_name,
                                                  zoe_bool_t create_module_thread,
                                                  zoe_bool_t create_rpc_thread
							                      )
{
    if (p_zoe_module_base)
    {
        zoe_errs_t  err;
        c_thread    *p_thread;
        uint32_t    i;
        char        name[ZOE_SOSAL_OBJNAME_MAXLEN] = {'\0'};

        // zero init ourselves
        //
        memset((void *)p_zoe_module_base, 
               0, 
               sizeof(c_zoe_module_base)
               );

        // c_object
        //
        c_object_constructor(&p_zoe_module_base->m_object, 
                             p_parent, 
                             who_am_i,
                             attributes
                             );

        p_zoe_module_base->m_create_module_thread = create_module_thread;
        p_zoe_module_base->m_create_rpc_thread = create_rpc_thread;

        if (create_module_thread)
        {
            // thread name
            sprintf(name, 
			        "%s_%d",
                    p_thread_name,
                    (int)inst
			        );

            // c_thread
            //
            p_thread = c_thread_constructor(&p_zoe_module_base->m_thread, 
                                            &name[0],
                                            (zoe_void_ptr_t)p_zoe_module_base,
                                            priority,
                                            stack_words,
                                            num_inputs + num_extra_events,
                                            timeout_us,
                                            ZOE_FALSE,   // do not remove user space mapping
                                            dbg_id
                                            );
		    if (!p_thread)
		    {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbg_id,
		                       "c_zoe_module_base_constructor c_thread_constructor() FAILED\n"
		                       );
                c_zoe_module_base_destructor(p_zoe_module_base);
                return (ZOE_NULL);
		    }

            // override do_command
            p_zoe_module_base->m_thread.m_do_command = zoe_module_base_do_command;
            // override init_events
            p_zoe_module_base->m_thread.m_init_events = zoe_module_base_init_thread_events;
            // override handle_event
            p_zoe_module_base->m_thread.m_handle_events = zoe_module_base_handle_events;
        }

        // c_zoe_module_base
        //
        p_zoe_module_base->m_p_hal = p_hal;
        p_zoe_module_base->m_dbg_id = dbg_id;
        p_zoe_module_base->m_cpu = cpu;
        p_zoe_module_base->m_inst = inst;
        p_zoe_module_base->m_p_module = p_module;
        p_zoe_module_base->m_state = ZOE_STATE_STOPPED;
        p_zoe_module_base->m_state_to = ZOE_STATE_STOPPED;
        zoe_module_base_get_memory_type(p_zoe_module_base);
        err = zoe_sosal_event_create(ZOE_NULL, 
                                     &p_zoe_module_base->m_evt_ack_state
                                     );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_base_constructor zoe_sosal_event_create() FAILED\n"
		                   );
            c_zoe_module_base_destructor(p_zoe_module_base);
            return (ZOE_NULL);
        }
        p_zoe_module_base->m_num_inputs = num_inputs;
        p_zoe_module_base->m_num_outputs = num_outputs;
        p_zoe_module_base->m_num_extra_events = num_extra_events;

        // data ports
        //
        if (0 != (num_inputs + num_outputs))
        {
            // allocate storage for ports
            p_zoe_module_base->m_ports = zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                sizeof(c_zoe_port) * (num_inputs + num_outputs), 
                                                                0
                                                                );
            if (!p_zoe_module_base->m_ports)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbg_id,
		                       "c_zoe_module_base_constructor zoe_sosal_memory_alloc m_ports() FAILED\n"
		                       );
                c_zoe_module_base_destructor(p_zoe_module_base);
                return (ZOE_NULL);
            }
            // zero init ports storage
            memset((void *)p_zoe_module_base->m_ports, 
                   0, 
                   sizeof(c_zoe_port) * (num_inputs + num_outputs)
                   );
        }        
        for (i = 0; i < num_inputs; i++)
        {
            p_zoe_module_base->m_ports[i].m_port_dir = ZOE_MODULE_DATA_IN;
        }
        for (i = 0; i < num_outputs; i++)
        {
            p_zoe_module_base->m_ports[i + num_inputs].m_port_dir = ZOE_MODULE_DATA_OUT;
        }

        // input port buffer descriptor fifo
        for (i = 0; i < num_inputs; i++)
        {
            c_fifo *p_fifo;

            // allocate memory for the fifo
            p_fifo = (c_fifo *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                     sizeof(c_fifo), 
                                                     0
                                                     );
		    if (p_fifo)
		    {
			    p_zoe_module_base->m_ports[i].m_p_input_bufdesc_fifo = c_fifo_constructor(p_fifo,
                                                                                          &p_zoe_module_base->m_object,
												                                          OBJECT_CRITICAL_HEAVY,
												                                          ZOE_MODULE_BUF_DESC_FIFO_DEPTH + 1,
											                                              sizeof(ZOE_BUFFER_DESCRIPTOR)
												                                          );
		    }

		    if (!p_zoe_module_base->m_ports[i].m_p_input_bufdesc_fifo)
		    {
                if (p_fifo)
                {
			        zoe_sosal_memory_free((void *)p_fifo);
                }

		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbg_id,
					           "c_zoe_module_base_constructor() - Unable to create m_p_input_bufdesc_fifos!\n"
					           );
                c_zoe_module_base_destructor(p_zoe_module_base);
                return (ZOE_NULL);
		    }
        }

        for (i = 0; i < (num_inputs + num_outputs); i++)
        {
            p_zoe_module_base->m_ports[i].m_id = i;
            p_zoe_module_base->m_ports[i].m_p_base_module = p_zoe_module_base;

            // default write to port method
            p_zoe_module_base->m_ports[i].m_p_write_2_port_func = zoe_module_base_write_to_port_null;

            // port events
            err = zoe_sosal_event_create(ZOE_NULL, 
                                         &p_zoe_module_base->m_ports[i].m_evt_port
                                         );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbg_id,
		                       "c_zoe_module_base_constructor zoe_sosal_event_create() FAILED\n"
		                       );
                c_zoe_module_base_destructor(p_zoe_module_base);
                return (ZOE_NULL);
            }
        }

        for (i = 0; i < num_extra_events; i++)
        {
            err = zoe_sosal_event_create(ZOE_NULL, 
                                         &p_zoe_module_base->m_evt_extra[i]
                                         );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbg_id,
		                       "c_zoe_module_base_constructor zoe_sosal_event_create() FAILED\n"
		                       );
                c_zoe_module_base_destructor(p_zoe_module_base);
                return (ZOE_NULL);
            }
        }

        // setup default process and notify
        p_zoe_module_base->processes = zoe_module_base_processes;
        p_zoe_module_base->process = zoe_module_base_process;
        p_zoe_module_base->notify = zoe_module_base_notify;

        // setup default state transition handler
        p_zoe_module_base->do_play = zoe_module_base_do_play;        
        p_zoe_module_base->do_stop = zoe_module_base_do_stop;        
        p_zoe_module_base->do_pause = zoe_module_base_do_pause;

        // setup default state transition thread command handler
        p_zoe_module_base->do_play_cmd = zoe_module_base_do_play_cmd;        
        p_zoe_module_base->do_stop_cmd = zoe_module_base_do_stop_cmd;        
        p_zoe_module_base->do_pause_cmd = zoe_module_base_do_pause_cmd;

        // setup default unblock function for stop 
        p_zoe_module_base->unblock = zoe_module_base_unblock;

        // default module control interface
        p_zoe_module_base->play = zoe_module_base_play;
        p_zoe_module_base->stop = zoe_module_base_stop;
        p_zoe_module_base->flush = zoe_module_base_flush;
        p_zoe_module_base->pause = zoe_module_base_pause;
        p_zoe_module_base->get_state = zoe_module_base_get_state;
        p_zoe_module_base->evt_notify = zoe_module_base_evt_notify;

        // default module data interface
        p_zoe_module_base->allocate_buffer = zoe_module_base_allocate_buffer;
        p_zoe_module_base->release_buffer = zoe_module_base_release_buffer;
        p_zoe_module_base->release_buffer_with_info = zoe_module_base_release_buffer_with_info;
        p_zoe_module_base->allocate_yuv_buffer = zoe_module_base_allocate_yuv_buffer;
        p_zoe_module_base->release_yuv_buffer = zoe_module_base_release_yuv_buffer;
        p_zoe_module_base->release_yuv_buffer_with_info = zoe_module_base_release_yuv_buffer_with_info;
        p_zoe_module_base->get_mem_type = zoe_module_base_get_mem_type;
        p_zoe_module_base->get_mem_usage = zoe_module_base_get_mem_usage;
        p_zoe_module_base->write = zoe_module_base_write;
        p_zoe_module_base->buffer_available = zoe_module_base_buffer_available;

        // register module control api
        err = c_zoe_ipc_service_register_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                   zoe_module_control_intf_interface,
                                                   ZOE_MODULE_ID(who_am_i),
                                                   p_zoe_module_base->m_inst,
                                                   (void *)p_zoe_module_base,
                                                   zoe_module_control_intf_dispatch,
                                                   create_rpc_thread,
                                                   zoe_sosal_thread_maxpriorities_get() - 1
                                                   );
        if (ZOE_SUCCESS(err))
        {
            p_zoe_module_base->m_intf_control_registered = ZOE_TRUE;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_base_constructor c_zoe_ipc_service_register_interface() zoe_module_control_intf_interface FAILED\n"
		                   );
            c_zoe_module_base_destructor(p_zoe_module_base);
            return (ZOE_NULL);
        }

        // register module connection api
        err = c_zoe_ipc_service_register_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                   zoe_module_connection_intf_interface,
                                                   ZOE_MODULE_ID(who_am_i),
                                                   p_zoe_module_base->m_inst,
                                                   (void *)p_zoe_module_base,
                                                   zoe_module_connection_intf_dispatch,
                                                   create_rpc_thread,
                                                   zoe_sosal_thread_maxpriorities_get() - 1
                                                   );
        if (ZOE_SUCCESS(err))
        {
            p_zoe_module_base->m_intf_connection_registered = ZOE_TRUE;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_base_constructor c_zoe_ipc_service_register_interface() zoe_module_connection_intf_interface FAILED\n"
		                   );
            c_zoe_module_base_destructor(p_zoe_module_base);
            return (ZOE_NULL);
        }

        // register module data api
        err = c_zoe_ipc_service_register_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                   zoe_module_data_intf_interface,
                                                   ZOE_MODULE_ID(who_am_i),
                                                   p_zoe_module_base->m_inst,
                                                   (void *)p_zoe_module_base,
                                                   zoe_module_data_intf_dispatch,
                                                   create_rpc_thread,
                                                   zoe_sosal_thread_maxpriorities_get() - 1
                                                   );
        if (ZOE_SUCCESS(err))
        {
            p_zoe_module_base->m_intf_data_registered = ZOE_TRUE;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_base_constructor c_zoe_ipc_service_register_interface() zoe_module_data_intf_interface FAILED\n"
		                   );
            c_zoe_module_base_destructor(p_zoe_module_base);
            return (ZOE_NULL);
        }
    }

    return (p_zoe_module_base);
}



void c_zoe_module_base_destructor(c_zoe_module_base *This)
{
    zoe_errs_t  err;
    c_zoe_port  *port;
    uint32_t    i, j;

    // make sure the module is stopped
    //
    zoe_module_base_stop(This, 
                         ZOE_MODULE_DATA_SEL_MODULE
                         );

	// stop the module thread if not done so already
	//
    if (This->m_create_module_thread)
    {
        c_thread_thread_done(&This->m_thread);
    }
    
    // delete port resources
    //
    for (i = 0; i < (This->m_num_inputs + This->m_num_outputs); i++)
    {
        port = &This->m_ports[i];

        // flush the port
        zoe_port_flush(port);

        // delete input buf desc fifos
        //
	    if (port->m_p_input_bufdesc_fifo)
	    {
		    c_fifo_destructor(port->m_p_input_bufdesc_fifo);
		    zoe_sosal_memory_free((void *)port->m_p_input_bufdesc_fifo);
		    port->m_p_input_bufdesc_fifo = ZOE_NULL;
	    }

        // delete data fifos
        //
        for (j = 0; j < 2; j++)
        {
            if (port->m_p_data_fifo[j])
            {
                c_data_fifo_destructor(port->m_p_data_fifo[j]);
                zoe_sosal_memory_free((void *)port->m_p_data_fifo[j]);
		        port->m_p_data_fifo[j] = ZOE_NULL;
            }
        }

        if (port->m_evt_port)
        {
            zoe_sosal_event_delete(port->m_evt_port);
            port->m_evt_port = ZOE_NULL;
        }

        if (port->m_evt_dma)
        {
            zoe_sosal_event_delete(port->m_evt_dma);
            port->m_evt_dma = ZOE_NULL;
        }
    }

    // delete any additional thread events
    //
    for (i = 0; i < This->m_num_extra_events; i++)
    {
        if (This->m_evt_extra[i])
        {
            zoe_sosal_event_delete(This->m_evt_extra[i]);
            This->m_evt_extra[i] = ZOE_NULL;
        }
    }

    // delete set state ack event
    //
    if (This->m_evt_ack_state)
    {
        zoe_sosal_event_delete(This->m_evt_ack_state);
        This->m_evt_ack_state = ZOE_NULL;
    }

    // unregister module connection api
    if (This->m_intf_connection_registered)
    {
        err = c_zoe_ipc_service_unregister_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                     zoe_module_connection_intf_interface,
                                                     ZOE_MODULE_ID(c_object_who_am_i(&This->m_object)),
                                                     This->m_inst
                                                     );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "c_zoe_module_base_destructor c_zoe_ipc_service_unregister_interface() zoe_module_connection_intf_interface FAILED\n"
		                   );
        }
        This->m_intf_connection_registered = ZOE_FALSE;
    }

    // unregister module data api
    if (This->m_intf_data_registered)
    {
        err = c_zoe_ipc_service_unregister_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                     zoe_module_data_intf_interface,
                                                     ZOE_MODULE_ID(c_object_who_am_i(&This->m_object)),
                                                     This->m_inst
                                                     );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "c_zoe_module_base_destructor c_zoe_ipc_service_unregister_interface() zoe_module_data_intf_interface FAILED\n"
		                   );
        }
        This->m_intf_data_registered = ZOE_FALSE;
    }

    // unregister module control api
    if (This->m_intf_control_registered)
    {
        err = c_zoe_ipc_service_unregister_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                     zoe_module_control_intf_interface,
                                                     ZOE_MODULE_ID(c_object_who_am_i(&This->m_object)),
                                                     This->m_inst
                                                     );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "c_zoe_module_base_destructor c_zoe_ipc_service_unregister_interface() zoe_module_control_intf_interface FAILED\n"
		                   );
        }
        This->m_intf_control_registered = ZOE_FALSE;
    }

    if (This->m_ports)
    {
		zoe_sosal_memory_free((void *)This->m_ports);
        This->m_ports = ZOE_NULL;
    }

    // c_thread
    //
    if (This->m_create_module_thread)
    {
        c_thread_destructor(&This->m_thread);
    }

	// c_object
	//
	c_object_destructor(&This->m_object);
}



