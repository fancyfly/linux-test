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
// Definitions common to Host and Device OSes
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Sep 20, 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OS_STUBS_H__
#define __OS_STUBS_H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"
#include "zoe_sosal_priv.h"

typedef enum {
    OBJTYPE_VOID = ZOE_FOURCC('v','o','i','d'),
    OBJTYPE_THREAD = ZOE_FOURCC('t','h','r','d'),
    OBJTYPE_MUTEX = ZOE_FOURCC('m','u','t','x'),
    OBJTYPE_EVENT = ZOE_FOURCC('e','v','n','t'),
    OBJTYPE_SPINLOCK = ZOE_FOURCC('s','p','l','k'),
    OBJTYPE_SEMAPHORE = ZOE_FOURCC('s','e','m','a'),
    OBJTYPE_TIMER = ZOE_FOURCC('t','m','e','r')

    // Don't forget to update the objTypeValid macro below when adding/removing object types
} EObjTypes;

#define zoe_object_type(type) \
	((type==OBJTYPE_VOID)		? "void" : \
	 (type==OBJTYPE_THREAD)		? "thrd" : \
	 (type==OBJTYPE_MUTEX)		? "mutx" : \
	 (type==OBJTYPE_EVENT)		? "evnt" : \
	 (type==OBJTYPE_SPINLOCK)	? "splk" : \
	 (type==OBJTYPE_SEMAPHORE)	? "sema" : \
	 (type==OBJTYPE_TIMER)		? "tmer" : \
	 "UNKNOWN")

#define objTypeValid(type) ((type == OBJTYPE_THREAD) || (type == OBJTYPE_MUTEX) || (type == OBJTYPE_EVENT)	\
						|| (type == OBJTYPE_SPINLOCK) || (type == OBJTYPE_SEMAPHORE) || (type == OBJTYPE_TIMER))

// Every OS specific implementation must define an object info structure
// with the following fields at the beginning:
#define _OBJ_INFO_HEADER \
	EObjTypes				type;   \
	zoe_sosal_obj_name_t	name;	\
	void *					clientData;

#define OBJ_INFO_HEADER 	_OBJ_INFO_HEADER

#define EVENT_INFO_HEADER \
	zoe_bool_t          set;		\
	PTEventWaiter       waiterP;	\
	uint32_t            waitIdx;


// Generic object block structure (the OS specific stuff is added at the end)
typedef struct _TObjInfo {
    OBJ_INFO_HEADER
} TObjInfo, *PTObjInfo;


// Special case for the event object block structure since the stub handles the
// multiple events (the OS specific stuff is added at the end)
typedef struct {
    zoe_sosal_event_wait_ptr_t	eventsList;
    uint32_t					eventsNum;
    uint32_t					eventsLeft;
    zoe_bool_t					all;
    struct _TEventInfo *		eventP;
} TEventWaiter, *PTEventWaiter;

typedef struct _TEventInfo {
    OBJ_INFO_HEADER
    EVENT_INFO_HEADER
} TEventInfo, *PTEventInfo;


extern zoe_bool_t OSSosalInit (void);
extern void OSSosalSetHalP (void * halP);
extern void * OSSosalGetHalP (void);
extern void OSSosalTerm (void);
extern void OSSosalIdle (void);
extern void OSSosalIRQ (void);

extern void OSInfo (zoe_sosal_info_ptr_t info_ptr);


//////////////////////
// Threads          //
//////////////////////

extern uint32_t OSThreadMaxprioritiesGet (void);
extern void * OSThreadObjectAlloc (void);
extern void OSThreadObjectFree (void * threadObjP);
extern zoe_errs_t OSThreadCreate (void * threadObjP, zoe_sosal_thread_parms_ptr_t parmsP);
extern zoe_errs_t OSThreadDelete (void * threadObjP);
extern zoe_sosal_obj_id_t OSThreadGetID (void);
extern zoe_errs_t OSThreadSleepMS (uint32_t milliseconds);
extern zoe_errs_t OSThreadSleepUS (uint32_t microseconds);
extern zoe_errs_t OSThreadPriorityGet (void * threadObjP, uint32_t * priorityP);
extern zoe_errs_t OSThreadPrioritySet (void * threadObjP, uint32_t priority);
extern zoe_errs_t OSThreadAbort (void * threadObjP);
extern int OSThreadCPUIndex (void);


//////////////////////
//   Time           //
//////////////////////

extern zoe_sosal_ticks_t OSTimeSysTicks (void);
extern uint64_t OSTimeTicksPerSecond (void);


//////////////////////////
//	Synchronization     //
//////////////////////////

// Events.

extern void * OSEventObjectAlloc (void);
extern void OSEventObjectFree (void * eventObjP);
extern zoe_errs_t OSEventCreate (void * eventObjP);
extern zoe_errs_t OSEventDelete (void * eventObjP);
extern zoe_errs_t OSEventReset (void * eventObjP);
extern zoe_errs_t OSEventPost (void * eventObjP);
extern zoe_errs_t OSEventWait (void * eventObjP, int32_t microseconds);
extern zoe_bool_t OSEventWaited (void * eventObjP);

// Mutexes

extern void * OSMutexObjectAlloc (void);
extern void OSMutexObjectFree (void * mutexObjP);
extern zoe_errs_t OSMutexCreate (void * mutexObjP);
extern zoe_errs_t OSMutexDelete (void * mutexObjP);
extern zoe_errs_t OSMutexRelease (void * mutexObjP);
extern zoe_errs_t OSMutexGet (void * mutexObjP, int32_t milliseconds);

// Spinlocks

extern void * OSSpinlockObjectAlloc (void);
extern void OSSpinlockObjectFree (void * spinlockObjP);
extern zoe_errs_t OSSpinlockCreate (void * spinlockObjP);
extern zoe_errs_t OSSpinlockDelete (void * spinlockObjP);
extern zoe_errs_t OSSpinlockRelease (void * spinlockObjP);
extern zoe_errs_t OSSpinlockGet (void * spinlockObjP);

// Semaphores

extern void * OSSemaphoreObjectAlloc (void);
extern void OSSemaphoreObjectFree (void * semaphoreObjP);
extern zoe_errs_t OSSemaphoreCreate (void * semaphoreObjP, zoe_sosal_semaphore_parms_ptr_t parms_ptr);
extern zoe_errs_t OSSemaphoreDelete (void * semaphoreObjP);
extern zoe_errs_t OSSemaphoreGive (void * semaphoreObjP);
extern zoe_errs_t OSSemaphoreTake (void * semaphoreObjP, int32_t milliseconds);
extern zoe_errs_t OSSemaphoreReset (void * semaphoreObjP);



//////////////////////////
//	Timers				//
//////////////////////////

extern void * OSTimerObjectAlloc (void);
extern void OSTimerObjectFree (void * timerObjP);
extern zoe_errs_t OSTimerCreate (void * timerObjP, zoe_sosal_time_timer_proc_t proc, void * parm);
extern zoe_errs_t OSTimerDelete (void * timerObjP);
extern zoe_errs_t OSTimerStart (void * timerObjP, uint64_t us, zoe_bool_t periodic);
extern zoe_errs_t OSTimerStop (void * timerObjP);


//////////////////////////
//  Memory management   //
//////////////////////////

extern void * OSMemoryAlloc (zoe_sosal_memory_pools_t pool, uint32_t size, uint32_t alignment);
extern void OSMemoryFree (void * mem_ptr);
extern void * OSMemoryCached (void * noncached_ptr);
extern void * OSMemoryNoncached (void * cached_ptr);
extern uint32_t OSMemoryCachelineLog2 (void);
extern zoe_errs_t OSMemoryCacheClean (void * start_ptr, uint32_t lines);
extern zoe_errs_t OSMemoryCacheInval (void * start_ptr, uint32_t lines);
extern void * OSMemoryGetPhy (void * vir_ptr);
extern void * OSMemoryGetVir (void * phy_ptr);
extern zoe_bool_t OSMemoryIsValidVir (void * vir_ptr);


//////////////////////////
// ISR handling
//////////////////////////

extern zoe_bool_t OSInISR (void);

extern int32_t OSISRMySWISRNum (void);
extern zoe_errs_t OSISRSWHandlerInstall (int32_t from_num,
										zoe_sosal_isr_sw_proc_t proc, void * ctxt);
extern zoe_errs_t OSISRSWEnableStateSet (int32_t from_num,
                                         zoe_bool_t * enable_ptr);
extern zoe_errs_t OSISRSWTrigger (int32_t to_num);
extern zoe_errs_t OSISRSWClear (int32_t from_num);

extern zoe_errs_t OSISRHWHandlerInstall (uint8_t bank_num, uint8_t bit_num, zoe_bool_t pulse, zoe_sosal_isr_hw_proc_t proc, void * ctxt, zoe_sosal_affinity_ptr_t affinity_ptr);
extern zoe_errs_t OSISRHWEnableStateGet (uint8_t bank_num, uint8_t bit_num, zoe_bool_t * enable_ptr);
extern zoe_errs_t OSISRHWEnableStateSet (uint8_t bank_num, uint8_t bit_num, zoe_bool_t * enable_ptr);



//////////////////////////
// SOSAL utility functions
//////////////////////////


extern void InitObj (PTObjInfo, EObjTypes type, const zoe_sosal_obj_name_t);
extern void TermObj (PTObjInfo oiP);


#ifdef __cplusplus
}
#endif

#endif //__OS_STUBS_H__
