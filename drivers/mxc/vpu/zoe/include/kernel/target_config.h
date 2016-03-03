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


//
// History:
//	June 13, 2013	(dtao) Created
//
// Authors: (dtao) dtao
//

// Definitions for the windows kernel target:
//
// This file contains all the definitions for configuring a linux kernel driver using sosal.
// Some constants can only be defined/undefined (boolean), others take a value.
// All except ZOE_PLATFORM can be omitted; they will take default
// values defined in osoptscommon.h.
//

#ifndef __TARGET_CONFIG_H__
#define __TARGET_CONFIG_H__

// This one must be defined to one of the supported targets (see zoe_target.h)
#define ZOE_TARGET_CHIP     ZOE_TARGET_CHIP_MX8
#define ZOE_TARGET_CHIP_VAR ZOE_TARGET_CHIP_VAR_
#define ZOE_TARGET_OS       ZOE_TARGET_OS_LINUX_KER
#define ZOE_TARGET_PUSET    ZOE_TARGET_PUSET_HPU

// Enable Debugging API (zoe_dbg.h)
#ifdef ZOE_DEBUG
	// Control output of zoe_dbg_printf
	//      TIME_STAMP    COMP_NAME                 FILE_NAME                                FUNCTION_NAME
	// (E) 323348471611 |Unspecified| [C:/Z/code/zv_root/zoe/system/ipc/zoe_ipc_srv.c] CZoeIPCService_DebugQueues:
//	#define ZOE_DEBUG_PRINT_TIME_STAMP
//	#define ZOE_DEBUG_PRINT_COMP_NAME
//	#define ZOE_DEBUG_PRINT_FILE_NAME
//	#define ZOE_DEBUG_PRINT_FUNCTION_NAME
#endif //ZOE_DEBUG

#define _USE_VIDEO_IOCTL2
#define LINUX_KER_THREAD_DEF_TIMEOUT    1000 // 1 msec
#define VDEC_LOGO_ONLY
#define USE_MEMRD_4_DMA
#define USE_MEMWR_4_DMA
#define VEDC_IN_PLACE_FRAME_BUFFER


#endif //__TARGET_CONFIG_H__

