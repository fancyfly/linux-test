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
// cpciecntl.c
//
// Description: 
//
//  ZOE PCIe controller interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "cpciecntl.h"
#include "zoe_sosal.h"
#include "zv_codec.h"
#include "zoe_util.h"
#include "zoe_objids.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD


/////////////////////////////////////////////////////////////////////////////
//
//
																

// IGenericDevice
//
static zoe_errs_t CPCIeCntl_InitDevice(CPCIeCntl *This, 
                                       ZV_DEVICE_CALLBACK pFuncCallback,
                                       zoe_void_ptr_t context,
                                       zoe_void_ptr_t pInitData
                                       )
{
    zoe_errs_t          err;
    PZVCODEC_INITDATA   pCodecInitData = (PZVCODEC_INITDATA)pInitData;

    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CPCIeCntl_InitDevice()\n"
                   );

    err = CPCIeInterface_StartDevice(This->m_pPCIeIntf, 
                                     pCodecInitData->BusData, 
                                     pCodecInitData->BusDataSize
                                     );
    if (ZOE_SUCCESS(err))
    {
        // c_object Init
        //
        c_object_init(&This->m_Object);
    }
    else
    {
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_InitDevice() PciStartDevice() Failed! err(%d)\n",
                       err
                       );
    }

    return (err);
}



static zoe_errs_t CPCIeCntl_Release(CPCIeCntl *This)
{
    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CPCIeCntl_Release()\n"
                   );
    if (c_object_is_initialized(&This->m_Object))
    {
        CPCIeInterface_StopDevice(This->m_pPCIeIntf);

        c_object_done(&This->m_Object);
    }
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CPCIeCntl_Reset(CPCIeCntl *This) 
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CPCIeCntl_Set(CPCIeCntl *This,
                                ZOE_REFGUID guid,
                                ZOE_OBJECT_HANDLE hIndex,
                                uint32_t dwCode,
                                zoe_void_ptr_t pInput,
                                zoe_void_ptr_t pOutput,
                                uint32_t dwSize
                                )
{
    return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t CPCIeCntl_Get(CPCIeCntl *This,
                                ZOE_REFGUID guid,
                                ZOE_OBJECT_HANDLE hIndex,
                                uint32_t dwCode,
                                zoe_void_ptr_t pInput,
                                zoe_void_ptr_t pOutput,
                                uint32_t * pdwSizeGot
                                )
{
    return (ZOE_ERRS_NOTIMPL);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// CPCIeCntl
//
static zoe_errs_t CPCIeCntl_AddDevice(CPCIeCntl *This,
                                      zoe_void_ptr_t FunctionalDeviceObject,
                                      zoe_void_ptr_t PhysicalDeviceObject,
								      ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								      zoe_void_ptr_t pBusCallbackContext
                                      )
{
    CPCIeInterface  *pPCIeInterface;

    // create PCIe bus interafce
    //
    pPCIeInterface = (CPCIeInterface *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                              sizeof(CPCIeInterface), 
                                                              sizeof(void *)
                                                              );
    if (!pPCIeInterface)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_AddDevice() pPCIeInterface == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }

	This->m_pPCIeIntf = CPCIeInterface_Constructor(pPCIeInterface, 
                                                   &This->m_Object,
                                                   OBJECT_CRITICAL_LIGHT, 
                                                   FunctionalDeviceObject, 
                                                   PhysicalDeviceObject,
								                   pBusCallbackFunc,
								                   pBusCallbackContext,
                                                   This->m_dbgID
                                                   );
    if (!This->m_pPCIeIntf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeInterface_Constructor() Failed!!!\n"
                       );
        zoe_sosal_memory_free(pPCIeInterface);
        return (ZOE_ERRS_NOMEMORY);
    }

    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CPCIeCntl_RemoveDevice(CPCIeCntl *This)
{
    if (This->m_pPCIeIntf)
    {
        CPCIeInterface_Destructor(This->m_pPCIeIntf);
        zoe_sosal_memory_free(This->m_pPCIeIntf);
        This->m_pPCIeIntf = ZOE_NULL;
    }
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CPCIeCntl_RegisterRead(CPCIeCntl *This,
                                  uint32_t dwAddr,
                                  uint32_t *pData
                                  )
{
    zoe_errs_t  err;
    uint32_t    val;

    err = CPCIeInterface_ReadReg(This->m_pPCIeIntf, 
                                 dwAddr, 
                                 &val, 
                                 1
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_RegisterRead() CPCIeInterface_ReadReg(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    else
    {
        *pData = val;
//	    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
//                       This->m_dbgID,
//                       "CPCIeInterface_ReadReg(0x%x) (0x%x)\n", 
//                       dwAddr,
//                       *pData
//                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_RegisterWrite(CPCIeCntl *This,
                                   uint32_t dwAddr,
                                   uint32_t dwData 
                                   )
{
    zoe_errs_t  err;
    uint32_t    val = dwData;

    err = CPCIeInterface_WriteReg(This->m_pPCIeIntf, 
                                  dwAddr, 
                                  &val, 
                                  1
                                  );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_RegisterWrite() CPCIeInterface_WriteReg(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
//    else
//    {
//	    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
//                       This->m_dbgID,
//                       "CPCIeInterface_WriteReg(0x%x) (0x%x)\n", 
//                       dwAddr,
//                       val
//                       );
//    }
    return (err);
}



zoe_errs_t CPCIeCntl_RegisterReadEx(CPCIeCntl *This,
                                    uint32_t dwAddr,
                                    uint32_t *pData,
                                    uint32_t numReg
                                    )
{
    zoe_errs_t  err;

    err = CPCIeInterface_ReadReg(This->m_pPCIeIntf, 
                                 dwAddr, 
                                 pData, 
                                 numReg
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_RegisterReadEx() CPCIeInterface_ReadReg(0x%x) num(numReg) Failed Status(%d)\n", 
                       dwAddr,
                       numReg,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_RegisterWriteEx(CPCIeCntl *This,
                                     uint32_t dwAddr,
                                     uint32_t *pData,
                                     uint32_t numReg
                                     )
{
    zoe_errs_t  err;

    err = CPCIeInterface_WriteReg(This->m_pPCIeIntf, 
                                  dwAddr, 
                                  pData, 
                                  numReg
                                  );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_RegisterWriteEx() CPCIeInterface_WriteReg(0x%x) num(%d) Failed Status(%d)\n", 
                       dwAddr,
                       numReg,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_MemoryRead(CPCIeCntl *This,
                                zoe_dev_mem_t dwAddr,
                                uint32_t *pData 
                                )
{
    zoe_errs_t  err;

    err = CPCIeInterface_ReadMem(This->m_pPCIeIntf, 
                                 dwAddr, 
                                 (uint8_t *)pData, 
                                 sizeof(uint32_t)
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_MemoryRead() CPCIeInterface_ReadMem(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_MemoryWrite(CPCIeCntl *This,
								 zoe_dev_mem_t dwAddr,
								 uint32_t dwData 
								 )
{
    zoe_errs_t  err;

    err = CPCIeInterface_WriteMem(This->m_pPCIeIntf, 
                                  dwAddr, 
                                  (uint8_t *)&dwData, 
                                  sizeof(uint32_t)
                                  );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_MemoryWrite() CPCIeInterface_WriteMem(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_MemoryReadEx(CPCIeCntl *This,
                                  zoe_dev_mem_t dwAddr,
                                  uint8_t * pData, 
                                  uint32_t ulLength // in bytes
                                  )
{
    zoe_errs_t  err;

    err = CPCIeInterface_ReadMem(This->m_pPCIeIntf, 
                                 dwAddr, 
                                 pData, 
                                 ulLength
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_MemoryReadEx() CPCIeInterface_ReadMem(0x%x) (%d)bytes Failed Status(%d)\n", 
                       dwAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_MemoryWriteEx(CPCIeCntl *This,
                                   zoe_dev_mem_t dwAddr,
                                   uint8_t * pData, 
                                   uint32_t ulLength// in bytes
                                   )
{
    zoe_errs_t  err;

    err = CPCIeInterface_WriteMem(This->m_pPCIeIntf, 
                                  dwAddr, 
                                  pData, 
                                  ulLength
                                  );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_MemoryWriteEx() CPCIeInterface_WriteMem(0x%x) (%d)bytes Failed Status(%d)\n", 
                       dwAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_StartDMAWrite(CPCIeCntl *This,
                                   zoe_dev_mem_t ulAddr,  // in byte
                                   uint32_t ulLength, // in byte
                                   uint8_t * pHostAddr,
                                   uint32_t ulXferMode,
                                   uint32_t ulSwap,
                                   zoe_sosal_obj_id_t evt,
                                   zoe_void_ptr_t p_private
                                   )
{
    zoe_errs_t  err;

    err = CPCIeInterface_DMAWrite(This->m_pPCIeIntf, 
                                  ulAddr, 
                                  ulLength,
                                  pHostAddr,
                                  ulXferMode,
                                  ulSwap,
                                  evt,
                                  p_private
                                  );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_StartDMAWrite() CPCIeInterface_DMAWrite(0x%x) len(%d) Failed Status(%d)\n", 
                       ulAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_StartDMARead(CPCIeCntl *This,
                                  zoe_dev_mem_t ulAddr,   // in byte
                                  uint32_t ulLength,  // in byte
                                  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
                                  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt,
                                  zoe_void_ptr_t p_private
                                  )
{
    zoe_errs_t  err;

    err = CPCIeInterface_DMARead(This->m_pPCIeIntf, 
                                 ulAddr, 
                                 ulLength,
                                 pHostAddr,
                                 ulXferMode,
                                 ulSwap,
                                 evt,
                                 p_private
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CPCIeCntl_StartDMARead() CPCIeInterface_DMARead(0x%x) len(%d) Failed Status(%d)\n", 
                       ulAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CPCIeCntl_EnableInterrupts(CPCIeCntl *This)
{
    return (CPCIeInterface_EnableInterrupts(This->m_pPCIeIntf));
}



zoe_errs_t CPCIeCntl_DisableInterrupt(CPCIeCntl *This)
{
    return (CPCIeInterface_DisableInterrupt(This->m_pPCIeIntf));
}



zoe_errs_t CPCIeCntl_RegisterISR(CPCIeCntl *This)
{
    return (CPCIeInterface_RegisterISR(This->m_pPCIeIntf));
}



zoe_errs_t CPCIeCntl_UnregisterISR(CPCIeCntl *This)
{
    return (CPCIeInterface_UnregisterISR(This->m_pPCIeIntf));
}



uint32_t CPCIeCntl_GetMaxDMASize(CPCIeCntl *This)
{
    return (CPCIeInterface_GetMaxDMASize(This->m_pPCIeIntf));
}




CPCIeCntl * CPCIeCntl_Constructor(CPCIeCntl *pPCIeCntl, 
                                  c_object *pParent,
                                  uint32_t dwAttributes,
                                  zoe_void_ptr_t pPhysicalDeviceObject,
                                  zoe_void_ptr_t pFunctionalDeviceObject,
                                  ZV_BUSINTF_CALLBACK pBusCallbackFunc,
                                  zoe_void_ptr_t pBusCallbackContext,
                                  zoe_dbg_comp_id_t dbgID
                                  )
{
    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   dbgID,
                   "CPCIeCntl_Constructor()\n"
                   );

    if (pPCIeCntl)
    {
        zoe_errs_t      err;

        // c_object
        //
        c_object_constructor(&pPCIeCntl->m_Object, 
                             pParent,
                             OBJECT_ZOE_PCIE_CNTL,
                             dwAttributes
                             );

        // i_zv_generic_device
        //
        pPCIeCntl->init_device = CPCIeCntl_InitDevice;
        pPCIeCntl->release = CPCIeCntl_Release;
        pPCIeCntl->reset = CPCIeCntl_Reset;
        pPCIeCntl->set = CPCIeCntl_Set;
        pPCIeCntl->get = CPCIeCntl_Get;

        // CPCIeCntl
        //
        pPCIeCntl->m_pPCIeIntf = ZOE_NULL;

        pPCIeCntl->m_pBusCallbackFunc = pBusCallbackFunc;
        pPCIeCntl->m_pBusCallbackContext = pBusCallbackContext;

        pPCIeCntl->m_dbgID = dbgID;

        // create PCIe bus interface
        //
        err = CPCIeCntl_AddDevice(pPCIeCntl,
                                  pFunctionalDeviceObject,
                                  pPhysicalDeviceObject,
                                  pBusCallbackFunc,
                                  pBusCallbackContext
                                  );
        if (!ZOE_SUCCESS(err)) 
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
                           "CPCIeCntl_AddDevice() Failed! err(%d)\n",
                           err
                           );
            CPCIeCntl_Destructor(pPCIeCntl);
            return (ZOE_NULL);
		}
    }

    return (pPCIeCntl);
}



void CPCIeCntl_Destructor(CPCIeCntl *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CPCIeCntl_Destructor()\n"
                   );
    // delete PCIe bus interface
    //
    CPCIeCntl_RemoveDevice(This);

    // c_object
    //
    c_object_destructor(&This->m_Object);
}


