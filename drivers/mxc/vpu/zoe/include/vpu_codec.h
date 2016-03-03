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

#ifndef __VPU_CODEC_H__
#define __VPU_CODEC_H__

#include "vpu_picture.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Enum \ref vpu_ret defines common return codes for video functions; VPU_RET_OK specifies successful completion.
typedef enum vpu_ret
{
    VPU_RET_OK = 0,    ///< The function was executed successfully.
    VPU_RET_ERROR = 1,    ///< An unknown issue was encountered.
    VPU_RET_OUT_OF_MEMORY = 2,    ///< An issue was encountered trying to allocate memory.
    VPU_RET_OUT_OF_BUFFERS = 3,    ///< The function ran out of buffers.
    VPU_RET_NONFATAL = 4,    ///< A non-fatal recoverable issue occurred.
    VPU_RET_FATAL = 5,    ///< A fatal issue occurred - reset may be the only option.
    VPU_RET_INVALID_PARAM = 6,    ///< A paramter that was passed is not valid.
    VPU_RET_INVALID_HANDLE = 7,    ///< A passed handle is not valid.
    VPU_RET_INVALID_BUFFER = 8,    ///< The specified buffer is not valid.
    VPU_RET_BUFFER_IN_USE = 9,    ///< The specified buffer is in use still.
    VPU_RET_NOT_SUPPORTED = 10,    ///< The function is not supported in the context in which it is called.
    VPU_RET_PRECONDITION = 11    ///< A precondition was violated.
} vpu_ret_t;


/// Type \ref vpu_data_buffer_id_t identifies an instance of a data buffer; a buffer signifying a chunk of compressed video data.
typedef uint8_t vpu_data_buffer_id_t;

/// Enum \ref vpu_component describes a single component of a \ref vpu_buffer; each component has an address, offset for
/// alignment, stride and number of bytes.
typedef struct vpu_component
{
	vpu_address_t address_;       ///< The address in memory of the component.
	uint32_t      offset_;        ///< The offset from the address for the start of the pixel data. Alignment value.
	uint32_t      stride_;        ///< The stride in number of bytes.
	uint32_t      nr_of_bytes_;   ///< The number of bytes in memory of the component(includes bytes between address and offset).
} vpu_component_t;


/// Struct \ref vpu_buffer represents yuv/raw buffers consisting of a number of \ref vpu_component; it is uniquely identified 
/// by \ref vpu_stream_id_t and \ref vpu_buffer_id_t.
typedef struct vpu_buffer
{
	vpu_id_t           id_; ///< Unique identification of the buffer
    uint32_t           nr_of_components_; ///< Actual number of components used.
	vpu_pixel_format_t pixel_format_;     ///< The pixel format of the components
	vpu_component_t    component_[8];     ///< Address/size/stride of each component (0 if not in use)
} vpu_buffer_t;

STATIC_INLINE vpu_pixel_format_t vpu_pixel_format(vpu_buffer_t* buffer)
{
    return (vpu_pixel_format_t)((buffer->pixel_format_) & VPU_PF_MASK);
}

STATIC_INLINE uint32_t vpu_has_colocated(vpu_buffer_t* buffer)
{
    return((buffer->pixel_format_) & VPU_HAS_COLOCATED);
}

STATIC_INLINE uint32_t vpu_has_split_fld(vpu_buffer_t* buffer)
{
    return((buffer->pixel_format_) & VPU_HAS_SPLIT_FLD);
}

/// Struct \ref vpu_data_buffer specifies a data segment in memory containing compressed data; it is uniquely
/// identified by its \ref vpu_stream_id_t and \ref vpu_data_buffer_id_t members.
typedef struct vpu_data_buffer
{
	vpu_stream_id_t      stream_id_;
	vpu_data_buffer_id_t data_buffer_id_;
	vpu_address_t        address_;           ///< Address in memory of the compressed data.
	uint32_t             nr_of_bytes_;       ///< Number of bytes.
} vpu_data_buffer_t;


// Helper functions that are locally defined.
/// Function \ref vpu_pts_field_time returns the time in 90kHz ticks of one field time.
/// \param[in] frame_rate The specified frame rate.
/// \param[out] adjustment The fraction of a 90kHz tick to get to an accurate value expressed in 1/8th of a tick.
/// \returns The field time associated with the specified frame rate (so half a frame time).
uint32_t vpu_field_time(vpu_frame_rate_t frame_rate, uint32_t* adjustment);

/// Function \ref vpu_iterate_pts calculates a next pts from the specified pts by adding specified amount of field times while taking into 
/// acount adjustments needed.
/// \param[in] pts          Input pts to which the specified number of field times will be added.
/// \param[in] field_time   Field time in number of 90kHz ticks.
/// \param[in] field_adjust The fraction of a 90kHz tick to get to an accurate value expressed in 1/8th of a tick.
/// \param[in] nr_field_times The number of field times to be added
/// \param[inout] adjustment_accumulator The unaccounted fractions of a tick that will be carried forward for a next iteration.
uint64_t vpu_iterate_pts(uint64_t pts, uint32_t field_time, uint32_t field_adjust, uint32_t nr_field_times, uint32_t* adjustment_accumulator);


#ifdef __cplusplus
}
#endif

#endif
