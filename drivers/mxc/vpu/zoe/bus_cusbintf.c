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
// cusbintf.c
//
// Description: 
//
//  stub USB bus interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "cusbintf.h"

/////////////////////////////////////////////////////////////////////////////
//
//

// CUsbInterface
//

zoe_errs_t CUsbInterface_UsbCallBusDriver(CUsbInterface *This,
                                          zoe_void_ptr_t pUrb
                                          )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t CUsbInterface_UsbSendCmd(CUsbInterface *This,
                                    uint32_t dwPipeNum, 
                                    zoe_void_ptr_t pCmd, 
                                    uint32_t dwSize,
                                    uint32_t* pdwXfered
                                    )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t CUsbInterface_UsbAsyncIo(CUsbInterface *This,
                                    PZVUSB_IO_CONTEXT pIoContext
                                    )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t CUsbInterface_UsbResetPipe(CUsbInterface *This,
                                      uint32_t dwPipeNum
                                      )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t CUsbInterface_UsbAbortPipe(CUsbInterface *This,
                                      uint32_t dwPipeNum
                                      )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_bool_t CUsbInterface_UsbIsDataPipeInput(CUsbInterface *This,
                                            uint32_t dwPipe
                                            )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t CUsbInterface_StartDevice(CUsbInterface *This,
                                     zoe_void_ptr_t pBusData,
                                     uint32_t nBusDataSize
                                     )
{
    return (ZOE_ERRS_NOTIMPL);
}



zoe_errs_t CUsbInterface_StopDevice(CUsbInterface *This)
{
    return (ZOE_ERRS_NOTIMPL);
}



void CUsbInterface_InitializeDeviceState(CUsbInterface *This)
{
}



CUsbInterface * CUsbInterface_Constructor(CUsbInterface *pUsbInterface,
										  c_object *pParent,
										  uint32_t dwAttributes,
										  zoe_void_ptr_t pDO,
										  zoe_void_ptr_t pDOLayered,
								          ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								          zoe_void_ptr_t pBusCallbackContext,
                                          zoe_dbg_comp_id_t dbgID
										  )
{
    return (ZOE_NULL);
}



void CUsbInterface_Destructor(CUsbInterface *This)
{
}


