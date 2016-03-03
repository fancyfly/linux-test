/*
 * Copyright 2012-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


///////////////////////////////////////////////////////////////////////////////
//
// cport_vb2.c
//
// Description: 
//
//  base port implementation using videobuf 2
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-dma-sg.h>

#include "zoe_dbg.h"
#include "cport_vb2.h"
#include "ccomponent.h"
#include "cdevice.h"
#include "objids.h"
#include "zoe_linuxker_errmap.h"


#define _CONTIG_MEM_ONLY

/////////////////////////////////////////////////////////////////////////////
//
//

extern zoe_dbg_comp_id_t    g_ZVV4LDevDBGCompID;


/////////////////////////////////////////////////////////////////////////////
//
//

static int64_t timeval_2_pts(struct timeval timestamp)
{
    ktime_t kt = timeval_to_ktime(timestamp);
    int64_t pts;

    pts = kt.tv64 * 9;
    do_div(pts, 100000);
    return (pts);
}



static void pts_2_timeval(int64_t pts, struct timeval *timestamp)
{
    struct timeval  tv;
    ktime_t         kt;

    kt.tv64 = pts * 100000;
    do_div(kt.tv64, 9);

    tv = ktime_to_timeval(kt);
    timestamp->tv_sec = tv.tv_sec;
    timestamp->tv_usec = tv.tv_usec;
}


/////////////////////////////////////////////////////////////////////////////
//
//

static void c_port_flush_drv_q(c_port *This)
{
    PORT_VB2_DATA_REQ   *p_data_req = ZOE_NULL;
    PORT_VB2_DATA_REQ   *p_temp;

    // flush the driver queue
    down(&This->m_drv_q_lock);
    if (!list_empty(&This->m_drv_q))
    {
        list_for_each_entry_safe(p_data_req, p_temp, &This->m_drv_q, list) 
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
					       "%s(%d) - list_del(%p)\n",
					       __FUNCTION__,
					       This->m_id,
                           p_data_req
					       );
            list_del(&p_data_req->list);
            vb2_buffer_done(p_data_req->p_vb2_buf, VB2_BUF_STATE_ERROR);
        }
    }
    INIT_LIST_HEAD(&This->m_drv_q);
    up(&This->m_drv_q_lock);
}



static PORT_VB2_DATA_REQ *c_port_find_drv_q_entry_by_buf_desc(c_port *This, 
                                                              PZV_BUFFER_DESCRIPTOR p_buf_desc
                                                              )
{
    PORT_VB2_DATA_REQ   *p_data_req;
    PORT_VB2_DATA_REQ   *p_temp;

    down(&This->m_drv_q_lock);
    if (!list_empty(&This->m_drv_q))
    {
        list_for_each_entry_safe(p_data_req, p_temp, &This->m_drv_q, list) 
        {
	        if (&p_data_req->buf_desc == p_buf_desc)
            {
                if (COMPONENT_PORT_YUV_OUT == This->m_id)
                {
		        zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               g_ZVV4LDevDBGCompID,
					           "%s(%d) : MPGCODEC_CMD_DONE_DATA id(%d) list_del(0x%p) status(%d)\n",
					           __FUNCTION__,
					           This->m_hStreamLib,
					           p_data_req->n_id,
					           p_data_req, 
					           p_buf_desc->Status
					           );
                }
                list_del(&p_data_req->list);
                up(&This->m_drv_q_lock);
                return (p_data_req);
            }
        }
    }
    up(&This->m_drv_q_lock);
    return (ZOE_NULL);
}



static void c_port_time_stamp(c_port *This,
							  PZV_BUFFER_DESCRIPTOR p_buf_desc,
							  struct v4l2_buffer *p_v4l2_buf
							  )
{
	if (PORT_DIR_READ == This->m_dir)
	{
		if (p_buf_desc->ulFlags & ZV_BUFDESC_FLAG_PTS)
		{
			// construct time stamp
			//
            pts_2_timeval(p_buf_desc->ulPTS, 
                          &p_v4l2_buf->timestamp
                          );
			p_v4l2_buf->flags |= V4L2_BUF_FLAG_TIMESTAMP_COPY;
		}
		else
		{
			p_v4l2_buf->flags &= ~V4L2_BUF_FLAG_TIMESTAMP_COPY;
		}
	}

    if (This->m_discontinuity)
	{
		This->m_discontinuity = ZOE_FALSE;
	}
}



static void c_port_fill_frame_info(c_port *This,
								   struct vb2_buffer *p_vb2_buf,
								   uint32_t *DataUsed,
								   uint32_t ulFlags
								   ) 
{
	struct v4l2_buffer  *p_v4l2_buf = &p_vb2_buf->v4l2_buf;
    uint32_t            i;

    if ((V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == p_v4l2_buf->type) ||
        (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == p_v4l2_buf->type)
        )
    {
        for (i = 0; i < p_v4l2_buf->length; i++)
        {
            vb2_set_plane_payload(p_vb2_buf, i, DataUsed[i]);
        }
    }
    else
    {
        vb2_set_plane_payload(p_vb2_buf, 0, *DataUsed);
    }

	This->m_picture_num++;

    if (ulFlags & ZV_BUFDESC_FLAG_EOS)
    {
        const struct v4l2_event ev = 
        {
            .type = V4L2_EVENT_EOS
        };
        This->m_EOS = ZOE_TRUE;
        v4l2_event_queue_fh(&This->m_pComponent->m_fh, &ev);
    }
}



zoe_errs_t c_port_build_buf_desc(c_port *This,
                                 PORT_VB2_DATA_REQ *p_data_req
								 )
{
	PZV_BUFFER_DESCRIPTOR   p_buf_desc = &p_data_req->buf_desc;
    struct vb2_buffer       *p_vb2_buf = p_data_req->p_vb2_buf;
	struct v4l2_buffer      *p_v4l2_buf = &p_vb2_buf->v4l2_buf;
    uint32_t                total_used = 0;
    uint32_t                buf_idx;
    zoe_uintptr_t           alignment_mask;

	if (p_v4l2_buf->flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) 
	{
		p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_PTS;
		p_buf_desc->ulPTS = timeval_2_pts(p_v4l2_buf->timestamp);
	} 
	else 
	{
		p_buf_desc->ulFlags = 0;
		p_buf_desc->ulPTS = 0;
	}

    if ((COMPONENT_PORT_YUV_IN == This->m_id) ||
        (COMPONENT_PORT_YUV_OUT == This->m_id)
        )
    {
        //alignment_mask = ~((zoe_uintptr_t)This->m_pic_format.alignment - 1);
        alignment_mask = ~((zoe_uintptr_t)0);
    }
    else
    {
        alignment_mask = ~((zoe_uintptr_t)0);
    }
    
    if ((V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == p_v4l2_buf->type) ||
        (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == p_v4l2_buf->type)
        )
    {
	    p_buf_desc->NumberOfBuffers = p_v4l2_buf->length;
	    p_buf_desc->ulBufferSize = 0;

		zoe_dbg_printf(ZOE_DBG_LVL_XTRACE, 
                       g_ZVV4LDevDBGCompID,
		               "%s() num planes(%d)\n", 
					   __FUNCTION__,
                       p_v4l2_buf->length
					   );

        for (buf_idx = 0; buf_idx < p_v4l2_buf->length; buf_idx++)
        {
	        p_buf_desc->DataBuffer[buf_idx].Duration = 0;
	        p_buf_desc->DataBuffer[buf_idx].BufferSize = vb2_plane_size(p_vb2_buf, buf_idx);
	        p_buf_desc->DataBuffer[buf_idx].DataOffset = p_vb2_buf->v4l2_planes[buf_idx].data_offset;
	        p_buf_desc->ulBufferSize += vb2_plane_size(p_vb2_buf, buf_idx);
	        p_buf_desc->DataBuffer[buf_idx].DataUsed = vb2_get_plane_payload(p_vb2_buf, buf_idx);
            total_used += p_buf_desc->DataBuffer[buf_idx].DataUsed;
#ifndef _CONTIG_MEM_ONLY
            if (V4L2_MEMORY_USERPTR == p_vb2_buf->v4l2_buf.memory)
            {
                // dma-sg
                PZOE_USER_PAGES p_user_pages = &p_data_req->user_pages[buf_idx]; 
                uint64_t        data;

                p_buf_desc->DataBuffer[buf_idx].p_user_mappings = (zoe_void_ptr_t)p_user_pages;
		        p_buf_desc->DataBuffer[buf_idx].Data = (zoe_void_ptr_t)p_vb2_buf->v4l2_planes[buf_idx].m.userptr;

                // setup user page
                memset(p_user_pages, 0, sizeof(ZOE_USER_PAGES));
                p_user_pages->size = p_buf_desc->DataBuffer[buf_idx].BufferSize;
                if (p_user_pages->size)
                {
                    data = (uint64_t)p_buf_desc->DataBuffer[buf_idx].Data;
   	                p_user_pages->first = (data & PAGE_MASK) >> PAGE_SHIFT;
	                p_user_pages->last  = ((data + p_user_pages->size - 1) & PAGE_MASK) >> PAGE_SHIFT;
	                p_user_pages->nr_pages = (unsigned int)(p_user_pages->last - p_user_pages->first + 1);
                    p_user_pages->sg = vb2_dma_sg_plane_desc(p_vb2_buf, buf_idx);
                }
            }
            else
            {
                // dma-contig
		        p_buf_desc->DataBuffer[buf_idx].Data = (zoe_void_ptr_t)vb2_dma_contig_plane_dma_addr(p_vb2_buf, buf_idx);
            }
		    p_buf_desc->DataBuffer[buf_idx].Data_mapped = vb2_plane_vaddr(p_vb2_buf, buf_idx);
#else // _CONTIG_MEM_ONLY
		    p_buf_desc->DataBuffer[buf_idx].Data = (zoe_void_ptr_t)vb2_dma_contig_plane_dma_addr(p_vb2_buf, buf_idx);
		    p_buf_desc->DataBuffer[buf_idx].Data_mapped = vb2_plane_vaddr(p_vb2_buf, buf_idx);
#endif // !_CONTIG_MEM_ONLY

		    p_buf_desc->DataBuffer[buf_idx].Data = (zoe_void_ptr_t)((zoe_uintptr_t)p_buf_desc->DataBuffer[buf_idx].Data & alignment_mask);
		    p_buf_desc->DataBuffer[buf_idx].Data_mapped = (zoe_void_ptr_t)((zoe_uintptr_t)p_buf_desc->DataBuffer[buf_idx].Data_mapped & alignment_mask);

		    zoe_dbg_printf(ZOE_DBG_LVL_XTRACE, 
                           g_ZVV4LDevDBGCompID,
		                   "(%p:%p:%d:%d) ",
                           p_buf_desc->DataBuffer[buf_idx].Data,
                           p_buf_desc->DataBuffer[buf_idx].Data_mapped,
                           p_buf_desc->DataBuffer[buf_idx].BufferSize,
                           p_buf_desc->DataBuffer[buf_idx].DataOffset
					       );
        }

		zoe_dbg_printf(ZOE_DBG_LVL_XTRACE, 
                       g_ZVV4LDevDBGCompID,
		               "\n"
				       );

    }
    else
    {
	    p_buf_desc->NumberOfBuffers = 1;
	    p_buf_desc->ulBufferSize = p_v4l2_buf->length;
	    p_buf_desc->DataBuffer[0].Duration = 0;
	    p_buf_desc->DataBuffer[0].BufferSize = vb2_plane_size(p_vb2_buf, 0);
	    p_buf_desc->DataBuffer[0].DataOffset = 0;
	    p_buf_desc->DataBuffer[0].DataUsed = vb2_get_plane_payload(p_vb2_buf, 0);
        total_used += p_buf_desc->DataBuffer[0].DataUsed;
#ifndef _CONTIG_MEM_ONLY
        if (V4L2_MEMORY_USERPTR == p_vb2_buf->v4l2_buf.memory)
        {
            // dma-sg
            PZOE_USER_PAGES p_user_pages = &p_data_req->user_pages[0]; 
            uint64_t        data;

            p_buf_desc->DataBuffer[0].p_user_mappings = (zoe_void_ptr_t)p_user_pages;
		    p_buf_desc->DataBuffer[0].Data = (zoe_void_ptr_t)p_vb2_buf->v4l2_planes[0].m.userptr;

            // setup user page
            memset(p_user_pages, 0, sizeof(ZOE_USER_PAGES));
            p_user_pages->size = p_buf_desc->DataBuffer[0].BufferSize;
            if (p_user_pages->size)
            {
                data = (uint64_t)p_buf_desc->DataBuffer[0].Data;
   	            p_user_pages->first = (data & PAGE_MASK) >> PAGE_SHIFT;
	            p_user_pages->last  = ((data + p_user_pages->size - 1) & PAGE_MASK) >> PAGE_SHIFT;
	            p_user_pages->nr_pages = (unsigned int)(p_user_pages->last - p_user_pages->first + 1);
                p_user_pages->sg = vb2_dma_sg_plane_desc(p_vb2_buf, 0);
            }
        }
        else
        {
            // dma-contig
		    p_buf_desc->DataBuffer[0].Data = (zoe_void_ptr_t)vb2_dma_contig_plane_dma_addr(p_vb2_buf, 0);

        }
		p_buf_desc->DataBuffer[0].Data_mapped = vb2_plane_vaddr(p_vb2_buf, 0);
#else // _CONTIG_MEM_ONLY
        p_buf_desc->DataBuffer[0].Data = (zoe_void_ptr_t)vb2_dma_contig_plane_dma_addr(p_vb2_buf, 0);
		p_buf_desc->DataBuffer[0].Data_mapped = vb2_plane_vaddr(p_vb2_buf, 0);
#endif // !_CONTIG_MEM_ONLY

		p_buf_desc->DataBuffer[0].Data = (zoe_void_ptr_t)((zoe_uintptr_t)p_buf_desc->DataBuffer[0].Data & alignment_mask);
		p_buf_desc->DataBuffer[0].Data_mapped = (zoe_void_ptr_t)((zoe_uintptr_t)p_buf_desc->DataBuffer[0].Data_mapped & alignment_mask);
    }

    if (PORT_DIR_WRITE == This->m_dir)
    {
        if (0 == total_used)
        {
		    p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_EOS;
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
		                   "%s() - EOS", 
					       __FUNCTION__
					       );
        }
    }

    p_buf_desc->ulFlags &= ~DMA_BUFFER_MODE_MASK;
#ifndef _CONTIG_MEM_ONLY
	if (V4L2_MEMORY_USERPTR == p_v4l2_buf->memory)
	{
		p_buf_desc->ulFlags |= DMA_BUFFER_MODE_USERPTR;
	}
	else
	{
		p_buf_desc->ulFlags |= DMA_BUFFER_MODE_CONTIG;
	}
#else // _CONTIG_MEM_ONLY
	p_buf_desc->ulFlags |= DMA_BUFFER_MODE_CONTIG;
#endif //!_CONTIG_MEM_ONLY
    p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_KERNEL_MAPPED;

	p_buf_desc->ulBufferIndex = 0;
	p_buf_desc->ulBufferOffset = 0;
	p_buf_desc->ulTotalUsed = 0;
	if (This->m_bBufferPartialFill)
	{
		p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_PARTIAL_FILL_OK;
	}
	if (This->m_bBufferFrameAligned)
	{
		p_buf_desc->ulFlags |= ZV_BUFDESC_FLAG_FRAME_ALIGNED;
	}
	p_buf_desc->Status = ZOE_ERRS_PENDING;

	return (ZOE_ERRS_SUCCESS);
}



static void c_port_clean_up_data_request(c_port *This, 
									     PORT_VB2_DATA_REQ *p_data_req
									     )
{
	PZV_BUFFER_DESCRIPTOR	p_buf_desc = &p_data_req->buf_desc;
    uint32_t                i;

    for (i = 0; i < p_buf_desc->NumberOfBuffers; i++)
    {
        p_buf_desc->DataBuffer[i].Data_mapped = ZOE_NULL;
    }

    // clear the flag
    p_buf_desc->ulFlags &= ~ZV_BUFDESC_FLAG_KERNEL_MAPPED;
}



static zoe_errs_t c_port_submit_buffer(c_port *This,
                                       PORT_VB2_DATA_REQ *p_data_req
                                       )
{
    i_zv_codec  *p_codec = c_device_get_codec(This->m_pComponent->m_pDevice);
    zoe_errs_t  err;

    if (COMPONENT_PORT_YUV_OUT == This->m_id)
    {
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   g_ZVV4LDevDBGCompID,
			       "%s(%d) req(%p)\n",
				   __FUNCTION__,
				   This->m_id,
                   p_data_req
				   );
    }

	// build the buffer descriptor
	//
	err = c_port_build_buf_desc(This,
						        p_data_req
						        );
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) - c_port_build_buf_desc FAILED(err)!!!!\n",
					   __FUNCTION__,
					   This->m_id,
                       err
					   );
	}
    else
    {
	    // add the buffer to the codec library
	    //
	    err = p_codec->add_buffer(p_codec,
								  This->m_hStreamLib,
								  &p_data_req->buf_desc
								  );
	    if (!ZOE_SUCCESS(err))
	    {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
					       "%s(%d) - hStreamLib(%d) add_buffer id(%d) (0x%p) FAILED!!!!\n",
					       __FUNCTION__,
					       This->m_id,
					       This->m_hStreamLib,
					       p_data_req->n_id,
                           &p_data_req->buf_desc
					       );
	    }
    }

	if (!ZOE_SUCCESS(err))
	{
        c_port_clean_up_data_request(This, 
                                     p_data_req
                                     );

        down(&This->m_drv_q_lock);
		zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                       g_ZVV4LDevDBGCompID,
				       "%s(%d) - list_del(%p)\n",
				       __FUNCTION__,
				       This->m_id,
                       p_data_req
				       );
        list_del(&p_data_req->list);
        up(&This->m_drv_q_lock);

        vb2_buffer_done(p_data_req->p_vb2_buf, VB2_BUF_STATE_ERROR);
	}
    return (err);
}



static void c_port_process_completed_buffer(c_port *This,
										    struct v4l2_buffer *p_v4l2_buf
										    )
{
}



static zoe_errs_t c_port_on_buffer_complete(c_port *This,
										    PORT_VB2_DATA_REQ *p_data_req
										    )
{
	PZV_BUFFER_DESCRIPTOR	p_buf_desc = &p_data_req->buf_desc;
    struct vb2_buffer       *p_vb2_buf = p_data_req->p_vb2_buf;
	struct v4l2_buffer		*p_v4l2_buf = &p_vb2_buf->v4l2_buf;
    uint32_t                data_used[ZV_MAX_BUF_PER_DESC] = {0};
    uint32_t                i;


    for (i = 0; i < p_buf_desc->NumberOfBuffers; i++)
    {
        data_used[i] = p_buf_desc->DataBuffer[i].DataUsed;
    }

    if (2 == p_buf_desc->NumberOfBuffers)
    {
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   g_ZVV4LDevDBGCompID,
				   "%s() %d (%d:%d)\n",
				   __FUNCTION__,
                   p_buf_desc->NumberOfBuffers,
                   data_used[0],
                   data_used[1]
				   );
    }

    c_port_time_stamp(This,
					  p_buf_desc,
					  p_v4l2_buf
					  );
	c_port_fill_frame_info(This,
						   p_vb2_buf,
						   data_used, 
						   p_buf_desc->ulFlags
						   );
	c_port_process_completed_buffer(This, 
								    p_v4l2_buf
								    );	
	c_port_clean_up_data_request(This, 
							     p_data_req
							     );
    vb2_buffer_done(p_data_req->p_vb2_buf, 
                    VB2_BUF_STATE_DONE
                    );
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_port_stream_callback(zoe_void_ptr_t Context,
									     uint32_t dwCode,
									     zoe_void_ptr_t pParam
									     )
{
	c_port		*This = (c_port *)Context;
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	switch (dwCode)
	{
		case ZVCODEC_CMD_STOP_COMPLETED:
			break;

		case ZVCODEC_CMD_DONE_DATA:
		{
            PORT_VB2_DATA_REQ   *p_data_req;

            p_data_req = c_port_find_drv_q_entry_by_buf_desc(This, 
                                                             (PZV_BUFFER_DESCRIPTOR)pParam
                                                             );
            if (p_data_req)
            {
				err = c_port_on_buffer_complete(This,
									            p_data_req
									            );
            }
            else
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               g_ZVV4LDevDBGCompID,
						       "%s(%d) ZVCODEC_CMD_DONE_DATA NOT FOUND!!!!\n",
						       __FUNCTION__,
                               This->m_id
						       );

            }
			break;
		}

		case ZVCODEC_CMD_END_OF_STREAM:
			break;

		case ZVCODEC_CMD_START_TIMEOUT:
			break;

        case ZVCODEC_CMD_DEC_VID_ERR:
            break;

        case ZVCODEC_CMD_DEC_SEQ_PARAM_CHANGED:
        {
            uint32_t    *evt_data = (uint32_t *)pParam;
            const struct v4l2_event ev = 
            {
                .type = V4L2_EVENT_SOURCE_CHANGE,
                .u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION
            };

			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s ZVCODEC_CMD_DEC_SEQ_PARAM_CHANGED (0x%x)(0x%x)(0x%x)(0x%x)\n",
						   __FUNCTION__,
                           evt_data[0],
                           evt_data[1],
                           evt_data[2],
                           evt_data[3]
						   );
		    v4l2_event_queue_fh(&This->m_pComponent->m_fh, &ev);
            break;
        }

		case ZVCODEC_CMD_DEC_COMPLETE:
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           g_ZVV4LDevDBGCompID,
						   "%s ZVCODEC_CMD_DEC_COMPLETE\n",
						   __FUNCTION__
						   );
			break;

		default:
			err = ZOE_ERRS_NOTIMPL;
			break;
	}

	return (err);
}



static zoe_errs_t c_port_codec_open(c_port *This,
								    i_zv_codec *pMpegCodec
								    )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	if (!This->m_bDisabled &&
		(ZOE_NULL_HANDLE == This->m_hStreamLib)
		)
	{
		err = pMpegCodec->open(pMpegCodec,
							   This->m_pComponent->m_hTask,
							   This->m_dwOpenType,
							   &This->m_openFormat,
							   &This->m_hStreamLib,
							   c_port_stream_callback,
							   (zoe_void_ptr_t)This
							   );
    
		if (!ZOE_SUCCESS(err))
		{
			This->m_hStreamLib = ZOE_NULL_HANDLE;

			zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                           g_ZVV4LDevDBGCompID,
						   "%s pMpegCodec->open failed(%d)\n", 
						   __FUNCTION__,
						   err
						   );
		}
		else
		{
			zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                           g_ZVV4LDevDBGCompID,
						   "%s pMpegCodec->open task(%d) codec handle(%d)\n", 
						   __FUNCTION__,
						   This->m_pComponent->m_hTask,
						   This->m_hStreamLib
						   );
		}
	}

	return (err);
}



static void c_port_reset_counters(c_port *This)
{
	This->m_picture_num = 0;
	This->m_discontinuity = ZOE_FALSE;
	This->m_EOS = ZOE_FALSE;
}



/////////////////////////////////////////////////////////////////////////////
//
//

static int c_port_queue_setup(struct vb2_queue *vq,
                              const struct v4l2_format *fmt, 
                              unsigned int *buf_count,
                              unsigned int *plane_count, 
                              unsigned int psize[],
                              void *allocators[]
                              )
{
    c_port  *This = (c_port *)vq->drv_priv;

    if ((COMPONENT_PORT_YUV_IN == This->m_id) ||
        (COMPONENT_PORT_YUV_OUT == This->m_id)
        )
    {
        if (This->m_format_valid)
        {
            if (V4L2_TYPE_IS_MULTIPLANAR(This->m_buf_type))
            {
                *plane_count = 2;
                psize[0] = This->m_pic_format.planes[0].sizeimage + This->m_pic_format.alignment - 1;
                psize[1] = This->m_pic_format.planes[1].sizeimage + This->m_pic_format.colocated_size + This->m_pic_format.alignment - 1;
                allocators[0] = This->m_alloc_ctx;
                allocators[1] = This->m_alloc_ctx;
            }
            else
            {
                psize[0] = This->m_pic_format.planes[0].sizeimage + This->m_pic_format.planes[1].sizeimage + This->m_pic_format.alignment - 1;
                *plane_count = 1;
                allocators[0] = This->m_alloc_ctx;
            }

            if (*buf_count < This->m_frame_nbs)
            {
                *buf_count = This->m_frame_nbs;
            }
            if (*buf_count > ZV_AVLIB_MAX_DATA_ENTRIES)
            {
                *buf_count = ZV_AVLIB_MAX_DATA_ENTRIES;
            }
        }
        else
        {
            return (-EINVAL);
        }
    }
    else
    {
        *plane_count = 1;
        if (*buf_count < 1)
        {
            *buf_count = 1;
        }
        if (*buf_count > ZV_AVLIB_MAX_DATA_ENTRIES)
        {
            *buf_count = ZV_AVLIB_MAX_DATA_ENTRIES;
        }

        psize[0] = This->m_frame_size;
        allocators[0] = This->m_alloc_ctx;

		zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) buf(%d) plane(%d) size(%d) set(%d)\n",
					   __FUNCTION__,
					   This->m_id,
                       *buf_count,
                       *plane_count,
                       psize[0],
                       This->m_frame_size
					   );
    }
    return (0);
}



static void c_port_unlock(struct vb2_queue *q)
{
}



static void c_port_lock(struct vb2_queue *q)
{
}



static int c_port_buf_init(struct vb2_buffer *vb)
{
    return (0);
}


static int c_port_buf_prepare(struct vb2_buffer *vb)
{
    return (0);
}



static void c_port_buf_finish(struct vb2_buffer *vb)
{
}



static void c_port_buf_cleanup(struct vb2_buffer *vb)
{
}



static int c_port_start_streaming(struct vb2_queue *q, 
                                  unsigned int count
                                  )
{
    c_port      *This = (c_port *)q->drv_priv;
    zoe_errs_t  err;

    err = c_component_start(This->m_pComponent, 
                            This->m_id
                            ); 
    return (ZoeToLinuxKerStatus(err));
}



static void c_port_stop_streaming(struct vb2_queue *q)
{
    c_port  *This = (c_port *)q->drv_priv;

	c_component_stop(This->m_pComponent, 
                     This->m_id
                     ); 
}



static void c_port_buf_queue(struct vb2_buffer *vb)
{
    struct vb2_queue    *vq = vb->vb2_queue;
    c_port              *This = (c_port *)vq->drv_priv;
    PORT_VB2_DATA_REQ   *p_data_req;
    zoe_errs_t          err;

    down(&This->m_drv_q_lock);
    p_data_req = &This->m_port_reqs[vb->v4l2_buf.index];
    p_data_req->p_vb2_buf = vb;
    list_add_tail(&p_data_req->list, &This->m_drv_q);
    if (COMPONENT_PORT_YUV_OUT == This->m_id)
    {
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   g_ZVV4LDevDBGCompID,
				   "%s(%d) - list_add_tail p_data_req(%p) buf_id(%d)\n",
				   __FUNCTION__,
				   This->m_id,
                   p_data_req,
                   vb->v4l2_buf.index
				   );
    }
    up(&This->m_drv_q_lock);

    err = c_port_submit_buffer(This, p_data_req);

	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) - c_port_buf_queue FAILED!!!!(err)\n",
					   __FUNCTION__,
					   This->m_id,
                       err
					   );
	}
}



static struct vb2_ops   zvv4l_qops = 
{
    .queue_setup        = c_port_queue_setup,
    .wait_prepare       = c_port_unlock,
    .wait_finish        = c_port_lock,
    .buf_init           = c_port_buf_init,
    .buf_prepare        = c_port_buf_prepare,
    .buf_finish         = c_port_buf_finish,
    .buf_cleanup        = c_port_buf_cleanup,
    .start_streaming    = c_port_start_streaming,
    .stop_streaming     = c_port_stop_streaming,
    .buf_queue          = c_port_buf_queue,
};



/////////////////////////////////////////////////////////////////////////////
//
//


// constructor
//
c_port * c_port_constructor(c_port *pPort,
						    c_component *pComponent,
						    uint32_t dwAttributes,
						    COMPONENT_PORT_TYPE id,
						    PORT_DIRECTION dir,
						    uint32_t dwOpenType,
						    PCOMPONENT_PORT_OPEN_FORMAT pOpenFormat,
						    uint32_t frame_nbs,
						    uint32_t frame_size,
						    zoe_bool_t bBufferPartialFill,
						    zoe_bool_t bBufferFrameAligned,
						    zoe_bool_t bSupportUserPtr
						    )
{
	if (pPort)
	{
        int i;
        // zero init ourself
        //
        memset(pPort, 
               0, 
               sizeof(c_port)
               );

		// c_object
		//
		c_object_constructor(&pPort->m_Object, 
							 &pComponent->m_Object, 
                             OBJECT_ZOE_V4L2_PORT,
		  					 dwAttributes
		  					 );

		// initialize members
		//
		pPort->m_valid = ZOE_TRUE;
		pPort->m_pComponent = pComponent;
		pPort->m_id = id;
		pPort->m_hStreamLib = ZOE_NULL_HANDLE;
		pPort->m_dir = dir;
		pPort->m_dwOpenType = dwOpenType;
		if (pOpenFormat)
		{
			memcpy(&pPort->m_openFormat,  
				   pOpenFormat, 
				   sizeof(COMPONENT_PORT_OPEN_FORMAT)
				   );
		}
		else
		{
			memset(&pPort->m_openFormat, 
				   0, 
				   sizeof(COMPONENT_PORT_OPEN_FORMAT)
				   );
		}
		memset(&pPort->m_pic_format, 
			   0, 
			   sizeof(VPU_PICTURE)
			   );
        pPort->m_format_valid = (0 != frame_nbs) && (0 != frame_size);
        pPort->m_pixel_format = 0;
        pPort->m_vdec_std = 0;
		pPort->m_memoryType = -1;
		pPort->m_frame_nbs = frame_nbs;
		pPort->m_frame_size = frame_size;
		pPort->m_bBufferPartialFill = bBufferPartialFill;
		pPort->m_bBufferFrameAligned = bBufferFrameAligned;
		pPort->m_bSupportUserPtr = bSupportUserPtr;
		pPort->m_State = PORT_STATE_STOP;
		pPort->m_EOS = ZOE_FALSE;
		pPort->m_picture_num = 0;
		pPort->m_discontinuity = ZOE_FALSE;
		pPort->m_bDisabled = ZOE_FALSE;

        for (i = 0; i < ZV_AVLIB_MAX_DATA_ENTRIES; i++)
        {
            pPort->m_port_reqs[i].n_id = i;
        }

        pPort->m_buf_type = -1;             // v4l2_buf_type
        pPort->m_vb2_q_inited = ZOE_FALSE;
        INIT_LIST_HEAD(&pPort->m_drv_q);    // driver queue
        sema_init(&pPort->m_drv_q_lock, 1);
        pPort->m_alloc_ctx = ZOE_NULL;

		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) buf(%d) size(%d) \n",
					   __FUNCTION__,
					   pPort->m_id,
                       pPort->m_frame_nbs,
                       pPort->m_frame_size
					   );
	}

	return (pPort);
}



// destructor
//
void c_port_destructor(c_port *This)
{
    if (This->m_vb2_q_inited)
    {
        vb2_queue_release(&This->m_vb2_q);
        This->m_vb2_q_inited = ZOE_FALSE;
    }

    if (This->m_alloc_ctx)
    {
        vb2_dma_contig_cleanup_ctx(This->m_alloc_ctx);
        This->m_alloc_ctx = ZOE_NULL;
    }

	This->m_valid = ZOE_FALSE;

	c_object_destructor(&This->m_Object);
}



zoe_errs_t c_port_create(c_port *This)
{
	zoe_errs_t	        err;
	i_zv_codec	        *pMpegCodec = c_device_get_codec(This->m_pComponent->m_pDevice);
    struct vb2_queue    *vb2_q = &This->m_vb2_q;
    int                 ret;

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                   g_ZVV4LDevDBGCompID,
				   "%s\n", 
				   __FUNCTION__
				   );

	if (PORT_DIR_NONE != This->m_dir)
	{
        // allocate dma context
#if defined(CONFIG_ZV_HPU_EVK) && defined(CONFIG_HOST_PLATFORM_X86_LINUX)
        This->m_alloc_ctx = vb2_dma_contig_init_ctx(0);
#else //!CONFIG_ZV_HPU_EVK || !CONFIG_HOST_PLATFORM_X86_LINUX
        This->m_alloc_ctx = vb2_dma_contig_init_ctx(This->m_pComponent->m_pDevice->m_pGenericDevice);
#endif //CONFIG_ZV_HPU_EVK && CONFIG_HOST_PLATFORM_X86_LINUX

        // initialze driver queue
	    INIT_LIST_HEAD(&This->m_drv_q);

        // initialize vb2 queue
        vb2_q->type = (PORT_DIR_READ == This->m_dir) ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        vb2_q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
        vb2_q->gfp_flags = GFP_DMA32;
        vb2_q->ops = &zvv4l_qops;
        vb2_q->drv_priv = This;
        vb2_q->mem_ops = (struct vb2_mem_ops *)&vb2_dma_contig_memops;
        //vb2_q->mem_ops = (struct vb2_mem_ops *)&vb2_dma_sg_memops;
        vb2_q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
        ret = vb2_queue_init(vb2_q);
        if (ret) 
        {
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s vb2_queue_init() failed (%d)!\n", 
						   __FUNCTION__,
                           ret
						   );
			return (LinuxKerToZoeStatus(ret));
        }
    }

	This->m_bDisabled = ZOE_FALSE;

	err = c_port_codec_open(This, 
						    pMpegCodec
						    );

	return (err);
}



zoe_errs_t c_port_close(c_port *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                   g_ZVV4LDevDBGCompID,
				   "%s\n", 
				   __FUNCTION__
				   );

	// make sure we are stopped
	//
	if (PORT_STATE_STOP != This->m_State)
	{
		c_port_set_state(This,
					     PORT_STATE_STOP, 
					     This->m_State
					     );
	}

	// close codec lib handle
	//
	if (ZOE_NULL_HANDLE != This->m_hStreamLib)
	{
		i_zv_codec  *pMpegCodec = c_device_get_codec(This->m_pComponent->m_pDevice);
		pMpegCodec->close(pMpegCodec, 
						  This->m_hStreamLib
						  );
		This->m_hStreamLib = ZOE_NULL_HANDLE;
	}

	// flush driver queue
	//
    c_port_flush_drv_q(This);

    // release vb2 queue
    //
    if (This->m_vb2_q_inited)
    {
        vb2_queue_release(&This->m_vb2_q);
        This->m_vb2_q_inited = ZOE_FALSE;
    }

    // clean up vb2 mem op context
    //
    if (This->m_alloc_ctx)
    {
        vb2_dma_contig_cleanup_ctx(This->m_alloc_ctx);
        This->m_alloc_ctx = ZOE_NULL;
    }

	This->m_bDisabled = ZOE_FALSE;

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_port_enable(c_port *This)
{
	ENTER_CRITICAL(&This->m_Object)

	if (This->m_bDisabled)
	{
		This->m_bDisabled = ZOE_FALSE;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_port_disable(c_port *This)
{
	ENTER_CRITICAL(&This->m_Object)

	This->m_hStreamLib = ZOE_NULL_HANDLE;
	This->m_bDisabled = ZOE_TRUE;

	LEAVE_CRITICAL(&This->m_Object)

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_port_set_state(c_port *This,
						    PORT_STATE to_state, 
						    PORT_STATE from_state
						    )
{
	zoe_errs_t  err = ZOE_ERRS_SUCCESS;
	i_zv_codec	*pMpegCodec = c_device_get_codec(This->m_pComponent->m_pDevice);

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                   g_ZVV4LDevDBGCompID,
				   "%s port_id(%d) hStreamLib(%d) to_state(%d) [\n", 
				   __FUNCTION__,
				   This->m_id,
				   This->m_hStreamLib,
				   to_state
				   );

	if (This->m_State == to_state)
	{
		return (ZOE_ERRS_SUCCESS);
	}

	switch (to_state) 
	{
		case PORT_STATE_STOP:

			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s PORT_STATE_STOP codec handle(%d)\n", 
						   __FUNCTION__,
						   This->m_hStreamLib
						   );

			if (ZOE_NULL_HANDLE != This->m_hStreamLib)
			{
				err = pMpegCodec->stop(pMpegCodec, 
									   This->m_hStreamLib
									   );

				// flush driver queue
				//
                c_port_flush_drv_q(This);
            }

			if (!ZOE_SUCCESS(err))
			{
				err = ZOE_ERRS_SUCCESS;
			}        
			break;
        
		case PORT_STATE_ACQUIRE:

			if (PORT_STATE_STOP == from_state)
			{
				err = c_port_codec_open(This,
									    pMpegCodec
									    );
        
				c_port_reset_counters(This);
			}
        
			break;
        
		case PORT_STATE_PAUSE:
        
			if (!This->m_bDisabled)
			{
				err = c_port_codec_open(This,
									    pMpegCodec
									    );

				if (ZOE_NULL_HANDLE != This->m_hStreamLib)
				{
					if (PORT_STATE_RUN == from_state)
					{
						err = pMpegCodec->stop(pMpegCodec, 
											   This->m_hStreamLib
											   );
					}
					else
					{
						err = pMpegCodec->acquire(pMpegCodec, 
												  This->m_hStreamLib
												  );
					}
				}
			}
			else
			{
				err = ZOE_ERRS_INVALID;				 
			}

			zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                           g_ZVV4LDevDBGCompID,
						   "%s PORT_STATE_PAUSE handle(%d) err(%d)\n", 
						   __FUNCTION__,
						   This->m_hStreamLib,
						   err
						   );
			if (!ZOE_SUCCESS(err) &&
				(PORT_STATE_RUN == from_state)
				)
			{
				err = ZOE_ERRS_SUCCESS;
			}
        
			break;
        
		case PORT_STATE_RUN:

			This->m_discontinuity = ZOE_TRUE;
        
			err = c_port_codec_open(This, 
								    pMpegCodec
								    );

			if (ZOE_NULL_HANDLE != This->m_hStreamLib)
			{
				err = pMpegCodec->start(pMpegCodec, 
										This->m_hStreamLib
										);
			}
			else
			{
				err = ZOE_ERRS_INVALID;				 
			}

			zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                           g_ZVV4LDevDBGCompID,
						   "%s PORT_STATE_RUN handle(%d) err(%d)\n", 
						   __FUNCTION__,
						   This->m_hStreamLib,
						   err
						   );
			break;

		default:
			err = ZOE_ERRS_NOTIMPL;
			break;
	}

	if (ZOE_SUCCESS(err))
	{
		This->m_State = to_state;
	}

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                   g_ZVV4LDevDBGCompID,
				   "%s task(%d) handle(%d) from_state(%d) to_state(%d) err(%d)]\n", 
				   __FUNCTION__,
				   This->m_pComponent->m_hTask,
				   This->m_hStreamLib,
				   from_state,
				   to_state,
				   err
				   );

	return (err);
}



zoe_errs_t c_port_req_buf(c_port *This, 
                          struct file *file,
						  struct v4l2_requestbuffers *reqbuf
						  ) 
{
    int ret;

    if (0 == reqbuf->count)
    {
        ret = vb2_reqbufs(&This->m_vb2_q, reqbuf);
        if (!ret)
        {
            This->m_frame_nbs = 0;
        }
    }
    else
    {
        // save buffer type before calling vb2_reqbufs, this will be needed in our queue_setup
        This->m_buf_type = reqbuf->type;
        ret = vb2_reqbufs(&This->m_vb2_q, reqbuf);
        if (!ret)
        {
            This->m_frame_nbs = reqbuf->count;
            This->m_memoryType = reqbuf->memory;
        }
    }

    return (LinuxKerToZoeStatus(ret));
}



zoe_errs_t c_port_query_buf(c_port *This, 
                            struct file *file,
						    struct v4l2_buffer *buf
						    ) 
{
    int ret;
    int i;

	ret = vb2_querybuf(&This->m_vb2_q, buf);
    if (!ret)
    {
        if (V4L2_MEMORY_MMAP == buf->memory)
        {
            if (V4L2_TYPE_IS_MULTIPLANAR(buf->type))
            {
                for (i = 0; i < buf->length; i++)
                {
                    buf->m.planes[i].m.mem_offset |= (This->m_id << PORT_MMAP_BUF_TYPE_SHIFT);
                }
            }
            else
            {
                buf->m.offset |= (This->m_id << PORT_MMAP_BUF_TYPE_SHIFT);
            }
        }
    }
    return (LinuxKerToZoeStatus(ret));
}



zoe_errs_t c_port_dqbuf(c_port *This, 
                        struct file *file,
					    struct v4l2_buffer *pv4lbufapp
					    ) 
{
    int ret;

    ret = vb2_dqbuf(&This->m_vb2_q, pv4lbufapp, file->f_flags & O_NONBLOCK);
    return (LinuxKerToZoeStatus(ret));
}



zoe_errs_t c_port_qbuf(c_port *This, 
                       struct file *file,
					   struct v4l2_buffer *pv4lbufapp
					   ) 
{
    int ret;

    ret = vb2_qbuf(&This->m_vb2_q, pv4lbufapp);
    return (LinuxKerToZoeStatus(ret));
}



zoe_bool_t c_port_buf_rdy(c_port *This,
                          struct file *file,
                          poll_table *wait
                          )
{
    poll_wait(file, &This->m_vb2_q.done_wq, wait);
    return (!list_empty(&This->m_vb2_q.done_list));
}
