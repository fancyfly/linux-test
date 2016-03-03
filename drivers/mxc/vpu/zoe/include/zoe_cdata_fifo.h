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
// zoe_cdata_fifo.h
//
// Description: 
//
//	Header file for the data fifo.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_CDATA_FIFO_H__
#define __ZOE_CDATA_FIFO_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_dbg.h"


/////////////////////////////////////////////////////////////////////////////
//
//

// C defination, visible to both C and C++ code
//

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifndef __CDATA_FIFO_FWD_DEFINED__
#define __CDATA_FIFO_FWD_DEFINED__
typedef struct c_data_fifo c_data_fifo;
#endif // !__CDATA_FIFO_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

struct c_data_fifo
{
    // c_object
    //
    c_object            m_object;

    // c_data_fifo
    //
    uint32_t            m_read_ptr;     // read pointer (initial 0)
    uint32_t            m_write_ptr;    // write pointer (initial 0)
    uint64_t            m_read_cnt;     // make it 64 bit to avoid count overflow
    uint64_t            m_write_cnt;    // make it 64 bit to avoid count overflow
    uint8_t             *m_p_buffer;
    uint32_t            m_size;
    zoe_dbg_comp_id_t   m_dbg_id;
};


/////////////////////////////////////////////////////////////////////////////
//
//

c_data_fifo * c_data_fifo_constructor(c_data_fifo *p_fifo,
                                      c_object *p_parent, 
                                      uint32_t attributes,                                      
                                      uint32_t size,
                                      zoe_dbg_comp_id_t dbg_id
                                      );
void c_data_fifo_destructor(c_data_fifo *This);
void c_data_fifo_flush(c_data_fifo *This);
uint8_t * c_data_fifo_allocate_buffer(c_data_fifo *This, 
                                      uint32_t size, 
                                      uint32_t *p_size_got
                                      );
uint8_t * c_data_fifo_allocate_buffer_dont_commit(c_data_fifo *This, 
                                                  uint32_t size, 
                                                  uint32_t *p_size_got
                                                  );
zoe_bool_t c_data_fifo_allocate_buffer_commit(c_data_fifo *This, 
                                              uint8_t *ptr,
                                              uint32_t size
                                              );
zoe_bool_t c_data_fifo_release_buffer(c_data_fifo *This, 
                                      uint8_t *ptr, 
                                      uint32_t size
                                      );
uint32_t c_data_fifo_get_fifo_level(c_data_fifo *This);
uint32_t c_data_fifo_get_available_space(c_data_fifo *This);
zoe_bool_t c_data_fifo_is_empty(c_data_fifo *This);
zoe_bool_t c_data_fifo_is_full(c_data_fifo *This);

#ifdef __cplusplus
}
#endif //__cplusplus


/////////////////////////////////////////////////////////////////////////////
//
//

// C++ defination, visible only to C++ code
//

#ifdef __cplusplus

class cpp_data_fifo : public cpp_object
{
public:
    cpp_data_fifo(cpp_object *p_parent,
                  uint8_t *p_buffer,
                  uint32_t size,
                  zoe_dbg_comp_id_t dbg_id
                  )
        : cpp_object(p_parent, 
                    OBJECT_DATA_FIFO, 
                    OBJECT_CRITICAL
                    )
        , m_read_ptr(0)
        , m_write_ptr(0)
        , m_read_cnt(0)
        , m_write_cnt(0)
        , m_p_buffer(p_buffer)
        , m_size(size)
        , m_dbg_id(dbg_id)
	{
	}

    cpp_data_fifo(cpp_object *p_parent, 
                  uint32_t attributes,
                  uint8_t *p_buffer,
                  uint32_t size,
                  zoe_dbg_comp_id_t dbg_id
                  )
        : cpp_object(p_parent, 
                     OBJECT_DATA_FIFO, 
                     attributes
                     )
        , m_read_ptr(0)
        , m_write_ptr(0)
        , m_read_cnt(0)
        , m_write_cnt(0)
        , m_p_buffer(p_buffer)
        , m_size(size)
        , m_dbg_id(dbg_id)
	{
	}

    virtual ~cpp_data_fifo() {}

	void flush() 
	{
        ENTER_CRITICAL_CPP

        m_read_ptr = m_write_ptr = 0;
        m_read_cnt = m_write_cnt = 0;

        LEAVE_CRITICAL_CPP
	}

	uint8_t * allocate_buffer(uint32_t size, 
                              uint32_t *p_size_got
                              ) 
    {
		ENTER_CRITICAL_CPP
        uint32_t max_alloc;
        uint8_t *ptr = m_p_buffer + m_write_ptr;
        if (m_write_ptr > m_read_ptr)
        {
            max_alloc = m_size - m_write_ptr;
        }
        else if (m_write_ptr < m_read_ptr)
        {
            max_alloc = m_read_ptr - m_write_ptr;
        }
        else
        {
            max_alloc = (m_read_cnt != m_write_cnt) ? 0 : (m_size - m_write_ptr);
        }
        max_alloc = ZOE_MIN(size, max_alloc);
        m_write_ptr += max_alloc;
        *p_size_got = max_alloc;
		LEAVE_CRITICAL_CPP
		return (ptr);
    }

    zoe_bool_t release_buffer(uint8_t *ptr, 
                              uint32_t size
                              )
    {
        zoe_bool_t  ret;

        if ((ptr < m_p_buffer) ||
            (ptr >=  m_p_buffer + m_size)
            )
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           m_dbg_id,
						   "cpp_data_fifo::release_buffer() - ptr out of range!\n"
						   );
            return (ZOE_FALSE);
        }

		ENTER_CRITICAL_CPP
        uint32_t rel_ptr = (uint32_t)((ptr - m_p_buffer) & 0xFFFFFFFF);
        uint32_t max_rel;

        if (rel_ptr != m_read_ptr)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           m_dbg_id,
						   "cpp_data_fifo::release_buffer() - ptr out of order!\n"
						   );
            ret = ZOE_FALSE;
        }
        else
        {
            if (m_write_ptr > m_read_ptr)
            {
                max_rel = m_write_ptr - m_read_ptr;
            }
            else if (m_write_ptr < m_read_ptr)
            {
                max_rel = m_size - m_read_ptr;
            }
            else
            {
                max_rel = (m_read_cnt == m_write_cnt) ? 0 : (m_size - m_write_ptr);
            }

            if (size > max_rel)
            {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               m_dbg_id,
						       "cpp_data_fifo::release_buffer() - size %d > max allowed %d!\n",
                               size,
                               max_rel
						       );
                ret = ZOE_FALSE;
            }
            else
            {
                m_read_ptr += size;
                if (m_read_ptr >= m_size)
                {
                    m_read_ptr = 0; // wrap around
                }
                ret = ZOE_TRUE;
            }
        }
		LEAVE_CRITICAL_CPP
		return (ret);
    }

	uint32_t get_fifo_level() 
    {
		ENTER_CRITICAL_CPP
        uint32_t ret = uint32_t((m_write_cnt - m_read_cnt) & 0xFFFFFFFF);
		LEAVE_CRITICAL_CPP
		return (ret);
    }

    zoe_bool_t is_empty() 
    {
		ENTER_CRITICAL_CPP
        zoe_bool_t ret = (m_read_cnt == m_write_cnt);
		LEAVE_CRITICAL_CPP
		return (ret);
    }
    zoe_bool_t is_full() 
    {
		ENTER_CRITICAL_CPP
        zoe_bool_t ret = (m_read_ptr == m_write_ptr) &&
                         (m_read_cnt != m_write_cnt);
		LEAVE_CRITICAL_CPP
		return (ret);
    }
protected:
    uint32_t            m_read_ptr;     // read pointer (initial 0)
    uint32_t            m_write_ptr;    // write pointer (initial 0)
    uint64_t            m_read_cnt;     // make it 64 bit to avoid count overflow
    uint64_t            m_write_cnt;    // make it 64 bit to avoid count overflow
    uint8_t             *m_p_buffer;
    uint32_t            m_size;
    zoe_dbg_comp_id_t   m_dbg_id;
};

#endif //__cplusplus

#endif //__ZOE_CDATA_FIFO_H__

