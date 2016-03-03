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
*               (C) COPYRIGHT 2013-2014 Zenverge Inc.                         *
*                   ALL RIGHTS RESERVED                                       *
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
// zoe sosal Timers entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Sep 12, 2013
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
// Timers
//
// Used used to execute short functions after a given time or periodically.
// Timer callbacks execute in ISRs context.
//


// Create a new timer object; the timer's id is returned in *timer_id_ptr
zoe_errs_t
zoe_sosal_timer_create (const zoe_sosal_obj_name_t name, zoe_sosal_time_timer_proc_t proc,
								void * parm, zoe_sosal_obj_id_t * timer_id_ptr)
{
    zoe_errs_t				rv = ZOE_ERRS_SUCCESS;
    PTObjInfo				tiP = ZOE_NULL;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, timer_id_ptr(%p) name(%p), proc(0x%X), parm(0x%X)\n"
        ,timer_id_ptr,name,proc,parm);

    if (timer_id_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL timer ID pointer\r\n");
        rv = ZOE_ERRS_PARMS;
        goto timer_createExit;
    }

    if (proc == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL callback pointer\r\n");
        rv = ZOE_ERRS_PARMS;
        goto timer_createExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto timer_createExit;
	}

    tiP = (PTObjInfo) OSTimerObjectAlloc();
    if (tiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSTimerObjectAlloc failed\n");
        rv = ZOE_ERRS_NOMEMORY;
        goto timer_createExit;
    }

	InitObj((PTObjInfo) tiP,OBJTYPE_TIMER,name);
    rv = OSTimerCreate(tiP,proc,parm);
    if (ZOE_SUCCESS(rv)) {
        *timer_id_ptr = tiP;
    }else{
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSTimerCreate failed (%lu)\n",rv);
        goto timer_createExit;
    }

timer_createExit:
	if (ZOE_FAIL(rv)) {
		if (tiP != ZOE_NULL) {
			TermObj((PTObjInfo) tiP);
			OSTimerObjectFree(tiP);
		}
	}
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Delete an existing timer object
zoe_errs_t
zoe_sosal_timer_delete (zoe_sosal_obj_id_t timer_id)
{
    zoe_errs_t		rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       tiP;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, timer_id(0x%llX)\n",timer_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto timer_deleteExit;
	}

	tiP = (PTObjInfo) timer_id;
    if ((timer_id == 0) || ((tiP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid timer ID 0x%llX\n",timer_id);
		rv = ZOE_ERRS_PARMS;
        goto timer_deleteExit;
	}

	if (tiP->type != OBJTYPE_TIMER) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",tiP->type);
		rv = ZOE_ERRS_INVALID;
        goto timer_deleteExit;
	}

	rv = OSTimerDelete(tiP);
	if (ZOE_SUCCESS(rv)) {
		// Should the timer object be freed also when OSTimerDelete fails?
		TermObj(tiP);
		OSTimerObjectFree(tiP);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSTimerDelete failed (%lu)\n",rv);
		goto timer_deleteExit;
	}

timer_deleteExit:
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Starts the timer with relative (drifty) timing; if periodic is FALSE the callback is called only once
zoe_errs_t
zoe_sosal_timer_start (zoe_sosal_obj_id_t timer_id, uint64_t us, zoe_bool_t periodic)
{
	zoe_errs_t		rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       tiP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, timer_id(0x%llX), us (%d), periodic (%d)\n"
    		,timer_id,us,periodic);

	tiP = (PTObjInfo) timer_id;
    if ((timer_id == 0) || ((tiP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid timer ID 0x%llX\n",timer_id);
		rv = ZOE_ERRS_PARMS;
        goto timer_startExit;
	}

	if (tiP->type == OBJTYPE_TIMER) {
		rv = OSTimerStart(tiP,us,periodic);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",tiP->type);
		rv = ZOE_ERRS_INVALID;
		goto timer_startExit;
	}

timer_startExit:
	zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
	return (rv);
}


// Stops a running timer; the timeout procedure may or may not be called depending on system delays
zoe_errs_t
zoe_sosal_timer_stop (zoe_sosal_obj_id_t timer_id)
{
	zoe_errs_t		rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       tiP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, timer_id(0x%llX)\n",timer_id);

	tiP = (PTObjInfo) timer_id;
    if ((timer_id == 0) || ((tiP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid timer ID 0x%llX\n",timer_id);
		rv = ZOE_ERRS_PARMS;
        goto timer_stopExit;
	}

	if (tiP->type == OBJTYPE_TIMER) {
		rv = OSTimerStop(tiP);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",tiP->type);
		rv = ZOE_ERRS_INVALID;
		goto timer_stopExit;
	}

timer_stopExit:
	zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
	return (rv);
}


