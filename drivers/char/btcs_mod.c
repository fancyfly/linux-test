/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Author: John Rigby, jrigby@freescale.com
 * Original Author: Bernhard Kuhn, bkuhn@freescale.com
 */

/*
 * ISR support for BTCS
 */

/*
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <linux/uaccess.h>
#include <linux/fcntl.h>

#include <linux/btcs_defs.h>

static DEFINE_SPINLOCK(btcs_lock);
static DECLARE_WAIT_QUEUE_HEAD(btcs_wait);

static struct btcs_cb_interface *btcs_interface =
    (struct btcs_cb_interface *)CONFIG_BTCS_START;

static int btcs_register_changed[BTCS_NUM_REGS];
static int btcs_register_fifo_data[BTCS_NUM_REGS];
static int btcs_register_fifo_ipos;
static int btcs_register_fifo_opos;
static int btcs_register_fifo_cnt;

static int btcs_register_fifo_push(int num)
{
	if (btcs_register_fifo_cnt < BTCS_NUM_REGS) {
		btcs_register_fifo_data[btcs_register_fifo_ipos] = num;
		btcs_register_fifo_ipos++;
		if (btcs_register_fifo_ipos == BTCS_NUM_REGS)
			btcs_register_fifo_ipos = 0;
		btcs_register_fifo_cnt++;
		return 0;
	}
	return 1;
}

static int btcs_register_fifo_pull(int *num)
{
	if (btcs_register_fifo_cnt > 0) {
		*num = btcs_register_fifo_data[btcs_register_fifo_opos];
		btcs_register_fifo_opos++;
		if (btcs_register_fifo_opos == BTCS_NUM_REGS)
			btcs_register_fifo_opos = 0;
		btcs_register_fifo_cnt--;
		return 0;
	}
	return 1;
}

static int btcs_ossignal(struct btcs_event *event)
{
	int num;
	switch (event->cmd) {
	case BTCS_REG_MODIFIED:
		num = (int)event->arg;
		if (!btcs_register_changed[num]) {
			btcs_register_changed[num] = 1;
			btcs_register_fifo_push(num);
			wake_up_interruptible(&btcs_wait);
		}
		break;
	}
	return 0;
}

static int btcs_get(int num, int *value)
{
	if (num < 0)
		return -EINVAL;
	if (num >= BTCS_NUM_REGS)
		return -EINVAL;
	*value = btcs_interface->shm->reg[num];
	return 0;
}

static int btcs_set(int num, int value)
{
	unsigned long flags;
	struct btcs_event event;
	if (num < 0)
		return -EINVAL;
	if (num >= BTCS_NUM_REGS)
		return -EINVAL;

	btcs_interface->shm->reg[num] = value;
	event.cmd = BTCS_REG_MODIFIED;
	event.arg = (void *)num;

	local_irq_save(flags);
	btcs_interface->callback(&event);
	local_irq_restore(flags);

	return 0;
}

#define BTCS_CMD_GET  0x00010000
#define BTCS_CMD_SET  0x00020000
#define BTCS_CMD_MASK 0xffff0000
#define BTCS_NUM_MASK 0x0000ffff

static ssize_t btcs_read(struct file *file, char *buf,
			 size_t count, loff_t *ppos)
{

	ssize_t retval;
	int empty, num = 0;

	DECLARE_WAITQUEUE(wait, current);
	if (count != sizeof(unsigned long))
		return -EINVAL;

	add_wait_queue(&btcs_wait, &wait);
	current->state = TASK_INTERRUPTIBLE;

	do {
		spin_lock_irq(&btcs_lock);
		empty = btcs_register_fifo_pull(&num);
		if (!empty)
			btcs_register_changed[num] = 0;
		spin_unlock_irq(&btcs_lock);

		if (!empty)
			break;

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		}
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	} while (1);

	retval = put_user(num, (unsigned long *)buf);
	if (!retval)
		retval = sizeof(unsigned long);

out:
	current->state = TASK_RUNNING;
	remove_wait_queue(&btcs_wait, &wait);

	return retval;
}

static int btcs_ioctl(struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int status;
	u32 val;

	switch (cmd & BTCS_CMD_MASK) {

	case BTCS_CMD_GET:
		status = btcs_get(cmd & BTCS_NUM_MASK, &val);
		if (status != 0)
			return status;

		return put_user(val, (u32 *) arg);

	case BTCS_CMD_SET:
		if (get_user(val, (u32 *) arg))
			return -EFAULT;

		return btcs_set(cmd & BTCS_NUM_MASK, val);

	default:
		return -ENOIOCTLCMD;

	}

	return 0;
}

static int btcs_open(struct inode *inode, struct file *file)
{
	btcs_interface->ossignal = btcs_ossignal;
	return 0;
}

static int btcs_release(struct inode *inode, struct file *file)
{
	btcs_interface->ossignal = 0;
	return 0;
}

static const struct file_operations btcs_fops = {
      .owner = THIS_MODULE,
      .ioctl = btcs_ioctl,
      .read = btcs_read,
      .open = btcs_open,
      .release = btcs_release,
};

static struct miscdevice btcs_miscdev = {
	MISC_DYNAMIC_MINOR,
	"btcs",
	&btcs_fops
};

int __init btcs_module_init(void)
{
	printk(KERN_INFO "BTCS interface driver\n");
	misc_register(&btcs_miscdev);
	return 0;
};

void __exit btcs_module_exit(void)
{
	misc_deregister(&btcs_miscdev);
};

module_init(btcs_module_init);
module_exit(btcs_module_exit);

MODULE_DESCRIPTION("BTCS interface driver");
MODULE_AUTHOR("Bernhard Kuhn <bkuhn@freescale.com");
MODULE_LICENSE("GPL");
