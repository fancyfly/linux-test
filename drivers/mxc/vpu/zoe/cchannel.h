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
// cchannel.h
//
// Description: 
//
//	channels represent the data streaming on the codec chip
// 
// Authors: (dt) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __CCHANNEL_H__
#define __CCHANNEL_H__


#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_cobject.h"
#include "zoe_cqueue.h"
#include "zoe_hal.h"
#include "zv_avlib.h"
#include "zv_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
//

typedef enum _CHANNEL_DIRECTION
{
	CHANNEL_DIR_READ = 0,
	CHANNEL_DIR_WRITE,
	CHANNEL_DIR_NONE

} CHANNEL_DIRECTION, *PCHANNEL_DIRECTION;

#define MAX_CHANNEL_DIRECTION	CHANNEL_DIR_NONE



/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __CTASK_FWD_DEFINED__
#define __CTASK_FWD_DEFINED__
typedef struct c_task c_task;
#endif // !__CTASK_FWD_DEFINED__


#ifndef __CCHANNEL_FWD_DEFINED__
#define __CCHANNEL_FWD_DEFINED__
typedef struct c_channel c_channel;
#endif // !__CCHANNEL_FWD_DEFINED__


struct c_channel
{
	// public c_object
	//
	c_object	m_Object;

	// c_channel
	//
	zoe_errs_t (*open)(c_channel *This,
					   ZOE_OBJECT_HANDLE hChannel,
					   uint32_t dwFlags,
					   zoe_void_ptr_t pDataFormat,
					   ZV_DEVICE_CALLBACK pFuncCallback,
					   zoe_void_ptr_t context
					   );
	zoe_errs_t (*close)(c_channel *This);
	zoe_errs_t (*start)(c_channel *This);
	zoe_errs_t (*stop)(c_channel *This);
	zoe_errs_t (*acquire)(c_channel *This);
	zoe_errs_t (*pause)(c_channel *This);
	zoe_errs_t (*set_rate)(c_channel *This,
						   int32_t lNewRate
						   );
	zoe_errs_t (*get_rate)(c_channel *This,
						   int32_t * plRate
						   );
	zoe_errs_t (*begin_flush)(c_channel *This);
	zoe_errs_t (*flush)(c_channel *This);
	zoe_errs_t (*end_flush)(c_channel *This);
	zoe_errs_t (*add_buffer)(c_channel *This,
							 PZV_BUFFER_DESCRIPTOR pBufDesc
							 );
	zoe_errs_t (*cancel_buffer)(c_channel *This,
							    PZV_BUFFER_DESCRIPTOR pBufDesc, 
							    zoe_bool_t bTimeOut
							    );
	zoe_errs_t (*timeout_buffer)(c_channel *This,
								 PZV_BUFFER_DESCRIPTOR pBufDesc
								 );
	zoe_bool_t (*get_buffer)(c_channel *This,
						    PZV_BUFFER_DESCRIPTOR *ppBufDesc,
						    uint8_t * *ppBuffer,
						    uint32_t * pSize
						    );
    zoe_bool_t (*get_buffer_yuv)(c_channel *This,
							     PZV_BUFFER_DESCRIPTOR *ppBufDesc,
							     uint8_t * *ppYBuffer,
							     uint32_t * pYSize, 
							     uint8_t * *ppUVBuffer, 
							     uint32_t * pUVSize
							     );
	void (*complete_buffer)(c_channel *This,
						    PZV_BUFFER_DESCRIPTOR pBufDesc
						    );
	zoe_errs_t (*get_resolution)(c_channel *This,
								 uint32_t * pWidth,
								 uint32_t * pHeight
								 );
	zoe_errs_t (*get_yuv_format)(c_channel *This,
							     uint32_t * pYUVFormat
							     );

    IZOEHALAPI          *m_pHal;
	uint32_t		    m_dwOpenFlags;

	ZV_DEVICE_CALLBACK	m_pDeviceCallback;				// codec client call back function
	zoe_void_ptr_t		m_callbackContext;				// codec client call back function context

	ZOE_OBJECT_HANDLE	m_hTask;						// task handle this channel is attached
	ZOE_OBJECT_HANDLE	m_hChannel;						// handle to this channel
	CHANNEL_DIRECTION	m_ChannelDirection;				// channel direction
	ZV_CODEC_OPEN_TYPE  m_ChannelType;					// channel open type
	c_task				*m_pTask;

	zoe_bool_t			m_bOpened;
	ZVSTATE				m_State;						// channel state
	zoe_bool_t			m_bPaused;

	// data request management stuffs
	//
	QUEUE_ENTRY			m_Entries[ZV_AVLIB_MAX_DATA_ENTRIES];
	c_queue				*m_pFreeQueue;
	c_queue				*m_pDataRequestQueue;
	c_queue				*m_pDataPendingQueue;

	uint64_t		    m_llStartTime;

	// in flush flag
	//
	zoe_bool_t			m_bFlushing;

	// need byte swap
	//
	zoe_bool_t			m_bByteSwap;

	uint64_t		    m_ullCntBytes;

    zoe_dbg_comp_id_t   m_dbgID;

	zoe_bool_t			m_bYUV;
};



// constructor
//
c_channel * c_channel_constructor(c_channel *pChannel,
								  c_object *pParent,
								  uint32_t dwAttributes,
								  CHANNEL_DIRECTION ChannelDirection,
								  ZV_CODEC_OPEN_TYPE ChannelType,
								  c_task *pTask,
                                  IZOEHALAPI *pHal,
                                  zoe_dbg_comp_id_t dbgID
								  );
// destructor
//
void c_channel_destructor(c_channel *This);



// devcie callback
//
zoe_errs_t c_channel_device_callback(c_channel *This,
								     uint32_t dwCode,
						   		     zoe_void_ptr_t pParam
								     );

// find the buffer descriptor with matching buffer pointer
//
PZV_BUFFER_DESCRIPTOR c_channel_find_buf_desc_by_buf_ptr(c_channel *This, 
                                                         zoe_void_ptr_t p_buffer
                                                         );

// helper
//
ZVSTATE c_channel_get_state(c_channel *This);
zoe_bool_t c_channel_is_opened(c_channel *This);
zoe_bool_t c_channel_need_byte_swapping(c_channel *This);
zoe_bool_t c_channel_is_yuv(c_channel *This);



/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct c_null_channel
{
	// public c_channel
	//
	c_channel	m_Channel;
} c_null_channel;


// constructor
//
c_null_channel * c_null_channel_constructor(c_null_channel *pNullChannel, 
										    c_object *pParent,
										    c_task *pTask,
                                            IZOEHALAPI *pHal,
                                            zoe_dbg_comp_id_t dbgID
										    );

// destructor
//
void c_null_channel_destructor(c_null_channel *This);


/////////////////////////////////////////////////////////////////////////////
//
//


typedef struct c_vid_out_channel 
{
	// public c_channel
	//
	c_channel	m_Channel;
} c_vid_out_channel;


// constructor
//
c_vid_out_channel * c_vid_out_channel_constructor(c_vid_out_channel *pVidOutChannel,
											      c_object *pParent,
											      uint32_t dwAttributes,
											      c_task *pTask,
                                                  IZOEHALAPI *pHal,
                                                  zoe_dbg_comp_id_t dbgID
											      );

// destructor
//
void c_vid_out_channel_destructor(c_vid_out_channel *This);



/////////////////////////////////////////////////////////////////////////////
//
//


typedef struct c_yuv_out_channel 
{
	// public c_channel
	//
	c_channel	m_Channel;

	// c_yuv_out_channel 
	//
	uint32_t	m_dwFrameSize;

	uint32_t	m_ulVideoFramesCount;
	uint32_t	m_ulDroppedFramesCount;

	int32_t	    m_lWidth;
	int32_t	    m_lHeight;
	int32_t	    m_nBitCount;
	int32_t	    m_nDataType;

} c_yuv_out_channel;


// constructor
//
c_yuv_out_channel * c_yuv_out_channel_constructor(c_yuv_out_channel *pYUVOutChannel, 
											      c_object *pParent,
											      uint32_t dwAttributes,
											      c_task *pTask,
                                                  IZOEHALAPI *pHal,
                                                  zoe_dbg_comp_id_t dbgID
											      );

// destructor
//
void c_yuv_out_channel_destructor(c_yuv_out_channel *This);



/////////////////////////////////////////////////////////////////////////////
//
//


typedef struct c_meta_out_channel 
{
	// public c_channel
	//
	c_channel   m_Channel;
} c_meta_out_channel;


// constructor
//
c_meta_out_channel * c_meta_out_channel_constructor(c_meta_out_channel *p_meta_out_channel, 
												    c_object *pParent,
												    uint32_t dwAttributes,
												    c_task *pTask,
                                                    IZOEHALAPI *pHal,
                                                    zoe_dbg_comp_id_t dbgID
												    );

// destructor
//
void c_meta_out_channel_destructor(c_meta_out_channel *This);




/////////////////////////////////////////////////////////////////////////////
//
//


typedef struct c_yuv_in_channel 
{
	// public c_channel
	//
	c_channel	m_Channel;

	// c_yuv_in_channel 
	//
	uint32_t    m_dwFrameSize;
	uint32_t	m_ulVideoFramesCount;

	int32_t	    m_lWidth;
	int32_t	    m_lHeight;
	int32_t	    m_nBitCount;
	int32_t	    m_nDataType;

} c_yuv_in_channel;


// constructor
//
c_yuv_in_channel * c_yuv_in_channel_constructor(c_yuv_in_channel *pYUVInChannel, 
										        c_object *pParent,
										        uint32_t dwAttributes,
										        c_task *pTask,
                                                IZOEHALAPI *pHal,
                                                zoe_dbg_comp_id_t dbgID
										        );

// destructor
//
void c_yuv_in_channel_destructor(c_yuv_in_channel *This);



/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct c_vid_in_channel 
{
	// public c_channel
	//
	c_channel	m_Channel;

} c_vid_in_channel;


// constructor
//
c_vid_in_channel * c_vid_in_channel_constructor(c_vid_in_channel *pVidInChannel, 
										        c_object *pParent,
										        uint32_t dwAttributes,
										        c_task *pTask,
                                                IZOEHALAPI *pHal,
                                                zoe_dbg_comp_id_t dbgID
										        );
// destructor
//
void c_vid_in_channel_destructor(c_vid_in_channel *This);



#ifdef __cplusplus
}
#endif

#endif //__CCHANNEL_H__



