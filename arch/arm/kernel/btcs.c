/*
 * Copyright (c) 2003, Metrowerks Corporation.
 * Copyright 2007-2010 Freescale Semiconductor, Inc.
 *
 * Author: John Rigby, jrigby@freescale.com
 * Original Author: Bernhard Kuhn, bkuhn@freescale.com
 *
 * Hooks to BTCS (Boot Time Critical Services)
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/btcs_defs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/bootmem.h>
#include <asm/pgtable.h>
#include <mach/hardware.h>

#ifdef CONFIG_BTCS

#define BTCS_IRQ_NUMBER    MXC_INT_CAN2

void __init btcs_reserve_sdram(void)
{
	reserve_bootmem(__virt_to_phys(CONFIG_BTCS_START),
		    CONFIG_BTCS_SIZE, BOOTMEM_DEFAULT);
	pr_info("Size of 0x%x sdram reserved for BTCS\n", CONFIG_BTCS_SIZE);
}

void __init set_fld_for_vaddr(unsigned int vaddr, unsigned int value)
{
	unsigned int *table = (unsigned int *)cpu_get_pgd();
	table[vaddr>>20] = value;
	asm("mcr p15, 0, %0, c8, c7, 1" : : "r" (vaddr) : "cc");
}


static void btcs_printk(void *msg, int arg)
{
	printk((char *)msg, arg);
}

void btcs_poll(void)
{
	struct btcs_cb_interface *btcs;
	struct btcs_event event;
	int (*btcs_callback) (struct btcs_event *);

	btcs = (struct btcs_cb_interface *)CONFIG_BTCS_START;

	if (btcs->magic == 0xdeadbeef) {
		event.cmd = BTCS_CAN_IRQ;
		event.arg = 0;
		btcs_callback = btcs->callback;
		btcs_callback(&event);
	}
}

irqreturn_t btcs_isr(int irq, void *dev_id)
{
	unsigned long flags;
	local_irq_save(flags);
	btcs_poll();
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static struct irqaction btcs_irq = {
	.name           = "BTCS",
	.flags          = IRQF_DISABLED,
	.handler        = btcs_isr,
};

int __init btcs_init(void)
{
	struct btcs_cb_interface *btcs;

	btcs = (struct btcs_cb_interface *)CONFIG_BTCS_START;
	if (btcs->magic == 0xdeadbeef)
		btcs->printk = (void *)btcs_printk;

	/* Install isr */
	if (setup_irq(BTCS_IRQ_NUMBER, &btcs_irq) != 0)
		panic("could not allocate BTCS CAN IRQ!");
	/* Guarantee the priority */
	if (imx_irq_set_priority(BTCS_IRQ_NUMBER, 15) != 0)
		pr_err("BTCS interrupt's priority cann't be
			    guaranteed as 15(highest)\n");
	return 0;
}

static struct resource btcs_resources = {
	.name = "BTCS mem",
	.start = __virt_to_phys(CONFIG_BTCS_START),
	.end = __virt_to_phys(CONFIG_BTCS_START +
		    CONFIG_BTCS_SIZE - 1),
	.flags = IORESOURCE_MEM,
};

static int __exit btcs_release_resource(void)
{
	int ret = 0;
	ret = release_resource(&btcs_resources);
	if (ret)
		pr_err("BTCS: Cannot release resource\n");
	return ret;
}

static int __init btcs_request_resource(void)
{
	int ret = 0;
	ret = request_resource(iomem_resource.child, &btcs_resources);
	if (ret)
		pr_err("BTCS: Cannot reserve resource\n");
	return ret;
}

pure_initcall(btcs_request_resource);
module_exit(btcs_release_resource);
#else

int __init btcs_init(void)
{

}

void __init btcs_reserve_sdram(void)
{

}

void __init set_fld_for_vaddr(unsigned int vaddr, unsigned int value)
{

}

void btcs_poll(void)
{

}

#endif

