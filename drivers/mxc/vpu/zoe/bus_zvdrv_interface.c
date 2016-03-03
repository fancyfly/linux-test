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
// zvdrv_interface.c
//
// Description: 
//
//  user mode hardware access interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zvdrv_interface.h"


zoe_errs_t zvdrv_open_device(uint32_t inst, 
                             ZOEHAL_BUS bus_type,
                             PZOE_OBJECT_HANDLE pHandle
                             )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_close_device(ZOE_OBJECT_HANDLE hDevice)
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_read_reg(ZOE_OBJECT_HANDLE hDevice, 
                          uint32_t addr, 
                          uint32_t * pData
                          )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_write_reg(ZOE_OBJECT_HANDLE hDevice, 
                           uint32_t addr, 
                           uint32_t value 
                           )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_read_reg_block(ZOE_OBJECT_HANDLE hDevice, 
                                uint32_t addr, 
                                uint32_t * pData, 
                                uint32_t numReg
                                )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_write_reg_block(ZOE_OBJECT_HANDLE hDevice, 
                                 uint32_t addr, 
                                 uint32_t * pData, 
                                 uint32_t numReg
                                 )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_read_mem(ZOE_OBJECT_HANDLE hDevice, 
                          zoe_dev_mem_t addr, 
                          uint8_t * pData, 
                          uint32_t len
                          )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_write_mem(ZOE_OBJECT_HANDLE hDevice, 
                           zoe_dev_mem_t addr, 
                           uint8_t * pData, 
                           uint32_t len
                           )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_read_mem_direct(ZOE_OBJECT_HANDLE hDevice, 
                                 zoe_dev_mem_t addr, 
                                 uint8_t * pData, 
                                 uint32_t len,
                                 uint32_t ulXferMode,
                                 zoe_bool_t bSwap,
                                 zoe_sosal_obj_id_t evt
                                 )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_write_mem_direct(ZOE_OBJECT_HANDLE hDevice, 
                                  zoe_dev_mem_t addr, 
                                  uint8_t * pData, 
                                  uint32_t len,
                                  uint32_t ulXferMode,
                                  zoe_bool_t bSwap,
                                  zoe_sosal_obj_id_t evt
                                  )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_wait_isr(ZOE_OBJECT_HANDLE hDevice, 
                          zoe_sosal_isr_sw_numbers_t from_num,
                          uint32_t timeout_ms
                          )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_enable_wait_isr(ZOE_OBJECT_HANDLE hDevice,
                                 zoe_sosal_isr_sw_numbers_t from_num
                                 )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_disable_wait_isr(ZOE_OBJECT_HANDLE hDevice,
                                  zoe_sosal_isr_sw_numbers_t from_num
                                  )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_isr_sw_trigger(ZOE_OBJECT_HANDLE hDevice,
                                zoe_sosal_isr_sw_numbers_t to_num,
                                zoe_sosal_isr_sw_numbers_t from_num
                                )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_wait_proxy_event(ZOE_OBJECT_HANDLE hDevice,
                                  uint32_t *p_event,
                                  uint32_t timeout_ms
                                  )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_set_proxy(ZOE_OBJECT_HANDLE hDevice)
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t zvdrv_term_proxy(ZOE_OBJECT_HANDLE hDevice)
{
    return (ZOE_ERRS_NOTIMPL);
}



void * zvdrv_mmap(ZOE_OBJECT_HANDLE hDevice, 
                  size_t offset, 
                  size_t size
                  )
{
    return (ZOE_NULL);
}



zoe_errs_t zvdrv_unmap(ZOE_OBJECT_HANDLE hDevice,
                       void *ptr, 
                       size_t length
                       )
{
    return (ZOE_ERRS_NOTIMPL);
}



uint32_t zvdrv_get_max_dma_size(ZOE_OBJECT_HANDLE hDevice)
{
    return (0);
}




