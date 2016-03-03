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
// zoe_cqueue.h
//
// Description: 
//
//	Header that defines the queue.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_CQUEUE_H__
#define __ZOE_CQUEUE_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_dbg.h"



typedef zoe_errs_t (*QUEUE_CALLBACK)(zoe_void_ptr_t p_context, zoe_void_ptr_t p_param);


/////////////////////////////////////////////////////////////////////////////
//
//

// C defination, visible to both C and C++ code
//

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#ifndef __CQUEUE_FWD_DEFINED__
#define __CQUEUE_FWD_DEFINED__
typedef struct c_queue c_queue;
typedef struct QUEUE_ENTRY QUEUE_ENTRY;
typedef struct QUEUE_LIST QUEUE_LIST;
#endif // !__CQUEUE_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

struct QUEUE_ENTRY
{
    QUEUE_ENTRY         *pNext;
    zoe_void_ptr_t      Data;
};


struct QUEUE_LIST
{
    QUEUE_ENTRY         *pHead;             // head of the list
    QUEUE_ENTRY         *pTail;             // tail of the list
};


struct c_queue
{
    // c_object
    //
    c_object            m_Object;

    // c_queue
    //
    QUEUE_LIST          m_Queue;            // queue list
    uint32_t            m_dwNbInQueue;      // nb of items in the queue
	QUEUE_CALLBACK      m_pFuncCallback;    // callback
    zoe_void_ptr_t      m_p_cb_context;     // callback context

    zoe_dbg_comp_id_t   m_dbgID;
};



/////////////////////////////////////////////////////////////////////////////
//
//

c_queue * c_queue_constructor(c_queue *pQueue,
							  c_object *pParent, 
							  uint32_t dwAttributes,
					          QUEUE_CALLBACK pFuncCallback,
                              zoe_void_ptr_t p_cb_context,
                              zoe_dbg_comp_id_t dbgID
							  );
void c_queue_destructor(c_queue *This);
// add one entry to the end of the queue
//
void c_queue_add_entry(c_queue *This, 
					 QUEUE_ENTRY *pEntry
					 );
// add one entry to the top of the queue
//
void c_queue_add_entry_to_top(c_queue *This, 
						      QUEUE_ENTRY *pEntry
						      );
// remove the specified entry from the queue
//
QUEUE_ENTRY* c_queue_remove_entry(c_queue *This, 
								  QUEUE_ENTRY *pEntry
								  );
// get one entry actually remove the first entry from the queue.
//
QUEUE_ENTRY* c_queue_get_one_entry(c_queue *This);
// return the entry without remove it from the queue
//
QUEUE_ENTRY* c_queue_peek_entry_by_data(c_queue *This, 
									    zoe_void_ptr_t Data
									    );
// remove the specified entry from the queue
//
QUEUE_ENTRY* c_queue_get_entry_by_data(c_queue *This, 
								       zoe_void_ptr_t Data
								       );
QUEUE_ENTRY* c_queue_peek_one_entry(c_queue *This);
QUEUE_ENTRY* c_queue_peek_entry_by_index(c_queue *This, 
									     uint32_t index
									     );
void c_queue_reset_queue(c_queue *This);
// flush the queue
//
void c_queue_flush_queue(c_queue *This);
zoe_bool_t c_queue_is_empty(c_queue *This);
uint32_t c_queue_get_queue_level(c_queue *This);
// is data already in queue?
//
zoe_bool_t c_queue_in_queue(c_queue *This, 
						    zoe_void_ptr_t Data
						    );
void c_queue_walk_queue(c_queue *This);

#ifdef __cplusplus
}
#endif //__cplusplus



/////////////////////////////////////////////////////////////////////////////
//
//

// C++ defination, visible only to C++ code
//

#ifdef __cplusplus

/////////////////////////////////////////////////////////////////////////////
//
//

template <class T> class QUEUE_ENTRY_CPP
{
public:
	QUEUE_ENTRY_CPP() 
		: pNext(ZOE_NULL) 
	{
	}
	virtual ~QUEUE_ENTRY_CPP() {}

	QUEUE_ENTRY_CPP	*pNext;
	T				Data;
};



/////////////////////////////////////////////////////////////////////////////
//
//

template <class T> class cpp_queue : public cpp_object
{
public:
	cpp_queue(cpp_object *pParent, 
			  QUEUE_CALLBACK pFuncCallback,
              zoe_void_ptr_t p_cb_context
			  );
	cpp_queue(cpp_object *pParent, 
			  uint32_t dwWhoAmI, 
			  uint32_t dwAttributes, 
			  QUEUE_CALLBACK pFuncCallback,
              zoe_void_ptr_t p_cb_context
			  );
	virtual ~cpp_queue();

	virtual void add_entry(QUEUE_ENTRY_CPP<T> *pEntry);
	virtual void add_entry_to_top(QUEUE_ENTRY_CPP<T> *pEntry);
	QUEUE_ENTRY_CPP<T>* remove_entry(QUEUE_ENTRY_CPP<T> *pEntry);
	QUEUE_ENTRY_CPP<T>* get_one_entry();
	QUEUE_ENTRY_CPP<T>* peek_entry_by_data(T& Data);
	QUEUE_ENTRY_CPP<T>* get_entry_by_data(T& Data);
	QUEUE_ENTRY_CPP<T>* peek_one_entry();
	QUEUE_ENTRY_CPP<T>* peek_entry_by_index(uint32_t index);
	void reset_queue() 
	{
		ENTER_CRITICAL_CPP
		m_Queue.pHead = m_Queue.pTail = ZOE_NULL;
		m_dwNbInQueue = 0;
		LEAVE_CRITICAL_CPP
	}
	virtual void flush_queue();
	zoe_bool_t is_empty() 
	{
		return (m_Queue.pHead ? ZOE_FALSE : ZOE_TRUE);
	}
	uint32_t get_queue_level() 
	{
		return (m_dwNbInQueue);
	}
	zoe_bool_t in_queue(T& Data);

protected:

	class QUEUE_LIST_CPP
	{
		public:
		QUEUE_ENTRY_CPP<T>	*pHead;				// head of the list
		QUEUE_ENTRY_CPP<T>	*pTail;				// tail of the list
	};

	QUEUE_LIST_CPP		m_Queue;				// queue list
	uint32_t		    m_dwNbInQueue;			// nb of items in the queue
	QUEUE_CALLBACK		m_pFuncCallback;		// callback
    zoe_void_ptr_t      m_p_cb_context;         // callback context
};


/////////////////////////////////////////////////////////////////////////////
//
//

template <class T> 
cpp_queue<T>::cpp_queue(cpp_object *pParent, 
					    QUEUE_CALLBACK pFuncCallback,
                        zoe_void_ptr_t p_cb_context
					    )
	: cpp_object(pParent, 
				 OBJECT_QUEUE,
				 OBJECT_CRITICAL
				 )
	, m_dwNbInQueue(0)
	, m_pFuncCallback(pFuncCallback)
    , m_p_cb_context(p_cb_context)
{
	reset_queue();
}



template <class T> 
cpp_queue<T>::cpp_queue(cpp_object *pParent, 
					    uint32_t dwWhoAmI, 
					    uint32_t dwAttributes, 
					    QUEUE_CALLBACK pFuncCallback,
                        zoe_void_ptr_t p_cb_context
					    )
	: cpp_object(pParent, 
				 dwWhoAmI, 
				 dwAttributes
				 )
	, m_dwNbInQueue(0)
	, m_pFuncCallback(pFuncCallback)
    , m_p_cb_context(p_cb_context)
{
	reset_queue();
}



template <class T> 
cpp_queue<T>::~cpp_queue()
{
	flush_queue();
}



// add one entry to the end of the queue
//
template <class T> 
void cpp_queue<T>::add_entry(QUEUE_ENTRY_CPP<T> *pEntry)
{
	ENTER_CRITICAL_CPP

	if (pEntry)
	{
		if (m_Queue.pTail)
		{
			m_Queue.pTail->pNext = pEntry;
			m_Queue.pTail = pEntry;
		}
		else
		{
			m_Queue.pHead = pEntry;
			m_Queue.pTail = pEntry;
		}
		pEntry->pNext = ZOE_NULL;
		m_dwNbInQueue++;
	}

	LEAVE_CRITICAL_CPP
}



// add one entry to the top of the queue
//
template <class T> 
void cpp_queue<T>::add_entry_to_top(QUEUE_ENTRY_CPP<T> *pEntry)
{
	ENTER_CRITICAL_CPP

	if (pEntry)
	{
        pEntry->pNext = m_Queue.pHead;
        m_Queue.pHead = pEntry;

	    if (!m_Queue.pTail)
	    {
		    m_Queue.pTail = pEntry;
	    }
	    m_dwNbInQueue++;
	}

	LEAVE_CRITICAL_CPP
}



// remove the specified entry from the queue
//
template <class T> 
QUEUE_ENTRY_CPP<T>* cpp_queue<T>::remove_entry(QUEUE_ENTRY_CPP<T> *pEntry)
{
	QUEUE_ENTRY_CPP<T>	*pthis_queue;
	QUEUE_ENTRY_CPP<T>	*pprev_queue = ZOE_NULL;

	ENTER_CRITICAL_CPP

	pthis_queue = m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue == pEntry)
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



// get one entry actually remove the first entry from the queue.
//
template <class T> 
QUEUE_ENTRY_CPP<T>* cpp_queue<T>::get_one_entry()
{
	QUEUE_ENTRY_CPP<T>	*pEntry = ZOE_NULL;

	ENTER_CRITICAL_CPP

	if (m_Queue.pHead)
	{
		pEntry = m_Queue.pHead;
		m_Queue.pHead = m_Queue.pHead->pNext;
		if (!m_Queue.pHead)
			m_Queue.pTail = m_Queue.pHead;
		m_dwNbInQueue--;
	}

	LEAVE_CRITICAL_CPP

	return (pEntry);
}



// return the entry without remove it from the queue
//
template <class T> 
QUEUE_ENTRY_CPP<T>* cpp_queue<T>::peek_entry_by_data(T& Data)
{
	QUEUE_ENTRY_CPP<T>	*pthis_queue;
	QUEUE_ENTRY_CPP<T>	*pRet = ZOE_NULL;
		  
	ENTER_CRITICAL_CPP

	pthis_queue = m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue->Data == Data)
		{
			pRet = pthis_queue;
			break;
		}
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL_CPP

	return (pRet);
}



// remove the specified entry from the queue
//
template <class T> 
QUEUE_ENTRY_CPP<T>* cpp_queue<T>::get_entry_by_data(T& Data)
{
	QUEUE_ENTRY_CPP<T>	*pthis_queue;
	QUEUE_ENTRY_CPP<T>	*pprev_queue = ZOE_NULL;

	ENTER_CRITICAL_CPP

	pthis_queue = m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue->Data == Data)
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



template <class T> 
QUEUE_ENTRY_CPP<T>* cpp_queue<T>::peek_one_entry()
{
	QUEUE_ENTRY_CPP<T>	*pEntry;

	ENTER_CRITICAL_CPP

	pEntry = m_Queue.pHead;

	LEAVE_CRITICAL_CPP

	return (pEntry);
}



template <class T>
QUEUE_ENTRY_CPP<T>* cpp_queue<T>::peek_entry_by_index(uint32_t index)
{
	QUEUE_ENTRY_CPP<T>	*pthis_queue;
	QUEUE_ENTRY_CPP<T>	*pRet = ZOE_NULL;
	uint32_t        count = 0;
		  
	ENTER_CRITICAL_CPP

	pthis_queue = m_Queue.pHead;

	while (pthis_queue)
	{
		if (count++ == index)
		{
			pRet = pthis_queue;
			break;
		}
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL_CPP

	return (pRet);
}



// flush the queue
//
template <class T> 
void cpp_queue<T>::flush_queue()
{
	QUEUE_ENTRY_CPP<T>	*pthis_queue;

	ENTER_CRITICAL_CPP

	while (m_Queue.pHead)
	{
		pthis_queue = m_Queue.pHead;
		m_Queue.pHead = pthis_queue->pNext;
		m_dwNbInQueue--;

		if (m_pFuncCallback && pthis_queue->Data)
		{
			m_pFuncCallback(m_p_cb_context, (zoe_void_ptr_t)pthis_queue->Data);
		}
	}

	m_Queue.pTail = ZOE_NULL;

	LEAVE_CRITICAL_CPP
}



// is data already in queue?
//
template <class T> 
zoe_bool_t cpp_queue<T>::in_queue(T& Data)
{
	QUEUE_ENTRY_CPP<T>	*pthis_queue;
	zoe_bool_t			bRet = ZOE_FALSE;

	ENTER_CRITICAL_CPP

	pthis_queue = m_Queue.pHead;

	while (pthis_queue)
	{
		if (pthis_queue->Data == Data)
		{
			bRet = ZOE_TRUE;
			break;
		}
		pthis_queue = pthis_queue->pNext;
	}

	LEAVE_CRITICAL_CPP

	return (bRet);
}



/////////////////////////////////////////////////////////////////////////////
//
//

template <class T> class cpp_stack : public cpp_queue<T>
{
public:
	cpp_stack(cpp_object *pParent, 
			  QUEUE_CALLBACK pFuncCallback
			  )
        : cpp_queue<T>(pParent,
                       OBJECT_STACK,
                       OBJECT_CRITICAL,
					   pFuncCallback
					   )
    {
    }
	
	cpp_stack(cpp_object *pParent, 
			  uint32_t dwWhoAmI, 
			  uint32_t dwAttributes, 
			  QUEUE_CALLBACK pFuncCallback
			  )
        : cpp_queue<T>(pParent, 
					   dwWhoAmI, 
					   dwAttributes, 
					   pFuncCallback
					   )
    {
    }

	void push_entry(QUEUE_ENTRY_CPP<T> *pEntry)
	{

        cpp_object::enter_critical();

	    if (pEntry)
	    {
            pEntry->pNext = cpp_queue<T>::m_Queue.pHead;
            cpp_queue<T>::m_Queue.pHead = pEntry;

		    if (!cpp_queue<T>::m_Queue.pTail)
		    {
			    cpp_queue<T>::m_Queue.pTail = pEntry;
		    }
		    cpp_queue<T>::m_dwNbInQueue++;
	    }

        cpp_object::leave_critical();
	}

	// pop one entry out of the stack
	//
	QUEUE_ENTRY_CPP<T>* pop_entry()
	{
		return (cpp_queue<T>::get_one_entry());
	}

	virtual void add_entry(QUEUE_ENTRY_CPP<T> *pEntry)
    {
		push_entry(pEntry);
    }
};

#endif //__cplusplus

#endif //__ZOE_CQUEUE_H__

