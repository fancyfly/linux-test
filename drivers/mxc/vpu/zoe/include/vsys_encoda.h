//HEADER
#ifndef __VSYS_ENCODA_H__
#define __VSYS_ENCODA_H__
#include "vpu_enc_if_impl.h"
#include "venc_if.h"

class c_vpu_enc_if;

/// Class c_vsys_encoda implements a video system instance that manages vpu encoder interfaces (\ref c_vpu_enc_if) and encoder instances implementing these interfaces.
/// See \ref videoModulesOverview for overview.
class c_vsys_encoda
{
	static c_vsys_encoda* vsys_encoda;

	c_vsys_encoda();
	~c_vsys_encoda();
    enum { MAX_INSTANCES = VPU_MAX_ENCODE_STREAMS };
    c_vpu_enc_if*        vpu_enc_instances_[MAX_INSTANCES];
    static c_vpu_enc_if& itf(vpu_stream_id_t stream_id = (vpu_stream_id_t)-1, vpu_enc_video_standard_t video_standard = VPU_ENC_VIDEO_UNDEFINED, bool destroy_only = false);
    void init_vsys_encoda();

    void switch_video_standard(vpu_stream_id_t stream_id, vpu_enc_video_standard_t video_standard);
public:
    static void create_vsys_encoda(); ///< Create the single instance of this video system.
    static void destroy_vsys_encoda();///< Destroy the single instance of this video system.
};

#endif
