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
// cpicicntl.h
//
// Description: 
//
//   ZOE PCIe controller
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __CPCIECNTL_H__
#define __CPCIECNTL_H__

#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zv_genericdevice.h"
#include "zoe_dbg.h"
#include "zv_busintf.h"
#include "cpcieintf.h"


#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __CPCIECNTL_FWD_DEFINED__
#define __CPCIECNTL_FWD_DEFINED__
typedef struct CPCIeCntl CPCIeCntl;
#endif //__CPCIECNTL_FWD_DEFINED__

struct CPCIeCntl
{
	// public c_object
	//
	c_object				m_Object;

	// i_zv_generic_device
	//
	DECLARE_IZVGENERICDEVICE(CPCIeCntl)

	// CPCIeCntl
	//
    CPCIeInterface      *m_pPCIeIntf;

	// bus callback
	ZV_BUSINTF_CALLBACK	m_pBusCallbackFunc;
	zoe_void_ptr_t      m_pBusCallbackContext;

    zoe_dbg_comp_id_t   m_dbgID;
};


zoe_errs_t CPCIeCntl_RegisterRead(CPCIeCntl *This,
                                  uint32_t dwAddr,
                                  uint32_t * pData
                                  );
zoe_errs_t CPCIeCntl_RegisterWrite(CPCIeCntl *This,
                                   uint32_t dwAddr,
                                   uint32_t dwData 
                                   );
zoe_errs_t CPCIeCntl_RegisterReadEx(CPCIeCntl *This,
                                    uint32_t dwAddr,
                                    uint32_t * pData,
                                    uint32_t numReg
                                    );
zoe_errs_t CPCIeCntl_RegisterWriteEx(CPCIeCntl *This,
                                     uint32_t dwAddr,
                                     uint32_t * pData,
                                     uint32_t numReg
                                     );
zoe_errs_t CPCIeCntl_MemoryRead(CPCIeCntl *This,
                                zoe_dev_mem_t dwAddr,
                                uint32_t * pData 
                                );
zoe_errs_t CPCIeCntl_MemoryWrite(CPCIeCntl *This,
                                 zoe_dev_mem_t dwAddr,
                                 uint32_t dwData 
                                 );
zoe_errs_t CPCIeCntl_MemoryReadEx(CPCIeCntl *This,
                                  zoe_dev_mem_t dwAddr,
                                  uint8_t * pData, 
                                  uint32_t ulLength // in bytes
                                  );
zoe_errs_t CPCIeCntl_MemoryWriteEx(CPCIeCntl *This,
                                   zoe_dev_mem_t dwAddr,
                                   uint8_t * pData, 
                                   uint32_t ulLength// in bytes
                                   );
zoe_errs_t CPCIeCntl_StartDMAWrite(CPCIeCntl *This,
                                   zoe_dev_mem_t ulAddr,  // in byte
                                   uint32_t ulLength, // in byte
                                   uint8_t * pHostAddr,
                                   uint32_t ulXferMode,
                                   uint32_t ulSwap,
                                   zoe_sosal_obj_id_t evt,
                                   zoe_void_ptr_t p_private
                                   );
zoe_errs_t CPCIeCntl_StartDMARead(CPCIeCntl *This,
                                  zoe_dev_mem_t ulAddr,   // in byte
                                  uint32_t ulLength,  // in byte
                                  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
                                  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt,
                                  zoe_void_ptr_t p_private
                                  );
zoe_errs_t CPCIeCntl_EnableInterrupts(CPCIeCntl *This);
zoe_errs_t CPCIeCntl_DisableInterrupt(CPCIeCntl *This);
zoe_errs_t CPCIeCntl_RegisterISR(CPCIeCntl *This);
zoe_errs_t CPCIeCntl_UnregisterISR(CPCIeCntl *This);
uint32_t CPCIeCntl_GetMaxDMASize(CPCIeCntl *This);
CPCIeCntl * CPCIeCntl_Constructor(CPCIeCntl *pPCIeCntl, 
                                  c_object *pParent,
                                  uint32_t dwAttributes,
                                  zoe_void_ptr_t pPhysicalDeviceObject,
                                  zoe_void_ptr_t pFunctionalDeviceObject,
                                  ZV_BUSINTF_CALLBACK pBusCallbackFunc,
                                  zoe_void_ptr_t pBusCallbackContext,
                                  zoe_dbg_comp_id_t dbgID
                                  );
void CPCIeCntl_Destructor(CPCIeCntl *This);


#ifdef __cplusplus
}
#endif

#endif //__CPCIECNTL_H__



