//HEADER
#ifndef __VDEC_LOGO_H__
#define __VDEC_LOGO_H__

#include "vdec_if.h"
//#include "vhw_vpu.h"

#include "freescale_logo.h"

/// Class c_vdec_logo implements YUV generator that puts a moving LOGO on the screen.
/// \copydoc c_vdec_if
class c_vdec_logo : public c_vdec_if
{
    friend class c_vsys_one;
    c_vdec_logo();

    vpu_sequence_t current_sequence_;
    vpu_picture_t current_picture_;
    freescale_picture_t** picture_pool_;

    void init_current_picture();
    freescale_picture_t* pic_;
    int                  semiplanar;
    enum { OPEN, CLOSING, CLOSED };
    volatile uint32_t open_state;
    volatile bool compute_next_picture;
    bool seq_sent_;

    bool is_open()
    {
        return open_state == OPEN;
    }

    volatile uint32_t nr_pictures_to_produce; ///< Will produce this amount of YUV pictures with logo.
    volatile uint32_t nr_pictures_produced; ///< Actual amount of pictures with logo produced.

    static const uint32_t MAX_REGISTERED_BUFFERS = 4;
    vpu_buffer_t          registered_buffers[MAX_REGISTERED_BUFFERS];
    bool                  registered_buffer_in_use[MAX_REGISTERED_BUFFERS];
    bool                  registered_buffer_sent[MAX_REGISTERED_BUFFERS];
    vpu_buffer_t*         find_buffer_by_id(uint32_t buffer_id, uint32_t& idx);

    int    x_;
    int    y_;
    int    delta_x_;
    int    delta_y_;

    static const uint32_t MAX_BIT_BUFFERS = 32; // Must be power of 2.
    vpu_data_buffer_t     bitbuffers[MAX_BIT_BUFFERS ];
    uint32_t              bitbuffer_r; ///< Read index into bitbuffers.
    uint32_t              bitbuffer_w; ///< Write index into bitbuffers.
    void reset_buffers(); ///< Reset arrays of buffers.
    bool bitbuffer_is_empty()
    {
        return bitbuffer_r == bitbuffer_w;
    }

    uint32_t bitbuffer_inc(uint32_t rw)
    {
        return (rw + 1) & (MAX_BIT_BUFFERS - 1);
    }

    uint32_t bitbuffer_nr_elem()
    {
        return (bitbuffer_w + MAX_BIT_BUFFERS - bitbuffer_r) & (MAX_BIT_BUFFERS - 1);
    }

    bool bitbuffer_is_full()
    {
        return bitbuffer_r == bitbuffer_inc(bitbuffer_w);
    }
    vpu_ret_t init_on_first_use();

    vpu_decode_event_nfy nfy_;
    uint32_t event_mask_;
    
    void nfy_framedecodecomplete(vpu_picture_t* picture);
    void nfy_streamseqsync(vpu_sequence_t* sequence);
    void nfy_bitbufferadded(vpu_buffer_id_t buffer_id);
public:
    virtual ~c_vdec_logo();

    // these are documented in vdec_if.h (Doxygen will copy these descriptions to the functions below).
    virtual vpu_ret_t subscribe(vpu_decode_event_nfy ntf, uint32_t event_mask);
    static vpu_ret_t init();
    static vpu_ret_t deinit();
    virtual vpu_ret_t reconfig(vpu_reconfig_t reconfig);
    virtual vpu_ret_t start(vpu_start_cfg_t start_cfg);
    virtual vpu_ret_t stop(vpu_stop_cfg_t stop_cfg);
    virtual vpu_ret_t mode_set(vpu_mode_t mode);
    virtual vpu_ret_t order_set(vpu_order_t order);
    virtual vpu_ret_t next_picture();
    virtual vpu_ret_t next_sequence();
    virtual vpu_ret_t finish_stream();
    virtual vpu_ret_t status(vpu_status_t* status);
    virtual vpu_ret_t add_bitbuffer(vpu_data_buffer_t buffer);
    virtual vpu_ret_t bitbuffer_size(uint32_t* buffer);
    virtual vpu_ret_t register_picture_buffer(vpu_buffer_t* buffer);
    virtual vpu_ret_t unregister_picture_buffer(vpu_buffer_id_t buffer_id);
    virtual vpu_ret_t done_with_picture_buffer(vpu_buffer_id_t buffer_id);
    virtual vpu_ret_t register_ud_buffer(vpu_data_buffer_t* buffer);
    virtual vpu_ret_t unregister_ud_buffer(vpu_data_buffer_id_t buffer_id);
    virtual vpu_ret_t done_with_ud_buffer(vpu_data_buffer_id_t buffer_id);
    virtual vpu_ret_t ud_config(vpu_syntax_flags_t syntax_flags, vpu_sei_flags_t sei_flags, vpu_order_t order);

    virtual void process_decode();

};

#endif
