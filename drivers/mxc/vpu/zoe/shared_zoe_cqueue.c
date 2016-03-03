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
// zoe_cqueue.c
//
// Description: 
//
//	Queue implementation.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zoe_cqueue.h"


c_queue * c_queue_constructor(c_queue *pQueue,
							  c_object *pParent, 
							  uint32_t dwAttributes,
					          QUEUE_CALLBACK pFuncCallback,
                              zoe_void_ptr_t p_cb_context,
                              zoe_dbg_comp_id_t dbgID
							  )
{
	if (pQueue)
	{
		c_object_constructor(&pQueue->m_Object, 
							 pParent, 
                             OBJECT_QUEUE,
							 dwAttributes
							 );
		pQueue->m_dwNbInQueue = 0;
        pQueue->m_pFuncCallback = pFuncCallback;
        pQueue->m_p_cb_context = p_cb_context;
        pQueue->m_dbgID = dbgID;
		c_queue_reset_queue(pQueue);
	}

	return (pQueue);
}



void c_queue_destructor(c_queue *This)
{
	c_queue_flush_queue(This);

	c_object_destructor(&This->m_Object);
}



// add one entry to the end of the queue
//
void c_queue_add_entry(c_queue *This, 
					   QUEUE_ENTRY *pEntry
					   )
{
	ENTER_CRITICAL(&This->m_Object)

	if (pEntry)
	{
		if (This->m_Queue.pTail)
		{
			This->m_Queue.pTail->pNext = pEntry;
			This->m_Queue.pTail = pEntry;
		}
		else
		{
			This->m_Queue.pHead = pEntry;
			This->m_Queue.pTail = pEntry;
		}
		pEntry->pNext = ZOE_NULL;
		This->m_dwNbInQueue++;
	}

	LEAVE_CRITICAL(&This->m_Object)
}



// add one entry to the top of the queue
//
void c_queue_add_entry_to_top(c_queue *This, 
						      QUEUE_ENTRY *pEntry
						      )
{
	ENTER_CRITICAL(&This->m_Object)

	if (pEntry)
	{
        pEntry->pNext = This->m_Queue.pHead;
        This->m_Queue.pHead = pEntry;

	    if (!This->m_Queue.pTail)
	    {
		    This->m_Queue.pTail = pEntry;
	    }
	    This->m_dwNbInQueue++;
	}

	LEAVE_CRITICAL(&This->m_Object)
}



// remove the specified entry from the queue
//
QUEUE_ENTRY* c_queue_remove_entry(c_queue *This, 
								  QUEUE_ENTRY *pEntry
								  )
{
	QUEUE_ENTRY	*pthis_queue;
	QUEUE_ENTRY	*pprev_queue = ZOE_NULL;

	ENTER_CRITICAL(&This->m_Object)

	pthis_queue = This->m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue == pEntry)
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



// get one entry actually remove the first entry from the queue.
//
QUEUE_ENTRY* c_queue_get_one_entry(c_queue *This)
{
	QUEUE_ENTRY	*pEntry = ZOE_NULL;

	ENTER_CRITICAL(&This->m_Object)

	if (This->m_Queue.pHead)
	{
		pEntry = This->m_Queue.pHead;
		This->m_Queue.pHead = This->m_Queue.pHead->pNext;
		if (!This->m_Queue.pHead)
			This->m_Queue.pTail = This->m_Queue.pHead;
		This->m_dwNbInQueue--;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (pEntry);
}



// return the entry without remove it from the queue
//
QUEUE_ENTRY* c_queue_peek_entry_by_data(c_queue *This, 
									    zoe_void_ptr_t Data
									    )
{
	QUEUE_ENTRY	*pthis_queue;
	QUEUE_ENTRY	*pRet = ZOE_NULL;
		  
	ENTER_CRITICAL(&This->m_Object)

	pthis_queue = This->m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue->Data == Data)
		{
			pRet = pthis_queue;
			break;
		}
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (pRet);
}



// remove the specified entry from the queue
//
QUEUE_ENTRY* c_queue_get_entry_by_data(c_queue *This, 
								       zoe_void_ptr_t Data
								       )
{
	QUEUE_ENTRY	*pthis_queue;
	QUEUE_ENTRY	*pprev_queue = ZOE_NULL;

	ENTER_CRITICAL(&This->m_Object)

	pthis_queue = This->m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue->Data == Data)
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



QUEUE_ENTRY* c_queue_peek_one_entry(c_queue *This)
{
	QUEUE_ENTRY	*pEntry;

	ENTER_CRITICAL(&This->m_Object)

	pEntry = This->m_Queue.pHead;

	LEAVE_CRITICAL(&This->m_Object)

	return (pEntry);
}



QUEUE_ENTRY* c_queue_peek_entry_by_index(c_queue *This, 
									     uint32_t index
									     )
{
	QUEUE_ENTRY     *pthis_queue;
	QUEUE_ENTRY	    *pRet = ZOE_NULL;
	uint32_t	count = 0;
		  
	ENTER_CRITICAL(&This->m_Object)

	pthis_queue = This->m_Queue.pHead;

	while (pthis_queue)
	{
		if (count++ == index)
		{
			pRet = pthis_queue;
			break;
		}
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (pRet);
}



void c_queue_reset_queue(c_queue *This) 
{
	This->m_Queue.pHead = This->m_Queue.pTail = ZOE_NULL;
	This->m_dwNbInQueue = 0;
}



// flush the queue
//
void c_queue_flush_queue(c_queue *This)
{
	QUEUE_ENTRY	*pthis_queue;

	ENTER_CRITICAL(&This->m_Object)

	while (This->m_Queue.pHead)
	{
		pthis_queue = This->m_Queue.pHead;
		This->m_Queue.pHead = pthis_queue->pNext;
		This->m_dwNbInQueue--;

		if (This->m_pFuncCallback && pthis_queue->Data)
		{
			This->m_pFuncCallback(This->m_p_cb_context, (zoe_void_ptr_t)pthis_queue->Data);
		}

	}

	This->m_Queue.pTail = ZOE_NULL;

	LEAVE_CRITICAL(&This->m_Object)
}



zoe_bool_t c_queue_is_empty(c_queue *This) 
{
	return (This->m_Queue.pHead ? ZOE_FALSE : ZOE_TRUE);
}



uint32_t c_queue_get_queue_level(c_queue *This) 
{
	return (This->m_dwNbInQueue);
}



// is data already in queue?
//
zoe_bool_t c_queue_in_queue(c_queue *This, 
                            zoe_void_ptr_t Data
                            )
{
	QUEUE_ENTRY	*pthis_queue;
	zoe_bool_t	bRet = ZOE_FALSE;

	ENTER_CRITICAL(&This->m_Object)

	pthis_queue = This->m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue->Data == Data)
		{
			bRet = ZOE_TRUE;
			break;
		}
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (bRet);
}



void c_queue_walk_queue(c_queue *This)
{
	QUEUE_ENTRY	*pthis_queue;

	ENTER_CRITICAL(&This->m_Object)

	pthis_queue = This->m_Queue.pHead;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
				   "c_queue_walk_queue() ######################################\n"
				   );
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_queue::walkQueue() head = %lx, tail = %lx\n", 
				   This->m_Queue.pHead, 
				   This->m_Queue.pTail
				   );
	while (pthis_queue)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       This->m_dbgID,
					   "c_queue_walk_queue() ptr=%lx, next=%lx\n", 
					   pthis_queue, 
					   pthis_queue->pNext
					   );
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)
}

