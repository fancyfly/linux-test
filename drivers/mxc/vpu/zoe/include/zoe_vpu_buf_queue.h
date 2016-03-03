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
// zoe_vpu_buf_queue.h
//
// Description: 
//
//	c++ vpu buffer queue.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_VPU_BUF_QUEUE_H__
#define __ZOE_VPU_BUF_QUEUE_H__

#include "zoe_cqueue.h"
#include "zoe_module_data_intf.h"


typedef struct _VPU_BUFFER
{
    ZOE_BUFFER_DESCRIPTOR   buf_desc;
    uint32_t                id;
    zoe_bool_t              registered;
} VPU_BUFFER, *PVPU_BUFFER;


/////////////////////////////////////////////////////////////////////////////
//
//
// hide the template syntax
//
typedef QUEUE_ENTRY_CPP<VPU_BUFFER *>   VPU_BUF_ENTRY;
typedef cpp_queue<VPU_BUFFER *>         _vpu_buf_queue;

/////////////////////////////////////////////////////////////////////////////
//
//
class vpu_buf_queue
    : public _vpu_buf_queue
{
public:
	vpu_buf_queue(cpp_object *pParent, 
			      uint32_t dwWhoAmI, 
			      uint32_t dwAttributes, 
			      QUEUE_CALLBACK pFuncCallback,
                  zoe_void_ptr_t p_cb_context
			      )
        : _vpu_buf_queue(pParent,
					     dwWhoAmI, 
					     dwAttributes, 
					     pFuncCallback,
                         p_cb_context
					     )
    {
    }

	VPU_BUF_ENTRY* get_entry_by_buf_ptr(zoe_dev_mem_t buf_ptr)
    {
	    ENTER_CRITICAL_CPP

        VPU_BUF_ENTRY	*pthis_queue = m_Queue.pHead;
        VPU_BUF_ENTRY	*pprev_queue = 0;

	    while (pthis_queue)
	    {
		    if (pthis_queue->Data->buf_desc.buffers[ZOE_BUF_DATA].buf_ptr == buf_ptr)
			{
			    if (!pprev_queue)
			    {
				    m_Queue.pHead = pthis_queue->pNext;
				    if (!m_Queue.pHead)
					    m_Queue.pTail = m_Queue.pHead;
			    }
			    else
			    {
				    pprev_queue->pNext = pthis_queue->pNext;
				    if (!pthis_queue->pNext)
					    m_Queue.pTail = pprev_queue;
			    }
			    m_dwNbInQueue--;
			    break;
		    }
		    pprev_queue = pthis_queue;
		    pthis_queue = pthis_queue->pNext;
	    }

	    LEAVE_CRITICAL_CPP

	    return (pthis_queue);
    }

	VPU_BUF_ENTRY* peek_entry_by_buf_ptr(zoe_dev_mem_t buf_ptr)
    {
	    ENTER_CRITICAL_CPP

        VPU_BUF_ENTRY  *pthis_queue = m_Queue.pHead;

	    while (pthis_queue)
	    {
		    if (pthis_queue->Data->buf_desc.buffers[ZOE_BUF_DATA].buf_ptr == buf_ptr)
            {
			    break;
            }
		    pthis_queue = pthis_queue->pNext;
	    }

	    LEAVE_CRITICAL_CPP

	    return (pthis_queue);
    }

	VPU_BUF_ENTRY* get_entry_by_buf_id(uint32_t id)
    {
	    ENTER_CRITICAL_CPP

        VPU_BUF_ENTRY	*pthis_queue = m_Queue.pHead;
        VPU_BUF_ENTRY	*pprev_queue = 0;

	    while (pthis_queue)
	    {
		    if (pthis_queue->Data->id == id)
			{
			    if (!pprev_queue)
			    {
				    m_Queue.pHead = pthis_queue->pNext;
				    if (!m_Queue.pHead)
					    m_Queue.pTail = m_Queue.pHead;
			    }
			    else
			    {
				    pprev_queue->pNext = pthis_queue->pNext;
				    if (!pthis_queue->pNext)
					    m_Queue.pTail = pprev_queue;
			    }
			    m_dwNbInQueue--;
			    break;
		    }
		    pprev_queue = pthis_queue;
		    pthis_queue = pthis_queue->pNext;
	    }

	    LEAVE_CRITICAL_CPP

	    return (pthis_queue);
    }

	VPU_BUF_ENTRY* peek_entry_by_buf_id(uint32_t id)
    {
	    ENTER_CRITICAL_CPP

        VPU_BUF_ENTRY  *pthis_queue = m_Queue.pHead;

	    while (pthis_queue)
	    {
		    if (pthis_queue->Data->id == id)
            {
			    break;
            }
		    pthis_queue = pthis_queue->pNext;
	    }

	    LEAVE_CRITICAL_CPP

	    return (pthis_queue);
    }
};


#endif //__ZOE_VPU_BUF_QUEUE_H__

