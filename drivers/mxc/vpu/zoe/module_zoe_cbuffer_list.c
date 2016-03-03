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
// zoe_cbuffer_list.c
//
// Description: 
//
//  Simple linked list to manage data buffers
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_cbuffer_list.h"
#include "zoe_sosal.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD
#include "zoe_module_mgr.h"
#include "zoe_module_objids.h"


/////////////////////////////////////////////////////////////////////////////
//
//
static void c_buffer_list_add_buffer_to_list(ZOE_MODULE_BUFFER **p_list, 
                                             ZOE_MODULE_BUFFER *p_buffer
                                             )
{
    ZOE_MODULE_BUFFER   *p_temp;

    // reset buffer links
    p_buffer->p_next = ZOE_NULL;
    p_buffer->p_prev = ZOE_NULL;

    if (!p_list)
    {
	    zoe_dbg_printf_nc(ZOE_DBG_LVL_ERROR,
		                  "c_buffer_list_add_buffer_to_list p_list address NULL???\n"
		                  );
        return;
    }

    // add to the list
    if (*p_list)
    {
        p_temp = *p_list;
        while (p_temp)
        {
            if (p_temp->p_next)
            {
                p_temp = p_temp->p_next;
            }
            else
            {
                p_temp->p_next = p_buffer;
                p_buffer->p_prev = p_temp;
                break;
            }
        }
    }
    else
    {
        *p_list = p_buffer;
    }
}



static ZOE_MODULE_BUFFER *c_buffer_list_get_buffer_from_list(ZOE_MODULE_BUFFER **p_list)
{
    ZOE_MODULE_BUFFER   *p_buffer = ZOE_NULL;

    if (!p_list)
    {
	    zoe_dbg_printf_nc(ZOE_DBG_LVL_ERROR,
		                  "c_buffer_list_get_buffer_from_list p_list address NULL???\n"
		                  );
        return (p_buffer);
    }

    if (*p_list)
    {
        p_buffer = *p_list;
        *p_list = p_buffer->p_next;
        p_buffer->p_next = ZOE_NULL;
        p_buffer->p_prev = ZOE_NULL;
        if (*p_list)
        {
            (*p_list)->p_prev = ZOE_NULL;
        }
    }
    return (p_buffer);
}



/////////////////////////////////////////////////////////////////////////////
//
//

c_buffer_list * c_buffer_list_constructor(c_buffer_list *p_list,
                                          c_object *p_parent, 
                                          uint32_t attributes,
                                          uint32_t nb_buffer,
                                          uint32_t size,                                          
                                          zoe_dbg_comp_id_t dbg_id
                                          )
{
    if (p_list)
    {
        uint32_t            i;
        ZOE_MODULE_BUFFER   *p_entry;

        // c_object
        c_object_constructor(&p_list->m_object,
                             p_parent,
                             OBJECT_BUFFER_LIST,
                             attributes
                             );
        // c_buffer_list
        p_list->m_dbg_id = dbg_id;
        p_list->m_p_free_list = ZOE_NULL;
        p_list->m_p_used_list = ZOE_NULL;
        p_list->m_p_partial_buffer = ZOE_NULL;

        // create all the buffers and put them to the free list
        for (i = 0; i < nb_buffer; i++)
        {
            // alloc buffer entry
            //
            p_entry = zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                             sizeof(ZOE_MODULE_BUFFER), 
                                             0
                                             );
            if (p_entry)
            {
                memset(p_entry, 
                       0, 
                       sizeof(ZOE_MODULE_BUFFER)
                       );

                // alloc buffer
                //
                p_entry->p_buffer = (zoe_dev_mem_t)((zoe_uintptr_t)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,//ZOE_SOSAL_MEMORY_POOLS_SHARED, 
                                                                                          size, 
                                                                                          zoe_sosal_memory_cacheline_log2()
                                                                                          ));
                if (p_entry->p_buffer)
                {
                    p_entry->size = size;
                    c_buffer_list_add_buffer_to_list(&p_list->m_p_free_list, 
                                                     p_entry
                                                     );
                    p_list->m_nb_buffer++;
                }
                else
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   dbg_id,
		                           "c_buffer_list_constructor zoe_sosal_memory_alloc() buffer FAILED\n"
		                           );
                    zoe_sosal_memory_free(p_entry);
                    break;
                }
            }
            else
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               dbg_id,
		                       "c_buffer_list_constructor zoe_sosal_memory_alloc() entry FAILED\n"
		                       );
                break;
            }
        }

        if (!p_list->m_nb_buffer)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
					       "c_buffer_list_constructor() - no buffer\n"
					       );
            c_buffer_list_destructor(p_list);
            p_list = ZOE_NULL;
        }
    }
    return (p_list);
}



void c_buffer_list_destructor(c_buffer_list *This)
{
    ZOE_MODULE_BUFFER   *p_temp;

    // delete all the buffers
    while (This->m_p_used_list)
    {
        p_temp = This->m_p_used_list;
        // move to the next one
        This->m_p_used_list = p_temp->p_next;
        // free the buffer
        if (p_temp->p_buffer)
        {
            zoe_sosal_memory_free((void *)((zoe_uintptr_t)p_temp->p_buffer));
        }
        zoe_sosal_memory_free(p_temp);
    }
    if (This->m_p_partial_buffer)
    {
        if (This->m_p_partial_buffer->p_buffer)
        {
            zoe_sosal_memory_free((void *)((zoe_uintptr_t)This->m_p_partial_buffer->p_buffer));
        }
        zoe_sosal_memory_free(This->m_p_partial_buffer);
    }
    while (This->m_p_free_list)
    {
        p_temp = This->m_p_free_list;
        // move to the next one
        This->m_p_free_list = p_temp->p_next;
        // free the buffer
        if (p_temp->p_buffer)
        {
            zoe_sosal_memory_free((void *)((zoe_uintptr_t)p_temp->p_buffer));
        }
        zoe_sosal_memory_free(p_temp);
    }

    c_object_destructor(&This->m_object);
}



void c_buffer_list_flush(c_buffer_list *This)
{
    ZOE_MODULE_BUFFER   *p_temp;

    ENTER_CRITICAL(&This->m_object)

    // take buffer from the used list
    while ((p_temp = c_buffer_list_get_buffer_from_list(&This->m_p_used_list)))
    {
        p_temp->rel_size = 0;
        p_temp->alloc_size = 0;
        // put it back to the free list
        c_buffer_list_add_buffer_to_list(&This->m_p_free_list, 
                                         p_temp
                                         );
    }

    if (This->m_p_partial_buffer)
    {
        This->m_p_partial_buffer->rel_size = 0;
        This->m_p_partial_buffer->alloc_size = 0;
        // put it back to the free list
        c_buffer_list_add_buffer_to_list(&This->m_p_free_list, 
                                         This->m_p_partial_buffer
                                         );
        This->m_p_partial_buffer = ZOE_NULL;
    }

    This->m_total_alloc = 0;
    This->m_total_rel = 0;

    LEAVE_CRITICAL(&This->m_object)
}



zoe_dev_mem_t c_buffer_list_allocate_buffer(c_buffer_list *This, 
                                            uint32_t size, 
                                            uint32_t *p_size_got
                                            )
{
    ZOE_MODULE_BUFFER   *p_entry;
    uint32_t            alloc = 0;
    zoe_dev_mem_t       ptr = 0;

    ENTER_CRITICAL(&This->m_object)

    if (This->m_p_partial_buffer)
    {
        alloc = ZOE_MIN(This->m_p_partial_buffer->size - This->m_p_partial_buffer->alloc_size, size);
        ptr = This->m_p_partial_buffer->p_buffer + This->m_p_partial_buffer->alloc_size;
        This->m_p_partial_buffer->alloc_size += alloc;
        if (This->m_p_partial_buffer->alloc_size == This->m_p_partial_buffer->size)
        {
            // put it to the used list
            c_buffer_list_add_buffer_to_list(&This->m_p_used_list, 
                                             This->m_p_partial_buffer
                                             );
            This->m_p_partial_buffer = ZOE_NULL;
        }
    }
    else
    {
        if ((p_entry = c_buffer_list_get_buffer_from_list(&This->m_p_free_list)))
        {
            alloc = ZOE_MIN(p_entry->size, size);
            ptr = p_entry->p_buffer;
            p_entry->alloc_size = alloc;
            if (p_entry->alloc_size == p_entry->size)
            {
                // put it to the used list
                c_buffer_list_add_buffer_to_list(&This->m_p_used_list, 
                                                 p_entry
                                                 );
            }
            else
            {
                This->m_p_partial_buffer = p_entry;
            }
        }
    }

    This->m_total_alloc += alloc;

    LEAVE_CRITICAL(&This->m_object)

    *p_size_got = alloc;
    return (ptr);
}



zoe_errs_t c_buffer_list_release_buffer(c_buffer_list *This, 
                                        zoe_dev_mem_t ptr, 
                                        uint32_t size
                                        )
{
    zoe_errs_t          err = ZOE_ERRS_NOTFOUND;
    ZOE_MODULE_BUFFER   *p_entry = ZOE_NULL;
    zoe_dev_mem_t       end = ptr + size;

    ENTER_CRITICAL(&This->m_object)

    This->m_total_rel += size;

    // first look in the used list
    p_entry = This->m_p_used_list;
    while (p_entry)
    {
        if ((ptr >= p_entry->p_buffer) &&
            (end <= p_entry->p_buffer + p_entry->size) 
            )
        {
            break;
        }
        // move to the next one
        p_entry = p_entry->p_next;
    }

    // check the partil entry if not found in the used list
    if (!p_entry)
    {
        if ((ptr >= This->m_p_partial_buffer->p_buffer) &&
            (end <= This->m_p_partial_buffer->p_buffer + This->m_p_partial_buffer->alloc_size)
            )
        {
            p_entry = This->m_p_partial_buffer;
        }
    }

    if (p_entry)
    {
        if ((p_entry->rel_size + size) > p_entry->alloc_size)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                           This->m_dbg_id,
		                   "c_buffer_list_release_buffer(%s) - size overflow rel_size(%d) alloc_size(%d) this_rel(%d)\n",
                           zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent))),
                           p_entry->rel_size,
                           p_entry->alloc_size,
                           size                           
				           );
            // release the whole buffer
            p_entry->rel_size = p_entry->alloc_size;
        }
        else
        {
            //add rel_size
            p_entry->rel_size += size;
        }

        // check if the entire buffer has been released
        if (p_entry->rel_size == p_entry->alloc_size)
        {
            if (p_entry == This->m_p_partial_buffer)
            {
                This->m_p_partial_buffer = ZOE_NULL;
            }
            else
            {
                // get this out of the used list
                if (This->m_p_used_list == p_entry)
                {
                    // top of the list
                    This->m_p_used_list = p_entry->p_next;
                    if (This->m_p_used_list)
                    {
                        This->m_p_used_list->p_prev = ZOE_NULL;
                    }
                }
                else
                {
                    p_entry->p_prev->p_next = p_entry->p_next;
                    if (p_entry->p_next)
                    {
                        p_entry->p_next->p_prev = p_entry->p_prev;
                    }
                }
            }

            // reset both the release size and the alloc size
            p_entry->rel_size = 0;
            p_entry->alloc_size = 0;

            // move this to free list
            c_buffer_list_add_buffer_to_list(&This->m_p_free_list, 
                                             p_entry
                                             );
        }

        err = ZOE_ERRS_SUCCESS;
    }

    LEAVE_CRITICAL(&This->m_object)

    if (ZOE_FAIL(err))
    {
        zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       This->m_dbg_id,
		               "c_buffer_list_release_buffer(%s) - NOT FOUND! %d\n",
                       zoe_module_get_name(ZOE_MODULE_ID(c_object_who_am_i(This->m_object.m_pParent))),
                       err
				       );
    }
    return (err);
}


