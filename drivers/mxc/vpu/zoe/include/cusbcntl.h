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
// cusbcntl.h
//
// Description: 
//
//  ZOE USB controller interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CUSBCNTL_H__
#define __CUSBCNTL_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_cqueue.h"
#include "zv_genericdevice.h"
#include "zoe_dbg.h"
#include "cusbintf.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __CUSBCNTL_FWD_DEFINED__
#define __CUSBCNTL_FWD_DEFINED__
typedef struct CUsbCntl CUsbCntl;
#endif //__CUSBCNTL_FWD_DEFINED__

struct CUsbCntl
{
    // public c_object
    //
    c_object             m_Object;

	// i_zv_generic_device
	//
	DECLARE_IZVGENERICDEVICE(CUsbCntl)

    // CUsbCntl
    //
    CUsbInterface       *m_pUsbIntf;

    uint32_t            m_dwPipeCmdWrite;
    uint32_t            m_dwPipeCmdRead;
    uint32_t            m_dwPipeDataWrite;
    uint32_t            m_dwPipeDataRead;
    uint32_t            m_dwPipeIntr;

    // bus callback
    ZV_BUSINTF_CALLBACK m_pBusCallbackFunc;
    zoe_void_ptr_t      m_pBusCallbackContext;

    uint32_t            m_cmdCnt;
    zoe_dbg_comp_id_t   m_dbgID;

    PZVUSB_IO_CONTEXT   m_pDmaRequest[ZVUSB_DIR_MAX];
	c_queue				*m_pDmaRequestQueue[ZVUSB_DIR_MAX];
    zoe_sosal_obj_id_t	m_DmaRequestLock[ZVUSB_DIR_MAX];
};


zoe_errs_t CUsbCntl_GenericCmd(CUsbCntl *This,
                               uint8_t * pOutBuf,
                               uint32_t dwOutSize,
                               uint8_t * pInBuf,
                               uint32_t dwInSize,
                               zoe_bool_t bCritical
                               );
zoe_errs_t CUsbCntl_RegisterRead(CUsbCntl *This,
								 uint32_t dwAddr,
								 uint32_t * pData
								 );
zoe_errs_t CUsbCntl_RegisterWrite(CUsbCntl *This,
								  uint32_t dwAddr,
								  uint32_t dwData 
								  );
zoe_errs_t CUsbCntl_RegisterReadEx(CUsbCntl *This,
								   uint32_t dwAddr,
								   uint32_t * pData,
								   uint32_t numReg
								   );
zoe_errs_t CUsbCntl_RegisterWriteEx(CUsbCntl *This,
								    uint32_t dwAddr,
								    uint32_t * pData,
								    uint32_t numReg
								    );
zoe_errs_t CUsbCntl_MemoryRead(CUsbCntl *This,
							   zoe_dev_mem_t dwAddr,
							   uint32_t * pData 
							   );
zoe_errs_t CUsbCntl_MemoryWrite(CUsbCntl *This,
								zoe_dev_mem_t dwAddr,
								uint32_t dwData 
								);
zoe_errs_t CUsbCntl_MemoryReadEx(CUsbCntl *This,
							     zoe_dev_mem_t dwAddr,
							     uint8_t * pData, 
							     uint32_t ulLength  // in bytes
							     );
zoe_errs_t CUsbCntl_MemoryWriteEx(CUsbCntl *This,
								  zoe_dev_mem_t dwAddr,
							      uint8_t * pData, 
							      uint32_t ulLength // in bytes
								  );
zoe_errs_t CUsbCntl_StartDMAWrite(CUsbCntl *This,
								  zoe_dev_mem_t ulAddr,
								  uint32_t ulLength,// in bytes
								  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
								  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt
								  );
zoe_errs_t CUsbCntl_StartDMARead(CUsbCntl *This,
								 zoe_dev_mem_t ulAddr,
								 uint32_t ulLength, // in bytes
								 uint8_t * pHostAddr,
                                 uint32_t ulXferMode,
								 uint32_t ulSwap,
                                 zoe_sosal_obj_id_t evt
								 );
CUsbCntl * CUsbCntl_Constructor(CUsbCntl *pUsbCntl, 
								c_object *pParent,
								uint32_t dwAttributes,
                                zoe_void_ptr_t pPhysicalDeviceObject,
                                zoe_void_ptr_t pFunctionalDeviceObject,
								ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								zoe_void_ptr_t pBusCallbackContext,
                                zoe_dbg_comp_id_t dbgID
								);

void CUsbCntl_Destructor(CUsbCntl *This);


#ifdef __cplusplus
}
#endif

#endif //__CUSBCNTL_H__



