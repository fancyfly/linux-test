
//*********************************************************************************************************************
// Sequence and picture type.
//*********************************************************************************************************************
//

/// Struct \ref vpu_sequence contains sequence meta data; DONOT use this struct directly; use \ref 
/// _vpu_sequence_functions.
typedef struct vpu_sequence
{
    uint32_t version_nr_; ///< version_nr_ must match VPU_VERSION_NR for the firmware and client to be in sync.
    vpu_seq_id_t seq_id_;  ///< ID of this sequence parameter set.

    /*sequence header data*/
    uint32_t            seq_flags_;     ///< OR'd \ref vpu_seq_flags_t
    vpu_width_height_t  display_wh_;    ///< The display width/height in pixels.
    vpu_width_height_t  decoded_wh_;    ///< The decoded width/height in pixels.
    vpu_frame_rate_t    frame_rate_;    ///< Coded framerate as enum value or direct coding.
    vpu_aspect_ratio_t  aspect_ratio_;  ///< Coded framerate as enum value or direct coding.
    vpu_color_space_t   color_space_;   ///< The color space.
    vpu_window_t        crop_;          ///< The amount of pixels to crop from the top, bottom, left and right
    uint32_t            profile_level_; ///< The standard dependent profile/level;

    /* Required sizes/amount of buffers */
    vpu_vbv_t           required_vbv_buffer_;  ///< Required specification of the input buffer for decoder and output buffer for encoder.
    vpu_raw_spec_t      required_nr_raw_[VPU_MAX_TYPES_OF_BUFFERS];    ///< Required number of raw picture buffers and pixel format.

} vpu_sequence_t;

/// Struct \ref vpu_picture contains a raw input picture meta data; DONOT use this struct directly; use \ref 
/// _vpu_picture_functions.
typedef struct vpu_picture
{
    uint32_t version_nr_;    ///< version_nr_ must match VPU_VERSION_NR for the firmware and client to be in sync.
    vpu_id_t id_;            ///< Sequence parameter set id of the sequence parameter set that applies to this picture
    ///< Buffer type id, buffer id and stream id of the stream it belongs to.
    /*picture data*/
    uint32_t           pic_flags_;  ///< OR'd \ref vpu_pic_flags_t, vpu_slice_t, vpu_structure_t
    vpu_address_t      pic_address_; ///< Address of the picture in the input stream buffer.
    vpu_pic_timing_t   pic_timing_; ///< pts/dts
    vpu_window_t       pan_scan_;   ///< Pan scan window.
    vpu_address_t      err_address_; ///< Address of the error in the input stream buffer.
    uint32_t           err_types_; ///< Mask of error types seen during frame decode
    vpu_view_t         view_; ///< multiview information.
} vpu_picture_t;
