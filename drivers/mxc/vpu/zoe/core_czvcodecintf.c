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
// czvcodecintf.h
//
// Description: 
//
//  This is the implementation of i_zv_codec interface of the VPU codec
// 
// Authors: (dt) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zv_codec.h"
#include "cchannel.h"
#include "czvcodec.h"
#include "czvavlib.h"
#include "ctask.h"
#include "zoe_dbg.h"
#include "zoe_util.h"



// i_zv_codec
//
zoe_errs_t c_zv_codec_release_task(i_zv_codec *This_p, 
								   ZOE_OBJECT_HANDLE hTask
								   )
{
	zoe_errs_t	err;
    c_task      *pTask;
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);

	// find and remove the task from the task manager
	//
	pTask = (c_task *)c_object_mgr_remove_object(This->m_pTaskMgr, 
											    hTask
											    );
	if (pTask)
	{
	    err = pTask->release(pTask);
        c_task_destructor(pTask);
        zoe_sosal_memory_free((void *)pTask);
    }
    else
    {
        err = ZOE_ERRS_NOTFOUND;
    }

    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_release_task() err(%d) hTask(%d)\n",
				   err,
				   hTask
				   );
	return (err);
}



zoe_errs_t c_zv_codec_alloc_task(i_zv_codec *This_p, 
                                 ZV_CODEC_TASK_TYPE type,
						         PZOE_OBJECT_HANDLE phTask
						         )
{
	zoe_errs_t	        err;
	c_zv_codec	        *This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
    c_zv_av_lib         *p_avlib = GET_INHERITED_OBJECT(c_zv_av_lib, m_Object, This->m_Object.m_pParent);
    c_task              *pTask;
    ZOE_OBJECT_HANDLE   hTask = ZOE_NULL_HANDLE;
    uint32_t            t_priority = zoe_sosal_thread_maxpriorities_get();

	if (ZOE_NULL == phTask)
	{
		return (ZOE_ERRS_PARMS);
	}

	// downloading firmware
	//
	ENTER_CRITICAL(&This->m_Object)
    if (!This->m_fw_loaded)
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
				       "%s() c_zv_av_lib_firmware_download...\n",
                       __FUNCTION__
				       );
        err = c_zv_av_lib_firmware_download(p_avlib);
        if (ZOE_FAIL(err))
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
					       "%s() c_zv_av_lib_firmware_download Failed(%d)!!!!\n",
                           __FUNCTION__,
                           err
					       );
            return (err);
        }
        else
        {
            This->m_fw_loaded = ZOE_TRUE;
        }
    }
	LEAVE_CRITICAL(&This->m_Object)

    // allocate and init c_task
    //
	pTask = (c_task *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL, 
                                             sizeof(c_task),
                                             0
                                             );
	if (!c_task_constructor(pTask,
						    &This->m_Object,
						    OBJECT_CRITICAL_LIGHT,
                            t_priority - 1,
                            type,
                            This,
                            This->m_pHal,
                            This->m_dbgID
						    ))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
					   "c_zv_codec_alloc_task() c_task_constructor Failed!!!!\n"
					   );
        if (pTask)
        {
            zoe_sosal_memory_free((void *)pTask);
        }
		return (ZOE_ERRS_NOMEMORY);
	}

	// add the task to the object manager
	//
	hTask = c_object_mgr_add_object(This->m_pTaskMgr, 
								    pTask
								    );
    // call task alloc
    //
	err = pTask->alloc(pTask, 
					   hTask
					   );
	if (ZOE_SUCCESS(err))
	{
        zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       This->m_dbgID,
				       "c_zv_codec_alloc_task() hTask(%d)\n",
				       hTask
				       );
		*phTask = hTask;
	}
	else
	{
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
				       "c_zv_codec_alloc_task() err(%d)!!\n",
				       err
				       );
		c_zv_codec_release_task(This_p, 
					            hTask
					            );
	}
	return (err);
}



zoe_errs_t c_zv_codec_close(i_zv_codec *This_p, 
						    ZOE_OBJECT_HANDLE hStream
						    )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;
	zoe_errs_t	err = ZOE_ERRS_PARMS;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_close() hStream(%d)\n",
				   hStream
				   );

	// find and remove the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_remove_object(This->m_pChannelMgr, 
												       hStream
												       );

	if (pChannel)
	{
		// close the channel
		//
		err = pChannel->close(pChannel);

		// call the proper destructor and free the memory
		//
		switch (pChannel->m_ChannelType)
		{
			case ZV_CODEC_VID_OUT:
			{
				c_vid_out_channel   *pVidOutChannel = GET_INHERITED_OBJECT(c_vid_out_channel, m_Channel, pChannel);
				c_vid_out_channel_destructor(pVidOutChannel);
				zoe_sosal_memory_free((void *)pVidOutChannel);
				break;
			}

			case ZV_CODEC_META_OUT:
			{
				c_meta_out_channel	*p_meta_out_channel = GET_INHERITED_OBJECT(c_meta_out_channel, m_Channel, pChannel);
				c_meta_out_channel_destructor(p_meta_out_channel);
				zoe_sosal_memory_free((void *)p_meta_out_channel);
				break;
			}

			case ZV_CODEC_YUV_OUT:
			{
				c_yuv_out_channel   *pYuvOutChannel = GET_INHERITED_OBJECT(c_yuv_out_channel, m_Channel, pChannel);
				c_yuv_out_channel_destructor(pYuvOutChannel);
				zoe_sosal_memory_free((void *)pYuvOutChannel);
				break;
			}

			case ZV_CODEC_VID_IN:
			{
				c_vid_in_channel	*pVidInChannel = GET_INHERITED_OBJECT(c_vid_in_channel, m_Channel, pChannel);
				c_vid_in_channel_destructor(pVidInChannel);
				zoe_sosal_memory_free((void *)pVidInChannel);
				break;
			}

			case ZV_CODEC_YUV_IN:
			{
				c_yuv_in_channel    *pYuvInChannel = GET_INHERITED_OBJECT(c_yuv_in_channel, m_Channel, pChannel);
				c_yuv_in_channel_destructor(pYuvInChannel);
				zoe_sosal_memory_free((void *)pYuvInChannel);
				break;
			}

			case ZV_CODEC_VIRTUAL:
			{
				c_null_channel	*pNullChannel = GET_INHERITED_OBJECT(c_null_channel, m_Channel, pChannel);
				c_null_channel_destructor(pNullChannel);
				zoe_sosal_memory_free((void *)pNullChannel);
				break;
			}

			case ZV_CODEC_META_IN:
			default:
				zoe_sosal_memory_free((void *)pChannel);
				break;
		}
	}

	return (err);
}



zoe_errs_t c_zv_codec_open(i_zv_codec *This_p, 
						   ZOE_OBJECT_HANDLE hTask,
				           uint32_t dwFlags,  // open type
				           zoe_void_ptr_t pDataFormat,
				           PZOE_OBJECT_HANDLE phStream,
				           ZV_DEVICE_CALLBACK pFuncCallback,
				           zoe_void_ptr_t context
						   )
{
	ZOE_OBJECT_HANDLE	hChannel = ZOE_NULL_HANDLE;
	c_channel			*pChannel = ZOE_NULL;
    c_task              *pTask;
	zoe_errs_t			err;
	c_zv_codec			*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_open() hTask(%d) dwFlags(%d)\n",
				   hTask,
				   dwFlags
				   );

	if (ZOE_NULL == phStream)
	{
		return (ZOE_ERRS_PARMS);
	}

	// find the task from the task manager
	//
	pTask = (c_task *)c_object_mgr_get_object_by_handle(This->m_pTaskMgr, 
											            hTask
											            );
	if (!pTask)
	{
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
				       "pTask(%d) not found!!\n",
				       hTask
				       );
		return (ZOE_ERRS_PARMS);
	}

	// allocate channel
	//
	switch (dwFlags)
	{
		case ZV_CODEC_VID_OUT:
		{
			c_vid_out_channel   *pVidOutChannel;

			// create vid out channel
			//
			pVidOutChannel = (c_vid_out_channel *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                         sizeof(c_vid_out_channel),
                                                                         sizeof(void *)
                                                                         );
			if (!c_vid_out_channel_constructor(pVidOutChannel, 
											   &This->m_Object,
							                   OBJECT_CRITICAL_LIGHT,
											   pTask,
											   This->m_pHal,
											   This->m_dbgID
											   ))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_open() Failed can not create pVidOutChannel!!!!\n"
							   );
				if (pVidOutChannel)
				{
					zoe_sosal_memory_free((void *)pVidOutChannel);
				}
				return (ZOE_ERRS_NOMEMORY);
			}

			pChannel = &pVidOutChannel->m_Channel;
			break;
		}

		case ZV_CODEC_META_OUT:
		{
			c_meta_out_channel	*p_meta_out_channel;

			// create index out channels
			//
			p_meta_out_channel = (c_meta_out_channel *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                              sizeof(c_meta_out_channel),
                                                                              sizeof(void *)
                                                                              );
			if (!c_meta_out_channel_constructor(p_meta_out_channel, 
											    &This->m_Object,
											    OBJECT_CRITICAL_LIGHT,
											    pTask,
											    This->m_pHal,
											    This->m_dbgID
											    ))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_open() Failed can not create p_meta_out_channel!!!!\n"
							   );
				if (p_meta_out_channel)
				{
					zoe_sosal_memory_free((void *)p_meta_out_channel);
				}
				return (ZOE_ERRS_NOMEMORY);
			}

			pChannel = &p_meta_out_channel->m_Channel;
			break;
		}

		case ZV_CODEC_YUV_OUT:
		{
			c_yuv_out_channel	*pYuvOutChannel;

			// create yuv out channel
			//
			pYuvOutChannel = (c_yuv_out_channel *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                         sizeof(c_yuv_out_channel),
                                                                         sizeof(void *)
                                                                         );
			if (!c_yuv_out_channel_constructor(pYuvOutChannel, 
											   &This->m_Object,
											   OBJECT_CRITICAL_LIGHT,
											   pTask,
											   This->m_pHal,
											   This->m_dbgID
											   ))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_open() Failed can not create pYuvOutChannel!!!!\n"
							   );
				if (pYuvOutChannel)
				{
					zoe_sosal_memory_free((void *)pYuvOutChannel);
				}
				return (ZOE_ERRS_NOMEMORY);
			}

			pChannel = &pYuvOutChannel->m_Channel;
			break;
		}

		case ZV_CODEC_VID_IN:
		{
			c_vid_in_channel    *pVidInChannel;

			// create vid in channel
			//
			pVidInChannel = (c_vid_in_channel *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                       sizeof(c_vid_in_channel),
                                                                       sizeof(void *)
                                                                       );
			if (!c_vid_in_channel_constructor(pVidInChannel, 
										      &This->m_Object,
										      OBJECT_CRITICAL_LIGHT,
										      pTask,
										      This->m_pHal,
										      This->m_dbgID
										      ))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_open() Failed can not create pMpgInChannel!!!!\n"
							   );
				if (pVidInChannel)
				{
					zoe_sosal_memory_free((void *)pVidInChannel);
				}
				return (ZOE_ERRS_NOMEMORY);
			}

			pChannel = &pVidInChannel->m_Channel;
			break;
		}

		case ZV_CODEC_YUV_IN:
		{
			c_yuv_in_channel    *pYuvInChannel;

			// create yuv in channel
			//
			pYuvInChannel = (c_yuv_in_channel *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                       sizeof(c_yuv_in_channel),
                                                                       sizeof(void *)
                                                                       );
			if (!c_yuv_in_channel_constructor(pYuvInChannel, 
										      &This->m_Object,
										      OBJECT_CRITICAL_LIGHT,
										      pTask,
										      This->m_pHal,
										      This->m_dbgID
										      ))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_open() Failed can not create pYuvInChannel!!!!\n"
							   );
				if (pYuvInChannel)
				{
					zoe_sosal_memory_free((void *)pYuvInChannel);
				}
				return (ZOE_ERRS_NOMEMORY);
			}

			pChannel = &pYuvInChannel->m_Channel;
			break;
		}

		case ZV_CODEC_VIRTUAL:
		{
			c_null_channel  *pNullChannel;

			// create null channel
			//
			pNullChannel = (c_null_channel *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                    sizeof(c_null_channel),
                                                                    sizeof(void *)
                                                                    );
			if (!c_null_channel_constructor(pNullChannel, 
										    &This->m_Object,
										    pTask,
										    This->m_pHal,
										    This->m_dbgID
										    ))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
							   "c_zv_codec_open() Failed can not create pNullChannel!!!!\n"
							   );
				if (pNullChannel)
				{
					zoe_sosal_memory_free((void *)pNullChannel);
				}
				return (ZOE_ERRS_NOMEMORY);
			}

			pChannel = &pNullChannel->m_Channel;
			break;
		}

		case ZV_CODEC_META_IN:
		default:
			return (ZOE_ERRS_PARMS);
	}

	// add the channel to the object manager
	//
	hChannel = c_object_mgr_add_object(This->m_pChannelMgr, 
									   pChannel
									   );

	// actually open the channel
	//
	err = pChannel->open(pChannel, 
						 hChannel,
						 dwFlags,
						 pDataFormat,
						 pFuncCallback,
						 context
						 );
	if (ZOE_SUCCESS(err))
	{
		// return handle
		//
		*phStream = hChannel;
	}
	else
	{
		c_zv_codec_close(This_p, 
					     hChannel
					     );
	}

	return (err);
}





zoe_errs_t c_zv_codec_start(i_zv_codec *This_p, 
						    ZOE_OBJECT_HANDLE hStream
						    )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_start() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														     hStream
														     );

	if (pChannel)
	{
		return (pChannel->start(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_stop(i_zv_codec *This_p, 
						   ZOE_OBJECT_HANDLE hStream
						   )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_stop() hStream(%d)\n",
				   hStream
				   );
	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->stop(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_acquire(i_zv_codec *This_p, 
							  ZOE_OBJECT_HANDLE hStream
							  )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_acquire() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->acquire(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_pause(i_zv_codec *This_p, 
						    ZOE_OBJECT_HANDLE hStream
						    )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_pause() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->pause(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_set_rate(i_zv_codec *This_p, 
							   ZOE_OBJECT_HANDLE hStream,
							   int32_t lNewRate
							   )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_set_rate() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->set_rate(pChannel, 
								   lNewRate
								   ));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_get_rate(i_zv_codec *This_p, 
							   ZOE_OBJECT_HANDLE hStream,
							   int32_t * plRate
							   )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_get_rate() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->get_rate(pChannel, 
								   plRate
								   ));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_begin_flush(i_zv_codec *This_p, 
							      ZOE_OBJECT_HANDLE hStream
							      )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_begin_flush() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->begin_flush(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_flush(i_zv_codec *This_p, 
						    ZOE_OBJECT_HANDLE hStream
						    )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_flush() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->flush(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_end_flush(i_zv_codec *This_p, 
							    ZOE_OBJECT_HANDLE hStream
							    )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_zv_codec_end_flush() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->end_flush(pChannel));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_add_buffer(i_zv_codec *This_p, 
							     ZOE_OBJECT_HANDLE hStream,
							     PZV_BUFFER_DESCRIPTOR pBufDesc										 
							     )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                   This->m_dbgID,
				   "c_zv_codec_add_buffer() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->add_buffer(pChannel, 
									 pBufDesc
									 ));
	}

	return (ZOE_ERRS_PARMS);
}



zoe_errs_t c_zv_codec_cancel_buffer(i_zv_codec *This_p, 
								    ZOE_OBJECT_HANDLE hStream,
								    PZV_BUFFER_DESCRIPTOR pBufDesc
								    )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                   This->m_dbgID,
				   "c_zv_codec_cancel_buffer() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->cancel_buffer(pChannel, 
									    pBufDesc,
									    ZOE_FALSE
									    ));
	}

	return (ZOE_ERRS_NOTFOUND);
}



zoe_errs_t c_zv_codec_timeout_buffer(i_zv_codec *This_p, 
								     ZOE_OBJECT_HANDLE hStream,
								     PZV_BUFFER_DESCRIPTOR pBufDesc
								     )
{
	c_zv_codec	*This = GET_INHERITED_OBJECT(c_zv_codec, m_iZVCodec, This_p);
	c_channel	*pChannel = ZOE_NULL;

	zoe_dbg_printf(ZOE_DBG_LVL_XTRACE,
                   This->m_dbgID,
				   "c_zv_codec_timeout_buffer() hStream(%d)\n",
				   hStream
				   );

	// find the channel from the channel manager
	//
	pChannel = (c_channel *)c_object_mgr_get_object_by_handle(This->m_pChannelMgr, 
														      hStream
														      );

	if (pChannel)
	{
		return (pChannel->timeout_buffer(pChannel, 
										 pBufDesc
										 ));
	}

	return (ZOE_ERRS_NOTFOUND);
}





