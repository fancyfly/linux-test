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
// cusbcntl.c
//
// Description: 
//
//  ZOE USB controller interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "cusbcntl.h"
#include "izvusbfw.h"
#include "zoe_sosal.h"
#include "zoe_dbg.h"
#include "zoe_objids.h"
#include "zv_codec.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD


static void UsbDmaComplete(zoe_void_ptr_t UserContext,
                           zoe_void_ptr_t pUserContext2
                           );

/////////////////////////////////////////////////////////////////////////////
//
//

static PZVUSB_IO_CONTEXT CUsbCntl_DmaRequestAlloc(CUsbCntl *This, 
                                                  uint32_t dir,
								                  zoe_dev_mem_t ulAddr,
								                  uint32_t ulLength,// in bytes
								                  uint8_t * pHostAddr,
                                                  uint32_t ulXferMode,
								                  uint32_t ulSwap,
                                                  zoe_sosal_obj_id_t evt
                                                  )
{
    PZVUSB_IO_CONTEXT   pDmaReq;

    pDmaReq = (PZVUSB_IO_CONTEXT)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                        sizeof(ZVUSB_IO_CONTEXT), 
                                                        sizeof(uint32_t)
                                                        );
    if (!pDmaReq)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_DmaRequestAlloc() pDmaReq == NULL!!!\n"
                       );
    }
    else
    {
        pDmaReq->dir = dir;
        pDmaReq->pUsb = This->m_pUsbIntf;
        pDmaReq->Pipe = (ZVUSB_DIR_OUT == dir) ? This->m_dwPipeDataWrite : This->m_dwPipeDataRead;
        pDmaReq->pCompletion = UsbDmaComplete;
        pDmaReq->UserContext = (zoe_void_ptr_t)This;
        pDmaReq->ullStartTime = 0;
        pDmaReq->dwDevAddr = ulAddr;
        pDmaReq->dwXferSize = ulLength;
        pDmaReq->pCallerBuffer = pHostAddr;
        pDmaReq->evtComplete = evt;
        pDmaReq->status = ZOE_ERRS_PENDING;
        pDmaReq->transferSize = 0;
    }

    return (pDmaReq);
}



static void CUsbCntl_DmaRequestFree(CUsbCntl *This, 
                                    PZVUSB_IO_CONTEXT pDmaReq
                                    )
{
    if (pDmaReq)
    {
        zoe_sosal_memory_free(pDmaReq);
    }
}



static zoe_errs_t CUsbCntl_DmaStartTransfer(CUsbCntl *This,
							                PZVUSB_IO_CONTEXT pDmaReq,
                                            uint32_t dir,
                                            zoe_bool_t bCritical
							                )
{
    QUEUE_ENTRY                 *pEntry;
    zoe_errs_t                  err = ZOE_ERRS_SUCCESS;
    zoe_errs_t                  errXfer;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_DIRECT)]; 
    PUSBFW_COMMAND              p_cmd;
    PUSBFW_CMD_DATA_MEM_DIRECT  p_data;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
                   "CUsbCntl_DmaStartTransfer()\n"
                   );

    if (bCritical)
    {
        zoe_sosal_mutex_get(This->m_DmaRequestLock[dir], 
                            -1
                            );
    }

    if (This->m_pDmaRequest[dir])
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                       This->m_dbgID, 
				       "CUsbCntl_DmaStartTransfer() DMA%u is still busy. request:0x%08X\n", 
				       dir,
				       This->m_pDmaRequest[dir]
				       );
        if (pDmaReq)
        {
		    c_queue_add_entry(This->m_pDmaRequestQueue[dir], 
						      &pDmaReq->ListEntry
						      );
        }
        err = ZOE_ERRS_PENDING;
        goto e_DmaStartTransfer;
    }
    else
    {
        if (ZOE_NULL != (pEntry = c_queue_get_one_entry(This->m_pDmaRequestQueue[dir])))
        {
            This->m_pDmaRequest[dir] = (PZVUSB_IO_CONTEXT)GET_INHERITED_OBJECT(ZVUSB_IO_CONTEXT, ListEntry, pEntry);
            if (pDmaReq)
            {
		        c_queue_add_entry(This->m_pDmaRequestQueue[dir], 
						          &pDmaReq->ListEntry
						          );
            }
            err = ZOE_ERRS_PENDING;
        }
        else
        {
            This->m_pDmaRequest[dir] = pDmaReq;
        }
    }

    if (This->m_pDmaRequest[dir])
    {
        p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
        p_data = (PUSBFW_CMD_DATA_MEM_DIRECT)p_cmd->cmd_data;

        p_cmd->cmd_len = sizeof(usb_cmd);
        p_cmd->cmd_cnt = This->m_cmdCnt++;
        p_cmd->cmd_type = (ZVUSB_DIR_OUT == dir) ? ZVUSB_CMD_WRITE_MEM_DIRECT : ZVUSB_CMD_READ_MEM_DIRECT;
        p_data->mem_ptr = (uint32_t)This->m_pDmaRequest[dir]->dwDevAddr;
        p_data->data_length = This->m_pDmaRequest[dir]->dwXferSize;

        errXfer = CUsbCntl_GenericCmd(This,
                                      &usb_cmd[0], 
                                      sizeof(usb_cmd),
                                      ZOE_NULL, 
                                      0,
                                      ZOE_TRUE
                                      );
        if (!ZOE_SUCCESS(errXfer))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_DmaStartTransfer() GenericCmd Failed Status(%d)\n",
                           errXfer
                           );
        }
        else
        {
            // Send buffer through USB interface
            //
            errXfer = CUsbInterface_UsbAsyncIo(This->m_pUsbIntf,
                                               This->m_pDmaRequest[dir]
                                               );
            if (!ZOE_SUCCESS(errXfer))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
                               "CUsbCntl_DmaStartTransfer() CUsbInterface_UsbAsyncIo Failed Status(%d)\n",
                               errXfer
                               );
            }
        }

        if (ZOE_FAIL(errXfer))
        {
            if (This->m_pDmaRequest[dir] != pDmaReq)
            {
				PZVUSB_IO_CONTEXT pDmaReqXfer = This->m_pDmaRequest[dir];
                This->m_pDmaRequest[dir] = ZOE_NULL;

                UsbDmaComplete((zoe_void_ptr_t)This, 
                               (zoe_void_ptr_t)pDmaReqXfer
                               );
            }
        }
    }

e_DmaStartTransfer:
    if (bCritical)
    {
        zoe_sosal_mutex_release(This->m_DmaRequestLock[dir]);
    }
	return (err);
}






/////////////////////////////////////////////////////////////////////////////
//
//

// IGenericDevice
//
static zoe_errs_t CUsbCntl_InitDevice(CUsbCntl *This, 
                                      ZV_DEVICE_CALLBACK pFuncCallback,
                                      zoe_void_ptr_t context,
                                      zoe_void_ptr_t pInitData
                                      )
{
    zoe_errs_t          err;
    PZVCODEC_INITDATA   pCodecInitData = (PZVCODEC_INITDATA)pInitData;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CUsbCntl_InitDevice()\n"
                   );

    err = CUsbInterface_StartDevice(This->m_pUsbIntf, 
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
                       "CUsbCntl_InitDevice() CUsbInterface_StartDevice() Failed! err(%d)\n",
                       err
                       );
    }

    return (err);
}



static zoe_errs_t CUsbCntl_Release(CUsbCntl *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CUsbCntl_Release()\n"
                   );
    if (c_object_is_initialized(&This->m_Object))
    {
        CUsbInterface_StopDevice(This->m_pUsbIntf);

        c_object_done(&This->m_Object);
    }
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CUsbCntl_Reset(CUsbCntl *This) 
{
    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CUsbCntl_Set(CUsbCntl *This,
                               ZOE_REFGUID guid,
                               ZOE_OBJECT_HANDLE hIndex,
                               uint32_t dwCode,
                               zoe_void_ptr_t pSettings,
                               zoe_void_ptr_t pExtra,
                               uint32_t dwSize
                               )
{
    return (ZOE_ERRS_NOTIMPL);
}



static zoe_errs_t CUsbCntl_Get(CUsbCntl *This,
                               ZOE_REFGUID guid,
                               ZOE_OBJECT_HANDLE hIndex,
                               uint32_t dwCode,
                               zoe_void_ptr_t pSettings,
                               zoe_void_ptr_t pExtra,
                               uint32_t * pdwSizeGot
                               )
{
    return (ZOE_ERRS_NOTIMPL);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// CUsbCntl
//
static zoe_errs_t CUsbCntl_AddDevice(CUsbCntl *This,
                                     zoe_void_ptr_t FunctionalDeviceObject,
                                     zoe_void_ptr_t PhysicalDeviceObject,
								     ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								     zoe_void_ptr_t pBusCallbackContext
                                     )
{
    CUsbInterface *pUsbInterface;

    // create USB bus interafce
    //
    pUsbInterface = (CUsbInterface *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                            sizeof(CUsbInterface), 
                                                            sizeof(void *)
                                                            );
    if (!pUsbInterface)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_AddDevice() pUsbInterface == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }

	This->m_pUsbIntf = CUsbInterface_Constructor(pUsbInterface, 
                                                 &This->m_Object,
                                                 OBJECT_CRITICAL_LIGHT, 
                                                 FunctionalDeviceObject, 
                                                 PhysicalDeviceObject,
								                 pBusCallbackFunc,
								                 pBusCallbackContext,
                                                 This->m_dbgID
                                                 );
    if (!This->m_pUsbIntf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbInterface_Constructor() Failed!!!\n"
                       );
        zoe_sosal_memory_free(pUsbInterface);
        return (ZOE_ERRS_NOMEMORY);
    }

    return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t CUsbCntl_RemoveDevice(CUsbCntl *This)
{
    if (This->m_pUsbIntf)
    {
        CUsbInterface_Destructor(This->m_pUsbIntf);
        zoe_sosal_memory_free(This->m_pUsbIntf);
        This->m_pUsbIntf = ZOE_NULL;
    }
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CUsbCntl_GenericCmd(CUsbCntl *This,
                               uint8_t * pOutBuf,
                               uint32_t dwOutSize,
                               uint8_t * pInBuf,
                               uint32_t dwInSize,
                               zoe_bool_t bCritical
                               )
{
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    uint32_t    dwXfered = 0;

    if (bCritical)
    {
        ENTER_CRITICAL(&This->m_Object)
    }

	if (dwOutSize)
	{
        err = CUsbInterface_UsbSendCmd(This->m_pUsbIntf,
                                       This->m_dwPipeCmdWrite,
                                       pOutBuf,
                                       dwOutSize,
                                       &dwXfered
                                       );

        if (!ZOE_SUCCESS(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_GenericCmd() write pipe(%d) Failed Status(%d) xfered(%d)\n", 
                           This->m_dwPipeCmdWrite,
                           err,
                           dwXfered
                           );
        }
    }

    if (ZOE_SUCCESS(err) && 
        dwInSize
        )
    {
        err = CUsbInterface_UsbSendCmd(This->m_pUsbIntf,
                                       This->m_dwPipeCmdRead,
                                       pInBuf,
                                       dwInSize,
                                       &dwXfered
                                       );
        if (!ZOE_SUCCESS(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_GenericCmd() read pipe(%d) Failed Status(%d) xfered(%d)\n",
                           This->m_dwPipeCmdRead,
                           err,
                           dwXfered
                           );
        }
    }

    if (bCritical)
    {
        LEAVE_CRITICAL(&This->m_Object)
    }
    return (err);
}




zoe_errs_t CUsbCntl_RegisterRead(CUsbCntl *This,
								 uint32_t dwAddr,
								 uint32_t * pData
								 )
{
    zoe_errs_t                  err;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY)]; 
    PUSBFW_COMMAND              p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;
    uint8_t                     usb_reply[sizeof(USBFW_REPLY) + sizeof(uint32_t)]; 
    PUSBFW_REPLY                p_reply = (PUSBFW_REPLY)&usb_reply[sizeof(USBFW_REPLY)];

    if (!pData)
    {
        return (ZOE_ERRS_PARMS);
    }

    p_cmd->cmd_len = sizeof(usb_cmd);
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_READ_MEM_COPY;
    p_data->mem_ptr = dwAddr;
    p_data->data_length = sizeof(uint32_t);
    p_data->data_type = 32;

	err = CUsbCntl_GenericCmd(This,
                              &usb_cmd[0],
                              p_cmd->cmd_len, 
                              &usb_reply[0],
                              sizeof(usb_reply),
                              ZOE_TRUE
                              );
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_RegisterRead() GenericCmd Failed Status(%d)\n",
                       err
                       );
	}
    else
    {
        if ((p_reply->cmd_cnt != p_cmd->cmd_cnt) ||
            (p_reply->reply_len != sizeof(usb_reply))
            )
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_RegisterRead() Inconsistency! cmd_cnt(%d:%d) len(%d:%d)\n",
                           p_reply->cmd_cnt, p_cmd->cmd_cnt,
                           p_reply->reply_len, sizeof(usb_reply)
                           );
            err = ZOE_ERRS_INVALID;
        }
        else
        {
            *pData = *((uint32_t *)p_reply->reply_data);
        }
    }
	return (err);
}



zoe_errs_t CUsbCntl_RegisterWrite(CUsbCntl *This,
								  uint32_t dwAddr,
								  uint32_t dwData 
								  )
{
    zoe_errs_t                  err;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY) + sizeof(uint32_t)]; 
    PUSBFW_COMMAND              p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;

    p_cmd->cmd_len = sizeof(usb_cmd);
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_WRITE_MEM_COPY;
    p_data->mem_ptr = dwAddr;
    p_data->data_length = sizeof(uint32_t);
    p_data->data_type = 32;
    *((uint32_t *)p_data->data) = dwData;

    err = CUsbCntl_GenericCmd(This,
                              &usb_cmd[0], 
                              sizeof(usb_cmd),
                              ZOE_NULL, 
                              0,
                              ZOE_TRUE
                              );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_RegisterWrite() GenericCmd Failed Status(%d)\n",
                       err
                       );
    }
	return (err);
}



zoe_errs_t CUsbCntl_RegisterReadEx(CUsbCntl *This,
								   uint32_t dwAddr,
								   uint32_t * pData,
								   uint32_t numReg
								   )
{
    zoe_errs_t                  err;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY)]; 
    PUSBFW_COMMAND              p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;
    uint32_t                    reply_len = sizeof(USBFW_REPLY) + (numReg * sizeof(uint32_t));
    uint8_t *                   p_reply_buf;
    PUSBFW_REPLY                p_reply;

    if (!pData)
    {
        return (ZOE_ERRS_PARMS);
    }

    p_cmd->cmd_len = sizeof(usb_cmd);
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_READ_MEM_COPY;
    p_data->mem_ptr = dwAddr;
    p_data->data_length = numReg * sizeof(uint32_t);
    p_data->data_type = 32;

    p_reply_buf = (uint8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                    reply_len, 
                                                    sizeof(uint32_t)
                                                    );
    if (!p_reply_buf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_RegisterReadEx() p_reply_buf == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }
    else
    {
        p_reply = (PUSBFW_REPLY)p_reply_buf;
    }

	err = CUsbCntl_GenericCmd(This,
                              &usb_cmd[0],
                              p_cmd->cmd_len, 
                              p_reply_buf,
                              reply_len,
                              ZOE_TRUE
                              );
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_RegisterReadEx() GenericCmd Failed Status(%d)\n",
                       err
                       );
	}
    else
    {
        if ((p_reply->cmd_cnt != p_cmd->cmd_cnt) ||
            (p_reply->reply_len != reply_len)
            )
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_RegisterReadEx() Inconsistency! cmd_cnt(%d:%d) len(%d:%d)\n",
                           p_reply->cmd_cnt, p_cmd->cmd_cnt,
                           p_reply->reply_len, reply_len
                           );
            err = ZOE_ERRS_INVALID;
        }
        else
        {
            memcpy((void *)pData,
                   (const void *)p_reply->reply_data,
                   numReg * sizeof(uint32_t)
                   );
        }
    }
    zoe_sosal_memory_free(p_reply_buf);
	return (err);
}



zoe_errs_t CUsbCntl_RegisterWriteEx(CUsbCntl *This,
								    uint32_t dwAddr,
								    uint32_t * pData,
								    uint32_t numReg
								    )
{
    zoe_errs_t                  err;
    uint32_t                    cmd_len = sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY) + (numReg * sizeof(uint32_t));
    uint8_t *                   p_cmd_buf;
    PUSBFW_COMMAND              p_cmd; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data;

    p_cmd_buf = (uint8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                  cmd_len, 
                                                  sizeof(uint32_t)
                                                  );
    if (!p_cmd_buf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_RegisterWriteEx() p_cmd_buf == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }
    else
    {
        p_cmd = (PUSBFW_COMMAND)p_cmd_buf; 
        p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;
    }

    p_cmd->cmd_len = cmd_len;
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_WRITE_MEM_COPY;
    p_data->mem_ptr = dwAddr;
    p_data->data_length = numReg * sizeof(uint32_t);
    p_data->data_type = 32;
    memcpy((void *)p_data->data,
           (const void *)pData,
           p_data->data_length
           );

    err = CUsbCntl_GenericCmd(This,
                              p_cmd_buf, 
                              cmd_len,
                              ZOE_NULL, 
                              0,
                              ZOE_TRUE
                              );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_RegisterWriteEx() GenericCmd Failed Status(%d)\n",
                       err
                       );
    }
    zoe_sosal_memory_free(p_cmd_buf);
	return (err);
}



zoe_errs_t CUsbCntl_MemoryRead(CUsbCntl *This,
							   zoe_dev_mem_t dwAddr,
							   uint32_t * pData 
							   )
{
    zoe_errs_t                  err;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY)]; 
    PUSBFW_COMMAND              p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;
    uint8_t                     usb_reply[sizeof(USBFW_REPLY) + sizeof(uint32_t)]; 
    PUSBFW_REPLY                p_reply = (PUSBFW_REPLY)&usb_reply[0];

    if (!pData)
    {
        return (ZOE_ERRS_PARMS);
    }

    p_cmd->cmd_len = sizeof(usb_cmd);
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_READ_MEM_COPY;
    p_data->mem_ptr = (uint32_t)dwAddr;
    p_data->data_length = sizeof(uint32_t);
    p_data->data_type = 32;

	err = CUsbCntl_GenericCmd(This,
                              &usb_cmd[0],
                              p_cmd->cmd_len, 
                              &usb_reply[0],
                              sizeof(usb_reply),
                              ZOE_TRUE
                              );
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_MemoryRead() GenericCmd Failed Status(%d)\n",
                       err
                       );
	}
    else
    {
        if ((p_reply->cmd_cnt != p_cmd->cmd_cnt) ||
            (p_reply->reply_len != sizeof(usb_reply))
            )
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_MemoryRead() Inconsistency! cmd_cnt(%d:%d) len(%d:%d)\n",
                           p_reply->cmd_cnt, p_cmd->cmd_cnt,
                           p_reply->reply_len, sizeof(usb_reply)
                           );
            err = ZOE_ERRS_INVALID;
        }
        else
        {
            *pData = *((uint32_t *)p_reply->reply_data);
        }
    }
	return (err);
}



zoe_errs_t CUsbCntl_MemoryWrite(CUsbCntl *This,
								zoe_dev_mem_t dwAddr,
								uint32_t dwData 
								)
{
    zoe_errs_t                  err;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY) + sizeof(uint32_t)]; 
    PUSBFW_COMMAND              p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;

    p_cmd->cmd_len = sizeof(usb_cmd);
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_WRITE_MEM_COPY;
    p_data->mem_ptr = (uint32_t)dwAddr;
    p_data->data_length = sizeof(uint32_t);
    p_data->data_type = 32;
    *((uint32_t *)p_data->data) = dwData;

    err = CUsbCntl_GenericCmd(This,
                              &usb_cmd[0], 
                              sizeof(usb_cmd),
                              ZOE_NULL, 
                              0,
                              ZOE_TRUE
                              );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_MemoryWrite() GenericCmd Failed Status(%d)\n",
                       err
                       );
    }
	return (err);
}



zoe_errs_t CUsbCntl_MemoryReadEx(CUsbCntl *This,
							     zoe_dev_mem_t dwAddr,
							     uint8_t * pData, 
							     uint32_t ulLength  // in bytes
							     )
{
    zoe_errs_t                  err;
    uint8_t                     usb_cmd[sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY)]; 
    PUSBFW_COMMAND              p_cmd = (PUSBFW_COMMAND)&usb_cmd[0]; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;
    uint32_t                    reply_len = sizeof(USBFW_REPLY) + ulLength;
    uint8_t *                   p_reply_buf;
    PUSBFW_REPLY                p_reply;

    if (!pData)
    {
        return (ZOE_ERRS_PARMS);
    }

    p_cmd->cmd_len = sizeof(usb_cmd);
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_READ_MEM_COPY;
    p_data->mem_ptr = (uint32_t)dwAddr;
    p_data->data_length = ulLength;
    p_data->data_type = 8;

    p_reply_buf = (uint8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                    reply_len, 
                                                    sizeof(uint32_t)
                                                    );
    if (!p_reply_buf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_MemoryReadEx() p_reply_buf == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }
    else
    {
        p_reply = (PUSBFW_REPLY)p_reply_buf;
    }

	err = CUsbCntl_GenericCmd(This,
                              &usb_cmd[0],
                              p_cmd->cmd_len, 
                              p_reply_buf,
                              reply_len,
                              ZOE_TRUE
                              );
	if (!ZOE_SUCCESS(err))
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_MemoryReadEx() GenericCmd Failed Status(%d)\n",
                       err
                       );
	}
    else
    {
        if ((p_reply->cmd_cnt != p_cmd->cmd_cnt) ||
            (p_reply->reply_len != reply_len)
            )
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
                           "CUsbCntl_MemoryReadEx() Inconsistency! cmd_cnt(%d:%d) len(%d:%d)\n",
                           p_reply->cmd_cnt, p_cmd->cmd_cnt,
                           p_reply->reply_len, reply_len
                           );
            err = ZOE_ERRS_INVALID;
        }
        else
        {
            memcpy((void *)pData,
                   (const void *)p_reply->reply_data,
                   ulLength
                   );
        }
    }
    zoe_sosal_memory_free(p_reply_buf);
	return (err);
}



zoe_errs_t CUsbCntl_MemoryWriteEx(CUsbCntl *This,
								  zoe_dev_mem_t dwAddr,
							      uint8_t * pData, 
							      uint32_t ulLength // in bytes
								  )
{
    zoe_errs_t                  err;
    uint32_t                    cmd_len = sizeof(USBFW_COMMAND) + sizeof(USBFW_CMD_DATA_MEM_COPY) + ulLength;
    uint8_t *                   p_cmd_buf;
    PUSBFW_COMMAND              p_cmd; 
    PUSBFW_CMD_DATA_MEM_COPY    p_data;

    p_cmd_buf = (uint8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                  cmd_len, 
                                                  sizeof(uint32_t)
                                                  );
    if (!p_cmd_buf)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_MemoryWriteEx() p_cmd_buf == NULL!!!\n"
                       );
        return (ZOE_ERRS_NOMEMORY);
    }
    else
    {
        p_cmd = (PUSBFW_COMMAND)p_cmd_buf; 
        p_data = (PUSBFW_CMD_DATA_MEM_COPY)p_cmd->cmd_data;
    }

    p_cmd->cmd_len = cmd_len;
    p_cmd->cmd_cnt = This->m_cmdCnt++;
    p_cmd->cmd_type = ZVUSB_CMD_WRITE_MEM_COPY;
    p_data->mem_ptr = (uint32_t)dwAddr;
    p_data->data_length = ulLength;
    p_data->data_type = 8;
    memcpy((void *)p_data->data,
           (const void *)pData,
           ulLength
           );

    err = CUsbCntl_GenericCmd(This,
                              p_cmd_buf, 
                              cmd_len,
                              ZOE_NULL, 
                              0,
                              ZOE_TRUE
                              );
    if (!ZOE_SUCCESS(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "CUsbCntl_MemoryWriteEx() GenericCmd Failed Status(%d)\n",
                       err
                       );
    }
    zoe_sosal_memory_free(p_cmd_buf);
	return (err);
}



static void UsbDmaComplete(zoe_void_ptr_t UserContext,
                           zoe_void_ptr_t pUserContext2
                           )
{
    CUsbCntl            *This = (CUsbCntl *)UserContext;
    PZVUSB_IO_CONTEXT   pIoContext = (PZVUSB_IO_CONTEXT)pUserContext2;
    uint32_t            dir = pIoContext->dir;
    uint32_t            cmd = (ZVUSB_DIR_IN == dir) ? ZV_BUSINTF_DMA_DONE_READ : ZV_BUSINTF_DMA_DONE_WRITE;
    uint32_t            size = pIoContext->transferSize;
    zoe_errs_t          status = pIoContext->status;
    zoe_sosal_obj_id_t  evt = pIoContext->evtComplete;


	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                   This->m_dbgID,
                   "UsbDmaComplete() %s status(%d) req(%d) xfer(%d)\n",
                   (ZVUSB_DIR_IN == pIoContext->dir) ? "IN" : "OUT",
                   pIoContext->status,
                   pIoContext->dwXferSize,
                   pIoContext->transferSize
                   );

    zoe_sosal_mutex_get(This->m_DmaRequestLock[dir], 
                        -1
                        );

    // clear current DMA
    //
    This->m_pDmaRequest[dir] = ZOE_NULL;

    // free this io context
    //
    CUsbCntl_DmaRequestFree(This, 
                            pIoContext
                            );

    // start next request
    //
    CUsbCntl_DmaStartTransfer(This, 
                              ZOE_NULL, 
                              dir, 
                              ZOE_FALSE
                              );

    zoe_sosal_mutex_release(This->m_DmaRequestLock[dir]);

    // call back client
    //
    This->m_pBusCallbackFunc(This->m_pBusCallbackContext, 
                             cmd,
							 (zoe_void_ptr_t)evt,
                             (uint32_t)status,
                             size
							 );
}



zoe_errs_t CUsbCntl_StartDMAWrite(CUsbCntl *This,
								  zoe_dev_mem_t ulAddr,
								  uint32_t ulLength,// in bytes
								  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
								  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt
								  )
{
    PZVUSB_IO_CONTEXT   pDmaReq;
    zoe_errs_t          err;

    // allocate DMA request
    //
    pDmaReq = CUsbCntl_DmaRequestAlloc(This, 
                                       ZVUSB_DIR_OUT, 
                                       ulAddr, 
                                       ulLength, 
                                       pHostAddr, 
                                       ulXferMode, 
                                       ulSwap, 
                                       evt
                                       );
    // start DMA transfer
    //
    if (pDmaReq)
    {
        err = CUsbCntl_DmaStartTransfer(This, 
                                        pDmaReq, 
                                        ZVUSB_DIR_OUT, 
                                        ZOE_TRUE
                                        );
        if (ZOE_FAIL(err))
        {
            CUsbCntl_DmaRequestFree(This, 
                                    pDmaReq
                                    );
        }
    }
    else
    {
        err = ZOE_ERRS_NOMEMORY;
    }

    return (err);
}



zoe_errs_t CUsbCntl_StartDMARead(CUsbCntl *This,
								 zoe_dev_mem_t ulAddr,
								 uint32_t ulLength, // in bytes
								 uint8_t * pHostAddr,
                                 uint32_t ulXferMode,
								 uint32_t ulSwap,
                                 zoe_sosal_obj_id_t evt
								 )
{
    PZVUSB_IO_CONTEXT   pDmaReq;
    zoe_errs_t          err;

    // allocate DMA request
    //
    pDmaReq = CUsbCntl_DmaRequestAlloc(This, 
                                       ZVUSB_DIR_IN, 
                                       ulAddr, 
                                       ulLength, 
                                       pHostAddr, 
                                       ulXferMode, 
                                       ulSwap, 
                                       evt
                                       );
    // start DMA transfer
    //
    if (pDmaReq)
    {
        err = CUsbCntl_DmaStartTransfer(This, 
                                        pDmaReq, 
                                        ZVUSB_DIR_IN, 
                                        ZOE_TRUE
                                        );
        if (ZOE_FAIL(err))
        {
            CUsbCntl_DmaRequestFree(This, 
                                    pDmaReq
                                    );
        }
    }
    else
    {
        err = ZOE_ERRS_NOMEMORY;
    }

    return (err);
}



CUsbCntl * CUsbCntl_Constructor(CUsbCntl *pUsbCntl, 
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
				   "CUsbCntl_Constructor()\n"
				   );

	if (pUsbCntl)
	{
        zoe_errs_t      err;
        uint32_t        i;
        c_queue         *pQueue;

        // zero init 
        //
		memset((void *)pUsbCntl,
			   0,
			   sizeof(CUsbCntl)
			   );

		// c_object
		//
		c_object_constructor(&pUsbCntl->m_Object, 
							 pParent, 
                             OBJECT_ZOE_USB_CNTL,
							 dwAttributes
		  					 );

        // i_zv_generic_device
        //
        pUsbCntl->init_device = CUsbCntl_InitDevice;
        pUsbCntl->release = CUsbCntl_Release;
        pUsbCntl->reset = CUsbCntl_Reset;
        pUsbCntl->set = CUsbCntl_Set;
        pUsbCntl->get = CUsbCntl_Get;

        // CUsbCntl
        //
        pUsbCntl->m_pUsbIntf = ZOE_NULL;

        pUsbCntl->m_dwPipeCmdWrite = ZVUSB_EP_CMD_OUT;
        pUsbCntl->m_dwPipeCmdRead = ZVUSB_EP_CMD_RESP;
        pUsbCntl->m_dwPipeDataWrite = ZVUSB_EP_DATA_OUT;
        pUsbCntl->m_dwPipeDataRead = ZVUSB_EP_DATA_IN;
        pUsbCntl->m_dwPipeIntr = ZVUSB_EP_UNBLOCK;

        pUsbCntl->m_pBusCallbackFunc = pBusCallbackFunc;
        pUsbCntl->m_pBusCallbackContext = pBusCallbackContext;

        pUsbCntl->m_cmdCnt = 0;
        pUsbCntl->m_dbgID = dbgID;

	    // create USB bus interafce
	    //
        err = CUsbCntl_AddDevice(pUsbCntl,
                                 pFunctionalDeviceObject,
                                 pPhysicalDeviceObject,
                                 pBusCallbackFunc,
                                 pBusCallbackContext
                                 );
        if (!ZOE_SUCCESS(err)) 
		{
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
                           "CUsbCntl_AddDevice() Failed! err(%d)\n",
                           err
                           );
            CUsbCntl_Destructor(pUsbCntl);
            return (ZOE_NULL);
		}

        for (i = 0; i < ZVUSB_DIR_MAX; i++)
        {
            // init current DMA request
            //
            pUsbCntl->m_pDmaRequest[i] = ZOE_NULL;

            // init DMA request lock
            //
		    zoe_sosal_mutex_create(ZOE_NULL, 
                                   &pUsbCntl->m_DmaRequestLock[i]
                                   );

		    // init DMA request queue
		    //
            pQueue = (c_queue *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                                      sizeof(c_queue), 
                                                      sizeof(void *)
                                                      );
			if (!pQueue)
			{
				CUsbCntl_Destructor(pUsbCntl);
				return (ZOE_NULL);
			}

			pUsbCntl->m_pDmaRequestQueue[i] = c_queue_constructor(pQueue,
														          &pUsbCntl->m_Object, 
														          OBJECT_CRITICAL_LIGHT,
                                                                  ZOE_NULL,
                                                                  ZOE_NULL,
                                                                  dbgID
														          );
			if (!pUsbCntl->m_pDmaRequestQueue[i])
			{
			    zoe_sosal_memory_free((void *)pQueue);
				CUsbCntl_Destructor(pUsbCntl);
				return (ZOE_NULL);
			}
        }
    }

	return (pUsbCntl);
}



void CUsbCntl_Destructor(CUsbCntl *This)
{
    uint32_t            i;
    QUEUE_ENTRY         *pEntry;
    PZVUSB_IO_CONTEXT   pDmaReq;
    uint32_t            dwPipeNum;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CUsbCntl_Destructor()\n"
                   );

    for (i = 0; i < ZVUSB_DIR_MAX; i++)
    {
        if (This->m_pDmaRequest[i])
        {
            dwPipeNum = (ZVUSB_DIR_OUT == i) ? This->m_dwPipeDataWrite : This->m_dwPipeDataRead;

            CUsbInterface_UsbAbortPipe(This->m_pUsbIntf, 
                                       dwPipeNum
                                       );
            CUsbCntl_DmaRequestFree(This, 
                                    This->m_pDmaRequest[i]
                                    );
            This->m_pDmaRequest[i] = ZOE_NULL;
        }

        if (This->m_pDmaRequestQueue[i])
        {
	        while (ZOE_NULL != (pEntry = c_queue_get_one_entry(This->m_pDmaRequestQueue[i])))
	        {
                pDmaReq = (PZVUSB_IO_CONTEXT)GET_INHERITED_OBJECT(ZVUSB_IO_CONTEXT, ListEntry, pEntry);
                CUsbCntl_DmaRequestFree(This, 
                                        pDmaReq
                                        );
	        }

	        c_queue_destructor(This->m_pDmaRequestQueue[i]);
	        zoe_sosal_memory_free((void *)This->m_pDmaRequestQueue[i]);
	        This->m_pDmaRequestQueue[i] = ZOE_NULL;
        }
    }

    // delete USB bus interface
    //
    CUsbCntl_RemoveDevice(This);

    // c_object
    //
    c_object_destructor(&This->m_Object);
}



