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

#ifndef __VPU_NFY_H__
#define __VPU_NFY_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "vpu_picture.h"
#include "vpu_codec.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// \page _vpu_nfy vpu_nfy
///
/// \ref _vpu_nfy notifies events to the client of the VPU decoders. If \ref vpu_nfy_signal
/// is used as a parameter to \ref vpu_subscribe it will call the functions specified below.
/// The client must implement these functions.
///
/// | Event                                 | Function called                            |
/// |---------------------------------------|--------------------------------------------|
/// | \ref VPU_DEVT_INITIALIZED             | \copybrief vpu_nfy_initialized             |
/// | \ref VPU_DEVT_TERMINATED              | \copybrief vpu_nfy_terminated              |
/// | \ref VPU_DEVT_STREAMACQUIRED          | \copybrief vpu_nfy_streamacquired          |
/// | \ref VPU_DEVT_STREAMLOSTOVERFLOW      | \copybrief vpu_nfy_streamlostoverflow      |
/// | \ref VPU_DEVT_STREAMLOSTSTARVED       | \copybrief vpu_nfy_streamloststarved       |
/// | \ref VPU_DEVT_STREAMLWMHIT            | \copybrief vpu_nfy_streamlwmhit            |
/// | \ref VPU_DEVT_STREAMHWMHIT            | \copybrief vpu_nfy_streamhwmhit            |
/// | \ref VPU_DEVT_STREAMSYNCACQUIRED      | \copybrief vpu_nfy_streamsyncacquired      |
/// | \ref VPU_DEVT_STREAMSYNCLOST          | \copybrief vpu_nfy_streamsynclost          |
/// | \ref VPU_DEVT_STREAMSEQSYNC           | \copybrief vpu_nfy_streamseqsync           |
/// | \ref VPU_DEVT_FRAMEHEADERFOUND        | \copybrief vpu_nfy_frameheaderfound        |
/// | \ref VPU_DEVT_FRAMEDECODECOMPLETE     | \copybrief vpu_nfy_framedecodecomplete     |
/// | \ref VPU_DEVT_SEQUENCEHEADERUPDATE    | \copybrief vpu_nfy_sequenceheaderupdate    |
/// | \ref VPU_DEVT_USERDATARECEIVED        | \copybrief vpu_nfy_userdatareceived        |
/// | \ref VPU_DEVT_USERDATARECEIVEDPICTURE | \copybrief vpu_nfy_userdatareceivedpicture |
/// | \ref VPU_DEVT_ENDSEQUENCERECEIVED     | \copybrief vpu_nfy_endsequencereceived     |
/// | \ref VPU_DEVT_DECODEERROR             | \copybrief vpu_nfy_decodeerror             |
/// | \ref VPU_DEVT_SYSTEMERROR             | \copybrief vpu_nfy_systemerror             |
/// | \ref VPU_DEVT_CHUNKDONE               | \copybrief vpu_nfy_chunkdone               |
/// | \ref VPU_DEVT_STREAMCOMPLETE          | \copybrief vpu_nfy_streamcomplete          |
/// | \ref VPU_DEVT_BITBUFFERADDED          | \copybrief vpu_nfy_bitbufferadded          |

/// Enum \ref vpu_decode_event are the events signalled to the client.
typedef enum vpu_decode_event
{
    VPU_DEVT_INITIALIZED = 0,              ///< \copybrief vpu_nfy_initialized
    VPU_DEVT_TERMINATED = 1,               ///< \copybrief vpu_nfy_terminated
    VPU_DEVT_STREAMACQUIRED = 2,           ///< \copybrief vpu_nfy_streamacquired
    VPU_DEVT_STREAMLOSTOVERFLOW = 3,       ///< \copybrief vpu_nfy_streamlostoverflow
    VPU_DEVT_STREAMLOSTSTARVED = 4,        ///< \copybrief vpu_nfy_streamloststarved
    VPU_DEVT_STREAMLWMHIT = 5,             ///< \copybrief vpu_nfy_streamlwmhit
    VPU_DEVT_STREAMHWMHIT = 6,             ///< \copybrief vpu_nfy_streamhwmhit  
    VPU_DEVT_STREAMSYNCACQUIRED = 7,       ///< \copybrief vpu_nfy_streamsyncacquired
    VPU_DEVT_STREAMSYNCLOST = 8,           ///< \copybrief vpu_nfy_streamsynclost
    VPU_DEVT_STREAMSEQSYNC = 9,            ///< \copybrief vpu_nfy_streamseqsync
    VPU_DEVT_FRAMEHEADERFOUND = 10,        ///< \copybrief vpu_nfy_frameheaderfound
    VPU_DEVT_FRAMEDECODECOMPLETE = 11,     ///< \copybrief vpu_nfy_framedecodecomplete
    VPU_DEVT_SEQUENCEHEADERUPDATE = 14,    ///< \copybrief vpu_nfy_sequenceheaderupdate
    VPU_DEVT_USERDATARECEIVED = 16,        ///< \copybrief vpu_nfy_userdatareceived
    VPU_DEVT_USERDATARECEIVEDPICTURE = 17, ///< \copybrief vpu_nfy_userdatareceivedpicture
    VPU_DEVT_ENDSEQUENCERECEIVED = 18,     ///< \copybrief vpu_nfy_endsequencereceived
    VPU_DEVT_DECODEERROR = 20,             ///< \copybrief vpu_nfy_decodeerror
    VPU_DEVT_SYSTEMERROR = 21,             ///< \copybrief vpu_nfy_systemerror
    VPU_DEVT_CHUNKDONE = 23,               ///< \copybrief vpu_nfy_chunkdone
    VPU_DEVT_STREAMCOMPLETE = 31,          ///< \copybrief vpu_nfy_streamcomplete
    // New event
    VPU_DEVT_BITBUFFERADDED = 24,          ///< \copybrief vpu_nfy_bitbufferadded
	VPU_DEVT_STREAMBITSNEED
    // TODO 
    //        VPU_DEVT_PVRCOMMANDDONE = 19,          ///< ?

    // return broken link/closed gop in vpu_picture_t with event FRAMEHEADERFOUND
    //        VPU_DEVT_GOPDECODED = 12,              ///< GOP header has been decoded from stream
    // return panscan info in vpu_picture_t with event FRAMEHEADERFOUND
    //VPU_DEVT_FRAMEPANSCANUPDATE = 13,      ///< Pan - scan data has been decoded.                              

    // not exported:
    //VPU_DEVT_SVCINITSEQUENCEHEADER = 15,
    //VPU_DEVT_DISPLAYCRCREADY = 22,
    //VPU_DEVT_INTERNAL_DECODERESET = 24,
    //VPU_DEVT_INTERNAL_HANDLERSTATECHANGE = 25,
    //VPU_DEVT_INTERNAL_FSALLOCATED = 26,
    //VPU_DEVT_INTERNAL_DEBUG = 27,
    //VPU_DEVT_PERF_DEBUG_LOOP = 28,
    //VPU_DEVT_INTERNAL_STREAMCONTROL = 29,
    //VPU_DEVT_INTERNAL_MALONEISR = 30,
    //VPU_DEVT_MEMREGION_ATTACHED = 31,
} vpu_decode_event_t;

// Forward declaration of structs
struct vpu_stream_buffer;
typedef struct vpu_stream_buffer vpu_stream_buffer_t;
struct vpu_sequence;
typedef struct vpu_sequence vpu_sequence_t;
struct vpu_picture;
typedef struct vpu_picture vpu_picture_t;
enum vpu_fw_err;
typedef enum vpu_fw_err vpu_fw_err_t;

/// Function \ref vpu_nfy_signal calls a function for the event; use this as a callback for \ref vpu_subscribe.
void vpu_nfy_signal(vpu_stream_id_t stream_id, vpu_decode_event_t decode_event, void* event_data, uint32_t event_data_size);

/// Notification \ref vpu_nfy_initialized indicates that the stream has been initialized and waiting for stream now.
void vpu_nfy_initialized(void);

/// Notification \ref vpu_nfy_terminated indicates that the stream has been terminated and a new start can be applied for the same stream ID.
void vpu_nfy_terminated(void); 

/// Notification \ref vpu_nfy_streamacquired reports that the stream level has gone non-zero after stream lost conditions.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamacquired(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamlostoverflow reports that stream overflow corruption of following decode.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamlostoverflow(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamloststarved reports that no stream update has been seen for the timeout period.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamloststarved(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamlwmhit indicates that the low water mark level has been reached.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamlwmhit(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamhwmhit indicates that the high water mark level has been reached; possibly overflowing.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamhwmhit(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamsyncacquired indicates that stream startcodes have been acquired.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamsyncacquired(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamsynclost indicates that no stream startcodes have been acquired within the timeout period.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] stream_buffer The current status of the stream buffer.
void vpu_nfy_streamsynclost(vpu_stream_id_t stream_id, vpu_stream_buffer_t* stream_buffer);

/// Notification \ref vpu_nfy_streamseqsync reports that the initial sequence header was found or a sequence header with resolution change was found.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] sequence The sequence header information retrieved from the stream.
void vpu_nfy_streamseqsync(vpu_stream_id_t stream_id, vpu_sequence_t* sequence);

/// Notification \ref vpu_nfy_frameheaderfound reports a new picture/frame header was found.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param[in] picture The picture information retrieved from the stream.
void vpu_nfy_frameheaderfound(vpu_stream_id_t stream_id, vpu_picture_t* picture);

/// Notification \ref vpu_nfy_framedecodecomplete Indicates that a frame has been fully decoded.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param[in] picture The picture information retrieved from the stream.
void vpu_nfy_framedecodecomplete(vpu_stream_id_t stream_id, vpu_picture_t* picture);

/// Notification \ref vpu_nfy_sequenceheaderupdate reports a new sequence header update after \ref VPU_DEVT_STREAMSEQSYNC event.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param[in] sequence The sequence header information retrieved from the stream.
void vpu_nfy_sequenceheaderupdate(vpu_stream_id_t stream_id, vpu_sequence_t* sequence);

/// Notification \ref vpu_nfy_userdatareceived reports that user data was received and packaged for a level above picture level.
/// \param [in] stream_id Identifies the stream the event applies to.
void vpu_nfy_userdatareceived(vpu_stream_id_t stream_id);

/// Notification \ref vpu_nfy_userdatareceivedpicture reports that user data received at the picture level; also signalled through the decode complete message.
/// \param [in] stream_id Identifies the stream the event applies to.
void vpu_nfy_userdatareceivedpicture(vpu_stream_id_t stream_id);

/// Notification \ref vpu_nfy_endsequencereceived indicates that an end of sequence startcode was found.
/// \param [in] stream_id Identifies the stream the event applies to.
void vpu_nfy_endsequencereceived(vpu_stream_id_t stream_id);

/// Notification \ref vpu_nfy_decodeerror returns a set of decode errors, particular to the codec type. 
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] error one of \ref vpu_h264_err_t (default),
///  \ref vpu_mp2_err_t, \ref vpu_vc1_err_t, \ref vpu_avs_err_t, \ref vpu_asp_err_t, \ref vpu_jpeg_err_t.                                                                                                 
void vpu_nfy_decodeerror(vpu_stream_id_t stream_id, uint32_t error);

/// Notification \ref vpu_nfy_systemerror reports general system errors.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] error The system error found.
void vpu_nfy_systemerror(vpu_stream_id_t stream_id, vpu_fw_err_t error);

/// Notification \ref vpu_nfy_chunkdone indicates that the JPEG decoding of chunk(s) is done.
/// \param [in] stream_id Identifies the stream the event applies to.
void vpu_nfy_chunkdone(vpu_stream_id_t stream_id);

/// Notification \ref vpu_nfy_streamcomplete notifies that the stream is complete.
/// \param [in] stream_id Identifies the stream the event applies to.
void vpu_nfy_streamcomplete(vpu_stream_id_t stream_id);                                          

/// Notification \ref vpu_nfy_bitbufferadded reports that the bitbuffer was added and memory can be reused.
/// \param [in] stream_id Identifies the stream the event applies to.
/// \param [in] buffer_id The buffer id of the buffer add that was completed.
void vpu_nfy_bitbufferadded(vpu_stream_id_t stream_id, vpu_buffer_id_t buffer_id);

#ifdef __cplusplus
}
#endif

#endif
