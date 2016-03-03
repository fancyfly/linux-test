/******************************************************************************
*                                                                             *
* All rights are reserved. This confidential and proprietary HDL/C/HVL soft   *
* description of a Hardware/Software component may be used only as authorized *
* by a licensing agreement from Zenverge Incorporated. In the event of        *
* publication, the following notice is applicable:                            *
*                                                                             *
*                    (C) COPYRIGHT 2011-2013 Zenverge Inc.                    *
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
// stubs_isr.c  zoe sosal ISR management entry points implementation
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Nov 9, 2011
//
///////////////////////////////////////////////////////////////////////////////


#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#include <stdio.h>
#endif

#include "zoe_types.h"
#include "zoe_dbg.h"

#include "zoe_sosal.h"
#include "zoe_sosal_priv.h"

#include "os_common.h"
#include "os_stubs.h"


ZOE_DBG_COMP_EXT(ZoeSosalDbgCompID);



////////////////////////////////////////////
//
// Software ISR handling
//


// Return the current CPU's ISR number
zoe_sosal_isr_sw_numbers_t
zoe_sosal_isr_sw_my_isr_num (void)
{
    return(OSISRMySWISRNum());
}


// Install a custom ISR handler (set proc to NULL to uninstall)
zoe_errs_t
zoe_sosal_isr_sw_handler_install (zoe_sosal_isr_sw_numbers_t from_num,
								 zoe_sosal_isr_sw_proc_t proc,
								 void * ctxt)
{
    zoe_errs_t          rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered from_num(%d) proc(%p) ctxt(%p)\n"
        ,from_num,proc,ctxt);

    if (from_num == ZOE_SOSAL_ISR_SW_SELF) {
        from_num = OSISRMySWISRNum();
    }
    if ((from_num >= ZOE_SOSAL_ISR_SW_NUM) || (from_num < 0)) {
        zoe_dbg_printf_w(ZoeSosalDbgCompID,"invalid isr FROM number %d\n",from_num);
    }else{
        rv = OSISRSWHandlerInstall(from_num,proc,ctxt);
    }

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Set the enable/disable state of  a given ISR
zoe_errs_t
zoe_sosal_isr_sw_enable_state_set (zoe_sosal_isr_sw_numbers_t from_num,
							 zoe_bool_t * enable_ptr)
{
    zoe_errs_t          rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered from_num(%d) enable_ptr(%p)\n"
        ,from_num,enable_ptr);

    if (from_num == ZOE_SOSAL_ISR_SW_SELF) {
        from_num = OSISRMySWISRNum();
    }
    if ((from_num >= ZOE_SOSAL_ISR_SW_NUM) || (from_num < 0)) {
        zoe_dbg_printf_w(ZoeSosalDbgCompID,"invalid isr FROM number %d\n",from_num);
    }else{
        rv = OSISRSWEnableStateSet(from_num,enable_ptr);
    }

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Trigger a SW ISR
zoe_errs_t
zoe_sosal_isr_sw_trigger (zoe_sosal_isr_sw_numbers_t to_num)
{
    zoe_errs_t      rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered to_num(%d)\n",to_num);

    if (to_num == ZOE_SOSAL_ISR_SW_SELF) {
        to_num = OSISRMySWISRNum();
    }
    if ((to_num >= ZOE_SOSAL_ISR_SW_NUM) || (to_num < 0)) {
        zoe_dbg_printf_w(ZoeSosalDbgCompID,"invalid isr TO number %d\n",to_num);
    }else{
        rv = OSISRSWTrigger(to_num);
    }

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Clear a SW ISR
zoe_errs_t
zoe_sosal_isr_sw_clear (zoe_sosal_isr_sw_numbers_t from_num)
{
    zoe_errs_t  rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered from_num(%d)\n",from_num);

    if (from_num == ZOE_SOSAL_ISR_SW_SELF) {
        from_num = OSISRMySWISRNum();
    }
if ((from_num >= ZOE_SOSAL_ISR_SW_NUM) || (from_num < 0)) {
        zoe_dbg_printf_w(ZoeSosalDbgCompID,"invalid isr FROM number %d\n",from_num);
    }else{
        rv = OSISRSWClear(from_num);
    }

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}



////////////////////////////////////////////
//
// Hardware ISR handling
//


// Install/uninstall a custom HW ISR handler (set 'proc' to NULL to uninstall) on
// a given bank,bit pair; processor affinity is defined in affinity_ptr
zoe_errs_t
zoe_sosal_isr_hw_handler_install (zoe_uint8_t bank_num, zoe_uint8_t bit_num, zoe_bool_t pulse,
									zoe_sosal_isr_hw_proc_t proc,
									void * ctxt,
									zoe_sosal_affinity_ptr_t affinity_ptr)
{
    zoe_errs_t          rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered bank,bit(%d,%d) proc(%p) ctxt(%p) affinity_ptr(%X)\n"
        ,bank_num,bit_num,proc,ctxt,affinity_ptr);

    rv = OSISRHWHandlerInstall(bank_num,bit_num,pulse,proc,ctxt,affinity_ptr);

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Return enable state of an ISR
zoe_errs_t
zoe_sosal_isr_hw_enable_state_get (zoe_uint8_t bank_num, zoe_uint8_t bit_num, zoe_bool_t * enable_ptr)
{
    zoe_errs_t          rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered bank,bit(%d,%d) enable_ptr(%p)\n"
        ,bank_num,bit_num,enable_ptr);

    if (enable_ptr != ZOE_NULL) {
		rv = OSISRHWEnableStateGet(bank_num,bit_num,enable_ptr);
    }

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}


// Set the enable/disable state of a given HW ISR; the previous state is returned
// in *enable_ptr
zoe_errs_t
zoe_sosal_isr_hw_enable_state_set (zoe_uint8_t bank_num, zoe_uint8_t bit_num, zoe_bool_t * enable_ptr)
{
    zoe_errs_t          rv = ZOE_ERRS_PARMS;

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Entered bank,bit(%d,%d) enable_ptr(%p)\n"
        ,bank_num,bit_num,enable_ptr);

    if (enable_ptr != ZOE_NULL) {
		rv = OSISRHWEnableStateSet(bank_num,bit_num,enable_ptr);
    }

    zoe_dbg_printf_t(ZoeSosalDbgCompID,"Exiting (%d)\n",rv);
    return (rv);
}

