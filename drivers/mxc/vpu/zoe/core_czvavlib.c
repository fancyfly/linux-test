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
// czvavlib.c
//
// Description: 
//
//	zpu codec library.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "czvavlib.h"
#include "zoe_sosal.h"
#include "zoe_objids.h"
#include "zv_genericdevice.h"
#include "zv_devdiag.h"
#include "zoe_util.h"
#include "zoe_module_objids.h"
#include "zoe_module_core_src.h"
#include "zoe_module_core_sink.h"
#include "zoe_module_vdec_adaptor.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD


/////////////////////////////////////////////////////////////////////////////
//
//

// globals
//

extern c_zv_av_lib *g_zvavlib;

// module table
//
const ZOE_MODULE_TABLE_ENTRY g_module_table[] = 
{
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE),  zoe_module_core_source_create,  zoe_module_core_source_destroy},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),    zoe_module_core_sink_create,    zoe_module_core_sink_destroy},
#ifdef CONFIG_ZV4L2_USE_FWSIM
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VDEC_ADAPTOR), zoe_module_vdec_adaptor_create, zoe_module_vdec_adaptor_destroy}
#endif //CONFIG_ZV4L2_USE_FWSIM
};
uint32_t    g_module_table_count = SIZEOF_ARRAY(g_module_table);



/////////////////////////////////////////////////////////////////////////////
//
//

// statics
//
static CZoeIPCService   *s_pZoeIPCService = ZOE_NULL;  // IPC service
static c_zoe_module_mgr *s_p_zoe_module_mgr = ZOE_NULL;// module manager


/////////////////////////////////////////////////////////////////////////////
//
//

// singleton to get the ipc service
//
CZoeIPCService *c_zoe_ipc_service_get_ipc_svc(void)
{
    return (s_pZoeIPCService);
}



// singleton to get the module manager
//
c_zoe_module_mgr * c_zoe_module_mgr_get_module_mgr(void)
{
    return (s_p_zoe_module_mgr);
}


/////////////////////////////////////////////////////////////////////////////
//
//

#define I_ZVCODEC(This)	    (&This->m_pZVCodec->m_iZVCodec)


/////////////////////////////////////////////////////////////////////////////
//
//

// define _ZVCODEC_PURE to build the core driver 
// without the peripheral support 
//
zoe_bool_t c_zv_av_lib_create_usb(c_zv_av_lib *This);
void c_zv_av_lib_delete_usb(c_zv_av_lib *This);
zoe_errs_t c_zv_av_lib_init_usb(c_zv_av_lib *This);
void c_zv_av_lib_done_usb(c_zv_av_lib *This);
zoe_bool_t c_zv_av_lib_create_pcie(c_zv_av_lib *This);
void c_zv_av_lib_delete_pcie(c_zv_av_lib *This);
zoe_errs_t c_zv_av_lib_init_pcie(c_zv_av_lib *This);
void c_zv_av_lib_done_pcie(c_zv_av_lib *This);
zoe_bool_t c_zv_av_lib_create_hpu(c_zv_av_lib *This);
void c_zv_av_lib_delete_hpu(c_zv_av_lib *This);
zoe_errs_t c_zv_av_lib_init_hpu(c_zv_av_lib *This);
void c_zv_av_lib_done_hpu(c_zv_av_lib *This);


void zoe_ipc_test_register_srv(CZoeIPCService *p_zoe_ipc_srv, 
                               zoe_bool_t use_thread
                               );
void zoe_ipc_test_unregister_srv(CZoeIPCService *p_zoe_ipc_srv);
void zoe_ipc_test(CZoeIPCService *p_zoe_ipc_srv, 
                  ZOE_IPC_CPU to_cpu,
                  int iteration,
                  int seconds
                  );

/////////////////////////////////////////////////////////////////////////////
//
//

// c_object
//
zoe_bool_t c_zv_av_lib_init(c_object *This_p)
{
	c_zv_av_lib         *This = GET_INHERITED_OBJECT(c_zv_av_lib, m_Object, This_p);
    c_zv_codec          *pZVCodec;
    CZoeIPCService      *pZoeIPCService;
    c_zoe_module_mgr    *p_zoe_module_mgr;
    zoe_errs_t	        err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
				   "c_zv_av_lib_init()\n"
				   );

	ENTER_CRITICAL(This_p)

    err = c_zv_av_lib_open_scu(This);
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
				       "c_zv_av_lib_init() c_zv_av_lib_open_scu() Failed status(%d)!!!\n",
				       err
				       );
	    // goto c_zv_av_lib_init_err;
	}

    err = c_zv_av_lib_do_vpu_start(This);
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
				       "c_zv_av_lib_init() c_zv_av_lib_do_vpu_start() Failed status(%d)!!!\n",
				       err
				       );
	    // goto c_zv_av_lib_init_err;
	}

    switch (This->m_InitData.codecInitData.BusType)
    {
        case ZOEHAL_BUS_USB:

		    if (!c_zv_av_lib_create_usb(This))
		    {
			    goto c_zv_av_lib_init_err;
		    }

		    err = c_zv_av_lib_init_usb(This);

		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_zv_av_lib_init() c_zv_av_lib_init_usb() Failed status(%d)!!!\n",
						       err
						       );
			    goto c_zv_av_lib_init_err;
		    }
            break;

        case ZOEHAL_BUS_PCIe:

		    if (!c_zv_av_lib_create_pcie(This))
		    {
			    goto c_zv_av_lib_init_err;
		    }

		    err = c_zv_av_lib_init_pcie(This);

		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_zv_av_lib_init() c_zv_av_lib_init_pcie() Failed status(%d)!!!\n",
						       err
						       );
			    goto c_zv_av_lib_init_err;
		    }
            break;

        case ZOEHAL_BUS_HPU:

		    if (!c_zv_av_lib_create_hpu(This))
		    {
			    goto c_zv_av_lib_init_err;
		    }

		    err = c_zv_av_lib_init_hpu(This);

		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_zv_av_lib_init() c_zv_av_lib_init_hpu() Failed status(%d)!!!\n",
						       err
						       );
			    goto c_zv_av_lib_init_err;
		    }
            break;

        case ZOEHAL_BUS_EMULATION_USB:
        case ZOEHAL_BUS_EMULATION_PCIe:
        case ZOEHAL_BUS_EMULATION_HPU:
        default:
            break;
    }

	// Initialize HAL
	//
	err = zoehal_init(&This->m_iHal, 
                      This->m_InitData.codecInitData.BusType,
                      This->m_InitData.codecInitData.Instance,
                      This->m_pUsbCntl,
                      This->m_pPCIeCntl,
                      This->m_pHPUCntl,
                      This->m_dbgID
					  );
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() zoehal_init failed status(%d)\n", 
					   err
					   );
		This->m_bHALInited = ZOE_FALSE;
		goto c_zv_av_lib_init_err;
	}
	else
	{
		This->m_bHALInited = ZOE_TRUE;
	}

	// create components
	//

	// create codec chip
	//
	pZVCodec = (c_zv_codec *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                    sizeof(c_zv_codec),
                                                    0
                                                    );
	if (!pZVCodec)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() pZVCodec == NULL!!!\n"
					   );
		goto c_zv_av_lib_init_err;
	}

	// zero init the allocated memory
	//
	memset((void *)pZVCodec,
		   0,
		   sizeof(c_zv_codec)
		   );

	This->m_pZVCodec = c_zv_codec_constructor(pZVCodec,
											  &This->m_Object,
											  OBJECT_CRITICAL_LIGHT,
											  &This->m_iHal,
											  This->m_dbgID
											  );
	if (ZOE_NULL == This->m_pZVCodec)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() c_zv_codec_constructor() failed!!!\n" 
					   );
		zoe_sosal_memory_free((void *)pZVCodec);
		goto c_zv_av_lib_init_err;
	}

    // create IPC service
    //
	pZoeIPCService = (CZoeIPCService *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                              sizeof(CZoeIPCService),
                                                              0
                                                              );
	if (!pZoeIPCService)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() pZoeIPCService == NULL!!!\n"
					   );
		goto c_zv_av_lib_init_err;
	}

	// zero init the allocated memory
	//
	memset((void *)pZoeIPCService,
		   0,
		   sizeof(CZoeIPCService)
		   );

	This->m_pZoeIPCService = c_zoe_ipc_service_constructor(pZoeIPCService,
											               &This->m_Object,
											               OBJECT_CRITICAL_LIGHT,
											               &This->m_iHal,
											               This->m_dbgID,
                                                           zoe_sosal_isr_sw_my_isr_num(),
                                                           This->m_PDOLayered
											               );
	if (ZOE_NULL == This->m_pZoeIPCService)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() c_zoe_ipc_service_constructor() failed!!!\n" 
					   );
		zoe_sosal_memory_free((void *)pZoeIPCService);
		goto c_zv_av_lib_init_err;
	}
    else
    {
        s_pZoeIPCService = This->m_pZoeIPCService;
    }

    // create module manager
    //
	p_zoe_module_mgr = (c_zoe_module_mgr *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                  sizeof(c_zoe_module_mgr),
                                                                  0
                                                                  );
	if (!p_zoe_module_mgr)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() p_zoe_module_mgr == NULL!!!\n"
					   );
        goto c_zv_av_lib_init_err;
	}

    This->m_p_zoe_module_mgr = c_zoe_module_mgr_constructor(p_zoe_module_mgr,
											                ZOE_NULL, 
							                                OBJECT_CRITICAL_LIGHT,
                                                            &This->m_iHal,
                                                            This->m_dbgID
							                                );
	if (ZOE_NULL == This->m_p_zoe_module_mgr)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init() c_zoe_module_mgr_constructor() failed!!!\n" 
					   );
		zoe_sosal_memory_free((void *)p_zoe_module_mgr);
        goto c_zv_av_lib_init_err;
	}
    else
    {
        s_p_zoe_module_mgr = This->m_p_zoe_module_mgr;
    }

    // we are done
    //
	This_p->m_fInitialized = ZOE_TRUE; 
	LEAVE_CRITICAL(This_p)
	return (ZOE_TRUE);

c_zv_av_lib_init_err:
    c_zv_av_lib_do_vpu_stop(This);
    c_zv_av_lib_close_scu(This);
	LEAVE_CRITICAL(This_p)
	return (ZOE_FALSE);
}



zoe_bool_t c_zv_av_lib_done(c_object *This_p)
{
	c_zv_av_lib *This = GET_INHERITED_OBJECT(c_zv_av_lib, m_Object, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_done()\n"
				   );

	ENTER_CRITICAL(This_p)

	// delete all the objects we created during init()
	//
	if (This->m_pZVCodec)
	{
		c_zv_codec_destructor(This->m_pZVCodec);
		zoe_sosal_memory_free((void *)This->m_pZVCodec);
		This->m_pZVCodec = ZOE_NULL;
	}

    // delete module manager
    //
    if (This->m_p_zoe_module_mgr)
    {
        c_zoe_module_mgr_destructor(This->m_p_zoe_module_mgr);
		zoe_sosal_memory_free((void *)This->m_p_zoe_module_mgr);
        s_p_zoe_module_mgr = This->m_p_zoe_module_mgr = ZOE_NULL;
    }

    // delete IPC service
    //
    if (This->m_pZoeIPCService)
    {
        c_zoe_ipc_service_destructor(This->m_pZoeIPCService);
		zoe_sosal_memory_free((void *)This->m_pZoeIPCService);
		s_pZoeIPCService = This->m_pZoeIPCService = ZOE_NULL;
    }

	// HAL done
	//
	if (This->m_bHALInited)
	{
		zoehal_done(&This->m_iHal);
		This->m_bHALInited = ZOE_FALSE;
	}

	if (ZOEHAL_BUS_USB == This->m_InitData.codecInitData.BusType)
	{
		c_zv_av_lib_done_usb(This);
		c_zv_av_lib_delete_usb(This);
	}
	else if (ZOEHAL_BUS_PCIe == This->m_InitData.codecInitData.BusType)
	{
		c_zv_av_lib_done_pcie(This);
		c_zv_av_lib_delete_pcie(This);
	}
	else if (ZOEHAL_BUS_HPU == This->m_InitData.codecInitData.BusType)
	{
		c_zv_av_lib_done_hpu(This);
		c_zv_av_lib_delete_hpu(This);
	}

    c_zv_av_lib_do_vpu_stop(This);
    c_zv_av_lib_close_scu(This);

	This_p->m_fInitialized = ZOE_FALSE; 
	LEAVE_CRITICAL(This_p)
	return (ZOE_TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// i_zv_generic_device
//
static zoe_errs_t c_zv_av_lib_init_device(i_zv_av_lib *This_p, 
									      ZV_DEVICE_CALLBACK pFuncCallback,
		  							      zoe_void_ptr_t context,
		  							      zoe_void_ptr_t pInitData
		  							      )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_zv_av_lib	*This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_init_device() initdata(0x%x)\n",
				   pInitData
				   );

	ENTER_CRITICAL(&This->m_Object)

	if (!c_object_is_initialized(&This->m_Object))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID,
					   "c_zv_av_lib_init_device() Error Library Not Initialized\n" 
					   );
		err = ZOE_ERRS_FAIL;
		goto c_zv_av_lib_init_device_exit;
	}

	// Clear error condition
	//
	This->m_dwDeviceError = 0;

	// save callback address and context
	//
	This->m_pDeviceCallback = pFuncCallback;
	This->m_callbackContext = context;

	// initialize the codec chip
	//
	err = i_zv_generic_device_init_device(I_ZVCODEC(This), 
									      pFuncCallback,
									      context,
									      (zoe_void_ptr_t)&This->m_InitData.codecInitData
									      );
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_av_lib_init_device() m_pZVCodec->init_device Failed status(0x%x)!!!\n",
					   err
					   );
		goto c_zv_av_lib_init_device_exit;
	}

#if 1
	// downloading firmware
	//
    c_zv_av_lib_firmware_download(This);
#endif

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
    // enable nios jtag, sel_urt0_hw to secondary
    //
    ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800558, 2);
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL

    // open ipc service
    //
    err = c_zoe_ipc_service_open(This->m_pZoeIPCService);
    if (ZOE_FAIL(err))
    {
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID, 
                       "error c_zoe_ipc_service_open failed (%d)\n", 
                       err
                       );
		goto c_zv_av_lib_init_device_exit;
    }

c_zv_av_lib_init_device_exit:
	LEAVE_CRITICAL(&This->m_Object)
	return (err);
}




static zoe_errs_t c_zv_av_lib_release(i_zv_av_lib *This_p)
{
	c_zv_av_lib	*This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_release()\n"
				   );

	ENTER_CRITICAL(&This->m_Object)

	if (This->m_pZVCodec)
    {
		i_zv_generic_device_release(I_ZVCODEC(This));
    }

	LEAVE_CRITICAL(&This->m_Object)
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_zv_av_lib_reset(i_zv_av_lib *This_p)
{
	return (ZOE_ERRS_SUCCESS);
}



extern void CZoeIPCService_ISR(void * ctxt, 
                               zoe_sosal_isr_sw_numbers_t from_num
                               );


static zoe_errs_t c_zv_av_lib_set(i_zv_av_lib *This_p,
						          ZOE_REFGUID guid,
						          ZOE_OBJECT_HANDLE hIndex,
						          uint32_t dwCode,
						          zoe_void_ptr_t pInput,
						          zoe_void_ptr_t pOutput,
						          uint32_t dwSize
						          )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_zv_av_lib	*This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_set() code(%d)\n",
				   dwCode
				   );

	if (util_is_equal_guid((ZOE_REFGUID)&PROPSETID_ZV_CODEC_DIAG, 
					        guid
					        ))
	{
		switch (dwCode)
		{
			case ZV_CODEC_DIAG_MEM_WRITE:
			{
				PZV_CODEC_DIAG_MEMORY_STRUCT    pMem = (PZV_CODEC_DIAG_MEMORY_STRUCT)pInput;

				if (ZVCODEC_PWR_STATE_D0 != This->m_pZVCodec->m_PowerState)
				{
					zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
								   "ZV_CODEC_DIAG_MEM_WRITE wrong power state(%d)\n",
								   This->m_pZVCodec->m_PowerState
								   );
					err = ZOE_ERRS_INVALID;
				}
				else
				{
				    err = ZOEHAL_MEM_WRITE_EX(&This->m_iHal,
											  pMem->address, 
											  pMem->data,
                                              pMem->size,
                                              ZOE_FALSE
											  );
				}

				break;
			}

			case ZV_CODEC_DIAG_REG_WRITE:
			{
				PZV_CODEC_DIAG_REGISTER_STRUCT	pReg = (PZV_CODEC_DIAG_REGISTER_STRUCT)pInput;

				err = ZOEHAL_REG_WRITE(&This->m_iHal,
									   pReg->address, 
									   pReg->value
									   );
				break;
			}

			case ZV_CODEC_DIAG_REG_WRITE_EX:
			{
				PZV_CODEC_DIAG_REGISTER_STRUCT_EX   pReg = (PZV_CODEC_DIAG_REGISTER_STRUCT_EX)pInput;

				err = ZOEHAL_REG_WRITE_EX(&This->m_iHal,
									      pReg->address, 
									      &pReg->value[0],
                                          pReg->num_reg
									      );
				break;
			}

			case ZV_CODEC_DIAG_DMA_WRITE:
			{
				PZV_CODEC_DIAG_DMA_STRUCT	pDMA = (PZV_CODEC_DIAG_DMA_STRUCT)pInput;

				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_dbgID,
							   "c_zv_av_lib_set() ZV_CODEC_DIAG_DMA_WRITE addr(0x%x) size(%d)\n",
							   pDMA->deviceAddress,
							   pDMA->size
							   );

				if (ZVCODEC_PWR_STATE_D0 != This->m_pZVCodec->m_PowerState)
				{
					zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
								   "c_zv_av_lib_set() ZV_CODEC_DIAG_DMA_WRITE wrong power state(%d)\n",
								   This->m_pZVCodec->m_PowerState
								   );
					err = ZOE_ERRS_INVALID;
				}
				else
				{
					ENTER_CRITICAL(&This->m_Object)
					err = c_zv_codec_dma_write(This->m_pZVCodec,
											   pDMA->deviceAddress,
											   pDMA->data,
											   pDMA->size,
											   pDMA->bSwap,
											   ZOE_TRUE,
											   DMA_BUFFER_MODE_COMMON,
                                               ZOE_NULL
											   );
					LEAVE_CRITICAL(&This->m_Object)
				}

				break;
			}

			case ZV_CODEC_DIAG_DMA_WRITE_PTR:
			{
				PZV_CODEC_DIAG_DMA_PTR_STRUCT	pDMA = (PZV_CODEC_DIAG_DMA_PTR_STRUCT)pInput;

				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_dbgID,
							   "c_zv_av_lib_set() ZV_CODEC_DIAG_DMA_WRITE_PTR addr(0x%x) size(%d) buf(0x%x)\n",
							   pDMA->deviceAddress,
							   pDMA->size,
							   pDMA->pData
							   );

				if (ZVCODEC_PWR_STATE_D0 != This->m_pZVCodec->m_PowerState)
				{
					zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
								   "c_zv_av_lib_set() ZV_CODEC_DIAG_DMA_WRITE_PTR wrong power state(%d)\n",
								   This->m_pZVCodec->m_PowerState
								   );
					err = ZOE_ERRS_INVALID;
				}
				else
				{
					ENTER_CRITICAL(&This->m_Object)
					err = c_zv_codec_dma_write(This->m_pZVCodec,
											   pDMA->deviceAddress,
											   pDMA->pData,
											   pDMA->size,
											   pDMA->bSwap,
											   ZOE_TRUE,
											   pDMA->ulXferMode,
                                               ZOE_NULL
											   );
					LEAVE_CRITICAL(&This->m_Object)
				}

				break;
			}

            case ZV_CODEC_DIAG_ENABLE_WAIT_ISR:
			{
				PZV_CODEC_DIAG_ENABLE_WAIT_ISR_STRUCT   pEnable = (PZV_CODEC_DIAG_ENABLE_WAIT_ISR_STRUCT)pInput;

	            ENTER_CRITICAL(&This->m_Object)

                if (pEnable && 
                    (pEnable->from_cpu_num < ZOE_SOSAL_ISR_SW_NUM)
                    )
                {

                    This->m_iHal.m_b_enable_wait_isrs[pEnable->from_cpu_num] = ZOE_TRUE;
                }
                else
                {
                    err = ZOE_ERRS_PARMS;
                }
	            LEAVE_CRITICAL(&This->m_Object)
				break;
			}

            case ZV_CODEC_DIAG_DISABLE_WAIT_ISR:
			{
				PZV_CODEC_DIAG_DISABLE_WAIT_ISR_STRUCT  pDisable = (PZV_CODEC_DIAG_DISABLE_WAIT_ISR_STRUCT)pInput;

	            ENTER_CRITICAL(&This->m_Object)

                if (pDisable && 
                    (pDisable->from_cpu_num < ZOE_SOSAL_ISR_SW_NUM)
                    )
                {

                    This->m_iHal.m_b_enable_wait_isrs[pDisable->from_cpu_num] = ZOE_FALSE;
                    zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[pDisable->from_cpu_num]);
                }
                else
                {
                    err = ZOE_ERRS_PARMS;
                }
	            LEAVE_CRITICAL(&This->m_Object)
				break;
			}

            case ZV_CODEC_DIAG_SET_ISR:
			{
				PZV_CODEC_DIAG_SET_ISR_STRUCT   pSet = (PZV_CODEC_DIAG_SET_ISR_STRUCT)pInput;

                if (zoe_sosal_isr_sw_my_isr_num() == pSet->to_cpu_num)
                {
                    // user mode 2 kernel interrupt
                    CZoeIPCService_ISR((void *)This->m_pZoeIPCService, pSet->from_cpu_num);
                }
                else if (pSet->to_cpu_num == pSet->from_cpu_num)
                {
                    // user mode loopback
                    if (This->m_iHal.m_b_enable_wait_isrs[pSet->from_cpu_num])
                    {
                        zoe_sosal_event_set(This->m_iHal.m_evt_wait_isrs[pSet->from_cpu_num]);
                    }
                }
                else
                {
                    // user mode 2 other cpu's
                    err = ZOEHAL_ISR_SW_TRIGGER(&This->m_iHal, 
                                                pSet->to_cpu_num, 
                                                pSet->from_cpu_num
                                                );
                }
				break;
			}

			case ZV_CODEC_DIAG_IPC_REG:
			{
				PZV_CODEC_DIAG_IPC_REG_STRUCT   pIPCReg = (PZV_CODEC_DIAG_IPC_REG_STRUCT)pInput;

                if (0 == pIPCReg->reg)
                {
                    zoe_ipc_test_unregister_srv(This->m_pZoeIPCService);
                }
                else
                {
                    zoe_ipc_test_register_srv(This->m_pZoeIPCService, 
                                              ZOE_FALSE
                                              );
                }
				break;
			}

			case ZV_CODEC_DIAG_IPC_TEST:
			{
				PZV_CODEC_DIAG_IPC_TEST_STRUCT  pIPCtest = (PZV_CODEC_DIAG_IPC_TEST_STRUCT)pInput;

				zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
							   "\r\n   c_zv_av_lib_set() ZV_CODEC_DIAG_IPC_TEST Set IPC test to cpu(%d) %d X %d sec\r\n",
                               pIPCtest->to_cpu, 
                               pIPCtest->iteration, 
                               pIPCtest->second
							   );
                zoe_ipc_test(This->m_pZoeIPCService, 
                             pIPCtest->to_cpu, 
                             pIPCtest->iteration,
                             pIPCtest->second
                             );
				break;
			}

			default:
		        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
					           "c_zv_av_lib_set() unknown code(%d)\n",
                               dwCode
					           );
		        err = ZOE_ERRS_NOTIMPL;
				break;
		}
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
					   "c_zv_av_lib_set() unknown property\n" 
					   );
		err = ZOE_ERRS_NOTIMPL;
	}

	return (err);
}



static zoe_errs_t c_zv_av_lib_get(i_zv_av_lib *This_p, 
							      ZOE_REFGUID guid,
							      ZOE_OBJECT_HANDLE hIndex,
							      uint32_t dwCode,
							      zoe_void_ptr_t pInput,
							      zoe_void_ptr_t pOutput,
							      uint32_t * pdwSizeGot
							      )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_zv_av_lib	*This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_get() code(%d)\n", 
				   dwCode
				   );

	if (util_is_equal_guid((ZOE_REFGUID)&PROPSETID_ZV_CODEC_DIAG, 
					       guid
					       ))
	{
		switch (dwCode)
		{
			case ZV_CODEC_DIAG_MEM_READ:
			{
				PZV_CODEC_DIAG_MEMORY_STRUCT	pMemIn = (PZV_CODEC_DIAG_MEMORY_STRUCT)pInput;
				PZV_CODEC_DIAG_MEMORY_STRUCT	pMemOut = (PZV_CODEC_DIAG_MEMORY_STRUCT)pOutput;

				if (ZVCODEC_PWR_STATE_D0 != This->m_pZVCodec->m_PowerState)
				{
					zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
								   "ZV_CODEC_DIAG_MEM_READ wrong power state(%d)\n",
								   This->m_pZVCodec->m_PowerState
								   );
					err = ZOE_ERRS_INVALID;
				}
				else
				{
					err = ZOEHAL_MEM_READ_EX(&This->m_iHal,
										     pMemIn->address, 
										     pMemOut->data,
                                             pMemIn->size,
                                             ZOE_FALSE
											 );
				}
				pMemOut->size = pMemIn->size;
				pMemOut->address = pMemIn->address;
				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_MEMORY_STRUCT);
				break;
			}

			case ZV_CODEC_DIAG_REG_READ:
			{
				PZV_CODEC_DIAG_REGISTER_STRUCT	pRegIn = (PZV_CODEC_DIAG_REGISTER_STRUCT)pInput;
				PZV_CODEC_DIAG_REGISTER_STRUCT	pRegOut = (PZV_CODEC_DIAG_REGISTER_STRUCT)pOutput;

				err = ZOEHAL_REG_READ(&This->m_iHal,
									  pRegIn->address, 
									  &pRegOut->value
									  );
				pRegOut->address = pRegIn->address;
				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_REGISTER_STRUCT);
				break;
			}

			case ZV_CODEC_DIAG_REG_READ_EX:
			{
				PZV_CODEC_DIAG_REGISTER_STRUCT_EX	pRegIn = (PZV_CODEC_DIAG_REGISTER_STRUCT_EX)pInput;
				PZV_CODEC_DIAG_REGISTER_STRUCT_EX	pRegOut = (PZV_CODEC_DIAG_REGISTER_STRUCT_EX)pOutput;

				err = ZOEHAL_REG_READ_EX(&This->m_iHal,
										 pRegIn->address, 
										 &pRegOut->value[0],
										 pRegIn->num_reg
										 );
				pRegOut->address = pRegIn->address;
				pRegOut->num_reg = pRegIn->num_reg;
				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_REGISTER_STRUCT_EX);
				break;
			}

			case ZV_CODEC_DIAG_DMA_READ:
			{
				PZV_CODEC_DIAG_DMA_STRUCT	pDMA = (PZV_CODEC_DIAG_DMA_STRUCT)pOutput;
				PZV_CODEC_DIAG_DMA_STRUCT	pCmd = (PZV_CODEC_DIAG_DMA_STRUCT)pInput;

				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_dbgID,
							   "c_zv_av_lib_get() ZV_CODEC_DIAG_DMA_READ addr(0x%x) size(%d)\n",
							   pCmd->deviceAddress,
							   pCmd->size
							   );

				if (ZVCODEC_PWR_STATE_D0 != This->m_pZVCodec->m_PowerState)
				{
					zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
								   "c_zv_av_lib_get() ZV_CODEC_DIAG_DMA_READ wrong power state(%d)\n",
								   This->m_pZVCodec->m_PowerState
								   );
					err = ZOE_ERRS_INVALID;
				}
				else
				{
					ENTER_CRITICAL(&This->m_Object)
					err = c_zv_codec_dma_read(This->m_pZVCodec,
											  pCmd->deviceAddress,
											  pDMA->data,
											  pCmd->size,
											  pCmd->bSwap,
											  ZOE_TRUE,
											  DMA_BUFFER_MODE_COMMON,
                                              ZOE_NULL
											  );
					LEAVE_CRITICAL(&This->m_Object)
				}

				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_DMA_STRUCT);
				break;
			}

			case ZV_CODEC_DIAG_DMA_READ_PTR:
			{
				PZV_CODEC_DIAG_DMA_PTR_STRUCT	pDMA = (PZV_CODEC_DIAG_DMA_PTR_STRUCT)pOutput;
				PZV_CODEC_DIAG_DMA_PTR_STRUCT	pCmd = (PZV_CODEC_DIAG_DMA_PTR_STRUCT)pInput;

				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_dbgID,
							   "c_zv_av_lib_get() ZV_CODEC_DIAG_DMA_READ_PTR addr(0x%x) size(%d) buf(0x%x)\n",
							   pCmd->deviceAddress,
							   pCmd->size,
							   pCmd->pData
							   );

				if (ZVCODEC_PWR_STATE_D0 != This->m_pZVCodec->m_PowerState)
				{
					zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
								   "c_zv_av_lib_get() ZV_CODEC_DIAG_DMA_READ_PTR wrong power state(%d)\n",
								   This->m_pZVCodec->m_PowerState
								   );
					err = ZOE_ERRS_INVALID;
				}
				else
				{
					ENTER_CRITICAL(&This->m_Object)
					err = c_zv_codec_dma_read(This->m_pZVCodec,
											  pCmd->deviceAddress,
											  pCmd->pData,
											  pCmd->size,
											  pCmd->bSwap,
											  ZOE_TRUE,
                                              pCmd->ulXferMode,
                                              ZOE_NULL
											  );
					LEAVE_CRITICAL(&This->m_Object)
				}

				pDMA->deviceAddress = pCmd->deviceAddress;
				pDMA->pData = pCmd->pData;
				pDMA->size = pCmd->size;
				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_DMA_PTR_STRUCT);
				break;
			}

            case ZV_CODEC_DIAG_WAIT_ISR:
            {
                PZV_CODEC_DIAG_WAIT_ISR_STRUCT  pWait = (PZV_CODEC_DIAG_WAIT_ISR_STRUCT)pOutput;
                PZV_CODEC_DIAG_WAIT_ISR_STRUCT  pCmd = (PZV_CODEC_DIAG_WAIT_ISR_STRUCT)pInput;

                if (pCmd && 
                    (pCmd->from_cpu_num < ZOE_SOSAL_ISR_SW_NUM) &&
                    (ZOE_TRUE == This->m_iHal.m_b_enable_wait_isrs[pCmd->from_cpu_num])
                    )
                {
                    err = zoe_sosal_event_wait(This->m_iHal.m_evt_wait_isrs[pCmd->from_cpu_num], 
                                               pCmd->timeout_ms * 1000
                                               );
                }
                else
                {
                    err = ZOE_ERRS_PARMS;
                }
                pWait->from_cpu_num = pCmd->from_cpu_num;
                pWait->timeout_ms = pCmd->timeout_ms;
				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_WAIT_ISR_STRUCT);
                break;
            }

			case ZV_CODEC_DIAG_CHIP_VERSION:
			{
				PZV_CODEC_DIAG_CHIP_VERSION_STRUCT  pVerOut = (PZV_CODEC_DIAG_CHIP_VERSION_STRUCT)pOutput;

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
                pVerOut->chip = ZV_CODEC_DIAG_CHIP_VERSION_CHISEL;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
                pVerOut->chip = ZV_CODEC_DIAG_CHIP_VERSION_CAFE;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
                pVerOut->chip = ZV_CODEC_DIAG_CHIP_VERSION_iMX8;
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP
				*pdwSizeGot = sizeof(ZV_CODEC_DIAG_CHIP_VERSION_STRUCT);
				break;
			}

			default:
		        zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                               This->m_dbgID,
					           "c_zv_av_lib_get() unknown code(%d)\n",
                               dwCode
					           );
		        err = ZOE_ERRS_NOTIMPL;
				break;
		}
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                       This->m_dbgID,
					   "c_zv_av_lib_get() unknown property\n" 
					   );
		err = ZOE_ERRS_NOTIMPL;
	}

	return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// i_zv_av_lib
//
static zoe_errs_t c_zv_av_lib_get_codec(i_zv_av_lib *This_p, 
									    i_zv_codec** ppCodec
									    )
{
	c_zv_av_lib    *This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	*ppCodec = This->m_pZVCodec ? I_ZVCODEC(This) : ZOE_NULL;
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_get_codec() *ppCodec(0x%X)\n", 
				   *ppCodec
				   );
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_zv_av_lib_disable(i_zv_av_lib *This_p)
{
	zoe_errs_t	err;
	c_zv_av_lib	*This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_disable()\n"
				   );

	// need to program something to turn off the power
	//

	err = i_zv_generic_device_release(This_p);

	if (ZOE_SUCCESS(err))
	{
        switch (This->m_InitData.codecInitData.BusType)
        {
            case ZOEHAL_BUS_PCIe:
		        c_zv_av_lib_done_pcie(This);
                break;
            case ZOEHAL_BUS_USB:
                c_zv_av_lib_done_usb(This);
                break;
            case ZOEHAL_BUS_HPU:
		        c_zv_av_lib_done_hpu(This);
                break;
            default:
                break;
        }
	}

	return (err);
}



static zoe_errs_t c_zv_av_lib_enable(i_zv_av_lib *This_p)
{
	zoe_errs_t	err;
	c_zv_av_lib	*This =	GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_enable()\n" 
				   );

    switch (This->m_InitData.codecInitData.BusType)
	{
        case ZOEHAL_BUS_PCIe:
		    err = c_zv_av_lib_init_pcie(This);
		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_zv_av_lib_enable() c_zv_av_lib_init_pcie() Failed status(%d)!!!\n",
						       err
						       );
			    return (err);
		    }
            break;

        case ZOEHAL_BUS_USB:
		    err = c_zv_av_lib_init_usb(This);
		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_zv_av_lib_enable() c_zv_av_lib_init_usb() Failed status(%d)!!!\n",
						       err
						       );
			    return (err);
		    }
            break;

        case ZOEHAL_BUS_HPU:
		    err = c_zv_av_lib_init_hpu(This);
		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_zv_av_lib_enable() c_zv_av_lib_init_hpu() Failed status(%d)!!!\n",
						       err
						       );
			    return (err);
		    }
            break;

        default:
            break;
	}

	err = i_zv_generic_device_init_device(This_p, 
									      This->m_pDeviceCallback, 
									      This->m_callbackContext, 
									      ZOE_NULL
									      );
	return (err);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// Error
//
void c_zv_av_lib_set_error(c_zv_av_lib *This, 
					       uint32_t dwError
					       )
{
	This->m_dwDeviceError |= dwError;
}



void c_zv_av_lib_clr_error(c_zv_av_lib *This)
{
	This->m_dwDeviceError = 0;
}



uint32_t c_zv_av_lib_get_error(c_zv_av_lib *This)
{
	return (This->m_dwDeviceError);
}


zoe_bool_t c_zv_av_lib_is_error(c_zv_av_lib *This)
{
	return (0 != This->m_dwDeviceError);
}



// constructor
//
c_zv_av_lib * c_zv_av_lib_constructor(c_zv_av_lib *pZVAVLib,
							          zoe_void_ptr_t pPDO,
								      zoe_void_ptr_t pPDOLayered,
								      PZV_AVLIB_INITDATA pInitData,
                                      zoe_dbg_comp_id_t dbgID
								      )
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   dbgID,
				   "c_zv_av_lib_constructor()\n"
				   );

	if (pZVAVLib)
	{
		c_object_constructor(&pZVAVLib->m_Object, 
							 ZOE_NULL,
                             OBJECT_ZOE_ZVAVLIB,
							 OBJECT_CRITICAL_LIGHT
		  					 );

		// fill in the function table
		//

        // c_object
        //
		pZVAVLib->m_Object.init = c_zv_av_lib_init;
		pZVAVLib->m_Object.done = c_zv_av_lib_done;

		// i_zv_generic_device
		//
		pZVAVLib->m_iZVAVLib.init_device = c_zv_av_lib_init_device;
		pZVAVLib->m_iZVAVLib.release = c_zv_av_lib_release;
		pZVAVLib->m_iZVAVLib.reset = c_zv_av_lib_reset;
		pZVAVLib->m_iZVAVLib.set = c_zv_av_lib_set;
		pZVAVLib->m_iZVAVLib.get = c_zv_av_lib_get;

		// i_zv_av_lib
		//
		pZVAVLib->m_iZVAVLib.get_codec = c_zv_av_lib_get_codec;
		pZVAVLib->m_iZVAVLib.disable = c_zv_av_lib_disable;
		pZVAVLib->m_iZVAVLib.enable = c_zv_av_lib_enable;

		pZVAVLib->m_pDO = pPDO;
		pZVAVLib->m_PDOLayered = pPDOLayered;

		// copy init data
		//
		memcpy(&pZVAVLib->m_InitData,
			   pInitData,
			   sizeof(ZV_AVLIB_INITDATA)
			   );

		pZVAVLib->m_pDeviceCallback = ZOE_NULL;
		pZVAVLib->m_callbackContext = ZOE_NULL;
        pZVAVLib->m_dbgID = dbgID;

		// hardware components
		//
		pZVAVLib->m_pPCIeCntl = ZOE_NULL;
		pZVAVLib->m_pUsbCntl = ZOE_NULL;
		pZVAVLib->m_pHPUCntl = ZOE_NULL;
		pZVAVLib->m_pZVCodec = ZOE_NULL;

        // software components
        //
        pZVAVLib->m_pZoeIPCService = ZOE_NULL;
        pZVAVLib->m_p_zoe_module_mgr = ZOE_NULL;

        // firmware space
        //
        pZVAVLib->m_p_fw_space_vir = ZOE_NULL;
        pZVAVLib->m_p_fw_space_phy = 0;

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#ifdef ZOE_LINUXKER_BUILD
        pZVAVLib->m_p_fw_space_vir = dma_alloc_coherent((struct device *)pPDOLayered, 
                                                        0x800000, // 8 MB??
                                                        (dma_addr_t *)&pZVAVLib->m_p_fw_space_phy,
					                                    GFP_KERNEL | GFP_DMA32
                                                        );
        if (!pZVAVLib->m_p_fw_space_vir)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
                           "%s() dma_alloc_coherent failed!!!\n",
                           __FUNCTION__
		                   );

//            c_zv_av_lib_destructor(pZoeIPCService);
//            return (ZOE_NULL);
        }
#endif //ZOE_LINUXKER_BUILD
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       dbgID,
                       "%s() dma_alloc_coherent phy(%p) vir(%p)\n",
                       __FUNCTION__,
                       (void *)pZVAVLib->m_p_fw_space_phy,
                       pZVAVLib->m_p_fw_space_vir
		               );
#endif //ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8

		// Error
        //
		pZVAVLib->m_dwDeviceError = 0;

#ifndef _NO_SCU_API
        // scu ipc handle
        //
        pZVAVLib->m_ipcHndl = 0;
#endif //!_NO_SCU_API
	}

	return (pZVAVLib);
}



// destructor
//
void c_zv_av_lib_destructor(c_zv_av_lib *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_av_lib_destructor()\n"
				   );
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
    if (This->m_p_fw_space_vir)
    {
#ifdef ZOE_LINUXKER_BUILD
		dma_free_coherent(0,
					      0x800000, 
					      This->m_p_fw_space_vir, 
					      This->m_p_fw_space_phy
					      );
#endif //ZOE_LINUXKER_BUILD
		This->m_p_fw_space_vir = ZOE_NULL;
		This->m_p_fw_space_phy = 0;
    }
#endif //ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8

    c_zv_av_lib_do_vpu_stop(This);
    c_zv_av_lib_close_scu(This);

	// c_object
	//
	c_object_destructor(&This->m_Object);
}





