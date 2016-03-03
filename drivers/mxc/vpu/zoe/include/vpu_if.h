/* ==========================================================================
 *
 *  COPYRIGHT:     Freescale Inc. Proprietary and Confidential
 *                      Copyright (c) 2015, Freescale Inc.
 *                            All rights reserved.
 *
 *         Permission to use, modify or copy this code is prohibited
 *           without the express written consent of Freescale Inc.
 *
 * ==========================================================================*/

#ifndef __VPU_IF_H__
#define __VPU_IF_H__


#include "vpu_nfy.h"
#include "vpu_codec.h"
///
///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// \page _vpu_if vpu_if 
///
/// \ref _vpu_if is the client interface to the VPU.
/// ----------------------------------------------------------------------------------------------
/// \section _vpu_if_page_content Page Content
///
/// \ref _vpu_if_intro
///
/// \ref _vpu_if_model
///
/// \ref _vpu_input_compressed
///
/// \ref _vpu_events
///
/// \ref _vpu_sequence_information
///
/// \ref _vpu_picture_information
///
/// \ref _vpu_frame_buffers
///
/// \ref _vpu_types_overview
///
/// \ref _vpu_functions_overview
///
/// \ref _vpu_picture_structure
///
/// ----------------------------------------------------------------------------------------------
///	\section _vpu_if_intro VPU Introduction
/// 
/// The VPU (Video Processing unit) is a hardware engine that can decode streams of different standards .
///
/// \copydoc vpu_video_standard
///
/// ----------------------------------------------------------------------------------------------
///	\section _vpu_if_model VPU Model
///
///	The VPU has a dedicated CPU to run its firmware. To communicate with this firmware marshalling will take place on
/// top of the interface that is defined here.
/// As such this interface is exposed to the host CPU where a V4L2 API implementation will use it.
/// Inputs are the compressed data stream, configuration settings, stream settings when starting a stream, yuv buffer 
/// specifications, and stream control commands.
/// Outputs are YUV data, User data/SEI messages, events specifying status, decoding state, sequence information,
/// and PES timestamps.
///
/// \subsection _vpu_input_compressed VPU Input compressed data
///
/// Data can be input into the system as data chunks specifying start and size (see \ref vpu_add_bitbuffer). When
/// the buffer is not needed any more an event is signalled to notify the client (see \ref vpu_subscribe, 
/// \ref VPU_DEVT_BITBUFFERADDED). The client can then recycle the buffer. In the sequence header passed back 
/// to the client, the size of the VBV buffer is indicated (see \ref vpu_required_vbv_size). Basically buffers will
/// be circulating between the client and the VPU where the client fills the buffers and the VPU empties the buffers.
/// The VPU will detect start codes to find the picture/header boundaries and parse all header data. The VPU will do
/// the emulation prevention byte removal.
/// The VPU can operate in PES mode or ES mode except for the HEVC and VP9 video standards.
///
/// \subsection _vpu_events VPU Events
/// A client can subscribe to events from the VPU using a subscribe function that has a callback function parameter
/// (see \ref vpu_subscribe). A client must subscribe for each stream separately. In the implementation a dedicated 
/// notification channel used for each stream so that streams are independent. The client needs to implement the 
/// callback function (see \ref vpu_decode_event_nfy). This callback function specifies the stream, the event and 
/// passes event data for the client to use (see \ref vpu_decode_event).
///
/// \copydetails _vpu_nfy
///
/// \subsection _vpu_sequence_information VPU Sequence information
/// As headers are being processed, meta data is collected from a stream. The sequence parameter set information is 
/// signalled to the client by event \ref VPU_DEVT_SEQUENCEHEADERUPDATE). Each sequence header sent with this event
/// has a sequence id (\ref vpu_seq_id_t) that identifies this sequence. Picture meta data that is received will refer
/// to the sequence using this id. The client needs to hold on to the sequence data until no pictures refer to it 
/// any more.
/// \copybrief vpu_sequence
///
/// \subsection _vpu_picture_information VPU Picture Information.
/// Pictures are sent to the client through event \ref VPU_DEVT_FRAMEDECODECOMPLETE. Each picture sent with this event
/// has a sequence id (see \ref vpu_seq_id_t) that identifies the sequence information that applies to this picture.
/// Also it contains a buffer id that specifies the video frame buffer used.
/// \copybrief vpu_picture
/// 
/// \subsection _vpu_frame_buffers VPU Picture buffer registration
/// Picture buffers are created by the client and registered at the VPU (see \ref vpu_register_picture_buffer).
/// Each picture buffer has a unique buffer id that is unique per stream. The picture buffers are shared between
/// the client and the VPU. The VPU may be using the buffer as a reference frame while at the same time the client
/// uses it as a display buffer. The VPU selects the buffer into which to decode next. The client indicates for
/// each buffer it received from the VPU that it is done with it using \ref vpu_done_with_picture_buffer. This
/// way the VPU will know when to re-use a buffer. In the sequence data (see \ref vpu_sequence) it is indicated
/// how many buffers are needed for the current profile and level. Use \ref vpu_required_nr_raw. Buffers can be 
/// added and removed dynamically. For a stream with a certain resolution and profile and level, it would stay
/// the same though.
///
/// The client must create and destroy the buffers. The client receives decoded buffers for which is must indicate
/// that it is done. The client can unregister a buffer, but it will get an error back when the buffer is still
/// in use. It could try again later. The client must not destroy a buffer which is still registered.
///
/// ----------------------------------------------------------------------------------------------
///
/// \section _vpu_types_overview VPU Types Overview
///
/// \copybrief VPU_MAX_DECODE_STREAMS
///
/// \copybrief vpu_stream_id_t
///
/// \copybrief vpu_buffer_id_t
///
/// \copybrief vpu_data_buffer_id_t
///
/// \copybrief vpu_ret
///
/// \copybrief vpu_video_standard
///
/// \copybrief vpu_data_buffer
///
/// \copybrief vpu_pixel_format
///
/// \copybrief vpu_component
///
/// \copybrief vpu_buffer
///
/// \copybrief vpu_mode
///
/// \copybrief vpu_order
///
/// \copybrief vpu_start_flags
///
/// \copybrief vpu_stream_buffer
///
/// \copybrief vpu_jpeg_rotation
///
/// \copybrief vpu_jpeg_scale
///
/// \copybrief vpu_jpeg_rotation
///
/// \copybrief vpu_jpeg_yuv
///
/// \copybrief vpu_reconfig
///
/// \copybrief vpu_start_cfg
///
/// \copybrief vpu_start_flags_are_set
///
/// \copybrief vpu_start_flag_is_set
///
/// \copybrief vpu_stop_cfg
///
/// \copybrief vpu_state
///
/// \copybrief vpu_status
///
/// \copybrief vpu_decode_event
///
/// ----------------------------------------------------------------------------------------------
///
/// \section _vpu_functions_overview VPU Functions Overview
///
/// \copybrief vpu_decode_event_nfy
///
/// \copybrief vpu_subscribe
///
/// \copybrief vpu_init
///
/// \copybrief vpu_deinit
///
/// \copybrief vpu_reconfig
///
/// \copybrief vpu_start
///
/// \copybrief vpu_stop
///
/// \copybrief vpu_mode_set
///
/// \copybrief vpu_order_set
///
/// \copybrief vpu_next_picture
///
/// \copybrief vpu_next_sequence
///
/// \copybrief vpu_finish_stream
///
/// \copybrief vpu_status
///
/// \copybrief vpu_add_bitbuffer
///
/// \copybrief vpu_bitbuffer_size
///
/// \copybrief vpu_register_picture_buffer
///
/// \copybrief vpu_unregister_picture_buffer
///
/// \copybrief vpu_done_with_picture_buffer
///
/// \copybrief vpu_register_ud_buffer
///
/// \copybrief vpu_unregister_ud_buffer
///
/// \copybrief vpu_done_with_ud_buffer
///
/// \copybrief vpu_ud_config
///
/// ----------------------------------------------------------------------------------------------
///
/// \copydoc _vpu_picture
///
#ifdef __cplusplus
extern "C" {
#endif

    /// Enum \ref vpu_video_standard specifies the compressed video standard, or undefined; for example \ref VPU_VIDEO_HEVC.
    typedef enum vpu_video_standard
    {
        VPU_VIDEO_UNDEFINED = 0,
        VPU_VIDEO_AVC = 1,     ///< https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC
        VPU_VIDEO_VC1 = 2,     ///< https://en.wikipedia.org/wiki/VC-1
        VPU_VIDEO_MPEG2 = 3,   ///< https://en.wikipedia.org/wiki/H.262/MPEG-2_Part_2
        VPU_VIDEO_AVS = 4,     ///< https://en.wikipedia.org/wiki/Audio_Video_Standard
        VPU_VIDEO_ASP = 5,     ///< https://en.wikipedia.org/wiki/MPEG-4_Part_2
        VPU_VIDEO_JPEG = 6,    ///< https://en.wikipedia.org/wiki/JPEG
        VPU_VIDEO_RV8 = 7,     ///< https://en.wikipedia.org/wiki/RealVideo
        VPU_VIDEO_RV9 = 8,     ///< https://en.wikipedia.org/wiki/RealVideo
        VPU_VIDEO_VP6 = 9,     ///< https://en.wikipedia.org/wiki/VP6
        VPU_VIDEO_VP7 = 10,    ///< https://en.wikipedia.org/wiki/VP7
        VPU_VIDEO_SPK = 11,    ///< https://en.wikipedia.org/wiki/Sorenson_Media#Sorenson_Spark
        VPU_VIDEO_VP8 = 12,    ///< https://en.wikipedia.org/wiki/VP8
        VPU_VIDEO_AVC_MVC = 13,///< https://en.wikipedia.org/wiki/Multiview_Video_Coding
        VPU_VIDEO_VP3 = 14,    ///< https://en.wikipedia.org/wiki/VP3
        VPU_VIDEO_HEVC = 15,   ///< https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding
        VPU_VIDEO_VP9 = 16,    ///< https://en.wikipedia.org/wiki/VP9
    } vpu_video_standard_t;

     /// Enum \ref vpu_mode indicates the mode of operation of the decoder.
    /// Normal mode of operation is STEP where each frame of the decoded stream is output.
    typedef enum vpu_mode
    {
        VPU_STEP = 1,   ///< Return each frame.
        VPU_SKIP_PB = 2,   ///< Skip P and B frames.
        VPU_SKIP_B = 3,   ///< Skip B frames.
        VPU_SKIP_ALL = 4,   ///< Skip all frames. Note meta data of picture is still returned.
        VPU_IDR_ONLY = 5,   ///< Skip all but IDR frames.
    } vpu_mode_t;

    /// Enum \ref vpu_order sets the order in which frames are returned, which is either decode order or presentation order.
    typedef enum vpu_order
    {
        VPU_ORDER_PRESENTATION = 1, ///< The frames are returned in presentation order.
        VPU_ORDER_DECODE = 2  ///< The frames are returned in decode order.
    } vpu_order_t;



    /// Enum \ref vpu_start_flags specifies flags for the \ref vpu_start() function.
    typedef enum vpu_start_flags
    {
        VPU_START_PES = 0x00000001, ///< Decode PES format.
        VPU_START_STILL = 0x00000002, ///< Mark that the stream is a low bitrate still image and hence decode needs to be controlled by special flush commands.
        VPU_START_DEBLOCK = 0x00000004, ///< Enable deblocking for MP2 and ASP
        VPU_START_DERING = 0x00000008, ///< Enable deblocking for MP2 and ASP
        VPU_START_ADAPT_DBDR = 0x00000010, ///< Use adaptive deblocking/deringing for MP2 and ASP
        VPU_START_FRAMERATE = 0x00000020, ///< Override stream frame rate.
        VPU_START_1FIELDONLY = 0x00000040, ///< Decode only one field for H.264 to gain speedup (specific streams only).
        VPU_START_AVOID_HDR_RESYNC = 0x00000080, ///< Used for I-frame trickmodes to avoid having to find another SPS/PPS prior to the I-frame decode.
        VPU_START_EXPECT_MVC = 0x00000100, ///< Used for I-frame trickmodes to avoid having to find another SPS/PPS prior to the I-frame decode.
        VPU_START_REALTIME = 0x00000200, ///< Realtime priority streams get priority over non-realtime.
    } vpu_start_flags_t;

    /// Struct \ref vpu_stream_buffer specifies a circular buffer with high and low water marks, read, write, start and end pointers.
    typedef struct vpu_stream_buffer
    {
        vpu_address_t buf_address_;  ///< Base address of the buffer;
        uint32_t write_offset_;      ///< Write pointer of the buffer is base_address_ + write_offset_, the highest 4 bits contain the wrap count.
        uint32_t read_offset_;       ///< Read pointer of the buffer is base_address_ + read_offset_, the highest 4 bits contain the wrap count.
        uint32_t start_offset_;      ///< Start address of the buffer is base_address_ + start_offset_, the highest 4 bits are reserved.
        uint32_t end_offset_;        ///< End address of the buffer is base_address_ + end_offset_, the highest 4 bits are reserved.
        uint32_t low_water_mark_;    ///< Low water mark in the buffer is base_address_ + low_water_mark_
        uint32_t high_water_mark_;   ///< High water mark in the buffer is base_address_ + high_water_mark_
    } vpu_stream_buffer_t;
    
    /// Function \ref vpu_stream_write_ptr returns the write pointer; when data is sourced the write pointer is modified accordingly to a byte boundary.
    STATIC_INLINE vpu_address_t vpu_stream_write_ptr(vpu_stream_buffer_t* stream_buffer, uint32_t* nr_wrap_arounds)
    {
        if(nr_wrap_arounds) {
            *nr_wrap_arounds = (stream_buffer->write_offset_ >> 28);
        }
        return stream_buffer->buf_address_ + (stream_buffer->write_offset_ & 0x0fffffff);
    }

    /// Function \ref vpu_stream_read_ptr returns the read pointer as data is read from the buffer.
    STATIC_INLINE vpu_address_t vpu_stream_read_ptr(vpu_stream_buffer_t* stream_buffer, uint32_t* nr_wrap_arounds)
    {
        if(nr_wrap_arounds) {
            *nr_wrap_arounds = (stream_buffer->write_offset_ >> 28);
        }
        return stream_buffer->buf_address_ + (stream_buffer->read_offset_ & 0x0fffffff);
    }

    /// Function \ref vpu_stream_start_ptr returns the start pointer indicating the start addrss of the buffer
    STATIC_INLINE vpu_address_t vpu_stream_start_ptr(vpu_stream_buffer_t* stream_buffer)
    {
        return stream_buffer->buf_address_ + (stream_buffer->start_offset_ & 0x0fffffff);
    }

    /// Function \ref vpu_stream_end_ptr_ returns the end pointer.
    STATIC_INLINE vpu_address_t vpu_stream_end_ptr_(vpu_stream_buffer_t* stream_buffer)
    {
        return stream_buffer->buf_address_ + (stream_buffer->end_offset_ & 0x0fffffff);
    }

    /// Function \ref vpu_low_water_mark returns the low water mark.
    STATIC_INLINE vpu_address_t vpu_low_water_mark(vpu_stream_buffer_t* stream_buffer)
    {
        return stream_buffer->buf_address_ + (stream_buffer->low_water_mark_ & 0x0fffffff);
    }

    /// Function \ref vpu_low_water_mark returns the low water mark.
    STATIC_INLINE vpu_address_t vpu_high_water_mark(vpu_stream_buffer_t* stream_buffer)
    {
        return stream_buffer->buf_address_ + (stream_buffer->high_water_mark_ & 0x0fffffff);
    }

    //
    //typedef enum
    //{
    //    VcipMsd_AnchorMode = 0,
    //    VcipMsd_SubGopMode,
    //    VcipMsd_SmoothFFWD,
    //    VcipMsd_Null
    //
    //} VcipMsd_PvrType;

    /// Enum \ref vpu_jpeg_rotation specifies 90% rotation angles.
    typedef enum vpu_jpeg_rotation
    {
        VPU_JPEG_ROTATION_NONE = 0, ///< No rotation specified.
        VPU_JPEG_ROTATION_90,       ///< Rotate 90 degrees.
        VPU_JPEG_ROTATION_180,      ///< Rotate 180 degrees.
        VPU_JPEG_ROTATION_270       ///< Rotate 270 degrees.
    } vpu_jpeg_rotation_t;

    /// Enum \ref vpu_jpeg_scale specifies scaling ratios 2,4 and 8.
    typedef enum vpu_jpeg_scale
    {
        VPU_JPEG_SCALE_NONE = 0,   ///< No scaling specified.
        VPU_JPEG_SCALE_2_TO_1,     ///< Scale down 2.
        VPU_JPEG_SCALE_4_TO_1,     ///< Scale down 4.
        VPU_JPEG_SCALE_8_TO_1      ///< Scale down 8.
    } vpu_jpeg_scale_t;

    /// Enum \ref vpu_jpeg_yuv specifies the JPEG output format 420, 444 or RGB.
    typedef enum vpu_jpeg_yuv
    {
        VPU_JPEG_YUV_420 = 0,      ///< YUV 420
        VPU_JPEG_YUV_444,          ///< YUV 444
        VPU_JPEG_YUV_RGB           ///< YUV RGB
    } vpu_jpeg_yuv_t;


    /// Struct \ref vpu_reconfig specifies settings that can be done before and after \ref vpu_start..
    typedef struct vpu_reconfig
    {
        uint32_t            start_flags_;     ///< OR'd \ref vpu_start_flags_t, only VPU_START_DEBLOCK/VPU_START_DERING are supported
        vpu_jpeg_rotation_t jpeg_rotation_;   ///< 0, 90, 180 or 270 degrees rotation.
        vpu_jpeg_scale_t    jpeg_hscale_;     ///< Scale down horizontally 1, 2, 4, or 8 times.
        vpu_jpeg_scale_t    jpeg_vscale_;     ///< Scale down vertically 1, 2, 4, or 8 times.
        vpu_jpeg_yuv_t      jpeg_yuv_;        ///< YUV 420, 444 or RGB format.
    } vpu_reconfig_t;

    /// Struct \ref vpu_start_cfg contains settings that can be done at \ref vpu_start only.
    typedef struct vpu_start_cfg
    {
        uint32_t                start_flags_;     ///< OR'd \ref vpu_start_flags_t
        vpu_video_standard_t    video_standard_;  ///< The compressed video standard to decode.
        vpu_stream_buffer_t     stream_buffer_;   ///< Description of the circular buffer.
        vpu_frame_rate_t        frame_rate_; ///< Frame rate override when start flag VPU_START_FRAMERATE is set.
        vpu_sequence_t*         sequence_; ///< Sequence parameter set override, NULL if not overriding.
    } vpu_start_cfg_t;


    /// Function \ref vpu_seq_flags_are_set returns true when all flags are set.
    STATIC_INLINE uint32_t vpu_start_flags_are_set(const vpu_start_cfg_t* start_cfg, uint32_t flags)
    {
        return (start_cfg->start_flags_ & flags) == flags;
    }

    /// Function \ref vpu_seq_flag_is_set returns true when a flag is set.
    STATIC_INLINE uint32_t vpu_start_flag_is_set(const vpu_start_cfg_t* start_cfg, vpu_pic_flags_t flag)
    {
        return vpu_start_flags_are_set(start_cfg, flag);
    }

    /// Function \ref vpu_cfg_video_standard returns the video standard as set in the start configuration.
    STATIC_INLINE vpu_video_standard_t vpu_cfg_video_standard(vpu_start_cfg_t* start_cfg)
    {
        return start_cfg->video_standard_;
    }

    /// Function \ref vpu_cfg_video_standard_set sets the video standard in the start configuration.
    STATIC_INLINE void vpu_cfg_video_standard_set(vpu_start_cfg_t* start_cfg, vpu_video_standard_t video_standard)
    {
        start_cfg->video_standard_ = video_standard;
    }

    /// Function \ref vpu_cfg_stream_buffer returns the stream buffer as set in the start configuration.
    STATIC_INLINE vpu_stream_buffer_t vpu_cfg_stream_buffer(vpu_start_cfg_t* start_cfg)
    {
        return start_cfg->stream_buffer_;
    }

    /// Function \ref vpu_cfg_stream_buffer_set sets the stream buffer in the start configuration.
    STATIC_INLINE void vpu_cfg_stream_buffer_set(vpu_start_cfg_t* start_cfg, vpu_stream_buffer_t stream_buffer)
    {
        start_cfg->stream_buffer_ = stream_buffer;
    }

    /// Function \ref vpu_cfg_frame_rate returns the frame rate as set in the start configuration.
    STATIC_INLINE vpu_frame_rate_t vpu_cfg_frame_rate(vpu_start_cfg_t* start_cfg)
    {
        return start_cfg->frame_rate_;
    }

    /// Function \ref vpu_cfg_frame_rate_set sets the frame rate in the start configuration.
    STATIC_INLINE void vpu_cfg_frame_rate_set(vpu_start_cfg_t* start_cfg, vpu_frame_rate_t frame_rate)
    {
        start_cfg->frame_rate_ = frame_rate;
    }

    /// Function \ref vpu_cfg_sequence returns the frame rate as set in the start configuration.
    STATIC_INLINE vpu_sequence_t* vpu_cfg_sequence(vpu_start_cfg_t* start_cfg)
    {
        return start_cfg->sequence_;
    }

    /// Function \ref vpu_cfg_sequence_set sets the frame rate in the start configuration.
    STATIC_INLINE void vpu_cfg_sequence_set(vpu_start_cfg_t* start_cfg, vpu_sequence_t* sequence)
    {
        start_cfg->sequence_ = sequence;
    }


    /// Enum \ref vpu_stop_cfg indicates how to stop; immediately or gracefully.
    typedef enum vpu_stop_cfg
    {
        VPU_STOP_IMMEDIATE = 0, ///< Stop immediately dropping all output data.
        VPU_STOP_FINISH = 1,    ///< Stop immediately, but output remaining data in order.
        VPU_STOP_CLEAN = 2      ///< Complete frame cleanly and output all data in order.

    } vpu_stop_cfg_t;

    /// Enum \ref vpu_state represents the state of decoding.
    typedef enum vpu_state
    {
        VPU_STATE_INACTIVE,         ///< Stream is not active.
        VPU_STATE_DECODING,         ///< Stream is actively decoding, when in pause state it means it is active but currently not scheduled.
        VPU_STATE_PAUSED,           ///< Finished workload requested, internal state.
        VPU_STATE_STALLED,          ///< Stalled waiting on frame store to decode into.
        VPU_STATE_WAITING_COMMAND,  ///< Stopped waiting for next command from control.
        VPU_STATE_STOPPING,         ///< In the process of stopping. Waiting to flush all frames or complete last frame.
        VPU_STATE_STOPPED,          ///< Transitory state to InActive.
    } vpu_state_t;

    /// Struct \ref vpu_status returns the status of a stream.
    typedef struct vpu_status
    {
        vpu_state_t state_;         ///< Current state of decoding.
        uint32_t nr_decoded_;       ///< Count of total frames decoded since start.
        uint32_t nr_errors_;        ///< Count of errors received when decoding the stream since start.
        uint32_t nr_ref_frames_;    ///< Number of reference frames currently used.
        uint32_t nr_active_frames_; ///< Number of frames active in decode space, ref and ones not ready yet to be displayed.
    
        vpu_stream_buffer_t stream_buffer_;  ///< Current stream buffer positions.
    } vpu_status_t;



    /// Notification \ref vpu_decode_event_nfy specifies the client defined callback function.
    /// \pre Decoder is initialized.
    /// \param[in] decode_event The event that is being signalled.
    /// \param[in] event_data The event data with the event. See \ref vpu_decode_event to see what event data comes with a specific event.
    /// \param[in] event_data_size The size of the event data.
    typedef void(*vpu_decode_event_nfy) (vpu_stream_id_t stream_id, vpu_decode_event_t decode_event, void* event_data, uint32_t event_data_size);

    /// Callback \ref vpu_pts_generate_callback to call into client side code that generates PTSs.
    /// \pre Decoder is initialized.
    /// \param[in] picture The picture in which the PTS is to be filled in.
    typedef void(*vpu_pts_generate_callback) (vpu_stream_id_t stream_id, vpu_picture_t* picture);

    /// Function \ref vpu_bind_pts_generate_callback installs the callback function that is used to generate PTSs.
    void vpu_bind_pts_generate_callback(vpu_pts_generate_callback pts_generate_callback);

    /// Function \ref vpu_init initializes the VPU for decoding.
    /// \pre Decoder is uninitialized.
    /// \returns VPU_RET_OK when the decoder was successfully initialized.
    /// \returns VPU_RET_PRECONDITION when already initialized.
    /// \returns VPU_RET_OUT_OF_MEMORY when any memory needed could not be created. All memory will have been released.
    vpu_ret_t vpu_init(void);

    /// Function \ref vpu_deinit de-initializes the VPU for decoding.
    /// \pre None
    /// \returns VPU_RET_OK when the decoder instance was successfully closed, or if it was closed already.
    vpu_ret_t vpu_deinit(void);

    /// Function \ref vpu_subscribe subscribes a callback per stream, events for that stream will be sent to the specified stream.
    /// The event_mask specifies the events that are enabled, it is an OR of all (1 << vpu_decode_event_t)
    /// To unsubscribe pass a null function pointer.
    /// \param[in] ntf The callback that is called when an event occurs.
    /// \param[in] event_mask The events that are enabled, it is an OR of all (1 << vpu_decode_event_t) that apply.
    /// \pre Decoder is initialized.
    /// \returns VPU_RET_OK when subscription was done
    vpu_ret_t vpu_subscribe(vpu_stream_id_t stream_id, vpu_decode_event_nfy ntf, uint32_t event_mask);

    /// Function \ref vpu_create creates a decoder instance for a specific stream id. The decoder will be an hevc, vp9 or multistandard decoder.
    /// If there is already a decoder created for that stream id, it will be reused if it supports the video standard. If it does not support
    /// the video standard, it will be destroyed prior to creating a new one.
    /// \pre None
    /// \param[in] video_standard The compressed video standard.
    /// \returns VPU_RET_OK when the decoder instance was successfully created, or if it was created already.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    vpu_ret_t vpu_create(vpu_stream_id_t stream_id, vpu_video_standard_t video_standard);

    /// Function \ref vpu_destroy destroys a decoder instance for a specific stream id.
    /// \pre Decoder is initialized. Can be called before or after start.
    /// \returns VPU_RET_OK 
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    vpu_ret_t vpu_destroy(vpu_stream_id_t stream_id);

    /// Function \ref vpu_reconfig (re)configures the decoder.
    /// \pre Decoder is initialized. Can be called before or after start.
    /// \param[in] reconfig The configuration options that can be set before or after start.
    /// \returns VPU_RET_OK when the decoder instance was successfully reconfigured.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    vpu_ret_t vpu_reconfig(vpu_stream_id_t stream_id, vpu_reconfig_t reconfig);

    /// Function \ref vpu_start configures and starts the decoder.
    /// \pre Decoder is initialized.
    /// \param[in] start_cfg The configuration options that must be set before start and cannot be changed until after stop.
    /// \returns VPU_RET_OK when the decoder instance was successfully started.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    vpu_ret_t vpu_start(vpu_stream_id_t stream_id, vpu_start_cfg_t start_cfg);

    /// Function \ref vpu_stop stops the decoder.
    /// \pre Decoder is initialized.
    /// \param[in] stop_cfg Stop immediately, imediately and output remaining data, or finish cleanly and output data.
    /// \returns VPU_RET_OK when the decoder was successfully stopped.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    vpu_ret_t vpu_stop(vpu_stream_id_t stream_id, vpu_stop_cfg_t stop_cfg);

    /// Function \ref vpu_mode_set sets the decode mode (step, skip, idr only, ...). See \ref vpu_mode.
    /// \pre Decoder is initialized.
    /// \param[in] mode Indicates whether to skip Bs, Ps, and so on.
    /// \returns VPU_RET_OK when the decode mode is set.
    /// \returns VPU_RET_INVALID_PARAM when the decode mode is out of its range.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
	vpu_ret_t vpu_mode_set(vpu_stream_id_t stream_id, vpu_mode_t mode);

    /// Function \ref vpu_order_set sets the decode order (presentation order or decode order). See \ref vpu_order.
    /// \pre Decoder is initialized.
    /// \param[in] order Presentation or decode order.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_PARAM when the order is out of its range.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
	vpu_ret_t vpu_order_set(vpu_stream_id_t stream_id, vpu_order_t order);

    /// Function \ref vpu_next_picture returns the next video frame/field.
    /// VPU_PIC_SKIP in vpu_pic_flags_t will indicate if the picture is skipped. Reference frames typically won't be skipped as they are
    /// needed for decoding subsequent pictures.
    /// \pre Decoder is initialized.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_PARAM when the parameters passed are null pointers or skip is out of range.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
	vpu_ret_t vpu_next_picture(vpu_stream_id_t stream_id);

    /// Function \ref vpu_next_sequence makes the decoder search for a next sequence entry point and parse the sequence header.
    /// \pre Decoder is initialized.
    /// \returns VPU_RET_OK when the decoder searching has started.
	vpu_ret_t vpu_next_sequence(vpu_stream_id_t stream_id);

    /// Function \ref vpu_finish_stream informs the decoder that enough data is available for still image or JPEG decode.
    /// \pre Decoder is initialized.
    /// \returns VPU_RET_OK when the decoder finalizing stream has started.
	vpu_ret_t vpu_finish_stream(vpu_stream_id_t stream_id);

    /// Function \ref vpu_status returns the status of the stream
    /// \pre Decoder is initialized.
    /// \param[out] status The status of the stream since \ref vpu_start.
    /// \returns VPU_RET_OK
	vpu_ret_t vpu_status(vpu_stream_id_t stream_id, vpu_status_t* status);

    
    /// Function \ref vpu_add_bitbuffer puts compressed bits data into the decoder; the decoder will find the next startcode 
    /// before attempting to parse or decode anything.
    /// These buffers are returned to the client using notify_event() after which it won't be accessed anymore.
    /// The amount of buffering needed is returned with the picture type (vpu_required_vbv_size()).
    /// \pre Decoder is initialized.
    /// \invariant The buffer will be returned to the client with the same ids that were passed in.
    /// \param[in] buffer The addresses of the buffer containing the compressed data.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_OUT_OF_MEMORY too many buffers are being added - not enough room to store in local array.
    /// \returns VPU_RET_INVALID_BUFFER when the specified buffer is not valid.
	vpu_ret_t vpu_add_bitbuffer(vpu_stream_id_t stream_id, vpu_data_buffer_t buffer);

    /// Function \ref vpu_bitbuffer_size compressed bits data input buffer size - the part not consumed yet. The sum of all buffers passed through
    /// \ref vpu_add_bitbuffer.
    /// \pre Decoder is initialized.
    /// \param[out] bitbuffer_size The amount of data accumulated in the VPU that has not been processed yet.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
	vpu_ret_t vpu_bitbuffer_size(vpu_stream_id_t stream_id, uint32_t* bitbuffer_size);


    /// Function \ref vpu_register_picture_buffer register buffer to be used as a frame buffer.
    /// The sizes and amount of buffers needed is returned in the sequence information.
    /// \pre Decoder is initialized.
    /// \pre buffer->buffer_id_ uniquely identifies the buffer in stream context.
    /// \param[in] buffer The frame buffer to register.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the specified buffer is not valid (not right size, format).
    /// \returns VPU_RET_BUFFER_IN_USE when the buffer has already been registered before.
    /// \returns VPU_RET_OUT_OF_MEMORY too many buffers are being registered - not enough room to store in local array.
	vpu_ret_t vpu_register_picture_buffer(vpu_stream_id_t stream_id, vpu_buffer_t* buffer);

    /// Function \ref vpu_unregister_picture_buffer removes a buffer from the set of
    /// buffers being used to decode.
    /// \pre Decoder is initialized.
    /// \param[in] buffer_id The buffer id of the frame buffer to unregister.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the buffer is invalid; the buffer must have been registered before.
    /// \returns VPU_RET_BUFFER_IN_USE when the buffer is still being used as a reference.
	vpu_ret_t vpu_unregister_picture_buffer(vpu_stream_id_t stream_id, vpu_buffer_id_t buffer_id);

    /// Function \ref vpu_done_with_picture_buffer declares that buffer is not
    /// being used by the client anymore allowing the decoder to recycle the buffer
    /// as soon as it is not being used as a reference frame anymore.
    /// The buffer is specified in the picture returned by vpu_next_picture().
    /// To ensure buffer is not used at all anymore the client must use the STREAM_END marker to force
    /// pictures out of the decoder.
    /// \pre Decoder is initialized.
    /// \param[in] buffer_id The buffer id of the frame buffer that is not needed anymore by the client.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the buffer is invalid; the buffer must have been registered before.
	vpu_ret_t vpu_done_with_picture_buffer(vpu_stream_id_t stream_id, vpu_buffer_id_t buffer_id);

    /// Enum \ref vpu_syntax_flags indicates the type of user data to send.
    typedef enum vpu_syntax_flags
    {
        VPU_STL_SEQUENCE = 0x00000001,
        VPU_STL_PICTURE  = 0x00000002,
        VPU_STL_SLICE    = 0x00000004
    } vpu_syntax_flags_t;

    /// Enum \ref vpu_sei lists types of SEI messages
    typedef enum vpu_sei
    {
        VPU_SEI_BUFFERING_PERIOD = 0,
        VPU_SEI_PIC_TIMING = 1,
        VPU_SEI_PAN_SCAN_RECT = 2,
        VPU_SEI_FILLER_PAYLOAD = 3,
        VPU_SEI_USER_DATA_REGISTERED_ITU_T_T35 = 4,
        VPU_SEI_USER_DATA_UNREGISTERED = 5,
        VPU_SEI_RECOVERY_POINT = 6,
        VPU_SEI_DEC_REF_PIC_MARKING_REPETITION = 7,
        VPU_SEI_SPARE_PIC = 8,
        VPU_SEI_SCENE_INFO = 9,
        VPU_SEI_SUB_SEQ_INFO = 10,
        VPU_SEI_SUB_SEQ_LAYER_CHARACTERISTICS = 11,
        VPU_SEI_SUB_SEQ_CHARACTERISTICS = 12,
        VPU_SEI_FULL_FRAME_FREEZE = 13,
        VPU_SEI_FULL_FRAME_FREEZE_RELEASE = 14,
        VPU_SEI_FULL_FRAME_SNAPSHOT = 15,
        VPU_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,   
        VPU_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END = 17,
        VPU_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET = 18,
        VPU_SEI_FILM_GRAIN_CHARACTERISTICS = 19,
        VPU_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE = 20,
        VPU_SEI_STEREO_VIDEO_INFORMATION = 21,
        VPU_SEI_PARALLEL_DECODING_INFO = 22,
        VPU_SEI_MVC_SCALABLE_NESTING = 23,
        VPU_SEI_VIEW_SCALABILITY_INFO = 24,
        VPU_SEI_MULTIVIEW_SCENE_INFO = 25,
        VPU_SEI_MULTIVIEW_ACQUISITION_INFO = 26,
        VPU_SEI_NON_REQUIRED_VIEW_COMPONENT = 27,
        VPU_SEI_VIEW_DEPENDENCY_CHANGE = 28,
        VPU_SEI_OPERATION_POINTS_NOT_PRESENT = 29,
        VPU_SEI_BASE_VIEW_TEMPORAL_HRD = 30,
        VPU_SEI_FRAME_PACKING_ARRANGEMENT = 31,
    } vpu_sei_t;

    /// Enum \ref vpu_sei_flags_t lists types of SEI messages as flags that can be specified simultaneously
    /// for enabling purposes.
    typedef enum vpu_sei_flags_t
    {
        VPU_SEI_FL_BUFFERING_PERIOD                      = 1 << VPU_SEI_BUFFERING_PERIOD,
        VPU_SEI_FL_PIC_TIMING                            = 1 << VPU_SEI_PIC_TIMING,
        VPU_SEI_FL_PAN_SCAN_RECT                         = 1 << VPU_SEI_PAN_SCAN_RECT,
        VPU_SEI_FL_FILLER_PAYLOAD                        = 1 << VPU_SEI_FILLER_PAYLOAD,
        VPU_SEI_FL_USER_DATA_REGISTERED_ITU_T_T35        = 1 << VPU_SEI_USER_DATA_REGISTERED_ITU_T_T35,
        VPU_SEI_FL_USER_DATA_UNREGISTERED                = 1 << VPU_SEI_USER_DATA_UNREGISTERED,
        VPU_SEI_FL_RECOVERY_POINT                        = 1 << VPU_SEI_RECOVERY_POINT,
        VPU_SEI_FL_DEC_REF_PIC_MARKING_REPETITION        = 1 << VPU_SEI_DEC_REF_PIC_MARKING_REPETITION,
        VPU_SEI_FL_SPARE_PIC                             = 1 << VPU_SEI_SPARE_PIC,
        VPU_SEI_FL_SCENE_INFO                            = 1 << VPU_SEI_SCENE_INFO,
        VPU_SEI_FL_SUB_SEQ_INFO                          = 1 << VPU_SEI_SUB_SEQ_INFO,
        VPU_SEI_FL_SUB_SEQ_LAYER_CHARACTERISTICS         = 1 << VPU_SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
        VPU_SEI_FL_SUB_SEQ_CHARACTERISTICS               = 1 << VPU_SEI_SUB_SEQ_CHARACTERISTICS,
        VPU_SEI_FL_FULL_FRAME_FREEZE                     = 1 << VPU_SEI_FULL_FRAME_FREEZE,
        VPU_SEI_FL_FULL_FRAME_FREEZE_RELEASE             = 1 << VPU_SEI_FULL_FRAME_FREEZE_RELEASE,
        VPU_SEI_FL_FULL_FRAME_SNAPSHOT                   = 1 << VPU_SEI_FULL_FRAME_SNAPSHOT,
        VPU_SEI_FL_PROGRESSIVE_REFINEMENT_SEGMENT_START  = 1 << VPU_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
        VPU_SEI_FL_PROGRESSIVE_REFINEMENT_SEGMENT_END    = 1 << VPU_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
        VPU_SEI_FL_MOTION_CONSTRAINED_SLICE_GROUP_SET    = 1 << VPU_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
        VPU_SEI_FL_FILM_GRAIN_CHARACTERISTICS            = 1 << VPU_SEI_FILM_GRAIN_CHARACTERISTICS,
        VPU_SEI_FL_DEBLOCKING_FILTER_DISPLAY_PREFERENCE  = 1 << VPU_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
        VPU_SEI_FL_STEREO_VIDEO_INFORMATION              = 1 << VPU_SEI_STEREO_VIDEO_INFORMATION,
        VPU_SEI_FL_PARALLEL_DECODING_INFO                = 1 << VPU_SEI_PARALLEL_DECODING_INFO,
        VPU_SEI_FL_MVC_SCALABLE_NESTING                  = 1 << VPU_SEI_MVC_SCALABLE_NESTING,
        VPU_SEI_FL_VIEW_SCALABILITY_INFO                 = 1 << VPU_SEI_VIEW_SCALABILITY_INFO,
        VPU_SEI_FL_MULTIVIEW_SCENE_INFO                  = 1 << VPU_SEI_MULTIVIEW_SCENE_INFO,
        VPU_SEI_FL_MULTIVIEW_ACQUISITION_INFO            = 1 << VPU_SEI_MULTIVIEW_ACQUISITION_INFO,
        VPU_SEI_FL_NON_REQUIRED_VIEW_COMPONENT           = 1 << VPU_SEI_NON_REQUIRED_VIEW_COMPONENT,
        VPU_SEI_FL_VIEW_DEPENDENCY_CHANGE                = 1 << VPU_SEI_VIEW_DEPENDENCY_CHANGE,
        VPU_SEI_FL_OPERATION_POINTS_NOT_PRESENT          = 1 << VPU_SEI_OPERATION_POINTS_NOT_PRESENT,
        VPU_SEI_FL_BASE_VIEW_TEMPORAL_HRD                = 1 << VPU_SEI_BASE_VIEW_TEMPORAL_HRD,
        VPU_SEI_FL_FRAME_PACKING_ARRANGEMENT             = 1 << VPU_SEI_FRAME_PACKING_ARRANGEMENT
    } vpu_sei_flags_t;

    /// Function \ref vpu_register_ud_buffer adds a buffer to the set of buffers being used to store user data.
    /// \pre Decoder is initialized.
    /// \param[in] buffer The data buffer used to store user data.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the buffer is invalid; the buffer must have been registered before.
    /// \returns VPU_RET_BUFFER_IN_USE when the buffer is still being used as a reference.
    vpu_ret_t vpu_register_ud_buffer(vpu_stream_id_t stream_id, vpu_data_buffer_t* buffer);
	
    /// Function \ref vpu_unregister_ud_buffer removes a buffer to the set of buffers being used to store user data.
    /// \pre Decoder is initialized.
    /// \param[in] buffer_id The buffer id of the data buffer used to store user data.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the buffer is invalid; the buffer must have been registered before.
    /// \returns VPU_RET_BUFFER_IN_USE when the buffer is still being used as a reference.
    vpu_ret_t vpu_unregister_ud_buffer(vpu_stream_id_t stream_id, vpu_data_buffer_id_t buffer_id);

    /// Function \ref vpu_done_with_ud_buffer indicates the client is done with the user data buffer such that
    /// the VPU can reuse it.
    /// \pre Decoder is initialized.
    /// \param[in] buffer_id The buffer id of the data buffer not needed anymore by the client
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the buffer is invalid; the buffer must have been registered before.
    /// \returns VPU_RET_BUFFER_IN_USE when the buffer is still being used as a reference.
    vpu_ret_t vpu_done_with_ud_buffer(vpu_stream_id_t stream_id, vpu_data_buffer_id_t buffer_id);

    /// Function \ref vpu_ud_config Configures what user data is to be collected. 
    /// \pre Decoder is initialized.
    /// \param[in] syntax_flags Indicates which level of syntax data to provide; sequence, picture or slice. 
    /// \param[in] sei_flags Specifies which SEI data is to be collected.
    /// \param[in] order  The order of data delivery; presentation or decode order.
    /// \returns VPU_RET_OK when succesful.
    /// \returns VPU_RET_INVALID_HANDLE when the decoder is not initialized.
    /// \returns VPU_RET_INVALID_BUFFER when the buffer is invalid; the buffer must have been registered before.
    /// \returns VPU_RET_BUFFER_IN_USE when the buffer is still being used as a reference.
    vpu_ret_t vpu_ud_config(vpu_stream_id_t stream_id, vpu_syntax_flags_t syntax_flags, vpu_sei_flags_t sei_flags, vpu_order_t order);

    /// Function \ref vpu_hevc_buffer_sizes returns buffer sizes, widths and heights for HEVC frame buffers.
    /// When allocated the buffer needs to be aligned. Allocating buffer_size + alignment should fit all buffers.
    /// The luma start address should then be (address + (alignment - 1)) & (alignment - 1). Chroma and colocated buffers can be 
    /// allocated back to back without them getting misaligned.
    ///
    /// \code
    /// luma_address = (allocated_address + (alignment - 1)) & (alignment - 1);
    ///
    /// chroma_address = luma + luma_size
    ///
    /// colocated_address = chroma_address + chroma_size
    ///\endcode
    ///
    /// \param[in] width width of the picture 
    /// \param[in] height height of the picture 
    /// \param[in] is10bits A boolean indicating that it is 10 bits per pixel instead of 8
    /// \param[out] luma_width Stride of the luma buffer in bytes.
    /// \param[out] luma_height Height of the luma buffer in bytes.
    /// \param[out] luma_size Size of the luma buffer.
    /// \param[out] chroma_width Stride of the chroma buffer in bytes.
    /// \param[out] chroma_height Height of the chroma buffer in bytes.
    /// \param[out] chroma_size Size of the chroma buffer.
    /// \param[out] colocated_width Stride of the colocated buffer in bytes.
    /// \param[out] colocated_height Height of the colocated buffer in bytes.
    /// \param[out] colocated_size Size of the colocated buffer.
    /// \param[out] buffer_size Size of the total buffer when luma, chroma and colocated are allocated back to back.
    /// \param[out] alignment Alignment of the buffers.
    void vpu_hevc_buffer_sizes(uint32_t width, uint32_t height, uint32_t is10bits,
        uint32_t* luma_width, uint32_t* luma_height, uint32_t* luma_size,
        uint32_t* chroma_width, uint32_t* chroma_height, uint32_t* chroma_size,
        uint32_t* colocated_width, uint32_t* colocated_height, uint32_t* colocated_size,
        uint32_t* buffer_size, uint32_t* alignment);

    /// Temporary for hacking purposes
	void process_decode(vpu_stream_id_t stream_id);
#ifdef __cplusplus
}
#endif

#endif
