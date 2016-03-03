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


//----------------------------------------------------------------------------
// zoe_module_connection_intf_srv.c
//
// Description:
//  ZOE module connection API wrapper
//
// Author: 
//	David Tao
//
//----------------------------------------------------------------------------

#include "zoe_module_base.h"


uint32_t zoe_module_base_get_num_ports_srv(
    void * pContext
    )
{
    return zoe_module_base_get_num_ports((c_zoe_module_base *)pContext);
}



zoe_errs_t zoe_module_base_get_port_selector_from_index_srv(
    uint32_t index,
    uint32_t* sel,
    void * pContext
    )
{
    return zoe_module_base_get_port_selector_from_index((c_zoe_module_base *)pContext, 
                                                        index, 
                                                        sel
                                                        );
}



zoe_errs_t zoe_module_base_get_port_dir_srv(
    uint32_t sel,
    uint32_t* dir,
    void * pContext
    )
{
    return zoe_module_base_get_port_dir((c_zoe_module_base *)pContext, 
                                        sel, 
                                        dir
                                        );

}



zoe_errs_t zoe_module_base_get_port_type_srv(
    uint32_t sel,
    uint32_t* type,
    uint32_t* sub_type,
    void * pContext
    )
{
    return zoe_module_base_get_port_type((c_zoe_module_base *)pContext, 
                                         sel, 
                                         type, 
                                         sub_type
                                         );
}



zoe_errs_t zoe_module_base_is_port_set_srv(
    uint32_t sel,
    zoe_bool_t* set,
    void * pContext
    )
{
    return zoe_module_base_is_port_set((c_zoe_module_base *)pContext, 
                                       sel, 
                                       set
                                       );
}



zoe_errs_t zoe_module_base_port_set_srv(
    uint32_t sel,
    ZOE_MODULE_DATA_CONNECTOR* port,
    void * pContext
    )
{
    return zoe_module_base_port_set((c_zoe_module_base *)pContext, 
                                    sel, 
                                    port
                                    );
}



zoe_errs_t zoe_module_base_port_clear_srv(
    uint32_t sel,
    void * pContext
    )
{
    return zoe_module_base_port_clear((c_zoe_module_base *)pContext, 
                                      sel
                                      );
}



zoe_errs_t zoe_module_base_port_get_srv(
    uint32_t sel,
    ZOE_MODULE_DATA_CONNECTOR* port,
    void * pContext
    )
{
    return zoe_module_base_port_get((c_zoe_module_base *)pContext, 
                                    sel, 
                                    port
                                    );
}
