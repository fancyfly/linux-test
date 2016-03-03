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


#include <asm/byteorder.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/random.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "zoe_types.h"
#include "os_specific.h"
#include "os_common.h"
#include "zoe_sosal_priv.h"
#include "zoe_sosal.h"
#include "zoe_cthread.h"


#define configMAX_PRIORITIES 4

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
#define ZOE_SOSAL_ISR_LOCAL_NUM ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
#define ZOE_SOSAL_ISR_LOCAL_NUM ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#define ZOE_SOSAL_ISR_LOCAL_NUM ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL
#else //ZOE_TARGET_CHIP == ???
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP

ZOE_DBG_COMP_DECL(SosalTestDbgComp);

static void *HalP = ZOE_NULL;
static zoe_dev_t g_dev;


/*
 * linux kernel impl of os_common.h
 */
zoe_bool_t OSSosalInit( void )
{
    zoe_bool_t rv = ZOE_TRUE;

#ifdef ZOE_DEBUG
    OSDbgInit();
#endif //ZOE_DEBUG

    return (rv);
}

void OSSosalSetHalP (void * halP)
{
    HalP = halP;
}

void * OSSosalGetHalP (void)
{
    return (HalP);
}

void OSSosalTerm( void )
{
}

void OSSosalIdle( void )
{
}

void OSSosalIRQ( void )
{
}

void OSInfo (zoe_sosal_info_ptr_t info_ptr)
{
    info_ptr->os_type = ZOE_SOSAL_OSTYPE_LINUX_KERNEL;
    info_ptr->sosal_version = ZOE_SOSAL_VERSION;
    info_ptr->major_version = 0;
    info_ptr->minor_version = 0;
    info_ptr->cores_num = 1;
    info_ptr->vpes_per_core = 1;
}


/* Threads */


typedef struct _TThreadInfo {
    TObjInfo	info;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
    pid_t       threadT;	
#else //LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    struct task_struct *task;
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
} TThreadInfo, *PTThreadInfo;

zoe_uint32_t OSThreadMaxprioritiesGet (void)
{
    return (configMAX_PRIORITIES);
}

void * OSThreadObjectAlloc (void)
{
    void *rv;

    rv = kmalloc( sizeof (TThreadInfo ), GFP_KERNEL);

    return (rv);
}

void OSThreadObjectFree (void * threadObjP)
{
    if ( threadObjP ) {
        kfree( threadObjP );
    }
}

/*
 * OSThreadCreate - Create and start a new thread
 * @parmsP: parameters of the created thread
 * @threadObjP: previously created thread obj
 */

zoe_errs_t OSThreadCreate( void * threadObjP, zoe_sosal_thread_parms_ptr_t parmsP)
{
    PTThreadInfo tobj = (PTThreadInfo)threadObjP;
    void* arg;
    int (*threadfn)(void *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    struct task_struct *task;
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)

    threadfn = (int (*)(void *))(parmsP->proc);
    arg = parmsP->parm;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	tobj->threadT = kernel_thread(threadfn, arg, CLONE_FS|CLONE_FILES);
#else //LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)    
    task = kthread_run(threadfn, arg, "%s", &tobj->info.name);
    if (IS_ERR(task)) {
        if (tobj)
            tobj->task = NULL;
        return ZOE_ERRS_FAIL;
    }

    if (tobj)
        tobj->task = task;
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)

    return (ZOE_ERRS_SUCCESS);
}

/*
 * OSThreadDelete - delete an existing thread
 */
zoe_errs_t OSThreadDelete (void * threadObjP)
{
    PTThreadInfo tobj = (PTThreadInfo)threadObjP;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
    kill_pid(find_get_pid(tobj->threadT), SIGKILL, 1);
#else //LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    if (tobj->task) {
        kthread_stop(tobj->task);
        return ZOE_ERRS_SUCCESS;
    }
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)

    return ZOE_ERRS_INVALID;
}


zoe_sosal_obj_id_t OSThreadGetID (void)
{
#ifdef DEBUG
    printk( "Warning: OSThreadGetID not implemented" );
#endif
    return ZOE_NULL;
}


/*
 * OSThreadSleepMS - Suspend executing thread for a number of milliseconds
 */
zoe_errs_t OSThreadSleepMS (zoe_uint32_t milliseconds)
{
    msleep( milliseconds );
    return (ZOE_ERRS_SUCCESS);
}


zoe_errs_t OSThreadSleepUS (zoe_uint32_t microseconds)
{
    return (ZOE_ERRS_SUCCESS);
}


zoe_errs_t OSThreadPriorityGet (void * threadObjP, zoe_uint32_ptr_t priorityP)
{
#ifdef DEBUG
    printk( "Warning: OSThreadPriorityGet not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSThreadPrioritySet (void * threadObjP, zoe_uint32_t priority)
{
#ifdef DEBUG
    printk( "Warning: OSThreadPrioritySet not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSThreadAbort (void * threadObjP)
{
#ifdef DEBUG
    printk( "Warning: OSThreadAbort not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}



/* Time */

/*
 * OSTimeSysTicks -
 * Return the number of system ticks elapsed since system startup
 * In Linux Kernel, it is jiffies
 */
zoe_sosal_ticks_t OSTimeSysTicks (void)
{
    return (zoe_sosal_ticks_t)jiffies;
}

/*
 * OSTimeTicksPerSecond -
 * Return the number of system ticks per second
 * (usually the clock frequency)
 */
uint64_t OSTimeTicksPerSecond (void)
{
    return (uint64_t)HZ;
}


/* Synchronization */

/* events */

void OSEventObjectFree (void *eventObjP )
{
    if ( ZOE_NULL != eventObjP ) {
        kfree( (TOSEvent)eventObjP );
    }
}


void *OSEventObjectAlloc( void )
{
    TOSEvent zq;

//    printk( "OSEventObjectAlloc called\n" );
    zq = (TOSEvent)kzalloc(sizeof(lk_sosal_event_t), GFP_KERNEL);

    return (void *)zq;
}

zoe_errs_t OSEventCreate (void *eventObjP )
{
    TOSEvent zq;

    zq = (TOSEvent)eventObjP;
    if (zq) {
        init_waitqueue_head(&(zq->queue));
        zq->condition = ZOE_FALSE;
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}

zoe_errs_t OSEventDelete (void *eventObjP )
{
    TOSEvent zq;

    zq = (TOSEvent)eventObjP;
    if ( zq ) {
        zq = ZOE_NULL;
    }

    return ZOE_ERRS_SUCCESS;
}


zoe_errs_t
OSEventReset (void * eventObjP)
{
    TOSEvent zq;

    zq = (TOSEvent)eventObjP;

    if ( zq ) {
        zq->condition = ZOE_FALSE;
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}


zoe_errs_t OSEventPost (void *eventObjP )
{
    TOSEvent zq;

    zq = (TOSEvent)eventObjP;

    if ( zq ) {
        zq->condition = ZOE_TRUE;
        wake_up(&zq->queue);
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}

zoe_errs_t OSEventWait (void *eventObjP, zoe_int32_t microseconds)
{
    TOSEvent zq;
    zoe_errs_t  rv;

    zq = (TOSEvent)eventObjP;
    ////printk( "OSEventWait called\n" );
    ////printk( "OSEventWait: warning: provides only millisecond granularity, not microseconds\n" );

    if ( !zq ) {
        return ZOE_ERRS_FAIL;
    }

    if (microseconds >= 0) {
        rv = wait_event_timeout( zq->queue, (zq->condition == ZOE_TRUE),
            msecs_to_jiffies(microseconds / 1000) );
        zq->condition = ZOE_FALSE;
        if ( 0 == rv ) {
            return ZOE_ERRS_TIMEOUT;
        } else {
            return ZOE_ERRS_SUCCESS;
        }
    }
    else {
        wait_event( zq->queue, (zq->condition == ZOE_TRUE));
        zq->condition = ZOE_FALSE;
        return ZOE_ERRS_SUCCESS;
    }
}


zoe_bool_t
OSEventWaited (void * eventObjP)
{
	return (ZOE_FALSE);
}


/* mutexes */
void * OSMutexObjectAlloc (void)
{
    TOSMutex mutex;

    mutex = (TOSMutex)kmalloc(sizeof(lk_sosal_mutex_t), GFP_KERNEL);
    mutex->pSem = (struct semaphore *)kmalloc(sizeof(struct semaphore),
        GFP_KERNEL );

    return mutex;
}

void OSMutexObjectFree (void * mutexObjP)
{
    TOSMutex mutex;

    mutex = (TOSMutex)mutexObjP;

    if ( mutex && mutex->pSem ) {
        kfree( mutex->pSem );
    }
    if ( mutex ) {
        kfree( mutex );
    }
}

zoe_errs_t OSMutexCreate (void * mutexObjP)
{
    /* N.B. - mutex->OBJ_INF_HEADER filled in by stubs_mutex.c */

    TOSMutex mutex;

    mutex = (TOSMutex)mutexObjP;
    if ( mutex ) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
	sema_init( mutex->pSem, 1 );
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
	init_MUTEX( mutex->pSem );
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)        
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}

zoe_errs_t OSMutexDelete (void * mutexObjP)
{
    if ( mutexObjP ) {
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}

zoe_errs_t OSMutexRelease (void * mutexObjP)
{
    TOSMutex mutex;

    mutex = (TOSMutex)mutexObjP;

    if ( mutex && mutex->pSem ) {
        up( mutex->pSem );
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}

zoe_errs_t OSMutexGet (void * mutexObjP, zoe_int32_t milliseconds)
{
    TOSMutex mutex;

    mutex = (TOSMutex)mutexObjP;

////    printk( "warning: OSMutexGet: ignoring milliseconds argument\n" );
    if ( mutex && mutex->pSem ) {
        down( mutex->pSem );
        return ZOE_ERRS_SUCCESS;
    } else {
        return ZOE_ERRS_FAIL;
    }
}




/* Spinlocks */
void * OSSpinlockObjectAlloc (void)
{
#ifdef DEBUG
    printk( "Warning: OSSpinlockObjectAlloc not implemented" );
#endif
    return ZOE_NULL;
}

void OSSpinlockObjectFree (void * spinlockObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSpinlockObjectFree not implemented" );
#endif
}

zoe_errs_t OSSpinlockCreate (void * spinlockObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSpinlockCreate not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSpinlockDelete (void * spinlockObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSpinlockDelete not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSpinlockRelease (void * spinlockObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSpinlockRelease not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSpinlockGet (void * spinlockObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSpinlockGet not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

/* Semaphores */
void * OSSemaphoreObjectAlloc (void)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreObjectAlloc not implemented" );
#endif
    return ZOE_NULL;
}

void OSSemaphoreObjectFree (void * semaphoreObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreObjectFree not implemented" );
#endif
}

zoe_errs_t OSSemaphoreCreate (void * semaphoreObjP, zoe_sosal_semaphore_parms_ptr_t parms_ptr)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreCreate not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSemaphoreDelete (void * semaphoreObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreDelete not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSemaphoreGive (void * semaphoreObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreGive not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSemaphoreTake (void * semaphoreObjP, zoe_int32_t milliseconds)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreTake not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}

zoe_errs_t OSSemaphoreReset (void * semaphoreObjP)
{
#ifdef DEBUG
    printk( "Warning: OSSemaphoreReset not implemented" );
#endif
    return ZOE_ERRS_NOTIMPL;
}


/* Memory management */
void *OSMemoryAlloc( zoe_sosal_memory_pools_t pool, zoe_uint32_t size,
    zoe_uint32_t alignment )
{
#ifdef DEBUG
////    printk( "Warning: OSMemoryAlloc() ignores memory pool and alignment" );
#endif
    
    ////return (kmalloc(size, GFP_KERNEL));
    return (vmalloc(size));

}

void OSMemoryFree( void *mem_ptr )
{
    ////return (kfree(mem_ptr));
    vfree(mem_ptr);
    return;
}

void *OSMemoryCached( void *noncached_ptr )
{
#ifdef DEBUG
    printk( "Warning: OSMemoryCached not implemented" );
#endif

    return (void *)0;
}

void *OSMemoryNoncached( void *cached_ptr )
{
#ifdef DEBUG
    printk( "Warning: OSMemoryNoncached not implemented" );
#endif

    return (void *)0;
}

zoe_uint32_t OSMemoryCachelineLog2( void )
{
#ifdef DEBUG
    printk( "Warning: OSMemoryCachelineLog2 not implemented" );
#endif

    return 0;
}

zoe_errs_t OSMemoryCacheClean( void *start_ptr, zoe_uint32_t lines )
{
#ifdef DEBUG
    printk( "Warning: OSMemoryCacheClean not implemented" );
#endif

    return ZOE_ERRS_SUCCESS;
}

zoe_errs_t OSMemoryCacheInval( void *start_ptr, zoe_uint32_t lines )
{
#ifdef DEBUG
    printk( "Warning: OSMemoryCacheInval not implemented" );
#endif

    return ZOE_ERRS_SUCCESS;
}


void * OSMemoryGetPhy (void * vir_ptr)
{
#ifdef DEBUG
    printk( "Warning: OSMemoryGetPhy not implemented" );
#endif
    return vir_ptr;
}

void * OSMemoryGetVir (void * phy_ptr)
{
#ifdef DEBUG
    printk( "Warning: OSMemoryGetVir not implemented" );
#endif
    return phy_ptr;
}

zoe_bool_t OSMemoryIsValidVir (void * vir_ptr)
{
#ifdef DEBUG
    printk( "Warning: OSMemoryIsValidVir not implemented" );
#endif
    return ZOE_TRUE;
}




/* ISR handling */

lk_hw_irq_info_t LK_HW_IRQ_INFO[1+LK_MAX_HW_IRQ]; /* TODO - needs to be initialized */
lk_sw_irq_info_t LK_SW_IRQ_INFO[1+LK_MAX_SW_IRQ]; /* TODO - needs to be initialized */

zoe_bool_t OSInISR( void )
{

/* TODO - fill in the code */

    return in_irq();
}

zoe_sosal_isr_sw_numbers_t
OSISRMySWISRNum( void )
{
    return ( ZOE_SOSAL_ISR_LOCAL_NUM );
}

void _lk_sw_irq_handler( unsigned long arg );

zoe_errs_t OSISRSWHandlerInstall( zoe_sosal_isr_sw_numbers_t from_num, zoe_sosal_isr_sw_proc_t proc, void * ctxt)
{
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;
#if 0
    lk_irq_func_ptr_t lk_proc;

    /*
    if ( to_num != from_num ) {
        printk( "OSISRSWHandlerInstall FAIL: sw interrupt currently implemented as Linux tasklets.  As such, to_num must equal from_num" );
        rv = ZOE_ERRS_FAIL;
        goto bail1;
    }
    CHECK_SW_IRQ( bail2, bail3 )
    */
    g_dev.swIrqInfo[from_num].proc = proc;
    g_dev.swIrqInfo[from_num].ctxt = ctxt;
    tasklet_init(&(g_dev.swIrqInfo[from_num].tasklet), _lk_sw_irq_handler, (unsigned long)&g_dev );

    rv = ZOE_ERRS_SUCCESS;
#endif

/*
bail3:
bail2:
bail1:
*/
    return (rv);
}

zoe_errs_t OSISRSWEnableStateSet ( zoe_sosal_isr_sw_numbers_t from_num, zoe_bool_t * enable_ptr )
{
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;

    return (rv);
}

zoe_errs_t OSISRSWTrigger( zoe_sosal_isr_sw_numbers_t to_num )
{
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;

    /*
    if ( to_num != from_num ) {
        printk( "OSISRSWTrigger FAIL: sw interrupt currently implemented as Linux tasklets.  As such, to_num must equal from_num" );
        rv = ZOE_ERRS_FAIL;
        goto bail1;
    }
    CHECK_SW_IRQ( bail2, bail3 )

    g_dev.sw_from_irq = from_num;
    */
    g_dev.sw_to_irq = to_num;
    tasklet_schedule( &(g_dev.swIrqInfo[to_num].tasklet) );

    rv = ZOE_ERRS_SUCCESS;

/*
bail3:
bail2:
bail1:
*/
    return (rv);
}


zoe_errs_t OSISRSWClear( zoe_sosal_isr_sw_numbers_t from_num)
{
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;

    return (rv);
}

irqreturn_t _lk_hw_irq_handler( int irq, void *dev_id )
{
    lk_hw_irq_info_t *dev = dev_id;
    irqreturn_t rv = 0;

    zoe_uint8_t bank_num = dev->bank_num;
    zoe_uint8_t bit_num = dev->bit_num;
    zoe_sosal_isr_hw_proc_t proc = dev->proc;
    void *ctxt = dev->ctxt;

    (*proc)(ctxt, bank_num, bit_num);

    printk( "warning: _lk_hw_irq_handler return value needs attention\n" );

    return rv;
}

void _lk_sw_irq_handler( unsigned long arg )
{
    zoe_errs_t rv;

    zoe_dev_t *dev=(zoe_dev_t *)arg;
    zoe_sosal_isr_sw_numbers_t to_num, from_num;
    zoe_sosal_isr_sw_proc_t proc;
    void *ctxt;

    printk( "_lk_sw_irq_handler: dev=%p\n", dev );
    from_num = dev->sw_from_irq;
    to_num = dev->sw_to_irq;
    printk( "_lk_sw_irq_handler: from_num = %d, to_num = %d\n",
        from_num, to_num );
    CHECK_SW_IRQ( bail1, bail2 )
    proc = dev->swIrqInfo[from_num].proc;
    ctxt = dev->swIrqInfo[from_num].ctxt;

    (*proc)( ctxt, from_num );

    printk( "warning: _lk_sw_irq_handler return value needs attention\n" );
    printk( "called _lk_sw_irq_handler\n" );

bail1:
bail2:
    return;
}


zoe_errs_t OSISRHWHandlerInstall (zoe_uint8_t bank_num, zoe_uint8_t bit_num, zoe_bool_t pulse, zoe_sosal_isr_hw_proc_t proc, 
                                  void * ctxt, zoe_sosal_affinity_ptr_t affinity_ptr)
{
#if 1
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;
#else
    zoe_errs_t rv;
    unsigned int irq;
    unsigned long flags;
    const char *dev_name;
    lk_hw_irq_info_t dev_id;
    int lk_irq_req_rv;

    if ( int_num > LK_MAX_HW_IRQ ) {
        printk( "OSISRHWHandlerInstall > %d\n", LK_MAX_HW_IRQ );
        return ZOE_ERRS_FAIL;
    }

    irq = (unsigned int)int_num;
    flags = 0;
    dev_name = LK_DEV_NAME;
    dev_id = LK_HW_IRQ_INFO[int_num];
    dev_id.ctxt = ctxt;
    dev_id.proc = proc;
    dev_id.bank_num = bank_num;
    dev_id.bit_num = bit_num;

    /* TODO - dev_id and ctxt need to be rethought.  What is the lifecycle of */
    /*        dev_id? */

    lk_irq_req_rv = request_irq( irq, _lk_hw_irq_handler, flags, dev_name, (void *)&dev_id );
    if ( 0 == lk_irq_req_rv ) {
        rv = ZOE_ERRS_SUCCESS;
    } else {
        rv = ZOE_ERRS_FAIL;
    }
#endif

    return (rv);
}


zoe_errs_t OSISRHWEnableStateGet(zoe_uint8_t bank_num, zoe_uint8_t bit_num, zoe_bool_t * enable_ptr )
{
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;

    return (rv);
}

zoe_errs_t OSISRHWEnableStateSet (zoe_uint8_t bank_num, zoe_uint8_t bit_num, zoe_bool_t * enable_ptr)
{
    zoe_errs_t rv = ZOE_ERRS_SUCCESS;

    return (rv);
}

TOSLockID OSLockCreate( zoe_sosal_obj_name_t name )
{
    /* TODO - need to check whether use of zoe_sosal_memory_local_alloc() */
    /* is the way to go, or to hash name */
    spinlock_t *pLock;

    pLock = kmalloc( sizeof (spinlock_t ), GFP_KERNEL);
    spin_lock_init( pLock );

    return (TOSLockID)pLock;
}

void OSLockDelete (TOSLockID lockID)
{
    /* need to match the allocation method in OSLockCreate */
    kfree( (spinlock_t *)lockID );
}

void OSLockGet (TOSLockID lockID)
{
    spinlock_t *pLock;

    pLock = (spinlock_t *)lockID;
    spin_lock( pLock );
}

void OSLockRelease (TOSLockID lockID)
{
    spinlock_t *pLock;

    pLock = (spinlock_t *)lockID;
    spin_unlock( pLock );
}

extern const char *ObjNames[];
#if 0
const char * ObjNames[] = {
    "void",
    "thread",
    "mutex",
    "event"
};
#endif


#if 0
void
InitObj (PTObjInfo oiP, EObjTypes type, zoe_sosal_obj_name_t name)
{
    oiP->type = type;
    if (name == ZOE_NULL) {
         int                    idx = 0;

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
        default:
            idx = 0;
            break;
        }
        snprintf(oiP->name,sizeof(zoe_sosal_obj_name_t),"%s %p",ObjNames[idx],oiP);
    }else{
        memcpy(oiP->name,name,sizeof(zoe_sosal_obj_name_t));
    }
    // make sure it is NULL terminated
    oiP->name[sizeof(zoe_sosal_obj_name_t)-1] = 0;
}
#endif

#if 0
void
TermObj (PTObjInfo oiP)
{
    oiP->type = OBJTYPE_VOID;
}


TOSLockID EventsLockID;
#endif
