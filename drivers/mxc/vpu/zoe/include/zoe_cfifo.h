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
// zoe_cfifo.h
//
// Description: 
//
//	Header file for the fifo.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_CFIFO_H__
#define __ZOE_CFIFO_H__


#include "zoe_types.h"
#include "zoe_cobject.h"


/////////////////////////////////////////////////////////////////////////////
//
//

// C defination, visible to both C and C++ code
//

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifndef __CFIFO_FWD_DEFINED__
#define __CFIFO_FWD_DEFINED__
typedef struct c_fifo c_fifo;
#endif // !__CFIFO_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

struct c_fifo
{
    c_object	m_Object;

    uint32_t	m_dwReadPtr;			// read pointer (initial 0)
    uint32_t	m_dwWritePtr;			// write pointer (initial 0)
    int8_t	    *m_Fifo;
    uint32_t	m_dwFifoLevel;
    uint32_t	m_size;
    uint32_t	m_sizeEntry;
};


/////////////////////////////////////////////////////////////////////////////
//
//

c_fifo * c_fifo_constructor(c_fifo *pFifo,
                            c_object *pParent, 
                            uint32_t dwAttributes,
                            uint32_t size,
                            uint32_t sizeEntry
                            );
void c_fifo_destructor(c_fifo *This);
zoe_bool_t c_fifo_peek_fifo(c_fifo *This, 
                            zoe_void_ptr_t val
                            );
zoe_bool_t c_fifo_get_fifo(c_fifo *This, 
                           zoe_void_ptr_t val
                           );
zoe_bool_t c_fifo_set_fifo(c_fifo *This, 
                           zoe_void_ptr_t Data
                           );
void c_fifo_flush_fifo(c_fifo *This);
void c_fifo_update_read_ptr(c_fifo *This);
zoe_bool_t c_fifo_update_entry(c_fifo *This, 
                               zoe_void_ptr_t val
                               );
uint32_t c_fifo_get_fifo_level(c_fifo *This);
zoe_bool_t c_fifo_is_empty(c_fifo *This);

#ifdef __cplusplus
}
#endif //__cplusplus


/////////////////////////////////////////////////////////////////////////////
//
//

// C++ defination, visible only to C++ code
//

#ifdef __cplusplus

template <class T, uint32_t size> class cpp_fifo : public cpp_object
{
public:
	cpp_fifo(cpp_object *pParent)
		: cpp_object(pParent, 
					 OBJECT_FIFO, 
					 OBJECT_CRITICAL
					 )
		, m_dwReadPtr(0)
		, m_dwWritePtr(0)
		, m_dwFifoLevel(0)
		, m_size(size)
	{
	}

	cpp_fifo(cpp_object *pParent, 
			 uint32_t dwWhoAmI, 
			 uint32_t dwAttributes
			 )
		: cpp_object(pParent, 
					 dwWhoAmI, 
					 dwAttributes
					 )
		, m_dwReadPtr(0)
		, m_dwWritePtr(0)
		, m_dwFifoLevel(0)
		, m_size(size)
	{
	}

	virtual ~cpp_fifo() {}

	zoe_bool_t peek_fifo(T& val)
	{
		ENTER_CRITICAL_CPP

		zoe_bool_t ret = (m_dwReadPtr != m_dwWritePtr);
		if (ret)
			val = m_Fifo[m_dwReadPtr];

		LEAVE_CRITICAL_CPP
		return (ret);
	}

	zoe_bool_t get_fifo(T& val)
	{
		ENTER_CRITICAL_CPP

		zoe_bool_t ret = (m_dwReadPtr != m_dwWritePtr);
		if (ret)
		{
			val = m_Fifo[m_dwReadPtr];

			if (m_dwReadPtr >= m_dwWritePtr)
			{
				if (m_dwReadPtr == (m_size - 1))
					m_dwReadPtr = 0; // wrap around
				else
					m_dwReadPtr++;
			}
			else
				m_dwReadPtr++;

			// decrease number in fifo
			m_dwFifoLevel--;
		}

		LEAVE_CRITICAL_CPP
		return (ret);
	}

	zoe_bool_t set_fifo(T& Data)
	{
		ENTER_CRITICAL_CPP

		// sacrifice one fifo entry for efficency
		if (m_dwFifoLevel == m_size - 1)
		{
			LEAVE_CRITICAL_CPP

			return (ZOE_FALSE);
		}

		// save it in the fifo
		m_Fifo[m_dwWritePtr] = Data;

		// increase number of fifo
		m_dwFifoLevel++;

		// update write pointer
		if (m_dwWritePtr >= m_dwReadPtr)
		{
			if (m_dwWritePtr == (m_size - 1))
				m_dwWritePtr = 0; // wrap around
			else
				m_dwWritePtr++;
		}
		else
			m_dwWritePtr++;

		LEAVE_CRITICAL_CPP
		return (ZOE_TRUE);
	}

	void flush_fifo() 
	{
		ENTER_CRITICAL_CPP

		m_dwReadPtr = m_dwWritePtr = m_dwFifoLevel = 0;

		LEAVE_CRITICAL_CPP
	}

	void update_read_ptr()
	{
		ENTER_CRITICAL_CPP

		// m_dwReadPtr == m_dwWritePtr should never happen in this case
		if (m_dwReadPtr >= m_dwWritePtr)
		{
			if (m_dwReadPtr == (size - 1))
				m_dwReadPtr = 0; // wrap around
			else
				m_dwReadPtr++;
		}
		else
			m_dwReadPtr++;

		// decrease number in fifo
		m_dwFifoLevel--;

		LEAVE_CRITICAL_CPP
	}

	zoe_bool_t update_entry(T& val)
	{
		ENTER_CRITICAL_CPP

		zoe_bool_t ret = (m_dwReadPtr != m_dwWritePtr);
		if (ret)
		{
			m_Fifo[m_dwReadPtr] = val;
		}

		LEAVE_CRITICAL_CPP
		return (ret);
	}

	uint32_t get_fifo_level() {return (m_dwFifoLevel);}

	zoe_bool_t is_empty() {return (0 == m_dwFifoLevel);}

protected:
	uint32_t    m_dwReadPtr;			// read pointer (initial 0)
	uint32_t    m_dwWritePtr;			// write pointer (initial 0)
	T           m_Fifo[size];
	uint32_t    m_dwFifoLevel;
	uint32_t    m_size;
};

#endif //__cplusplus

#endif //__ZOE_CFIFO_H__

