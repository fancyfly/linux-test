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


///////////////////////////////////////////////////////////////////////////////
//
// os_specific.h  Secure OS Abstraction Layer definitions specific to
// Linux user mode
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Mar 4, 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OS_SPECIFIC_H__
#define __OS_SPECIFIC_H__

//#include <semaphore.h>

#include "zoe_types.h"
#include "zoe_sosal.h"
#include "os_common.h"
#include "os_stubs.h"
#include <linux/kernel.h>

#define LK_MAX_HW_IRQ 16
#define LK_MAX_SW_IRQ 32

#define LK_DEV_NAME "/dev/zoe"
#define LK_DEV_ID 1

#define CHECK_RV(x) { if ( ZOE_ERRS_SUCCESS != rv ) { testResult = ZOE_FALSE; goto x; } } while(0);
#define CHECK_SW_IRQ(x,y) { if ( from_num > LK_MAX_SW_IRQ ) { zoe_dbg_printf_t( SosalTestDbgComp, "CHECK_SW FAILED\n" ); rv = ZOE_ERRS_FAIL; goto x; } if (to_num > LK_MAX_SW_IRQ) { rv = ZOE_ERRS_FAIL; goto y; }} while(0);

typedef unsigned long TISRContext;

typedef irqreturn_t (lk_irq_func_ptr_t)(int, void *, struct pt_regs *); 

typedef struct _zoe_hw_irq_info_t {
    /* OSISRHWHandlerInstall() parameters */
    zoe_uint8_t bank_num; 
    zoe_uint8_t bit_num;
    zoe_sosal_isr_hw_proc_t proc;
    void *ctxt;
} lk_hw_irq_info_t;

typedef struct _zoe_sw_irq_info_t {
    /* OSISRSWHandlerInstall() parameters */
    zoe_sosal_isr_sw_numbers_t to_num, from_num;
    zoe_sosal_isr_sw_proc_t proc;
    struct tasklet_struct tasklet;
    void *ctxt;
} lk_sw_irq_info_t;

typedef struct _zoe_dev_t {
    dev_t devno;
    void *ctxt;
    zoe_sosal_isr_sw_numbers_t sw_from_irq, sw_to_irq;
    lk_hw_irq_info_t hwIrqInfo[1+LK_MAX_HW_IRQ];
    lk_sw_irq_info_t swIrqInfo[1+LK_MAX_SW_IRQ];
} zoe_dev_t;

extern lk_hw_irq_info_t LK_HW_IRQ_INFO[1+LK_MAX_HW_IRQ];
extern lk_sw_irq_info_t LK_SW_IRQ_INFO[1+LK_MAX_SW_IRQ];

/*
typedef struct _TObjInfo {
    struct task_struct *task;
} TObjInfo, *PTObjInfo;
*/

typedef struct lk_sosal_event {
    OBJ_INFO_HEADER
    EVENT_INFO_HEADER
	wait_queue_head_t    queue;
	volatile unsigned long        condition;
} lk_sosal_event_t;

typedef lk_sosal_event_t *TOSEvent;

typedef struct lk_sosal_mutex {
    OBJ_INFO_HEADER
    struct semaphore *pSem;
} lk_sosal_mutex_t;
typedef lk_sosal_mutex_t *TOSMutex;

/*
typedef struct lk_sosal_mutex {
	OBJ_INFO_HEADER
	struct mutex  mutex;
} lk_sosal_mutex_t;
*/

#endif //__OS_SPECIFIC_H__


