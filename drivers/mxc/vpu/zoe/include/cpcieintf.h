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
// cpcieintf.h
//
// Description: 
//
//  ZOE OS independent PCIe bus interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////



#ifndef __CPCIEINTF_H__
#define __CPCIEINTF_H__

#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_cobject.h"
#include "zv_busintf.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct CPCIeInterface
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
} CPCIeInterface;


/////////////////////////////////////////////////////////////////////////////
//
//
CPCIeInterface * CPCIeInterface_Constructor(CPCIeInterface *pPCIeInterface,
                                            c_object *pParent,
                                            uint32_t dwAttributes,
                                            zoe_void_ptr_t pDO,
                                            zoe_void_ptr_t pDOLayered,
								            ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								            zoe_void_ptr_t pBusCallbackContext,
                                            zoe_dbg_comp_id_t dbgID
                                            );
void CPCIeInterface_Destructor(CPCIeInterface *This);

zoe_errs_t CPCIeInterface_StartDevice(CPCIeInterface *This,
                                      zoe_void_ptr_t pBusData,
                                      uint32_t nBusDataSize
                                      );
zoe_errs_t CPCIeInterface_StopDevice(CPCIeInterface *This);
void CPCIeInterface_InitializeDeviceState(CPCIeInterface *This);
zoe_errs_t CPCIeInterface_ReadReg(CPCIeInterface *This,
                                  uint32_t dwAddr,
                                  uint32_t * pData,
                                  uint32_t numReg
                                  );
zoe_errs_t CPCIeInterface_WriteReg(CPCIeInterface *This,
                                   uint32_t dwAddr,
                                   uint32_t * pData,
                                   uint32_t numReg
                                   );
zoe_errs_t CPCIeInterface_ReadMem(CPCIeInterface *This,
                                  zoe_dev_mem_t dwAddr,
                                  uint8_t * pBuffer,  // Address of memory where read data is to be copied to.
                                  uint32_t numBytes     // Number of bytes to be read.
                                  );
zoe_errs_t CPCIeInterface_WriteMem(CPCIeInterface *This,
                                   zoe_dev_mem_t dwAddr,
                                   uint8_t * pBuffer, // Address of memory where write data is to be copied from.
                                   uint32_t numBytes    // Number of bytes to be written.
                                   );
zoe_errs_t CPCIeInterface_DMARead(CPCIeInterface *This,
                                  zoe_dev_mem_t ulAddr,     // in byte
                                  uint32_t ulLength,    // in byte
                                  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
                                  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt,
                                  zoe_void_ptr_t p_private
                                  );
zoe_errs_t CPCIeInterface_DMAWrite(CPCIeInterface *This,
                                   zoe_dev_mem_t ulAddr,    // in byte
                                   uint32_t ulLength,   // in byte
                                   uint8_t * pHostAddr,
                                   uint32_t ulXferMode,
                                   uint32_t ulSwap,
                                   zoe_sosal_obj_id_t evt,
                                   zoe_void_ptr_t p_private
                                   );
zoe_errs_t CPCIeInterface_EnableInterrupts(CPCIeInterface *This);
zoe_errs_t CPCIeInterface_DisableInterrupt(CPCIeInterface *This);
zoe_errs_t CPCIeInterface_RegisterISR(CPCIeInterface *This);
zoe_errs_t CPCIeInterface_UnregisterISR(CPCIeInterface *This);
uint32_t CPCIeInterface_GetMaxDMASize(CPCIeInterface *This);

#ifdef __cplusplus
}
#endif

#endif //__CPCIEINTF_H__


