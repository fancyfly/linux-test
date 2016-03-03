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
// zoe_module_mgr_intf.c
//
// Description:
//  ZOE module manager api wrapper.
//
// Author: 
//	David Tao
//
//----------------------------------------------------------------------------

#include "zoe_module_mgr.h"


zoe_errs_t zoe_module_mgr_create_module_srv(
    uint32_t module,
    uint32_t* inst,
    void * pContext
    )
{
    return c_zoe_module_mgr_create_module((c_zoe_module_mgr *)pContext,
                                          module, 
                                          inst
                                          );

}



zoe_errs_t zoe_module_mgr_destroy_module_srv(
    uint32_t module,
    uint32_t inst,
    void * pContext
    )
{
    return c_zoe_module_mgr_destroy_module((c_zoe_module_mgr *)pContext,
                                           module, 
                                           inst
                                           );

}



zoe_errs_t zoe_module_mgr_find_module_srv(
    uint32_t module,
    uint32_t inst,
    void * pContext
    )
{
    return c_zoe_module_mgr_find_module((c_zoe_module_mgr *)pContext,
                                        module, 
                                        inst
                                        );

}



zoe_errs_t zoe_module_mgr_discover_module_srv(
    uint32_t module,
    void * pContext
    )
{
    return c_zoe_module_mgr_discover_module((c_zoe_module_mgr *)pContext,
                                            module
                                            );
}

