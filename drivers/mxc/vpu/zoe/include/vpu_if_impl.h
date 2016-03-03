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

#ifndef __VPU_IF_IMPL_H__
#define __VPU_IF_IMPL_H__
#include "vpu_if.h"

/// Class \ref c_vpu_if implements the client interface to the VPU.
/// \ref _vpu_if is the full specification.
///
/// \image html vpu_interface.svg
///
/// The component \ref _vpu_if declares the API but does not define the functions.
/// The functions definition is in component \ref _vpu_if_impl and also
/// in \ref _vpu_if_client. This is a component that marshalls the API accross the CPU boundary from
/// the application processor to the firmware processor. On the firmware side component \ref _vpu_if_server
/// will receive the function calls and call the implementation in \ref _vpu_if_impl.
/// As such there are two possible implementations.
/// 1. \ref _vpu_if_impl and \ref _vpu_if on the firmware side. 
/// 2. \ref _vpu_if and \ref _vpu_if_client on the application processor and \ref _vpu_if_server and \ref _vpu_if_impl
///    on the firmware processor.
/// 
/// \ref _vpu_if_impl is unaware of what video system it is being used by. As such a video system needs to bind to
/// \ref _vpu_if_impl. This means it connects a callback that returns a decoder instance and init/deinit callbacks.
/// Another video system could make a similar binding for a different platform instance. As such the API definition
/// and associated code are independent of the platform in which it is used.
///
/// Component \ref _vpu_pts_gen also binds to the interface - but this is done on the client side.
/// This allows for it to be replaced by similar components with a different implementation, so it can be tailored
/// to different use cases without the need to change the firmware.
class c_vpu_if
{
    friend class c_vsys_one;
    typedef c_vpu_if& (*fn_itf)(vpu_stream_id_t stream_id, vpu_video_standard_t video_standard, bool destroy_only);
    static fn_itf itf_cb_;
    static void set_itf_callback(fn_itf itf_cb); ///< This binds the function that returns a specified decoder instance.

    typedef vpu_ret_t (*fn_ini)();
    static fn_ini init_cb_;
    static fn_ini deinit_cb_;
    static void set_init_callback(fn_ini ini_cb); ///< This binds the function that initializes the decoder subsystem.
    static void set_deinit_callback(fn_ini deini_cb); ///< This binds the function that deinitializes the decoder subsystem.

    vpu_decode_event_nfy decode_event_nfy_;
    uint32_t             event_mask_;
protected:
    uint32_t             stream_id_;

public:
    virtual ~c_vpu_if() { };

    void stream_id_set(vpu_stream_id_t stream_id)
    {
        stream_id_ = stream_id;
    }

    uint32_t stream_id()
    {
        return stream_id_;
    }

    bool is_valid_decoder() 
    { 
        return this != 0; 
    }
    
    static c_vpu_if& invalid_decoder()
    {
        return *(c_vpu_if*)0;
    }

    static c_vpu_if& itf(vpu_stream_id_t stream_id = (vpu_stream_id_t)-1, vpu_video_standard_t video_standard = VPU_VIDEO_UNDEFINED, bool destroy_only = false);

    virtual vpu_ret_t subscribe(vpu_decode_event_nfy ntf, uint32_t event_mask) = 0; ///<\copydoc vpu_subscribe
    static vpu_ret_t init();   ///<\copydoc vpu_init
    static vpu_ret_t deinit(); ///<\copydoc vpu_deinit
    virtual vpu_ret_t reconfig(vpu_reconfig_t reconfig)                                                        = 0; ///<\copydoc vpu_reconfig
    virtual vpu_ret_t start(vpu_start_cfg_t start_cfg)                                                         = 0; ///<\copydoc vpu_start
    virtual vpu_ret_t stop(vpu_stop_cfg_t stop_cfg)                                                            = 0; ///<\copydoc vpu_stop
    virtual vpu_ret_t mode_set(vpu_mode_t mode)                                                                = 0; ///<\copydoc vpu_mode_set
    virtual vpu_ret_t order_set(vpu_order_t order)                                                             = 0; ///<\copydoc vpu_order_set
    virtual vpu_ret_t next_picture()                                                                           = 0; ///<\copydoc vpu_next_picture
    virtual vpu_ret_t next_sequence()                                                                          = 0; ///<\copydoc vpu_next_sequence
    virtual vpu_ret_t finish_stream()                                                                          = 0; ///<\copydoc vpu_finish_stream
    virtual vpu_ret_t status(vpu_status_t* status)                                                             = 0; ///<\copydoc vpu_status
    virtual vpu_ret_t add_bitbuffer(vpu_data_buffer_t buffer)                                                  = 0; ///<\copydoc vpu_add_bitbuffer
    virtual vpu_ret_t bitbuffer_size(uint32_t* bitbuffer_size)                                                 = 0; ///<\copydoc vpu_bitbuffer_size
    virtual vpu_ret_t register_picture_buffer(vpu_buffer_t* buffer)                                            = 0; ///<\copydoc vpu_register_picture_buffer
    virtual vpu_ret_t unregister_picture_buffer(vpu_buffer_id_t buffer_id)                                     = 0; ///<\copydoc vpu_unregister_picture_buffer
    virtual vpu_ret_t done_with_picture_buffer(vpu_buffer_id_t buffer_id)                                      = 0; ///<\copydoc vpu_done_with_picture_buffer
    virtual vpu_ret_t register_ud_buffer(vpu_data_buffer_t* buffer)                                            = 0; ///<\copydoc vpu_register_ud_buffer
    virtual vpu_ret_t unregister_ud_buffer(vpu_data_buffer_id_t buffer_id)                                     = 0; ///<\copydoc vpu_unregister_ud_buffer
    virtual vpu_ret_t done_with_ud_buffer(vpu_data_buffer_id_t buffer_id)                                      = 0; ///<\copydoc vpu_done_with_ud_buffer
    virtual vpu_ret_t ud_config(vpu_syntax_flags_t syntax_flags, vpu_sei_flags_t sei_flags, vpu_order_t order) = 0; ///<\copydoc vpu_ud_config
    virtual void process_decode() = 0; // temporary for hacking purposes
};

#endif
