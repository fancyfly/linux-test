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
// chpucntl.c
//
// Description: 
//
//  hpu bus controller interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "chpucntl.h"
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
static zoe_errs_t CHPUCntl_InitDevice(CHPUCntl *This, 
                                      ZV_DEVICE_CALLBACK pFuncCallback,
                                      zoe_void_ptr_t context,
                                      zoe_void_ptr_t pInitData
                                      )
{
    zoe_errs_t          err;
    PZVCODEC_INITDATA   pCodecInitData = (PZVCODEC_INITDATA)pInitData;

    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CHPUCntl_InitDevice()\n"
                   );

    err = CHPUInterface_StartDevice(This->m_pHPUIntf, 
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
                       "CHPUCntl_InitDevice() PciStartDevice() Failed! err(%d)\n",
                       err
                       );
    }

    return (err);
}



static zoe_errs_t CHPUCntl_Release(CHPUCntl *This)
{
    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CHPUCntl_Release()\n"
                   );
    if (c_object_is_initialized(&This->m_Object))
    {
        CHPUInterface_StopDevice(This->m_pHPUIntf);

        c_object_done(&This->m_Object);
    }
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CHPUCntl_Reset(CHPUCntl *This) 
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CHPUCntl_Set(CHPUCntl *This,
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



static zoe_errs_t CHPUCntl_Get(CHPUCntl *This,
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

// CHPUCntl
//
static zoe_errs_t CHPUCntl_AddDevice(CHPUCntl *This,
                                     zoe_void_ptr_t FunctionalDeviceObject,
                                     zoe_void_ptr_t PhysicalDeviceObject,
								     ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								     zoe_void_ptr_t pBusCallbackContext
                                     )
{
    CHPUInterface  *pHPUInterface;

    // create HPU bus interafce
    //
    pHPUInterface = (CHPUInterface *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                            sizeof(CHPUInterface), 
                                                            sizeof(void *)
                                                            );
    if (!pHPUInterface)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_AddDevice() pHPUInterface == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }

	This->m_pHPUIntf = CHPUInterface_Constructor(pHPUInterface, 
                                                 &This->m_Object,
                                                 OBJECT_CRITICAL_LIGHT, 
                                                 FunctionalDeviceObject, 
                                                 PhysicalDeviceObject,
								                 pBusCallbackFunc,
								                 pBusCallbackContext,
                                                 This->m_dbgID
                                                 );
    if (!This->m_pHPUIntf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUInterface_Constructor() Failed!!!\n"
                       );
        zoe_sosal_memory_free(pHPUInterface);
        return (ZOE_ERRS_NOMEMORY);
    }

    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CHPUCntl_RemoveDevice(CHPUCntl *This)
{
    if (This->m_pHPUIntf)
    {
        CHPUInterface_Destructor(This->m_pHPUIntf);
        zoe_sosal_memory_free(This->m_pHPUIntf);
        This->m_pHPUIntf = ZOE_NULL;
    }
    return (ZOE_ERRS_SUCCESS);
}



/////////////////////////////////////////////////////////////////////////////
//
//
zoe_errs_t CHPUCntl_RegisterRead(CHPUCntl *This,
                                 uint32_t dwAddr,
                                 uint32_t * pData
                                 )
{
    zoe_errs_t      err;
    uint32_t    val;

    err = CHPUInterface_ReadReg(This->m_pHPUIntf, 
                                dwAddr, 
                                &val, 
                                1
                                );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_RegisterRead() CHPUInterface_ReadReg(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    else
    {
        *pData = val;
    }
    return (err);
}



zoe_errs_t CHPUCntl_RegisterWrite(CHPUCntl *This,
                                  uint32_t dwAddr,
                                  uint32_t dwData 
                                  )
{
    zoe_errs_t      err;
    uint32_t    val = dwData;

    err = CHPUInterface_WriteReg(This->m_pHPUIntf, 
                                 dwAddr, 
                                 &val, 
                                 1
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_RegisterWrite() CHPUInterface_WriteReg(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_RegisterReadEx(CHPUCntl *This,
                                   uint32_t dwAddr,
                                   uint32_t * pData,
                                   uint32_t numReg
                                   )
{
    zoe_errs_t  err;

    err = CHPUInterface_ReadReg(This->m_pHPUIntf, 
                                dwAddr, 
                                pData, 
                                numReg
                                );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_RegisterReadEx() CHPUInterface_ReadReg(0x%x) num(numReg) Failed Status(%d)\n", 
                       dwAddr,
                       numReg,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_RegisterWriteEx(CHPUCntl *This,
                                    uint32_t dwAddr,
                                    uint32_t * pData,
                                    uint32_t numReg
                                    )
{
    zoe_errs_t  err;

    err = CHPUInterface_WriteReg(This->m_pHPUIntf, 
                                 dwAddr, 
                                 pData, 
                                 numReg
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_RegisterWriteEx() CHPUInterface_WriteReg(0x%x) num(%d) Failed Status(%d)\n", 
                       dwAddr,
                       numReg,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_MemoryRead(CHPUCntl *This,
                               zoe_dev_mem_t dwAddr,
                               uint32_t * pData 
                               )
{
    zoe_errs_t  err;

    err = CHPUInterface_ReadMem(This->m_pHPUIntf, 
                                dwAddr, 
                                (uint8_t *)pData, 
                                sizeof(uint32_t)
                                );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_MemoryRead() CHPUInterface_ReadMem(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_MemoryWrite(CHPUCntl *This,
								zoe_dev_mem_t dwAddr,
								uint32_t dwData 
								)
{
    zoe_errs_t  err;

    err = CHPUInterface_WriteMem(This->m_pHPUIntf, 
                                 dwAddr, 
                                 (uint8_t *)&dwData, 
                                 sizeof(uint32_t)
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_MemoryWrite() CHPUInterface_WriteMem(0x%x) Failed Status(%d)\n", 
                       dwAddr,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_MemoryReadEx(CHPUCntl *This,
                                 zoe_dev_mem_t dwAddr,
                                 uint8_t * pData, 
                                 uint32_t ulLength // in bytes
                                 )
{
    zoe_errs_t  err;

    err = CHPUInterface_ReadMem(This->m_pHPUIntf, 
                                dwAddr, 
                                pData, 
                                ulLength
                                );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_MemoryReadEx() CHPUInterface_ReadMem(0x%x) (%d)bytes Failed Status(%d)\n", 
                       dwAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_MemoryWriteEx(CHPUCntl *This,
                                  zoe_dev_mem_t dwAddr,
                                  uint8_t * pData, 
                                  uint32_t ulLength// in bytes
                                  )
{
    zoe_errs_t  err;

    err = CHPUInterface_WriteMem(This->m_pHPUIntf, 
                                 dwAddr, 
                                 pData, 
                                 ulLength
                                 );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CHPUCntl_MemoryWriteEx() CHPUInterface_WriteMem(0x%x) (%d)bytes Failed Status(%d)\n", 
                       dwAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_StartDMAWrite(CHPUCntl *This,
                                  zoe_dev_mem_t ulAddr,   // in byte
                                  uint32_t ulLength, // in byte
                                  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
                                  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt,
                                  zoe_void_ptr_t p_private
                                  )
{
    zoe_errs_t  err;

    err = CHPUInterface_DMAWrite(This->m_pHPUIntf, 
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
                       "CHPUCntl_StartDMAWrite() CHPUInterface_DMAWrite(0x%x) len(%d) Failed Status(%d)\n", 
                       ulAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_StartDMARead(CHPUCntl *This,
                                 zoe_dev_mem_t ulAddr,    // in byte
                                 uint32_t ulLength,  // in byte
                                 uint8_t * pHostAddr,
                                 uint32_t ulXferMode,
                                 uint32_t ulSwap,
                                 zoe_sosal_obj_id_t evt,
                                 zoe_void_ptr_t p_private
                                 )
{
    zoe_errs_t  err;

    err = CHPUInterface_DMARead(This->m_pHPUIntf, 
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
                       "CHPUCntl_StartDMARead() CHPUInterface_DMARead(0x%x) len(%d) Failed Status(%d)\n", 
                       ulAddr,
                       ulLength,
                       err
                       );
    }
    return (err);
}



zoe_errs_t CHPUCntl_EnableInterrupts(CHPUCntl *This)
{
    return (CHPUInterface_EnableInterrupts(This->m_pHPUIntf));
}



zoe_errs_t CHPUCntl_DisableInterrupt(CHPUCntl *This)
{
    return (CHPUInterface_DisableInterrupt(This->m_pHPUIntf));
}



zoe_errs_t CHPUCntl_RegisterISR(CHPUCntl *This)
{
    return (CHPUInterface_RegisterISR(This->m_pHPUIntf));
}



zoe_errs_t CHPUCntl_UnregisterISR(CHPUCntl *This)
{
    return (CHPUInterface_UnregisterISR(This->m_pHPUIntf));
}



uint32_t CHPUCntl_GetMaxDMASize(CHPUCntl *This)
{
    return (CHPUInterface_GetMaxDMASize(This->m_pHPUIntf));
}




CHPUCntl * CHPUCntl_Constructor(CHPUCntl *pHPUCntl, 
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
                   "CHPUCntl_Constructor()\n"
                   );

    if (pHPUCntl)
    {
        zoe_errs_t      err;

        // c_object
        //
        c_object_constructor(&pHPUCntl->m_Object, 
                             pParent,
                             OBJECT_ZOE_HPU_CNTL,
                             dwAttributes
                             );

        // i_zv_generic_device
        //
        pHPUCntl->init_device = CHPUCntl_InitDevice;
        pHPUCntl->release = CHPUCntl_Release;
        pHPUCntl->reset = CHPUCntl_Reset;
        pHPUCntl->set = CHPUCntl_Set;
        pHPUCntl->get = CHPUCntl_Get;

        // CHPUCntl
        //
        pHPUCntl->m_pHPUIntf = ZOE_NULL;

        pHPUCntl->m_pBusCallbackFunc = pBusCallbackFunc;
        pHPUCntl->m_pBusCallbackContext = pBusCallbackContext;

        pHPUCntl->m_dbgID = dbgID;

        // create HPU bus interface
        //
        err = CHPUCntl_AddDevice(pHPUCntl,
                                 pFunctionalDeviceObject,
                                 pPhysicalDeviceObject,
                                 pBusCallbackFunc,
                                 pBusCallbackContext
                                 );
        if (!ZOE_SUCCESS(err)) 
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
                           "CHPUCntl_AddDevice() Failed! err(%d)\n",
                           err
                           );
            CHPUCntl_Destructor(pHPUCntl);
            return (ZOE_NULL);
		}
    }

    return (pHPUCntl);
}



void CHPUCntl_Destructor(CHPUCntl *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CHPUCntl_Destructor()\n"
                   );
    // delete HPU bus interface
    //
    CHPUCntl_RemoveDevice(This);

    // c_object
    //
    c_object_destructor(&This->m_Object);
}


