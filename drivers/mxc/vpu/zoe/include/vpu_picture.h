#ifndef __VPU_PICTURE_H__
#define __VPU_PICTURE_H__

#include "zoe_types.h"

#ifdef __cplusplus
extern "C" {
#endif
///
///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// \page _vpu_picture VPU picture
///
/// Definition of a data type with functions describing a YUV picture.
///
/// \section _vpu_picture_structure VPU Picture Types
///
/// \copybrief vpu_sequence
///
/// \copybrief vpu_picture
///
/// \subsection _vpu_sequence_functions VPU sequence functions
///
/// \copybrief vpu_seq_valid_version
///
/// \copybrief vpu_seq_id
///
/// \copybrief vpu_seq_flags_are_set
///
/// \copybrief vpu_seq_flag_is_set
///
/// \copybrief vpu_display_wh
///
/// \copybrief vpu_decoded_wh
///
/// \copybrief vpu_frame_rate
///
/// \copybrief vpu_aspect_ratio
///
/// \copybrief vpu_color_space
///
/// \copybrief vpu_crop_top
///
/// \copybrief vpu_crop_bottom
///
/// \copybrief vpu_crop_left
///
/// \copybrief vpu_crop_right
///
/// \copybrief vpu_profile_level
///
/// \copybrief vpu_required_vbv_size
///
/// \copybrief vpu_required_nr_raw
///
/// \subsection _vpu_picture_functions VPU picture functions
///
/// \copybrief vpu_pic_valid_version
///
/// \copybrief vpu_seq_param_id
///
/// \copybrief vpu_buffer_id
///
/// \copybrief vpu_pic_flags_are_set
///
/// \copybrief vpu_pic_flag_is_set
///
/// \copybrief vpu_pic_address
///
/// \copybrief vpu_pic_number
///
/// \copybrief vpu_pan_scan_top
///
/// \copybrief vpu_pan_scan_bottom
///
/// \copybrief vpu_pan_scan_left
///
/// \copybrief vpu_pan_scan_right
///
/// \copybrief vpu_poc
///
/// \copybrief vpu_pts
///
/// \copybrief vpu_dts
///

/// The version number is contained in vpu_picture_t; version number should match
#define VPU_VERSION(major, minor)(0xDABA0000 | ((major) << 8) | (minor))
#define VPU_VERSION_NR VPU_VERSION(0, 1)

typedef uint64_t vpu_address_t;

enum {
	VPU_MAX_SEQ_ID = 16,          ///< Maximum sequence ID.
    VPU_MAX_DECODE_STREAMS = 16u  ///< \ref VPU_MAX_DECODE_STREAMS defines the maximum number of streams supported simultaneously.
};


/// Type \ref vpu_stream_id_t identifies streams with index 0 to \ref VPU_MAX_DECODE_STREAMS - 1
typedef uint8_t vpu_stream_id_t;

/// Type \ref vpu_seq_id_t indexes a sequence parameter set from 0 to VPU_MAX_SEQ_ID - 1 per stream.
typedef uint8_t vpu_seq_id_t;

/// Type \ref vpu_buftype_id_t classifies instances of picture buffers (yuv buffer).
typedef uint8_t vpu_buftype_id_t;

/// Type \ref vpu_buffer_id_t identifies an instance of a picture buffer (yuv buffer).
typedef uint8_t vpu_buffer_id_t;

typedef struct vpu_id
{
    vpu_stream_id_t  stream_id_;
    vpu_seq_id_t     seq_id_;
    vpu_buftype_id_t buftype_id_;
    vpu_buffer_id_t  buffer_id_;
} vpu_id_t;

/// Struct \ref vpu_position is an (x,y) position.
typedef struct vpu_position
{
    uint16_t x_;         ///< Horizontal component value of position.
    uint16_t y_;         ///< Vertical component value of position.
} vpu_position_t;

/// Struct \ref vpu_width_height specifies width and height information in pixels.
typedef struct vpu_width_height
{
    uint16_t width_;     ///< Width in pixels.
    uint16_t height_;    ///< Height in pixels.
} vpu_width_height_t;

/// Struct \ref vpu_width_height specifies width and height information in pixels. Can be used to define resolutions as well as aspect ratios.
typedef struct vpu_window
{
    uint16_t top_;      ///< Top offset in pixels.
    uint16_t bottom_;   ///< Bottom offset in pixels.
    uint16_t left_;     ///< Left offset in pixels.
    uint16_t right_;    ///< Right offset in pixels.
} vpu_window_t;

/// Enum \ref vpu_color_space defines the color spaces
typedef enum vpu_color_space
{
    VPU_CS_UNDEFINED = 0,  ///< Client to derive BT709 for HD or BT470 for SD image sizes
    VPU_CS_BT709 = 1,      ///<
    VPU_CS_UNSPECIFIED = 2,///< Client to derive BT709 for HD or BT470 for SD image sizes
    VPU_CS_470_2_M = 4,
    VPU_CS_470_2_BG = 5,
    VPU_CS_SMPTE_170M = 6,
    VPU_CS_SMPTE_240M = 7,
    VPU_CS_FILM = 7,
    VPU_CS_BT2020 = 8 /// HEVC4K output
} vpu_color_space_t;

/// Enum \ref vpu_transfer_char specifies the video transfer characteristics.
typedef enum vpu_transfer_char
{
    VPU_TC_UNDEFINED = 0
} vpu_transfer_char_;

/// Enum \ref vpu_matrix_coeffs specifies the matrix coefficients.
typedef enum vpu_matrix_coeffs
{
    VPU_MC_UNDEFINED = 0
} vpu_matrix_coeffs_t;

/// Enum \ref vpu_seq_flags specifies properties of a video sequence.
typedef enum vpu_seq_flags
{
    VPU_SEQ_PROGRESSIVE      = 0x00000001,  ///< If set it is a progressive sequence, if not interlaced sequence.
    VPU_SEQ_DECODE_ORDER     = 0x00000002,  ///< If set it is decode order sequence, if not presentation order.
    VPU_SEQ_CROPPING_PRESENT = 0x00000004,  ///< Indicates if cropping is present.
    VPU_SEQ_VUI_PRESENT      = 0x00000008,  ///< Indicates if cropping is present.
    VPU_SEQ_END = 0x20000000,  ///< The end of sequence indicator.
} vpu_seq_flags_t;

/// Enum \ref vpu_pic_flags specifies properties of a video picture.
typedef enum vpu_pic_flags
{
    VPU_PIC_TYPE_SHIFT = 0,     ///< [2:0] vpu_slice_t, vpu_mp2_frame_t, vpu_vc1_pic_t, vpu_avs_pic_t, vpu_asp_pic_t
    VPU_PIC_TYPE_NR_BITS = 3,  
    VPU_PIC_STRUCT_SHIFT = 3,     ///< [4:3] vpu_structure_t
    VPU_PIC_STRUCT_NR_BITS = 2,   

    VPU_PIC_REFERENCE         = 0x00000100, ///< The picture is used as reference frame.
    VPU_PIC_NON_PAIRED_FIELD  = 0x00000200, ///< The picture is a non paired field (H.264).
    VPU_PIC_PAN_SCAN          = 0x00000400, ///< Pan scan is enabled.
    VPU_PIC_CLOSED_GOP        = 0x00001000, ///< It is a closed GOP.
    VPU_PIC_BROKEN_LINK       = 0x00002000, ///< It is a broken link.
    VPU_PIC_PROGRESSIVE_FRAME = 0x00020000, ///< Its a progressive (non-interlaced) frame picture.
    VPU_PIC_ORIG_PTS          = 0x10000000, ///< The PTS has not been modified.
    VPU_PIC_SKIP              = 0x40000000, ///< The picture is to be skipped to meet specified output frame rate.
} vpu_pic_flags_t;

/// Enum \ref vpu_frame_rate defines the frame rate of a video sequence.
typedef enum vpu_frame_rate
{
    VPU_FR_UNDEFINED = 0,///<
    VPU_FR_23_97_HZ = 1, ///<
    VPU_FR_24_HZ = 2,    ///<
    VPU_FR_25_HZ = 3,    ///<
    VPU_FR_29_97_HZ = 4, ///<
    VPU_FR_30_HZ = 5,    ///<
    VPU_FR_50_HZ = 6,    ///<
    VPU_FR_59_94_HZ = 7, ///<
    VPU_FR_60_HZ = 8,    ///<
    VPU_FR_15_HZ = 9,    ///<
    VPU_FR_12_HZ = 10,   ///<
    VPU_FR_DIRECT_MASK = 0x80000000 ///< Coded as 0x80000nnn [3:0] frame rate value [5:4] Ext_n [10:6] Ext_d
} vpu_frame_rate_t;

/// Enum \ref vpu_slice defines the slice type of a slice of a h264/hevc video picture.
typedef enum vpu_slice
{
    VPU_SLICE_I = 0,    ///< 
    VPU_SLICE_P = 1,    ///<
    VPU_SLICE_B = 2,    ///<
    VPU_SLICE_SP = 3,   ///<
    VPU_SLICE_SI = 4,   ///<
    VPU_SLICE_IDR = 5   ///<
} vpu_slice_t;

/// Enum \ref vpu_mp2_frame defines the frame type of an mpeg2 video picture.
typedef enum vpu_mp2_frame
{
    VPU_MP2_I = 0,    ///<
    VPU_MP2_P = 1,    ///<
    VPU_MP2_B = 2,    ///<
    VPU_MP2_D = 3     ///<
} vpu_mp2_frame_t;


/// Enum \ref vpu_vc1_pic defines the frame type of a VC1 video picture.
typedef enum vpu_vc1_pic
{
    VPU_VC1_I = 0,    ///<
    VPU_VC1_P = 1,    ///<
    VPU_VC1_B = 2,    ///<
    VPU_VC1_SKIP = 3, ///<
    VPU_VC1_BI = 4    ///<
} vpu_vc1_pic_t;

/// Enum \ref vpu_avs_pic defines the frame type of an AVS video picture.
typedef enum vpu_avs_pic
{
    VPU_AVS_I = 1,    ///<
    VPU_AVS_P = 2,    ///<
    VPU_AVS_B = 3     ///<
} vpu_avs_pic_t;

/// Enum \ref vpu_asp_pic defines the frame type of an ASP video picture.
typedef enum vpu_asp_pic
{
    VPU_ASP_I = 1,    ///<
    VPU_ASP_P = 2,    ///<
    VPU_ASP_B = 3,    ///<
    VPU_ASP_S = 4,    ///<
    VPU_ASP_NC = 5    ///<
} vpu_asp_pic_t;

/// Enum \ref vpu_structure specifies top field first, field or frame.
typedef enum vpu_structure
{
    VPU_FIELD_TFF = 0, ///< Top field of a field stream
    VPU_FIELD_BFF = 1, ///< Bottom field of a field stream
    VPU_FRAME_BFF = 2, ///< Bottom field first frame.
    VPU_FRAME_TFF = 3  ///< Top field first frame.
} vpu_structure_t;


/// Struct \ref vpu_pic_timing contains timing information for a video frame.
typedef struct vpu_pic_timing
{
    uint32_t pic_number_;      ///< Picture number incremented by 1 for each picture (frame or field).
    uint32_t poc_;             ///< Picture order count from the stream.
    uint32_t nr_field_times_;  ///< The duration in number of field times.
    uint64_t pts_;             ///< The presentation timestamp, 90Khz.
    uint64_t dts_;             ///< The decode timestamp, 90Khz.
} vpu_pic_timing_t;

/// Enum \ref vpu_view contains 3D views information.
typedef struct vpu_view
{
    uint32_t t;    ///<
} vpu_view_t;

/// Enum \ref vpu_aspect_ratio defines the aspect ratio of a video sequence.
typedef enum vpu_aspect_ratio
{
    VPU_AR_UNDEFINED = 0,   ///<
    VPU_AR_1_1 = 1,         ///<
    VPU_AR_12_11 = 2,       ///<
    VPU_AR_10_11 = 3,       ///<
    VPU_AR_16_11 = 4,       ///<
    VPU_AR_40_33 = 5,       ///<
    VPU_AR_24_11 = 6,       ///<
    VPU_AR_20_11 = 7,       ///<
    VPU_AR_32_11 = 8,       ///<
    VPU_AR_88_33 = 9,       ///<
    VPU_AR_18_11 = 10,      ///<
    VPU_AR_15_11 = 11,      ///<
    VPU_AR_64_33 = 12,      ///<
    VPU_AR_160_99 = 13,     ///<
    VPU_AR_MP2_1_0 = 0x80,  ///<
    VPU_AR_MP2_4_3 = 0x81,  ///<
    VPU_AR_MP2_16_9 = 0x82, ///<
    VPU_AR_MP2_21_1 = 0x83, ///<
    VPU_AR_DIRECT_MASK = 0x80000000 ///< Coded as 0x8hhhXwww [11:0] width [27:16] height
} vpu_aspect_ratio_t;

/// Enum \ref vpu_fw_err defines a set of errors.
typedef enum vpu_fw_err
{
    VPU_FW_DECODEWDOG = 0,          ///<
    VPU_FW_PVRDECODEFAILURE = 1,    ///<
    VPU_FW_PESERROR = 2,            ///<
    VPU_FW_USERDATABUFFERFULL = 3,  ///<
    VPU_FW_DQFULL = 4,              ///<
    VPU_FW_STRBUFIDERROR = 5,       ///<
    VPU_FW_DQREQSTALLWDOG = 6,      ///<
    VPU_FW_DQRELEASESTALLWDOG = 7,  ///<
    VPU_FW_CHANNELSTOPPROBLEM = 8,  ///<
    VPU_FW_CHANNELSTARTPROBLEM = 9, ///<
    VPU_FW_DPBBUFAVAILEXCEED = 10,  ///<
    VPU_FW_SUPPORTNOTIMPLEMENTED = 11, ///<
    VPU_FW_MALONEISRMISMATCH = 12,  // MEANS WE GOT AN ISR ASSOCIATED WITH WRONG STREAM ID
    VPU_FW_DECODEHDRPARSERECOVERY = 13,   ///<
    VPU_FW_INSTREAMRESOLUTIONCHANGE = 15, ///<
    VPU_FW_PREPRODUCTIONHWISSUE = 16      ///<
} vpu_fw_err_t;

/// Enum \ref vpu_h264_err defines h.264 specific error codes.
typedef enum vpu_h264_err
{
    VPU_H264_UNEXPECTED_SC = 0,        ///<
    VPU_H264_ILLEGAL_SPS = 1,          ///<
    VPU_H264_UNSUPP_FORMAT = 2,        ///<
    VPU_H264_UNSUPP_PROFILE = 3,       ///<
    VPU_H264_ILLEGAL_SEQ_CHANGE = 4,   ///<
    VPU_H264_ILLEGAL_PPS = 5,          ///<
    VPU_H264_ILLEGAL_SLICE_HDR = 6,    ///<
    VPU_H264_ILLEGAL_SLICE = 7,        ///<
    VPU_H264_ILLEGAL_MB = 8,           ///<
    VPU_H264_SEQ_ERROR_CODE = 9,       ///<
    VPU_H264_ILLEGAL_SC = 10,          ///<
    VPU_H264_SEI = 11,                 ///<
    VPU_H264_FORCED_RESYNC = 12,       ///<
    VPU_H264_GAPS_IN_FRAMENUM = 13,    ///<
    VPU_H264_OLD_REF_FRAME_INLIST = 14 ///<
//    H264_ERROR_MVC_ILLEGALVIEW
//    H264_ERROR_ILLEGALSVCSUPPORT
} vpu_h264_err_t;

/// Enum \ref vpu_mp2_err defines mpeg2 specific error codes.
typedef enum vpu_mp2_err
{
    VPU_MP2_UNEXPECTED_SC = 0,     ///<
    VPU_MP2_ILLEGAL_SEQ_HDR = 1,   ///<
    VPU_MP2_UNSUPP_FORMAT = 2,     ///<
    VPU_MP2_UNSUPP_PROFILE = 3,    ///<
    VPU_MP2_ILLEGAL_SEQ_CHANGE = 4,///<
    VPU_MP2_ILLEGAL_PICT_HDR = 5,  ///<
    VPU_MP2_ILLEGAL_SLICE = 6,     ///<
    VPU_MP2_ILLEGAL_MB = 7,        ///<
    VPU_MP2_SEQ_ERROR_CODE = 8,    ///<
    VPU_MP2_ILLEGAL_SC = 9,        ///<
    VPU_MP2_USER_DATA = 10,        ///<
    VPU_MP2_ILLEGAL_GOP_HDR = 11,  ///<
    VPU_MP2_ILLEGAL_EXT_HDR = 12,  ///<
    VPU_MP2_GOP_BEFORE_2ND_FIELD = 13 ///<
//    VPU_MP2_SW_PES_PKT_ERROR = 14
} vpu_mp2_err_t;

/// Enum \ref vpu_vc1_err_t defines VC1 specific error codes.
typedef enum vpu_vc1_err_t
{
    VPU_VC1_UNEXPECTED_SC = 0,       ///<
    VPU_VC1_ILLEGAL_SEQ_HDR = 1,     ///<
    VPU_VC1_UNSUPP_FORMAT = 2,       ///<
    VPU_VC1_UNSUPP_PROFILE = 3,      ///<
    VPU_VC1_UNSUPP_SIZE = 4,         ///<
    VPU_VC1_ILLEGAL_ENTRY_POINT = 5, ///<
    VPU_VC1_ILLEGAL_PICT_HDR = 6,    ///<
    VPU_VC1_ILLEGAL_FIELD_HDR = 7,   ///<
    VPU_VC1_ILLEGAL_SLICE = 8,       ///<
    VPU_VC1_ILLEGAL_MB = 9,          ///<
    VPU_VC1_ILLEGAL_SC = 10,         ///<
    VPU_VC1_USER_DATA = 11,          ///<
    VPU_VC1_ILLEGAL_PES_HDR = 12     ///<
} vpu_vc1_err_t;

/// Enum \ref vpu_avs_err defines AVS specific error codes.
typedef enum vpu_avs_err
{
    VPU_AVS_UNEXPECTED_SC = 0,       ///<
    VPU_AVS_ILLEGAL_SEQ_HDR = 1,     ///<
    VPU_AVS_UNSUPP_FORMAT = 2,       ///<
    VPU_AVS_UNSUPP_PROFILE = 3,      ///<
    VPU_AVS_ILLEGAL_SEQ_CHANGE = 4,  ///<
    VPU_AVS_ILLEGAL_PICT_HDR = 5,    ///<
    VPU_AVS_ILLEGAL_SLICE = 6,       ///<
    VPU_AVS_ILLEGAL_MB = 7,          ///<
    VPU_AVS_SEQ_ERROR_CODE = 8,      ///<
    VPU_AVS_ILLEGAL_SC = 9,          ///<
    VPU_AVS_USER_DATA = 10,          ///<
    VPU_AVS_ILLEGAL_EXT_HDR = 11     ///<
} vpu_avs_err_t;

/// Enum \ref vpu_asp_err defines ASP specific error codes.
typedef enum vpu_asp_err
{
    VPU_ASP_UNEXPECTED_SC = 0,             ///<
    VPU_ASP_ENDOFGOBPARSEERROR = 1,        ///<
    VPU_ASP_VIDEOPACKETPARSEERROR = 2,     ///<
    VPU_ASP_ILLEGALSLICE = 3,              ///<
    VPU_ASP_USERDATAERROR = 4,             ///<
    VPU_ASP_BUGZILLA70_SUPPORTNOTDONE = 5, ///<
    VPU_ASP_ILLEGAL_PES_HDR = 6            ///<
} vpu_asp_err_t;

/// Enum \ref vpu_jpeg_err defines JPEG specific error codes.
typedef enum vpu_jpeg_err
{
    VPU_JPEG_UNEXPECTED_SC = 0,         ///<
    VPU_JPEG_UNSUPPORTED_SOF_TYPE = 1,  ///<
    VPU_JPEG_CORRUPT_IMAGE_HEADERS = 2, ///<
    VPU_JPEG_CORRUPT_SCAN = 3           ///<
} vpu_jpeg_err_t;

/// Enum \ref vpu_buffer_type classifies the buffers that are to be registered.
typedef enum vpu_buffer_type
{
	VPU_FRAME_BUFFER = 0,
	VPU_REFERENCE_BUFFER = 1,
	VPU_DECIMATED_BUFFER = 2,
    VPU_MAX_TYPES_OF_BUFFERS = 3 
} vpu_buffer_type_t;

/// Struct vpu_vbv constains settings for the VBV size and VBV delay.
typedef struct vpu_vbv 
{
	uint32_t            vbv_size_;  ///< VBV size of the output buffer. OUTPUT parameter
	uint32_t			vbv_delay_; ///< VBV delay for the input buffer.
} vpu_vbv_t;

/// Enum \ref vpu_pixel_format contains information on how pixels are laid out in memory; whether is is tiled, it has 10 bit pixels, if it is planar and so on.
/// The output of the VPU decoders is a tiled format. Each tile is 8 bytes wide by 128 bytes high. A tile is laid out in memory linearly.
/// For 8 bit pixels the tile is 8 pixels wide. For 10 bit pixels, the pixels will straddle the tile boundary into the next tile in the x direction.
/// 10 bit pixels are packed. 4 pixels are contained in 5 bytes. The left most pixel is in byte 0 and the two highest bits of byte 1. The right most pixel is
/// in byte 4 and the two lowest bit of byte 3.
/// The first 4 pixels are in the first 5 bytes of the first tile. The next 4 pixels are in the 3 remaining bytes of the first tile and the first two bytes of 
/// the next tile. The next four pixels are in the second tile. The next four pixels are in the last byte of the second tile and first four bytes of the 
/// third tile. The next four pixels are in the last four bytes of the third tile and the first byte of the fourth tile.
/// The next four pixels are in the fourth tile. The next four pixels are in the last two bytes of the fourth tile and the first three bytes of the fifth tile.
/// The next four pixels are the remainig 5 bytes in the fifth tile. This completes the pattern. Basically 5 tiles of bytes contain 4 tiles of pixels.
///
/// \image html ten_bit_pixel.svg
///
typedef enum vpu_pixel_format
{
    VPU_HAS_COLOCATED = 0x00000001, ///< Last component has colocated data (H.264/H.265)
    VPU_HAS_SPLIT_FLD = 0x00000002, ///< Frame is split in top and bottom fields; doubles YUV components.
    VPU_PF_MASK = ~(VPU_HAS_COLOCATED | VPU_HAS_SPLIT_FLD), ///< Flags that can apply to any format are to be masked out by AND'ing to VPU_PF_MASK

    VPU_IS_TILED = 0x000000100, ///< Tiling is enabled
    VPU_HAS_10BPP = 0x00000200, ///< 10 Bits per pixel

    VPU_IS_PLANAR = 0x00001000, ///< A plane for Y, U and V
    VPU_IS_SEMIPLANAR = 0x00002000, ///< A plane for Y and a plane for both U and V
    VPU_IS_PACKED = 0x00004000, ///< A plane for Y U and V


    // Merged definitions using above flags:
    VPU_PF_UNDEFINED = 0, ///<  INDICATES UNSPECIFIED.
    VPU_PF_YUV420_SEMIPLANAR = 0x00010000 | VPU_IS_SEMIPLANAR, ///< Semiplanar buffer with Y and UV components.
    VPU_PF_YUV420_PLANAR = 0x00020000 | VPU_IS_PLANAR, ///< Planar buffer with Y, U and V components.
    VPU_PF_UYVY = 0x00040000 | VPU_IS_PACKED, ///< Interleaved UYVY 8 bits per pixel per component.
    VPU_PF_TILED_8BPP = 0x00080000 | VPU_IS_TILED | VPU_IS_SEMIPLANAR, ///< 8x128 tiles
    VPU_PF_TILED_10BPP = 0x00100000 | VPU_IS_TILED | VPU_IS_SEMIPLANAR | VPU_HAS_10BPP ///< 8x128 tiles with pixel spilling into neighboring tile in x direction
} vpu_pixel_format_t;

/// Struct vpu_vbv constains settings for the VBV size and VBV delay.
typedef struct vpu_raw_spec
{
    uint32_t            required_nr_raw_;  ///< Number of raw buffers needed.
    vpu_pixel_format_t  required_pixel_format_; ///< Pixel format of the raw buffers
} vpu_raw_spec_t;


#include "vpu_picture.inl"

//*********************************************************************************************************************
// Sequence functions.
//*********************************************************************************************************************
//
/// Function \ref vpu_seq_version_nr returns version number that should match VPU_VERSION_NR.
STATIC_INLINE uint32_t vpu_seq_version_nr(const vpu_sequence_t* seq)
{
    return seq->version_nr_;
}

/// Function \ref vpu_seq_valid_version validates integrity of the picture struct - and whether it matches
/// this header file.
STATIC_INLINE uint32_t vpu_seq_valid_version(const vpu_sequence_t* seq)
{
    return seq->version_nr_ == VPU_VERSION_NR;
}

/// Function \ref vpu_seq_id returns the sequence id.
STATIC_INLINE vpu_seq_id_t vpu_seq_id(const vpu_sequence_t* seq)
{
    return seq->seq_id_;
}

/// Function \ref vpu_seq_init initializes the sequence id and version number.
STATIC_INLINE void vpu_seq_init(vpu_sequence_t* seq, vpu_seq_id_t seq_id)
{
    seq->version_nr_ = VPU_VERSION_NR;
    seq->seq_id_ = seq_id;
}

/// Function \ref vpu_seq_flags returns the sequence flags.
STATIC_INLINE uint32_t vpu_seq_flags(const vpu_sequence_t* seq)
{
    return seq->seq_flags_;
}

/// Function \ref vpu_seq_flags_are_set returns true when all flags are set.
STATIC_INLINE uint32_t vpu_seq_flags_are_set(const vpu_sequence_t* seq, uint32_t flags)
{
    return (seq->seq_flags_ & flags) == flags;
}

/// Function \ref vpu_seq_flag_is_set returns true when a flag is set.
STATIC_INLINE uint32_t vpu_seq_flag_is_set(const vpu_sequence_t* seq, vpu_seq_flags_t flag)
{
    return vpu_seq_flags_are_set(seq, flag);
}

/// Function \ref vpu_seq_flags_set sets the sequence flags.
STATIC_INLINE void vpu_seq_flags_set(vpu_sequence_t* seq, uint32_t flags)
{
    seq->seq_flags_ = flags;
}

/// Function \ref vpu_display_wh returns display width and height of the picture.
STATIC_INLINE vpu_width_height_t vpu_display_wh(const vpu_sequence_t* seq)
{
    return seq->display_wh_;
}

/// Function \ref vpu_display_wh_set sets display width and height of the picture.
STATIC_INLINE void vpu_display_wh_set(vpu_sequence_t* seq, vpu_width_height_t display_wh)
{
    seq->display_wh_ = display_wh;
}

/// Function \ref vpu_decoded_wh returns decoded width and height of the picture.
STATIC_INLINE vpu_width_height_t vpu_decoded_wh(const vpu_sequence_t* seq)
{
    return seq->decoded_wh_;
}

/// Function \ref vpu_decoded_wh_set sets decoded width and height of the picture.
STATIC_INLINE void vpu_decoded_wh_set(vpu_sequence_t* seq, vpu_width_height_t decoded_wh)
{
    seq->decoded_wh_ = decoded_wh;
}

/// Function \ref vpu_frame_rate returns the frame rate of the picture.
STATIC_INLINE vpu_frame_rate_t vpu_frame_rate(const vpu_sequence_t* seq)
{
    return seq->frame_rate_;
}

/// Function \ref vpu_frame_rate sets the frame rate of the picture.
STATIC_INLINE void vpu_frame_rate_set(vpu_sequence_t* seq, vpu_frame_rate_t frame_rate)
{
    seq->frame_rate_ = frame_rate;
}

/// Function \ref vpu_aspect_ratio returns the aspect ratio of the picture.
STATIC_INLINE vpu_aspect_ratio_t vpu_aspect_ratio(const vpu_sequence_t* seq)
{
    return seq->aspect_ratio_;
}

/// Function \ref vpu_aspect_ratio_set sets the aspect ratio of the picture.
STATIC_INLINE void vpu_aspect_ratio_set(vpu_sequence_t* seq, vpu_aspect_ratio_t aspect_ratio)
{
    seq->aspect_ratio_ = aspect_ratio;
}

/// Function \ref vpu_color_space returns the color space of the picture (601 YCbCr or 709 YCbCr).
STATIC_INLINE vpu_color_space_t vpu_color_space(const vpu_sequence_t* seq)
{
    return seq->color_space_;
}

/// Function \ref vpu_color_space_set sets the color space of the picture (601 YCbCr or 709 YCbCr).
STATIC_INLINE void vpu_color_space_set(vpu_sequence_t* seq, vpu_color_space_t color_space)
{
    seq->color_space_ = color_space;
}

/// Function \ref vpu_crop_top returns the cropping at the top of the image.
STATIC_INLINE uint32_t vpu_crop_top(const vpu_sequence_t* seq)
{
    return seq->crop_.top_;
}

/// Function \ref vpu_crop_bottom returns the cropping at the bottom of the image.
STATIC_INLINE uint32_t vpu_crop_bottom(const vpu_sequence_t* seq)
{
    return seq->crop_.bottom_;
}

/// Function \ref vpu_crop_left returns the cropping at the left of the image.
STATIC_INLINE uint32_t vpu_crop_left(const vpu_sequence_t* seq)
{
    return seq->crop_.left_;
}

/// Function \ref vpu_crop_right returns the cropping at the right of the image.
STATIC_INLINE uint32_t vpu_crop_right(const vpu_sequence_t* seq)
{
    return seq->crop_.right_;
}

/// Function \ref vpu_crop_set sets the cropping of the image.
STATIC_INLINE  void vpu_crop_set(vpu_sequence_t* seq, uint32_t top, uint32_t bottom, uint32_t left, uint32_t right)
{
    seq->crop_.top_ = top;
    seq->crop_.bottom_ = bottom;
    seq->crop_.left_ = left;
    seq->crop_.right_ = right;
}

/// Function \ref vpu_profile_level returns the video standard dependent profile.
STATIC_INLINE uint32_t vpu_profile_level(const vpu_sequence_t* seq)
{
    return seq->profile_level_;
}

/// Function \ref vpu_profile_level_set sets the video standard dependent profile.
STATIC_INLINE void vpu_profile_level_set(vpu_sequence_t* seq, uint32_t profile_level)
{
    seq->profile_level_ = profile_level;
}

/// Function \ref vpu_required_vbv_size returns the required VBV size. Enough buffers need to be supplied
/// totalling to this size for the correct functioning a decoder.
STATIC_INLINE uint32_t vpu_required_vbv_size(const vpu_sequence_t* seq)
{
    return seq->required_vbv_buffer_.vbv_size_;
}

/// Function \ref vpu_required_vbv_delay returns the required VBV delay. Enough buffers need to be supplied
/// totalling to this size for the correct functioning a decoder.
STATIC_INLINE uint32_t vpu_required_vbv_delay(const vpu_sequence_t* seq)
{
	return seq->required_vbv_buffer_.vbv_delay_;
}

/// Function \ref vpu_required_vbv_set sets the required VBV size and delay. 
STATIC_INLINE void vpu_required_vbv_set(vpu_sequence_t* seq, uint32_t vbv_size, uint32_t vbv_delay)
{
    seq->required_vbv_buffer_.vbv_size_ = vbv_size;
    seq->required_vbv_buffer_.vbv_delay_ = vbv_delay;
}

/// Function \ref vpu_required_nr_raw returns the required number of raw picture buffers to meet the profile/level
/// constraints.
STATIC_INLINE uint32_t vpu_required_nr_raw(const vpu_sequence_t* seq, vpu_buffer_type_t buffer_type)
{
    return seq->required_nr_raw_[buffer_type].required_nr_raw_;
}

/// Function \ref vpu_required_nr_raw_set sets the required number of raw picture buffers to meet the profile/level
/// constraints.
STATIC_INLINE void vpu_required_nr_raw_set(vpu_sequence_t* seq, vpu_buffer_type_t buffer_type, uint32_t nr_raw)
{
    seq->required_nr_raw_[buffer_type].required_nr_raw_ = nr_raw;
}

/// Function \ref vpu_required_pixel_format returns the required pixel format of raw picture buffers.
/// constraints.
STATIC_INLINE uint32_t vpu_required_pixel_format(const vpu_sequence_t* seq, vpu_buffer_type_t buffer_type)
{
    return seq->required_nr_raw_[buffer_type].required_pixel_format_;
}

/// Function \ref vpu_required_pixel_format_set sets the required number of the required pixel format of raw picture buffers.
/// constraints.
STATIC_INLINE void vpu_required_pixel_format_set(vpu_sequence_t* seq, vpu_buffer_type_t buffer_type, vpu_pixel_format_t pixel_format)
{
    seq->required_nr_raw_[buffer_type].required_pixel_format_ = pixel_format;
}

//*********************************************************************************************************************
// Picture functions.
//*********************************************************************************************************************
//

/// Function \ref vpu_pic_version_nr returns version number that should match VPU_VERSION_NR.
STATIC_INLINE uint32_t vpu_pic_version_nr(const vpu_picture_t* pic)
{
    return pic->version_nr_;
}

/// Function \ref vpu_pic_valid_version validates integrity of the picture struct - and whether it matches
/// this header file.
STATIC_INLINE uint32_t vpu_pic_valid_version(const vpu_picture_t* pic)
{
    return pic->version_nr_ == VPU_VERSION_NR;
}

/// Function \ref vpu_same_id returns 1 if the picture ids matches the provided ids.
STATIC_INLINE int32_t vpu_same_id(const vpu_picture_t* pic, vpu_id_t id)
{
    int32_t r = 0;
    if(pic) {
        r = 1;
        r &= pic->id_.stream_id_ == id.stream_id_;
        r &= pic->id_.seq_id_ == id.seq_id_;
        r &= pic->id_.buftype_id_ == id.buftype_id_;
        r &= pic->id_.buffer_id_ == id.buffer_id_;
    }
    return r;
}

/// Function \ref vpu_id_set sets the ids of the picture.
STATIC_INLINE void vpu_id_set(vpu_picture_t* pic, vpu_id_t id)
{
    pic->id_ = id;
}

/// Function \ref vpu_pic_init sets the ids of the picture, and the version number.
STATIC_INLINE void vpu_pic_init(vpu_picture_t* pic, vpu_stream_id_t stream_id,  
    vpu_seq_id_t seq_id, vpu_buftype_id_t buftype_id, vpu_buffer_id_t buffer_id)
{
    pic->version_nr_ = VPU_VERSION_NR;
    pic->id_.stream_id_ = stream_id;
    pic->id_.seq_id_ = seq_id;
    pic->id_.buftype_id_ = buftype_id;
    pic->id_.buffer_id_ = buffer_id;
}

/// Function \ref vpu_seq_param_id returns the sequence parameter set id that this picture refers to.
STATIC_INLINE vpu_stream_id_t vpu_stream_id(const vpu_picture_t* pic)
{
    return pic->id_.stream_id_;
}

/// Function \ref vpu_seq_param_id returns the sequence parameter set id that this picture refers to.
STATIC_INLINE vpu_seq_id_t vpu_seq_param_id(const vpu_picture_t* pic)
{
    return pic->id_.seq_id_;
}

/// Function \ref vpu_buffer_id returns the sequence parameter set id that this picture refers to.
STATIC_INLINE vpu_buftype_id_t vpu_buftype_id(const vpu_picture_t* pic)
{
    return pic->id_.buftype_id_;
}

/// Function \ref vpu_buffer_id returns the sequence parameter set id that this picture refers to.
STATIC_INLINE vpu_buffer_id_t vpu_buffer_id(const vpu_picture_t* pic)
{
    return pic->id_.buffer_id_;
}

/// Function \ref vpu_pic_flags returns picture flags value.
STATIC_INLINE uint32_t vpu_pic_flags(const vpu_picture_t* pic)
{
    return pic->pic_flags_;
}

/// Function \ref vpu_structure_t returns picture structure.
STATIC_INLINE vpu_structure_t vpu_pic_structure(const vpu_picture_t* pic)
{
    return (vpu_structure_t)((pic->pic_flags_ >> VPU_PIC_STRUCT_SHIFT) & ((1 << VPU_PIC_STRUCT_NR_BITS) - 1));
}

/// Function \ref vpu_pic_type returns picture type being vpu_slice_t, vpu_mp2_frame_t, vpu_vc1_pic_t, vpu_avs_pic_t, vpu_asp_pic_t.
STATIC_INLINE uint32_t vpu_pic_type(const vpu_picture_t* pic)
{
    return (pic->pic_flags_ >> VPU_PIC_TYPE_SHIFT) & ((1 << VPU_PIC_TYPE_NR_BITS) - 1);
}

/// Function \ref vpu_pic_flags_are_set returns true when all flags are set.
STATIC_INLINE uint32_t vpu_pic_flags_are_set(const vpu_picture_t* pic, uint32_t flags)
{
    return (pic->pic_flags_ & flags) == flags;
}

/// Function \ref vpu_pic_flag_is_set returns true when a flag is set.
STATIC_INLINE uint32_t vpu_pic_flag_is_set(const vpu_picture_t* pic, vpu_pic_flags_t flag)
{
    return vpu_pic_flags_are_set(pic, flag);
}

/// Function \ref vpu_pic_type_init sets up picture type structure and flags.
STATIC_INLINE void vpu_pic_type_init(vpu_picture_t* pic, uint32_t pic_type, vpu_structure_t structure, uint32_t flags)
{
    pic->pic_flags_ = pic_type << VPU_PIC_TYPE_SHIFT | structure << VPU_PIC_STRUCT_SHIFT | flags;
}

///< Function \ref vpu_pic_address returns the address of the picture in the input stream buffer.
STATIC_INLINE vpu_address_t vpu_pic_address(const vpu_picture_t* pic)
{
    return pic->pic_address_;
}

///< Function \ref vpu_pic_address_set sets the address of the picture in the input stream buffer.
STATIC_INLINE void vpu_pic_address_set(vpu_picture_t* pic, vpu_address_t address)
{
    pic->pic_address_ = address;
}

/// Function \ref vpu_pic_number returns the picture number; increases by 1 for each picture (frame or field)
STATIC_INLINE uint32_t vpu_pic_number(const vpu_picture_t* pic)
{
    return pic->pic_timing_.pic_number_;
}

/// Function \ref vpu_pic_number_set sets the picture number; must increases by 1 for each picture (frame or field)
STATIC_INLINE void vpu_pic_number_set(vpu_picture_t* pic, uint32_t pic_number)
{
    pic->pic_timing_.pic_number_ = pic_number;
}

/// Function \ref vpu_poc the video standard dependent POC of the picture.
STATIC_INLINE uint32_t vpu_poc(const vpu_picture_t* pic)
{
    return pic->pic_timing_.poc_;
}

/// Function \ref vpu_poc_set sets the video standard dependent POC of the picture.
STATIC_INLINE void vpu_poc_set(vpu_picture_t* pic, uint32_t poc)
{
    pic->pic_timing_.poc_ = poc;
}

/// Function \ref vpu_nr_field_times is the number of field times the picture is to be presented.
STATIC_INLINE uint32_t vpu_nr_field_times(const vpu_picture_t* pic)
{
    return pic->pic_timing_.nr_field_times_;
}

/// Function \ref vpu_nr_field_times_set sets the number of field times the picture is to be presented.
STATIC_INLINE void vpu_nr_field_times_set(vpu_picture_t* pic, uint32_t nr_field_times)
{
    pic->pic_timing_.nr_field_times_ = nr_field_times;
}

/// Function \ref vpu_pts returns the PTS of the picture.
STATIC_INLINE uint64_t vpu_pts(const vpu_picture_t* pic)
{
    return pic->pic_timing_.pts_;
}

/// Function \ref vpu_pts_set sets the PTS of the picture.
STATIC_INLINE void vpu_pts_set(vpu_picture_t* pic, uint64_t pts)
{
    pic->pic_timing_.pts_ = pts;
}

/// Function \ref vpu_dts returns the DTS of the picture.
STATIC_INLINE uint64_t vpu_dts(const vpu_picture_t* pic)
{
    return pic->pic_timing_.dts_;
}

/// Function \ref vpu_dts_set sets the DTS of the picture.
STATIC_INLINE void vpu_dts_set(vpu_picture_t* pic, uint64_t dts)
{
    pic->pic_timing_.dts_ = dts;
}

/// Function \ref vpu_pan_scan_top returns the pan-scan offset at the top of the image.
STATIC_INLINE uint32_t vpu_pan_scan_top(const vpu_picture_t* pic)
{
    return pic->pan_scan_.top_;
}

/// Function \ref vpu_pan_scan_bottom returns the pan-scan offset at the bottom of the image.
STATIC_INLINE uint32_t vpu_pan_scan_bottom(const vpu_picture_t* pic)
{
    return pic->pan_scan_.bottom_;
}

/// Function \ref vpu_pan_scan_left returns the pan-scan offset at the left of the image.
STATIC_INLINE uint32_t vpu_pan_scan_left(const vpu_picture_t* pic)
{
    return pic->pan_scan_.left_;
}

/// Function \ref vpu_pan_scan_right returns the pan-scan offset at the right of the image.
STATIC_INLINE uint32_t vpu_pan_scan_right(const vpu_picture_t* pic)
{
    return pic->pan_scan_.right_;
}

/// Function \ref vpu_err_address returns the buffer address at which the error occurred.
STATIC_INLINE vpu_address_t vpu_err_address(const vpu_picture_t* pic)
{
    return pic->err_address_;
}

/// Function \ref vpu_err_types returns a mask of the error types.
STATIC_INLINE uint32_t vpu_err_types(const vpu_picture_t* pic)
{
    return pic->err_types_;
}


#ifdef __cplusplus
}
#endif

#endif
