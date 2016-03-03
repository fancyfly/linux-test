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
// zoe_cdata_fifo.c
//
// Description: 
//
//	C implementation for the data fifo.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_cdata_fifo.h"
#include "zoe_sosal.h"
#include "zoe_module_mgr.h"
#include "zoe_module_objids.h"
#include "zoe_module_base.h"

/////////////////////////////////////////////////////////////////////////////
//
//

//#define _DEBUG_DFIFO
#define _DEBUG_OBJECT   OBJECT_ZOE_MODULE_VIDDEC

/////////////////////////////////////////////////////////////////////////////
//
//

c_data_fifo * c_data_fifo_constructor(c_data_fifo *p_fifo,
                                      c_object *p_parent, 
                                      uint32_t attributes,                                      
                                      uint32_t size,
                                      zoe_dbg_comp_id_t dbg_id
                                      )
{
    if (p_fifo)
    {
        c_object_constructor(&p_fifo->m_object,
                             p_parent,
                             OBJECT_DATA_FIFO,
                             attributes
                             );
        p_fifo->m_read_ptr = 0;
        p_fifo->m_write_ptr = 0;
        p_fifo->m_read_cnt = 0;
        p_fifo->m_write_cnt = 0;        
        p_fifo->m_size = size;
        p_fifo->m_dbg_id = dbg_id;
        p_fifo->m_p_buffer = (uint8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,//ZOE_SOSAL_MEMORY_POOLS_SHARED,
                                                               size, 
                                                               0//zoe_sosal_memory_cacheline_log2()
                                                               );
        if (!p_fifo->m_p_buffer)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
					       "c_data_fifo_constructor() - zoe_sosal_memory_alloc failed SHARED %d\n",
                           size
					       );
            c_data_fifo_destructor(p_fifo);
            p_fifo = ZOE_NULL;
        }
    }
    return (p_fifo);
}



void c_data_fifo_destructor(c_data_fifo *This)
{
    if (This->m_p_buffer)
    {
        zoe_sosal_memory_free((void *)This->m_p_buffer);
    }
    c_object_destructor(&This->m_object);
}



void c_data_fifo_flush(c_data_fifo *This)
{
    ENTER_CRITICAL(&This->m_object)

    This->m_read_ptr = This->m_write_ptr = 0;
    This->m_read_cnt = This->m_write_cnt = 0;

    LEAVE_CRITICAL(&This->m_object)
}



uint8_t * c_data_fifo_allocate_buffer(c_data_fifo *This, 
                                      uint32_t size, 
                                      uint32_t *p_size_got
                                      )
{
    uint32_t    max_alloc;
    uint8_t     *ptr;

	ENTER_CRITICAL(&This->m_object)

    ptr = This->m_p_buffer + This->m_write_ptr;
    if (This->m_write_ptr > This->m_read_ptr)
    {
        max_alloc = This->m_size - This->m_write_ptr;
    }
    else if (This->m_write_ptr < This->m_read_ptr)
    {
        max_alloc = This->m_read_ptr - This->m_write_ptr;
    }
    else
    {
        max_alloc = (This->m_read_cnt != This->m_write_cnt) ? 0 : (This->m_size - This->m_write_ptr);
    }
    max_alloc = ZOE_MIN(size, max_alloc);
#ifdef _DEBUG_DFIFO
    if (c_object_who_am_i(This->m_object.m_pParent) == _DEBUG_OBJECT)
    {
	    c_zoe_module_base *p_base = GET_INHERITED_OBJECT(c_zoe_module_base, m_object, This->m_object.m_pParent);
        MODULE_PRINTF("alloc_%d_%X_%d\r\n", p_base->m_inst, This->m_write_ptr, max_alloc);
    }
#endif //_DEBUG_DFIFO
    This->m_write_ptr += max_alloc;
    This->m_write_cnt += max_alloc;
    if (This->m_write_ptr >= This->m_size)
    {
        This->m_write_ptr = 0; // wrap around
    }
    *p_size_got = max_alloc;
	LEAVE_CRITICAL(&This->m_object)
    //MODULE_PRINTF("alloc_%d\r\n", max_alloc);
	return (ptr);
}



uint8_t * c_data_fifo_allocate_buffer_dont_commit(c_data_fifo *This, 
                                                  uint32_t size, 
                                                  uint32_t *p_size_got
                                                  )
{
    uint32_t    max_alloc;
    uint8_t     *ptr;

	ENTER_CRITICAL(&This->m_object)

    ptr = This->m_p_buffer + This->m_write_ptr;
    if (This->m_write_ptr > This->m_read_ptr)
    {
        max_alloc = This->m_size - This->m_write_ptr;
    }
    else if (This->m_write_ptr < This->m_read_ptr)
    {
        max_alloc = This->m_read_ptr - This->m_write_ptr;
    }
    else
    {
        max_alloc = (This->m_read_cnt != This->m_write_cnt) ? 0 : (This->m_size - This->m_write_ptr);
    }
    max_alloc = ZOE_MIN(size, max_alloc);
#ifdef _DEBUG_DFIFO
    if (c_object_who_am_i(This->m_object.m_pParent) == _DEBUG_OBJECT)
    {
	    c_zoe_module_base *p_base = GET_INHERITED_OBJECT(c_zoe_module_base, m_object, This->m_object.m_pParent);
        MODULE_PRINTF("alloc_%d_%X_%d\r\n", p_base->m_inst, This->m_write_ptr, max_alloc);
    }
#endif //_DEBUG_DFIFO
    *p_size_got = max_alloc;
	LEAVE_CRITICAL(&This->m_object)
    //MODULE_PRINTF("alloc_%d\r\n", max_alloc);
	return (ptr);
}



zoe_bool_t c_data_fifo_allocate_buffer_commit(c_data_fifo *This, 
                                              uint8_t *ptr,
                                              uint32_t size
                                              )
{
    uint32_t    max_commit;

	ENTER_CRITICAL(&This->m_object)

    if (ptr != (This->m_p_buffer + This->m_write_ptr))
    {
        zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       This->m_dbg_id,
					   "c_data_fifo_allocate_buffer_commit(%s) - ptr out of order!\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent)))
					   );
	    LEAVE_CRITICAL(&This->m_object)
        return (ZOE_FALSE);
    }

    if (This->m_write_ptr > This->m_read_ptr)
    {
        max_commit = This->m_size - This->m_write_ptr;
    }
    else if (This->m_write_ptr < This->m_read_ptr)
    {
        max_commit = This->m_read_ptr - This->m_write_ptr;
    }
    else
    {
        max_commit = (This->m_read_cnt != This->m_write_cnt) ? 0 : (This->m_size - This->m_write_ptr);
    }

    if (max_commit < size)
    {
        zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       This->m_dbg_id,
					   "c_data_fifo_allocate_buffer_commit(%s) - size out of bound!\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent)))
					   );
	    LEAVE_CRITICAL(&This->m_object)
        return (ZOE_FALSE);
    }

    This->m_write_ptr += size;
    This->m_write_cnt += size;
    if (This->m_write_ptr >= This->m_size)
    {
        This->m_write_ptr = 0; // wrap around
    }

	LEAVE_CRITICAL(&This->m_object)
    return (ZOE_TRUE);
}



zoe_bool_t c_data_fifo_release_buffer(c_data_fifo *This, 
                                      uint8_t *ptr, 
                                      uint32_t size
                                      )
{
    zoe_bool_t  ret;
    uint32_t    rel_ptr;
    uint32_t    max_rel;


    if ((ptr < This->m_p_buffer) ||
        (ptr >=  This->m_p_buffer + This->m_size)
        )
    {
        zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       This->m_dbg_id,
					   "cpp_data_fifo::release_buffer(%s) - ptr out of range!\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent)))
					   );
        return (ZOE_FALSE);
    }

	ENTER_CRITICAL(&This->m_object)

    rel_ptr = (uint32_t)((ptr - This->m_p_buffer) & 0xFFFFFFFF);

    if (rel_ptr != This->m_read_ptr)
    {
        zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       This->m_dbg_id,
					   "cpp_data_fifo::release_buffer(%s) - ptr out of order! rel_ptr(0x%x) != m_read_ptr(0x%x)\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent))),
                       rel_ptr,
                       This->m_read_ptr
					   );
        ret = ZOE_FALSE;
    }
    else
    {
        if (This->m_write_ptr > This->m_read_ptr)
        {
            max_rel = This->m_write_ptr - This->m_read_ptr;
        }
        else if (This->m_write_ptr < This->m_read_ptr)
        {
            max_rel = This->m_size - This->m_read_ptr;
        }
        else
        {
            max_rel = (This->m_read_cnt == This->m_write_cnt) ? 0 : (This->m_size - This->m_write_ptr);
        }

        if (size > max_rel)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                           This->m_dbg_id,
					       "cpp_data_fifo::release_buffer(%s) - size %d > max allowed %d!\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent))),
                           size,
                           max_rel
					       );
            ret = ZOE_FALSE;
        }
        else
        {
            This->m_read_ptr += size;
            This->m_read_cnt += size;
            if (This->m_read_ptr >= This->m_size)
            {
                This->m_read_ptr = 0; // wrap around
            }
            ret = ZOE_TRUE;
#ifdef _DEBUG_DFIFO
            if (c_object_who_am_i(This->m_object.m_pParent) == _DEBUG_OBJECT)
            {
	            c_zoe_module_base *p_base = GET_INHERITED_OBJECT(c_zoe_module_base, m_object, This->m_object.m_pParent);
                MODULE_PRINTF("rel_%d_%X_%d\r\n", p_base->m_inst, rel_ptr, size);
            }
#endif //_DEBUG_DFIFO
        }
    }
	LEAVE_CRITICAL(&This->m_object)
	return (ret);
}



uint32_t c_data_fifo_get_fifo_level(c_data_fifo *This)
{
    uint32_t    ret;

	ENTER_CRITICAL(&This->m_object)
    ret = (uint32_t)((This->m_write_cnt - This->m_read_cnt) & 0xFFFFFFFF);
	LEAVE_CRITICAL(&This->m_object)
	return (ret);
}



uint32_t c_data_fifo_get_available_space(c_data_fifo *This)
{
    uint32_t    ret;

	ENTER_CRITICAL(&This->m_object)
    ret = (uint32_t)((This->m_write_cnt - This->m_read_cnt) & 0xFFFFFFFF);
    ret = This->m_size - ret;
	LEAVE_CRITICAL(&This->m_object)
	return (ret);
}



zoe_bool_t c_data_fifo_is_empty(c_data_fifo *This)
{
    uint32_t    ret;

	ENTER_CRITICAL(&This->m_object)
    ret = (This->m_read_cnt == This->m_write_cnt);
	LEAVE_CRITICAL(&This->m_object)
	return (ret);
}



zoe_bool_t c_data_fifo_is_full(c_data_fifo *This)
{
    uint32_t    ret;

	ENTER_CRITICAL(&This->m_object)
    ret = (This->m_read_ptr == This->m_write_ptr) &&
          (This->m_read_cnt != This->m_write_cnt);
	LEAVE_CRITICAL(&This->m_object)
	return (ret);
}
