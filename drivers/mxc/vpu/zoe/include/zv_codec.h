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
// zv_codec.h
//
// Description: 
//
//   This is the definition of the zv codec device
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZV_CODEC_H__
#define __ZV_CODEC_H__


#include "zoe_types.h"
#include "zoe_hal.h"
#include "zv_genericdevice.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)

/////////////////////////////////////////////////////////////////////////////
//
//

typedef enum _ZV_CODEC_TASK_TYPE
{
	TASK_TYPE_CODEC = 0,
	TASK_TYPE_DECODER,
	TASK_TYPE_ENCODER,
	TASK_TYPE_INVALID = -1
} ZV_CODEC_TASK_TYPE, *PZV_CODEC_TASK_TYPE;


typedef enum _ZV_CODEC_OPEN_TYPE
{
	ZV_CODEC_VID_OUT = 0,
	ZV_CODEC_META_OUT,
	ZV_CODEC_YUV_OUT,
	ZV_CODEC_VID_IN,
	ZV_CODEC_META_IN,
	ZV_CODEC_YUV_IN,
	ZV_CODEC_VIRTUAL,
	ZV_CODEC_OPEN_END,
	ZV_CODEC_OPEN_INVALID = -1
} ZV_CODEC_OPEN_TYPE, *PZV_CODEC_OPEN_TYPE;


#define ZV_CODEC_OUT_START  ZV_CODEC_VID_OUT
#define ZV_CODEC_OUT_END    ZV_CODEC_YUV_OUT
#define ZV_CODEC_IN_START   ZV_CODEC_VID_IN
#define ZV_CODEC_IN_END     ZV_CODEC_YUV_IN
#define ZV_CODEC_OUT_NUM    (ZV_CODEC_OUT_END - ZV_CODEC_OUT_START + 1)
#define ZV_CODEC_IN_NUM     (ZV_CODEC_IN_END - ZV_CODEC_IN_START + 1)
#define ZV_CODEC_DATA_NUM   (ZV_CODEC_OPEN_END - 1) // minus virtual



// buffer descriptor flags
//
#define ZV_BUFDESC_FLAG_DATADISCONTINUITY	        0x00000010
#define ZV_BUFDESC_FLAG_TIMEDISCONTINUITY	        0x00000020
#define ZV_BUFDESC_FLAG_TYPECHANGED			        0x00000040
#define ZV_BUFDESC_FLAG_PARTIAL_FILL_OK				0x00010000
#define ZV_BUFDESC_FLAG_EOS							0x00020000
#define ZV_BUFDESC_FLAG_PTS							0x00040000
#define ZV_BUFDESC_FLAG_DTS							0x00080000
#define ZV_BUFDESC_FLAG_FRAME_ALIGNED				0x00100000
#define ZV_BUFDESC_FLAG_KERNEL_MAPPED			    0x00200000
#define ZV_BUFDESC_FLAG_BUFFER_MASK					DMA_BUFFER_MODE_MASK    //0xFF000000


typedef enum _ZVSTATE
{
	ZVSTATE_STOP,
	ZVSTATE_ACQUIRE,
	ZVSTATE_PAUSE,
	ZVSTATE_RUN
} ZVSTATE, *PZVSTATE;


typedef struct _ZV_BUFFER_DATA
{
    int64_t	        Duration;
    uint32_t	    BufferSize;		    // size of the entire buffer
    uint32_t	    DataUsed;			// actual size used
    uint32_t        DataOffset;         // data offset
    zoe_void_ptr_t	Data;				// data buffer
    zoe_void_ptr_t	Data_mapped;	    // data buffer
    zoe_void_ptr_t  p_user_mappings;    // user pages, linux only
} ZV_BUFFER_DATA, *PZV_BUFFER_DATA;


#define ZV_MAX_BUF_PER_DESC     3

// buffer descriptor format
//
typedef struct _ZV_BUFFER_DESCRIPTOR
{
	ZV_BUFFER_DATA	DataBuffer[ZV_MAX_BUF_PER_DESC];

	uint32_t		NumberOfBuffers;
	uint32_t		ulBufferIndex;
	uint32_t		ulBufferOffset;
	uint32_t		ulBufferSize;

	uint32_t		ulTotalUsed;
	uint32_t		ulFlags;
	zoe_errs_t		Status;

	int64_t			ulPTS;		// 64 bit PTS
	int64_t			ulDTS;		// 64 bit DTS

	// additional information for raw YUV
	//
    uint32_t		dwFrameFlags;
    int64_t			PictureNumber;
    int64_t			DropCount;

} ZV_BUFFER_DESCRIPTOR, *PZV_BUFFER_DESCRIPTOR;


// dwFrameFlags
//
#define ZV_VIDEO_FLAG_FRAME		0x0000L	// Frame or Field
#define ZV_VIDEO_FLAG_FIELD1	0x0001L
#define ZV_VIDEO_FLAG_FIELD2	0x0002L


// open formats
//
typedef struct _ZV_YUV_DATAFORMAT
{
	int32_t	        nWidth;
	int32_t	        nHeight;
	int32_t	        nBitCount;
	uint32_t	    nFrameRate;

	uint32_t	    nDataType;

} ZV_YUV_DATAFORMAT, *PZV_YUV_DATAFORMAT;

#define ZV_YUV_DATA_TYPE_NV12   1
#define ZV_YUV_DATA_TYPE_NV21   2


/////////////////////////////////////////////////////////////////////////////
//
//

// device specific initialization data
//

// bus data
//


// init data
//
typedef struct _ZVCODEC_INITDATA
{
	// flag to control if we need to init the chip
	//
	zoe_bool_t			    bDontInitHW;

	// bus type
	//
	ZOEHAL_BUS			    BusType;

    // device instance
    //
    uint32_t                Instance;

	// memory size
	//
	uint32_t			    MemSize;	// in units of Mb

	// chip type
	//
	ZOEHAL_CHIP_ID	        ChipType;

	// video input configuration
	//

	// audio input configuration
	//

	// video output configuration
	//

	// audio output configuration
	//

	// bus specific data
	//
	zoe_void_ptr_t			BusData;
	uint32_t			    BusDataSize;

} ZVCODEC_INITDATA, *PZVCODEC_INITDATA;


/////////////////////////////////////////////////////////////////////////////
//
//

// property supported by this interface
//



/////////////////////////////////////////////////////////////////////////////
//
//

// codec cllback command code
//
#define ZVCODEC_CMD_STOP_COMPLETED			0x00000010
#define ZVCODEC_CMD_DONE_DATA				0x00000020
#define ZVCODEC_CMD_END_OF_STREAM			0x00000100
#define ZVCODEC_CMD_START_TIMEOUT			0x00000200
#define ZVCODEC_CMD_DEC_VID_ERR				0x00001000
#define ZVCODEC_CMD_DEC_SEQ_PARAM_CHANGED	0x00002000
#define ZVCODEC_CMD_DEC_COMPLETE			0x00004000


/////////////////////////////////////////////////////////////////////////////
//
//

// codec cllback command data
//



/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __IZV_CODEC_FWD_DEFINED__
#define __IZV_CODEC_FWD_DEFINED__
typedef struct i_zv_codec i_zv_codec;
#endif //__IZV_CODEC_FWD_DEFINED__

    struct i_zv_codec
    {
		// i_zv_generic_device
		DECLARE_IZVGENERICDEVICE(i_zv_codec)

		// i_zv_codec
		zoe_errs_t (*alloc_task)(i_zv_codec *This, 
                                 ZV_CODEC_TASK_TYPE type,
								 PZOE_OBJECT_HANDLE phTask
								 );
		zoe_errs_t (*release_task)(i_zv_codec *This, 
								  ZOE_OBJECT_HANDLE hTask
								  );
		zoe_errs_t (*open)(i_zv_codec *This, 
						   ZOE_OBJECT_HANDLE hTask,
						   uint32_t dwFlags,	// open type
						   zoe_void_ptr_t pDataFormat,
						   PZOE_OBJECT_HANDLE phStream,
						   ZV_DEVICE_CALLBACK pFuncCallback,
						   zoe_void_ptr_t context
						   );
		zoe_errs_t (*close)(i_zv_codec *This, 
							ZOE_OBJECT_HANDLE hStream
							);
		zoe_errs_t (*start)(i_zv_codec *This, 
							ZOE_OBJECT_HANDLE hStream
							);
		zoe_errs_t (*stop)(i_zv_codec *This, 
							ZOE_OBJECT_HANDLE hStream
							);
		zoe_errs_t (*acquire)(i_zv_codec *This, 
							  ZOE_OBJECT_HANDLE hStream
							  );
		zoe_errs_t (*pause)(i_zv_codec *This, 
							ZOE_OBJECT_HANDLE hStream
							);
		zoe_errs_t (*set_rate)(i_zv_codec *This, 
							   ZOE_OBJECT_HANDLE hStream,
							   int32_t lNewRate
							   );
		zoe_errs_t (*get_rate)(i_zv_codec *This, 
							   ZOE_OBJECT_HANDLE hStream,
							   int32_t * plRate
							   );
		zoe_errs_t (*begin_flush)(i_zv_codec *This, 
								  ZOE_OBJECT_HANDLE hStream
								  );
		zoe_errs_t (*flush)(i_zv_codec *This, 
							ZOE_OBJECT_HANDLE hStream
							);
		zoe_errs_t (*end_flush)(i_zv_codec *This, 
							    ZOE_OBJECT_HANDLE hStream
							    );
		zoe_errs_t (*add_buffer)(i_zv_codec *This, 
								 ZOE_OBJECT_HANDLE hStream,
								 PZV_BUFFER_DESCRIPTOR pBufDesc
								 );
		zoe_errs_t (*cancel_buffer)(i_zv_codec *This, 
								    ZOE_OBJECT_HANDLE hStream,
								    PZV_BUFFER_DESCRIPTOR pBufDesc
								    );
		zoe_errs_t (*timeout_buffer)(i_zv_codec *This, 
									 ZOE_OBJECT_HANDLE hStream,
			  						 PZV_BUFFER_DESCRIPTOR pBufDesc
			  						 );
    };


/////////////////////////////////////////////////////////////////////////////
//
//
#define i_zv_codec_alloc_task(This, type, phTask) \
	((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->alloc_task(This, type, phTask)))

#define i_zv_codec_release_task(This, hTask) \
	((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->release_task(This, hTask)))

#define i_zv_codec_open(This, hTask, dwFlags, pDataFormat, phStream, pFuncCallback, context) \
	((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->open(This, hTask, dwFlags, pDataFormat, phStream, pFuncCallback, context)))

#define i_zv_codec_close(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->close(This, hStream)))

#define i_zv_codec_start(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->start(This, hStream)))

#define i_zv_codec_stop(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->stop(This, hStream)))

#define i_zv_codec_acquire(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->acquire(This, hStream)))

#define i_zv_codec_pause(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->pause(This, hStream)))

#define i_zv_codec_set_rate(This, hStream, lNewRate) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->set_rate(This, hStream, lNewRate)))

#define i_zv_codec_get_rate(This, hStream, plRate) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->get_rate(This, hStream, plRate)))

#define i_zv_codec_begin_flush(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->begin_flush(This, hStream)))

#define i_zv_codec_flush(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->flush(This, hStream)))

#define i_zv_codec_end_flush(This, hStream) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->end_flush(This, hStream)))

#define i_zv_codec_add_buffer(This, hStream, pBufDesc) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->add_buffer(This, hStream, pBufDesc)))

#define i_zv_codec_cancel_buffer(This, hStream, pBufDesc) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->cancel_buffer(This, hStream, pBufDesc)))

#define i_zv_codec_timeout_buffer(This, hStream, pBufDesc) \
    ((ZOE_NULL == This) ? ZOE_ERRS_PARMS : ((This)->timeout_buffer(This, hStream, pBufDesc)))

#pragma pack()


#ifdef __cplusplus
}
#endif

#endif //__ZV_CODEC_H__




