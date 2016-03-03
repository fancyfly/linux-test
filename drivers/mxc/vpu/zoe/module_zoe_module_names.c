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
// zoe_module_names.c
//
// Description: 
//
//  ZOE module names
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_module_objids.h"
#include "zoe_module_mgr.h"
#include "zoe_module_core_src.h"
#include "zoe_module_core_sink.h"
#include "zoe_module_vdec_adaptor.h"
//#include "zoe_module_universal_src.h"
#ifndef ZOE_MODULE_MINIMAL
#include "zoe_graph_mgr.h"
#ifdef ZOE_MODULE_SAMPLE
#include "zoe_module_demux.h"
#include "zoe_module_sink.h"
#include "zoe_module_src.h"
#include "zoe_module_viddec.h"
#include "zoe_module_auddec.h"
#include "zoe_module_vidrenderer.h"
#include "zoe_module_audrenderer.h"
#include "zoe_module_my_module.h"
#include "zoe_module_viddec_cpp.h"
#include "zoe_module_my_module_cpp.h"
#include "zoe_module_vdec_h264.h"
#include "zoe_module_packet_ves.h"
#endif //ZOE_MODULE_SAMPLE
#endif //!ZOE_MODULE_MINIMAL

/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct _ZOE_MODULE_NAME
{
    uint32_t    module;
    char        *name;
} ZOE_MODULE_NAME, *PZOE_MODULE_NAME;

static const ZOE_MODULE_NAME s_module_names[] = 
{
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MGR),         ZOE_MODULE_NAME_MGR},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VDEC_ADAPTOR),ZOE_MODULE_NAME_VDEC_ADAPTOR},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SOURCE), ZOE_MODULE_NAME_CORE_SOURCE},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_CORE_SINK),   ZOE_MODULE_NAME_CORE_SINK},
//	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_UNIVERSAL_SOURCE),   ZOE_MODULE_NAME_UNIVERSAL_SOURCE},
#ifndef ZOE_MODULE_MINIMAL
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_GRAPH_MGR),   ZOE_MODULE_NAME_GRAPH_MGR},
#ifdef ZOE_MODULE_SAMPLE
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_DEMUX),       ZOE_MODULE_NAME_DEMUX},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_SOURCE),      ZOE_MODULE_NAME_SOURCE},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_SINK),        ZOE_MODULE_NAME_SINK},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VIDDEC),      ZOE_MODULE_NAME_VIDDEC},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_AUDDEC),      ZOE_MODULE_NAME_AUDDEC},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VIDRENDERER), ZOE_MODULE_NAME_VIDRENDERER},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_AUDRENDERER), ZOE_MODULE_NAME_AUDRENDERER},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MY_MODULE),   ZOE_MODULE_NAME_MY_MODULE},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VIDDEC_CPP),  ZOE_MODULE_NAME_VIDDEC_CPP},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_MY_MODULE_CPP),   ZOE_MODULE_NAME_MY_MODULE_CPP},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_VDEC_H264),   ZOE_MODULE_NAME_VDEC_H264},
	{ZOE_MODULE_ID(OBJECT_ZOE_MODULE_PACKET_VES),  ZOE_MODULE_NAME_PACKET_VES},
#endif //ZOE_MODULE_SAMPLE
#endif //!ZOE_MODULE_MINIMAL
};

static uint32_t s_module_name_count = SIZEOF_ARRAY(s_module_names);

#define ZOE_MODULE_NAME_UNKNOWN "unknown"


/////////////////////////////////////////////////////////////////////////////
//
//

char * zoe_module_get_name(uint32_t module_id)
{
    uint32_t    i;

    for (i = 0; i < s_module_name_count; i++)
    {
        if (s_module_names[i].module == module_id)
        {
            return (s_module_names[i].name);
        }
    }
    return (ZOE_MODULE_NAME_UNKNOWN);
}

