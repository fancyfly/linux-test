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
// zoe_module_control_intf_srv.c
//
// Description:
//  ZOE module control interface wrapper.
//
// Author: 
//	David Tao
//
//----------------------------------------------------------------------------

#include "zoe_module_base.h"


zoe_errs_t zoe_module_base_play_srv(
    uint32_t sel,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    return This->play(This, 
                      sel
                      );
}



zoe_errs_t zoe_module_base_stop_srv(
    uint32_t sel,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    return This->stop(This, 
                      sel
                      );
}



zoe_errs_t zoe_module_base_flush_srv(
    uint32_t sel,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    return This->flush(This, 
                       sel
                       );
}



zoe_errs_t zoe_module_base_pause_srv(
    uint32_t sel,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    return This->pause(This, 
                       sel
                       );
}



zoe_state_t zoe_module_base_get_state_srv(
    uint32_t sel,
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    return This->get_state(This, 
                           sel
                           );
}



zoe_errs_t zoe_module_base_notify_srv(
    uint32_t sel,
    uint32_t evt,
    uint32_t evt_data[4],
    void * pContext
    )
{
    c_zoe_module_base *This = (c_zoe_module_base *)pContext;
    return This->evt_notify(This, 
                            sel,
                            evt,
                            evt_data
                            );
}




