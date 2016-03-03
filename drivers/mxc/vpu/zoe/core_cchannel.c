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
// cchannel.c
//
// Description: 
//
//	channels represent the data streaming on the VPU
//
// Authors: (dt) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "cchannel.h"
#include "czvcodec.h"
#include "zoe_sosal.h"
#include "zoe_cqueue.h"
#include "zoe_util.h"
#include "zoe_objids.h"
#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#else //ZOE_LINUXKER_BUILD
#include <linux/string.h>
#endif //!ZOE_LINUXKER_BUILD
#include "ctask.h"



/////////////////////////////////////////////////////////////////////////////
//
//

// return the entry with matching buffer pointer
//
static QUEUE_ENTRY* c_queue_get_entry_by_buf_ptr(c_queue *This, 
                                                 zoe_void_ptr_t p_buffer
                                                 )
{
    QUEUE_ENTRY             *pthis_queue;
    QUEUE_ENTRY             *pprev_queue;
    PZV_BUFFER_DESCRIPTOR   pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;

    ENTER_CRITICAL(&This->m_Object)

    pthis_queue = This->m_Queue.pHead;
    pprev_queue = ZOE_NULL;

    while (pthis_queue)
    {
        pBufDesc = (PZV_BUFFER_DESCRIPTOR)pthis_queue->Data;
		pDataPacket = &pBufDesc->DataBuffer[0];

        if (pDataPacket->Data == p_buffer)
	    {
            if (!pprev_queue)
            {
                This->m_Queue.pHead = pthis_queue->pNext;
                if (!This->m_Queue.pHead)
                {
                    This->m_Queue.pTail = This->m_Queue.pHead;
                }
            }
            else
            {
                pprev_queue->pNext = pthis_queue->pNext;
                if (!pthis_queue->pNext)
                {
                    This->m_Queue.pTail = pprev_queue;
                }
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



// peek the entry data(buffer descriptor) with matching buffer pointer
//
static PZV_BUFFER_DESCRIPTOR c_queue_peek_buf_desc_by_buf_ptr(c_queue *This, 
                                                              zoe_void_ptr_t p_buffer
                                                              )
{
    QUEUE_ENTRY             *pthis_queue;
    QUEUE_ENTRY             *pprev_queue;
    PZV_BUFFER_DESCRIPTOR   pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;

    ENTER_CRITICAL(&This->m_Object)

    pthis_queue = This->m_Queue.pHead;
    pprev_queue = ZOE_NULL;

    while (pthis_queue)
    {
        pBufDesc = (PZV_BUFFER_DESCRIPTOR)pthis_queue->Data;
		pDataPacket = &pBufDesc->DataBuffer[0];

        if (pDataPacket->Data == p_buffer)
	    {
            break;
        }
        pprev_queue = pthis_queue;
        pthis_queue = pthis_queue->pNext;
    }
    LEAVE_CRITICAL(&This->m_Object)

    return (pthis_queue ? pBufDesc : ZOE_NULL);
}



// display buffer pointer
//
static void c_queue_print_buf_ptr(c_queue *This)
{
    QUEUE_ENTRY             *pthis_queue;
    QUEUE_ENTRY             *pprev_queue;
    PZV_BUFFER_DESCRIPTOR   pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;
    uint32_t                i;

    ENTER_CRITICAL(&This->m_Object)

    pthis_queue = This->m_Queue.pHead;
    pprev_queue = ZOE_NULL;

    while (pthis_queue)
    {
        pBufDesc = (PZV_BUFFER_DESCRIPTOR)pthis_queue->Data;
        for (i = 0; i < pBufDesc->NumberOfBuffers; i++)
        {
		    pDataPacket = &pBufDesc->DataBuffer[i];

		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
					       "(%p:%d) ",
                           pDataPacket->Data,
                           pDataPacket->BufferSize
					       );
        }
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
				   "\n"
				   );
        pprev_queue = pthis_queue;
        pthis_queue = pthis_queue->pNext;
    }
    LEAVE_CRITICAL(&This->m_Object)
}


/////////////////////////////////////////////////////////////////////////////
//
//


static zoe_bool_t c_channel_recycle_entry(c_channel *This,
									      QUEUE_ENTRY *pEntry, 
									      zoe_bool_t bCancel
									      )
{
	PZV_BUFFER_DESCRIPTOR	pBufDesc;

	if (pEntry)
	{
		if (bCancel)
		{
			if (!This->m_pTask->cancel_buffer(This->m_pTask, 
											  This->m_ChannelType,
											  pEntry->Data
											  ))
			{
				return (ZOE_FALSE);
			}
		}

		pBufDesc = (PZV_BUFFER_DESCRIPTOR)pEntry->Data;
		pEntry->Data = ZOE_NULL;
		pBufDesc->Status = ZOE_ERRS_SUCCESS/*ZOE_ERRS_CANCELLED*/;

		// recycle buffer entry
		//
		c_queue_add_entry(This->m_pFreeQueue,
						  pEntry
						  );

		// call back the interface driver
		//
		c_channel_device_callback(This,
								  ZVCODEC_CMD_DONE_DATA,
								  pBufDesc							
								  );
	}

	return (ZOE_TRUE);
}



// find the buffer descriptor with matching buffer pointer
//
PZV_BUFFER_DESCRIPTOR c_channel_find_buf_desc_by_buf_ptr(c_channel *This, 
                                                         zoe_void_ptr_t p_buffer
                                                         )
{
    PZV_BUFFER_DESCRIPTOR   p_buf_desc = c_queue_peek_buf_desc_by_buf_ptr(This->m_pDataPendingQueue, 
                                                                          p_buffer
                                                                          );
    if (!p_buf_desc)
    {
        c_queue_print_buf_ptr(This->m_pDataPendingQueue);
    }

    return (p_buf_desc);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_channel
//
static zoe_errs_t c_channel_open(c_channel *This,
								 ZOE_OBJECT_HANDLE hChannel,
								 uint32_t dwFlags,
								 zoe_void_ptr_t pDataFormat,
								 ZV_DEVICE_CALLBACK pFuncCallback,
								 zoe_void_ptr_t context
								 )
{
	int			i;
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_channel_open()\n"
				   );

	// save handle
	//
	This->m_hChannel = hChannel;

	// save the open flags
	//
	This->m_dwOpenFlags = dwFlags;

	// save callback address and context
	//
	This->m_pDeviceCallback = pFuncCallback;
	This->m_callbackContext = context;

	if (!This->m_pFreeQueue || 
		!This->m_pDataRequestQueue ||
		!This->m_pDataPendingQueue
		)
	{
		return (ZOE_ERRS_NOMEMORY);
	}

	// make sure there is no residule in the queues
	//
	c_queue_flush_queue(This->m_pFreeQueue);
	c_queue_flush_queue(This->m_pDataRequestQueue);
	c_queue_flush_queue(This->m_pDataPendingQueue);

	// initialize free queue
	//
	for (i = 0; i < ZV_AVLIB_MAX_DATA_ENTRIES; i++)
	{
		This->m_Entries[i].Data = ZOE_NULL;
		c_queue_add_entry(This->m_pFreeQueue, 
						  &This->m_Entries[i]
						  );
	}

	// reset channel state
	//
	This->m_State = ZVSTATE_STOP;
	This->m_bPaused = ZOE_FALSE;

	// clear flush flag
	//
	This->m_bFlushing = ZOE_FALSE;

	// reset counters and flags
	//
	This->m_ullCntBytes = 0;

	// yes, we are open
	//
	This->m_bOpened = ZOE_TRUE;

	// Task open
	//
	err = This->m_pTask->open(This->m_pTask, 
							  This->m_ChannelType,
							  This->m_ChannelDirection,
							  This
							  );
	if (!ZOE_SUCCESS(err))
    {
		This->close(This);
    }

	return (err);
}



static zoe_errs_t c_channel_close(c_channel *This)
{
	if (This->m_bOpened)
	{
		if (ZVSTATE_RUN == This->m_State)
		{
			This->pause(This);
		}

		if (ZVSTATE_STOP != This->m_State)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
						   "c_channel_close() !!! Not Stopped State (%d)!!!!\n", 
						   This->m_State
						   );
			This->stop(This);
		}

		This->flush(This);

		// task close
		//
		This->m_pTask->close(This->m_pTask, 
							 This->m_ChannelType,
                             ZOE_TRUE
							 );

		This->m_bOpened = ZOE_FALSE;
	}

	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_start(c_channel *This)
{
	This->m_State = ZVSTATE_RUN;
	if (This->m_bPaused)
    {
		This->m_bPaused = ZOE_FALSE;
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_stop(c_channel *This)
{
	This->m_bPaused = ZOE_FALSE;
	This->m_State = ZVSTATE_STOP;
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_acquire(c_channel *This)
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_channel_acquire()\n"
				   );

	err = This->m_pTask->acquire(This->m_pTask, 
								 This->m_ChannelType
								 );
	if (ZOE_SUCCESS(err))
	{
		This->m_State = ZVSTATE_ACQUIRE;
	}

	return (err);
}



static zoe_errs_t c_channel_pause(c_channel *This)
{
	if (ZVSTATE_RUN == This->m_State)
    {
		This->m_bPaused = ZOE_TRUE;
    }
	This->m_State = ZVSTATE_PAUSE;
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_set_rate(c_channel *This,
								     int32_t lNewRate
								     ) 
{
	return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t c_channel_get_rate(c_channel *This,
								     int32_t * plRate
								     ) 
{
	return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t c_channel_begin_flush(c_channel *This)
{
	This->m_bFlushing = ZOE_TRUE;
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_flush(c_channel *This)
{
	QUEUE_ENTRY	*pEntry;

	zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_channel_flush()[\n"
				   );

	// let the task know we are flushing
	//
	This->m_pTask->flush(This->m_pTask, 
						 This->m_ChannelType
						 );
	
	while (ZOE_NULL != (pEntry = c_queue_get_one_entry(This->m_pDataPendingQueue)))
	{
		if (!c_channel_recycle_entry(This,
								     pEntry,
								     ZOE_TRUE
								     ))
		{
			c_queue_add_entry(This->m_pDataPendingQueue, 
							  pEntry
							  );

			return (ZOE_ERRS_INVALID);
		}
	}

	while (ZOE_NULL != (pEntry = c_queue_get_one_entry(This->m_pDataRequestQueue)))
	{
		c_channel_recycle_entry(This,
							    pEntry,
							    ZOE_FALSE
							    );
	}

	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_end_flush(c_channel *This)
{
	This->m_bFlushing = ZOE_FALSE;
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_channel_add_buffer(c_channel *This,
									   PZV_BUFFER_DESCRIPTOR pBufDesc
									   )
{
	zoe_errs_t	err = ZOE_ERRS_FAIL;
	QUEUE_ENTRY	*pEntry;

	if (pBufDesc)
	{
		if (This->m_bFlushing)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
						   "c_channel_add_buffer() FLUSHING!\n"
						   );
			pBufDesc->Status = ZOE_ERRS_SUCCESS/*ZOE_ERRS_CANCELLED*/;
			return (ZOE_ERRS_SUCCESS/*ZOE_ERRS_CANCELLED*/);
		}

		zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                       This->m_dbgID,
					   "c_channel_add_buffer() (%d)\n", 
					   This->m_hChannel
					   );

		pEntry = c_queue_get_one_entry(This->m_pFreeQueue);

		if (!pEntry)
		{
			pBufDesc->Status = ZOE_ERRS_NOMEMORY;
			err = ZOE_ERRS_NOMEMORY;

			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
						   "c_channel_add_buffer() Failed No entry available for this buffer!\n"
						   );
		}
		else
		{
			pBufDesc->Status = ZOE_ERRS_PENDING;
			err = ZOE_ERRS_SUCCESS;

			pEntry->Data = (zoe_void_ptr_t)pBufDesc;
			c_queue_add_entry(This->m_pDataRequestQueue,
							  pEntry
							  );

			// inform task of the arrival of this buffer
			//
		    zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                           This->m_dbgID,
						   "c_channel_add_buffer() new_buffer\n"
						   );

			// don't touch the buffer descriptor after new_buffer to prevent a 
            // higher priority thread to complete the buffer before the addBuffer() 
            // return.
			//
			This->m_pTask->new_buffer(This->m_pTask, 
							          This->m_ChannelType
									  );
		}
	}

	return (err);
}



static zoe_errs_t c_channel_cancel_buffer(c_channel *This,
										  PZV_BUFFER_DESCRIPTOR pBufDesc, 
										  zoe_bool_t bTimeOut
										  )
{
	QUEUE_ENTRY	*pEntry;

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   This->m_dbgID,
				   "c_channel_cancel_buffer() %s (0x%x)\n",
				   bTimeOut ? "Time Out" : "Cancel",
				   pBufDesc
				   );

	pEntry = c_queue_get_entry_by_data(This->m_pDataRequestQueue, 
								       pBufDesc
								       );

	if (!pEntry)
	{
		if (c_queue_peek_entry_by_data(This->m_pDataPendingQueue, 
								       pBufDesc
								       ))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
						   "c_channel_cancel_buffer()(id : %d) ::%s - in pending queue try cancel\n",
						   This->m_hChannel,
						   bTimeOut ? "Time Out" : "Cancel"
						   );
			if (!This->m_pTask->cancel_buffer(This->m_pTask,
											  This->m_ChannelType,
											  pBufDesc
											  ))
			{
				return (ZOE_ERRS_FAIL);
			}
			else
			{
				return (ZOE_ERRS_SUCCESS);
			}
		}
	}

	if (pEntry)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
					   "c_channel_cancel_buffer()(id : %d) %s - GOT IT\n",
					   This->m_hChannel,
					   bTimeOut ? "Time Out" : "Cancel"
					   );
		c_channel_recycle_entry(This, 
							    pEntry,
							    ZOE_FALSE
							    );
		return (ZOE_ERRS_SUCCESS);
	}

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   This->m_dbgID,
				   "c_channel_cancel_buffer()(id : %d) %s - NOT FOUND\n",
				   This->m_hChannel,
				   bTimeOut ? "Time Out" : "Cancel"
				   );

	return (ZOE_ERRS_NOTFOUND);
}



static zoe_errs_t c_channel_timeout_buffer(c_channel *This,
										 PZV_BUFFER_DESCRIPTOR pBufDesc
										 )
{
	return (c_channel_cancel_buffer(This,
								    pBufDesc, 
								    ZOE_TRUE
								    ));
}



// called from streaming task
//
static zoe_bool_t c_channel_get_buffer(c_channel *This,
								       PZV_BUFFER_DESCRIPTOR *ppBufDesc,
								       uint8_t * *ppBuffer,
								       uint32_t * pSize
								       ) 
{
	return (ZOE_FALSE);
}



static zoe_bool_t c_channel_get_buffer_yuv(c_channel *This,
										   PZV_BUFFER_DESCRIPTOR *ppBufDesc,
										   uint8_t * *ppYBuffer,
										   uint32_t * pYSize, 
										   uint8_t * *ppUVBuffer, 
										   uint32_t * pUVSize
										   ) 
{
	return (ZOE_FALSE);
}



static void c_channel_complete_buffer(c_channel *This,
									  PZV_BUFFER_DESCRIPTOR pBufDesc
									  ) 
{
}


// call back helper to the interface driver
//
zoe_errs_t c_channel_device_callback(c_channel *This,
 								     uint32_t dwCode,
 				   				     zoe_void_ptr_t pParam
 								     )
{
	if (This->m_bOpened &&
		This->m_pDeviceCallback && 
		This->m_callbackContext
		) 
	{
		return (This->m_pDeviceCallback(This->m_callbackContext, 
										dwCode, 
										pParam
										));
	}

	return (ZOE_ERRS_FAIL);
}



static zoe_errs_t c_channel_get_resolution(c_channel *This,
										   uint32_t * pWidth,
										   uint32_t * pHeight
										   )
{
	return (ZOE_ERRS_NOTSUPP);
}



static zoe_errs_t c_channel_get_yuv_format(c_channel *This,
										   uint32_t * pYUVFormat
										   )
{
	return (ZOE_ERRS_NOTSUPP);
}



// helper
//
ZVSTATE c_channel_get_state(c_channel *This) 
{
	return (This->m_State);
}



zoe_bool_t c_channel_is_opened(c_channel *This) 
{
	return (This->m_bOpened);
}



zoe_bool_t c_channel_need_byte_swapping(c_channel *This)
{
	return (This->m_bByteSwap);
}



zoe_bool_t c_channel_is_yuv(c_channel *This)
{
	return (This->m_bYUV);
}


c_channel * c_channel_constructor(c_channel *pChannel,
								  c_object *pParent,
								  uint32_t dwAttributes,
								  CHANNEL_DIRECTION ChannelDirection,
								  ZV_CODEC_OPEN_TYPE ChannelType,
								  c_task *pTask,
                                  IZOEHALAPI *pHal,
                                  zoe_dbg_comp_id_t dbgID
								  )
{
	if (pChannel)
	{
		c_queue	*pQueue;

        // zero init 
        //
		memset((void *)pChannel,
			   0,
			   sizeof(c_channel)
			   );

		// c_object
		//
		c_object_constructor(&pChannel->m_Object, 
							 pParent, 
                             OBJECT_ZOE_CHANNEL,
		  					 dwAttributes
		  					 );
		// fill in the function table
		//
		pChannel->open = c_channel_open;
		pChannel->close = c_channel_close;
		pChannel->start = c_channel_start;
		pChannel->stop = c_channel_stop;
		pChannel->acquire = c_channel_acquire;
		pChannel->pause = c_channel_pause;
		pChannel->set_rate = c_channel_set_rate;
		pChannel->get_rate = c_channel_get_rate;
		pChannel->begin_flush = c_channel_begin_flush;
		pChannel->flush = c_channel_flush;
		pChannel->end_flush = c_channel_end_flush;
		pChannel->add_buffer = c_channel_add_buffer;
		pChannel->cancel_buffer = c_channel_cancel_buffer;
		pChannel->timeout_buffer = c_channel_timeout_buffer;
		pChannel->get_buffer = c_channel_get_buffer;
		pChannel->get_buffer_yuv = c_channel_get_buffer_yuv;
		pChannel->complete_buffer = c_channel_complete_buffer;
		pChannel->get_resolution = c_channel_get_resolution;
		pChannel->get_yuv_format = c_channel_get_yuv_format;

		// initialize members
		//
        pChannel->m_pHal = pHal;
		pChannel->m_dwOpenFlags = 0;

		pChannel->m_pDeviceCallback = ZOE_NULL;
		pChannel->m_callbackContext = ZOE_NULL;

		pChannel->m_hChannel = ZOE_NULL_HANDLE;
		pChannel->m_ChannelDirection = ChannelDirection;
		pChannel->m_ChannelType = ChannelType;
		pChannel->m_pTask = pTask;

		pChannel->m_bOpened = ZOE_FALSE;
		pChannel->m_State = ZVSTATE_STOP;	// channel state
		pChannel->m_bPaused = ZOE_FALSE;
		pChannel->m_bFlushing = ZOE_FALSE;

		pChannel->m_bByteSwap = ZOE_FALSE;
		
		pChannel->m_ullCntBytes = 0;

		// buffer management stuffs
		//
		pChannel->m_pFreeQueue = ZOE_NULL;
		pChannel->m_pDataRequestQueue = ZOE_NULL;
		pChannel->m_pDataPendingQueue = ZOE_NULL;

		pChannel->m_llStartTime = 0;

        pChannel->m_bYUV = ZOE_FALSE;

		// create queues
		//
		if (ZV_CODEC_VIRTUAL != ChannelType)
		{
			// free queue
			//
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                       sizeof(c_queue), 
                                                       sizeof(void *)
                                                       );
			if (!pQueue)
			{
				c_channel_destructor(pChannel);
				return (ZOE_NULL);
			}

			pChannel->m_pFreeQueue = c_queue_constructor(pQueue,
														 &pChannel->m_Object, 
														 OBJECT_CRITICAL_LIGHT,
                                                         ZOE_NULL,
                                                         ZOE_NULL,
                                                         dbgID
														 );
			if (!pChannel->m_pFreeQueue)
			{
			    zoe_sosal_memory_free((void *)pQueue);
				c_channel_destructor(pChannel);
				return (ZOE_NULL);
			}

			// request queue
			//
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                       sizeof(c_queue), 
                                                       sizeof(void *)
                                                       );
			if (!pQueue)
			{
				c_channel_destructor(pChannel);
				return (ZOE_NULL);
			}

			pChannel->m_pDataRequestQueue = c_queue_constructor(pQueue,
															    &pChannel->m_Object, 
															    OBJECT_CRITICAL_LIGHT,
                                                                ZOE_NULL,
                                                                ZOE_NULL,
                                                                dbgID
															    );
			if (!pChannel->m_pDataRequestQueue)
			{
			    zoe_sosal_memory_free((void *)pQueue);
				c_channel_destructor(pChannel);
				return (ZOE_NULL);
			}

			// pending queue
			//
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                       sizeof(c_queue), 
                                                       sizeof(void *)
                                                       );
			if (!pQueue)
			{
				c_channel_destructor(pChannel);
				return (ZOE_NULL);
			}

			pChannel->m_pDataPendingQueue = c_queue_constructor(pQueue,
															    &pChannel->m_Object, 
															    OBJECT_CRITICAL_LIGHT,
                                                                ZOE_NULL,
                                                                ZOE_NULL,
                                                                dbgID
															    );
			if (!pChannel->m_pDataPendingQueue)
			{
			    zoe_sosal_memory_free((void *)pQueue);
				c_channel_destructor(pChannel);
				return (ZOE_NULL);
			}
		}
	}

	return (pChannel);
}



void c_channel_destructor(c_channel *This)
{
	if (This->m_bOpened)
		This->close(This);

	if (This->m_pDataPendingQueue)
	{
		c_queue_destructor(This->m_pDataPendingQueue);
		zoe_sosal_memory_free((void *)This->m_pDataPendingQueue);
		This->m_pDataPendingQueue = ZOE_NULL;
	}

	if (This->m_pDataRequestQueue)
	{
		c_queue_destructor(This->m_pDataRequestQueue);
		zoe_sosal_memory_free((void *)This->m_pDataRequestQueue);
		This->m_pDataRequestQueue = ZOE_NULL;
	}

	if (This->m_pFreeQueue)
	{
		c_queue_destructor(This->m_pFreeQueue);
		zoe_sosal_memory_free((void *)This->m_pFreeQueue);
		This->m_pFreeQueue = ZOE_NULL;
	}

	c_object_destructor(&This->m_Object);
}




/////////////////////////////////////////////////////////////////////////////
//
//

// c_null_channel
//

// c_channel
//
static zoe_errs_t c_null_channel_open(c_channel *This,
									  ZOE_OBJECT_HANDLE hChannel,
									  uint32_t dwFlags,
									  zoe_void_ptr_t pDataFormat,
									  ZV_DEVICE_CALLBACK pFuncCallback,
									  zoe_void_ptr_t context
									  ) 
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_null_channel_open()\n"
				   );

	// save handle
	//
	This->m_hChannel = hChannel;

	// save the open flags
	//
	This->m_dwOpenFlags = dwFlags;

	// save callback address and context
	//
	This->m_pDeviceCallback = pFuncCallback;
	This->m_callbackContext = context;

	// reset channel state
	//
	This->m_State = ZVSTATE_STOP;
	This->m_bPaused = ZOE_FALSE;

	// clear flush flag
	//
	This->m_bFlushing = ZOE_FALSE;

	// reset counters and flags
	//
	This->m_ullCntBytes = 0;

	// yes, we are open
	//
	This->m_bOpened = ZOE_TRUE;

	// Task open
	//
	err = This->m_pTask->open(This->m_pTask, 
							  This->m_ChannelType,
							  This->m_ChannelDirection,
							  This
							  );
	if (!ZOE_SUCCESS(err))
    {
		This->close(This);
    }

	return (err);
}


static zoe_errs_t c_null_channel_close(c_channel *This) 
{
	if (This->m_bOpened)
	{
		if (ZVSTATE_STOP != This->m_State)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
						   "c_null_channel_close() !!! Not Stopped State (%d)!!!!\n", 
						   This->m_State
						   );
			This->stop(This);
		}

		// task close
		//
		This->m_pTask->close(This->m_pTask, 
							 This->m_ChannelType,
                             ZOE_TRUE
							 );

		This->m_bOpened = ZOE_FALSE;
	}

	return (ZOE_ERRS_SUCCESS);
}


static zoe_errs_t c_null_channel_start(c_channel *This_p) 
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_null_channel_start() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (This_p->m_bPaused)
	{
		err = This_p->m_pTask->resume(This_p->m_pTask, 
									  This_p->m_ChannelType
									  );

		if (ZOE_SUCCESS(err))
        {
			This_p->m_bPaused = ZOE_FALSE;
        }
	}
	else
	{
		// start the task
		//
		err = This_p->m_pTask->start(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );
	}

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_RUN;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_null_channel_start() Failed Status(%d)\n", 
					   err
					   );
	}

	return (err);
}



static zoe_errs_t c_null_channel_stop(c_channel *This_p) 
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_null_channel_stop() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	err = This_p->m_pTask->stop(This_p->m_pTask, 
								This_p->m_ChannelType
								);
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_null_channel_stop() m_pTask->stop failed(%d)\n",
					   err
					   );
	}

	// clear falgs
	//
	This_p->m_bPaused = ZOE_FALSE;

	// change state to stop
	//
	This_p->m_State = ZVSTATE_STOP;

	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_null_channel_pause(c_channel *This_p) 
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_null_channel_pause() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (ZVSTATE_RUN == This_p->m_State)
	{
		err = This_p->m_pTask->pause(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );
		This_p->m_bPaused = ZOE_SUCCESS(err);
	}

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_PAUSE;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_null_channel_pause() m_pTask->pause failed(%d)\n",
					   err
					   );
	}

	return (err);
}



static zoe_errs_t c_null_channel_begin_flush(c_channel *This_p) 
{
	return (ZOE_ERRS_SUCCESS);
}


static zoe_errs_t c_null_channel_flush(c_channel *This_p) 
{
	return (ZOE_ERRS_SUCCESS);
}


static zoe_errs_t c_null_channel_end_flush(c_channel *This_p) 
{
	return (ZOE_ERRS_SUCCESS);
}


static zoe_errs_t c_null_channel_add_buffer(c_channel *This_p,
										    PZV_BUFFER_DESCRIPTOR pBufDesc
										    ) 
{
	return (ZOE_ERRS_SUCCESS);
}


static zoe_errs_t c_null_channel_cancel_buffer(c_channel *This_p,
											   PZV_BUFFER_DESCRIPTOR pBufDesc, 
											   zoe_bool_t bTimeOut
											   ) 
{
	return (ZOE_ERRS_SUCCESS);
}


static zoe_errs_t c_null_channel_timeout_buffer(c_channel *This_p,
											    PZV_BUFFER_DESCRIPTOR pBufDesc
											    ) 
{
	return (ZOE_ERRS_SUCCESS);
}


// constructor
//
c_null_channel * c_null_channel_constructor(c_null_channel *pNullChannel, 
										    c_object *pParent,
										    c_task *pTask,
                                            IZOEHALAPI *pHal,
                                            zoe_dbg_comp_id_t dbgID
										    )
{
	if (pNullChannel)
	{
		c_channel	*pChannel;

		pChannel = c_channel_constructor(&pNullChannel->m_Channel, 
										 pParent,
										 OBJECT_DEFAULT,
										 CHANNEL_DIR_NONE,
										 ZV_CODEC_VIRTUAL,
										 pTask,
                                         pHal,
                                         dbgID
										 );
		if (!pChannel)
		{
			pNullChannel = ZOE_NULL;
		}
		else
		{
			// fill in the functions
			//
			pNullChannel->m_Channel.open = c_null_channel_open;
			pNullChannel->m_Channel.close = c_null_channel_close;
			pNullChannel->m_Channel.start = c_null_channel_start;
			pNullChannel->m_Channel.stop = c_null_channel_stop;
			pNullChannel->m_Channel.pause = c_null_channel_pause;
			pNullChannel->m_Channel.begin_flush = c_null_channel_begin_flush;
			pNullChannel->m_Channel.flush = c_null_channel_flush;
			pNullChannel->m_Channel.end_flush = c_null_channel_end_flush;
			pNullChannel->m_Channel.add_buffer = c_null_channel_add_buffer;
			pNullChannel->m_Channel.cancel_buffer = c_null_channel_cancel_buffer;
			pNullChannel->m_Channel.timeout_buffer = c_null_channel_timeout_buffer;
		}
	}

	return (pNullChannel);
}



// destructor
//
void c_null_channel_destructor(c_null_channel *This) 
{
	c_channel_destructor(&This->m_Channel);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// c_vid_out_channel
//

// c_channel
//
static zoe_errs_t c_vid_out_channel_start(c_channel *This_p)
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_vid_out_channel_start() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (This_p->m_bPaused)
	{
		err = This_p->m_pTask->resume(This_p->m_pTask, 
									  This_p->m_ChannelType
									  );

		if (ZOE_SUCCESS(err))
        {
			This_p->m_bPaused = ZOE_FALSE;
        }
	}
	else
	{
		// reset counter and flags
		//
		This_p->m_ullCntBytes = 0;

		// start the task
		//
		err = This_p->m_pTask->start(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );
		// remember the start time
		//
		This_p->m_llStartTime = util_get_system_time_ms();
	}

	This_p->m_bFlushing = ZOE_FALSE;

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_RUN;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_vid_out_channel_start() Failed Status(%d)\n", 
					   err
					   );
	}

	return (err);
}



static zoe_errs_t c_vid_out_channel_stop(c_channel *This_p)
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_vid_out_channel_stop() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	err = This_p->m_pTask->stop(This_p->m_pTask, 
								This_p->m_ChannelType
								);
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_vid_out_channel_stop() m_pTask->stop failed(%d)\n",
					   err
					   );
	}

	// clear falgs
	//
	This_p->m_bPaused = ZOE_FALSE;

	// change state to stop
	//
	This_p->m_State = ZVSTATE_STOP;

	// flush the data
	//
	This_p->begin_flush(This_p);
	This_p->flush(This_p);
	This_p->end_flush(This_p);

	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_vid_out_channel_pause(c_channel *This_p)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_vid_out_channel_pause() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (ZVSTATE_RUN == This_p->m_State)
	{
		err = This_p->m_pTask->pause(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );
		This_p->m_bPaused = ZOE_SUCCESS(err);
	}

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_PAUSE;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_vid_out_channel_pause() m_pTask->pause failed(%d)\n",
					   err
					   );
	}

	return (err);
}



// called from streaming task
//
static zoe_bool_t c_vid_out_channel_get_buffer(c_channel *This_p,
										       PZV_BUFFER_DESCRIPTOR *ppBufDesc,
										       uint8_t * *ppBuffer,
										       uint32_t * pSize
										       )
{

	QUEUE_ENTRY				*pEntry;
	PZV_BUFFER_DESCRIPTOR	pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;

	pEntry = c_queue_get_one_entry(This_p->m_pDataRequestQueue);

	if (ZOE_NULL != pEntry)
	{
		pBufDesc = (PZV_BUFFER_DESCRIPTOR)pEntry->Data;
		pDataPacket = &pBufDesc->DataBuffer[0];

		pBufDesc->ulBufferIndex = 0;
		pBufDesc->ulBufferOffset = 0;
		pBufDesc->ulTotalUsed = 0;

		pDataPacket->DataUsed = 0;

		*ppBuffer = (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset;
		*pSize = pDataPacket->BufferSize - pDataPacket->DataOffset;
		*ppBufDesc = pBufDesc;

		zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                       This_p->m_dbgID,
					   "get_buffer(%d) buffer(0x%x) size(%d) Desc(0x%x)\n", 
					   This_p->m_hChannel,
					   *ppBuffer,
					   *pSize,
					   *ppBufDesc
					   );

		// move this entry to the pending queue
		//
		c_queue_add_entry(This_p->m_pDataPendingQueue, 
						  pEntry
						  );
	}

	return (ZOE_NULL != pEntry);
}



static void c_vid_out_channel_complete_buffer(c_channel *This_p,
										      PZV_BUFFER_DESCRIPTOR pBufDesc
										      )
{
	QUEUE_ENTRY	    *pEntry;
	PZV_BUFFER_DATA	pDataPacket;
    uint32_t    i;

	////zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
    ////               This_p->m_dbgID,
	////			   "c_vid_out_channel_complete_buffer()\n"
	////			   );

	// get the buffer entry
	//
	pEntry = c_queue_get_entry_by_data(This_p->m_pDataPendingQueue, 
								       (zoe_void_ptr_t)pBufDesc
								       );	
	if (pEntry)
	{
		zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                       This_p->m_dbgID,
					   "c_vid_out_channel_complete_buffer() bufDesc(0x%x) Status(%d), totalUsed(%d)\n",
					   pBufDesc,
					   pBufDesc->Status,
					   pBufDesc->ulTotalUsed
					   );
        for (i = 0; i < pBufDesc->NumberOfBuffers; i++)
        {
		    pDataPacket = &pBufDesc->DataBuffer[i];

		    if (This_p->m_bByteSwap &&
			    !ZOEHAL_CanSwapData(This_p->m_pHal) &&
			    (0 != pDataPacket->DataUsed)
			    )
		    {
			    util_bytes_swap((uint8_t *)pDataPacket->Data + pDataPacket->DataOffset,
					            (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset, 
					            pDataPacket->DataUsed
					            );
		    }

		    // set these flags if we are at the begining of the stream
		    //
		    if (0 == This_p->m_ullCntBytes)
		    {
		        zoe_dbg_printf(ZOE_DBG_LVL_WARNING/*ZOE_DBG_LVL_TRACE*/,
                               This_p->m_dbgID,
						       "c_vid_out_channel_complete_buffer() (0 == m_ullCntBytes) ZV_BUFDESC_FLAG_DATADISCONTINUITY\n" 
						       );
			    pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_DATADISCONTINUITY |
								     ZV_BUFDESC_FLAG_TIMEDISCONTINUITY
									 ;
		    }

		    This_p->m_ullCntBytes += pDataPacket->DataUsed;
		    pBufDesc->ulTotalUsed += pDataPacket->DataUsed;
        }

		if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_EOS)
		{
		    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This_p->m_dbgID,
					       "c_vid_out_channel_complete_buffer() used(%d) total(%d) ZV_BUFDESC_FLAG_EOS\n", 
					       pBufDesc->ulTotalUsed,
					       This_p->m_ullCntBytes
					       );
		}

		// clear the entry content
		//
		pEntry->Data = ZOE_NULL;

		// move to free queue
		//
		c_queue_add_entry(This_p->m_pFreeQueue, 
						  pEntry
						  );

		// call back to the api
		//
		c_channel_device_callback(This_p,
								  ZVCODEC_CMD_DONE_DATA,
								  pBufDesc							
								  );
	}
}



// constructor
//
c_vid_out_channel * c_vid_out_channel_constructor(c_vid_out_channel *pVidOutChannel, 
											      c_object *pParent,
											      uint32_t dwAttributes,
											      c_task *pTask,
                                                  IZOEHALAPI *pHal,
                                                  zoe_dbg_comp_id_t dbgID
											      )
{
	if (pVidOutChannel)
	{
		c_channel	*pChannel;

		pChannel = c_channel_constructor(&pVidOutChannel->m_Channel, 
										 pParent,
										 dwAttributes,
										 CHANNEL_DIR_READ,
										 ZV_CODEC_VID_OUT,
										 pTask,
                                         pHal,
                                         dbgID
										 );
		if (!pChannel)
		{
			pVidOutChannel = ZOE_NULL;
		}
		else
		{
			pVidOutChannel->m_Channel.m_bByteSwap = ZOE_TRUE;

			// fill in the functions table
			//
			pVidOutChannel->m_Channel.start = c_vid_out_channel_start;
			pVidOutChannel->m_Channel.stop = c_vid_out_channel_stop;
			pVidOutChannel->m_Channel.pause = c_vid_out_channel_pause;
			pVidOutChannel->m_Channel.get_buffer = c_vid_out_channel_get_buffer;
			pVidOutChannel->m_Channel.complete_buffer = c_vid_out_channel_complete_buffer;
		}
	}

	return (pVidOutChannel);
}



// destructor
//
void c_vid_out_channel_destructor(c_vid_out_channel *This) 
{
	c_channel_destructor(&This->m_Channel);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_yuv_out_channel
//


// c_channel
//

static zoe_errs_t c_yuv_out_channel_open(c_channel *This_p,
									     ZOE_OBJECT_HANDLE hChannel,
									     uint32_t dwFlags,
									     zoe_void_ptr_t pDataFormat,
									     ZV_DEVICE_CALLBACK pFuncCallback,
									     zoe_void_ptr_t context
									     )
{
	zoe_errs_t			err;
	PZV_YUV_DATAFORMAT	pYUVFormat = (PZV_YUV_DATAFORMAT)pDataFormat;
	c_yuv_out_channel	*This =	GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, This_p);

	// Gather information we need for this stream
	//
	This->m_lWidth = pYUVFormat->nWidth;
	This->m_lHeight = pYUVFormat->nHeight;
	This->m_nBitCount = pYUVFormat->nBitCount;
	This->m_nDataType = pYUVFormat->nDataType;

	This->m_dwFrameSize = (This->m_lWidth * This->m_lHeight * This->m_nBitCount) / 8;

	This->m_Channel.m_bByteSwap = ZOE_TRUE; 

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_yuv_out_channel_open() m_dwFrameSize(%d) m_lWidth(%d) m_lHeight(%d) swap(%d) m_nDataType(%d)\n",
				   This->m_dwFrameSize,
				   This->m_lWidth,
				   This->m_lHeight,
				   This->m_Channel.m_bByteSwap,
				   This->m_nDataType
				   );

	// reset counters and flags
	//
	This->m_ulVideoFramesCount = 0;
	This->m_ulDroppedFramesCount = 0;

	// generic open
	//
	err = c_channel_open(This_p,
						 hChannel,
						 dwFlags,
						 pDataFormat,
						 pFuncCallback,
						 context
						 );
	return (err);
}



static zoe_errs_t c_yuv_out_channel_start(c_channel *This_p)
{
	zoe_errs_t		    err;
	c_yuv_out_channel	*This =	GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_yuv_out_channel_start() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (This_p->m_bPaused)
	{
		err = This_p->m_pTask->resume(This_p->m_pTask, 
									  This_p->m_ChannelType
									  );

		if (ZOE_SUCCESS(err))
        {
			This_p->m_bPaused = ZOE_FALSE;
        }
	}
	else
	{
		// reset counter and flags
		//
		This_p->m_ullCntBytes = 0;

		This->m_ulVideoFramesCount = 0;
		This->m_ulDroppedFramesCount = 0;

		// start the task
		//
		err = This_p->m_pTask->start(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );

		This_p->m_llStartTime = util_get_system_time_ms();
	}

	This_p->m_bFlushing = ZOE_FALSE;

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_RUN;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_yuv_out_channel_start() Failed Status(%d)\n", 
					   err
					   );
	}

	return (err);
}



// called from streaming task
//
static zoe_bool_t c_yuv_out_channel_get_buffer(c_channel *This_p,
										       PZV_BUFFER_DESCRIPTOR *ppBufDesc,
										       uint8_t * *ppYBuffer, 
										       uint32_t * pYSize, 
										       uint8_t * *ppUVBuffer, 
										       uint32_t * pUVSize
										       )
{
	QUEUE_ENTRY				*pEntry;
	PZV_BUFFER_DESCRIPTOR	pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;
	c_yuv_out_channel		*This =	GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, This_p);

	pEntry = c_queue_get_one_entry(This_p->m_pDataRequestQueue);

	if (ZOE_NULL != pEntry)
	{
		pBufDesc = (PZV_BUFFER_DESCRIPTOR)pEntry->Data;

		if ((pBufDesc->ulBufferSize < This->m_dwFrameSize) ||
            (2 != pBufDesc->NumberOfBuffers)
            )
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This_p->m_dbgID,
						   "c_yuv_out_channel_get_buffer() buffer size(%d) < frame size(%d) nb buffers(%d)\n", 
						   pBufDesc->ulBufferSize, 
						   This->m_dwFrameSize,
                           pBufDesc->NumberOfBuffers
						   );
			// clear the entry content
			//
			pEntry->Data = ZOE_NULL;

			// move to free queue
			//
			c_queue_add_entry(This_p->m_pFreeQueue, 
							  pEntry
							  );

			// nil pEntry so the function will return FALSE
			//
			pEntry = ZOE_NULL;

			// default success
			//
			pBufDesc->Status = ZOE_ERRS_SUCCESS;

			// call back the api
			//
			c_channel_device_callback(This_p,
									  ZVCODEC_CMD_DONE_DATA,
									  pBufDesc							
									  );
		}
		else
		{
			pBufDesc->ulBufferIndex = 0;
			pBufDesc->ulBufferOffset = 0;
			pBufDesc->ulTotalUsed = 0;

            // y buffer
		    pDataPacket = &pBufDesc->DataBuffer[0];
			pDataPacket->DataUsed = 0;
			*ppYBuffer = (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset;
			*pYSize = pDataPacket->BufferSize - pDataPacket->DataOffset;//This->m_lWidth * This->m_lHeight;

            // uv buffer
		    pDataPacket = &pBufDesc->DataBuffer[1];
			pDataPacket->DataUsed = 0;
			*ppUVBuffer = (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset;
			*pUVSize = pDataPacket->BufferSize - pDataPacket->DataOffset;//((*pYSize) >> 1);

			*ppBufDesc = pBufDesc;

		    zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                           This_p->m_dbgID,
						   "c_yuv_out_channel_get_buffer(%d) Y(0x%p) size(%d) UV(0x%p) size(%d) frame(%d)\n", 
						   *ppYBuffer,
						   *pYSize,
						   *ppUVBuffer,
						   *pUVSize,
						   This->m_ulVideoFramesCount
						   );

			// move this entry to the pending queue
			//
			c_queue_add_entry(This_p->m_pDataPendingQueue, 
							  pEntry
							  );
		}
	}

	return (ZOE_NULL != pEntry);
}



static void c_yuv_out_channel_complete_buffer(c_channel *This_p,
										      PZV_BUFFER_DESCRIPTOR pBufDesc
										      )
{
	QUEUE_ENTRY		    *pEntry;
	PZV_BUFFER_DATA	    pDataPacket;
	c_yuv_out_channel	*This =	GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, This_p);
    uint32_t        i;

	////zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
    ////               This_p->m_dbgID,
	////			   "c_yuv_out_channel_complete_buffer()\n"
	////			   );

	// get the buffer entry
	//
	pEntry = c_queue_get_entry_by_data(This_p->m_pDataPendingQueue, 
								       (zoe_void_ptr_t)pBufDesc
								       );
	if (pEntry)
	{
		// set these flags if we are at the begining of the stream
		//
		if (0 == This->m_ulVideoFramesCount)
		{
		    zoe_dbg_printf(ZOE_DBG_LVL_WARNING/*ZOE_DBG_LVL_TRACE*/,
                           This_p->m_dbgID,
						   "c_yuv_out_channel_complete_buffer() (0 == m_ulVideoFramesCount) ZV_BUFDESC_FLAG_DATADISCONTINUITY\n"
						   );
			pBufDesc->ulFlags |= ZV_BUFDESC_FLAG_DATADISCONTINUITY |
							     ZV_BUFDESC_FLAG_TIMEDISCONTINUITY
								 ;
		}

		// increment total frame captured
		//
		This->m_ulVideoFramesCount++;

		// update frame info
		//
		pBufDesc->dwFrameFlags = ZV_VIDEO_FLAG_FRAME;
		pBufDesc->PictureNumber = This->m_ulVideoFramesCount;
		pBufDesc->DropCount = 0;

		zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                       This_p->m_dbgID,
					   "c_yuv_out_channel_complete_buffer() flgs(0x%x) Cap(%d) size(%d)",
					   pBufDesc->ulFlags,
					   This->m_ulVideoFramesCount,
					   pBufDesc->ulTotalUsed
					   );
        pBufDesc->ulTotalUsed = 0;
        for (i = 0; i < pBufDesc->NumberOfBuffers; i++)
        {
            pDataPacket = &pBufDesc->DataBuffer[i];
            pBufDesc->ulTotalUsed += pDataPacket->DataUsed;
        }
        This_p->m_ullCntBytes += pBufDesc->ulTotalUsed;

		// clear the entry content
		//
		pEntry->Data = ZOE_NULL;

		// move to free queue
		//
		c_queue_add_entry(This_p->m_pFreeQueue, 
						  pEntry
						  );

		// call back to the api
		//
		c_channel_device_callback(This_p,
								  ZVCODEC_CMD_DONE_DATA,
								  pBufDesc							
								  );
	}
}



static zoe_errs_t c_yuv_out_channel_get_resolution(c_channel *This_p,
											       uint32_t * pWidth,
											       uint32_t * pHeight
											       )
{
	c_yuv_out_channel   *This = GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, This_p);

	if (pWidth &&
		pHeight
		)
	{
		*pWidth = (uint32_t)This->m_lWidth;
		*pHeight = (uint32_t)This->m_lHeight;
		return(ZOE_ERRS_SUCCESS);
	}
	else
	{
		return (ZOE_ERRS_PARMS);
	}
}



static zoe_errs_t c_yuv_out_channel_get_yuv_format(c_channel *This_p,
											       uint32_t * pYUVFormat
											       )
{
	c_yuv_out_channel   *This = GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, This_p);

	if (pYUVFormat)
	{
		*pYUVFormat = This->m_nDataType;
		return(ZOE_ERRS_SUCCESS);
	}
	else
	{
		return (ZOE_ERRS_PARMS);
	}
}



// constructor
//
c_yuv_out_channel * c_yuv_out_channel_constructor(c_yuv_out_channel *pYUVOutChannel, 
											      c_object *pParent,
											      uint32_t dwAttributes,
											      c_task *pTask,
                                                  IZOEHALAPI *pHal,
                                                  zoe_dbg_comp_id_t dbgID
											      )
{
	if (pYUVOutChannel)
	{
		c_channel	*pChannel;

		pChannel = c_channel_constructor(&pYUVOutChannel->m_Channel, 
										 pParent,
										 dwAttributes,
										 CHANNEL_DIR_READ,
										 ZV_CODEC_YUV_OUT,
										 pTask,
                                         pHal,
                                         dbgID
										 );
		if (!pChannel)
		{
			pYUVOutChannel = ZOE_NULL;
		}
		else
		{
			// fill in the functions table
			//
			pYUVOutChannel->m_Channel.open = c_yuv_out_channel_open;
			pYUVOutChannel->m_Channel.start = c_yuv_out_channel_start;
			pYUVOutChannel->m_Channel.stop = c_vid_out_channel_stop;   // same as VidOut
			pYUVOutChannel->m_Channel.pause = c_vid_out_channel_pause;	// same as VidOut
			pYUVOutChannel->m_Channel.get_buffer = c_vid_out_channel_get_buffer; // same as VidOut
			pYUVOutChannel->m_Channel.get_buffer_yuv = c_yuv_out_channel_get_buffer;
			pYUVOutChannel->m_Channel.complete_buffer = c_yuv_out_channel_complete_buffer;
			pYUVOutChannel->m_Channel.get_resolution = c_yuv_out_channel_get_resolution;
			pYUVOutChannel->m_Channel.get_yuv_format = c_yuv_out_channel_get_yuv_format;

            pYUVOutChannel->m_Channel.m_bYUV = ZOE_TRUE;

			// c_yuv_out_channel
			//
			pYUVOutChannel->m_dwFrameSize = 0;
			pYUVOutChannel->m_ulVideoFramesCount = 0;
			pYUVOutChannel->m_ulDroppedFramesCount = 0;

			pYUVOutChannel->m_lWidth = 0;
			pYUVOutChannel->m_lHeight = 0;
			pYUVOutChannel->m_nDataType = ZV_YUV_DATA_TYPE_NV12;
		}
	}

	return (pYUVOutChannel);
}



// destructor
//
void c_yuv_out_channel_destructor(c_yuv_out_channel *This)
{
	c_channel_destructor(&This->m_Channel);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_meta_out_channel
//


// c_channel
//


// constructor
//
c_meta_out_channel * c_meta_out_channel_constructor(c_meta_out_channel *p_meta_out_channel, 
												    c_object *pParent,
												    uint32_t dwAttributes,
												    c_task *pTask,
                                                    IZOEHALAPI *pHal,
                                                    zoe_dbg_comp_id_t dbgID
												    )
{
	if (p_meta_out_channel)
	{
		c_channel	*pChannel;

		pChannel = c_channel_constructor(&p_meta_out_channel->m_Channel, 
										 pParent,
										 dwAttributes,
										 CHANNEL_DIR_READ,
										 ZV_CODEC_META_OUT,
										 pTask,
                                         pHal,
                                         dbgID
										 );
		if (!pChannel)
		{
			p_meta_out_channel = ZOE_NULL;
		}
		else
		{
			// fill in the functions table
			//
			p_meta_out_channel->m_Channel.start = c_vid_out_channel_start;
			p_meta_out_channel->m_Channel.stop = c_vid_out_channel_stop;	    // same as MpegOut
			p_meta_out_channel->m_Channel.pause = c_vid_out_channel_pause;	// same as MpegOut
			p_meta_out_channel->m_Channel.get_buffer = c_vid_out_channel_get_buffer;	// same as MpegOut
			p_meta_out_channel->m_Channel.complete_buffer = c_vid_out_channel_complete_buffer; // same as MpegOut
		}
	}

	return (p_meta_out_channel);
}



// destructor
//
void c_meta_out_channel_destructor(c_meta_out_channel *This) 
{
	c_channel_destructor(&This->m_Channel);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_channel
//
static zoe_errs_t c_vid_in_channel_start(c_channel *This_p)
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_vid_in_channel_start() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (This_p->m_bPaused)
	{
		err = This_p->m_pTask->resume(This_p->m_pTask, 
									  This_p->m_ChannelType
									  );

		if (ZOE_SUCCESS(err))
        {
			This_p->m_bPaused = ZOE_FALSE;
        }
	}
	else
	{
		// reset counter and flags
		//
		This_p->m_ullCntBytes = 0;

		err = This_p->m_pTask->start(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );
		This_p->m_llStartTime = util_get_system_time_ms();
	}

	This_p->m_bFlushing = ZOE_FALSE;

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_RUN;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_vid_in_channel_start() Failed Status(%d)\n", 
					   err
					   );
	}

	return (err);
}



static zoe_errs_t c_vid_in_channel_set_rate(c_channel *This_p,
										    int32_t lNewRate
										    )
{
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_vid_in_channel_get_rate(c_channel *This_p,
										    int32_t * plRate
										    )
{
	return (ZOE_ERRS_SUCCESS);
}



// called from streaming task
//
static zoe_bool_t c_vid_in_channel_get_buffer(c_channel *This_p,
										      PZV_BUFFER_DESCRIPTOR *ppBufDesc,
										      uint8_t * *ppBuffer,
										      uint32_t * pSize
										      )
{
	QUEUE_ENTRY				*pEntry;
	PZV_BUFFER_DESCRIPTOR	pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;

	// take the buffer from the request queue
	//
	pEntry = c_queue_get_one_entry(This_p->m_pDataRequestQueue);

	if (ZOE_NULL != pEntry)
	{
		pBufDesc = (PZV_BUFFER_DESCRIPTOR)pEntry->Data;
		pDataPacket = &pBufDesc->DataBuffer[0];

		// find end of stream indication
		//
		if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_EOS)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This_p->m_dbgID,
						   "c_vid_in_channel_get_buffer() ZV_BUFDESC_FLAG_EOS\n"
						   );
		}

		if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_PTS)
		{
		    zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                           This_p->m_dbgID,
						   "c_vid_in_channel_get_buffer() PTS(%u)\n",
						   pBufDesc->ulPTS
						   );
		}

		if (This_p->m_bByteSwap &&
			!ZOEHAL_CanSwapData(This_p->m_pHal) &&
			(0 != pDataPacket->DataUsed)
			)
		{
			util_bytes_swap((uint8_t *)pDataPacket->Data + pDataPacket->DataOffset,
					        (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset, 
					        pDataPacket->DataUsed
					        );
		}

		pBufDesc->ulBufferIndex = 0;
		pBufDesc->ulBufferOffset = 0;
		pBufDesc->ulTotalUsed = 0;

		*ppBuffer = (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset;
		*pSize = pDataPacket->DataUsed;
		*ppBufDesc = pBufDesc;

		zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                       This_p->m_dbgID,
					   "c_vid_in_channel_get_buffer() size(%d) buffer(0x%x) Desc(0x%x)\n", 
					   pDataPacket->DataUsed,
					   *ppBuffer,
					   *ppBufDesc
					   );

		// move this entry to the pending queue
		//
		c_queue_add_entry(This_p->m_pDataPendingQueue, 
						  pEntry
						  );
	}

	return (ZOE_NULL != pEntry);
}



static void c_vid_in_channel_complete_buffer(c_channel *This_p,
										     PZV_BUFFER_DESCRIPTOR pBufDesc
										     )
{
	QUEUE_ENTRY	*pEntry;

	////zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
    ////               This_p->m_dbgID,
	////			   "c_vid_in_channel_complete_buffer()\n"
	////			   );

	// get the buffer entry
	//
	pEntry = c_queue_get_entry_by_data(This_p->m_pDataPendingQueue, 
								       (zoe_void_ptr_t)pBufDesc
								       );
	if (pEntry)
	{
		if (!ZOE_SUCCESS(pBufDesc->Status))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This_p->m_dbgID,
						   "c_vid_in_channel_complete_buffer(%d) Status(%d)\n",
						   This_p->m_hChannel,
						   pBufDesc->Status
						   );
		}

		This_p->m_ullCntBytes += pBufDesc->ulTotalUsed;

		if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_EOS)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           This_p->m_dbgID,
						   "c_vid_in_channel_complete_buffer(%d) ZV_BUFDESC_FLAG_EOS\n",
						   This_p->m_hChannel
						   );
		}

		// clear the entry content
		//
		pEntry->Data = ZOE_NULL;

		// move to the free queue
		//
		c_queue_add_entry(This_p->m_pFreeQueue, 
						  pEntry
						  );

		// call back the api
		//
		c_channel_device_callback(This_p,
								  ZVCODEC_CMD_DONE_DATA,
								  pBufDesc							
								  );
	}
}


// constructor
//
c_vid_in_channel * c_vid_in_channel_constructor(c_vid_in_channel *pVidInChannel, 
										        c_object *pParent,
										        uint32_t dwAttributes,
										        c_task *pTask,
                                                IZOEHALAPI *pHal,
                                                zoe_dbg_comp_id_t dbgID
										        )
{
	if (pVidInChannel)
	{
		c_channel	*pChannel;

		pChannel = c_channel_constructor(&pVidInChannel->m_Channel, 
										 pParent,
										 dwAttributes,
										 CHANNEL_DIR_WRITE,
										 ZV_CODEC_VID_IN,
										 pTask,
                                         pHal,
                                         dbgID
										 );
		if (!pChannel)
		{
			pVidInChannel = ZOE_NULL;
		}
		else
		{
			pVidInChannel->m_Channel.m_bByteSwap = ZOE_TRUE;

			// fill in the functions
			//
			pVidInChannel->m_Channel.start = c_vid_in_channel_start;
			pVidInChannel->m_Channel.stop = c_vid_out_channel_stop;
			pVidInChannel->m_Channel.pause = c_vid_out_channel_pause;
			pVidInChannel->m_Channel.set_rate = c_vid_in_channel_set_rate;
			pVidInChannel->m_Channel.get_rate = c_vid_in_channel_get_rate;
			pVidInChannel->m_Channel.get_buffer = c_vid_in_channel_get_buffer;
			pVidInChannel->m_Channel.complete_buffer = c_vid_in_channel_complete_buffer;
		}
	}

	return (pVidInChannel);
}



// destructor
//
void c_vid_in_channel_destructor(c_vid_in_channel *This) 
{
	c_channel_destructor(&This->m_Channel);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// c_yuv_in_channel
//

// c_channel
//

static zoe_errs_t c_yuv_in_channel_open(c_channel *This_p,
									    ZOE_OBJECT_HANDLE hChannel,
									    uint32_t dwFlags,
									    zoe_void_ptr_t pDataFormat,
									    ZV_DEVICE_CALLBACK pFuncCallback,
									    zoe_void_ptr_t context
									    )
{
	zoe_errs_t			err;
	PZV_YUV_DATAFORMAT	pYUVFormat = (PZV_YUV_DATAFORMAT)pDataFormat;
	c_yuv_in_channel	*This =	GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, This_p);

	// Gather information we need for this stream
	//
	This->m_lWidth = pYUVFormat->nWidth;
	This->m_lHeight = pYUVFormat->nHeight;
	This->m_nBitCount = pYUVFormat->nBitCount;
	This->m_nDataType = pYUVFormat->nDataType;

	This->m_dwFrameSize = (This->m_lWidth * This->m_lHeight * This->m_nBitCount) / 8;

	This->m_Channel.m_bByteSwap = ZOE_TRUE;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_yuv_in_channel_open() m_dwFrameSize(%d) m_lWidth(%d) m_lHeight(%d) bitCnt(%d) swap(%d) m_nDataType(%d)\n",
				   This->m_dwFrameSize,
				   This->m_lWidth,
				   This->m_lHeight,
				   This->m_nBitCount,
				   This->m_Channel.m_bByteSwap,
				   This->m_nDataType
				   );

	// reset counters and flags
	//
	This->m_ulVideoFramesCount = 0;

	// generic open
	//
	err = c_channel_open(This_p,
						 hChannel,
						 dwFlags,
						 pDataFormat,
						 pFuncCallback,
						 context
						 );
	return (err);
}



static zoe_errs_t c_yuv_in_channel_start(c_channel *This_p)
{
	zoe_errs_t		    err;
	c_yuv_in_channel	*This =	GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This_p->m_dbgID,
				   "c_yuv_in_channel_start() hChannel(%d) dataType(%d)\n",
				   This_p->m_hChannel,
				   This_p->m_ChannelType
				   );

	if (This_p->m_bPaused)
	{
		err = This_p->m_pTask->resume(This_p->m_pTask, 
									  This_p->m_ChannelType
									  );

		if (ZOE_SUCCESS(err))
        {
			This_p->m_bPaused = ZOE_FALSE;
        }
	}
	else
	{
		// reset counter and flags
		//
		This_p->m_ullCntBytes = 0;
		This->m_ulVideoFramesCount = 0;

		// start the encoder
		//
		err = This_p->m_pTask->start(This_p->m_pTask, 
									 This_p->m_ChannelType
									 );

		This_p->m_llStartTime = util_get_system_time_ms();
	}

	This_p->m_bFlushing = ZOE_FALSE;

	if (ZOE_SUCCESS(err))
	{
		This_p->m_State = ZVSTATE_RUN;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This_p->m_dbgID,
					   "c_yuv_in_channel_start() Failed Status(%d)\n", 
					   err
					   );
	}

	return (err);
}



// called from streaming task
//
static zoe_bool_t c_yuv_in_channel_get_buffer_yuv(c_channel *This_p,
										          PZV_BUFFER_DESCRIPTOR *ppBufDesc,
										          uint8_t * *ppYBuffer, 
										          uint32_t * pYSize, 
										          uint8_t * *ppUVBuffer, 
										          uint32_t * pUVSize
										          )
{
	QUEUE_ENTRY				*pEntry;
	PZV_BUFFER_DESCRIPTOR	pBufDesc;
	PZV_BUFFER_DATA		    pDataPacket;
	c_yuv_in_channel		*This =	GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, This_p);

	pEntry = c_queue_get_one_entry(This_p->m_pDataRequestQueue);

	if (ZOE_NULL != pEntry)
	{
		pBufDesc = (PZV_BUFFER_DESCRIPTOR)pEntry->Data;

		if ((pBufDesc->ulFlags & ZV_BUFDESC_FLAG_EOS) ||
			(0 == pBufDesc->ulBufferSize)
			)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This_p->m_dbgID,
						   "c_yuv_in_channel_get_buffer_yuv() ZV_BUFDESC_FLAG_EOS\n"
						   );
		}

		if ((pBufDesc->ulBufferSize < This->m_dwFrameSize) ||
            (2 != pBufDesc->NumberOfBuffers)
            )
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This_p->m_dbgID,
						   "c_yuv_in_channel_get_buffer_yuv() buffer size(%d) < frame size(%d) nb buffers(%d)\n", 
						   pBufDesc->ulBufferSize, 
						   This->m_dwFrameSize,
                           pBufDesc->NumberOfBuffers
						   );
			// clear the entry content
			//
			pEntry->Data = ZOE_NULL;

			// move to free queue
			//
			c_queue_add_entry(This_p->m_pFreeQueue, 
							  pEntry
							  );

			// nil pEntry so the function will return FALSE
			//
			pEntry = ZOE_NULL;

			// default success
			//
			pBufDesc->Status = ZOE_ERRS_SUCCESS;

			// call back the api
			//
			c_channel_device_callback(This_p,
									  ZVCODEC_CMD_DONE_DATA,
									  pBufDesc							
									  );
		}
		else
		{
            // y buffer
		    pDataPacket = &pBufDesc->DataBuffer[0];

			if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_PTS)
			{
		        zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                               This_p->m_dbgID,
							   "c_yuv_in_channel_get_buffer_yuv() PTS(0x%x:%x)\n",
							   (uint32_t)((pBufDesc->ulPTS >> 32) & 0xFFFFFFFF),
							   (uint32_t)(pBufDesc->ulPTS & 0xFFFFFFFF)
							   );
			}

			if (This_p->m_bByteSwap &&
			    !ZOEHAL_CanSwapData(This_p->m_pHal) &&
				(0 != pDataPacket->DataUsed)
				)
			{
				util_bytes_swap((uint8_t *)pDataPacket->Data + pDataPacket->DataOffset,
						        (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset, 
						        pDataPacket->DataUsed
						        );
			}
			*ppYBuffer = (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset;
			*pYSize = This->m_lWidth * This->m_lHeight;

            // uv buffer
		    pDataPacket = &pBufDesc->DataBuffer[1];

			if (This_p->m_bByteSwap &&
			    !ZOEHAL_CanSwapData(This_p->m_pHal) &&
				(0 != pDataPacket->DataUsed)
				)
			{
				util_bytes_swap((uint8_t *)pDataPacket->Data + pDataPacket->DataOffset,
						        (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset, 
						        pDataPacket->DataUsed
						        );
			}
			*ppUVBuffer = (uint8_t *)pDataPacket->Data + pDataPacket->DataOffset;
			*pUVSize = ((*pYSize) >> 1);

			pBufDesc->ulBufferIndex = 0;
			pBufDesc->ulBufferOffset = 0;
			pBufDesc->ulTotalUsed = 0;

			*ppBufDesc = pBufDesc;

		    zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                           This_p->m_dbgID,
						   "c_yuv_in_channel_GetBufferRas() Y(0x%p) size(%d) UV(0x%p) size(%d) frame(%d)\n", 
						   *ppYBuffer,
						   *pYSize,
						   *ppUVBuffer,
						   *pUVSize,
						   This->m_ulVideoFramesCount
						   );

			// move this entry to the pending queue
			//
			c_queue_add_entry(This_p->m_pDataPendingQueue, 
							  pEntry
							  );
		}
	}

	return (ZOE_NULL != pEntry);
}



static void c_yuv_in_channel_complete_buffer(c_channel *This_p,
										     PZV_BUFFER_DESCRIPTOR pBufDesc
										     )
{
	QUEUE_ENTRY		    *pEntry;
	PZV_BUFFER_DATA	    pDataPacket;
	c_yuv_in_channel	*This =	GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, This_p);

	////zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
    ////               This_p->m_dbgID,
	////			   "c_yuv_in_channel_complete_buffer()\n"
	////			   );

	// get the buffer entry
	//
	pEntry = c_queue_get_entry_by_data(This_p->m_pDataPendingQueue, 
								       (zoe_void_ptr_t)pBufDesc
								       );
	if (pEntry)
	{
		pDataPacket = pBufDesc->DataBuffer;

		if (!ZOE_SUCCESS(pBufDesc->Status))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This_p->m_dbgID,
						   "c_yuv_in_channel_complete_buffer() Status(%d)\n",
						   pBufDesc->Status
						   );
		}
		else
		{
			// increment total frame consumed
			//
			This->m_ulVideoFramesCount++;
		}

		if (pBufDesc->ulFlags & ZV_BUFDESC_FLAG_EOS)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           This_p->m_dbgID,
						   "c_yuv_in_channel_complete_buffer(%d) ZV_BUFDESC_FLAG_EOS\n",
						   This_p->m_hChannel
						   );
		}

		// clear the entry content
		//
		pEntry->Data = ZOE_NULL;

		// move to free queue
		//
		c_queue_add_entry(This_p->m_pFreeQueue, 
						  pEntry
						  );

		// call back to the api
		//
		c_channel_device_callback(This_p,
								  ZVCODEC_CMD_DONE_DATA,
								  pBufDesc							
								  );
	}
}



static zoe_errs_t c_yuv_in_channel_get_resolution(c_channel *This_p,
											      uint32_t * pWidth,
											      uint32_t * pHeight
											      )
{
	c_yuv_in_channel    *This = GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, This_p);

	if (pWidth &&
		pHeight
		)
	{
		*pWidth = (uint32_t)This->m_lWidth;
		*pHeight = (uint32_t)This->m_lHeight;
		return(ZOE_ERRS_SUCCESS);
	}
	else
	{
		return (ZOE_ERRS_PARMS);
	}
}



static zoe_errs_t c_yuv_in_channel_get_yuv_format(c_channel *This_p,
											      uint32_t * pYUVFormat
											      )
{
	c_yuv_in_channel    *This = GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, This_p);

	if (pYUVFormat)
	{
		*pYUVFormat = This->m_nDataType;
		return(ZOE_ERRS_SUCCESS);
	}
	else
	{
		return (ZOE_ERRS_PARMS);
	}
}



// constructor
//
c_yuv_in_channel * c_yuv_in_channel_constructor(c_yuv_in_channel *pYUVInChannel, 
										        c_object *pParent,
										        uint32_t dwAttributes,
										        c_task *pTask,
                                                IZOEHALAPI *pHal,
                                                zoe_dbg_comp_id_t dbgID
										        )
{
	if (pYUVInChannel)
	{
		c_channel	*pChannel;

		pChannel = c_channel_constructor(&pYUVInChannel->m_Channel, 
										 pParent,
										 dwAttributes,
										 CHANNEL_DIR_WRITE,
										 ZV_CODEC_YUV_IN,
										 pTask,
                                         pHal,
                                         dbgID
										 );
		if (!pChannel)
		{
			pYUVInChannel = ZOE_NULL;
		}
		else
		{
			pYUVInChannel->m_Channel.m_bByteSwap = ZOE_TRUE;

			// fill in the functions table
			//
			pYUVInChannel->m_Channel.open = c_yuv_in_channel_open;
			pYUVInChannel->m_Channel.start = c_yuv_in_channel_start;
			pYUVInChannel->m_Channel.stop = c_vid_out_channel_stop;	// same as VidOut
			pYUVInChannel->m_Channel.pause = c_vid_out_channel_pause;	// same as VidOut
			pYUVInChannel->m_Channel.get_buffer_yuv = c_yuv_in_channel_get_buffer_yuv;
			pYUVInChannel->m_Channel.complete_buffer = c_yuv_in_channel_complete_buffer;
			pYUVInChannel->m_Channel.get_resolution = c_yuv_in_channel_get_resolution;
			pYUVInChannel->m_Channel.get_yuv_format = c_yuv_in_channel_get_yuv_format;

            pYUVInChannel->m_Channel.m_bYUV = ZOE_TRUE;

			// c_yuv_in_channel
			//
			pYUVInChannel->m_dwFrameSize = 0;
			pYUVInChannel->m_ulVideoFramesCount = 0;
			pYUVInChannel->m_lWidth = 0;
			pYUVInChannel->m_lHeight = 0;
			pYUVInChannel->m_nBitCount = 0;
			pYUVInChannel->m_nDataType = ZV_YUV_DATA_TYPE_NV12;
		}
	}

	return (pYUVInChannel);
}



// destructor
//
void c_yuv_in_channel_destructor(c_yuv_in_channel *This)
{
	c_channel_destructor(&This->m_Channel);
}



