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
*                          ALL RIGHTS RESERVED                                *
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
// zoe sosal Event Management entry points implementation
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

extern TOSLockID EventsLockID;

///////////////////////////////////////////////////////////////////////////////
//
// Events.
// Can be set or cleared; when set they can wake up a thread; equivalent to
// FreeRTOS' binary semaphores; generally used by a thread (or an ISR) to wake up
// another thread.
//

// Create a new event object; event id is returned in *event_id_ptr
zoe_errs_t
zoe_sosal_event_create (const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * event_id_ptr)
{
    zoe_errs_t				rv = ZOE_ERRS_SUCCESS;
    PTEventInfo				eiP = ZOE_NULL;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, name(%p) event_id_ptr(%p)\n",name,event_id_ptr);

    if (event_id_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL event_id_ptr\n");
        rv = ZOE_ERRS_PARMS;
        goto event_createExit;
    }

    eiP = (PTEventInfo) OSEventObjectAlloc();
    if (eiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSEventObjectAlloc failed\n");
        rv = ZOE_ERRS_NOMEMORY;
        goto event_createExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto event_createExit;
	}

	InitObj((PTObjInfo) eiP,OBJTYPE_EVENT,name);
	rv = OSEventCreate(eiP);
    if (ZOE_FAIL(rv)) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSEventCreate failed (%d)\n",rv);
        goto event_createExit;
    }

    eiP->set = ZOE_FALSE;
    eiP->waiterP = ZOE_NULL;
    *event_id_ptr = eiP;

event_createExit:
    if (ZOE_FAIL(rv)) {
        if (eiP != ZOE_NULL) {
            TermObj((PTObjInfo) eiP);
            OSEventObjectFree(eiP);
        }
    }
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d) %X\n",rv,eiP);
    return (rv);
}


// Delete an event object
zoe_errs_t
zoe_sosal_event_delete (zoe_sosal_obj_id_t event_id)
{
    zoe_errs_t          rv = ZOE_ERRS_SUCCESS;
    PTEventInfo         eiP;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, event_id(0x%llX)\n",event_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto event_deleteExit;
	}

    if (event_id == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL event ID\n");
        rv = ZOE_ERRS_PARMS;
        goto event_deleteExit;
    }

    OSLockGet(EventsLockID);

	eiP = (PTEventInfo) event_id;
    if (eiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid event ID 0x%llX\n",event_id);
        rv = ZOE_ERRS_PARMS;
    }else{
		if ((eiP != ZOE_NULL) && (eiP->type == OBJTYPE_EVENT)) {
			// cannot delete if threads are waiting on it
			if (eiP->waiterP != ZOE_NULL) {
				if (OSEventWaited(eiP->waiterP->eventP)) {
					zoe_dbg_printf_e(ZoeSosalDbgCompID,"event is being waited upon\n");
					rv = ZOE_ERRS_FAIL;
				}
			}else{
				TermObj((PTObjInfo) eiP);
				(void) OSEventDelete(eiP);
				OSEventObjectFree(eiP);
			}
		}else{
			zoe_dbg_printf_e(ZoeSosalDbgCompID,"not a valid event object (%X)\n",eiP->type);
			rv = ZOE_ERRS_INVALID;
		}
    }
	OSLockRelease(EventsLockID);

event_deleteExit:
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Set an event; if any thread is waiting on it, it will wake up
zoe_errs_t
zoe_sosal_event_set (zoe_sosal_obj_id_t event_id)
{
    zoe_errs_t      rv = ZOE_ERRS_SUCCESS;
    PTEventInfo     eiP;
	void * 			eP = ZOE_NULL;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, event_id(0x%llX)\n",event_id);

    if (event_id == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL event ID\n");
        rv = ZOE_ERRS_PARMS;
        goto event_setExit;
    }

    OSLockGet(EventsLockID);

    eiP = (PTEventInfo) event_id;
    if (eiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid event ID 0x%llX\n",event_id);
        rv = ZOE_ERRS_PARMS;
    }else{
		if (eiP->type == OBJTYPE_EVENT) {
			PTEventWaiter   ewP = eiP->waiterP;

			eiP->set = ZOE_TRUE;
			// check if threads are waiting on it
			if (ewP != ZOE_NULL) {
				if (!ewP->eventsList[eiP->waitIdx].fired) {
					// mark this event as 'fired'
					ewP->eventsList[eiP->waitIdx].fired = ZOE_TRUE;
					// and decrement the events left count
					ewP->eventsLeft--;
				}
				// wake up the waiting thread if the condition has been reached
				if (!ewP->all || (ewP->eventsLeft == 0)) {
					eP = ewP->eventP;
				}
			}
		}else{
			zoe_dbg_printf_e(ZoeSosalDbgCompID,"not a valid event object (%X)\n",eiP->type);
			rv = ZOE_ERRS_INVALID;
		}
    }

	OSLockRelease(EventsLockID);

	if (eP != ZOE_NULL) {
		rv = OSEventPost(eP);
	}

event_setExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Clear an event
zoe_errs_t
zoe_sosal_event_clear (zoe_sosal_obj_id_t event_id)
{
    zoe_errs_t          rv = ZOE_ERRS_SUCCESS;
    PTEventInfo         eiP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, event_id(0x%llX)\n",event_id);

    if (event_id == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL event ID\n");
        rv = ZOE_ERRS_PARMS;
        goto event_clearExit;
    }

    OSLockGet(EventsLockID);

    eiP = (PTEventInfo) event_id;
    if (eiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid event ID 0x%llX\n",event_id);
        rv = ZOE_ERRS_PARMS;
    }else{
		if (eiP->type == OBJTYPE_EVENT) {
			// just clear the flag; this event might have been set earlier and
			// waken up a thread.  There is no action to do if a thread is waiting
			// on this event (cannot 'unwakeup' a thread)
			eiP->set = ZOE_FALSE;
		}else{
			zoe_dbg_printf_e(ZoeSosalDbgCompID,"not a valid event object (%X)\n",eiP->type);
			rv = ZOE_ERRS_INVALID;
		}
    }
	OSLockRelease(EventsLockID);

event_clearExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


//
// Wait on one or more events; can wait on any set of event objects that are
// not being waited upon by another thread; if all is FALSE, the first event
// that fires (is set) within the given timeout will make this function return;
// if all is TRUE the call returns when all events are fired within the given
// timeout
//
// 'microseconds' is interpreted as follows:
//      > 0: number of microseconds to wait
//      = 0: return immediately (poll for object's availability)
//      < 0: wait indefinitely
// Arguments are a list of events (array pointer and number of elements) and
// the timeout value.  If the event_id field is NULL, it is ignored.
// Return ZOE_ERRS_TIMEOUT if times out before any synch objects fires.
// Return ZOE_ERRS_SUCCESS if one or more synch objects fired; the objects
// that fired will have the 'fired 'field set to TRUE; more than one object
// could have fired.
//
zoe_errs_t
zoe_sosal_events_wait (zoe_sosal_event_wait_ptr_t	events_list,
						uint32_t                	events_num,
						int32_t						microseconds,
						zoe_bool_t					all)
{
    zoe_errs_t          rv = ZOE_ERRS_SUCCESS;
    uint32_t            i;
    int                 setCount = 0;
    TEventWaiter        evW;
    zoe_bool_t          notDone;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, events_list(%p) events_num(%d) microseconds(%d) all(%d)\n"
        ,events_list,events_num,microseconds,all);

    if ((events_list == ZOE_NULL) || (events_num == 0)) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"empty event list, list(%X) num(%d)\n",events_list,events_num);
        rv = ZOE_ERRS_PARMS;
        goto events_waitExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto events_waitExit;
	}

    // Initialize the Waiter structure
    evW.eventsList = events_list;
    evW.eventsNum = events_num;
    evW.eventsLeft = 0;
    evW.all = all;
    for (i = 0; i < events_num; i++) {
        if (events_list[i].event_id != ZOE_NULL) {
			// All events start in the non-fired state
			events_list[i].fired = ZOE_FALSE;
            evW.eventsLeft++;
        }
    }

    // error if there were no valid events in the list
    if (evW.eventsLeft == 0) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"no valid events in the list\n");
        rv = ZOE_ERRS_PARMS;
        goto events_waitExit;
    }

	OSLockGet(EventsLockID);
	// Check first if events are already set, in which case we might be able to return
	// immediately.
	for (i = 0; i < events_num; i++) {
		PTEventInfo     eiP = (PTEventInfo) events_list[i].event_id;

		if (eiP != ZOE_NULL) {
			if (eiP->type != OBJTYPE_EVENT) {
				zoe_dbg_printf_e(ZoeSosalDbgCompID,"not a valid event object, #%d (%X)\n",i,eiP->type);
				rv = ZOE_ERRS_FAIL;
				break;
			}
			if (eiP->waiterP != ZOE_NULL) {
				if (OSEventWaited(eiP->waiterP->eventP)) {
					// One of the events is already being waited upon
					zoe_dbg_printf_e(ZoeSosalDbgCompID,"event #%d (%X) is being waited upon already\n",i,eiP);
					rv = ZOE_ERRS_INVALID;
					break;
				}
			}
			if (eiP->set) {
				setCount++;
				events_list[i].fired = ZOE_TRUE;
				evW.eventsLeft--;
			}
		}
	}
	notDone = (setCount == 0) || (all && (evW.eventsLeft > 0));
	if (ZOE_SUCCESS(rv) && (microseconds == 0) && notDone) {
		rv = ZOE_ERRS_TIMEOUT;
	}
	if (ZOE_SUCCESS(rv)) {
		if ((microseconds != 0) && notDone) {
			// We must wait.
			// Setup the events
			for (i = 0; i < events_num; i++) {
				PTEventInfo     eiP = (PTEventInfo) events_list[i].event_id;

				if (eiP != ZOE_NULL) {
					// Must pick one of the OS events;
					// use the last valid one in the list
					evW.eventP = eiP;
					eiP->waiterP = &evW;
					eiP->waitIdx = i;
				}
			}
			rv = OSEventReset(evW.eventP);
			if (ZOE_FAIL(rv)) {
				zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSEventReset failed (%d)\n",rv);
			}else{
				OSLockRelease(EventsLockID);
				rv = OSEventWait(evW.eventP,microseconds);
				if (ZOE_FAIL(rv) && (rv != ZOE_ERRS_TIMEOUT) && (rv != ZOE_ERRS_CANCELLED)) {
					zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSEventWait failed (%d)\n",rv);
				}
				OSLockGet(EventsLockID);
			}
		}
	}
	// Mark the events as no longer being waited upon and clear all fired events
	// (autoclear events) if successful
	for (i = 0; i < events_num; i++) {
		PTEventInfo     eiP = (PTEventInfo) events_list[i].event_id;

		if (eiP != ZOE_NULL) {
			eiP->waiterP = ZOE_NULL;
			if (ZOE_SUCCESS(rv)) {
				if (events_list[i].fired) {
					eiP->set = ZOE_FALSE;
				}
			}
		}
	}
	OSLockRelease(EventsLockID);

events_waitExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


//
// Single event waiter function
//
zoe_errs_t
zoe_sosal_event_wait (zoe_sosal_obj_id_t event_id, int32_t microseconds)
{
    zoe_errs_t          rv = ZOE_ERRS_SUCCESS;
    PTEventInfo         eiP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, event_id(0x%llX) microseconds(%d)\n"
        ,event_id,microseconds);

    if (event_id == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL event ID\n");
        rv = ZOE_ERRS_PARMS;
        goto event_waitExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto event_waitExit;
	}

	OSLockGet(EventsLockID);

	eiP = (PTEventInfo) event_id;
    if (eiP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid event ID 0x%llX\n",event_id);
        rv = ZOE_ERRS_PARMS;
        goto event_waitExit;
    }

	if (eiP->type != OBJTYPE_EVENT) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"not a valid event object (%X)\n",eiP->type);
		rv = ZOE_ERRS_FAIL;
	}else if (eiP->waiterP != ZOE_NULL) {
		if (OSEventWaited(eiP->waiterP->eventP)) {
			zoe_dbg_printf_e(ZoeSosalDbgCompID,"event 0x%llX is being waited upon already\n",event_id);
			// The event is already being waited upon
			rv = ZOE_ERRS_INVALID;
		}
	}
	if (ZOE_SUCCESS(rv) && !eiP->set) {
		if (microseconds == 0) {
			rv = ZOE_ERRS_TIMEOUT;
		}else{
			TEventWaiter         	evW;
			zoe_sosal_event_wait_t	ew;

			ew.event_id = event_id;
			evW.eventsList = &ew;
			evW.eventsNum = 1;
			evW.eventsLeft = 1;
			evW.all = ZOE_FALSE;
			evW.eventP = eiP;
			eiP->waiterP = &evW;
			eiP->waitIdx = 0;
			rv = OSEventReset(evW.eventP);
			if (ZOE_FAIL(rv)) {
				zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSEventReset failed (%d)\n",rv);
			}else{
				OSLockRelease(EventsLockID);
				rv = OSEventWait(evW.eventP,microseconds);
				if (ZOE_FAIL(rv) && (rv != ZOE_ERRS_TIMEOUT) && (rv != ZOE_ERRS_CANCELLED)) {
					zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSEventWait failed (%d)\n",rv);
				}
				OSLockGet(EventsLockID);
			}
		}
	}
	eiP->waiterP = ZOE_NULL;
	if (ZOE_SUCCESS(rv)) {
		// clear the event (autoclear events)
		eiP->set = ZOE_FALSE;
	}
	OSLockRelease(EventsLockID);

event_waitExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}




