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
// zoe_module_mgr.h
//
// Description: 
//
//  ZOE module manager
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_MODULE_MGR_H__
#define __ZOE_MODULE_MGR_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_dbg.h"
#include "zoe_hal.h"
#include "zoe_ipc_def.h"
#include "zoe_cqueue.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

// define module name
#define ZOE_MODULE_NAME_MGR "module_mgr"


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __ZOE_MODULE_MGR_FWD_DEFINED__
#define __ZOE_MODULE_MGR_FWD_DEFINED__
typedef struct c_zoe_module_mgr c_zoe_module_mgr;
#endif //__ZOE_MODULE_MGR_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

typedef zoe_void_ptr_t (*ZOE_MODULE_CREATE_FUNC)(IZOEHALAPI *p_hal,
                                                 zoe_dbg_comp_id_t dbg_id,
                                                 uint32_t inst
                                                 );
typedef zoe_errs_t (*ZOE_MODULE_DESTROY_FUNC)(zoe_void_ptr_t p_inst_data);


typedef struct _ZOE_MODULE_TABLE_ENTRY
{
    uint32_t                module;
    ZOE_MODULE_CREATE_FUNC  create_func;
    ZOE_MODULE_DESTROY_FUNC destroy_func;
} ZOE_MODULE_TABLE_ENTRY, *PZOE_MODULE_TABLE_ENTRY;


typedef struct _ZOE_MODULE_ENTRY
{
    uint32_t                    module;
    zoe_bool_t                  inst_set[ZOE_IPC_MAX_INST];
    zoe_void_ptr_t              inst_data[ZOE_IPC_MAX_INST];
    uint32_t                    inst_num;
    ZOE_MODULE_CREATE_FUNC      create_func;
    ZOE_MODULE_DESTROY_FUNC     destroy_func;
    struct _ZOE_MODULE_ENTRY    *p_next;
} ZOE_MODULE_ENTRY, *PZOE_MODULE_ENTRY;


/////////////////////////////////////////////////////////////////////////////
//
//

// zoe module manager
//
struct c_zoe_module_mgr
{
    // c_object
    //
    c_object                 m_object;

    // c_zoe_module_mgr
    //
    IZOEHALAPI              *m_p_hal;
    zoe_dbg_comp_id_t       m_dbg_id;    
    PZOE_MODULE_ENTRY       m_p_module_list;    // modules
    zoe_bool_t              m_intf_module_mgr_registered;

};


/////////////////////////////////////////////////////////////////////////////
//
//

c_zoe_module_mgr * c_zoe_module_mgr_constructor(c_zoe_module_mgr *p_zoe_module_mgr,
											    c_object *p_parent, 
							                    uint32_t dw_attributes,
                                                IZOEHALAPI *p_hal,
                                                zoe_dbg_comp_id_t dbg_id
							                    );
void c_zoe_module_mgr_destructor(c_zoe_module_mgr *This);

// singleton to get the module manager
//
c_zoe_module_mgr * c_zoe_module_mgr_get_module_mgr(void);

// return module name by module id
//
char * zoe_module_get_name(uint32_t module_id);

// return module instance data
//
zoe_void_ptr_t c_zoe_module_mgr_get_module_inst_data(c_zoe_module_mgr *This,
                                                     uint32_t module, 
                                                     uint32_t inst
                                                     );
// return total number of instance for a module
//
uint32_t c_zoe_module_mgr_get_module_num_instance(c_zoe_module_mgr *This, 
                                                      uint32_t module,
                                                      zoe_bool_t critical
                                                      );

// zoe module manager api
//
zoe_errs_t c_zoe_module_mgr_create_module(c_zoe_module_mgr *This,
                                          uint32_t module, 
                                          uint32_t * p_inst
                                          );
zoe_errs_t c_zoe_module_mgr_destroy_module(c_zoe_module_mgr *This,
                                           uint32_t module, 
                                           uint32_t inst
                                           );
zoe_errs_t c_zoe_module_mgr_find_module(c_zoe_module_mgr *This,
                                        uint32_t module, 
                                        uint32_t inst
                                        );
zoe_errs_t c_zoe_module_mgr_discover_module(c_zoe_module_mgr *This,
                                            uint32_t module
                                            );


#ifdef __cplusplus
}
#endif

#endif //__ZOE_IPC_SRV_H__



