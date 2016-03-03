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
// zoe_module_core_sink.c
//
// Description: 
//
//  ZOE dummy sink module
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_module_core_sink.h"
#include "c_zoe_module_core_sink.h"
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

#define CORE_SINK_NO_RPC_THREAD


/////////////////////////////////////////////////////////////////////////////
//
//

#define ENTER_CRITICAL_CHANNEL(p_channel_data)  zoe_sosal_mutex_get((p_channel_data)->m_critical_section, -1);
#define LEAVE_CRITICAL_CHANNEL(p_channel_data)  zoe_sosal_mutex_release((p_channel_data)->m_critical_section);


/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_core_sink
//

static zoe_errs_t zoe_module_core_sink_complete_user(c_zoe_module_core_sink *This,
                                                     CHANNEL_DATA *p_channel_data,
                                                     zoe_errs_t status
                                                     )
{
    int i;

    if (p_channel_data->m_user_buffer.p_buf_desc)
    {
        // update the status
	    p_channel_data->m_user_buffer.p_buf_desc->Status = status;

	    // return this buffer
	    p_channel_data->m_p_channel->complete_buffer(p_channel_data->m_p_channel,
					                                 p_channel_data->m_user_buffer.p_buf_desc
					                                 );
        p_channel_data->m_user_buffer.p_buf_desc = ZOE_NULL;
        for (i = 0; i < 3; i++)
        {
            p_channel_data->m_user_buffer.p_buffer[i] = ZOE_NULL;
            p_channel_data->m_user_buffer.length[i] = 0;
        }
    }

    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zoe_module_core_sink_complete_firmware(c_zoe_module_core_sink *This,
                                                         CHANNEL_DATA *p_channel_data
                                                         )
{
    zoe_errs_t              err = ZOE_ERRS_SUCCESS;
    ZOE_BUFFER_DESCRIPTOR   *p_fw_buf_desc;
    int                     i;

    if (p_channel_data->m_fw_buffer.valid)
    {
        p_fw_buf_desc = &p_channel_data->m_fw_buffer.buf_desc;

        if (p_fw_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_SEP_CHROMA)
        {
            err = zoe_module_release_yuv_buffer_clnt(p_fw_buf_desc->owner.selector,
                                                     2,
                                                     p_channel_data->m_fw_buffer.rel_buf_ptr,
                                                     p_channel_data->m_fw_buffer.rel_size,
                                                     p_fw_buf_desc->owner.addr.cpu,
                                                     p_fw_buf_desc->owner.addr.module,
                                                     p_fw_buf_desc->owner.addr.inst
                                                     );
        }
        else
        {
            err = zoe_module_release_buffer_clnt(p_fw_buf_desc->owner.selector,
                                                 ZOE_BUF_DATA,
                                                 p_channel_data->m_fw_buffer.rel_buf_ptr[0],
                                                 p_channel_data->m_fw_buffer.rel_size[0],
                                                 p_fw_buf_desc->owner.addr.cpu,
                                                 p_fw_buf_desc->owner.addr.module,
                                                 p_fw_buf_desc->owner.addr.inst
                                                 );
        }
        if (ZOE_FAIL(err))
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_base.m_dbg_id,
                           "zoe_module_core_sink_complete_firmware() - zoe_module_release_buffer_clnt err(%d)\n",
                           err
	                       );
        }
        p_channel_data->m_fw_buffer.valid = ZOE_FALSE;
        for (i = 0; i < 3; i++)
        {
            p_channel_data->m_fw_buffer.rel_buf_ptr[i] = ZOE_NULL;
            p_channel_data->m_fw_buffer.rel_size[i] = 0;
        }
    }

    return (err);
}


static zoe_errs_t zoe_module_core_sink_streaming(c_zoe_module_core_sink *This, 
                                                 CHANNEL_DATA *p_channel_data,
                                                 c_zoe_port *port
                                                 )
{
    ZOE_BUFFER_DESCRIPTOR   *p_fw_buf_desc = &p_channel_data->m_fw_buffer.buf_desc;
    PZV_BUFFER_DESCRIPTOR   p_user_buf_desc = p_channel_data->m_user_buffer.p_buf_desc;
    uint32_t                copy;
    zoe_errs_t              err = ZOE_ERRS_SUCCESS;
#ifdef USE_MEMRD_4_DMA
    uint32_t                max_dma_size = 0x100000;
#else // !USE_MEMRD_4_DMA
    uint32_t                max_dma_size = ZOEHAL_GetMaxDMASize(This->m_base.m_p_hal);
#endif // USE_MEMRD_4_DMA

    // size of the dma
    copy = ZOE_MIN(p_channel_data->m_user_buffer.length[0] - p_user_buf_desc->ulBufferOffset,
                   ZOE_MIN(p_fw_buf_desc->buffers[ZOE_BUF_DATA].valid_size, max_dma_size)
                   );
    if (copy)
    {
        zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                       This->m_base.m_dbg_id,
                       "core_sink(%d) - ZOEHAL_DMA_READ(%d) [\n",
                       This->m_base.m_inst,
                       copy
	                   );

#ifdef USE_MEMRD_4_DMA
        err = ZOEHAL_MEM_READ_EX(This->m_base.m_p_hal, 
                                 p_fw_buf_desc->buffers[ZOE_BUF_DATA].buf_ptr + p_fw_buf_desc->buffers[ZOE_BUF_DATA].offset, 
                                 (uint8_t *)p_user_buf_desc->DataBuffer[0].Data_mapped + p_user_buf_desc->ulBufferOffset, 
                                 copy, 
                                 ZOE_TRUE
                                 );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_base.m_dbg_id,
	                       "zoe_module_core_sink_streaming ZOEHAL_MEM_READ_EX(%s) failed(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_base.m_object))),
                           err
	                       );
        }

#else //!USE_MEMRD_4_DMA

        // dma the data
        err = ZOEHAL_DMA_READ(This->m_base.m_p_hal, 
                              p_fw_buf_desc->buffers[ZOE_BUF_DATA].buf_ptr + p_fw_buf_desc->buffers[ZOE_BUF_DATA].offset, 
                              //(uint8_t *)(p_channel_data->m_user_buffer.p_buffer[0] + p_user_buf_desc->ulBufferOffset), 
                              (uint8_t *)p_user_buf_desc->DataBuffer[0].Data + p_user_buf_desc->ulBufferOffset, 
                              copy, 
                              p_user_buf_desc->ulFlags & DMA_BUFFER_MODE_MASK, 
                              ZOE_FALSE, 
                              port->m_evt_dma,
                              p_user_buf_desc->DataBuffer[0].p_user_mappings
                              );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_base.m_dbg_id,
	                       "zoe_module_core_sink_streaming ZOEHAL_DMA_READ(%s) failed(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_base.m_object))),
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
                                   This->m_base.m_dbg_id,
	                               "zoe_sosal_event_wait(%s on m_evt_dma) failed(%d)!\n",
                                   zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_base.m_object))),
                                   err
	                               );
                }
            }
        }
#endif //USE_MEMRD_4_DMA
        zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                       This->m_base.m_dbg_id,
                       "core_sink(%d) - ZOEHAL_DMA_READ ]\n",
                       This->m_base.m_inst
                       );

    }

    if (ZOE_SUCCESS(err))
    {
        // update buffer offset and valid size
        p_fw_buf_desc->buffers[ZOE_BUF_DATA].offset += copy;
        p_fw_buf_desc->buffers[ZOE_BUF_DATA].valid_size -= copy;
        p_user_buf_desc->ulBufferOffset += copy;
        p_user_buf_desc->DataBuffer[0].DataUsed += copy;

        // release user buffer if all the buffer has been written or partial fill ok or eos
        if ((p_user_buf_desc->ulFlags & ZV_BUFDESC_FLAG_PARTIAL_FILL_OK) ||
            (p_user_buf_desc->ulBufferOffset == p_channel_data->m_user_buffer.length[0]) ||
            (p_fw_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_EOS)
            )
        {
            if (p_fw_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_PTS)
            {
                p_user_buf_desc->ulPTS = p_fw_buf_desc->info.pts;
                p_user_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_PTS;
            }
            if (p_fw_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_EOS)
            {
                p_user_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_EOS;
            }

		    // return this buffer
            zoe_module_core_sink_complete_user(This, 
                                               p_channel_data, 
                                               err
                                               );
        }

        // release firmware buffer if all data were copied
        if (0 == p_fw_buf_desc->buffers[ZOE_BUF_DATA].valid_size)
        {
            err = zoe_module_core_sink_complete_firmware(This, 
                                                         p_channel_data
                                                         );
        }
    }
    return (err);
}



static zoe_errs_t zoe_module_core_sink_yuv_streaming(c_zoe_module_core_sink *This, 
                                                     CHANNEL_DATA *p_channel_data,
                                                     c_zoe_port *port
                                                     )
{
    ZOE_BUFFER_DESCRIPTOR   *p_fw_buf_desc = &p_channel_data->m_fw_buffer.buf_desc;
    PZV_BUFFER_DESCRIPTOR   p_user_buf_desc = p_channel_data->m_user_buffer.p_buf_desc;
    uint32_t                remain, copy;
    zoe_errs_t              err = ZOE_ERRS_SUCCESS;
#ifdef USE_MEMRD_4_DMA
    uint32_t                max_dma_size = 0x100000;
#else // !USE_MEMRD_4_DMA
    uint32_t                max_dma_size = ZOEHAL_GetMaxDMASize(This->m_base.m_p_hal);
#endif //USE_MEMRD_4_DMA
    uint32_t            i;

    for (i = 0; i < 2; i++)
    {
        p_user_buf_desc->ulBufferIndex = i;
        p_user_buf_desc->ulBufferOffset = 0;
        remain = ZOE_MIN(p_channel_data->m_user_buffer.length[i], p_fw_buf_desc->buffers[i].valid_size);
        zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                       This->m_base.m_dbg_id,
                       "core_sink_yuv(%d) - user(%d) fw(%d)\n",
                       This->m_base.m_inst,
                       p_channel_data->m_user_buffer.length[i],
                       p_fw_buf_desc->buffers[i].valid_size
	                   );

        while (remain)
        {
            // size of the dma
            copy = ZOE_MIN(remain, max_dma_size);

            zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                           This->m_base.m_dbg_id,
                           "core_sink(%d) - ZOEHAL_DMA_READ(%d) [\n",
                           This->m_base.m_inst,
                           copy
	                       );

#ifdef USE_MEMRD_4_DMA
            err = ZOEHAL_MEM_READ_EX(This->m_base.m_p_hal, 
                                     p_fw_buf_desc->buffers[i].buf_ptr + p_fw_buf_desc->buffers[i].offset,
                                     (uint8_t *)p_user_buf_desc->DataBuffer[i].Data_mapped + p_user_buf_desc->ulBufferOffset,
                                     copy, 
                                     ZOE_TRUE
                                     );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_base.m_dbg_id,
	                           "zoe_module_core_sink_yuv_streaming ZOEHAL_MEM_READ_EX(%s) failed(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_base.m_object))),
                               err
	                           );
                break;
            }
#else // !USE_MEMRD_4_DMA

            // dma the data
            err = ZOEHAL_DMA_READ(This->m_base.m_p_hal, 
                                  p_fw_buf_desc->buffers[i].buf_ptr + p_fw_buf_desc->buffers[i].offset, 
                                  //(uint8_t *)(p_channel_data->m_user_buffer.p_buffer[i] + p_user_buf_desc->ulBufferOffset), 
                                  (uint8_t *)p_user_buf_desc->DataBuffer[i].Data + p_user_buf_desc->ulBufferOffset,
                                  copy, 
                                  p_user_buf_desc->ulFlags & DMA_BUFFER_MODE_MASK, 
                                  ZOE_FALSE, 
                                  port->m_evt_dma,
                                  p_user_buf_desc->DataBuffer[i].p_user_mappings
                                  );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_base.m_dbg_id,
	                           "zoe_module_core_sink_yuv_streaming ZOEHAL_DMA_READ(%s) failed(%d)\n",
                               zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_base.m_object))),
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
                                       This->m_base.m_dbg_id,
	                                   "zoe_sosal_event_wait(%s on m_evt_dma) failed(%d)!\n",
                                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(&This->m_base.m_object))),
                                       err
	                                   );
                        break;
                    }
                }
            }
#endif // USE_MEMRD_4_DMA
            zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                           This->m_base.m_dbg_id,
                           "core_sink(%d) - ZOEHAL_DMA_READ ]\n",
                           This->m_base.m_inst
	                       );

            // update buffer offset and valid size
            p_fw_buf_desc->buffers[i].offset += copy;
            p_user_buf_desc->ulBufferOffset += copy;
            p_user_buf_desc->DataBuffer[i].DataUsed += copy;
            remain -= copy;
        }

        if (ZOE_FAIL(err))
        {
            break;
        }
    }

    p_user_buf_desc->Status = err;

    // transfer buffer flags
    if (p_fw_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_PTS)
    {
        p_user_buf_desc->ulPTS = p_fw_buf_desc->info.pts;
        p_user_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_PTS;
    }
    if (p_fw_buf_desc->info.flags & ZOE_BUF_DESC_FLAGS_EOS)
    {
        p_user_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_EOS;
    }
	// return this buffer
    zoe_module_core_sink_complete_user(This, 
                                       p_channel_data, 
                                       err
                                       );

    // release firmware buffer if all data were copied
    err = zoe_module_core_sink_complete_firmware(This, 
                                                 p_channel_data
                                                 );
    return (ZOE_SUCCESS(err) ? p_user_buf_desc->Status : err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_base
//

// data process routine
static zoe_errs_t zoe_module_core_sink_processes(c_zoe_module_base *This_p, 
                                                 uint32_t port_index
                                                 )
{
    c_zoe_module_core_sink  *This = (c_zoe_module_core_sink *)This_p->m_p_module;
    c_zoe_port              *port;
    c_fifo                  *p_fifo;
    CHANNEL_DATA            *p_channel_data;
    c_channel               *p_channel;
    zoe_errs_t              err = ZOE_ERRS_SUCCESS;
    zoe_errs_t              last_err = ZOE_ERRS_SUCCESS;
    uint32_t                i;

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                   This->m_base.m_dbg_id,
	               "zoe_module_core_sink_processes port(%d)\n",
                   port_index
	               );

    // get port data
    port = &This->m_base.m_ports[port_index];
    // get the data descriptor fifo
    p_fifo = port->m_p_input_bufdesc_fifo;

    // get channel data
    p_channel_data = &This->m_channels[port_index];
    p_channel = p_channel_data->m_p_channel;

    while (ZOE_STATE_PLAYING == port->m_state)
    {
        ENTER_CRITICAL_CHANNEL(p_channel_data)

        // check firmware buffer
        if (!p_channel_data->m_fw_buffer.valid)
        {
            if (c_fifo_get_fifo(p_fifo, 
                               &p_channel_data->m_fw_buffer.buf_desc
                               ))
            {
                for (i = 0; i < 2; i++)
                {
                    p_channel_data->m_fw_buffer.rel_buf_ptr[i] = p_channel_data->m_fw_buffer.buf_desc.buffers[i].buf_ptr + p_channel_data->m_fw_buffer.buf_desc.buffers[i].offset;
                    p_channel_data->m_fw_buffer.rel_size[i] = p_channel_data->m_fw_buffer.buf_desc.buffers[i].valid_size;
                }
                p_channel_data->m_fw_buffer.valid = ZOE_TRUE;
            }
            else
            {
                LEAVE_CRITICAL_CHANNEL(p_channel_data)
                break;
            }
        }

        // check user buffer
        if (!p_channel_data->m_user_buffer.p_buf_desc)
        {
            if (c_channel_is_yuv(p_channel))
            {
                if (!p_channel->get_buffer_yuv(p_channel,
                                               &p_channel_data->m_user_buffer.p_buf_desc,
                                               &p_channel_data->m_user_buffer.p_buffer[0],
                                               &p_channel_data->m_user_buffer.length[0],
                                               &p_channel_data->m_user_buffer.p_buffer[1],
                                               &p_channel_data->m_user_buffer.length[1]
                                               ))
                {
                    LEAVE_CRITICAL_CHANNEL(p_channel_data)
                    break;
                }
            }
            else
            {
                if (!p_channel->get_buffer(p_channel,
                                           &p_channel_data->m_user_buffer.p_buf_desc,
                                           &p_channel_data->m_user_buffer.p_buffer[0],
                                           &p_channel_data->m_user_buffer.length[0]
                                           ))
                {
                    LEAVE_CRITICAL_CHANNEL(p_channel_data)
                    break;
                }
            }
        }

        // start streaming
        if (c_channel_is_yuv(p_channel))
        {
            err = zoe_module_core_sink_yuv_streaming(This, 
                                                     p_channel_data,
                                                     &This->m_base.m_ports[port_index]
                                                     );
        }
        else
        {
            err = zoe_module_core_sink_streaming(This, 
                                                 p_channel_data,
                                                 &This->m_base.m_ports[port_index]
                                                 );
        }
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_base.m_dbg_id,
	                       "zoe_module_core_sink_processes zoe_module_core_sink_streaming() err(%d)\n",
                           err
	                       );
            last_err = err;
        }

        LEAVE_CRITICAL_CHANNEL(p_channel_data)
    }

    return (last_err);
}



// module control interface API
//
static zoe_errs_t zoe_module_core_sink_flush(c_zoe_module_base *This_p,
                                             uint32_t sel
                                             )
{
    c_zoe_module_core_sink  *This = (c_zoe_module_core_sink *)This_p->m_p_module;
    uint32_t                port_index;
    CHANNEL_DATA            *p_channel_data;
    zoe_errs_t              err;
    uint32_t                i;

    if ((sel >= ZOE_MODULE_DATA_SEL_PORT_START) &&
        (sel < (ZOE_MODULE_DATA_SEL_PORT_START + This->m_base.m_num_inputs + This->m_base.m_num_outputs))
        )
    {
        port_index = sel - ZOE_MODULE_DATA_SEL_PORT_START;
        p_channel_data = &This->m_channels[port_index];

        ENTER_CRITICAL_CHANNEL(p_channel_data)

        // release firmware buffer
        err = zoe_module_core_sink_complete_firmware(This, 
                                                     p_channel_data
                                                     );
        // call base flush function
        err = zoe_module_base_flush(This_p, 
                                    sel
                                    );
        // release user buffer
        err = zoe_module_core_sink_complete_user(This, 
                                                 p_channel_data, 
                                                 ZOE_ERRS_SUCCESS
                                                 );
        LEAVE_CRITICAL_CHANNEL(p_channel_data)
    }
    else
    {
        for (i = 0; i < ZOE_MODULE_CORE_SINK_NUM_INPUT; i++)
        {
            p_channel_data = &This->m_channels[i];
            ENTER_CRITICAL_CHANNEL(p_channel_data)
            // release firmware buffer
            err = zoe_module_core_sink_complete_firmware(This, 
                                                         p_channel_data
                                                         );
            // call base port flush function
            err = zoe_module_base_flush(This_p, 
                                        i + ZOE_MODULE_DATA_SEL_PORT_START
                                        );
            // release user buffer
            err = zoe_module_core_sink_complete_user(This, 
                                                     p_channel_data, 
                                                     ZOE_ERRS_SUCCESS
                                                     );
            LEAVE_CRITICAL_CHANNEL(p_channel_data)
        }

        // call base flush function
        err = zoe_module_base_flush(This_p, 
                                    sel
                                    );
    }
    return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_core_sink
//

static void c_zoe_module_core_sink_destructor(c_zoe_module_core_sink *This)
{
    uint32_t    i;

    // c_zoe_module_base
    c_zoe_module_base_destructor(&This->m_base);

    // delete channel data critical section
    for (i = 0; i < ZOE_MODULE_CORE_SINK_NUM_INPUT; i++)
    {
        if (This->m_channels[i].m_critical_section)
        {
            zoe_sosal_mutex_delete(This->m_channels[i].m_critical_section);
            This->m_channels[i].m_critical_section = ZOE_NULL;
        }
    }
}



static c_zoe_module_core_sink * c_zoe_module_core_sink_constructor(c_zoe_module_core_sink *p_zoe_module_core_sink,
											                       c_object *p_parent, 
							                                       uint32_t attributes,
                                                                   IZOEHALAPI *p_hal,
                                                                   zoe_dbg_comp_id_t dbg_id,
                                                                   uint32_t inst
							                                       )
{
    if (p_zoe_module_core_sink)
    {
        c_zoe_module_base           *p_base;
        zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
        uint32_t                cpu = (uint32_t)c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
        uint32_t                max_priority = zoe_sosal_thread_maxpriorities_get();
        uint32_t                i;

        // c_zoe_module_base
        //
        p_base = c_zoe_module_base_constructor(&p_zoe_module_core_sink->m_base,
							                   p_parent, 
                                               OBJECT_ZOE_MODULE_CORE_SINK,
							                   attributes,
                                               p_hal,
                                               dbg_id,
                                               cpu,
                                               inst,
                                               ZOE_MODULE_CORE_SINK_NUM_INPUT,
                                               ZOE_MODULE_CORE_SINK_NUM_OUTPUT,
                                               ZOE_MODULE_CORE_SINK_NUM_EXTRA,
                                               ZOE_MODULE_CORE_SINK_TIMEOUT_US,
                                               max_priority - 1,
                                               4096,
                                               (void *)p_zoe_module_core_sink,
                                               ZOE_MODULE_NAME_CORE_SINK,
                                               ZOE_TRUE,
#ifdef CORE_SINK_NO_RPC_THREAD
                                               ZOE_FALSE
#else //!CORE_SINK_NO_RPC_THREAD
                                               ZOE_TRUE
#endif //CORE_SINK_NO_RPC_THREAD
							                   );
		if (!p_base)
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_core_sink_constructor c_zoe_module_base_constructor() FAILED\n"
		                   );
            c_zoe_module_core_sink_destructor(p_zoe_module_core_sink);
            return (ZOE_NULL);
		}

        // c_zoe_module_core_sink
        //

        // clear channel data
        memset(&p_zoe_module_core_sink->m_channels[0], 
               0, 
               sizeof(CHANNEL_DATA) * ZOE_MODULE_CORE_SINK_NUM_INPUT
               );

        // create channel data critical section
        for (i = 0; i < ZOE_MODULE_CORE_SINK_NUM_INPUT; i++)
        {
            zoe_sosal_mutex_create(ZOE_NULL,
                                   &p_zoe_module_core_sink->m_channels[i].m_critical_section
                                   );
        }

        for (i = 0; i < ZOE_MODULE_CORE_SINK_NUM_INPUT; i++)
        {
            // setup input port memory usage and data fifo size
            p_zoe_module_core_sink->m_base.m_ports[i].m_input_port_mem_usage = ZOE_MODULE_MEM_USAGE_IN_PLACE;

            // setup port type and sub_type
            p_zoe_module_core_sink->m_base.m_ports[i].m_port_type = ZOE_MODULE_DATA_TYPE_NONE;
            p_zoe_module_core_sink->m_base.m_ports[i].m_port_sub_type = ZOE_MODULE_DATA_SUB_TYPE_NONE;

            // we need dma
            p_zoe_module_core_sink->m_base.m_ports[i].m_need_dma = ZOE_TRUE;
        }

        // override c_zoe_module_base processes()
        p_zoe_module_core_sink->m_base.processes = zoe_module_core_sink_processes;

        // override module control interface flush()
        p_zoe_module_core_sink->m_base.flush = zoe_module_core_sink_flush;

        // create sink main thread
		if (!c_thread_thread_init(&p_zoe_module_core_sink->m_base.m_thread))
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_core_sink_constructor c_thread_thread_init() FAILED\n"
                           );
            c_zoe_module_core_sink_destructor(p_zoe_module_core_sink);
            return (ZOE_NULL);
		}
    }

    return (p_zoe_module_core_sink);
}



/////////////////////////////////////////////////////////////////////////////
//
//

zoe_void_ptr_t zoe_module_core_sink_create(IZOEHALAPI *p_hal,
                                           zoe_dbg_comp_id_t dbg_id,
                                           uint32_t inst
                                           )
{
    c_zoe_module_core_sink  *p_sink, *p_ret;
    c_zoe_module_mgr        *p_mgr = c_zoe_module_mgr_get_module_mgr();

    // create c_zoe_module_core_sink
    //
    p_sink = (c_zoe_module_core_sink *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                              sizeof(c_zoe_module_core_sink), 
                                                              0
                                                              );
    if (!p_sink)
    {
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       dbg_id,
		               "zoe_module_core_sink_create() zoe_sosal_memory_alloc() failed!\n"
		               );
        // fail to allocate memory
        //
        return (ZOE_NULL);
    }

    p_ret = c_zoe_module_core_sink_constructor(p_sink,
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
                       "zoe_module_core_sink_create() c_zoe_module_core_sink_constructor failed!!!\n" 
                       );
        zoe_sosal_memory_free((void *)p_sink);
    }

    return ((zoe_void_ptr_t)p_ret);
}



zoe_errs_t zoe_module_core_sink_destroy(zoe_void_ptr_t p_inst_data)
{
    c_zoe_module_core_sink  *p_sink = (c_zoe_module_core_sink *)p_inst_data;

    if (p_sink)
    {
        c_zoe_module_core_sink_destructor(p_sink);
        zoe_sosal_memory_free((void *)p_sink);
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

static const int32_t s_port_2_channel_map[ZOE_MODULE_CORE_SINK_NUM_INPUT] = 
{
	ZV_CODEC_VID_OUT,   // 0
	ZV_CODEC_META_OUT,  // 1
	ZV_CODEC_YUV_OUT    // 2
};


static const int32_t s_channel_2_port_map[ZV_CODEC_OPEN_END] = 
{
	0,  // ZV_CODEC_VID_OUT
	1,  // ZV_CODEC_META_OUT
	2,  // ZV_CODEC_YUV_OUT
	-1, // ZV_CODEC_VID_IN
	-1, // ZV_CODEC_META_IN
	-1, // ZV_CODEC_YUV_IN
	-1  // ZV_CODEC_VIRTUAL
};



// interfaces for local cpu
//
zoe_errs_t c_zoe_module_core_sink_open(c_zoe_module_core_sink *This,
							           ZV_CODEC_OPEN_TYPE data_type,
							           c_channel *p_channel
							           )
{
    int32_t port = s_channel_2_port_map[data_type];

    if ((-1 != port) &&
        p_channel
        )
    {
        ENTER_CRITICAL_CHANNEL(&This->m_channels[port])
        This->m_channels[port].m_p_channel = p_channel;
        LEAVE_CRITICAL_CHANNEL(&This->m_channels[port])
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



zoe_errs_t c_zoe_module_core_sink_close(c_zoe_module_core_sink *This,
							            ZV_CODEC_OPEN_TYPE data_type
							            )
{
    int32_t port = s_channel_2_port_map[data_type];

    if (-1 != port)
    {
        ENTER_CRITICAL_CHANNEL(&This->m_channels[port])
        This->m_channels[port].m_p_channel = ZOE_NULL;
        LEAVE_CRITICAL_CHANNEL(&This->m_channels[port])
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



zoe_errs_t c_zoe_module_core_sink_cancel_buffer(c_zoe_module_core_sink *This,
						                        ZV_CODEC_OPEN_TYPE data_type,
                                                PZV_BUFFER_DESCRIPTOR pBufDesc
                                                )
{
    int32_t         port = s_channel_2_port_map[data_type];
    CHANNEL_DATA    *p_channel_data;
    zoe_errs_t      err = ZOE_ERRS_NOTFOUND;

    if (-1 != port)
    {
        p_channel_data = &This->m_channels[port];

        ENTER_CRITICAL_CHANNEL(p_channel_data)
        if (p_channel_data->m_user_buffer.p_buf_desc)
        {
            err = zoe_module_core_sink_complete_user(This, 
                                                     p_channel_data, 
                                                     ZOE_ERRS_CANCELLED
                                                     );
        }
        LEAVE_CRITICAL_CHANNEL(p_channel_data)
        return (err);
    }
    else
    {
        return (ZOE_ERRS_PARMS);
    }
}



zoe_bool_t c_zoe_module_core_sink_new_buffer(c_zoe_module_core_sink *This,
						                     ZV_CODEC_OPEN_TYPE data_type
                                             )
{
    int32_t port = s_channel_2_port_map[data_type];

    if ((-1 != port) &&
        This->m_channels[port].m_p_channel
        )
    {
        zoe_sosal_event_set(This->m_base.m_ports[port].m_evt_port);
        return (ZOE_TRUE);
    }
    else
    {
        return (ZOE_FALSE);
    }
}



int32_t c_zoe_module_core_sink_get_port_id(ZV_CODEC_OPEN_TYPE data_type)
{
    return (s_channel_2_port_map[data_type]);
}











