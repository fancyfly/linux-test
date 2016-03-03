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
*                  (C) COPYRIGHT 2011-2014 Zenverge Inc.                      *
*                         ALL RIGHTS RESERVED                                 *
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
// stubs.c  zoe sosal common entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Oct 27, 2011
//
///////////////////////////////////////////////////////////////////////////////


#if !defined(ZOE_LINUXKER_BUILD)
#include <string.h>
#include <stdio.h>
#endif

#if defined(ZOE_LINUXKER_BUILD)
#include <linux/kernel.h>
#include <linux/string.h>
#endif

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"

#include "os_common.h"
#include "os_stubs.h"

ZOE_DBG_COMP_DECL(ZoeSosalDbgCompID);

TOSLockID EventsLockID = ZOE_NULL;

static TOSLockID objsCounterLockID = ZOE_NULL;

zoe_bool_t
zoe_sosal_init (void)
{
    zoe_bool_t  rv = ZOE_FALSE;

    if (!OSSosalInit()) {
        goto sosal_initExit;
    }

    ZOE_DBG_COMP_REGISTER_DEF("sosal",ZoeSosalDbgCompID);

    EventsLockID = OSLockCreate(ZOE_NULL);
	if (EventsLockID == ZOE_NULL) {
		zoe_dbg_printf_f(ZoeSosalDbgCompID,"Cannot create events lock\n");
		goto sosal_initExit;
	}
	objsCounterLockID = OSLockCreate(ZOE_NULL);
	if (objsCounterLockID == ZOE_NULL) {
		zoe_dbg_printf_f(ZoeSosalDbgCompID,"Cannot create objects counter lock\n");
		goto sosal_initExit;
	}

    rv = ZOE_TRUE;

sosal_initExit:
    return (rv);
}


void
zoe_sosal_term (void)
{
	if (EventsLockID != ZOE_NULL) {
		OSLockDelete(EventsLockID);
	}
	if (objsCounterLockID != ZOE_NULL) {
		OSLockDelete(objsCounterLockID);
	}

    ZOE_DBG_COMP_UNREGISTER(ZoeSosalDbgCompID);

    OSSosalTerm();
}

void * zoe_sosal_get_hal_ptr (void)
{
    return(OSSosalGetHalP());
}



///////////////////////////////////////////////////////////////////////////////
//
// API calls
//


// Return information about the running OS
zoe_errs_t
zoe_sosal_info (zoe_sosal_info_ptr_t info_ptr)
{
    zoe_errs_t  rv = ZOE_ERRS_SUCCESS;

    if (info_ptr == ZOE_NULL) {
		rv = ZOE_ERRS_PARMS;
		zoe_dbg_printf_w(ZoeSosalDbgCompID,"NULL info pointer\n");
    }else{
    	OSInfo(info_ptr);
    }

    return (rv);
}


// Set an object name array with a given string
void
zoe_sosal_obj_name_set (zoe_sosal_obj_name_t name_ptr, const char * string_ptr)
{
	int 	l = 0;

	if (string_ptr != ZOE_NULL) {
		l = strnlen(string_ptr,ZOE_SOSAL_OBJNAME_MAXLEN);

		if (l >= sizeof(zoe_sosal_obj_name_t)) {
			l = sizeof(zoe_sosal_obj_name_t) - 1;
		}
		memcpy(name_ptr,string_ptr,l);
	}
	// name must always be NULL-terminated
	name_ptr[l] = 0;
}


// Get an object's name (if obj_id is NULL it applies to the running thread)
zoe_errs_t
zoe_sosal_obj_name_get (zoe_sosal_obj_id_t obj_id, zoe_sosal_obj_name_t name_ptr)
{
    zoe_errs_t  rv = ZOE_ERRS_SUCCESS;
    PTObjInfo   oiP;

    if (obj_id == 0) {
		if (OSInISR()) {
			rv = ZOE_ERRS_PARMS;
			goto obj_name_getExit;
		}else{
			obj_id = OSThreadGetID();
		}
    }

    oiP = (PTObjInfo) obj_id;
    if (oiP == ZOE_NULL) {
        rv = ZOE_ERRS_INVALID;
        goto obj_name_getExit;
    }

	strncpy(name_ptr,oiP->name,ZOE_SOSAL_OBJNAME_MAXLEN);

obj_name_getExit:
    return (rv);
}


// Set an object's client data (if obj_id is NULL it applies to the running thread)
zoe_errs_t
zoe_sosal_obj_client_data_set (zoe_sosal_obj_id_t obj_id, void * client_data)
{
	zoe_errs_t  rv = ZOE_ERRS_SUCCESS;
	PTObjInfo   oiP;

    if (obj_id == 0) {
		if (OSInISR()) {
			rv = ZOE_ERRS_PARMS;
			goto obj_client_data_setExit;
		}else{
			obj_id = OSThreadGetID();
		}
	}

    oiP = (PTObjInfo) obj_id;
    if (oiP == ZOE_NULL) {
		rv = ZOE_ERRS_INVALID;
		goto obj_client_data_setExit;
	}

	oiP->clientData = client_data;

obj_client_data_setExit:
	return (rv);
}


// Get an object's client data (if obj_id is NULL it applies to the running thread)
zoe_errs_t
zoe_sosal_obj_client_data_get (zoe_sosal_obj_id_t obj_id, void ** client_data_ptr)
{
    zoe_errs_t  rv = ZOE_ERRS_SUCCESS;
	PTObjInfo   oiP;

	if (client_data_ptr == ZOE_NULL) {
		rv = ZOE_ERRS_PARMS;
		goto obj_client_data_getExit;
	}

    if (obj_id == 0) {
		if (OSInISR()) {
			rv = ZOE_ERRS_PARMS;
			goto obj_client_data_getExit;
		}else{
			obj_id = OSThreadGetID();
		}
	}

    oiP = (PTObjInfo) obj_id;
    if (oiP == ZOE_NULL) {
		rv = ZOE_ERRS_INVALID;
		goto obj_client_data_getExit;
	}

	*client_data_ptr = oiP->clientData;

obj_client_data_getExit:
    return (rv);
}



//////////////////////
// Time             //
//////////////////////

// Return the number of system ticks elapsed since system startup
zoe_sosal_ticks_t
zoe_sosal_time_sys_ticks (void)
{
    return (OSTimeSysTicks());
}


// Return the number of system ticks per second (usually the clock frequency)
uint64_t
zoe_sosal_time_ticks_per_second (void)
{
    return (OSTimeTicksPerSecond());
}


///////////////////////////////////////////////////////////////////////////////
//
// Common functions
//

const char * ObjNames[] = {
    "void",
    "thread",
    "mutex",
    "event",
    "spinlock",
    "semaphore",
    "timer"
};

void
InitObj (PTObjInfo oiP, EObjTypes type, const zoe_sosal_obj_name_t name)
{
	int						idx;

	oiP->type = type;
	switch (type) {
	case OBJTYPE_THREAD:
		idx = 1;
		break;
	case OBJTYPE_MUTEX:
		idx = 2;
		break;
	case OBJTYPE_EVENT:
		idx = 3;
		break;
	case OBJTYPE_SPINLOCK:
		idx = 4;
		break;
	case OBJTYPE_SEMAPHORE:
		idx = 5;
		break;
	case OBJTYPE_TIMER:
		idx = 6;
		break;
	default:
		idx = 0;
		break;
	}

	if (name == ZOE_NULL) {
        snprintf(oiP->name,sizeof(zoe_sosal_obj_name_t),"%s %p",ObjNames[idx],oiP);
    }else{
        memcpy(oiP->name,name,sizeof(zoe_sosal_obj_name_t));
    }
	// make sure it is NULL terminated
    oiP->name[sizeof(zoe_sosal_obj_name_t)-1] = 0;

    // initialize the client data to NULL
    oiP->clientData = ZOE_NULL;
}


void
TermObj (PTObjInfo oiP)
{
    oiP->type = OBJTYPE_VOID;
}

