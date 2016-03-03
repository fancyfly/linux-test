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
// zoe_cbuffer_list.h
//
// Description: 
//
//	Simple linked list to manage data buffers
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_CBUFFER_LIST_H__
#define __ZOE_CBUFFER_LIST_H__


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

#ifndef __CBUFFER_LIST_FWD_DEFINED__
#define __CBUFFER_LIST_FWD_DEFINED__
typedef struct c_buffer_list c_buffer_list;
typedef struct ZOE_MODULE_BUFFER ZOE_MODULE_BUFFER;
#endif // !__CBUFFER_LIST_FWD_DEFINED__



/////////////////////////////////////////////////////////////////////////////
//
//

struct ZOE_MODULE_BUFFER
{
    zoe_dev_mem_t       p_buffer;
    uint32_t            size;
    uint32_t            alloc_size;
    uint32_t            rel_size;
    ZOE_MODULE_BUFFER   *p_prev;
    ZOE_MODULE_BUFFER   *p_next;
};


/////////////////////////////////////////////////////////////////////////////
//
//

struct c_buffer_list
{
    // c_object
    //
    c_object             m_object;

    // c_buffer_list
    //
    zoe_dbg_comp_id_t   m_dbg_id;
    ZOE_MODULE_BUFFER   *m_p_free_list;
    ZOE_MODULE_BUFFER   *m_p_used_list;
    ZOE_MODULE_BUFFER   *m_p_partial_buffer;
    uint32_t            m_nb_buffer;
    uint32_t            m_total_alloc;
    uint32_t            m_total_rel;
};


/////////////////////////////////////////////////////////////////////////////
//
//

c_buffer_list * c_buffer_list_constructor(c_buffer_list *p_list,
                                          c_object *p_parent, 
                                          uint32_t attributes,
                                          uint32_t nb_buffer,
                                          uint32_t size,
                                          zoe_dbg_comp_id_t dbg_id
                                          );
void c_buffer_list_destructor(c_buffer_list *This);
void c_buffer_list_flush(c_buffer_list *This);
zoe_dev_mem_t c_buffer_list_allocate_buffer(c_buffer_list *This, 
                                            uint32_t size, 
                                            uint32_t *p_size_got
                                            );
zoe_errs_t c_buffer_list_release_buffer(c_buffer_list *This, 
                                        zoe_dev_mem_t ptr, 
                                        uint32_t size
                                        );

#ifdef __cplusplus
}
#endif //__cplusplus


/////////////////////////////////////////////////////////////////////////////
//
//

// C++ defination, visible only to C++ code
//

#ifdef __cplusplus


#endif //__cplusplus

#endif //__ZOE_CBUFFER_LIST_H__

