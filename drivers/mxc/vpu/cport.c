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
// cport.c
//
// Description: 
//
//  base port implementation
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

#include "zoe_dbg.h"
#include "cport.h"
#include "ccomponent.h"
#include "cdevice.h"
#include "objids.h"
#include "zoe_linux_user_pages.h"


// define this to change the v4l2 mmap buffer transaction behavior to
// app -> regbuf
// app -> querybuf
// app -> qbuf
// app -> dqbuf 
//
// without this been defined the v4l2 mmap buffer transaction behavior
// app -> reqbuf
// app -> querybuf
// app -> dqbuf
// app -> qbuf
//
#define MMAP_NEED_QBUF

/////////////////////////////////////////////////////////////////////////////
//
//

extern zoe_dbg_comp_id_t    g_ZVV4LDevDBGCompID;


#define V4L2_BUFFER_QUEUED(p_buffer) \
    (p_buffer)->flags &= ~V4L2_BUF_FLAG_DONE; \
    (p_buffer)->flags |= V4L2_BUF_FLAG_QUEUED;

#define V4L2_BUFFER_DONE(p_buffer) \
    (p_buffer)->flags &= ~V4L2_BUF_FLAG_QUEUED; \
    (p_buffer)->flags |= V4L2_BUF_FLAG_DONE;

#define V4L2_BUFFER_USR(p_buffer) \
    (p_buffer)->flags &= ~(V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE);


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

static QUEUE_ENTRY* c_queue_get_entry_by_v4l2_buf(c_queue *This,
											      struct v4l2_buffer *pV4L2Buf
											      )
{
    QUEUE_ENTRY		*pthis_queue = This->m_Queue.pHead;
    QUEUE_ENTRY		*pprev_queue = ZOE_NULL;
	PPORT_DATA_REQ	pDataReq;
    zoe_bool_t      found = ZOE_FALSE;

	ENTER_CRITICAL(&This->m_Object)

    while (pthis_queue)
    {
		pDataReq = (PPORT_DATA_REQ)pthis_queue->Data;

        if (pDataReq->pV4L2Buf->type == pV4L2Buf->type)
        {
            if ((V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pV4L2Buf->type) ||
                (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pV4L2Buf->type)
                )
            {
	            if (pDataReq->pV4L2Buf->m.planes[0].m.mem_offset == (pV4L2Buf->m.planes[0].m.mem_offset & ~PORT_MMAP_BUF_TYPE_MASK))
		        {
                    found = ZOE_TRUE;
                }
            }
            else
            {
	            if (pDataReq->pV4L2Buf->m.offset == (pV4L2Buf->m.offset & ~PORT_MMAP_BUF_TYPE_MASK))
		        {
                    found = ZOE_TRUE;
                }
            }

            if (found)
            {
		        if (!pprev_queue)
		        {
			        This->m_Queue.pHead = pthis_queue->pNext;
			        if (!This->m_Queue.pHead)
				        This->m_Queue.pTail = This->m_Queue.pHead;
		        }
		        else
		        {
			        pprev_queue->pNext = pthis_queue->pNext;
			        if (!pthis_queue->pNext)
				        This->m_Queue.pTail = pprev_queue;
		        }
		        This->m_dwNbInQueue--;
		        break;
            }
	    }
	    pprev_queue = pthis_queue;
	    pthis_queue = pthis_queue->pNext;
    }

	LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}



static QUEUE_ENTRY* c_queue_peek_entry_by_v4l2_buf(c_queue *This,
											       struct v4l2_buffer *pV4L2Buf
											       )
{
    QUEUE_ENTRY		*pthis_queue = This->m_Queue.pHead;
	PPORT_DATA_REQ	pDataReq;

	ENTER_CRITICAL(&This->m_Object)

    while (pthis_queue)
    {
		pDataReq = (PPORT_DATA_REQ)pthis_queue->Data;

        if (pDataReq->pV4L2Buf->type == pV4L2Buf->type)
        {
            if ((V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pV4L2Buf->type) ||
                (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pV4L2Buf->type)
                )
            {
	            if (pDataReq->pV4L2Buf->m.planes[0].m.mem_offset == (pV4L2Buf->m.planes[0].m.mem_offset & ~PORT_MMAP_BUF_TYPE_MASK))
		        {
                    break;
                }
            }
            else
            {
	            if (pDataReq->pV4L2Buf->m.offset == (pV4L2Buf->m.offset & ~PORT_MMAP_BUF_TYPE_MASK))
		        {
                    break;
                }
            }
        }

	    pthis_queue = pthis_queue->pNext;
    }

	LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}



static QUEUE_ENTRY* c_queue_get_entry_by_buf_desc(c_queue *This,
											      PZV_BUFFER_DESCRIPTOR pBufDesc
											      )
{
    QUEUE_ENTRY		*pthis_queue = This->m_Queue.pHead;
    QUEUE_ENTRY		*pprev_queue = ZOE_NULL;
	PPORT_DATA_REQ	pDataReq;

	ENTER_CRITICAL(&This->m_Object)

    while (pthis_queue)
    {
		pDataReq = (PPORT_DATA_REQ)pthis_queue->Data;

	    if (pDataReq->pBufDesc == pBufDesc)
		{
		    if (!pprev_queue)
		    {
			    This->m_Queue.pHead = pthis_queue->pNext;
			    if (!This->m_Queue.pHead)
				    This->m_Queue.pTail = This->m_Queue.pHead;
		    }
		    else
		    {
			    pprev_queue->pNext = pthis_queue->pNext;
			    if (!pthis_queue->pNext)
				    This->m_Queue.pTail = pprev_queue;
		    }
		    This->m_dwNbInQueue--;
		    break;
	    }
	    pprev_queue = pthis_queue;
	    pthis_queue = pthis_queue->pNext;
    }

	LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}



static QUEUE_ENTRY* c_queue_peek_entry_by_buf_desc(c_queue *This,
											       PZV_BUFFER_DESCRIPTOR pBufDesc
											       )
{
    QUEUE_ENTRY		*pthis_queue = This->m_Queue.pHead;
	PPORT_DATA_REQ	pDataReq;

	ENTER_CRITICAL(&This->m_Object)

    while (pthis_queue)
    {
		pDataReq = (PPORT_DATA_REQ)pthis_queue->Data;

	    if (pDataReq->pBufDesc == pBufDesc)
		    break;

	    pthis_queue = pthis_queue->pNext;
    }

	LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}



static QUEUE_ENTRY* c_queue_get_entry_by_v4l2_buf_id(c_queue *This,
											         struct v4l2_buffer *pV4L2Buf
											         )
{
    QUEUE_ENTRY		*pthis_queue = This->m_Queue.pHead;
    QUEUE_ENTRY		*pprev_queue = ZOE_NULL;
	PPORT_DATA_REQ	pDataReq;

	ENTER_CRITICAL(&This->m_Object)

    while (pthis_queue)
    {
		pDataReq = (PPORT_DATA_REQ)pthis_queue->Data;

        if (pDataReq->pV4L2Buf->index == pV4L2Buf->index)
        {
		    if (!pprev_queue)
		    {
			    This->m_Queue.pHead = pthis_queue->pNext;
			    if (!This->m_Queue.pHead)
			        This->m_Queue.pTail = This->m_Queue.pHead;
		    }
		    else
		    {
			    pprev_queue->pNext = pthis_queue->pNext;
			    if (!pthis_queue->pNext)
			        This->m_Queue.pTail = pprev_queue;
		    }
		    This->m_dwNbInQueue--;
		    break;
	    }
	    pprev_queue = pthis_queue;
	    pthis_queue = pthis_queue->pNext;
    }

	LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}



static QUEUE_ENTRY* c_queue_peek_entry_by_v4l2_buf_id(c_queue *This,
											          struct v4l2_buffer *pV4L2Buf
											          )
{
    QUEUE_ENTRY		*pthis_queue = This->m_Queue.pHead;
	PPORT_DATA_REQ	pDataReq;

	ENTER_CRITICAL(&This->m_Object)

    while (pthis_queue)
    {
		pDataReq = (PPORT_DATA_REQ)pthis_queue->Data;

        if (pDataReq->pV4L2Buf->index == pV4L2Buf->index)
        {
            break;
        }
	    pthis_queue = pthis_queue->pNext;
    }

	LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue);
}



static void c_port_time_stamp(c_port *This,
							  PZV_BUFFER_DESCRIPTOR pBufDesc,
							  struct v4l2_buffer *pV4L2Buf
							  )
{
	if (PORT_DIR_READ == This->m_dir)
	{
		if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_PTS)
		{
			// construct time stamp
			//
            pts_2_timeval(pBufDesc->ulPTS, 
                          &pV4L2Buf->timestamp
                          );
			pV4L2Buf->flags |= V4L2_BUF_FLAG_TIMESTAMP_COPY;
		}
		else
		{
			pV4L2Buf->flags &= ~V4L2_BUF_FLAG_TIMESTAMP_COPY;
		}
	}

    if (This->m_discontinuity)
	{
		This->m_discontinuity = ZOE_FALSE;
	}
}



static void c_port_fill_frame_info(c_port *This,
								   struct v4l2_buffer *pV4L2Buf,
								   uint32_t *DataUsed,
								   uint32_t ulFlags
								   ) 
{
    uint32_t    i;

    if ((V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pV4L2Buf->type) ||
        (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pV4L2Buf->type)
        )
    {
	    pV4L2Buf->bytesused = 0;
        for (i = 0; i < pV4L2Buf->length; i++)
        {
            pV4L2Buf->m.planes[i].bytesused = DataUsed[i];
        }
    }
    else
    {
	    pV4L2Buf->bytesused = *DataUsed;
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



static void *c_port_map_kernel(c_port *This,
                               ZOE_USER_PAGES *p_user_pages,
                               uint64_t ptr
                               )
{
    void            *vaddr;
	pgprot_t        pgprot;
    unsigned int    offset = (unsigned int)(ptr & ~PAGE_MASK);

	if (!p_user_pages)
    {
		return NULL;
    }

    pgprot = PAGE_KERNEL;

    vaddr = vmap(p_user_pages->pages, 
                 p_user_pages->nr_pages, 
                 VM_MAP, 
                 pgprot
                 );
    vaddr += offset;
	return (vaddr);
}



static void c_port_unmap_kernel(c_port *This,
			                    void *vaddr
                                )
{
    if (vaddr)
    {
	    vunmap((void *)((uint64_t)vaddr & PAGE_MASK));
    }
}



static void c_port_build_buf_desc(c_port *This,
								  PZV_BUFFER_DESCRIPTOR pBufDesc,
								  struct v4l2_buffer *pV4L2Buf
								  )
{
    uint32_t    total_used = 0;
    uint32_t    i;

	if (pV4L2Buf->flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) 
	{
		pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_PTS;
		pBufDesc->ulPTS = timeval_2_pts(pV4L2Buf->timestamp);
	} 
	else 
	{
		pBufDesc->ulFlags = 0;
		pBufDesc->ulPTS = 0;
	}
    
    if ((V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pV4L2Buf->type) ||
        (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pV4L2Buf->type)
        )
    {
	    pBufDesc->NumberOfBuffers = pV4L2Buf->length;
	    pBufDesc->ulBufferSize = 0;

		zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                       g_ZVV4LDevDBGCompID,
		               "%s()", 
					   __FUNCTION__
					   );

        for (i = 0; i < pV4L2Buf->length; i++)
        {
	        pBufDesc->DataBuffer[i].Duration = 0;
	        pBufDesc->DataBuffer[i].BufferSize = pV4L2Buf->m.planes[i].length;
	        pBufDesc->DataBuffer[i].DataOffset = pV4L2Buf->m.planes[i].data_offset;
	        pBufDesc->ulBufferSize += pV4L2Buf->m.planes[i].length;
	        pBufDesc->DataBuffer[i].DataUsed = pV4L2Buf->m.planes[i].bytesused;
            total_used += pBufDesc->DataBuffer[i].DataUsed;
	        if (V4L2_MEMORY_MMAP == This->m_memoryType)
	        {
		        pBufDesc->DataBuffer[i].Data = This->m_pBufMem + pV4L2Buf->m.planes[i].m.mem_offset;
		        pBufDesc->DataBuffer[i].Data_mapped = This->m_pBufMem + pV4L2Buf->m.planes[i].m.mem_offset;
            }
            else
            {
		        pBufDesc->DataBuffer[i].Data = (zoe_void_ptr_t)pV4L2Buf->m.planes[i].m.userptr;
            }

		    zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                           g_ZVV4LDevDBGCompID,
		                   "(%p:%d:%d) ",
                           pBufDesc->DataBuffer[i].Data,
                           pBufDesc->DataBuffer[i].BufferSize,
                           pBufDesc->DataBuffer[i].DataOffset
					       );
        }

		zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                       g_ZVV4LDevDBGCompID,
		               "\n"
				       );

    }
    else
    {
	    pBufDesc->NumberOfBuffers = 1;
	    pBufDesc->ulBufferSize = pV4L2Buf->length;
	    pBufDesc->DataBuffer[0].Duration = 0;
	    pBufDesc->DataBuffer[0].BufferSize = pV4L2Buf->length;
	    pBufDesc->DataBuffer[0].DataOffset = 0;
	    pBufDesc->DataBuffer[0].DataUsed = pV4L2Buf->bytesused;
        total_used += pBufDesc->DataBuffer[0].DataUsed;
	    if (V4L2_MEMORY_MMAP == This->m_memoryType)
	    {
		    pBufDesc->DataBuffer[0].Data = This->m_pBufMem + pV4L2Buf->m.offset;
		    pBufDesc->DataBuffer[0].Data_mapped = This->m_pBufMem + pV4L2Buf->m.offset;
	    }
	    else
	    {
		    pBufDesc->DataBuffer[0].Data = (zoe_void_ptr_t)pV4L2Buf->m.userptr;
	    }
    }

    if (PORT_DIR_WRITE == This->m_dir)
    {
        if (0 == total_used)
        {
		    pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_EOS;
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
		                   "%s() - EOS", 
					       __FUNCTION__
					       );
        }
    }


    pBufDesc->ulFlags &= ~DMA_BUFFER_MODE_MASK;
	if (V4L2_MEMORY_MMAP == pV4L2Buf->memory)
	{
		pBufDesc->ulFlags |= DMA_BUFFER_MODE_KERNEL;
	}
	else
	{
		pBufDesc->ulFlags |= DMA_BUFFER_MODE_USERPTR;
	}

	pBufDesc->ulBufferIndex = 0;
	pBufDesc->ulBufferOffset = 0;
	pBufDesc->ulTotalUsed = 0;
	if (This->m_bBufferPartialFill)
	{
		pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_PARTIAL_FILL_OK;
	}
	if (This->m_bBufferFrameAligned)
	{
		pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_FRAME_ALIGNED;
	}
	pBufDesc->Status = ZOE_ERRS_PENDING;
}



static zoe_errs_t c_port_build_scatter_gather_list(c_port *This,
										           PZV_BUFFER_DESCRIPTOR pBufDesc
										           )
{
    uint32_t    i;

    if (V4L2_MEMORY_USERPTR == This->m_memoryType)
    {
        for (i = 0; i < pBufDesc->NumberOfBuffers; i++)
        {
            PZOE_USER_PAGES p_user_pages = (PZOE_USER_PAGES)pBufDesc->DataBuffer[i].p_user_mappings; 
            uint64_t        data = (uint64_t)pBufDesc->DataBuffer[i].Data;
	        long			ret = 0;	
            void            *vaddr;

            p_user_pages->size = pBufDesc->DataBuffer[i].BufferSize;

            if (pBufDesc->DataBuffer[i].BufferSize)
            {
                p_user_pages->sg = ZOE_NULL;
   	            p_user_pages->first = (data & PAGE_MASK) >> PAGE_SHIFT;
	            p_user_pages->last  = ((data + p_user_pages->size - 1) & PAGE_MASK) >> PAGE_SHIFT;
	            p_user_pages->nr_pages = (unsigned int)(p_user_pages->last - p_user_pages->first + 1);
	            p_user_pages->pages = kmalloc(p_user_pages->nr_pages * sizeof(struct page*),
						                      GFP_KERNEL
						                      );
	            if (ZOE_NULL == p_user_pages->pages)
	            {
		            zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                                   g_ZVV4LDevDBGCompID,
					               "%s() Unable to allocate page array\n", 
					               __FUNCTION__
					               );
		            return (ZOE_ERRS_NOMEMORY);
	            }

	            zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                               g_ZVV4LDevDBGCompID,
				               "%s() init user [%p + 0x%lx => %d pages]\n", 
				               __FUNCTION__,
				               data,
				               p_user_pages->size,
				               p_user_pages->nr_pages
				               );

	            down_read(&current->mm->mmap_sem);
	            ret = get_user_pages(current,
						             current->mm,
						             (uintptr_t)(data & PAGE_MASK), 
						             p_user_pages->nr_pages,
						             This->m_dir == PORT_DIR_READ, 
						             1, /* force */
						             p_user_pages->pages, 
						             NULL
						             );
	            up_read(&current->mm->mmap_sem);
	            if (ret != p_user_pages->nr_pages) 
	            {
		            zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                                   g_ZVV4LDevDBGCompID,
					               "%s() get_user_pages: err=%d nr_pages[%d] data[%p] size(%d)\n", 
					               __FUNCTION__,
					               ret,
					               p_user_pages->nr_pages,
					               data,
					               p_user_pages->size
					               );
		            p_user_pages->nr_pages = (ret >= 0) ? ret : 0;
		            return (ZOE_ERRS_INVALID);
	            }

#if 1
                // need to map user pages to kernel space for pseudo firmware modules
                //
                vaddr = c_port_map_kernel(This, 
                                          p_user_pages,
                                          data
                                          );
                if (vaddr)
                {
                    pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_KERNEL_MAPPED;
                    pBufDesc->DataBuffer[i].Data_mapped = vaddr;
                }
                else
                {
                    pBufDesc->DataBuffer[i].Data_mapped = ZOE_NULL;
		            zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                                   g_ZVV4LDevDBGCompID,
					               "%s() c_port_map_kernel() failed!\n", 
					               __FUNCTION__
					               );
                }
#endif
            }
            else
            {
                p_user_pages->nr_pages = 0;
                p_user_pages->pages = NULL;
                pBufDesc->ulFlags &= ~ZV_BUFDESC_FLAG_KERNEL_MAPPED;
                pBufDesc->DataBuffer[i].Data_mapped = ZOE_NULL;
            }
        }
    }
	return (ZOE_ERRS_SUCCESS);
}



static void c_port_clean_up_data_request(c_port *This, 
									     PPORT_DATA_REQ pDataReq
									     )
{
	PZV_BUFFER_DESCRIPTOR	pBufDesc = pDataReq->pBufDesc;
    uint32_t                i, j;

    if (V4L2_MEMORY_USERPTR == This->m_memoryType)
    {
        for (i = 0; i < pBufDesc->NumberOfBuffers; i++)
        {
            PZOE_USER_PAGES p_user_pages = (PZOE_USER_PAGES)pBufDesc->DataBuffer[i].p_user_mappings; 

	        if (p_user_pages && 
                p_user_pages->pages
                ) 
	        {
	            for (j = 0; j < p_user_pages->nr_pages; j++)
	            {
		            page_cache_release(p_user_pages->pages[j]);
	            }
	            kfree(p_user_pages->pages);
	            p_user_pages->pages = ZOE_NULL;
	        }
#if 1
            if (pBufDesc->DataBuffer[i].Data_mapped)
            {
                if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_KERNEL_MAPPED)
                {
                    c_port_unmap_kernel(This, 
                                       pBufDesc->DataBuffer[i].Data_mapped
                                       );
                }
                pBufDesc->DataBuffer[i].Data_mapped = ZOE_NULL;
            }
#endif
        }

        // clear the flag
        pBufDesc->ulFlags &= ~ZV_BUFDESC_FLAG_KERNEL_MAPPED;
    }
}



#ifndef MMAP_NEED_QBUF

static void c_port_submit_buffers(c_port *This)
{
	c_queue		*pQueue;
	QUEUE_ENTRY	*pEntry;
	i_zv_codec	*pMpegCodec;

    // Submit as many buffers as we can to the library
	//

	// Only give the buffers to the firmware if we are not stopped
	//
	if ((V4L2_MEMORY_MMAP != This->m_memoryType) ||
		(ZOE_NULL_HANDLE == This->m_hStreamLib) ||
		(PORT_STATE_STOP == This->m_State) ||
		(PORT_DIR_NONE == This->m_dir)
		)
	{
		return;
	}

	// get the codec chip interface
	//
	pMpegCodec = c_device_get_codec(This->m_pComponent->m_pDevice);

	// find the right queue based on the port direction
	//
	pQueue = (PORT_DIR_READ == This->m_dir) ? This->m_pQueueFree : This->m_pQueueData;

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE/*ZOE_DBG_LVL_ERROR*/,
                   g_ZVV4LDevDBGCompID,
				   "%s(%d) - hStreamLib(%d) dir(%d) pQueue(0x%x) free(0x%x) data(0x%x)\n",
				   __FUNCTION__,
				   This->m_id,
				   This->m_hStreamLib,
				   This->m_dir,
				   pQueue,
				   This->m_pQueueFree,
				   This->m_pQueueData
				   );

	// get a free queue entry
	//
	pEntry = c_queue_get_one_entry(pQueue);

	// save it to the data request queue
	//
	while (pEntry && 
		   !This->m_EOS
		   )
	{
		PPORT_DATA_REQ	pDataReq = (PPORT_DATA_REQ)pEntry->Data;
		zoe_errs_t		err;

   		c_port_build_buf_desc(This,
							  pDataReq->pBufDesc, 
							  pDataReq->pV4L2Buf
							  );
		c_port_build_scatter_gather_list(This,
									     pDataReq->pBufDesc
									     );

		// save this request to the lib queue
		//
		c_queue_add_entry(This->m_pQueueLib, 
						  pEntry
						  );

		// add the buffer to the codec library
		//
		err = pMpegCodec->add_buffer(pMpegCodec,
									 This->m_hStreamLib,
									 pDataReq->pBufDesc
									 );
		if (!ZOE_SUCCESS(err))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s(%d) - hStreamLib(%d) add_buffer id(%d) (0x%X) FAILED!!!!\n",
						   __FUNCTION__,
						   This->m_id,
						   This->m_hStreamLib,
						   pDataReq->nId,
						   pDataReq->pBufDesc
						   );

            c_port_clean_up_data_request(This, 
                                         pDataReq
                                         );
			c_queue_remove_entry(This->m_pQueueLib,
							     pEntry
							     );
			c_queue_add_entry(This->m_pQueueFree, 
							  pEntry
							  );
			break;
		}
		else
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_XTRACE/*ZOE_DBG_LVL_TRACE*/,
                           g_ZVV4LDevDBGCompID,
						   "%s(%d) - hStreamLib(%d) add_buffer id(%d) (0x%X)\n",
						   __FUNCTION__,
						   This->m_id,
						   This->m_hStreamLib,
						   pDataReq->nId,
						   pDataReq->pBufDesc
						   );
		}

		pEntry = c_queue_get_one_entry(pQueue);
	}
}
#endif //!MMAP_NEED_QBUF



static zoe_errs_t c_port_submit_buffer(c_port *This,
                                       QUEUE_ENTRY *p_entry,
                                       c_queue *p_queue_ret
                                       )
{
    PPORT_DATA_REQ	p_data_req = (PPORT_DATA_REQ)p_entry->Data;
    i_zv_codec      *p_codec = c_device_get_codec(This->m_pComponent->m_pDevice);
    zoe_errs_t      err;

	// build the buffer descriptor
	//
	c_port_build_buf_desc(This,
						  p_data_req->pBufDesc, 
						  p_data_req->pV4L2Buf
						  );
	err = c_port_build_scatter_gather_list(This,
								           p_data_req->pBufDesc
								           );
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) - c_port_build_scatter_gather_list FAILED!!!!\n",
					   __FUNCTION__,
					   This->m_id
					   );
        c_port_clean_up_data_request(This, 
                                     p_data_req
                                     );
        V4L2_BUFFER_USR(p_data_req->pV4L2Buf);
		c_queue_add_entry(p_queue_ret, 
						  p_entry
						  );
	}
    else
    {
	    // save this request to the lib queue
	    //
	    c_queue_add_entry(This->m_pQueueLib, 
					      p_entry
					      );

	    // add the buffer to the codec library
	    //
	    err = p_codec->add_buffer(p_codec,
								  This->m_hStreamLib,
								  p_data_req->pBufDesc
								  );
	    if (!ZOE_SUCCESS(err))
	    {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
					       "%s(%d) - hStreamLib(%d) add_buffer id(%d) (0x%X) FAILED!!!!\n",
					       __FUNCTION__,
					       This->m_id,
					       This->m_hStreamLib,
					       p_data_req->nId,
					       p_data_req->pBufDesc
					       );
            c_port_clean_up_data_request(This, 
                                         p_data_req
                                         );
		    c_queue_remove_entry(This->m_pQueueLib,
						         p_entry
						         );
            V4L2_BUFFER_USR(p_data_req->pV4L2Buf);
		    c_queue_add_entry(p_queue_ret, 
						      p_entry
						      );
	    }
    }
    return (err);
}


static void c_port_process_completed_buffer(c_port *This,
										    struct v4l2_buffer *pV4L2Buf
										    )
{
}



static zoe_errs_t c_port_on_buffer_complete(c_port *This,
										    QUEUE_ENTRY *pEntry
										    )
{
	PPORT_DATA_REQ			pDataReq = (PPORT_DATA_REQ)pEntry->Data;
	PZV_BUFFER_DESCRIPTOR	pBufDesc = pDataReq->pBufDesc;
	struct v4l2_buffer		*pV4L2Buf = pDataReq->pV4L2Buf;
	c_queue					*pQueue;
    uint32_t                data_used[ZV_MAX_BUF_PER_DESC];
    uint32_t                i;

    for (i = 0; i < pBufDesc->NumberOfBuffers; i++)
    {
        data_used[i] = pBufDesc->DataBuffer[i].DataUsed;
    }

    c_port_time_stamp(This,
					  pBufDesc,
					  pV4L2Buf
					  );
	c_port_fill_frame_info(This,
						   pV4L2Buf,
						   data_used, 
						   pBufDesc->ulFlags
						   );
	c_port_process_completed_buffer(This, 
								    pV4L2Buf
								    );	
	c_port_clean_up_data_request(This, 
							     pDataReq
							     );

#ifdef MMAP_NEED_QBUF
	pQueue = This->m_pQueueData;
#else //!MMAP_NEED_QBUF
	// find the right queue based on the port direction
	if (V4L2_MEMORY_MMAP == This->m_memoryType)
	{
		pQueue = (PORT_DIR_READ == This->m_dir) ? This->m_pQueueData : This->m_pQueueFree;
	}
	else
	{
		pQueue = This->m_pQueueData;
	}
#endif //MMAP_NEED_QBUF

    V4L2_BUFFER_DONE(pV4L2Buf);

	// move entry to the queue
	c_queue_add_entry(pQueue, 
					  pEntry
					  );

    // wake up the event for poll/select
    wake_up(&This->m_buf_ready_wq);

#ifndef MMAP_NEED_QBUF
	// push data through
	c_port_submit_buffers(This);
#endif //!MMAP_NEED_QBUF

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
			PZV_BUFFER_DESCRIPTOR	pBufDesc = (PZV_BUFFER_DESCRIPTOR)pParam;
			QUEUE_ENTRY				*pEntry;

			if (ZOE_NULL != (pEntry = c_queue_get_entry_by_buf_desc(This->m_pQueueLib, pBufDesc)))
			{
				PPORT_DATA_REQ	pDataReq = (PPORT_DATA_REQ)pEntry->Data;

				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               g_ZVV4LDevDBGCompID,
							   "%s(%d) : MPGCODEC_CMD_DONE_DATA id(%d) pBufDesc(0x%X) status(%d)\n",
							   __FUNCTION__,
							   This->m_hStreamLib,
							   pDataReq->nId,
							   pBufDesc, 
							   pBufDesc->Status
							   );

				err = c_port_on_buffer_complete(This,
											    pEntry
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



static zoe_errs_t c_port_init_free_queue(c_port *This)
{
	int i, j;

    if (!This->m_format_valid ||
        !This->m_frame_nbs ||
        !This->m_frame_size
        )
    {
        return (ZOE_ERRS_INVALID);
    }

    if (!This->m_free_queue_inited)
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s type(%d) buf_cnt(%d)\n", 
					   __FUNCTION__,
                       This->m_memoryType,
                       This->m_frame_nbs
					   );

		if (V4L2_MEMORY_MMAP == This->m_memoryType)
		{
            if (!This->m_pBufMem)
            {
			    This->m_pBufMem = vmalloc_32(This->m_frame_nbs * This->m_frame_size);
			    if (!This->m_pBufMem)
			    {
				    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                                   g_ZVV4LDevDBGCompID,
							       "%s Unable to allocate buffer(%d x %d)!\n", 
							       __FUNCTION__,
							       This->m_frame_nbs,
							       This->m_frame_size
							       );
				    return (ZOE_ERRS_NOMEMORY);
			    }
            }
        }

	    for (i = 0; i < This->m_frame_nbs; i++)
	    {
		    memset(&This->m_BufDesces[i], 
			       0, 
			       sizeof(ZV_BUFFER_DESCRIPTOR)
			       );
		    memset(&This->m_v4l2Buffers[i], 
			       0, 
			       offsetof(struct v4l2_buffer, m)
			       );
		    This->m_v4l2Buffers[i].index = i;
            This->m_v4l2Buffers[i].type = (PORT_DIR_READ == This->m_dir) ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		    This->m_v4l2Buffers[i].bytesused = 0;
		    This->m_v4l2Buffers[i].field = V4L2_FIELD_ANY;
		    This->m_v4l2Buffers[i].sequence = This->m_id;
		    This->m_v4l2Buffers[i].memory = This->m_memoryType;
		    if (V4L2_MEMORY_MMAP == This->m_memoryType)
		    {
#ifdef MMAP_NEED_QBUF
                This->m_v4l2Buffers[i].flags = 0;
#else //!MMAP_NEED_QBUF
                This->m_v4l2Buffers[i].flags = (PORT_DIR_WRITE == This->m_dir) ? V4L2_BUF_FLAG_DONE : V4L2_BUF_FLAG_QUEUED;
#endif //MMAP_NEED_QBUF

                if ((COMPONENT_PORT_YUV_IN == This->m_id) ||
                    (COMPONENT_PORT_YUV_OUT == This->m_id)
                    )
                {
			        This->m_v4l2Buffers[i].m.planes[0].m.mem_offset = i * This->m_frame_size;
		            This->m_v4l2Buffers[i].m.planes[0].length = (This->m_openFormat.yuv.nWidth * This->m_openFormat.yuv.nHeight);
			        This->m_v4l2Buffers[i].m.planes[0].bytesused = 0;
			        This->m_v4l2Buffers[i].m.planes[0].data_offset = 0;
			        This->m_v4l2Buffers[i].m.planes[1].m.mem_offset = This->m_v4l2Buffers[i].m.planes[0].m.mem_offset + This->m_v4l2Buffers[i].m.planes[0].length;
		            This->m_v4l2Buffers[i].m.planes[1].length = (This->m_v4l2Buffers[i].m.planes[0].length / 2);
			        This->m_v4l2Buffers[i].m.planes[1].bytesused = 0;
			        This->m_v4l2Buffers[i].m.planes[1].data_offset = 0;
			        This->m_v4l2Buffers[i].length = 2;
                }
                else
                {
			        This->m_v4l2Buffers[i].m.planes[0].m.mem_offset = i * This->m_frame_size;
			        This->m_v4l2Buffers[i].m.planes[0].length = This->m_frame_size;
			        This->m_v4l2Buffers[i].m.planes[0].bytesused = 0;
			        This->m_v4l2Buffers[i].m.planes[0].data_offset = 0;
			        This->m_v4l2Buffers[i].length = 1;
                }
		    }
		    else
		    {
                int nb_buf;

		        This->m_v4l2Buffers[i].flags = 0;

                if ((COMPONENT_PORT_YUV_IN == This->m_id) ||
                    (COMPONENT_PORT_YUV_OUT == This->m_id)
                    )
                {
                    nb_buf = ZV_MAX_BUF_PER_DESC;
                }
                else
                {
                    nb_buf = 1;
                }

                for (j = 0; j < nb_buf; j++)
                {
                    This->m_BufDesces[i].DataBuffer[j].p_user_mappings = vmalloc(sizeof(ZOE_USER_PAGES));
                    if (!This->m_BufDesces[i].DataBuffer[j].p_user_mappings)
                    {
		                zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                                       g_ZVV4LDevDBGCompID,
					                   "%s Unable to alloc ZOE_USER_PAGES!\n", 
					                   __FUNCTION__ 
					                   );
		                return (ZOE_ERRS_NOMEMORY);
                    }
                }
		    }

		    This->m_PortReqs[i].pBufDesc = &This->m_BufDesces[i];
		    This->m_PortReqs[i].pV4L2Buf = &This->m_v4l2Buffers[i];
		    This->m_PortReqs[i].nId = i;

		    This->m_Entries[i].pNext = ZOE_NULL;
		    This->m_Entries[i].Data = &This->m_PortReqs[i];
		    c_queue_add_entry(This->m_pQueueFree, 
						      &This->m_Entries[i]
						      );
	    }

        This->m_free_queue_inited = ZOE_TRUE;

#ifndef MMAP_NEED_QBUF
		if ((V4L2_MEMORY_MMAP == This->m_memoryType) &&
            (PORT_DIR_WRITE == This->m_dir)
            )
		{
            // wake up the event for poll/select
            wake_up(&This->m_buf_ready_wq);
        }
#endif //!MMAP_NEED_QBUF
    }

    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_port_done_free_queue(c_port *This)
{
	int i;

	if (PORT_STATE_RUN == This->m_State) 
	{
		return (ZOE_ERRS_INVALID);
	}

    if (This->m_free_queue_inited)
    {
        This->m_free_queue_inited = ZOE_FALSE;

        // flush free queue 
        //
        c_queue_flush_queue(This->m_pQueueFree);

        // free all the previously allocated memory
        //
	    if (This->m_pBufMem)
	    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
		    unsigned long	ulAddr = (unsigned long)This->m_pBufMem;
		    size_t			ulSize = This->m_frame_nbs * This->m_frame_size;

		    while (ulSize > 0) 
		    {
			    ClearPageReserved(vmalloc_to_page((void *)ulAddr));
			    ulAddr += PAGE_SIZE;
			    ulSize -= PAGE_SIZE;
		    }
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
		    vfree((void *)This->m_pBufMem);
		    This->m_pBufMem = ZOE_NULL;
	    }

        for (i = 0; i < ZV_AVLIB_MAX_DATA_ENTRIES; i++)
        {
            if (This->m_v4l2Buffers[i].m.planes)
            {
                vfree((void *)This->m_v4l2Buffers[i].m.planes);
                This->m_v4l2Buffers[i].m.planes = ZOE_NULL;
            }
        }
    }
    return (ZOE_ERRS_SUCCESS);
}


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
		pPort->m_pBufMem = ZOE_NULL;
		pPort->m_pQueueFree = ZOE_NULL;
		pPort->m_pQueueData = ZOE_NULL;
		pPort->m_pQueueLib = ZOE_NULL;
		pPort->m_pQueueUser = ZOE_NULL;
        pPort->m_free_queue_inited = ZOE_FALSE;
	}

	return (pPort);
}



// destructor
//
void c_port_destructor(c_port *This)
{
	if (This->m_pQueueUser)
	{
		c_queue_destructor(This->m_pQueueUser);
		vfree((void *)This->m_pQueueUser);
		This->m_pQueueUser = ZOE_NULL;
	}

	if (This->m_pQueueLib)
	{
		c_queue_destructor(This->m_pQueueLib);
		vfree((void *)This->m_pQueueLib);
		This->m_pQueueLib = ZOE_NULL;
	}

	if (This->m_pQueueData)
	{
		c_queue_destructor(This->m_pQueueData);
		vfree((void *)This->m_pQueueData);
		This->m_pQueueData = ZOE_NULL;
	}

	if (This->m_pQueueFree)
	{
		c_queue_destructor(This->m_pQueueFree);
		vfree((void *)This->m_pQueueFree);
		This->m_pQueueFree = ZOE_NULL;
	}

	if (This->m_pBufMem)
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
		unsigned long	ulAddr = (unsigned long)This->m_pBufMem;
		size_t			ulSize = This->m_frame_nbs * This->m_frame_size;

		while (ulSize > 0) 
		{
			ClearPageReserved(vmalloc_to_page((void *)ulAddr));
			ulAddr += PAGE_SIZE;
			ulSize -= PAGE_SIZE;
		}
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
		vfree((void *)This->m_pBufMem);
		This->m_pBufMem = ZOE_NULL;
	}

	This->m_valid = ZOE_FALSE;

	c_object_destructor(&This->m_Object);
}



zoe_errs_t c_port_create(c_port *This)
{
	c_queue		*pQueue;
	zoe_errs_t	err;
	i_zv_codec	*pMpegCodec = c_device_get_codec(This->m_pComponent->m_pDevice);
    int         i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s\n", 
				   __FUNCTION__
				   );

	if (PORT_DIR_NONE != This->m_dir)
	{
        init_waitqueue_head(&This->m_buf_ready_wq);

		// allocate data queues
		//

		// free queue 
		pQueue = (c_queue *)vmalloc(sizeof(c_queue));
		if (!pQueue)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create c_queue!\n", 
						   __FUNCTION__ 
						   );
			return (ZOE_ERRS_NOMEMORY);
		}

		This->m_pQueueFree = c_queue_constructor(pQueue,
												 &This->m_Object, 
												 OBJECT_CRITICAL_HEAVY,
                                                 ZOE_NULL,
                                                 ZOE_NULL,
                                                 g_ZVV4LDevDBGCompID
												 );
		if (!This->m_pQueueFree)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create m_pQueueFree!\n", 
						   __FUNCTION__ 
						   );
			vfree((void *)pQueue);
			return (ZOE_ERRS_FAIL);
		}

		// data queue
		pQueue = (c_queue *)vmalloc(sizeof(c_queue));
		if (!pQueue)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create c_queue!\n", 
						   __FUNCTION__ 
						   );
			return (ZOE_ERRS_NOMEMORY);
		}

		This->m_pQueueData = c_queue_constructor(pQueue,
												 &This->m_Object, 
												 OBJECT_CRITICAL_HEAVY,
                                                 ZOE_NULL,
                                                 ZOE_NULL,
                                                 g_ZVV4LDevDBGCompID
												 );
		if (!This->m_pQueueData)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create m_pQueueData!\n", 
						   __FUNCTION__ 
						   );
			vfree((void *)pQueue);
			return (ZOE_ERRS_FAIL);
		}

		// lib queue 
		pQueue = (c_queue *)vmalloc(sizeof(c_queue));
		if (!pQueue)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create c_queue!\n", 
						   __FUNCTION__ 
						   );
			return (ZOE_ERRS_NOMEMORY);
		}

		This->m_pQueueLib = c_queue_constructor(pQueue,
											    &This->m_Object, 
											    OBJECT_CRITICAL_HEAVY,
                                                ZOE_NULL,
                                                ZOE_NULL,
                                                g_ZVV4LDevDBGCompID
											    );
		if (!This->m_pQueueLib)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create m_pQueueLib!\n", 
						   __FUNCTION__ 
						   );
			vfree((void *)pQueue);
			return (ZOE_ERRS_FAIL);
		}

		// user queue 
		pQueue = (c_queue *)vmalloc(sizeof(c_queue));
		if (!pQueue)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create c_queue!\n", 
						   __FUNCTION__ 
						   );
			return (ZOE_ERRS_NOMEMORY);
		}

		This->m_pQueueUser = c_queue_constructor(pQueue,
												 &This->m_Object, 
												 OBJECT_CRITICAL_HEAVY,
                                                 ZOE_NULL,
                                                 ZOE_NULL,
                                                 g_ZVV4LDevDBGCompID
												 );
		if (!This->m_pQueueUser)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s Unable to create m_pQueueUser!\n", 
						   __FUNCTION__ 
						   );
			vfree((void *)pQueue);
			return (ZOE_ERRS_FAIL);
		}

        // allocate per v4l2 buffer multi-plane memory
        //
        for (i = 0; i < ZV_AVLIB_MAX_DATA_ENTRIES; i++)
        {
            This->m_v4l2Buffers[i].m.planes = (struct v4l2_plane *)vmalloc(ZV_MAX_BUF_PER_DESC * sizeof(struct v4l2_plane));
            if (!This->m_v4l2Buffers[i].m.planes)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                               g_ZVV4LDevDBGCompID,
						       "%s Unable to alloc planes buffer!\n", 
						       __FUNCTION__         
						       );
			    return (ZOE_ERRS_NOMEMORY);
            }
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
	int	i, j;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
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

	// flush queues
	//
	for (i = 0; i < This->m_frame_nbs; i++)
	{
        for (j = 0; j < ZV_MAX_BUF_PER_DESC; j++)
        {
		    if (This->m_BufDesces[i].DataBuffer[j].p_user_mappings)
		    {
			    vfree(This->m_BufDesces[i].DataBuffer[j].p_user_mappings);
			    This->m_BufDesces[i].DataBuffer[j].p_user_mappings = ZOE_NULL;
            }
        }
	}

	if (This->m_pQueueUser)
	{
		c_queue_destructor(This->m_pQueueUser);
		vfree((void *)This->m_pQueueUser);
		This->m_pQueueUser = ZOE_NULL;
	}

	if (This->m_pQueueLib)
	{
		c_queue_destructor(This->m_pQueueLib);
		vfree((void *)This->m_pQueueLib);
		This->m_pQueueLib = ZOE_NULL;
	}

	if (This->m_pQueueData)
	{
		c_queue_destructor(This->m_pQueueData);
		vfree((void *)This->m_pQueueData);
		This->m_pQueueData = ZOE_NULL;
	}

	if (This->m_pQueueFree)
	{
		c_queue_destructor(This->m_pQueueFree);
		vfree((void *)This->m_pQueueFree);
		This->m_pQueueFree = ZOE_NULL;
	}

	if (This->m_pBufMem)
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
		unsigned long	ulAddr = (unsigned long)This->m_pBufMem;
		size_t			ulSize = This->m_frame_nbs * This->m_frame_size;

		while (ulSize > 0) 
		{
			ClearPageReserved(vmalloc_to_page((void *)ulAddr));
			ulAddr += PAGE_SIZE;
			ulSize -= PAGE_SIZE;
		}
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
		vfree((void *)This->m_pBufMem);
		This->m_pBufMem = ZOE_NULL;
	}

    for (i = 0; i < ZV_AVLIB_MAX_DATA_ENTRIES; i++)
    {
        if (This->m_v4l2Buffers[i].m.planes)
        {
            vfree((void *)This->m_v4l2Buffers[i].m.planes);
            This->m_v4l2Buffers[i].m.planes = ZOE_NULL;
        }
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
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
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

			zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                           g_ZVV4LDevDBGCompID,
						   "%s PORT_STATE_STOP codec handle(%d)\n", 
						   __FUNCTION__,
						   This->m_hStreamLib
						   );

			if (ZOE_NULL_HANDLE != This->m_hStreamLib)
			{
				QUEUE_ENTRY	*pEntry;

				err = pMpegCodec->stop(pMpegCodec, 
									   This->m_hStreamLib
									   );

				// flush all the queues
				//
				if (This->m_pQueueLib)
				{
					while (pEntry = c_queue_get_one_entry(This->m_pQueueLib))
					{
						c_port_clean_up_data_request(This, 
												     (PPORT_DATA_REQ)pEntry->Data
												     );
						c_queue_add_entry(This->m_pQueueFree, 
										  pEntry
										  );
					}
				}

				if (This->m_pQueueData)
				{
					while (pEntry = c_queue_get_one_entry(This->m_pQueueData))
					{
						c_port_clean_up_data_request(This, 
												     (PPORT_DATA_REQ)pEntry->Data
												     );
						c_queue_add_entry(This->m_pQueueFree, 
										  pEntry
										  );
					}
				}

				if (This->m_pQueueUser)
				{
					while (pEntry = c_queue_get_one_entry(This->m_pQueueUser))
					{
						c_port_clean_up_data_request(This, 
												     (PPORT_DATA_REQ)pEntry->Data
												     );
						c_queue_add_entry(This->m_pQueueFree, 
										  pEntry
										  );
					}
				}
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

#ifndef MMAP_NEED_QBUF
			// pass buffers to the firmware
			//
			if (ZOE_SUCCESS(err))
			{
				c_port_submit_buffers(This);
			}
#endif //!MMAP_NEED_QBUF
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
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

    if (0 == reqbuf->count)
    {
        // free buffers
        err = c_port_done_free_queue(This);
        if (ZOE_SUCCESS(err))
        {
            This->m_frame_nbs = 0;
        }
    }
    else
    {
        // allocate and init queue
        if ((This->m_frame_nbs != reqbuf->count) ||
            (This->m_memoryType != reqbuf->memory)
            )
        {
            err = c_port_done_free_queue(This);
        }

        if (ZOE_SUCCESS(err))
        {
            This->m_frame_nbs = reqbuf->count;
            This->m_memoryType = reqbuf->memory;
            err = c_port_init_free_queue(This);
        }
    }

	return (err);
}



zoe_errs_t c_port_query_buf(c_port *This, 
                            struct file *file,
						    struct v4l2_buffer *buf
						    ) 
{
	zoe_errs_t	        err = ZOE_ERRS_SUCCESS;
    int			        index = buf->index;
    int			        type = buf->type;
    QUEUE_ENTRY         *p_entry;
    struct v4l2_buffer  *p_v4l2_buf = NULL;
    int                 i;

    if (!V4L2_TYPE_IS_MULTIPLANAR(type))
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s(%d)-> wrong buffer type(%d)\n", 
					   __FUNCTION__, 
					   This->m_id,
					   type
					   );
        err = ZOE_ERRS_INVALID;
    }
    else
    {
	    if (index < This->m_frame_nbs) 
	    {
            // find the v4l2 buffer
            //
            if (V4L2_MEMORY_MMAP == This->m_memoryType)
            {
                p_v4l2_buf = &This->m_v4l2Buffers[index];
            }
            else
            {
                // look in the lib queue
                p_entry = c_queue_peek_entry_by_v4l2_buf_id(This->m_pQueueLib, 
                                                            buf
                                                            );
                if (!p_entry)
                {
                    // look in the data queue
                    p_entry = c_queue_peek_entry_by_v4l2_buf_id(This->m_pQueueData, 
                                                                buf
                                                                );
                }
                if (p_entry)
                {
                    PPORT_DATA_REQ  p_data_req = (PPORT_DATA_REQ)p_entry->Data;
                    p_v4l2_buf = p_data_req->pV4L2Buf;
                }
            }

            if (p_v4l2_buf)
            {
		        memcpy(buf,
			           p_v4l2_buf,
			           offsetof(struct v4l2_buffer, m)
			           );
		        buf->index = index;
	            buf->length = p_v4l2_buf->length;

		        // keep the port type in 4 most significant bits 
                for (i = 0; i < p_v4l2_buf->length; i++)
                {
                    if (V4L2_MEMORY_MMAP == This->m_memoryType)
                    {
		                buf->m.planes[i].m.mem_offset = p_v4l2_buf->m.planes[i].m.mem_offset | (This->m_id << PORT_MMAP_BUF_TYPE_SHIFT);
                    }
                    else
                    {
		                buf->m.planes[i].m.userptr = p_v4l2_buf->m.planes[i].m.userptr;
                    }
                    buf->m.planes[i].data_offset = 0;
                    buf->m.planes[i].bytesused = 0;
                    buf->m.planes[i].length = p_v4l2_buf->m.planes[i].length;
                }
		        buf->type = type;

		        zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                               g_ZVV4LDevDBGCompID,
					           "%s(%d)-> idx=%d, addr=%x, size=%x\n", 
					           __FUNCTION__,
					           This->m_id,
					           buf->index, 
					           buf->m.offset, 
					           buf->length
					           );
            }
            else
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                               g_ZVV4LDevDBGCompID,
					           "%s(%d)-> idx=%d NOT FOUND\n", 
					           __FUNCTION__, 
					           This->m_id,
					           buf->index
					           );
		        err = ZOE_ERRS_PARMS;
            }
	    } 
	    else 
	    {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
					       "%s(%d)-> idx=%d >= frame_nb(%d)\n", 
					       __FUNCTION__, 
					       This->m_id,
					       buf->index,
					       This->m_frame_nbs
					       );
		    err = ZOE_ERRS_PARMS;
        }
    }

	return (err);
}



zoe_errs_t c_port_dqbuf(c_port *This, 
                        struct file *file,
					    struct v4l2_buffer *pv4lbufapp
					    ) 
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_queue		*pQueue = ZOE_NULL;
	QUEUE_ENTRY	*pEntry;
    int         i;

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s(%d)-> [\n", 
				   __FUNCTION__,
				   This->m_id
				   );

	if (V4L2_MEMORY_MMAP == This->m_memoryType)
	{
#ifdef MMAP_NEED_QBUF
		pQueue = This->m_pQueueData;
#else //!MMAP_NEED_QBUF
		if (This->m_State != PORT_STATE_RUN) 
		{
			return (ZOE_ERRS_AGAIN);
		}

		switch (This->m_dir)
		{
			case PORT_DIR_READ:
				pQueue = This->m_pQueueData;
				break;

			case PORT_DIR_WRITE:
				pQueue = This->m_pQueueFree;
				break;

			case PORT_DIR_NONE:
			default:
				return (ZOE_ERRS_NOTSUPP);
		}
#endif //MMAP_NEED_QBUF

        if (!pQueue)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
				           "%s(%d) NO QUEUE???\n", 
				           __FUNCTION__,
				           This->m_id
				           );
            return (ZOE_ERRS_INVALID);
        }

		// get one entry from the queue
		pEntry = c_queue_get_one_entry(pQueue);

		if (pEntry) 
		{
			PPORT_DATA_REQ		pDataReq = (PPORT_DATA_REQ)pEntry->Data;
			struct v4l2_buffer	*pV4L2Buf = pDataReq->pV4L2Buf;

            V4L2_BUFFER_USR(pV4L2Buf);
	        memcpy(pv4lbufapp,
                   pDataReq->pV4L2Buf, 
                   offsetof(struct v4l2_buffer, m)
                   );
			pv4lbufapp->length = pV4L2Buf->length;
	        pv4lbufapp->reserved2 = pDataReq->pV4L2Buf->reserved2;
			pv4lbufapp->reserved = pV4L2Buf->reserved;
            for (i = 0; i < pV4L2Buf->length; i++)
            {
		        pv4lbufapp->m.planes[i].m.mem_offset = pV4L2Buf->m.planes[i].m.mem_offset | (This->m_id << PORT_MMAP_BUF_TYPE_SHIFT);
                pv4lbufapp->m.planes[i].data_offset = pV4L2Buf->m.planes[i].data_offset;
                pv4lbufapp->m.planes[i].bytesused = pV4L2Buf->m.planes[i].bytesused;
                pv4lbufapp->m.planes[i].length = pV4L2Buf->m.planes[i].length;
            }
			// move entry to the user queue
			c_queue_add_entry(This->m_pQueueUser, 
							  pEntry
							  );

            // wake up the event for poll/select if queue is not empty
            if (!c_queue_is_empty(pQueue))
            {
                wake_up(&This->m_buf_ready_wq);
            }
		} 
		else 
		{
			err = ZOE_ERRS_AGAIN;
		}
	}
	else
	{
        if (!This->m_pQueueData)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
				           "%s(%d) NO QUEUE???\n", 
				           __FUNCTION__,
				           This->m_id
				           );
            return (ZOE_ERRS_INVALID);
        }

		// get one entry from the data queue
		pEntry = c_queue_get_one_entry(This->m_pQueueData);

		if (pEntry) 
		{
			PPORT_DATA_REQ		pDataReq = (PPORT_DATA_REQ)pEntry->Data;
			struct v4l2_buffer	*pV4L2Buf = pDataReq->pV4L2Buf;

            V4L2_BUFFER_USR(pV4L2Buf);
	        memcpy(pv4lbufapp,
                   pDataReq->pV4L2Buf, 
                   offsetof(struct v4l2_buffer, m)
                   );
			pv4lbufapp->length = pV4L2Buf->length;
	        pv4lbufapp->reserved2 = pDataReq->pV4L2Buf->reserved2;
			pv4lbufapp->reserved = pV4L2Buf->reserved;
			memcpy(pv4lbufapp->m.planes,
                   pDataReq->pV4L2Buf->m.planes,
				   pV4L2Buf->length * sizeof(struct v4l2_plane)
				   );

			// move entry back to the free queue
			c_queue_add_entry(This->m_pQueueFree, 
							  pEntry
							  );

            // wake up the event for poll/select if queue is not empty
            if (!c_queue_is_empty(This->m_pQueueData))
            {
                wake_up(&This->m_buf_ready_wq);
            }
		} 
		else 
		{
			err = ZOE_ERRS_AGAIN;
		}
	}

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s(%d)<- status(%d) buf index(%d) addr(0x%x) len(%d) used(%d) ]\n", 
				   __FUNCTION__, 
				   This->m_id, 
				   err,
				   pv4lbufapp->index,
				   pv4lbufapp->m.offset, 
				   pv4lbufapp->length,
				   pv4lbufapp->bytesused
				   );

	return (err);
}



zoe_errs_t c_port_qbuf(c_port *This, 
                       struct file *file,
					   struct v4l2_buffer *pv4lbufapp
					   ) 
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	QUEUE_ENTRY	*pEntry;
    int         i;

	if (PORT_DIR_NONE == This->m_dir)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s(%d)-> no direction(%d)\n", 
					   __FUNCTION__, 
					   This->m_id,
					   This->m_dir
					   );
		return (ZOE_ERRS_NOTSUPP);
	}

    if (!V4L2_TYPE_IS_MULTIPLANAR(pv4lbufapp->type))
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s(%d)-> wrong buffer type(%d)\n", 
					   __FUNCTION__, 
					   This->m_id,
					   pv4lbufapp->type
					   );
        return (ZOE_ERRS_INVALID);
    }

	if (V4L2_MEMORY_MMAP == This->m_memoryType)
	{
#ifdef MMAP_NEED_QBUF
        c_queue *p_queue_ret = NULL;

		pEntry = c_queue_get_entry_by_v4l2_buf(This->m_pQueueUser, 
										       pv4lbufapp
										       );
        if (!pEntry)
        {
		    pEntry = c_queue_get_entry_by_v4l2_buf(This->m_pQueueFree, 
										           pv4lbufapp
										           );
            if (pEntry)
            {
                p_queue_ret = This->m_pQueueFree;
            }
        }
        else
        {
            p_queue_ret = This->m_pQueueUser;
        }
		if (pEntry)
		{
			PPORT_DATA_REQ	pDataReq = (PPORT_DATA_REQ)pEntry->Data;

			// copy user v4l buffer structure
			//
	        memcpy(pDataReq->pV4L2Buf, 
                   pv4lbufapp, 
                   offsetof(struct v4l2_buffer, m)
                   );
            V4L2_BUFFER_QUEUED(pDataReq->pV4L2Buf);
	        pDataReq->pV4L2Buf->length = pv4lbufapp->length;
	        pDataReq->pV4L2Buf->reserved2 = pv4lbufapp->reserved2;
	        pDataReq->pV4L2Buf->reserved = pv4lbufapp->reserved;
            for (i = 0; i < pDataReq->pV4L2Buf->length; i++)
            {
                pDataReq->pV4L2Buf->m.planes[i].data_offset = pv4lbufapp->m.planes[i].data_offset;
                pDataReq->pV4L2Buf->m.planes[i].bytesused = pv4lbufapp->m.planes[i].bytesused;
            }

            // send the buffer to the codec library
            //
            err = c_port_submit_buffer(This, 
                                       pEntry, 
                                       p_queue_ret
                                       );
		} 
#else //!MMAP_NEED_QBUF
		pEntry = c_queue_get_entry_by_v4l2_buf(This->m_pQueueUser, 
										       pv4lbufapp
										       );
		if (pEntry)
		{
			PPORT_DATA_REQ	pDataReq = (PPORT_DATA_REQ)pEntry->Data;

			if (PORT_DIR_READ == This->m_dir)
			{
				pDataReq->pV4L2Buf->bytesused = 0;
				pDataReq->pV4L2Buf->reserved = 0;
				pDataReq->pV4L2Buf->flags = 0;
                V4L2_BUFFER_QUEUED(pDataReq->pV4L2Buf);
                for (i = 0; i < pDataReq->pV4L2Buf->length; i++)
                {
                    pDataReq->pV4L2Buf->m.planes[i].data_offset = 0;
                    pDataReq->pV4L2Buf->m.planes[i].bytesused = 0;
                }
				c_queue_add_entry(This->m_pQueueFree,
								  pEntry
								  );
			}
			else
			{
				pDataReq->pV4L2Buf->bytesused = pv4lbufapp->bytesused;
				pDataReq->pV4L2Buf->reserved = pv4lbufapp->reserved;
				pDataReq->pV4L2Buf->flags = pv4lbufapp->flags;
                V4L2_BUFFER_QUEUED(pDataReq->pV4L2Buf);
			    pDataReq->pV4L2Buf->field = pv4lbufapp->field;
                memcpy(&pDataReq->pV4L2Buf->timestamp, &pv4lbufapp->timestamp, sizeof(struct timeval));
                for (i = 0; i < pDataReq->pV4L2Buf->length; i++)
                {
                    pDataReq->pV4L2Buf->m.planes[i].data_offset = pv4lbufapp->m.planes[i].data_offset;
                    pDataReq->pV4L2Buf->m.planes[i].bytesused = pv4lbufapp->m.planes[i].bytesused;
                }
				c_queue_add_entry(This->m_pQueueData,
								  pEntry
								  );
			}

			// push data through
			c_port_submit_buffers(This);
		} 
#endif //MMAP_NEED_QBUF
		else 
		{
			err = ZOE_ERRS_FAIL;
		}
	}
	else
	{
		if ((ZOE_NULL_HANDLE == This->m_hStreamLib) ||
			(PORT_STATE_STOP == This->m_State) ||
			(PORT_DIR_NONE == This->m_dir)
			)
		{
			return (ZOE_ERRS_AGAIN);
		}

		// get a free queue entry
		//
		pEntry = c_queue_get_one_entry(This->m_pQueueFree);

		if (pEntry)
		{
			PPORT_DATA_REQ	pDataReq = (PPORT_DATA_REQ)pEntry->Data;

			// copy user v4l buffer structure
			//
	        memcpy(pDataReq->pV4L2Buf, 
                   pv4lbufapp, 
                   offsetof(struct v4l2_buffer, m)
                   );
            V4L2_BUFFER_QUEUED(pDataReq->pV4L2Buf);
	        pDataReq->pV4L2Buf->length = pv4lbufapp->length;
	        pDataReq->pV4L2Buf->reserved2 = pv4lbufapp->reserved2;
	        pDataReq->pV4L2Buf->reserved = pv4lbufapp->reserved;

			memcpy(pDataReq->pV4L2Buf->m.planes,
				   pv4lbufapp->m.planes,
				   pv4lbufapp->length * sizeof(struct v4l2_plane)
				   );
            // send the buffer to the codec library
            //
            err = c_port_submit_buffer(This, 
                                       pEntry, 
                                       This->m_pQueueFree
                                       );
		}
		else 
		{
			err = ZOE_ERRS_NOMEMORY;
		}
	}

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s(%d)-> buf index(%d) addr(0x%x) used(%d) status(%d)\n", 
				   __FUNCTION__,
				   This->m_id,
				   pv4lbufapp->index,
				   pv4lbufapp->m.offset,
				   pv4lbufapp->bytesused,
				   err
				   );
	return (err);
}



zoe_bool_t c_port_buf_rdy(c_port *This,
                          struct file *file,
                          poll_table *wait
                          )
{
    c_queue  *pQueue;

    poll_wait(file, &This->m_buf_ready_wq, wait);

#ifndef MMAP_NEED_QBUF
	if (V4L2_MEMORY_MMAP == This->m_memoryType)
	{
		if (This->m_State != PORT_STATE_RUN) 
		{
			return (ZOE_FALSE);
		}
		switch (This->m_dir)
		{
			case PORT_DIR_READ:
				pQueue = This->m_pQueueData;
				break;

			case PORT_DIR_WRITE:
				pQueue = This->m_pQueueFree;
				break;

			case PORT_DIR_NONE:
			default:
				return (ZOE_FALSE);
		}
	}
	else
	{
#endif //!MMAP_NEED_QBUF
        if (PORT_DIR_NONE == This->m_dir)
        {
			return (ZOE_FALSE);
        }
        else
        {
		    pQueue = This->m_pQueueData;
        }
#ifndef MMAP_NEED_QBUF
	}
#endif //!MMAP_NEED_QBUF

    return (0 != c_queue_get_queue_level(pQueue));
}









