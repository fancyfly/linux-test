//HEADER
#ifndef __VSYS_ONE_H__
#define __VSYS_ONE_H__
#include "vpu_if.h"
#include "vdec_if.h"

class c_vpu_if;

/// Class c_vsys_one implements a video system instance that manages vpu interfaces (\ref c_vpu_if) and decoder 
/// instances implementing these interfaces.
///
/// To instantiate this video system call \ref c_vsys_one::create_vsys_one "create_vsys_one", to destroy the instance
/// call \ref c_vsys_one::destroy_vsys_one "destroy_vsys_one".
/// There can be only one instance of a video system.
///
/// Class c_vsys_one will bind c_vsys_one::itf, c_vsys_one::init_codecs and c_vsys_one::deinit_codecs with \ref 
/// c_vpu_if using  c_vpu_if::set_itf_callback, c_vpu_if::set_init_callback and c_vpu_if::set_deinit_callback.
///
/// -------------------------------------------------------------------------------------------------------------------
/// See \ref videoModulesOverview for overview of all modules.
class c_vsys_one
{
    /// Single instance of this video system - will be 0 before  \ref c_vsys_one::create_vsys_one "create_vsys_one"
    /// and after \ref c_vsys_one::destroy_vsys_one "destroy_vsys_one".
    static c_vsys_one* vsys_one; 

    c_vsys_one(); ///< Constructor.
    ~c_vsys_one(); ///< Destructor.

    enum { MAX_INSTANCES = VPU_MAX_DECODE_STREAMS };
    c_vpu_if* vpu_instances_[MAX_INSTANCES];

    /// Returns the interface to a decoder instance for the specified stream; vsys_one will bind this to the 
    /// vpu_if_impl component.
    static c_vpu_if& itf(vpu_stream_id_t stream_id = (vpu_stream_id_t)-1, 
        vpu_video_standard_t video_standard = VPU_VIDEO_UNDEFINED, bool destroy_only = false);

    void init_vsys_one(); ///< Initialize the videso system on creation.
    
    static vpu_ret_t init_codecs(); ///< Function \ref init_codecs() is bound to the vpu_init interface in \ref _vpu_if_impl.
    static vpu_ret_t deinit_codecs(); ///< Function \ref init_codecs() is bound to the vpu_deinit interface in \ref _vpu_if_impl.

    /// Function \ref switch_video_standard fascilitates switching between different video standards and codecs.
    /// \param[in] stream_id The stream to which the standard switch applies.
    /// \param[in] video_standard The video standard to switch to.
    void switch_video_standard(vpu_stream_id_t stream_id, vpu_video_standard_t video_standard);

public:
    static void create_vsys_one(); ///< Create the single instance of this video system.
    static void destroy_vsys_one();///< Destroy the single instance of this video system.
};

#endif
