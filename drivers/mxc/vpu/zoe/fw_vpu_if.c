#include "vpu_if.h"

static vpu_pts_generate_callback vpu_pts_generate_callback_inst;

/// Struct \ref vpu_pts_info specifies field time and fraction of a field time.
typedef struct vpu_pts_info
{
    uint16_t field_time_; ///< one field time in 90kHz ticks
    uint16_t field_time_adjust_; ///< adjustment in 1/8ths of a 90kHz tick.
} vpu_pts_info_t;

static vpu_pts_info_t a_vpu_pts_info[] =
{
    { 0,    0 }, // VPU_FR_UNDEFINED
    { 1876, 7 }, // VPU_FR_23_97_HZ ; 1876.875
    { 1875, 0 }, // VPU_FR_24_HZ    ; 1875
    { 1800, 0 }, // VPU_FR_25_HZ    ; 1800
    { 1501, 4 }, // VPU_FR_29_97_HZ ; 1501.5
    { 1500, 0 }, // VPU_FR_30_HZ    ; 1500
    {  900, 0 }, // VPU_FR_50_HZ    ; 900
    {  750, 6 }, // VPU_FR_59_94_HZ ; 750.75
    {  750, 0 }, // VPU_FR_60_HZ    ; 750
    { 3000, 0 }, // VPU_FR_15_HZ    ; 3000
    { 3750, 0 }, // VPU_FR_12_HZ    ; 3750
    {    0, 0 }, // 
    {    0, 0 }, // 
    {    0, 0 }, // 
    {    0, 0 }, // 
    {    0, 0 }, // 
    {    0, 0 }, // 
};

uint32_t vpu_pts_field_time(vpu_frame_rate_t frame_rate, uint32_t* adjustment)
{
    *adjustment = a_vpu_pts_info[frame_rate].field_time_adjust_;
    return a_vpu_pts_info[frame_rate].field_time_;
}

///
uint64_t vpu_iterate_pts(uint64_t pts, uint32_t field_time, uint32_t field_adjust, uint32_t nr_field_times, uint32_t* adjustment_accumulator)
{
    uint64_t next_pts = pts;
    uint32_t next_adjust = *adjustment_accumulator + (field_adjust * nr_field_times);
    uint32_t total_time = nr_field_times * nr_field_times + next_adjust / 8;
    next_pts += total_time;
    *adjustment_accumulator = next_adjust - (next_adjust / 8) * 8;
    return next_pts;
}

void vpu_bind_pts_generate_callback(vpu_pts_generate_callback pts_generate_callback)
{
    vpu_pts_generate_callback_inst = pts_generate_callback;
}

void vpu_hevc_buffer_sizes(uint32_t width, uint32_t height, uint32_t is10bits,
    uint32_t* luma_width, uint32_t* luma_height, uint32_t* luma_size,
    uint32_t* chroma_width, uint32_t* chroma_height, uint32_t* chroma_size,
    uint32_t* colocated_width, uint32_t* colocated_height, uint32_t* colocated_size,
    uint32_t* buffer_size, uint32_t* alignment)
{
    uint32_t f = (is10bits ? 4 : 5);
    *luma_width = (width * 5 / f + 255) & ~255;
    *luma_height = (height + 127) & ~127;
    *chroma_width = (width * 5 / f + 255) & ~255;
    *chroma_height = (height / 2 + 127) & ~127;
    *colocated_width = (width / 2 + 255) & ~255;
    *colocated_height = (height / 8 + 127) & ~127;

    *luma_size = *luma_width * *luma_height; // is multiple of 32kB due to w 256 and h 128 alignment
    *chroma_size = *chroma_width * *chroma_height;
    *colocated_size = *colocated_width * *colocated_height;
    *buffer_size = *luma_size + *chroma_size + *colocated_size; // allocate base address to 32kB boundary
    *alignment = 32 * 1024; // allocate base address to 32kB boundary
}