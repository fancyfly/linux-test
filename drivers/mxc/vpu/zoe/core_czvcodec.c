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
// czvcodec.c
//
// Description: 
//
//  This is the implementation of the VPU codec
// 
// Authors: (dt) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "czvcodec.h"
#include "czvavlib.h"
#include "zoe_util.h"
#include "zoe_objids.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD
#include "zv_devcntl.h"
#include "cchannel.h"
#include "ctask.h"
#include "zoe_module_vdec_intf_clnt.h"


/////////////////////////////////////////////////////////////////////////////
//
//

// i_zv_codec
zoe_errs_t c_zv_codec_alloc_task(i_zv_codec *This, 
                                 ZV_CODEC_TASK_TYPE type,
						         PZOE_OBJECT_HANDLE phTask
						         );
zoe_errs_t c_zv_codec_release_task(i_zv_codec *This, 
						           ZOE_OBJECT_HANDLE hTask
						           );
zoe_errs_t c_zv_codec_open(i_zv_codec *This, 
				           ZOE_OBJECT_HANDLE hTask,
				           uint32_t dwFlags,  // open type
				           zoe_void_ptr_t pDataFormat,
				           PZOE_OBJECT_HANDLE phStream,
				           ZV_DEVICE_CALLBACK pFuncCallback,
				           zoe_void_ptr_t context
				           );
zoe_errs_t c_zv_codec_close(i_zv_codec *This, 
					        ZOE_OBJECT_HANDLE hStream
					        );
zoe_errs_t c_zv_codec_start(i_zv_codec *This, 
					        ZOE_OBJECT_HANDLE hStream
					        );
zoe_errs_t c_zv_codec_stop(i_zv_codec *This, 
					       ZOE_OBJECT_HANDLE hStream
					       );
zoe_errs_t c_zv_codec_acquire(i_zv_codec *This, 
					          ZOE_OBJECT_HANDLE hStream
					          );
zoe_errs_t c_zv_codec_pause(i_zv_codec *This, 
					        ZOE_OBJECT_HANDLE hStream
					        );
zoe_errs_t c_zv_codec_set_rate(i_zv_codec *This, 
					           ZOE_OBJECT_HANDLE hStream,
					           int32_t lNewRate
					           );
zoe_errs_t c_zv_codec_get_rate(i_zv_codec *This, 
					           ZOE_OBJECT_HANDLE hStream,
					           int32_t * plRate
					           );
zoe_errs_t c_zv_codec_begin_flush(i_zv_codec *This, 
						          ZOE_OBJECT_HANDLE hStream
						          );
zoe_errs_t c_zv_codec_flush(i_zv_codec *This, 
					        ZOE_OBJECT_HANDLE hStream
					        );
zoe_errs_t c_zv_codec_end_flush(i_zv_codec *This, 
					            ZOE_OBJECT_HANDLE hStream
					            );
zoe_errs_t c_zv_codec_add_buffer(i_zv_codec *This, 
						         ZOE_OBJECT_HANDLE hStream,
						         PZV_BUFFER_DESCRIPTOR pBufDesc
						         );
zoe_errs_t c_zv_codec_cancel_buffer(i_zv_codec *This, 
						            ZOE_OBJECT_HANDLE hStream,
						            PZV_BUFFER_DESCRIPTOR pBufDesc
						            );
zoe_errs_t c_zv_codec_timeout_buffer(i_zv_codec *This, 
							         ZOE_OBJECT_HANDLE hStream,
	  						         PZV_BUFFER_DESCRIPTOR pBufDesc
	  						         );


/////////////////////////////////////////////////////////////////////////////
//
//

static void c_zv_codec_load_default_settings(c_zv_codec *This)
{
}



/////////////////////////////////////////////////////////////////////////////
//
//

// call back function
//
zoe_errs_t c_zv_codec_device_callback(c_zv_codec *This, 
								      uint32_t dwCode,
								      zoe_void_ptr_t pParam
								      )
{
	if (This->m_pDeviceCallback && 
		This->m_callbackContext
		) 
	{
		return (This->m_pDeviceCallback(This->m_callbackContext, 
										dwCode, 
										pParam
										));
	}
	else
	{
		return (ZOE_ERRS_FAIL);
	}
}



// initialize memory controller 
//
zoe_errs_t c_zv_codec_initialize_memory(c_zv_codec *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_initialize_memory()\n"
				   );

	if ((ZOEHAL_BUS_INTERNAL == This->m_pHal->m_bus_type) ||
        (ZOEHAL_BUS_HPU == This->m_pHal->m_bus_type) ||
        (ZOEHAL_BUS_USB == This->m_pHal->m_bus_type)
		)
	{
		return (ZOE_ERRS_SUCCESS);
	}

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_zv_codec_power_up(c_zv_codec *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	if ((ZOEHAL_BUS_INTERNAL == This->m_pHal->m_bus_type) ||
        (ZOEHAL_BUS_HPU == This->m_pHal->m_bus_type)
        )
	{
		This->m_PowerState = ZVCODEC_PWR_STATE_D0;
		return (err);
	}

	ENTER_CRITICAL(&This->m_Object)

	if (ZVCODEC_PWR_STATE_D0 != This->m_PowerState)
	{
		This->m_PowerState = ZVCODEC_PWR_STATE_D0;
	}

	LEAVE_CRITICAL(&This->m_Object)

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   This->m_dbgID,
				   "c_zv_codec_power_up status(%d) power state(%d)\n", 
				   err,
				   This->m_PowerState
				   );
	return (err);
}



zoe_errs_t c_zv_codec_power_down(c_zv_codec *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	
	if ((ZOEHAL_BUS_INTERNAL == This->m_pHal->m_bus_type) ||
        (ZOEHAL_BUS_HPU == This->m_pHal->m_bus_type)
        )
	{
		return (err);
	}

	ENTER_CRITICAL(&This->m_Object)

	if (ZVCODEC_PWR_STATE_D0 == This->m_PowerState)
	{
		This->m_PowerState = ZVCODEC_PWR_STATE_D1;
	}

    This->m_fw_loaded = ZOE_FALSE;

	LEAVE_CRITICAL(&This->m_Object)

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   This->m_dbgID,
				   "c_zv_codec_power_down status(%d) power state(%d)\n", 
				   err,
				   This->m_PowerState
				   );
	return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// IGenericDevice
//
static zoe_errs_t c_zv_codec_init_device(i_zv_codec *This_p, 
									     ZV_DEVICE_CALLBACK pFuncCallback,
									     zoe_void_ptr_t context,
									     zoe_void_ptr_t pInitData
									     )
{
	zoe_errs_t			err = ZOE_ERRS_SUCCESS;
	PZVCODEC_INITDATA	pZvCodecInitData = (PZVCODEC_INITDATA)pInitData;
	c_zv_codec			*This =	GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_init_device() This_p(0x%x) This(0x%x) initData(0x%x)\n", 
				   This_p,
				   This,
				   pZvCodecInitData
				   );

	if (c_object_is_initialized(&This->m_Object))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       This->m_dbgID,
					   "c_zv_codec_init_device() Already Initialized. Skipping...\n"
					   );
		return (ZOE_ERRS_SUCCESS);
	}

	// remember initialization data
	//
	if (pZvCodecInitData)
	{
	}

	// load default settings based on chip type
	//
	c_zv_codec_load_default_settings(This);

	// initialize hardware
	//
	if (!pZvCodecInitData->bDontInitHW)
	{
		// initialize memory controller
		//
		c_zv_codec_initialize_memory(This);
	}

	// save callback address and context
	//
	This->m_pDeviceCallback = pFuncCallback;
	This->m_callbackContext = context;

    // power state
    //
	This->m_PowerState = ZVCODEC_PWR_STATE_D0;

	// c_object Init
	//
	c_object_init(&This->m_Object);

	return (err);
}



static zoe_errs_t c_zv_codec_release(i_zv_codec *This_p)
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_release()\n"
				   );

	// close all the opened channels
	//
	while (0 != c_object_mgr_get_number_of_objects(This->m_pChannelMgr))
	{
		c_channel   *pChannel = c_object_mgr_get_object_by_index(This->m_pChannelMgr, 
															     0
															     );
		if (pChannel)
		{
			c_zv_codec_close(&This->m_iZVCodec, 
						     pChannel->m_hChannel
						     );
		}
	}

	// close all the opened tasks
	//
	while (0 != c_object_mgr_get_number_of_objects(This->m_pTaskMgr))
	{
		c_task  *pTask = c_object_mgr_get_object_by_index(This->m_pTaskMgr, 
													      0
													      );
		if (pTask)
		{
			c_zv_codec_release_task(&This->m_iZVCodec, 
						            pTask->m_hTask
						            );
		}
	}

	// power state
	//
	This->m_PowerState = ZVCODEC_PWR_STATE_D3;

	// clear callback function
	//
	This->m_pDeviceCallback = ZOE_NULL;
	This->m_callbackContext = ZOE_NULL;

	// c_object Done
	//
	c_object_done(&This->m_Object);

	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t c_zv_codec_reset(i_zv_codec *This_p)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_zv_codec	*This =	GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_reset()\n"
				   );

	return (err);
}



static zoe_errs_t c_zv_codec_set(i_zv_codec *This_p, 
							     ZOE_REFGUID guid,
							     ZOE_OBJECT_HANDLE hIndex,
							     uint32_t dwCode,
							     zoe_void_ptr_t pInput,
							     zoe_void_ptr_t pOutput,
							     uint32_t dwSize
							     )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_zv_codec	*This =	GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
    c_task      *pTask;
    ZOE_IPC_CPU cpu_id;
    uint32_t    module;
    uint32_t    inst;

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
				   "c_zv_codec_set() hIndex(%d) dwCode(%d)\n",
				   hIndex,
                   dwCode
				   );

	pTask = (c_task *)c_object_mgr_get_object_by_handle(This->m_pTaskMgr, 
                                                        hIndex
                                                        );

	if (!pTask)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_set() invalid task\n" 
					   );
		return (ZOE_ERRS_PARMS);
	}

	if (util_is_equal_guid((ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL, 
					       guid
					       ))
	{
		switch (dwCode)
		{
            case ZV_CODEC_PROP_DEC_FMT:
            {
                uint32_t    *p_fmt = (uint32_t *)pInput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_format_set_clnt(*p_fmt,
                                                          cpu_id,
                                                          module,
                                                          inst
                                                          );
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_MODE:
            {
                uint32_t    *p_mode = (uint32_t *)pInput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_mode_set_clnt(*p_mode,
                                                        cpu_id,
                                                        module,
                                                        inst
                                                        );
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_ORDER:
            {
                uint32_t    *p_order = (uint32_t *)pInput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_order_set_clnt(*p_order,
                                                         cpu_id,
                                                         module,
                                                         inst
                                                         );
                }
                break;
            }   

            case ZV_CODEC_PROP_DEC_YUV_PIXEL_FMT:
            {
                uint32_t    *p_fmt = (uint32_t *)pInput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_yuv_format_set_clnt(*p_fmt,
                                                              cpu_id,
                                                              module,
                                                              inst
                                                              );
                }
                break;
            }   

			default:
		        err = ZOE_ERRS_NOTIMPL;
				break;
		}
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_set() unknown property\n" 
					   );
		err = ZOE_ERRS_NOTIMPL;
	}

	return (err);
}



static zoe_errs_t c_zv_codec_get(i_zv_codec *This_p, 
							     ZOE_REFGUID guid,
							     ZOE_OBJECT_HANDLE hIndex,
							     uint32_t dwCode,
							     zoe_void_ptr_t pInput,
							     zoe_void_ptr_t pOutput,
							     uint32_t * pdwSizeGot
							     )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_zv_codec	*This =	GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
    c_task      *pTask;
    ZOE_IPC_CPU cpu_id;
    uint32_t    module;
    uint32_t    inst;

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
				   "c_zv_codec_get() hIndex(%d) dwCode(%d)\n",
				   hIndex,
                   dwCode
				   );

	pTask = (c_task *)c_object_mgr_get_object_by_handle(This->m_pTaskMgr, 
                                                        hIndex
                                                        );

	if (!pTask)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_set() invalid task\n" 
					   );
		return (ZOE_ERRS_PARMS);
	}

	if (util_is_equal_guid((ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL, 
					       guid
					       ))
	{

		switch (dwCode)
		{
            case ZV_CODEC_PROP_DEC_FMT:
            {
                uint32_t    *p_fmt = (uint32_t *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_format_get_clnt(p_fmt,
                                                          cpu_id,
                                                          module,
                                                          inst
                                                          );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(uint32_t) : 0;
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_MODE:
            {
                uint32_t    *p_mode = (uint32_t *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_mode_get_clnt(p_mode,
                                                        cpu_id,
                                                        module,
                                                        inst
                                                        );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(uint32_t) : 0;
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_ORDER:
            {
                uint32_t    *p_order = (uint32_t *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_order_get_clnt(p_order,
                                                         cpu_id,
                                                         module,
                                                         inst
                                                         );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(uint32_t) : 0;
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_YUV_FMT:
            {
                VPU_PICTURE *p_fmt = (VPU_PICTURE *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_yuv_format_get_clnt(p_fmt,
                                                              cpu_id,
                                                              module,
                                                              inst
                                                              );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(VPU_PICTURE) : 0;
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_MIN_FRAME_BUF:
            {
                uint32_t    *p_nb = (uint32_t *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_get_required_nb_fb_clnt(p_nb,
                                                                  cpu_id,
                                                                  module,
                                                                  inst
                                                                  );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(uint32_t) : 0;
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_VBV_SIZE:
            {
                uint32_t    *p_size = (uint32_t *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_get_required_vbv_size_clnt(p_size,
                                                                     cpu_id,
                                                                     module,
                                                                     inst
                                                                     );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(uint32_t) : 0;
                }
                break;
            }

            case ZV_CODEC_PROP_DEC_CROP:
            {
                VPU_CROP_WINDOW *p_crop = (VPU_CROP_WINDOW *)pOutput;
                err = pTask->get_fw_addr(pTask, ZV_CODEC_VID_IN, &cpu_id, &module, &inst);
                if (ZOE_SUCCESS(err))
                {
                    err = zoe_module_vdec_get_crop_clnt(p_crop,
                                                        cpu_id,
                                                        module,
                                                        inst
                                                        );
                    *pdwSizeGot = ZOE_SUCCESS(err) ? sizeof(VPU_CROP_WINDOW) : 0;
                }
                break;
            }

			default:
		        err = ZOE_ERRS_NOTIMPL;
				break;
		}
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_get() unknown property\n"
					   );
		err = ZOE_ERRS_NOTIMPL;
	}

	return (err);
}


// c_zv_codec
//

// DMA
//
zoe_errs_t c_zv_codec_dma_write(c_zv_codec *This, 
								zoe_dev_mem_t ulFwAddr,
								uint8_t * pHostAddr,
								uint32_t ulLength,
								zoe_bool_t bSwap,
								zoe_bool_t bSync,
								uint32_t ulXferMode,
                                zoe_sosal_obj_id_t evt
								)
{
	zoe_errs_t	        err;
    zoe_sosal_obj_id_t  evt_sync;


	if ((0 == ulLength) ||
		(ZOE_NULL == pHostAddr)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
					   "c_zv_codec_dma_write() Param error ulLength(%d), pHostAddr(0x%x)\n",
					   ulLength,
					   pHostAddr
					   );
		return (ZOE_ERRS_PARMS);
	}

	zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_dma_write() fw(0x%x) Host(0x%x) len(%d) swap(%d) sync(%d) xferMode(0x%x) evt(0x%x) [\n", 
				   ulFwAddr,
				   pHostAddr,
				   ulLength,
				   bSwap,
				   bSync,
				   ulXferMode,
				   evt	  
                   );
    if (bSync)
    {
        err = zoe_sosal_event_create(ZOE_NULL, 
                                     &evt_sync
                                     );
        if (!ZOE_SUCCESS(err))
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
		                   "zoe_sosal_event_create failed err(%d)!!!\n",
                           err
		                   );
            return (err);
        }
    }
    else
    {
        // clear the event 
        //
        zoe_sosal_event_clear(evt);
        evt_sync = evt;
    }

	// start DMA
	//
	err = ZOEHAL_DMA_WRITE(This->m_pHal, 
						   ulFwAddr,
                           pHostAddr,
						   ulLength,
						   ulXferMode,
						   bSwap,
						   evt_sync,
                           ZOE_NULL
						   );
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_dma_write() ZOEHAL_DMA_WRITE() Failed status(%d)\n", 
					   err
					   );
	}
    else
    {	
	    if (bSync)
	    {
		    // wait for dma complete
		    //
		    err = zoe_sosal_event_wait(evt_sync, 
								       5500000
								       );
		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_dma_write() zoe_sosal_event_wait() Failed status(%d)\n", 
							   err
							   );
			}
		}
	}

    if (bSync)
    {
        // delete sync event
        //
        zoe_sosal_event_delete(evt_sync);
    }

	zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_dma_write() fw(0x%x) Host(0x%x) len(%d) err(%d)]\n", 
				   ulFwAddr,
				   pHostAddr,
				   ulLength,
                   err
				   );

	return (err);
}



zoe_errs_t c_zv_codec_dma_read(c_zv_codec *This, 
							   zoe_dev_mem_t ulFwAddr,
							   uint8_t * pHostAddr,
							   uint32_t ulLength,
							   zoe_bool_t bSwap,
							   zoe_bool_t bSync,
							   uint32_t ulXferMode,
                               zoe_sosal_obj_id_t evt
							   )
{
	zoe_errs_t	        err;
    zoe_sosal_obj_id_t  evt_sync;

	if ((0 == ulLength) ||
		(ZOE_NULL == pHostAddr)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
					   "c_zv_codec_dma_read() Param error ulLength(%d), pHostAddr(0x%x)\n",
					   ulLength,
					   pHostAddr
					   );
		return (ZOE_ERRS_PARMS);
	}

	zoe_dbg_printf(/*ZOE_DBG_LVL_WARNING*/ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_dma_read() fw(0x%x) Host(0x%x) len(%d) swap(%d) sync(%d) xferMode(0x%x) evt(0x%x)\n", 
				   ulFwAddr,
				   pHostAddr,
				   ulLength,
				   bSwap,
				   bSync,
				   ulXferMode,
				   evt
				   );

    if (bSync)
    {
        err = zoe_sosal_event_create(ZOE_NULL, 
                                     &evt_sync
                                     );
        if (!ZOE_SUCCESS(err))
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
		                   "zoe_sosal_event_create failed err(%d)!!!\n",
                           err
		                   );
            return (err);
        }
    }
    else
    {
        // clear the event 
        //
        zoe_sosal_event_clear(evt);
        evt_sync = evt;
    }

	// start DMA
	//
	err = ZOEHAL_DMA_READ(This->m_pHal, 
						  ulFwAddr,
                          pHostAddr,
						  ulLength,
						  ulXferMode,
						  bSwap,
						  evt_sync,
                          ZOE_NULL
						  );
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_dma_read() ZOEHAL_DMA_READ() Failed status(%d)\n", 
					   err
					   );
	}
    else
    {	
	    if (bSync)
	    {
		    // wait for dma complete
		    //
		    err = zoe_sosal_event_wait(evt_sync, 
								       5500000
								       );
		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_dma_read() zoe_sosal_event_wait() Failed status(%d)\n", 
							   err
							   );
			}
        }
	}

    if (bSync)
    {
        // delete sync event
        //
        zoe_sosal_event_delete(evt_sync);
    }

	return (err);
}



// helper
//
zoe_bool_t c_zv_codec_is_codec_idle(c_zv_codec *This)
{
	zoe_bool_t	bIdle = ZOE_TRUE;
	uint32_t	num, i;

	num = c_object_mgr_get_number_of_objects(This->m_pChannelMgr);

    for (i = 0; i < num; i++)
	{
		c_task  *pTask = c_object_mgr_get_object_by_index(This->m_pTaskMgr, 
													      i
													      );
		if (pTask)
		{
		    if (TASK_STATE_IDLE != c_task_get_task_state(pTask))
		    {
			    bIdle = ZOE_FALSE;
			    break;
		    }
		}
	}
	return (bIdle);
}



zoe_bool_t c_zv_codec_is_codec_open(c_zv_codec *This)
{
	zoe_bool_t	bOpen = ZOE_FALSE;
	uint32_t	num, i;

	num = c_object_mgr_get_number_of_objects(This->m_pChannelMgr);

    for (i = 0; i < num; i++)
	{
		c_task  *pTask = c_object_mgr_get_object_by_index(This->m_pTaskMgr, 
													      i
													      );
		if (pTask)
		{
		    if (c_task_is_opened(pTask))
		    {
			    bOpen = ZOE_TRUE;
			    break;
		    }
		}
	}
	return (bOpen);
}



void c_zv_codec_clr_codec_error(c_zv_codec *This)
{
	uint32_t	num, i;

	num = c_object_mgr_get_number_of_objects(This->m_pChannelMgr);

    for (i = 0; i < num; i++)
	{
		c_task  *pTask = c_object_mgr_get_object_by_index(This->m_pTaskMgr, 
													      i
													      );
		if (pTask)
		{
            pTask->m_Error = ZOE_ERRS_SUCCESS;
		}
	}
}



zoe_bool_t c_zv_codec_is_codec_error(c_zv_codec *This)
{
	uint32_t	num, i;

	num = c_object_mgr_get_number_of_objects(This->m_pChannelMgr);

    for (i = 0; i < num; i++)
	{
		c_task  *pTask = c_object_mgr_get_object_by_index(This->m_pTaskMgr, 
													      i
													      );
		if (pTask && ZOE_FAIL(pTask->m_Error))
		{
			return (ZOE_TRUE);
		}
	}
	return (ZOE_FALSE);
}



// constructor
//
c_zv_codec * c_zv_codec_constructor(c_zv_codec *pZVCodec,
								    c_object *pParent,
								    uint32_t dwAttributes,
                                    IZOEHALAPI *pHal,
                                    zoe_dbg_comp_id_t dbgID
								    )
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   dbgID,
				   "c_zv_codec_constructor()\n" 
				   );

	if (pZVCodec)
	{
        // zero init ourselves
        //
        memset((void *)pZVCodec, 
               0, 
               sizeof(c_zv_codec)
               );

		// c_object
		//
		c_object_constructor(&pZVCodec->m_Object, 
							 pParent, 
                             OBJECT_ZOE_ZVCODEC,
							 dwAttributes
		  					 );

		// fill in the function table
		//

		// i_zv_generic_device
		//
		pZVCodec->m_iZVCodec.init_device = c_zv_codec_init_device;
		pZVCodec->m_iZVCodec.release = c_zv_codec_release;
		pZVCodec->m_iZVCodec.reset = c_zv_codec_reset;
		pZVCodec->m_iZVCodec.set = c_zv_codec_set;
		pZVCodec->m_iZVCodec.get = c_zv_codec_get;

		// i_zv_codec
		//
		pZVCodec->m_iZVCodec.alloc_task = c_zv_codec_alloc_task;
		pZVCodec->m_iZVCodec.release_task = c_zv_codec_release_task;
		pZVCodec->m_iZVCodec.open = c_zv_codec_open;
		pZVCodec->m_iZVCodec.close = c_zv_codec_close;
		pZVCodec->m_iZVCodec.start = c_zv_codec_start;
		pZVCodec->m_iZVCodec.stop = c_zv_codec_stop;
		pZVCodec->m_iZVCodec.acquire = c_zv_codec_acquire;
		pZVCodec->m_iZVCodec.pause = c_zv_codec_pause;
		pZVCodec->m_iZVCodec.set_rate = c_zv_codec_set_rate;
		pZVCodec->m_iZVCodec.get_rate = c_zv_codec_get_rate;
		pZVCodec->m_iZVCodec.begin_flush = c_zv_codec_begin_flush;
		pZVCodec->m_iZVCodec.flush = c_zv_codec_flush;
		pZVCodec->m_iZVCodec.end_flush = c_zv_codec_end_flush;
		pZVCodec->m_iZVCodec.add_buffer = c_zv_codec_add_buffer;
		pZVCodec->m_iZVCodec.cancel_buffer = c_zv_codec_cancel_buffer;
		pZVCodec->m_iZVCodec.timeout_buffer = c_zv_codec_timeout_buffer;

		pZVCodec->m_pDeviceCallback = ZOE_NULL;
		pZVCodec->m_callbackContext = ZOE_NULL;
        pZVCodec->m_pHal = pHal;
        pZVCodec->m_dbgID = dbgID;
		pZVCodec->m_PowerState = ZVCODEC_PWR_STATE_D3;
        pZVCodec->m_State = ZOE_ERRS_SUCCESS;

		// create channel manager
		//
		pZVCodec->m_pChannelMgr = (c_object_mgr *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                                       sizeof(c_object_mgr),
                                                                       sizeof(void *)
                                                                       );
		if (!c_object_mgr_constructor(pZVCodec->m_pChannelMgr,
									  &pZVCodec->m_Object,
									  OBJECT_CRITICAL_HEAVY,
									  ZOE_NULL
									  ))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
						   "c_zv_codec_constructor() c_object_mgr_constructor Failed!!!!\n"
						   );
			c_zv_codec_destructor(pZVCodec);
			return (ZOE_NULL);
		}

		// create task manager
		//
		pZVCodec->m_pTaskMgr = (c_object_mgr *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                                      sizeof(c_object_mgr),
                                                                      sizeof(void *)
                                                                      );
		if (!c_object_mgr_constructor(pZVCodec->m_pTaskMgr,
									  &pZVCodec->m_Object,
									  OBJECT_CRITICAL_HEAVY,
									  ZOE_NULL
									  ))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
						   "c_zv_codec_constructor() c_object_mgr_constructor Failed!!!!\n"
						   );
			c_zv_codec_destructor(pZVCodec);
			return (ZOE_NULL);
		}
	}

	return (pZVCodec);
}



void c_zv_codec_destructor(c_zv_codec *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_destructor()\n"
				   );

	i_zv_generic_device_release(&This->m_iZVCodec);

	// clean up
	//
	if (This->m_pChannelMgr)
	{
		c_object_mgr_destructor(This->m_pChannelMgr);
		zoe_sosal_memory_free((void *)This->m_pChannelMgr);
		This->m_pChannelMgr = ZOE_NULL;
	}

	if (This->m_pTaskMgr)
	{
		c_object_mgr_destructor(This->m_pTaskMgr);
		zoe_sosal_memory_free((void *)This->m_pTaskMgr);
		This->m_pTaskMgr = ZOE_NULL;
	}

	// c_object
	//
	c_object_destructor(&This->m_Object);
}





