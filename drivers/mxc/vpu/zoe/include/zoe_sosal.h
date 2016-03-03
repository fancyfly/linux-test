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
// zoe_sosal.h  OS Abstraction Layer
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_SOSAL_H__
#define __ZOE_SOSAL_H__

#include "zoe_types.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#define ZOE_SOSAL_VERSION	1

// Get OS info
typedef enum {
    ZOE_SOSAL_OSTYPE_ZEOS = 0,
    ZOE_SOSAL_OSTYPE_WINDOWS_USER,
    ZOE_SOSAL_OSTYPE_WINDOWS_KERNEL,
    ZOE_SOSAL_OSTYPE_LINUX_USER,
    ZOE_SOSAL_OSTYPE_LINUX_KERNEL,
    ZOE_SOSAL_OSTYPE_FREERTOS
} zoe_sosal_ostypes_t;

typedef struct _zoe_sosal_info_t {
    zoe_sosal_ostypes_t     os_type;
    uint32_t            	sosal_version;
    uint32_t            	major_version;
    uint32_t            	minor_version;
    uint8_t					cores_num;
    uint8_t					vpes_per_core;
} zoe_sosal_info_t, *zoe_sosal_info_ptr_t;

zoe_errs_t zoe_sosal_info (zoe_sosal_info_ptr_t info_ptr);


// Get the pointer to the HAL object
void * zoe_sosal_get_hal_ptr (void);

// Generic object ID used to identify any sosal object
typedef void * zoe_sosal_obj_id_t;

typedef enum {
	ZOE_SOSAL_OBJECT_TYPE_ANY = -1,
	ZOE_SOSAL_OBJECT_TYPE_THREAD = 0,
	ZOE_SOSAL_OBJECT_TYPE_EVENT,
	ZOE_SOSAL_OBJECT_TYPE_MUTEX,
	ZOE_SOSAL_OBJECT_TYPE_SEMAPHORE,
	ZOE_SOSAL_OBJECT_TYPE_SPINLOCK,
	ZOE_SOSAL_OBJECT_TYPE_TIMER,

	ZOE_SOSAL_OBJECT_TYPE_NUM
} zoe_sosal_object_types_t;

// Max length of an object name
#define ZOE_SOSAL_OBJNAME_MAXLEN 64
// Array type for objects' names (NULL terminated or max length)
typedef char zoe_sosal_obj_name_t[ZOE_SOSAL_OBJNAME_MAXLEN];

// Set an object array with a given string; return the name length (excluding the NULL terminator)
void zoe_sosal_obj_name_set (zoe_sosal_obj_name_t name_ptr, const char * string_ptr);

// Get an object's name (if obj_id is NULL it applies to the running thread)
zoe_errs_t zoe_sosal_obj_name_get (zoe_sosal_obj_id_t obj_id, zoe_sosal_obj_name_t name_ptr);

// Get an object's ID given its name and type (unpredictable if names are duplicated)
zoe_errs_t zoe_sosal_obj_id_from_name (zoe_sosal_obj_name_t name_ptr, zoe_sosal_object_types_t type, zoe_sosal_obj_id_t * id_ptr);

// Get an object's type
zoe_errs_t zoe_sosal_obj_type (zoe_sosal_obj_id_t obj_id, zoe_sosal_object_types_t * type_ptr);

// Set an object's client data (if obj_id is NULL it applies to the running thread)
zoe_errs_t zoe_sosal_obj_client_data_set (zoe_sosal_obj_id_t obj_id, void * client_data);

// Get an object's client data (if obj_id is NULL it applies to the running thread)
zoe_errs_t zoe_sosal_obj_client_data_get (zoe_sosal_obj_id_t obj_id, void ** client_data_ptr);


// Initialization and termination entry points
zoe_bool_t zoe_sosal_init (void);
void zoe_sosal_term (void);



//////////////////////
//	Affinity parameters
//

// When an execution unit (EU: thread or ISR) is created, an affinity policy
// is assigned to it.

typedef enum {
	ZOE_SOSAL_AFFINITY_POLICY_OS = 0,			// defaul policy: OS decides scheduling
	ZOE_SOSAL_AFFINITY_POLICY_STRICT_CORE,		// keep on requested core
	ZOE_SOSAL_AFFINITY_POLICY_STRICT_CORE_VPE,	// keep on requested core and vpe

	ZOE_SOSAL_AFFINITY_POLICY_NUM
} zoe_sosal_affinity_policies_t;


typedef struct _zoe_sosal_affinity_t {
	zoe_sosal_affinity_policies_t	policy;
	uint8_t							core;		// selects the core if policy is CORE or VPE
	uint8_t							vpe;		// selects the vpe if policy is VPE
} zoe_sosal_affinity_t, *zoe_sosal_affinity_ptr_t;



/////////////////////
// Threads

// The type of the thread procedure
typedef void (*zoe_sosal_thread_proc_t) (void *);


// Parameters to define a new thread
typedef struct _zoe_sosal_thread_parms_t {
    zoe_sosal_thread_proc_t     proc;               // pointer to the thread's entry point
    void *                      parm;               // parameter passed to the thread
    void *                      client_data;        // custom data to be stored by the OS
    uint32_t                	stack;              // minimum size of stack in bytes
    uint32_t                	priority;           // priority level (0 = lowest up to max-1)
    zoe_sosal_affinity_t		affinity;			// requested affinity
} zoe_sosal_thread_parms_t, *zoe_sosal_thread_parms_ptr_t;

// Return the number of priorities supported; thread priorities can range between 0 (lowest priority)
// and (maxpriorities - 1).
uint32_t zoe_sosal_thread_maxpriorities_get (void);

// Create and start a new thread; the thread's ID is returned in *thread_id_ptr
zoe_errs_t zoe_sosal_thread_create (zoe_sosal_thread_parms_ptr_t parms_ptr, const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * thread_id_ptr);

// Delete an existing thread
zoe_errs_t zoe_sosal_thread_delete (zoe_sosal_obj_id_t thread_id);

// Get a thread's priority
zoe_errs_t zoe_sosal_thread_priority_get (zoe_sosal_obj_id_t thread_id, uint32_t * priority_ptr);

// Change a thread's priority
zoe_errs_t zoe_sosal_thread_priority_set (zoe_sosal_obj_id_t thread_id, uint32_t priority);

// Return executing thread's ID
zoe_sosal_obj_id_t zoe_sosal_thread_get_id (void);

// Suspend executing thread for a number of milliseconds
zoe_errs_t zoe_sosal_thread_sleep_ms (uint32_t milliseconds);

// Suspend executing thread for a number of microseconds
zoe_errs_t zoe_sosal_thread_sleep_us (uint32_t microseconds);

// Abort a suspended thread
zoe_errs_t zoe_sosal_thread_abort (zoe_sosal_obj_id_t thread_id);


///////////////////
// Time

// The OS keeps an absolute tick count, usually counts the clock periods; this count can be huge
// and should 'never' wrap around: 64-bit @ 1GHz wraps around every 583 years
typedef uint64_t zoe_sosal_ticks_t;

// Return the number of system ticks elapsed since system startup
zoe_sosal_ticks_t zoe_sosal_time_sys_ticks (void);

// Return the number of system ticks per second (usually the clock frequency)
uint64_t zoe_sosal_time_ticks_per_second (void);


////////////////////
//	Synchronization

// Events.
// Can be set or cleared; when set they can wake up a thread; generally used
// by a thread (or an ISR) to wake up another thread.
// Events are auto-clear (automatically cleared when a thread is woken up).
// Only one thread can be waiting on a given event at any time.

// Create a new event object; the event's id is returned in *event_id_ptr
zoe_errs_t zoe_sosal_event_create (const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * event_id_ptr);

// Delete an event object
zoe_errs_t zoe_sosal_event_delete (zoe_sosal_obj_id_t event_id);

// Set an event; if any thread is waiting on it, it will wake up
zoe_errs_t zoe_sosal_event_set (zoe_sosal_obj_id_t event_id);

// Clear an event
zoe_errs_t zoe_sosal_event_clear (zoe_sosal_obj_id_t event_id);

// Wait on an event; no other threads can be waiting on the same event.
// The 'microseconds' timeout value is interpreted as follows:
//      > 0: number of microseconds to wait
//      = 0: return immediately (i.e. poll for event's availability)
//      < 0: wait indefinitely
// Return ZOE_ERRS_TIMEOUT if times out before the event is set.
// Return ZOE_ERRS_SUCCESS when the event is set.

zoe_errs_t zoe_sosal_event_wait (zoe_sosal_obj_id_t event_id, int32_t microseconds);

// Wait on one or more events; can wait on any set of event objects that are
// not being waited on by another thread; if 'all' is FALSE, the first event
// that fires (is set) within the given timeout will make this function return;
// if 'all' is TRUE the call returns when all events are fired within the given
// timeout.
// Arguments are: pointer, size of an array of events to wait on, the
// timeout value.  If an event_id field in the array of events is NULL, that event
// is ignored.
// The 'microseconds' timeout value is interpreted as follows:
//      > 0: number of microseconds to wait
//      = 0: return immediately (i.e. poll for events' availability)
//      < 0: wait indefinitely
// Return ZOE_ERRS_TIMEOUT if times out before the terminating condition occurs.
// Return ZOE_ERRS_SUCCESS if one or more synch objects fired; the objects that fired
// will have the 'fired 'field set to TRUE; more than one object could have fired

typedef struct {
    zoe_sosal_obj_id_t          event_id;
    zoe_bool_t                  fired;
} zoe_sosal_event_wait_t, *zoe_sosal_event_wait_ptr_t;

zoe_errs_t zoe_sosal_events_wait (zoe_sosal_event_wait_ptr_t events_list,
                                        uint32_t events_num,
                                        int32_t microseconds,
                                        zoe_bool_t all);



// Mutexes
// Used for mutual exclusion; the thread that obtains the mutex must be the one
// that releases it.
// To mitigate priority inversion issues, the thread that obtains a mutex might
// have its priority raised to the highest priority among the threads that are
// waiting on it.
// Mutexes cannot be used in ISRs.

// Create a new mutex; the mutex' id is returned in *mutex_id_ptr; a mutex is
// created in the "released" state
zoe_errs_t zoe_sosal_mutex_create (const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * mutex_id_ptr);

// Delete mutex
zoe_errs_t zoe_sosal_mutex_delete (zoe_sosal_obj_id_t mutex_id);

// Get (lock) a mutex.
// 'microseconds' is interpreted as follows:
//  > 0: number of microseconds to wait
//  = 0: return immediately (poll for mutex' availability)
//  < 0: wait indefinitely
zoe_errs_t zoe_sosal_mutex_get (zoe_sosal_obj_id_t mutex_id, int32_t microseconds);

// Release a mutex; might cause a waiting thread to wake up
zoe_errs_t zoe_sosal_mutex_release (zoe_sosal_obj_id_t mutex_id);

// Helper functions for simple mutual exclusion (infinite timeout)
STATIC_INLINE zoe_bool_t zoe_sosal_mutex_lock (zoe_sosal_obj_id_t mutex_id)
{
    return (zoe_sosal_mutex_get(mutex_id,-1) == ZOE_ERRS_SUCCESS);
}

STATIC_INLINE void zoe_sosal_mutex_unlock (zoe_sosal_obj_id_t mutex_id)
{
	zoe_sosal_mutex_release(mutex_id);
}



// Semaphores
// Used for resource allocation.
// Threads can give and take a unit at a time from a pool of available resources (the semaphore's max count).
// In ISRs semaphores can be given (released) but not taken (acquired).

// Parameters to define a new semaphore
typedef struct _zoe_sosal_semaphore_parms_t {
    uint32_t                max_count;
    uint32_t                init_count;
} zoe_sosal_semaphore_parms_t, *zoe_sosal_semaphore_parms_ptr_t;

// Create a new semaphore with given max and initial counts; the semaphore's id is returned in *semaphore_id_ptr
zoe_errs_t zoe_sosal_semaphore_create (zoe_sosal_semaphore_parms_ptr_t parms_ptr, const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * semaphore_id_ptr);

// Delete semaphore
zoe_errs_t zoe_sosal_semaphore_delete (zoe_sosal_obj_id_t semaphore_id);

// Take a unit from a semaphore.
// 'microseconds' is interpreted as follows:
//  > 0: number of microseconds to wait
//  = 0: return immediately (poll for semaphore's availability)
//  < 0: wait indefinitely
zoe_errs_t zoe_sosal_semaphore_take (zoe_sosal_obj_id_t semaphore_id, int32_t microseconds);

// Give a unit back to a semaphore; might cause a waiting thread to wake up
zoe_errs_t zoe_sosal_semaphore_give (zoe_sosal_obj_id_t semaphore_id);

// Return a semaphore to its initial state; error if there are waiting threads
zoe_errs_t zoe_sosal_semaphore_reset (zoe_sosal_obj_id_t semaphore_id);


// Spinlocks
// Used for mutual exclusion (critical zone) between threads and ISRs or ISIRs and ISRs.
// Creation and deletion must still be done in thread context.

// Create a new spinlock; the spinlock's id is returned in *spinlock_id_ptr; a spinlock is
// created in the "released" state
zoe_errs_t zoe_sosal_spinlock_create (const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * spinlock_id_ptr);

// Delete a spinlock.
zoe_errs_t zoe_sosal_spinlock_delete (zoe_sosal_obj_id_t spinlock_id);

// Get (lock, acquire) a spinlock; if someone else has this spinlock, the caller will spin
// until the holder releases it.
zoe_errs_t zoe_sosal_spinlock_get (zoe_sosal_obj_id_t spinlock_id);

// Release a spinock.
zoe_errs_t zoe_sosal_spinlock_release (zoe_sosal_obj_id_t spinlock_id);

// Helper functions for simple mutual exclusion
STATIC_INLINE zoe_bool_t zoe_sosal_spinlock_lock (zoe_sosal_obj_id_t spinlock_id)
{
    return (zoe_sosal_spinlock_get(spinlock_id) == ZOE_ERRS_SUCCESS);
}

STATIC_INLINE void zoe_sosal_spinlock_unlock (zoe_sosal_obj_id_t spinlock_id)
{
	zoe_sosal_spinlock_release(spinlock_id);
}


////////////////////////
// Timers
//
// One-shot or periodic, 'drifty' timers.
//

// The type of the timer callback (executed at ISR level)
typedef void (*zoe_sosal_time_timer_proc_t) (void * parm);

// Create a new timer object; the timer's id is returned in *timer_id_ptr
zoe_errs_t zoe_sosal_timer_create (const zoe_sosal_obj_name_t name, zoe_sosal_time_timer_proc_t proc, void * parm,
                 	 	 	 	zoe_sosal_obj_id_t * timer_id_ptr);

// Delete an existing timer object
zoe_errs_t zoe_sosal_timer_delete (zoe_sosal_obj_id_t timer_id);

// Starts the timer with relative (drifty) timing; if periodic is FALSE the callback is called only once
zoe_errs_t zoe_sosal_timer_start (zoe_sosal_obj_id_t timer_id, uint64_t microseconds, zoe_bool_t periodic);

// Stops a running timer; the timeout procedure may or may not be called depending on system delays
zoe_errs_t zoe_sosal_timer_stop (zoe_sosal_obj_id_t timer_id);



////////////////////////
//  Memory management
//
// Provide a memalign-like memory allocation function.
//
// Provide memory pools selection: local, internal or shared heap.
//	Local heap is within the memory partition of a particular module and is
//	where the standard C library fucntions (malloc, free, ...) operate.
//	Internal heap is a small, high-performance memory area associated with
//	the current CPU (i.e. static/scratch RAM inside the chip).
//	Shared heap is a large memory area shared among the different modules.
//
// Provide dual mapping of memory blocks to cached and non-cached addresses
// on architectures that allow for it (not Windows nor Linux); in the systems
// that do not support this features the API functions will return an error
// or NULL or 0.
//
// C library's functions (malloc, memalign, free ...) map into local heap,
// cached access.
//

typedef enum {
    ZOE_SOSAL_MEMORY_POOLS_LOCAL = 0,
    ZOE_SOSAL_MEMORY_POOLS_INTERNAL,
    ZOE_SOSAL_MEMORY_POOLS_SHARED,

    ZOE_SOSAL_MEMORY_POOLS_NUM
} zoe_sosal_memory_pools_t;


// Allocate a block of memory with a given alignment (number of least
// significant bits set to 0 in the returned address).
// The address returned is for cached access.
void * zoe_sosal_memory_alloc (zoe_sosal_memory_pools_t pool, uint32_t size, uint32_t alignment);
void * zoe_sosal_memory_local_alloc (uint32_t size);
void * zoe_sosal_memory_internal_alloc (uint32_t size);
void * zoe_sosal_memory_shared_alloc (uint32_t size);

// Free a previously allocated memory block
void zoe_sosal_memory_free (void * mem_ptr);

// Return the cached pointer for a given non-cached pointer
void * zoe_sosal_memory_cached (void * noncached_ptr);

#define ZOE_SOSAL_MEMORY_CACHED(noncached_ptr) zoe_sosal_memory_cached((void *)noncached_ptr)

// Return the non-cached pointer for a given cached pointer
void * zoe_sosal_memory_noncached (void * cached_ptr);

#define ZOE_SOSAL_MEMORY_NONCACHED(cached_ptr) zoe_sosal_memory_noncached((void *)cached_ptr)

// Return the log2 of the cacheline size (e.g. return 5 for 32-byte lines)
uint32_t zoe_sosal_memory_cacheline_log2 (void);

// Clean (flush) the cache over a given memory area; address must be cache
// aligned, 'lines' cache lines will be cleaned.
zoe_errs_t zoe_sosal_memory_cache_clean (void * start_ptr, uint32_t lines);

// Invalidate the cache over a given memory area; address must be cache
// aligned, 'lines' cache lines will be invalidated.
zoe_errs_t zoe_sosal_memory_cache_inval (void * start_ptr, uint32_t lines);

// Return the physical address for a given virtual memory address
void * zoe_sosal_memory_get_phy (void * vir_ptr);

#define ZOE_SOSAL_MEMORY_GET_PHY(vir_ptr) zoe_sosal_memory_get_phy((void *)vir_ptr)

// Return the virtual (uncached) address for a given physical memory address
void * zoe_sosal_memory_get_vir (void * phy_ptr);

#define ZOE_SOSAL_MEMORY_GET_VIR(phy_ptr) zoe_sosal_memory_get_vir((void *)phy_ptr)

// Return whether the virtual address is valid or not
zoe_bool_t zoe_sosal_memory_is_valid_vir (void * vir_ptr);


////////////////////////
// HW ISR handling
//
// One ISR handler can be installed per each of a range of available
// interrupt sources.
//

// The prototype for a HW ISR handler
typedef void (*zoe_sosal_isr_hw_proc_t) (void * ctxt, uint8_t bank_num, uint8_t bit_num);


// Install/uninstall a custom HW ISR handler (set 'proc' to NULL to uninstall) on
// a given HW interrupt number; processor affinity is defined in affinity_ptr
zoe_errs_t zoe_sosal_isr_hw_handler_install (uint8_t bank_num, uint8_t bit_num, zoe_bool_t pulse,
                                             zoe_sosal_isr_hw_proc_t proc,
                                             void * ctxt,
                                             zoe_sosal_affinity_ptr_t affinity_ptr);

// Return enable state of an ISR
zoe_errs_t zoe_sosal_isr_hw_enable_state_get (uint8_t bank_num, uint8_t bit_num, zoe_bool_t * enable_ptr);

// Set the enable/disable state of a given HW ISR; the previous state is returned
// in *enable_ptr
zoe_errs_t zoe_sosal_isr_hw_enable_state_set (uint8_t bank_num, uint8_t bit_num, zoe_bool_t * enable_ptr);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__ZOE_SOSAL_H__

