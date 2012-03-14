/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/linkage.h>
#include <mach/hardware.h>
#include <mach/mx5x.h>

#define ARM_CTRL_DCACHE  1 << 2
#define ARM_CTRL_ICACHE  1 << 12
#define ARM_AUXCR_L2EN   1 << 1

/* M4IF Constants */
#define M4IF_MCR0_OFFSET                 (0x008C)
#define M4IF_MCR0_FDVFS                  (0x1 << 11)
#define M4IF_MCR0_FDVACK                 (0x1 << 27)
#define IOMUXC_BASE_ADDR_VIRT			 AIPS1_IO_ADDRESS(IOMUXC_BASE_ADDR)
#define M4IF_BASE_ADDR_VIRT				 AIPS2_IO_ADDRESS(M4IF_BASE_ADDR)
/*----------------------------------------------------------------------------
@
@  Function: mx53_wait 
@
@  This function provides the instruction sequence for requesting the ARM CPU 
@  to enter the WFI (wait-for-interrupt).  This routine will be called by
@  the arch_idle.  
@
@  Parameters:
@      None.
@
@  Returns:
@      None.
@
@----------------------------------------------------------------------------*/
ENTRY(mx53_wait)

    @ Save registers
    stmfd   sp!, {r4}

    @ Point R0 at M4IF register set
    ldr     r0, =M4IF_BASE_ADDR_VIRT
    @ldr     r0, [r0]

    @ Point R1 at IOMUX register set
    ldr     r1, =IOMUXC_BASE_ADDR_VIRT
    @ldr     r1, [r1]

    @ Point R3 at temporary IRAM storage for DDR pad config
    adr     r3, PAD_CTL_TEMP

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3
    ldr     r2, =0x554
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x00]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3
    ldr     r2, =0x558
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x04]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2
    ldr     r2, =0x560
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x08]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1
    ldr     r2, =0x564
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x0C]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2
    ldr     r2, =0x568
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x10]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_1
    ldr     r2, =0x570
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x14]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS
    ldr     r2, =0x574
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x18]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0
    ldr     r2, =0x578
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x1C]         

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0
    ldr     r2, =0x57C
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x20]

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0
    ldr     r2, =0x580
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x24]

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0
    ldr     r2, =0x584
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x28]

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS
    ldr     r2, =0x588
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x2C]

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1
    ldr     r2, =0x590
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x30]

    @ Save state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1
    ldr     r2, =0x594
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x34]

    @ Save state of IOMUXC_SW_PAD_CTL_GRP_ADDDS
    ldr     r2, =0x6F0
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x38]

    @ Save state of IOMUXC_SW_PAD_CTL_GRP_B0DS
    ldr     r2, =0x718
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x3C]

    @ Save state of IOMUXC_SW_PAD_CTL_GRP_B1DS
    ldr     r2, =0x71C
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x40]

    @ Save state of IOMUXC_SW_PAD_CTL_GRP_CTLDS
    ldr     r2, =0x720
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x44]

    @ Save state of IOMUXC_SW_PAD_CTL_GRP_B2DS
    ldr     r2, =0x728
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x48]

    @ Save state of IOMUXC_SW_PAD_CTL_GRP_B3DS
    ldr     r2, =0x72C
    ldr     r2, [r1, r2]
    str     r2, [r3, #0x4c]

    @ Set FDVFS bit of M4IF_MCR0 to request DDR to enter self-refresh
    ldr     r2,[r0, #M4IF_MCR0_OFFSET]
    orr     r2, r2, #M4IF_MCR0_FDVFS
    str     r2,[r0, #M4IF_MCR0_OFFSET]

    @ Poll FDVACK bit of M4IF_MCR to wait for DDR to enter self-refresh entry
WAIT_SR_ACK:
    ldr     r2,[r0, #M4IF_MCR0_OFFSET]
    ands    r2, r2, #M4IF_MCR0_FDVACK
    beq     WAIT_SR_ACK
    
    @ Set DSE of all DDR I/O pads to 0 => HighZ 
    @ except CKE which must drive during self-refresh 
    @ according to JEDEC

    ldr     r4, =0
    
    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3
    ldr     r2, =0x554
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3
    ldr     r2, =0x558
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2
    ldr     r2, =0x560
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1
    ldr     r2, =0x564
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2
    ldr     r2, =0x568
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_1
    ldr     r2, =0x570
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS
    ldr     r2, =0x574
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0
    ldr     r2, =0x578
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0
    ldr     r2, =0x57C
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0
    ldr     r2, =0x580
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0
    ldr     r2, =0x584
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS
    ldr     r2, =0x588
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1
    ldr     r2, =0x590
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1
    ldr     r2, =0x594
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_GRP_ADDDS
    ldr     r2, =0x6F0
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_GRP_B0DS
    ldr     r2, =0x718
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_GRP_B1DS
    ldr     r2, =0x71C
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_GRP_B2DS
    ldr     r2, =0x728
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_GRP_B3DS
    ldr     r2, =0x72C
    str     r4, [r1, r2]

    @ Update state of IOMUXC_SW_PAD_CTL_GRP_CTLDS
    ldr     r4, =1
    ldr     r2, =0x720
    str     r4, [r1, r2]

WFI:
	.long     0xe320f003              @ Opcode for WFI

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_B3DS
    ldr     r4, [r3, #0x00]         
    ldr     r2, =0x554
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3
    ldr     r4, [r3, #0x04]         
    ldr     r2, =0x558
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2
    ldr     r4, [r3, #0x08]         
    ldr     r2, =0x560
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1
    ldr     r4, [r3, #0x0C]         
    ldr     r2, =0x564
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2
    ldr     r4, [r3, #0x10]         
    ldr     r2, =0x568
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_1
    ldr     r4, [r3, #0x14]         
    ldr     r2, =0x570
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS
    ldr     r4, [r3, #0x18]         
    ldr     r2, =0x574
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0
    ldr     r4, [r3, #0x1C]         
    ldr     r2, =0x578
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0
    ldr     r4, [r3, #0x20]
    ldr     r2, =0x57C
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0
    ldr     r4, [r3, #0x24]
    ldr     r2, =0x580
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0
    ldr     r4, [r3, #0x28]
    ldr     r2, =0x584
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS
    ldr     r4, [r3, #0x2C]
    ldr     r2, =0x588
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1
    ldr     r4, [r3, #0x30]
    ldr     r2, =0x590
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1
    ldr     r4, [r3, #0x34]
    ldr     r2, =0x594
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_ADDDS
    ldr     r4, [r3, #0x38]
    ldr     r2, =0x6F0
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_B0DS
    ldr     r4, [r3, #0x3C]
    ldr     r2, =0x718
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_B1DS
    ldr     r4, [r3, #0x40]
    ldr     r2, =0x71C
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_CTLDS
    ldr     r4, [r3, #0x44]
    ldr     r2, =0x720
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_B2DS
    ldr     r4, [r3, #0x48]
    ldr     r2, =0x728
    str     r4, [r1, r2]

    @ Restore state of IOMUXC_SW_PAD_CTL_GRP_B3DS
    ldr     r4, [r3, #0x4c]
    ldr     r2, =0x72C
    str     r4, [r1, r2]
    
    @ Clear FDVFS bit of M4IF_MCR0 to request DDR to exit self-refresh
    ldr     r2,[r0, #M4IF_MCR0_OFFSET]
    bic     r2, r2, #M4IF_MCR0_FDVFS
    str     r2,[r0, #M4IF_MCR0_OFFSET]

    @ Poll FDVACK bit of M4IF_MCR to wait for DDR to exit self-refresh entry
WAIT_AR_ACK:
    ldr     r2,[r0, #M4IF_MCR0_OFFSET]
    ands    r2, r2, #M4IF_MCR0_FDVACK
    bne     WAIT_AR_ACK

    @ Restore registers
    ldmfd   sp!, {r4}
     mov		pc, lr

PAD_CTL_TEMP:
		.space 512

	.type	mx53_do_wait, #object
ENTRY(mx53_do_wait)
	.word	mx53_wait
	.size	mx53_wait, . - mx53_do_wait
