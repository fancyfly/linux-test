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
// cusbintf.h
//
// Description: 
//
//  ZOE OS independent USB bus interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////



#ifndef __CZVUSBINTF_H__
#define __CZVUSBINTF_H__

#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_cqueue.h"
#include "zoe_dbg.h"
#include "zv_busintf.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct CUsbInterface
{
    // public c_object
    //
    c_object             m_Object;

    // OS specific data
    //
    zoe_void_ptr_t      m_pPrivateData;

    // bus callback
    ZV_BUSINTF_CALLBACK m_pBusCallbackFunc;
    zoe_void_ptr_t      m_pBusCallbackContext;

    zoe_dbg_comp_id_t   m_dbgID;
} CUsbInterface;



/////////////////////////////////////////////////////////////////////////////
//
//

enum
{
    ZVUSB_DIR_OUT = 0,  // host to zv device
    ZVUSB_DIR_IN,       // zv device to host
    ZVUSB_DIR_MAX
};


// Callback function for asynchronous read/writes
//
typedef void (*ZVUSBCOMPLETION)(zoe_void_ptr_t UserContext, zoe_void_ptr_t pUserContext2);


// Context information for asynchronous read/writes
//
typedef struct _ZVUSB_IO_CONTEXT
{
    QUEUE_ENTRY         ListEntry;      // this is for dma request queue
    uint32_t            dir;            // io direction
    CUsbInterface       *pUsb;          // Ptr to the USB context used for the operation.
    uint32_t            Pipe;           // Pipe index.
    ZVUSBCOMPLETION     pCompletion;    // Pointer to the caller's completion routine.
    zoe_void_ptr_t      UserContext;    // Caller's completion context.
    uint64_t            ullStartTime;   // for profiling
    zoe_dev_mem_t       dwDevAddr;      // Device address
    uint32_t            dwXferSize;     // Amount of data to be transfer
    uint8_t             *pCallerBuffer;  // caller buffer
    zoe_sosal_obj_id_t  evtComplete;    // requester complete event
    zoe_errs_t          status;         // io status
    uint32_t        transferSize;   // actual size transfered
} ZVUSB_IO_CONTEXT, *PZVUSB_IO_CONTEXT;



/////////////////////////////////////////////////////////////////////////////
//
//
CUsbInterface * CUsbInterface_Constructor(CUsbInterface *pUsbInterface,
										  c_object *pParent,
										  uint32_t dwAttributes,
										  zoe_void_ptr_t pDO,
										  zoe_void_ptr_t pDOLayered,
								          ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								          zoe_void_ptr_t pBusCallbackContext,
                                          zoe_dbg_comp_id_t dbgID
										  );
void CUsbInterface_Destructor(CUsbInterface *This);

zoe_errs_t CUsbInterface_StartDevice(CUsbInterface *This,
                                     zoe_void_ptr_t pBusData,
                                     uint32_t nBusDataSize
                                     );
zoe_errs_t CUsbInterface_StopDevice(CUsbInterface *This);
void CUsbInterface_InitializeDeviceState(CUsbInterface *This);
zoe_errs_t CUsbInterface_UsbCallBusDriver(CUsbInterface *This,
                                          zoe_void_ptr_t pUrb
                                          );
zoe_errs_t CUsbInterface_UsbSendCmd(CUsbInterface *This,
                                    uint32_t dwPipeNum, 
                                    zoe_void_ptr_t pCmd, 
                                    uint32_t dwSize,
                                    uint32_t *pdwXfered
                                    );
zoe_errs_t CUsbInterface_UsbAsyncIo(CUsbInterface *This,
                                    PZVUSB_IO_CONTEXT pIoContext
                                    );
zoe_errs_t CUsbInterface_UsbResetPipe(CUsbInterface *This,
                                      uint32_t dwPipeNum
                                      );
zoe_errs_t CUsbInterface_UsbAbortPipe(CUsbInterface *This,
                                      uint32_t dwPipeNum
                                      );
zoe_bool_t CUsbInterface_UsbIsDataPipeInput(CUsbInterface *This,
                                            uint32_t dwPipeNum
                                            );


#ifdef __cplusplus
}
#endif

#endif //__CUSBINTF_H__



