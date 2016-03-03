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
// source.c
//
// Description: 
//
//  ZOE streaming core source module
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_module_core_src.h"
#include "c_zoe_module_core_src.h"
#include "zoe_sosal.h"
#include "zoe_ipc_srv.h"
#include "zoe_module_mgr.h"
#include "zoe_module_data_intf_clnt.h"
#include "zoe_module_objids.h"
#include "zv_codec.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD

#define CORE_SRC_NO_THREAD


/////////////////////////////////////////////////////////////////////////////
//
//

#define ENTER_CRITICAL_CHANNEL_PORT(This, port)  zoe_sosal_mutex_get(This->m_critical_section[port], -1);
#define LEAVE_CRITICAL_CHANNEL_PORT(This, port)  zoe_sosal_mutex_release(This->m_critical_section[port]);


/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_core_source
//
static zoe_errs_t zoe_module_core_source_streaming(c_zoe_module_core_source *This, 
                                                   uint32_t start_id
                                                   )
{
    c_zoe_port              *port;
    c_channel               *p_channel;
    uint8_t                 *p_buffer;
    uint8_t                 *p_buffer_chroma;
    uint32_t                length, length_chroma;
    PZV_BUFFER_DESCRIPTOR	p_buf_desc;
    ZOE_BUFFER_DESCRIPTOR   buf_desc;
    uint32_t                written[2];
    zoe_void_ptr_t          p_user_mappings[2];
    zoe_errs_t              err, last_err = ZOE_ERRS_SUCCESS;
    uint32_t                i, cur_id;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_base.m_dbg_id,
	               "zoe_module_core_source_streaming start_id(%d)\n",
                   start_id
	               );

    // fill in common portion of the zoe buffer descriptor
    //
    memset(&buf_desc, 
           0, 
           sizeof(ZOE_BUFFER_DESCRIPTOR)
           );
    buf_desc.owner.addr.cpu = This->m_base.m_cpu;
    buf_desc.owner.addr.module = ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE);
    buf_desc.owner.addr.inst = This->m_base.m_inst;

    // walk through all connected ports starts from "start_id"
    //
    for (i = 0; i < ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT; i++)
    {
        cur_id = (start_id + i) % ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT;
        buf_desc.owner.selector = ZOE_MODULE_DATA_SEL_PORT_START + cur_id;
        port = &This->m_base.m_ports[cur_id];

        ENTER_CRITICAL_CHANNEL_PORT(This, cur_id)

        p_channel = This->m_p_channels[cur_id];

        if ((port->m_port_set) &&
            p_channel
            )
        {
            // get a buffer from the channel
            while (ZOE_STATE_PLAYING == port->m_state)
            {
                if (c_channel_is_yuv(p_channel))
                {
                    if (!p_channel->get_buffer_yuv(p_channel,
                                                   &p_buf_desc,
                                                   &p_buffer,
                                                   &length,
                                                   &p_buffer_chroma,
                                                   &length_chroma
                                                   ))
                    {
                        break;
                    }

                    buf_desc.buffers[ZOE_BUF_CHROMA].buf_ptr = (zoe_dev_mem_t)((zoe_uintptr_t)p_buffer_chroma);
                    buf_desc.buffers[ZOE_BUF_CHROMA].buf_ptr_va = (zoe_dev_mem_t)((zoe_uintptr_t)p_buf_desc->DataBuffer[1].Data_mapped);
                    buf_desc.buffers[ZOE_BUF_CHROMA].size = length_chroma;
                    buf_desc.buffers[ZOE_BUF_CHROMA].offset = 0;
                    buf_desc.buffers[ZOE_BUF_CHROMA].valid_size = length_chroma;

                    p_user_mappings[1] = p_buf_desc->DataBuffer[1].p_user_mappings;

                    buf_desc.info.flags |= ZOE_BUF_DESC_FLAGS_SEP_CHROMA;
                }
                else
                {
                    if (!p_channel->get_buffer(p_channel,
                                               &p_buf_desc,
                                               &p_buffer,
                                               &length
                                               ))
                    {
                        break;
                    }

                    buf_desc.info.flags &= ~ZOE_BUF_DESC_FLAGS_SEP_CHROMA;
                }
                buf_desc.buffers[ZOE_BUF_DATA].buf_ptr = (zoe_dev_mem_t)((zoe_uintptr_t)p_buffer);
                buf_desc.buffers[ZOE_BUF_DATA].buf_ptr_va = (zoe_dev_mem_t)((zoe_uintptr_t)p_buf_desc->DataBuffer[0].Data_mapped);
                buf_desc.buffers[ZOE_BUF_DATA].size = length;
                buf_desc.buffers[ZOE_BUF_DATA].offset = 0;
                buf_desc.buffers[ZOE_BUF_DATA].valid_size = length;

                p_user_mappings[0] = p_buf_desc->DataBuffer[0].p_user_mappings;

                // set kernel mapped flag
                buf_desc.info.flags |= ZOE_BUF_DESC_FLAGS_MAP_KERNEL;

                // clear meta data flag
                buf_desc.info.flags &= ~ZOE_BUF_DESC_FLAGS_META;

                // transfer user buffer flags to firmware buffer
                //
                if (CHANNEL_DIR_WRITE == p_channel->m_ChannelDirection)
                {
                    if (p_buf_desc->ulFlags & ZV_BUFDESC_FLAG_EOS)
                    {
                        buf_desc.info.flags |= ZOE_BUF_DESC_FLAGS_EOS;
                    }
                    if (p_buf_desc->ulFlags & ZV_BUFDESC_FLAG_PTS)
                    {
                        buf_desc.info.flags |= ZOE_BUF_DESC_FLAGS_PTS;
                        buf_desc.info.pts = (uint64_t)p_buf_desc->ulPTS;
                    }
                    buf_desc.info.flags |= (p_buf_desc->ulFlags & DMA_BUFFER_MODE_MASK);
                }

	            zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_base.m_dbg_id,
	                           "core_src write_2_port(%d) port(%d) ptr(%p) fw_flags(0x%x) usr_flags(0x%x)[\n",
                               This->m_base.m_inst,
                               cur_id,
                               p_buffer,
                               buf_desc.info.flags,
                               p_buf_desc->ulFlags
	                           );
                // write to downstream port
                err = zoe_module_base_write_to_port(&This->m_base,
                                                    cur_id,
                                                    &buf_desc,
                                                    ZOE_MODULE_DATA_REQ_BLOCKING,
                                                    written,
                                                    p_user_mappings
                                                    );
                if (ZOE_FAIL(err) ||
                    (written[0] != buf_desc.buffers[ZOE_BUF_DATA].valid_size)
                    )
                {
                    // can not write to downstream
                    //
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_base.m_dbg_id,
	                               "zoe_module_core_source_streaming zoe_module_base_write_to_port() err(%d)\n",
                                   err
	                               );
                    last_err = err;
                }

	            zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_base.m_dbg_id,
	                           "core_src write_2_port(%d) ]\n",
                               This->m_base.m_inst
	                           );
                // update the status
		        p_buf_desc->Status = err;

		        // return this buffer
                if (ZOE_MODULE_MEM_USAGE_COPY == port->m_down_stream_mem_usage)
                {
		            p_channel->complete_buffer(p_channel,
								               p_buf_desc
								               );
                }
            }
        }

        LEAVE_CRITICAL_CHANNEL_PORT(This, cur_id)
    }
    return (last_err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_base
//
static zoe_errs_t zoe_module_core_source_notify(c_zoe_module_base *This_p, 
                                                uint32_t evt
                                                )
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    return (zoe_module_core_source_streaming(This, 
                                             evt
                                             ));
}



static zoe_errs_t zoe_module_core_source_do_play_cmd(c_zoe_module_base *This_p)
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    c_zoe_port                  *port;
    uint32_t                    i;

    for (i = 0; i < ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT; i++)
    {
        port = &This_p->m_ports[i];
        if (port->m_port_set)
        {
#ifdef CORE_SRC_NO_THREAD
            zoe_module_core_source_streaming(This, i);
#else //!CORE_SRC_NO_THREAD
            zoe_sosal_event_set(This->m_base.m_evt_extra[i]);
#endif //CORE_SRC_NO_THREAD
        }
    }

    return (ZOE_ERRS_SUCCESS);
}



// module control interface API
//
static zoe_state_t zoe_module_core_source_evt_notify(c_zoe_module_base *This_p, 
                                                     uint32_t sel,
                                                     uint32_t evt,
                                                     uint32_t evt_data[4]
                                                     )
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    c_channel                   *p_channel;

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This_p->m_dbg_id,
	               "zoe_module_core_source_evt_notify(%s) sel(%d) evt(%d)\n",
                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This_p->m_object))),
                   sel,
                   evt
	               );
    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START)
        ) 
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbg_id,
		               "zoe_module_core_source_evt_notify invalid sel(%d)!!!\n",
                       sel
		               );
        return (ZOE_ERRS_PARMS);
    }

    p_channel = This->m_p_channels[sel - ZOE_MODULE_DATA_SEL_PORT_START];

    if (p_channel)
    {
		c_channel_device_callback(p_channel,
								  ZVCODEC_CMD_DEC_SEQ_PARAM_CHANGED,
								  &evt_data[0]
								  );
    }

    return (ZOE_ERRS_SUCCESS);
}



// module data interface API
//
static zoe_errs_t zoe_module_core_source_release_buffer(c_zoe_module_base *This_p, 
                                                        uint32_t sel, 
                                                        uint32_t buf_sel,
                                                        zoe_dev_mem_t dev_mem,
                                                        uint32_t size
                                                        )
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    c_zoe_port                  *port;
    c_channel                   *p_channel;
    PZV_BUFFER_DESCRIPTOR       p_buf_desc;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (buf_sel != ZOE_BUF_DATA)
        ) 
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbg_id,
		               "zoe_module_core_source_release_buffer sel(%d) buf_sel(%d)!!!\n",
                       sel,
                       buf_sel
		               );
        return (ZOE_ERRS_PARMS);
    }

    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This_p->m_ports[sel];

    if (ZOE_MODULE_MEM_USAGE_IN_PLACE == port->m_down_stream_mem_usage)
    {
        p_channel = This->m_p_channels[sel];

        if (p_channel)
        {
            p_buf_desc = c_channel_find_buf_desc_by_buf_ptr(p_channel, 
                                                            (zoe_void_ptr_t)dev_mem
                                                            );
            if (p_buf_desc)
            {
		        p_channel->complete_buffer(p_channel,
							               p_buf_desc
							               );
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This_p->m_dbg_id,
		                       "zoe_module_core_source_release_buffer port(%d) mem(0x%p) size(%d) not found!!!\n",
                               sel,
                               (void *)dev_mem,
                               size
		                       );
                return (ZOE_ERRS_NOTFOUND);
            }
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This_p->m_dbg_id,
		                   "zoe_module_core_source_release_buffer port(%d) no channel!!!\n",
                           sel
		                   );
            return (ZOE_ERRS_INVALID);
        }
    }


    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_core_source_release_buffer_with_info(c_zoe_module_base *This_p, 
                                                                  uint32_t sel, 
                                                                  uint32_t buf_sel,
                                                                  zoe_dev_mem_t dev_mem,
                                                                  uint32_t size,
                                                                  ZOE_BUFFER_INFO* buf_info
                                                                  )
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    c_zoe_port                  *port;
    c_channel                   *p_channel;
    PZV_BUFFER_DESCRIPTOR       p_buf_desc;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (buf_sel != ZOE_BUF_DATA)
        ) 
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbg_id,
		               "zoe_module_core_source_release_buffer_with_info sel(%d) buf_sel(%d)!!!\n",
                       sel,
                       buf_sel
		               );
        return (ZOE_ERRS_PARMS);
    }

    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This_p->m_ports[sel];

    if (ZOE_MODULE_MEM_USAGE_IN_PLACE == port->m_down_stream_mem_usage)
    {
        p_channel = This->m_p_channels[sel];

        if (p_channel)
        {
            p_buf_desc = c_channel_find_buf_desc_by_buf_ptr(p_channel, 
                                                            (zoe_void_ptr_t)dev_mem
                                                            );
            if (p_buf_desc)
            {
                if (CHANNEL_DIR_READ == p_channel->m_ChannelDirection)
                {
                    if (buf_info->flags & ZOE_BUF_DESC_FLAGS_PTS)
                    {
                        p_buf_desc->ulPTS = buf_info->pts;
                        p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_PTS;
                    }
                    if (buf_info->flags & ZOE_BUF_DESC_FLAGS_EOS)
                    {
                        p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_EOS;
                    }
                    p_buf_desc->DataBuffer[0].DataUsed = size;
                }
		        p_channel->complete_buffer(p_channel,
							               p_buf_desc
							               );
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This_p->m_dbg_id,
		                       "zoe_module_core_source_release_buffer_with_info port(%d) mem(0x%p) size(%d) not found!!!\n",
                               sel,
                               (void *)dev_mem,
                               size
		                       );
                return (ZOE_ERRS_NOTFOUND);
            }
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This_p->m_dbg_id,
		                   "zoe_module_core_source_release_buffer_with_info port(%d) no channel!!!\n",
                           sel
		                   );
            return (ZOE_ERRS_INVALID);
        }
    }

    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_core_source_release_yuv_buffer(c_zoe_module_base *This_p, 
                                                            uint32_t sel, 
                                                            uint32_t num_planes,
                                                            zoe_dev_mem_t dev_mem[3],
                                                            uint32_t size[3]
                                                            )
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    c_zoe_port                  *port;
    c_channel                   *p_channel;
    PZV_BUFFER_DESCRIPTOR       p_buf_desc;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (num_planes != 2)
        ) 
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbg_id,
		               "zoe_module_core_source_release_yuv_buffer sel(%d) num_planes(%d)!!!\n",
                       sel,
                       num_planes
		               );
        return (ZOE_ERRS_PARMS);
    }

    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This_p->m_ports[sel];

    if (ZOE_MODULE_MEM_USAGE_IN_PLACE == port->m_down_stream_mem_usage)
    {
        p_channel = This->m_p_channels[sel];

        if (p_channel)
        {
            p_buf_desc = c_channel_find_buf_desc_by_buf_ptr(p_channel, 
                                                            (zoe_void_ptr_t)dev_mem[0]
                                                            );
            if (p_buf_desc)
            {
		        p_channel->complete_buffer(p_channel,
							               p_buf_desc
							               );
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This_p->m_dbg_id,
		                       "zoe_module_core_source_release_yuv_buffer port(%d) mem(0x%p) size(%d) not found!!!\n",
                               sel,
                               (void *)dev_mem[0],
                               size[0]
		                       );
                return (ZOE_ERRS_NOTFOUND);
            }
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This_p->m_dbg_id,
		                   "zoe_module_core_source_release_buffer port(%d) no channel!!!\n",
                           sel
		                   );
            return (ZOE_ERRS_INVALID);
        }
    }


    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_core_source_release_yuv_buffer_with_info(c_zoe_module_base *This_p, 
                                                                      uint32_t sel, 
                                                                      uint32_t num_planes,
                                                                      zoe_dev_mem_t dev_mem[3],
                                                                      uint32_t size[3],
                                                                      ZOE_BUFFER_INFO* buf_info
                                                                      )
{
    c_zoe_module_core_source    *This = (c_zoe_module_core_source *)This_p->m_p_module;
    c_zoe_port                  *port;
    c_channel                   *p_channel;
    PZV_BUFFER_DESCRIPTOR       p_buf_desc;

    if ((sel >= (ZOE_MODULE_DATA_SEL_PORT_START + ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT)) ||
        (sel < ZOE_MODULE_DATA_SEL_PORT_START) ||
        (num_planes != 2)
        ) 
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbg_id,
		               "zoe_module_core_source_release_yuv_buffer_with_info sel(%d) num_planes(%d)!!!\n",
                       sel,
                       num_planes
		               );
        return (ZOE_ERRS_PARMS);
    }

    sel -= ZOE_MODULE_DATA_SEL_PORT_START;
    port = &This_p->m_ports[sel];

    if (ZOE_MODULE_MEM_USAGE_IN_PLACE == port->m_down_stream_mem_usage)
    {
        p_channel = This->m_p_channels[sel];

        if (p_channel)
        {
            p_buf_desc = c_channel_find_buf_desc_by_buf_ptr(p_channel, 
                                                            (zoe_void_ptr_t)dev_mem[0]
                                                            );
            if (p_buf_desc)
            {
                if (CHANNEL_DIR_READ == p_channel->m_ChannelDirection)
                {
                    if (buf_info->flags & ZOE_BUF_DESC_FLAGS_PTS)
                    {
                        p_buf_desc->ulPTS = buf_info->pts;
                        p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_PTS;
                    }
                    if (buf_info->flags & ZOE_BUF_DESC_FLAGS_EOS)
                    {
                        p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_EOS;
                    }
                    p_buf_desc->DataBuffer[0].DataUsed = size[0];
                    p_buf_desc->DataBuffer[1].DataUsed = size[1];
                }
		        p_channel->complete_buffer(p_channel,
							               p_buf_desc
							               );
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This_p->m_dbg_id,
		                       "zoe_module_core_source_release_yuv_buffer_with_info port(%d) mem(0x%p) size(%d) not found!!!\n",
                               sel,
                               (void *)dev_mem[0],
                               size[0]
		                       );
                return (ZOE_ERRS_NOTFOUND);
            }
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This_p->m_dbg_id,
		                   "zoe_module_core_source_release_yuv_buffer_with_info port(%d) no channel!!!\n",
                           sel
		                   );
            return (ZOE_ERRS_INVALID);
        }
    }

    return (ZOE_ERRS_SUCCESS);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_core_source
//

static void c_zoe_module_core_source_destructor(c_zoe_module_core_source *This)
{
    uint32_t    i;

    // c_zoe_module_base
    c_zoe_module_base_destructor(&This->m_base);

    // delete per port critical section
    for (i = 0; i < ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT; i++)
    {
        if (This->m_critical_section[i])
        {
            zoe_sosal_mutex_delete(This->m_critical_section[i]);
            This->m_critical_section[i] = ZOE_NULL;
        }
    }
}



static c_zoe_module_core_source * c_zoe_module_core_source_constructor(c_zoe_module_core_source *p_zoe_module_core_source,
											                           c_object *p_parent, 
							                                           uint32_t attributes,
                                                                       IZOEHALAPI *p_hal,
                                                                       zoe_dbg_comp_id_t dbg_id,
                                                                       uint32_t inst
							                                           )
{
    if (p_zoe_module_core_source)
    {
        c_zoe_module_base           *p_base;
        zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
        uint32_t                    cpu = (uint32_t)c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
        uint32_t                    max_priority = zoe_sosal_thread_maxpriorities_get();
        uint32_t                    i;

        // zero init ourselves
        //
        memset((void *)p_zoe_module_core_source, 
               0, 
               sizeof(c_zoe_module_core_source)
               );

        // c_zoe_module_base
        //
        p_base = c_zoe_module_base_constructor(&p_zoe_module_core_source->m_base,
							                   p_parent, 
                                               OBJECT_ZOE_MODULE_CORE_SOURCE,
							                   attributes,
                                               p_hal,
                                               dbg_id,
                                               cpu,
                                               inst,
                                               ZOE_MODULE_CORE_SOURCE_NUM_INPUT,
                                               ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT,
                                               ZOE_MODULE_CORE_SOURCE_NUM_EXTRA,
                                               ZOE_MODULE_CORE_SOURCE_TIMEOUT_US,
                                               max_priority - 1,
                                               4096,
                                               (void *)p_zoe_module_core_source,
                                               ZOE_MODULE_NAME_CORE_SOURCE,
#ifdef CORE_SRC_NO_THREAD
                                               ZOE_FALSE,
                                               ZOE_FALSE
#else //!CORE_SRC_NO_THREAD
                                               ZOE_TRUE,
                                               ZOE_TRUE
#endif //CORE_SRC_NO_THREAD
							                   );
		if (!p_base)
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_core_source_constructor c_zoe_module_base_constructor() FAILED\n"
		                   );
            c_zoe_module_core_source_destructor(p_zoe_module_core_source);
            return (ZOE_NULL);
		}

        // c_zoe_module_core_source
        //

        // create per port critical section
        for (i = 0; i < ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT; i++)
        {
            zoe_sosal_mutex_create(ZOE_NULL,
                                   &p_zoe_module_core_source->m_critical_section[i]
                                   );
        }

        // setup port type and sub_type
        for (i = 0; i < ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT; i++)
        {
            p_zoe_module_core_source->m_base.m_ports[i].m_port_type = ZOE_MODULE_DATA_TYPE_NONE;
            p_zoe_module_core_source->m_base.m_ports[i].m_port_sub_type = ZOE_MODULE_DATA_SUB_TYPE_NONE;
        }

        // override c_zoe_module_base event notification handler
        p_zoe_module_core_source->m_base.notify = zoe_module_core_source_notify;
#ifdef CORE_SRC_NO_THREAD
        // override c_zoe_module_base play handler from the calling thread
        p_zoe_module_core_source->m_base.do_play = zoe_module_core_source_do_play_cmd;
#else //!CORE_SRC_NO_THREAD
        // override c_zoe_module_base play command handler
        p_zoe_module_core_source->m_base.do_play_cmd = zoe_module_core_source_do_play_cmd;
#endif //CORE_SRC_NO_THREAD
        // override module control interface evt_notify()
        p_zoe_module_core_source->m_base.evt_notify = zoe_module_core_source_evt_notify;
        // override module data interface release_buffer()
        p_zoe_module_core_source->m_base.release_buffer = zoe_module_core_source_release_buffer;
        // override module data interface release_buffer_with_info()
        p_zoe_module_core_source->m_base.release_buffer_with_info = zoe_module_core_source_release_buffer_with_info;
        // override module data interface release_yuv_buffer()
        p_zoe_module_core_source->m_base.release_yuv_buffer = zoe_module_core_source_release_yuv_buffer;
        // override module data interface release_yuv_buffer_with_info()
        p_zoe_module_core_source->m_base.release_yuv_buffer_with_info = zoe_module_core_source_release_yuv_buffer_with_info;

#ifndef CORE_SRC_NO_THREAD
        // create source main thread
		if (!c_thread_thread_init(&p_zoe_module_core_source->m_base.m_thread))
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_core_source_constructor c_thread_thread_init() FAILED\n"
                           );
            c_zoe_module_core_source_destructor(p_zoe_module_core_source);
            return (ZOE_NULL);
		}
#endif //!CORE_SRC_NO_THREAD
    }

    return (p_zoe_module_core_source);
}



/////////////////////////////////////////////////////////////////////////////
//
//

zoe_void_ptr_t zoe_module_core_source_create(IZOEHALAPI *p_hal,
                                             zoe_dbg_comp_id_t dbg_id,
                                             uint32_t inst
                                             )
{
    c_zoe_module_core_source    *p_source, *p_ret;
    c_zoe_module_mgr            *p_mgr = c_zoe_module_mgr_get_module_mgr();

    // create c_zoe_module_core_source
    //
    p_source = (c_zoe_module_core_source *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                  sizeof(c_zoe_module_core_source), 
                                                                  0
                                                                  );
    if (!p_source)
    {
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       dbg_id,
		               "zoe_module_core_source_create() zoe_sosal_memory_alloc() failed!\n"
		               );
        // fail to allocate memory
        //
        return (ZOE_NULL);
    }

    p_ret = c_zoe_module_core_source_constructor(p_source,
										         &p_mgr->m_object, 
							                     OBJECT_CRITICAL_HEAVY,
                                                 p_hal,
                                                 dbg_id,
                                                 inst
							                     );
    if (!p_ret)
    {
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       dbg_id,
                       "zoe_module_core_source_create() c_zoe_module_core_source_constructor failed!!!\n" 
                       );
        zoe_sosal_memory_free((void *)p_source);
    }

    return ((zoe_void_ptr_t)p_ret);
}



zoe_errs_t zoe_module_core_source_destroy(zoe_void_ptr_t p_inst_data)
{
    c_zoe_module_core_source    *p_source = (c_zoe_module_core_source *)p_inst_data;

    if (p_source)
    {
        c_zoe_module_core_source_destructor(p_source);
        zoe_sosal_memory_free((void *)p_source);
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//

static const ZV_CODEC_OPEN_TYPE s_port_2_channel_map[ZOE_MODULE_CORE_SOURCE_NUM_OUTPUT] = 
{
	ZV_CODEC_VID_OUT,   // 0
	ZV_CODEC_META_OUT,  // 1
	ZV_CODEC_YUV_OUT,   // 2
	ZV_CODEC_VID_IN,    // 3
	ZV_CODEC_META_IN,   // 4
	ZV_CODEC_YUV_IN     // 5
};


static const int32_t s_channel_2_port_map[ZV_CODEC_OPEN_END] = 
{
	0,  // ZV_CODEC_VID_OUT
	1,  // ZV_CODEC_META_OUT
	2,  // ZV_CODEC_YUV_OUT
	3,  // ZV_CODEC_VID_IN
	4,  // ZV_CODEC_META_IN
	5,  // ZV_CODEC_YUV_IN
	-1  // ZV_CODEC_VIRTUAL
};



// interfaces for local cpu
//
zoe_errs_t c_zoe_module_core_source_open(c_zoe_module_core_source *This,
							             ZV_CODEC_OPEN_TYPE data_type,
							             c_channel *p_channel
							             )
{
    int32_t port = s_channel_2_port_map[data_type];

    if ((-1 != port) &&
        p_channel
        )
    {
        ENTER_CRITICAL_CHANNEL_PORT(This, port)
        This->m_p_channels[port] = p_channel;
        LEAVE_CRITICAL_CHANNEL_PORT(This, port)
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



zoe_errs_t c_zoe_module_core_source_close(c_zoe_module_core_source *This,
							              ZV_CODEC_OPEN_TYPE data_type
							              )
{
    int32_t port = s_channel_2_port_map[data_type];

    if (-1 != port)
    {
        ENTER_CRITICAL_CHANNEL_PORT(This, port)
        This->m_p_channels[port] = ZOE_NULL;
        LEAVE_CRITICAL_CHANNEL_PORT(This, port)
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



zoe_errs_t c_zoe_module_core_source_cancel_buffer(c_zoe_module_core_source *This,
						                          ZV_CODEC_OPEN_TYPE data_type,
                                                  PZV_BUFFER_DESCRIPTOR pBufDesc
                                                  )
{
    int32_t     port_id = s_channel_2_port_map[data_type];
    c_zoe_port  *port;

    if (-1 != port_id)
    {
        port = &This->m_base.m_ports[port_id];
        if (ZOE_MODULE_MEM_USAGE_COPY == port->m_down_stream_mem_usage)
        {
            ENTER_CRITICAL_CHANNEL_PORT(This, port_id)
            // if we can grab the mutex that must mean we are not streaming 
            // and the buffer is either not in use or released already
            //
            LEAVE_CRITICAL_CHANNEL_PORT(This, port_id)
            return (ZOE_ERRS_SUCCESS);
        }
        else
        {
            // buffer is in the firmware
            return (ZOE_ERRS_BUSY);
        }
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



zoe_bool_t c_zoe_module_core_source_new_buffer(c_zoe_module_core_source *This,
						                       ZV_CODEC_OPEN_TYPE data_type
                                               )
{
    int32_t port = s_channel_2_port_map[data_type];

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_base.m_dbg_id,
	               "c_zoe_module_core_source_new_buffer port(%d)\n",
                   port
	               );

    if ((-1 != port) &&
        This->m_p_channels[port]
        )
    {
#ifdef CORE_SRC_NO_THREAD
        zoe_module_core_source_streaming(This, port);
#else //!CORE_SRC_NO_THREAD
        zoe_sosal_event_set(This->m_base.m_evt_extra[port]);
#endif //CORE_SRC_NO_THREAD
        return (ZOE_TRUE);
    }
    else
    {
        return (ZOE_FALSE);
    }
}



int32_t c_zoe_module_core_source_get_port_id(ZV_CODEC_OPEN_TYPE data_type)
{
    return (s_channel_2_port_map[data_type]);
}



zoe_errs_t c_zoe_module_core_source_get_fw_addr(c_zoe_module_core_source *This,
						                        ZV_CODEC_OPEN_TYPE data_type,
                                                ZOE_IPC_CPU *p_cpu_id,
                                                uint32_t *p_module,
                                                uint32_t *p_inst
                                                )
{
    int32_t     port_id = s_channel_2_port_map[data_type];
    c_zoe_port  *port;

    if (-1 != port_id)
    {
        port = &This->m_base.m_ports[port_id];
        *p_cpu_id = port->m_port_conn.addr.cpu;
        *p_module = port->m_port_conn.addr.module;
        *p_inst = port->m_port_conn.addr.inst;
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}









