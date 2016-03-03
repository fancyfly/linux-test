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
// zoe_module_data_intf.c
//
// Description:
//  ZOE module data API wrapper.
//
// Author: 
//	David Tao
//
//----------------------------------------------------------------------------

#include "zoe_module_base.h"


zoe_errs_t zoe_module_allocate_buffer_srv(
    uint32_t sel,
    uint32_t buf_sel,
    uint32_t size,
    zoe_dev_mem_t* dev_mem,
    uint32_t* size_got,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->allocate_buffer(This, 
                                 sel, 
                                 buf_sel,
                                 size, 
                                 dev_mem, 
                                 size_got
                                 );
}



int32_t zoe_module_release_buffer_srv(
    uint32_t sel,
    uint32_t buf_sel,
    zoe_dev_mem_t dev_mem,
    uint32_t size,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->release_buffer(This, 
                                sel, 
                                buf_sel,
                                dev_mem,
                                size
                                );
}



zoe_errs_t zoe_module_release_buffer_with_info_srv(
    uint32_t sel,
    uint32_t buf_sel,
    zoe_dev_mem_t dev_mem,
    uint32_t size,
    ZOE_BUFFER_INFO* buf_info,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->release_buffer_with_info(This, 
                                          sel, 
                                          buf_sel,
                                          dev_mem,
                                          size,
                                          buf_info
                                          );
}



zoe_errs_t zoe_module_allocate_yuv_buffer_srv(
    uint32_t sel,
    uint32_t num_planes,
    uint32_t size[3],
    zoe_dev_mem_t dev_mem[3],
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->allocate_yuv_buffer(This, 
                                     sel, 
                                     num_planes,
                                     size, 
                                     dev_mem
                                     );
}



zoe_errs_t zoe_module_release_yuv_buffer_srv(
    uint32_t sel,
    uint32_t num_planes,
    zoe_dev_mem_t dev_mem[3],
    uint32_t size[3],
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->release_yuv_buffer(This, 
                                    sel, 
                                    num_planes,
                                    dev_mem,
                                    size
                                    );
}



zoe_errs_t zoe_module_release_yuv_buffer_with_info_srv(
    uint32_t sel,
    uint32_t num_planes,
    zoe_dev_mem_t dev_mem[3],
    uint32_t size[3],
    ZOE_BUFFER_INFO* buf_info,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->release_yuv_buffer_with_info(This, 
                                              sel, 
                                              num_planes,
                                              dev_mem,
                                              size,
                                              buf_info
                                              );
}



int32_t zoe_module_get_mem_type_srv(
    uint32_t sel,
    uint32_t* mem_type,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->get_mem_type(This, 
                              sel, 
                              mem_type
                              );
}



zoe_errs_t zoe_module_get_mem_usage_srv(
    uint32_t sel,
    uint32_t* mem_usage,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->get_mem_usage(This, 
                               sel, 
                               mem_usage
                               );
}



zoe_errs_t zoe_module_write_srv(
    uint32_t sel,
    ZOE_BUFFER_DESCRIPTOR* buf_desc,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->write(This, 
                       sel, 
                       buf_desc
                       );
}



zoe_errs_t zoe_module_buffer_available_srv(
    uint32_t sel,
    uint32_t buf_sel,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    ZOE_BASE_CHECK_CONTEXT(This)
    return This->buffer_available(This, 
                                  sel,
                                  buf_sel
                                  );
}
