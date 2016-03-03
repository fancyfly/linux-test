/*
 * Copyright (c) 2014-2015, Freescale Semiconductor, Inc.
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
/******************************************************************************
*                                                                             *
* All rights are reserved. This confidential and proprietary HDL/C/HVL soft   *
* description of a Hardware/Software component may be used only as authorized *
* by a licensing agreement from Zenverge Incorporated. In the event of        *
* publication, the following notice is applicable:                            *
*                                                                             *
*                    (C) COPYRIGHT 2011-2014 Zenverge Inc.                    *
*                           ALL RIGHTS RESERVED                               *
* The entire notice above must be reproduced on all authorized copies of this *
* code. Reproduction in whole or in part is prohibited without the prior      *
* written consent of the Zenverge Incorporated.                               *
* Zenverge Incorporated reserves the right to make changes without notice at  *
* any time. Also, Zenverge Incorporated makes no warranty, expressed, implied *
* or statutory, including but not limited to any implied warranty of          *
* merchantability or fitness for any particular purpose, or that the use will *
* not infringe any third party patent, copyright or trademark. Zenverge       *
* Incorporated will not be liable for any loss or damage arising from its use *
******************************************************************************/


///////////////////////////////////////////////////////////////////////////////
//
// Debugging API - Stubs for all platforms
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Oct 12, 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifdef ZOE_DEBUG

//
// The information printed in each debug print statement can be controlled for the
// whole target in a target_config.h file using the following ZOE_DEBUG_PRINT... constants:
//
//	ZOE_DEBUG_PRINT_TIME_STAMP controls printing of the time at which the printout occurred (in local clock ticks)
//	ZOE_DEBUG_PRINT_COMP_NAME controls printing of the debugging component's name
//	ZOE_DEBUG_PRINT_FILE_NAME controls printing of the (full path) name of the source file
//	ZOE_DEBUG_PRINT_FUNCTION_NAME controls printing of the name of the function
//

#if ((ZOE_TARGET_OS == ZOE_TARGET_OS_LINUX_KER) || defined(ZOE_LINUXKER_BUILD))
    // Linux kernel uses printk for debug output
    #include <linux/kernel.h>
    #include <linux/string.h>
    #define oneLineOut(...) printk(__VA_ARGS__)
    #define initOut()
    #define add2Out(fmt,arg) printk(fmt,arg)
    #define termOut(fmt,parmsList) vprintk(fmt,parmsList)
#else
    #include <stdarg.h>
    #include <stdio.h>
    #include <string.h>

    #if ((ZOE_TARGET_OS == ZOE_TARGET_OS_WIN_KER) || defined(ZOE_WINKER_BUILD))
        // Windows kernel must use DbgPrint for debug outputs (they can be visualized by
        // the DbgView application).  And the zoe_dbg_printf call must be mapped
        // to a single DbgPrint call (or else there will be several lines in DbgView).
        // This limits the length of a debug string to a predefined size defined below
        // (DBGPF_BUFSIZE).
        #include <ntddk.h>
        #define MAX_DBGSTR 256
        #define DBGPF_BUFSIZE (MAX_COMPNAME_LEN+MAX_DBGSTR+2)
        #define oneLineOut(...) DbgPrint(__VA_ARGS__)
        #define initOut()   \
            char    buff[DBGPF_BUFSIZE];    \
            char *  p_buff = &buff[0]
        #define add2Out(fmt,arg) p_buff += sprintf(p_buff,fmt,arg)
        #define termOut(fmt,parmsList)  \
            vsprintf(p_buff,fmt,parmsList); \
            DbgPrint(p_buff)
    #else
        // All others use fprintf to stderr.
        #define oneLineOut(...) fprintf(stderr,__VA_ARGS__)
        #define initOut()
        #define add2Out(fmt,arg) fprintf(stderr,fmt,arg)
        #define termOut(fmt,parmsList) vfprintf(stderr,fmt,parmsList)
    #endif
#endif

#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_sosal.h"

#include "os_common.h"

#define UNSPEC_NAME "Unspecified"
#define MAX_COMPS 32
#define MAX_COMPNAME_LEN 32


const char LVL_LETTER[ZOE_DBG_LVL_NUM] = {'F','E','W','T','X'};

typedef struct {
    zoe_bool_t          inUse;
    zoe_dbg_levels_t    dbgLevel;
    char                name[MAX_COMPNAME_LEN];
} TCompInfo, *PTCompInfo;

static TCompInfo CompInfos[MAX_COMPS+1];

static TOSLockID CompsLockID = ZOE_NULL;

void
OSDbgInit (void)
{
    int     i;

    if (CompsLockID != ZOE_NULL) {
    	// already initialized
    	goto dbgInitExit;
    }

    CompInfos[0].inUse = ZOE_TRUE;
    CompInfos[0].dbgLevel = ZOE_DEBUG_DEFAULT_LEVEL;
    strncpy(CompInfos[0].name,UNSPEC_NAME,MAX_COMPNAME_LEN);
    for (i = 1; i <= MAX_COMPS; i++) {
        CompInfos[i].inUse = ZOE_FALSE;
        CompInfos[i].dbgLevel = ZOE_DEBUG_DEFAULT_LEVEL;
        CompInfos[i].name[0] = 0;
    }

    CompsLockID = OSLockCreate("DBG components lock");
    if (CompsLockID == ZOE_NULL) {
        oneLineOut("Could not create DBG components lock\n");
    }

dbgInitExit:
	return;
}


void
OSDbgTerm (void)
{
    if (CompsLockID != ZOE_NULL) {
		OSLockDelete(CompsLockID);
    }
}


// Generic printf with level and component id support.
// The debug printout is emitted only if the current debug level
// for the requested component is lower or equal to the requested level.
void
_zoe_dbg_printf_ (const char * fileName,
                  const char * funcName,
                  zoe_dbg_levels_t lvl,
                  zoe_dbg_comp_id_t c_id,
                  const char * fmt,
                  ...)
{
	zoe_uintptr_t		cIdx = (zoe_uintptr_t) c_id;

    if (cIdx <= MAX_COMPS) {
        PTCompInfo          ciP;
        va_list             parmsList;
        zoe_bool_t          inUse;
        zoe_dbg_levels_t    dbgLevel;
#ifdef ZOE_DEBUG_PRINT_COMP_NAME
        char                name[MAX_COMPNAME_LEN];
#endif
        initOut();

        va_start(parmsList,fmt);
        OSLockGet(CompsLockID);
        ciP = &CompInfos[cIdx];
		inUse = ciP->inUse;
		dbgLevel = ciP->dbgLevel;
#ifdef ZOE_DEBUG_PRINT_COMP_NAME
        strncpy(name,ciP->name,MAX_COMPNAME_LEN);
#endif
        OSLockRelease(CompsLockID);

        if (inUse && (lvl <= dbgLevel)) {
            add2Out("(%c) ",LVL_LETTER[lvl]);
#ifdef ZOE_DEBUG_PRINT_TIME_STAMP
            add2Out("%llu ",(long long unsigned int) zoe_sosal_time_sys_ticks());
#endif
#ifdef ZOE_DEBUG_PRINT_COMP_NAME
            add2Out("|%s| ",name);
#endif
#ifdef ZOE_DEBUG_PRINT_FILE_NAME
            add2Out("[%s] ",fileName);
#endif
#ifdef ZOE_DEBUG_PRINT_FUNCTION_NAME
            add2Out("%s: ",funcName);
#endif
            termOut(fmt,parmsList);
        }
    }
}


// Register/unregister debug components
zoe_dbg_comp_id_t
zoe_dbg_register (const char * name, zoe_dbg_levels_t lvl)
{
	zoe_uintptr_t		rv = 0;
    int                 i;

	OSLockGet(CompsLockID);

	// search for an available component info record
	for (i = MAX_COMPS; i >= 1; i--) {
		if (!CompInfos[i].inUse) {
			break;
		}
	}
	if (i > 0) {
		CompInfos[i].inUse = ZOE_TRUE;
		CompInfos[i].dbgLevel = lvl;
		if (name == ZOE_NULL) {
			snprintf(CompInfos[i].name,MAX_COMPNAME_LEN,"no name #%d",i);
		}else{
			strncpy(CompInfos[i].name,name,MAX_COMPNAME_LEN);
		}
		rv = i;
	}

	OSLockRelease(CompsLockID);

    return ((zoe_dbg_comp_id_t) rv);
}


void
zoe_dbg_unregister (zoe_dbg_comp_id_t c_id)
{
	zoe_uintptr_t	cIdx = (zoe_uintptr_t) c_id;

    if (cIdx <= MAX_COMPS) {
    	OSLockGet(CompsLockID);
		CompInfos[cIdx].inUse = ZOE_FALSE;
		OSLockRelease(CompsLockID);
    }
}


// Get/set debugging level
zoe_dbg_levels_t
zoe_dbg_level_get (zoe_dbg_comp_id_t c_id)
{
    zoe_dbg_levels_t    rv = ZOE_DBG_LVL_NUM;
    zoe_uintptr_t		cIdx = (zoe_uintptr_t) c_id;

    if (cIdx <= MAX_COMPS) {
    	OSLockGet(CompsLockID);
		if (CompInfos[cIdx].inUse) {
			rv = CompInfos[cIdx].dbgLevel;
		}
		OSLockRelease(CompsLockID);
    }

    return (rv);
}


void
zoe_dbg_level_set (zoe_dbg_comp_id_t c_id, zoe_dbg_levels_t lvl)
{
	zoe_uintptr_t		cIdx = (zoe_uintptr_t) c_id;

    if (cIdx <= MAX_COMPS) {
    	OSLockGet(CompsLockID);
		if (CompInfos[cIdx].inUse) {
			CompInfos[cIdx].dbgLevel = lvl;
		}
		OSLockRelease(CompsLockID);
    }
}


#endif //ZOE_DEBUG

