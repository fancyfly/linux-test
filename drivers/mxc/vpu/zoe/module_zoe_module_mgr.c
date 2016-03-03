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
// zoe_module_mgr.c
//
// Description: 
//
//  ZOE module manager
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_module_mgr.h"
#include "zoe_module_mgr_intf_srv.h"
#include "zoe_module_objids.h"
#include "zoe_ipc_srv.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD


/////////////////////////////////////////////////////////////////////////////
//
//

extern const ZOE_MODULE_TABLE_ENTRY g_module_table[];
extern uint32_t                     g_module_table_count;


/////////////////////////////////////////////////////////////////////////////
//
//

c_zoe_module_mgr * c_zoe_module_mgr_constructor(c_zoe_module_mgr *p_zoe_module_mgr,
											    c_object *p_parent, 
							                    uint32_t dw_attributes,
                                                IZOEHALAPI *p_hal,
                                                zoe_dbg_comp_id_t dbg_id
							                    )
{
    if (p_zoe_module_mgr)
    {
        zoe_errs_t  err;

        // zero init ourselves
        //
        memset((void *)p_zoe_module_mgr, 
               0, 
               sizeof(c_zoe_module_mgr)
               );

        // c_object
        //
        c_object_constructor(&p_zoe_module_mgr->m_object, 
                             p_parent, 
                             OBJECT_ZOE_MODULE_MGR,
                             dw_attributes
                             );
        // c_zoe_module_mgr
        //
        p_zoe_module_mgr->m_p_hal = p_hal;
        p_zoe_module_mgr->m_dbg_id = dbg_id;

        // register module manager interface
        err = c_zoe_ipc_service_register_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                   zoe_module_mgr_intf_interface,
                                                   ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR),
                                                   0,
                                                   (void *)p_zoe_module_mgr,
                                                   zoe_module_mgr_intf_dispatch,
                                                   ZOE_TRUE,
                                                   zoe_sosal_thread_maxpriorities_get() - 1
                                                   );
        if (ZOE_SUCCESS(err))
        {
            p_zoe_module_mgr->m_intf_module_mgr_registered = ZOE_TRUE;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbg_id,
		                   "c_zoe_module_mgr_constructor c_zoe_ipc_service_register_interface() zoe_module_mgr_intf_interface FAILED\n"
		                   );
            c_zoe_module_mgr_destructor(p_zoe_module_mgr);
            return (ZOE_NULL);
        }
    }

    return (p_zoe_module_mgr);
}



void c_zoe_module_mgr_destructor(c_zoe_module_mgr *This)
{
    PZOE_MODULE_ENTRY   p_module;
    zoe_errs_t          err;
    uint32_t            i;

    // unregister module manager api
    if (This->m_intf_module_mgr_registered)
    {
        err = c_zoe_ipc_service_unregister_interface(c_zoe_ipc_service_get_ipc_svc(), 
                                                     zoe_module_mgr_intf_interface,
                                                     ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR),
                                                     0
                                                     );
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbg_id,
		                   "c_zoe_module_mgr_destructor c_zoe_ipc_service_unregister_interface() zoe_module_mgr_intf_interface FAILED\n"
		                   );
        }
        This->m_intf_module_mgr_registered = ZOE_FALSE;
    }

    // walk the module list to delete all the remaining modules
    //
    p_module = This->m_p_module_list;

    while (p_module)
    {
        // call module destroy function for all the instances
        for (i = 0; i < ZOE_IPC_MAX_INST; i++)
        {
            if (p_module->inst_set[i])
            {
                p_module->destroy_func(p_module->inst_data[i]);
                p_module->inst_set[i] = ZOE_FALSE;
            }
        }

        // move to the next module in the list
        p_module = p_module->p_next;

        // release the memory
        zoe_sosal_memory_free(This->m_p_module_list);

        // update the list head
        This->m_p_module_list = p_module;
    }


	// c_object
	//
	c_object_destructor(&This->m_object);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// zoe module manager API
//
zoe_errs_t c_zoe_module_mgr_create_module(c_zoe_module_mgr *This,
                                          uint32_t module, 
                                          uint32_t * p_inst
                                          )
{
    PZOE_MODULE_ENTRY   p_module, p_module_prev;
    zoe_errs_t          err;
    uint32_t            i, j;

    ENTER_CRITICAL(&This->m_object)

    // search through module table to find matching module
    for (i = 0; i < g_module_table_count; i++)
    {
        if (g_module_table[i].module == module)
        {
            // module supported, now check if another module instance already exists
            p_module = p_module_prev = This->m_p_module_list;

            while (p_module)
            {
                if (p_module->module == module)
                {
                    if (p_module->inst_num < ZOE_IPC_MAX_INST)
                    {
                        for (j = 0; j < ZOE_IPC_MAX_INST; j++)
                        {
                            if (!p_module->inst_set[j])
                            {
                                // call module's create function
                                p_module->inst_data[j] = p_module->create_func(This->m_p_hal, 
                                                                               This->m_dbg_id, 
                                                                               j
                                                                               );
                                if (p_module->inst_data[j])
                                {
                                    p_module->inst_set[j] = ZOE_TRUE;
                                    p_module->inst_num++;
                                    *p_inst = j;
                                    err = ZOE_ERRS_SUCCESS;
                                }
                                else
                                {
                                    // create failed
                                    err = ZOE_ERRS_FAIL;
	                                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                                   This->m_dbg_id,
		                                           "module create_func() failed!\n"
		                                           );
                                }
                                goto c_zoe_module_mgr_create_module_exit;
                            }
                        }
                        // how do we get here?
                        err = ZOE_ERRS_INTERNAL;
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "how do we get here?\n"
		                               );
                    }
                    else
                    {
                        // no more instance
                        err = ZOE_ERRS_NOTSUPP;
	                    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                       This->m_dbg_id,
		                               "no more instance\n"
		                               );
                    }

                    goto c_zoe_module_mgr_create_module_exit;
                }

                // move to the next module in the list
                p_module_prev = p_module;
                p_module = p_module->p_next;
            }

            // first instance, add a new module
            p_module = (PZOE_MODULE_ENTRY)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                 sizeof(ZOE_MODULE_ENTRY), 
                                                                 0
                                                                 );
            if (!p_module)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
		                       "zoe_sosal_memory_alloc() failed!\n"
		                       );
                // fail to allocate memory
                err = ZOE_ERRS_NOMEMORY;
                goto c_zoe_module_mgr_create_module_exit;
            }

            // init the entry
            memset(p_module, 
                   0, 
                   sizeof(ZOE_MODULE_ENTRY)
                   );
            p_module->module = module;
            p_module->create_func = g_module_table[i].create_func;
            p_module->destroy_func = g_module_table[i].destroy_func;

            // call module's create function
            p_module->inst_data[0] = p_module->create_func(This->m_p_hal, 
                                                           This->m_dbg_id, 
                                                           0
                                                           );
            if (p_module->inst_data[0])
            {
                p_module->inst_set[0] = ZOE_TRUE;
                p_module->inst_num++;
                *p_inst = 0;
                err = ZOE_ERRS_SUCCESS;
                if (!p_module_prev)
                {
                    This->m_p_module_list = p_module;
                }
                else
                {
                    p_module_prev->p_next = p_module;
                }
            }
            else
            {
                // create failed
                err = ZOE_ERRS_FAIL;
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbg_id,
		                       "module create_func() failed!\n"
		                       );
            }
            goto c_zoe_module_mgr_create_module_exit;
        }
    }

    // module does not appear in the cpu module table
    err = ZOE_ERRS_NOTIMPL;
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbg_id,
	               "module not supported in this cpu\n"
	               );

c_zoe_module_mgr_create_module_exit:

    LEAVE_CRITICAL(&This->m_object)
    return (err);
}



zoe_errs_t c_zoe_module_mgr_destroy_module(c_zoe_module_mgr *This,
                                           uint32_t module, 
                                           uint32_t inst
                                           )
{
    PZOE_MODULE_ENTRY   p_module, p_module_prev;
    zoe_errs_t          err = ZOE_ERRS_NOTFOUND;

    if (inst >= ZOE_IPC_MAX_INST)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbg_id,
	                   "inst %d invalid\n",
                       inst
	                   );
        return (ZOE_ERRS_PARMS);
    }

    ENTER_CRITICAL(&This->m_object)

    // walk through module list
    p_module = p_module_prev = This->m_p_module_list;

    while (p_module)
    {
        if (p_module->module == module)
        {
            if (p_module->inst_set[inst])
            {
                // call module's destroy function
                err = p_module->destroy_func(p_module->inst_data[inst]);
                if (ZOE_SUCCESS(err))
                {
                    p_module->inst_set[inst] = ZOE_FALSE;
                    p_module->inst_data[inst] = ZOE_NULL;
                    p_module->inst_num--;
                }
                else
                {
                    // destroy failed
                    err = ZOE_ERRS_FAIL;
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbg_id,
	                               "module destroy_func() failed!\n"
	                               );
                }
            }
            else
            {
                // instance does not exist
                err = ZOE_ERRS_INVALID;
	            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbg_id,
	                           "module(%d) inst(%d) does not exist\n",
                               module,
                               inst
	                           );
            }
            break;
        }

        // move to the next module in the list
        p_module_prev = p_module;
        p_module = p_module->p_next;
    }

    if (ZOE_SUCCESS(err) &&
        (0 == p_module->inst_num)
        )
    {
        // no more instance for this module, remove it from the list
        // and free the memory
        //
        if (p_module != This->m_p_module_list)
        {
            p_module_prev->p_next = p_module->p_next;
        }
        else
        {
            This->m_p_module_list = p_module->p_next;
        }

        zoe_sosal_memory_free(p_module);
    }

    LEAVE_CRITICAL(&This->m_object)
    return (err);
}



zoe_errs_t c_zoe_module_mgr_find_module(c_zoe_module_mgr *This,
                                        uint32_t module, 
                                        uint32_t inst
                                        )
{
    PZOE_MODULE_ENTRY   p_module;
    zoe_errs_t          err;

    if (inst >= ZOE_IPC_MAX_INST)
    {
        return (ZOE_ERRS_PARMS);
    }

    ENTER_CRITICAL(&This->m_object)

    // point to the head of the list
    p_module = This->m_p_module_list;

    while (p_module)
    {
        if ((p_module->module == module) &&
            (p_module->inst_set[inst])
            )
        {
            err = ZOE_ERRS_SUCCESS;
            goto c_zoe_module_mgr_find_module_exit;
        }

        // move to the next one
        p_module = p_module->p_next;
    }

    err = ZOE_ERRS_NOTFOUND;

c_zoe_module_mgr_find_module_exit:

    LEAVE_CRITICAL(&This->m_object)
    return (err);
}



zoe_errs_t c_zoe_module_mgr_discover_module(c_zoe_module_mgr *This,
                                            uint32_t module
                                            )
{
    uint32_t    i;

    for (i = 0; i < g_module_table_count; i++)
    {
        if (g_module_table[i].module == module)
        {
            return (ZOE_ERRS_SUCCESS);
        }
    }
    return (ZOE_ERRS_NOTFOUND);
}


/////////////////////////////////////////////////////////////////////////////
//
//


zoe_void_ptr_t c_zoe_module_mgr_get_module_inst_data(c_zoe_module_mgr *This,
                                                     uint32_t module, 
                                                     uint32_t inst
                                                     )
{
    PZOE_MODULE_ENTRY   p_module;
    zoe_void_ptr_t      p_data = ZOE_NULL;

    if (inst >= ZOE_IPC_MAX_INST)
    {
        return (p_data);
    }

    ENTER_CRITICAL(&This->m_object)

    // point to the head of the list
    p_module = This->m_p_module_list;

    while (p_module)
    {
        if ((p_module->module == module) &&
            (p_module->inst_set[inst])
            )
        {
            p_data = p_module->inst_data[inst];
            goto c_zoe_module_mgr_get_module_inst_data_exit;
        }

        // move to the next one
        p_module = p_module->p_next;
    }

c_zoe_module_mgr_get_module_inst_data_exit:

    LEAVE_CRITICAL(&This->m_object)
    return (p_data);
}



// return total number of instance for a module
//
uint32_t c_zoe_module_mgr_get_module_num_instance(c_zoe_module_mgr *This, 
                                                      uint32_t module,
                                                      zoe_bool_t critical
                                                      )
{
    PZOE_MODULE_ENTRY   p_module;
    uint32_t            num_inst = 0;

    if (critical)
    {
        ENTER_CRITICAL(&This->m_object)
    }

    // point to the head of the list
    p_module = This->m_p_module_list;

    while (p_module)
    {
        if (p_module->module == module)
        {
            num_inst = p_module->inst_num;
            goto c_zoe_module_mgr_get_module_num_instance_exit;
        }

        // move to the next one
        p_module = p_module->p_next;
    }

c_zoe_module_mgr_get_module_num_instance_exit:

    if (critical)
    {
        LEAVE_CRITICAL(&This->m_object)
    }
    return (num_inst);
}









