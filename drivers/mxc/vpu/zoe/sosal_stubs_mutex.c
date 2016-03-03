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
*                   (C) COPYRIGHT 2011-2014 Zenverge Inc.                     *
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
// zoe sosal Mutex management entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Oct 27, 2011
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"

#include "os_common.h"
#include "os_stubs.h"


ZOE_DBG_COMP_EXT(ZoeSosalDbgCompID);

///////////////////////////////////////////////////////////////////////////////
//
// Mutexes
// Used for mutual exclusion; the thread that obtains the mutex is the one that must release it.
// To mitigate priority inversion issues, the thread that obtains a mutex will have its
// priority raised to the highest priority among the threads that are waiting on it.
//

// Create a new mutex; mutex id is returned in *mutex_id_ptr
zoe_errs_t
zoe_sosal_mutex_create (const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * mutex_id_ptr)
{
    zoe_errs_t			rv = ZOE_ERRS_SUCCESS;
    PTObjInfo			miP = ZOE_NULL;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, mutex_id_ptr(%p) name(%p)\n"
        ,mutex_id_ptr,name);

    if (mutex_id_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL mutex ID pointer\r\n");
        rv = ZOE_ERRS_PARMS;
        goto mutex_createExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto mutex_createExit;
	}

    miP = (PTObjInfo) OSMutexObjectAlloc();
    if (miP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSMutexObjectAlloc failed\n");
        rv = ZOE_ERRS_NOMEMORY;
        goto mutex_createExit;
    }

	InitObj((PTObjInfo) miP,OBJTYPE_MUTEX,name);
    rv = OSMutexCreate(miP);
    if (ZOE_SUCCESS(rv)) {
        *mutex_id_ptr = miP;
    }else{
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSMutexCreate failed (%l)\n",rv);
        goto mutex_createExit;
    }

mutex_createExit:
	if (ZOE_FAIL(rv)) {
		if (miP != ZOE_NULL) {
			TermObj((PTObjInfo) miP);
			OSMutexObjectFree(miP);
		}
	}
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Delete mutex
zoe_errs_t
zoe_sosal_mutex_delete (zoe_sosal_obj_id_t mutex_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       miP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, mutex_id(0x%llX)\n",mutex_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto mutex_deleteExit;
	}

	miP = (PTObjInfo) mutex_id;
    if ((mutex_id == 0) || ((miP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid mutex ID 0x%llX\n",mutex_id);
		rv = ZOE_ERRS_PARMS;
        goto mutex_deleteExit;
	}

	if (miP->type != OBJTYPE_MUTEX) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d \"%s\"\r\n",miP->type,zoe_object_type(miP->type));
		rv = ZOE_ERRS_INVALID;
        goto mutex_deleteExit;
	}

	rv = OSMutexDelete(miP);
	if (ZOE_SUCCESS(rv)) {
		// Should the mutex object be freed also when OSMutexDelete fails?
		TermObj(miP);
		OSMutexObjectFree(miP);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSMutexDelete failed (%d)\r\n",rv);
	}

mutex_deleteExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Release a mutex; might cause a waiting thread to wake up
zoe_errs_t
zoe_sosal_mutex_release (zoe_sosal_obj_id_t mutex_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       miP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, mutex_id(0x%llX)\n",mutex_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto mutex_releaseExit;
	}

	miP = (PTObjInfo) mutex_id;
    if ((mutex_id == 0) || ((miP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid mutex ID 0x%llX\n",mutex_id);
		rv = ZOE_ERRS_PARMS;
        goto mutex_releaseExit;
	}

	if (miP->type == OBJTYPE_MUTEX) {
		rv = OSMutexRelease(miP);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",miP->type);
		rv = ZOE_ERRS_INVALID;
	}

mutex_releaseExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


//
// Get (lock) a mutex.
// 'microseconds' is interpreted as follows:
//  > 0: number of microseconds to wait
//  = 0: return immediately (poll for object's availability)
//  < 0: wait indefinitely
//
zoe_errs_t
zoe_sosal_mutex_get (zoe_sosal_obj_id_t mutex_id, int32_t microseconds)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       miP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, mutex_id(0x%llX)\n",mutex_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto mutex_getExit;
	}

	miP = (PTObjInfo) mutex_id;
    if ((mutex_id == 0) || ((miP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid mutex ID 0x%llX\n",mutex_id);
		rv = ZOE_ERRS_PARMS;
        goto mutex_getExit;
	}

	if (miP->type == OBJTYPE_MUTEX) {
		rv = OSMutexGet(miP,microseconds);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",miP->type);
		rv = ZOE_ERRS_INVALID;
	}

mutex_getExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}





