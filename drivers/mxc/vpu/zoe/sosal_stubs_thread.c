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
// zoe sosal common thread management entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Oct 27, 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#include <stdio.h>
#endif

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"

#include "os_common.h"
#include "os_stubs.h"


ZOE_DBG_COMP_EXT(ZoeSosalDbgCompID);


//
// Return the number of priorities supported; thread priorities can range
// between 0 (lowest priority) and (maxpriorities - 1).
//
uint32_t
zoe_sosal_thread_maxpriorities_get (void)
{
    return (OSThreadMaxprioritiesGet());
}


//
// Create and start a new thread; thread's ID is returned in *thread_id_ptr
//
zoe_errs_t
zoe_sosal_thread_create (zoe_sosal_thread_parms_ptr_t parms_ptr,
						const zoe_sosal_obj_name_t name,
						zoe_sosal_obj_id_t * thread_id_ptr)
{
    zoe_errs_t   	rv = ZOE_ERRS_FAIL;
    PTObjInfo      	tiP = ZOE_NULL;
    uint32_t		priNum = OSThreadMaxprioritiesGet();

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered parms_ptr(%p) thread_id_ptr(%p)\r\n",parms_ptr,thread_id_ptr);

    if (parms_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"No thread parameters structure\r\n");
        rv = ZOE_ERRS_PARMS;
        goto thread_createExit;
    }

    if (parms_ptr->priority >= priNum) {
        zoe_dbg_printf_w(ZoeSosalDbgCompID,"Priority too high: %d set to max(%d)\r\n",parms_ptr->priority,priNum-1);
    	parms_ptr->priority = priNum - 1;
    }

	if (parms_ptr->proc == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL thread procedure pointer\r\n");
        rv = ZOE_ERRS_PARMS;
        goto thread_createExit;
	}

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto thread_createExit;
	}

    tiP = (PTObjInfo) OSThreadObjectAlloc();
    if (tiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSThreadObjectAlloc failed\r\n");
        rv = ZOE_ERRS_NOMEMORY;
        goto thread_createExit;
    }

	InitObj(tiP,OBJTYPE_THREAD,name);
	tiP->clientData = parms_ptr->client_data;
    rv = OSThreadCreate(tiP,parms_ptr);
    if (ZOE_SUCCESS(rv)) {
        if (thread_id_ptr != ZOE_NULL) {
            *thread_id_ptr = tiP;
        }
    }else{
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSThreadCreate failed (%d)\r\n",rv);
        goto thread_createExit;
    }

thread_createExit:
	if (ZOE_FAIL(rv)) {
		if (tiP != ZOE_NULL) {
			OSThreadObjectFree(tiP);
		}
	}
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}


// Delete an existing thread
zoe_errs_t
zoe_sosal_thread_delete (zoe_sosal_obj_id_t thread_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       tiP;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, thread_id(0x%llX)\r\n",thread_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto thread_deleteExit;
	}

	tiP = (PTObjInfo) thread_id;
    if ((thread_id == 0) || (tiP == ZOE_NULL) || (tiP->type != OBJTYPE_THREAD)) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid thread ID 0x%llX\n",thread_id);
		rv = ZOE_ERRS_PARMS;
        goto thread_deleteExit;
	}

	rv = OSThreadDelete(tiP);
	if (ZOE_SUCCESS(rv)) {
		TermObj(tiP);
		OSThreadObjectFree(tiP);
	}else if (rv == ZOE_ERRS_INVALID) {
		// This is the case of a thread that has already terminated naturally
		rv = ZOE_ERRS_SUCCESS;
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSThreadDelete failed (%d)\r\n",rv);
	}

thread_deleteExit:
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}


// Return executing thread's ID
zoe_sosal_obj_id_t
zoe_sosal_thread_get_id (void)
{
	zoe_sosal_obj_id_t	rv = ZOE_NULL;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered\r\n");

	if (!OSInISR()) {
		rv = OSThreadGetID();
	}

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (0x%llX)\r\n",rv);
	return (rv);
}


// Suspend executing thread for a number of milliseconds
zoe_errs_t
zoe_sosal_thread_sleep_ms (uint32_t milliseconds)
{
    zoe_errs_t          rv = ZOE_ERRS_SUCCESS;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered milliseconds(%d)\r\n",milliseconds);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
	}else{
		rv = OSThreadSleepMS(milliseconds);
	}

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}


// Suspend executing thread for a number of microseconds
zoe_errs_t
zoe_sosal_thread_sleep_us (uint32_t microseconds)
{
    zoe_errs_t          rv = ZOE_ERRS_SUCCESS;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered microseconds(%d)\r\n",microseconds);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
	}else{
		rv = OSThreadSleepUS(microseconds);
	}

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}


// Get a thread's priority
zoe_errs_t
zoe_sosal_thread_priority_get (zoe_sosal_obj_id_t thread_id, uint32_t * priority_ptr)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       tiP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, thread_id(0x%llX), priority_ptr(%p)\r\n"
    		,thread_id,priority_ptr);

    if (priority_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL priority_ptr\r\n");
        rv = ZOE_ERRS_PARMS;
        goto priority_getExit;
    }

	if (OSInISR()) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
		rv = ZOE_ERRS_INVALID;
        goto priority_getExit;
    }

	tiP = (PTObjInfo) thread_id;
    if (thread_id != 0) {
		if ((tiP == ZOE_NULL) || (tiP->type != OBJTYPE_THREAD)) {
			zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid thread ID 0x%llX\n",thread_id);
			rv = ZOE_ERRS_PARMS;
			goto priority_getExit;
		}
	}

	rv = OSThreadPriorityGet(tiP,priority_ptr);


priority_getExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}


// Change a thread's priority (and return current priority)
zoe_errs_t
zoe_sosal_thread_priority_set (zoe_sosal_obj_id_t thread_id, uint32_t priority)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       tiP;
    uint32_t		priNum = OSThreadMaxprioritiesGet();

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, thread_id(0x%llX), priority(%d)\r\n"
    		,thread_id,priority);

    if (priority >= priNum) {
        zoe_dbg_printf_w(ZoeSosalDbgCompID,"priority too high %d, set to max(%d)\r\n",priority,priNum-1);
    	priority = priNum - 1;
    }

	if (OSInISR()) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
		rv = ZOE_ERRS_INVALID;
        goto priority_setExit;
	}

	tiP = (PTObjInfo) thread_id;
    if ((thread_id != 0) && ((tiP == ZOE_NULL) || (tiP->type != OBJTYPE_THREAD))) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid thread ID 0x%llX\n",thread_id);
		rv = ZOE_ERRS_INVALID;
        goto priority_setExit;
	}

	rv = OSThreadPrioritySet(tiP,priority);

priority_setExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}


// Abort a suspended thread
zoe_errs_t
zoe_sosal_thread_abort (zoe_sosal_obj_id_t thread_id)
{
    zoe_errs_t      rv = ZOE_ERRS_FAIL;
    PTObjInfo       tiP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, thread_id(0x%llX)\r\n",thread_id);

	tiP = (PTObjInfo) thread_id;
    if ((thread_id == 0) || (tiP == ZOE_NULL) || (tiP->type != OBJTYPE_THREAD)) {
		//zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid thread ID 0x%016llX\n",thread_id);
		rv = ZOE_ERRS_INVALID;
        goto abortExit;
	}

	rv = OSThreadAbort(tiP);

abortExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\r\n",rv);
    return (rv);
}





