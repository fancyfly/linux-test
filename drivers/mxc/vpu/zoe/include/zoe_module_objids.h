/*
 * Copyright (c) 2012-2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


///////////////////////////////////////////////////////////////////////////////
//
// zoe_module_objids.h
//
// Description: 
//
//  object id for ZOE modules
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_MODULE_OBJIDS_H__
#define __ZOE_MODULE_OBJIDS_H__


#include "zoe_objids.h"

/////////////////////////////////////////////////////////////////////////////
//
//

// normal module object id
//

#define OBJECT_ZOE_MODULE_NORMAL_START  (OBJECT_ZOE_MODULE)

enum 
{
    OBJECT_ZOE_MODULE_MGR = OBJECT_ZOE_MODULE_NORMAL_START,
    OBJECT_ZOE_MODULE_GRAPH_MGR,

    
    OBJECT_ZOE_MODULE_NORMAL_LAST
    // no object behind this line
};

#define OBJECT_ZOE_MODULE_NORMAL_END    (OBJECT_ZOE_MODULE_NORMAL_LAST - 1)


// streaming module object id
//
#define OBJECT_ZOE_MODULE_STREAMING_START   (OBJECT_ZOE_MODULE + 50)

enum 
{
    OBJECT_ZOE_MODULE_DEMUX = OBJECT_ZOE_MODULE_STREAMING_START,
    OBJECT_ZOE_MODULE_SOURCE,
    OBJECT_ZOE_MODULE_SINK,
    OBJECT_ZOE_MODULE_VIDDEC,
    OBJECT_ZOE_MODULE_AUDDEC,
    OBJECT_ZOE_MODULE_VIDRENDERER,
    OBJECT_ZOE_MODULE_AUDRENDERER,
    OBJECT_ZOE_MODULE_MY_MODULE,
    OBJECT_ZOE_MODULE_CORE_SOURCE,
    OBJECT_ZOE_MODULE_CORE_SINK,
    OBJECT_ZOE_MODULE_VIDDEC_CPP,
    OBJECT_ZOE_MODULE_MY_MODULE_CPP,
    OBJECT_ZOE_MODULE_VDEC_H264,
    OBJECT_ZOE_MODULE_VDEC_HEVC,
    OBJECT_ZOE_MODULE_VDEC_MPEG2,
    OBJECT_ZOE_MODULE_PACKET_VES,
    OBJECT_ZOE_MODULE_VDEC_ADAPTOR,

    OBJECT_ZOE_MODULE_STREAMING_LAST
    // no object behind this line
};

#define OBJECT_ZOE_MODULE_STREAMING_END (OBJECT_ZOE_MODULE_STREAMING_LAST - 1)


/////////////////////////////////////////////////////////////////////////////
//
//

// macro to convert object id to module id
#define ZOE_MODULE_ID(x)                        ((x) - OBJECT_USER)



#endif //__ZOE_MODULE_OBJIDS_H__

