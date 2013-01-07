/*
 * Hibernation support specific for ARM
 *
 * Based on work by:
 *
 * Ubuntu project, hibernation support for mach-dove,
 * Copyright (C) 2010 Nokia Corporation (Hiroshi Doyu)
 * Copyright (C) 2010 Texas Instruments, Inc. (Teerth Reddy et al.)
 *	https://lkml.org/lkml/2010/6/18/4
 *	https://lists.linux-foundation.org/pipermail/linux-pm/2010-June/027422.html
 *	https://patchwork.kernel.org/patch/96442/
 *
 * Copyright (C) 2006 Rafael J. Wysocki <rjw@sisk.pl>
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/linkage.h>
#include <asm/memory.h>
#include <asm/page.h>
#include <asm/asm-offsets.h>
#include <asm/glue-proc.h>


/*
 * Save the current CPU state before suspend / poweroff.
 */
ENTRY(swsusp_arch_suspend)
	ldr	r0, =__swsusp_arch_ctx
	mrs	r1, cpsr
	mrs	r2, spsr
	stmia	r0!, {r1-r11,lr}	@ CPSR, SPSR, nonvolatile regs
	str	sp, [r0], #4		@ stack
	stmfd	sp!, {lr}
#ifdef	MULTI_CPU
	ldr	r1, =processor
	mov	lr, pc
	ldr	pc, [r1, #CPU_DO_SUSPEND]
#else
	bl	cpu_do_suspend
#endif
	ldmfd	sp!, {lr}
	b	swsusp_save		@ let framework write snapshot out
ENDPROC(swsusp_arch_suspend)

/*
 * Restore the memory image from the pagelists, and load the CPU registers
 * from saved state.
 */
ENTRY(swsusp_arch_resume)
	/*
	 * Switch stack to a nosavedata region to make sure image restore
	 * doesn't clobber it underneath itself.
	 */
	ldr	sp, =(__swsusp_resume_stk + PAGE_SIZE / 2)
	bl	__swsusp_arch_restore_image

	/*
	 * Restore the CPU registers.
	 */
	ldr	r0, =__swsusp_arch_ctx
	ldmia	r0!, {r1,r2}		@ CPSR / SPSR
	msr	cpsr, r1
	msr	spsr, r2
	ldr	r0, =__swsusp_arch_ctx	@ reload in case regset switched
	ldmia	r0!, {r1-r11,lr}	@ nonvolatile regs
	ldr	sp, [r0], #4		@ stack

	/*
	 * From here on we have a valid stack again. Core state is
	 * not restored yet, redirect to the machine-specific
	 * implementation to get that done.
	 * Resume has succeeded at this point; if the machine-specific
	 * code wants to fail it needs to panic.
	 */
	mov	r1, #0
	stmfd	sp!, {r1,r4-r11,lr}
#ifdef MULTI_CPU
	ldr	r1, =processor
	mov	lr, pc
	ldr	pc, [r1, #CPU_DO_RESUME]
#else
	bl	cpu_do_resume
#endif
	bl	cpu_init			@ reinitialize other modes
	ldmfd	sp!, {r0,r4-r11,pc}
ENDPROC(swsusp_arch_resume)