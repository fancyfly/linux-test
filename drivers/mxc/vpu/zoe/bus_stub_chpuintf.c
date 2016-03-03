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
// chpuintf.c
//
// Description: 
//
//  linux hpu bus interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/sched.h>
#include <linux/string.h>
#include "chpuintf.h"
#include "zoe_objids.h"


/////////////////////////////////////////////////////////////////////////////
//
//
zoe_errs_t CHPUInterface_ReleaseResources(CHPUInterface *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
				   "%s()\n",
                   __FUNCTION__
				   );
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DmaInit(CHPUInterface *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
				   "%s()\n",
                   __FUNCTION__
				   );
    return (ZOE_ERRS_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////
//
//
zoe_errs_t CHPUInterface_StartDevice(CHPUInterface *This,
                                     zoe_void_ptr_t pBusData,
                                     uint32_t nBusDataSize
                                     )
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_StopDevice(CHPUInterface *This)
{
    return (ZOE_ERRS_SUCCESS);
}



void CHPUInterface_InitializeDeviceState(CHPUInterface *This)
{
}



zoe_errs_t CHPUInterface_ReadReg(CHPUInterface *This,
                                 uint32_t dwAddr,
                                 uint32_t * pData,
                                 uint32_t numReg
                                 )
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_WriteReg(CHPUInterface *This,
                                  uint32_t dwAddr,
                                  uint32_t * pData,
                                  uint32_t numReg
                                  )
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_ReadMem(CHPUInterface *This,
                                 zoe_dev_mem_t dwAddr,
                                 uint8_t * pBuffer,  // Address of memory where read data is to be copied to.
                                 uint32_t numBytes     // Number of bytes to be read.
                                 )
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_WriteMem(CHPUInterface *This,
                                  zoe_dev_mem_t dwAddr,
                                  uint8_t * pBuffer, // Address of memory where write data is to be copied from.
                                  uint32_t numBytes    // Number of bytes to be written.
                                  )
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DMARead(CHPUInterface *This,
                                 zoe_dev_mem_t ulAddr,      // in byte
                                 uint32_t ulLength,    // in byte
                                 uint8_t * pHostAddr,
                                 uint32_t ulXferMode,
                                 uint32_t ulSwap,
                                 zoe_sosal_obj_id_t evt,
                                 zoe_void_ptr_t p_private
                                 )
{
    if (evt)
    {
        zoe_sosal_event_set(evt);
    }
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DMAWrite(CHPUInterface *This,
                                  zoe_dev_mem_t ulAddr,     // in byte
                                  uint32_t ulLength,   // in byte
                                  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
                                  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt,
                                  zoe_void_ptr_t p_private
                                  )
{
    if (evt)
    {
        zoe_sosal_event_set(evt);
    }
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_EnableInterrupts(CHPUInterface *This)
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DisableInterrupt(CHPUInterface *This)
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_RegisterISR(CHPUInterface *This)
{
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_UnregisterISR(CHPUInterface *This)
{
    return (ZOE_ERRS_SUCCESS);
}



uint32_t CHPUInterface_GetMaxDMASize(CHPUInterface *This)
{
	return (1024 * 1024);
}




CHPUInterface * CHPUInterface_Constructor(CHPUInterface *pHPUInterface,
                                          c_object *pParent,
                                          uint32_t dwAttributes,
                                          zoe_void_ptr_t pDO,
                                          zoe_void_ptr_t pDOLayered,
								          ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								          zoe_void_ptr_t pBusCallbackContext,
                                          zoe_dbg_comp_id_t dbgID
                                          )
{
    if (pHPUInterface)
    {
        memset(pHPUInterface, 
               0, 
               sizeof(CHPUInterface)
               );

        // c_object
        //
        c_object_constructor(&pHPUInterface->m_Object, 
                             pParent, 
                             OBJECT_ZOE_HPU_INTF,
                             dwAttributes
                             );
        pHPUInterface->m_pBusCallbackFunc = pBusCallbackFunc;
        pHPUInterface->m_pBusCallbackContext = pBusCallbackContext;
        pHPUInterface->m_dbgID = dbgID;

        // initialize OS specific data
        //

        // Initialize the device state information managed by the driver.
        CHPUInterface_InitializeDeviceState(pHPUInterface);
    }

    return (pHPUInterface);
}



void CHPUInterface_Destructor(CHPUInterface *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CHPUInterface_Destructor()\n"
                   );

	// Release resources.
    CHPUInterface_ReleaseResources(This);

    if (This->m_pPrivateData)
    {
        // free private data(device extension)
        kfree(This->m_pPrivateData);
        This->m_pPrivateData = ZOE_NULL;
	}
}

