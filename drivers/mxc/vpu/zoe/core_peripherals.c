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
// peripherals.c
//
// Description: 
//
//	peripheral and I/O bus init/done functions
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "czvavlib.h"

#ifdef _ZVCODEC_PURE

zoe_bool_t c_zv_av_lib_create_usb(c_zv_av_lib *This)
{
	return (ZOE_TRUE);
}



void c_zv_av_lib_delete_usb(c_zv_av_lib *This)
{
}



zoe_errs_t c_zv_av_lib_init_usb(c_zv_av_lib *This)
{
    return (ZOE_ERRS_SUCCESS);
}



void c_zv_av_lib_done_usb(c_zv_av_lib *This)
{
}



zoe_bool_t c_zv_av_lib_create_pcie(c_zv_av_lib *This)
{
	return (ZOE_TRUE);
}



void c_zv_av_lib_delete_pcie(c_zv_av_lib *This)
{
}



zoe_errs_t c_zv_av_lib_init_pcie(c_zv_av_lib *This)
{
	return (ZOE_ERRS_SUCCESS);
}



void c_zv_av_lib_done_pcie(c_zv_av_lib *This)
{
}



zoe_bool_t c_zv_av_lib_create_hpu(c_zv_av_lib *This)
{
	return (ZOE_TRUE);
}



void c_zv_av_lib_delete_hpu(c_zv_av_lib *This)
{
}



zoe_errs_t c_zv_av_lib_init_hpu(c_zv_av_lib *This)
{
	return (ZOE_ERRS_SUCCESS);
}



void c_zv_av_lib_done_hpu(c_zv_av_lib *This)
{
}



#else //!_ZVCODEC_PURE

#include "zoe_sosal.h"
#include "zoe_sosal_priv.h"

/////////////////////////////////////////////////////////////////////////////
//
//

extern void CZoeIPCService_ISR(void * ctxt, 
                               zoe_sosal_isr_sw_numbers_t from_num
                               );


static zoe_errs_t c_zv_av_lib_BusCallback(zoe_void_ptr_t context, 
                                          uint32_t cmd, 
                                          zoe_void_ptr_t param1, 
                                          uint32_t param2, 
                                          uint32_t param3
                                          )
{
    c_zv_av_lib    *This = (c_zv_av_lib *)context;
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;

    if (This)
    {
        switch (cmd)
        {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
            case ZV_BUSINTF_INT_CPU_FROM_EXT:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_DMAPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_DMAPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_DMAPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_DMAPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_AUD0:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_AUD1:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_EDPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_EDPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_EDPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_EDPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_EEPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_EEPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_EEPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_EEPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_MEPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_MEPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_MEPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_MEPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_SPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_SPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_SPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_SPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_HPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL]);
                }
                break;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
            case ZV_BUSINTF_INT_CPU_FROM_EXT:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_VID:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_DAWN_ISR_SW_FWPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_FWPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_FWPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_SPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_DAWN_ISR_SW_SPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_SPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_SPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_HPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_DAWN_ISR_SW_HPU_KERNEL);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_HPU_KERNEL])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_HPU_KERNEL]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_MEPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_DAWN_ISR_SW_MEPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_MEPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_DAWN_ISR_SW_MEPU]);
                }
                break;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
            case ZV_BUSINTF_INT_CPU_FROM_VID:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_MX8_ISR_SW_FWPU);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_MX8_ISR_SW_FWPU])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_MX8_ISR_SW_FWPU]);
                }
                break;
            case ZV_BUSINTF_INT_CPU_FROM_HPU:
                CZoeIPCService_ISR((void *)This->m_pZoeIPCService, ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL);
                if (This->m_iHal.m_b_enable_wait_isrs[ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL])
                {
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL]);
                }
                break;
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP
            case ZV_BUSINTF_DMA_DONE_READ:
            case ZV_BUSINTF_DMA_DONE_WRITE:
            {
                zoe_sosal_obj_id_t  evt = (zoe_sosal_obj_id_t)param1;
                if (evt)
                {
                    zoe_sosal_event_set(evt);
                }
                else
                {
                    err = ZOE_ERRS_PARMS;
                }
                break;
            }
            default:
                err = ZOE_ERRS_PARMS;
                break;
        }
    }
    else
    {
        err = ZOE_ERRS_PARMS;
    }
    return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//
zoe_bool_t c_zv_av_lib_create_usb(c_zv_av_lib *This)
{
	CUsbCntl    *pUsbCntl;

	// create the USB controller 
	//
	pUsbCntl = (CUsbCntl *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                  sizeof(CUsbCntl),
                                                  sizeof(void *)
                                                  );
	if (!pUsbCntl)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_create_usb() pUsbCntl == NULL!!!\n"
					   );
		return (ZOE_FALSE);
	}

	This->m_pUsbCntl = CUsbCntl_Constructor(pUsbCntl, 
											&This->m_Object, 
											OBJECT_CRITICAL_LIGHT,
											This->m_pDO, 
											This->m_PDOLayered,
											c_zv_av_lib_BusCallback,
											This,
                                            This->m_dbgID
											);
	if (!This->m_pUsbCntl)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_create_usb() CUsbCntl_Constructor() failed\n"
					   );
		zoe_sosal_memory_free((void *)pUsbCntl);
		return (ZOE_FALSE);
	}

	return (ZOE_TRUE);
}



void c_zv_av_lib_delete_usb(c_zv_av_lib *This)
{
	// delete USB controller
	//
	if (This->m_pUsbCntl)
	{
		CUsbCntl_Destructor(This->m_pUsbCntl);
		zoe_sosal_memory_free((void *)This->m_pUsbCntl);
		This->m_pUsbCntl = ZOE_NULL;
	}
}



zoe_errs_t c_zv_av_lib_init_usb(c_zv_av_lib *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	// initialize USB controller
	//
	if (This->m_pUsbCntl)
	{
		err = i_zv_generic_device_init_device(This->m_pUsbCntl, 
										      ZOE_NULL,
										      ZOE_NULL,
										      (zoe_void_ptr_t)&This->m_InitData.codecInitData
										      );
		if (!ZOE_SUCCESS(err))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
						   "c_zv_av_lib_init_usb() m_pUsbCntl->init_device Failed status(%d)!!!\n",
						   err
						   );
		}
	}

	return (err);
}



void c_zv_av_lib_done_usb(c_zv_av_lib *This)
{
	// done USB controller
	//
	if (This->m_pUsbCntl)
	{
		i_zv_generic_device_release(This->m_pUsbCntl);
	}
}



zoe_bool_t c_zv_av_lib_create_pcie(c_zv_av_lib *This)
{
	CPCIeCntl	*pPCIeCntl;

	// create PCIe bus controller
	//
	pPCIeCntl = (CPCIeCntl *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                    sizeof(CPCIeCntl),
                                                    sizeof(void *)
                                                    );
	if (!pPCIeCntl)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_create_pcie() pPCIeCntl == NULL!!!\n"
					   );
		return (ZOE_FALSE);
	}

	This->m_pPCIeCntl = CPCIeCntl_Constructor(pPCIeCntl, 
											  &This->m_Object,
											  OBJECT_CRITICAL_LIGHT, 
											  This->m_pDO, 
											  This->m_PDOLayered,
											  c_zv_av_lib_BusCallback,
											  This,
                                              This->m_dbgID
											  );
	if (!This->m_pPCIeCntl)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_create_pcie() CPCIeCntl_Constructor() failed\n"
					   );
		zoe_sosal_memory_free((void *)pPCIeCntl);
		return (ZOE_FALSE);
	}

	return (ZOE_TRUE);
}



void c_zv_av_lib_delete_pcie(c_zv_av_lib *This)
{
	// delete PCIe bus controller
	//
	if (This->m_pPCIeCntl)
	{
		CPCIeCntl_Destructor(This->m_pPCIeCntl);
		zoe_sosal_memory_free((void *)This->m_pPCIeCntl);
		This->m_pPCIeCntl = ZOE_NULL;
	}
}



zoe_errs_t c_zv_av_lib_init_pcie(c_zv_av_lib *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	// initialize PCIe bus controller
	//
	if (This->m_pPCIeCntl)
	{
		err = i_zv_generic_device_init_device(This->m_pPCIeCntl, 
										      ZOE_NULL,
										      ZOE_NULL,
										      (zoe_void_ptr_t)&This->m_InitData.codecInitData
										      );
		if (!ZOE_SUCCESS(err))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
						   "c_zv_av_lib_init_pcie() m_pPCIeCntl->init_device Failed status(%d)!!!\n",
						   err
						   );
		}
	}

	return (err);
}



void c_zv_av_lib_done_pcie(c_zv_av_lib *This)
{
	// release PCIe bus controller
	//
	if (This->m_pPCIeCntl)
	{
		i_zv_generic_device_release(This->m_pPCIeCntl);
	}
}



zoe_bool_t c_zv_av_lib_create_hpu(c_zv_av_lib *This)
{
	CHPUCntl    *pHPUCntl;

	// create HPU bus controller
	//
	pHPUCntl = (CHPUCntl *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                  sizeof(CHPUCntl),
                                                  sizeof(void *)
                                                  );
	if (!pHPUCntl)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_create_hpu() pHPUCntl == NULL!!!\n"
					   );
		return (ZOE_FALSE);
	}

	This->m_pHPUCntl = CHPUCntl_Constructor(pHPUCntl, 
											&This->m_Object,
											OBJECT_CRITICAL_LIGHT, 
											This->m_pDO, 
											This->m_PDOLayered,
											c_zv_av_lib_BusCallback,
											This,
                                            This->m_dbgID
											);
	if (!This->m_pHPUCntl)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_create_hpu() CHPUCntl_Constructor() failed\n"
					   );
		zoe_sosal_memory_free((void *)pHPUCntl);
		return (ZOE_FALSE);
	}

	return (ZOE_TRUE);
}



void c_zv_av_lib_delete_hpu(c_zv_av_lib *This)
{
	// delete HPU bus controller
	//
	if (This->m_pHPUCntl)
	{
		CHPUCntl_Destructor(This->m_pHPUCntl);
		zoe_sosal_memory_free((void *)This->m_pHPUCntl);
		This->m_pHPUCntl = ZOE_NULL;
	}
}



zoe_errs_t c_zv_av_lib_init_hpu(c_zv_av_lib *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	// initialize HPU bus controller
	//
	if (This->m_pHPUCntl)
	{
		err = i_zv_generic_device_init_device(This->m_pHPUCntl, 
										      ZOE_NULL,
										      ZOE_NULL,
										      (zoe_void_ptr_t)&This->m_InitData.codecInitData
										      );
		if (!ZOE_SUCCESS(err))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
						   "c_zv_av_lib_init_hpu() m_pHPUCntl->init_device Failed status(%d)!!!\n",
						   err
						   );
		}
	}

	return (err);
}



void c_zv_av_lib_done_hpu(c_zv_av_lib *This)
{
	// release HPU bus controller
	//
	if (This->m_pHPUCntl)
	{
		i_zv_generic_device_release(This->m_pHPUCntl);
	}
}

#endif //_ZVCODEC_PURE

