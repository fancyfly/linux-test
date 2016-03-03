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
*                 (C) COPYRIGHT 2012-2014 Zenverge Inc.                       *
*                        ALL RIGHTS RESERVED                                  *
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
// zoe sosal Semaphore management entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: May 25, 2012
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
// Semaphores
// Used for resource management.
// Different threads can acquire and release a semaphore.  An ISR can release a semaphore.
// Semaphores could be used for critical zone protection, but it is not recommended since
// they do not provide priority inversion resolution: Mutexes must be used instead.
// Semaphores also provide resource counting.
//

// Create a new semaphore with given max and initial counts; the semaphore's id is returned in *semaphore_id_ptr
zoe_errs_t
zoe_sosal_semaphore_create (zoe_sosal_semaphore_parms_ptr_t		parms_ptr,
							const zoe_sosal_obj_name_t			name,
							zoe_sosal_obj_id_t *				semaphore_id_ptr)
{
    zoe_errs_t				rv = ZOE_ERRS_SUCCESS;
    PTObjInfo				siP = ZOE_NULL;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, parms_ptr (%p), semaphore_id_ptr(%p) name(%p)\n"
        ,parms_ptr,semaphore_id_ptr,name);

    if (semaphore_id_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL semaphore ID pointer\r\n");
        rv = ZOE_ERRS_PARMS;
        goto semaphore_createExit;
    }

    if ((parms_ptr != ZOE_NULL) && ((parms_ptr->init_count > parms_ptr->max_count) || (parms_ptr->max_count == 0))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid count parameters: max_count(%d), init_count(%d)\r\n"
        		,parms_ptr->max_count,parms_ptr->init_count);
        rv = ZOE_ERRS_PARMS;
        goto semaphore_createExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto semaphore_createExit;
	}

    siP = (PTObjInfo) OSSemaphoreObjectAlloc();
    if (siP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSSemaphoreObjectAlloc failed\n");
        rv = ZOE_ERRS_NOMEMORY;
        goto semaphore_createExit;
    }

	InitObj((PTObjInfo) siP,OBJTYPE_SEMAPHORE,name);
    rv = OSSemaphoreCreate(siP,parms_ptr);
    if (ZOE_FAIL(rv)) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSSemaphoreCreate failed (%lu)\n",rv);
        goto semaphore_createExit;
    }

    *semaphore_id_ptr = siP;

semaphore_createExit:
	if (ZOE_FAIL(rv)) {
		if (siP != ZOE_NULL) {
			TermObj((PTObjInfo) siP);
			OSSemaphoreObjectFree(siP);
		}
	}
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Delete semaphore
zoe_errs_t
zoe_sosal_semaphore_delete (zoe_sosal_obj_id_t semaphore_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, semaphore_id(0x%llX)\n",semaphore_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto semaphore_deleteExit;
	}

	siP = (PTObjInfo) semaphore_id;
    if ((semaphore_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid semaphore ID 0x%llX\n",semaphore_id);
		rv = ZOE_ERRS_PARMS;
        goto semaphore_deleteExit;
	}

	if (siP->type != OBJTYPE_SEMAPHORE) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		rv = ZOE_ERRS_INVALID;
        goto semaphore_deleteExit;
	}

	rv = OSSemaphoreDelete(siP);
	if (ZOE_FAIL(rv)) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSSemaphoreDelete failed (%lu)\n",rv);
        goto semaphore_deleteExit;
	}

	// Should the semaphore object be freed also when OSSemaphoreDelete fails?
	TermObj(siP);
	OSSemaphoreObjectFree(siP);

semaphore_deleteExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Give a unit back to a semaphore; might cause a waiting thread to wake up
zoe_errs_t
zoe_sosal_semaphore_give (zoe_sosal_obj_id_t semaphore_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, semaphore_id(0x%llX)\n",semaphore_id);

	siP = (PTObjInfo) semaphore_id;
    if ((semaphore_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid semaphore ID 0x%llX\n",semaphore_id);
		rv = ZOE_ERRS_PARMS;
        goto semaphore_giveExit;
	}

	if (siP->type != OBJTYPE_SEMAPHORE) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		rv = ZOE_ERRS_INVALID;
        goto semaphore_giveExit;
	}

	rv = OSSemaphoreGive(siP);

semaphore_giveExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Take a unit from a semaphore.
// 'microseconds' is interpreted as follows:
//  > 0: number of microseconds to wait
//  = 0: return immediately (poll for semaphore's availability)
//  < 0: wait indefinitely
zoe_errs_t
zoe_sosal_semaphore_take (zoe_sosal_obj_id_t semaphore_id, int32_t microseconds)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, semaphore_id(0x%llX)\n",semaphore_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto semaphore_takeExit;
	}

	siP = (PTObjInfo) semaphore_id;
    if ((semaphore_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid semaphore ID 0x%llX\n",semaphore_id);
		rv = ZOE_ERRS_PARMS;
        goto semaphore_takeExit;
	}

	if (siP->type != OBJTYPE_SEMAPHORE) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		rv = ZOE_ERRS_INVALID;
		goto semaphore_takeExit;
	}

	rv = OSSemaphoreTake(siP,microseconds);

semaphore_takeExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Return a semaphore to its initial state; error if there are waiting threads
zoe_errs_t
zoe_sosal_semaphore_reset (zoe_sosal_obj_id_t semaphore_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, semaphore_id(0x%llX)\n",semaphore_id);

	siP = (PTObjInfo) semaphore_id;
    if ((semaphore_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid semaphore ID 0x%llX\n",semaphore_id);
		rv = ZOE_ERRS_PARMS;
        goto semaphore_resetExit;
	}

	if (siP->type != OBJTYPE_SEMAPHORE) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		rv = ZOE_ERRS_INVALID;
		goto semaphore_resetExit;
	}

	rv = OSSemaphoreReset(siP);

semaphore_resetExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}



