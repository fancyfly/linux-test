//HEADER
#ifndef __VDEC_IF_H__
#define __VDEC_IF_H__
#include "vpu_if_impl.h"


/// Class c_vdec_if exposes the the video decoder interface.
///
/// The interface is defined as an abstract class; decoders inherit from this class. Common functionality between
/// decoders are implemented here.
///
/// **Decoder general requirements**
///
/// The decoder shall provide sequence information to the client.
///
/// The decoder shall provide picture information to the client.
///
/// The decoder shall return pictures in decode order or presentation order.
///
/// The client can insert timing information, like PTS, that the decoder shall queue and return with
/// the applicable picture.
///
/// The decoder shall provide means to indicate the end of the stream which will force the output of any pending pictures.
///
/// The decoder shall support decode modes that allows automatic skipping of picture types, all pictures
/// (so only meta data is returned) or return only IDR (or similar) pictures.
///
///**Decoder buffer requirements**
///
/// The decoder does not own any memory for input buffers (bitbuffers) and decoded video pictures.
///
/// The decoder shall provide the number of bytes needed for the decoder input buffer, allowing the
/// the client to do a proper allocation.
///
/// The decoder shall find the coded units by searching for start codes.
///
/// The decoder shall remove emulation prevention bytes.
///
/// The decoder keeps track of buffers that are registered by the client.
///
/// The decoder will interchange buffers with the client; the client will indicate when it is done
/// with a buffer. The decoder will indicate when a buffer can be repurposed.
///
/// The decoder shall indicate the number of buffers needed for proper functioning of the decoder.
///
/// The decoder shall notify the client when it runs out of buffers.
///
/// The decoder shall indicate for the buffers; the number of bytes, stride(bytes per pixel row),
/// and offset from address for alignment purposes.
///
/// **Timing Info**
///
/// The association with the decoded frame is by proximity in the bitstream. The location in the bitstream is provided of each start 
/// of picture such that it can be related to PTSs coming in the PES layer. 
///
/// **Decoder usage**
/// Open the decoder using the \ref c_vdec_if::init() "init()" function.
///
/// Close the decoder using the \ref c_vdec_if::deinit() "deinit()" function to stop and free all resources. No notify_event() calls
/// will be called anymore.
///
/// Call \ref c_vdec_if::add_bitbuffer "add_bitbuffer()" to provide decoders with input buffers that it can parse.
///
/// Call \ref c_vdec_if::next_picture() "next_picture()" repeatedly to get sequence header information and/or pictures with meta data.
///
/// Adjust bitbuffer size to returned picture information. See \ref vpu_required_vbv_size().
/// Note once done with a bit buffer notify_event(BUFFER_RELEASE) is called and the client can
/// reuse the buffer and call add_bitbuffer again. Once released a bitbuffer is not used by the
/// decoder anymore.
///
/// Adjust number of picture buffers to returned information. See \ref vpu_required_nr_raw().
/// Call \ref c_vdec_if::register_picture_buffer() "register_picture_buffer()" to add picture buffers.
///
/// Once picture buffers are provided \ref c_vdec_if::next_picture() "next_picture()" will start returning actual pictures
/// next to sequence and picture information.
///
/// When done displaying a picture call \ref c_vdec_if::done_with_picture_buffer() "done_with_picture_buffer()" so frame buffer can be
/// recycled when not used as a reference anymore.
/// \ref c_vdec_if::unregister_picture_buffer() "unregister_picture_buffer()" can be called to remove buffers; notify
/// is called, allowing the client to repurpose the buffer.
///
/// -------------------------------------------------------------------------------------------------------------------
/// See \ref videoModulesOverview for overview of all modules.
class c_vdec_if : public c_vpu_if
{
protected:
    // c_vdec_if(c_vdec_nfy& notifier) : nfy_(notifier)
    // {
    // }

    void lock_api()
    {
        //  nfy_.notify_event(c_vdec_nfy::API_LOCK, 0);
    }

    void unlock_api()
    {
        //    nfy_.notify_event(nfy_.API_UNLOCK, 0);
    }

public:
    /// The main processing function to be called by a seperate task; will only exit when decider us destroyed.
    /// temporarilt moved to c_vpu_if virtual void process_decode() = 0;
};



#endif
