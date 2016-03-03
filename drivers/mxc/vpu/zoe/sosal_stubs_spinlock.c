/******************************************************************************
*                                                                             *
* All rights are reserved. This confidential and proprietary HDL/C/HVL soft   *
* description of a Hardware/Software component may be used only as authorized *
* by a licensing agreement from Zenverge Incorporated. In the event of        *
* publication, the following notice is applicable:                            *
*                                                                             *
*                  (C) COPYRIGHT 2012-2013 Zenverge Inc.                      *
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
// stubs_spinlock.c  zoe sosal Spinlocks entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Feb 17, 2012
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#include <stdio.h>
#else //ZOE_LINUXKER_BUILD
#include <linux/string.h>
#endif //!ZOE_LINUXKER_BUILD

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"

#include "os_common.h"
#include "os_stubs.h"


ZOE_DBG_COMP_EXT(ZoeSosalDbgCompID);


///////////////////////////////////////////////////////////////////////////////
//
// Spinlocks
//
// Used for mutual exclusion between threads and ISRs or between ISRs and ISRs.
//


zoe_errs_t
zoe_sosal_spinlock_create (const zoe_sosal_obj_name_t name, zoe_sosal_obj_id_t * spinlock_id_ptr)
{
    zoe_errs_t				rv = ZOE_ERRS_SUCCESS;
    PTObjInfo				siP = ZOE_NULL;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, spinlock_id_ptr(%p) name(%p)\n"
        ,spinlock_id_ptr,name);

    if (spinlock_id_ptr == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"NULL spinlock ID pointer\r\n");
        rv = ZOE_ERRS_PARMS;
        goto spinlock_createExit;
    }

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto spinlock_createExit;
	}

    siP = (PTObjInfo) OSSpinlockObjectAlloc();
    if (siP == ZOE_NULL) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSSpinlockObjectAlloc failed\n");
        rv = ZOE_ERRS_NOMEMORY;
        goto spinlock_createExit;
    }

	InitObj((PTObjInfo) siP,OBJTYPE_SPINLOCK,name);
    rv = OSSpinlockCreate(siP);
    if (ZOE_SUCCESS(rv)) {
        *spinlock_id_ptr = siP;
    }else{
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSSpinlockCreate failed (%lu)\n",rv);
        goto spinlock_createExit;
    }

spinlock_createExit:
	if (ZOE_FAIL(rv)) {
		if (siP != ZOE_NULL) {
			TermObj((PTObjInfo) siP);
			OSSpinlockObjectFree(siP);
		}
	}
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


zoe_errs_t
zoe_sosal_spinlock_delete (zoe_sosal_obj_id_t spinlock_id)
{
    zoe_errs_t		rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered, spinlock_id(0x%llX)\n",spinlock_id);

	if (OSInISR()) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"Cannot call from ISR\r\n");
        rv = ZOE_ERRS_INVALID;
        goto spinlock_deleteExit;
	}

	siP = (PTObjInfo) spinlock_id;
    if ((spinlock_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid spinlock ID 0x%llX\n",spinlock_id);
		rv = ZOE_ERRS_PARMS;
        goto spinlock_deleteExit;
	}

	if (siP->type != OBJTYPE_SPINLOCK) {
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		rv = ZOE_ERRS_INVALID;
        goto spinlock_deleteExit;
	}

	rv = OSSpinlockDelete(siP);
	if (ZOE_SUCCESS(rv)) {
		// Should the spinlock object be freed also when OSSpinlockDelete fails?
		TermObj(siP);
		OSSpinlockObjectFree(siP);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"OSSpinlockDelete failed (%lu)\n",rv);
		goto spinlock_deleteExit;
	}

spinlock_deleteExit:
    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


zoe_errs_t
zoe_sosal_spinlock_get (zoe_sosal_obj_id_t spinlock_id)
{
	zoe_errs_t		rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, spinlock_id(0x%llX)\n",spinlock_id);

	siP = (PTObjInfo) spinlock_id;
    if ((spinlock_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid spinlock ID 0x%llX\n",spinlock_id);
		rv = ZOE_ERRS_PARMS;
        goto spinlock_getExit;
	}

	if (siP->type == OBJTYPE_SPINLOCK) {
		rv = OSSpinlockGet(siP);
	}else{
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		rv = ZOE_ERRS_INVALID;
		goto spinlock_getExit;
	}

spinlock_getExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


zoe_errs_t
zoe_sosal_spinlock_release (zoe_sosal_obj_id_t spinlock_id)
{
	zoe_errs_t		rv = ZOE_ERRS_SUCCESS;
    PTObjInfo       siP;

    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Entered, spinlock_id(0x%llX)\n",spinlock_id);

	siP = (PTObjInfo) spinlock_id;
    if ((spinlock_id == 0) || ((siP == ZOE_NULL))) {
        zoe_dbg_printf_e(ZoeSosalDbgCompID,"invalid spinlock ID 0x%llX\n",spinlock_id);
		rv = ZOE_ERRS_PARMS;
        goto spinlock_releaseExit;
	}

	if (siP->type == OBJTYPE_SPINLOCK) {
		rv = OSSpinlockRelease(siP);
	}else{
		rv = ZOE_ERRS_INVALID;
		zoe_dbg_printf_e(ZoeSosalDbgCompID,"Invalid object type %d\r\n",siP->type);
		goto spinlock_releaseExit;
	}

spinlock_releaseExit:
    zoe_dbg_printf_x(ZoeSosalDbgCompID,"Exiting\n");
    return (rv);
}

