/*
 * driver/input/touchscreen/guitar_update.c
 *
 * Copyright (C) 2010 Goodix, Inc. All rights reserved.  
 * Author: Eltonny; Hua.Zhong
 * Date:   2010.9.26	
 *
 */
/* Copyright (c) 2011 Freescale Semiconductor, Inc. */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/reboot.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <linux/goodix_touch.h>

#if !defined(GUITAR_GT80X) && !defined(MX53_PCBA_TOUCH_RST)
#error The code does not match this touchscreen.
#endif

#define guitar_iap_log  "GT80X Update"
#define debug_printk(fmt, arg...)  printk("%s: "fmt, guitar_iap_log, ##arg)

enum ts_state{
	GOODIX_TS_STOP		=	0,
	GOODIX_TS_CONTINUE	=	1,
}; 

extern struct i2c_client * i2c_connect_client;

static void Set_ShutdownPin(int state)
{
	static int isRequested = 0;
	if(isRequested)
		gpio_set_value(MX53_PCBA_TOUCH_RST, state);
	else
	{
		gpio_request(MX53_PCBA_TOUCH_RST, "TS_SHUTDOWN");
		gpio_direction_output(MX53_PCBA_TOUCH_RST, state);
		isRequested++;
	}	
}

static inline void Guitar_ShutdownPinOutputHigh(void)
{
	Set_ShutdownPin(1);	
}

static inline void Guitar_ShutdownPinOutputLow(void)
{
	Set_ShutdownPin(0);
}


/*******************************************************
function:
	set Guitar driver running mode
parameters:
	cmd: command code,GOODIX_TS_CONTINUE,start: GOODIX_TS_STOP,stop
return:
	the action result,1 means ok
*******************************************************/
static int goodix_ts_set(int cmd)
{
	int ret = -1;
	struct goodix_ts_data * ts = NULL;
	if(i2c_connect_client != NULL)
		ts = i2c_get_clientdata(i2c_connect_client);
	if(ts == NULL)
		return 0;
	switch(cmd)
	{
	case GOODIX_TS_STOP:
		if(ts->use_irq)
			disable_irq(ts->client->irq);
		ret = cancel_work_sync(&ts->work);
		if (ret && ts->use_irq) /* disable irq twice, must enable it*/
			enable_irq(ts->client->irq);
		else
			if(ts->timer.state)
				hrtimer_cancel(&ts->timer);
		if(ts->use_shutdown)
			unregister_early_suspend(&ts->early_suspend);
		break ;
	case GOODIX_TS_CONTINUE:
		if(ts->use_irq)
			enable_irq(ts->client->irq);
		else
			hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		if(ts->use_shutdown)
			register_early_suspend(&ts->early_suspend);
		break ;
		
	}
	msleep(2);
	return 1;
}


static struct proc_dir_entry *goodix_proc_entry;
static const char * node_name = "driver/gt80x-update";
#define MAX_LENGTH 100+1

#define ADAPTER_NUM	1

enum cmd_type {
#define I2C_IOCTL 0x10
	I2C_WRITE_BYTES		= I2C_IOCTL|0x00,
	I2C_READ_ADDR			= I2C_IOCTL|0x02,
	I2C_READ_BYTES		= I2C_IOCTL|0x03,
#define CMD_IOCTL 0x80
	SHUTDOWN_SET_LOW		= CMD_IOCTL|0x00,
	SHUTDOWN_SET_HIGH 	= CMD_IOCTL|0x01,
	SHUTDOWN_GET_STATE	= CMD_IOCTL|0x02,
	TOUCHSCREEN_STOP		= CMD_IOCTL|0x04,
	TOUCHSCREEN_CONTINUE	= CMD_IOCTL|0x05,
	SYSTEM_REBOOT			= CMD_IOCTL|0x10,
	READ_VERSION			= 0x00,
	READ_COMMAND			= 0xFF
};

static int proc_i2c_read_bytes(uint8_t slave_addr, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	/*Send write address*/
	msgs[0].flags=!I2C_M_RD;
	msgs[0].addr=slave_addr;
	msgs[0].len=1;
	msgs[0].buf=&buf[0];
	/*Receive message*/
	msgs[1].flags=I2C_M_RD;
	msgs[1].addr=slave_addr;
	msgs[1].len=len-1;
	msgs[1].buf=&buf[1];
	
	ret = i2c_transfer(i2c_get_adapter(ADAPTER_NUM), msgs, 2);
	return ret;
}

static int proc_i2c_write_bytes(uint8_t slave_addr, uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	/*Send device address*/
	msg.flags=!I2C_M_RD;
	msg.addr=slave_addr;
	msg.len=len;
	msg.buf=data;		
	
	ret = i2c_transfer(i2c_get_adapter(ADAPTER_NUM), &msg, 1);
	return ret;
}


static uint8_t read_command = READ_VERSION;
static uint8_t slave_addr;
static uint8_t read_addr;


static int goodix_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
	int ret = 0;
	uint8_t type;
	uint8_t buffer[MAX_LENGTH+1];

	if(len > MAX_LENGTH)
		return -EINVAL;
	
	if(copy_from_user(buffer, buff, len))
		return -EFAULT;

	type = buffer[0];
	switch(type)
	{
		case I2C_WRITE_BYTES:
			ret = proc_i2c_write_bytes(buffer[1], buffer+2, len-2);
			break;
		case SHUTDOWN_SET_LOW:
			Guitar_ShutdownPinOutputLow();
			break;
		case SHUTDOWN_SET_HIGH:
			Guitar_ShutdownPinOutputHigh();
			break;
		case TOUCHSCREEN_STOP:
			goodix_ts_set(GOODIX_TS_STOP);
			break;
		case TOUCHSCREEN_CONTINUE:
			goodix_ts_set(GOODIX_TS_CONTINUE);
			break;
		case SYSTEM_REBOOT:
			debug_printk("Kernel: Restart system command.\n");
			msleep(500);
			kernel_restart(NULL);
			break;
		case READ_COMMAND:
			read_command = buffer[1];
			break;
		case I2C_READ_ADDR:
			read_command = buffer[1];
			slave_addr = buffer[2];
			read_addr = buffer[3];
			break;
		default:
			debug_printk("No such command.\n");
			return -ENOSYS;
		
	}

	if(type != I2C_WRITE_BYTES)
		ret = len;
	else
		ret = (ret == 1?len-2:-1);
	
	return ret;	
}

#define GT80X_VERSION_LENGTH	40	
static const char * error_msg = "Version data:NULL\n";

static int  goodix_read_version(char * page)
{
	int ret;
	uint8_t version[2] = {0x69,0xff}; 
	uint8_t version_data[GT80X_VERSION_LENGTH+2];
	
	ret=proc_i2c_write_bytes(0x55, version,2);
	if (ret < 0) 
		goto error_i2c_version;
	msleep(50);				
	version_data[0] = 0x6A;
	memset(version_data+1, 0, GT80X_VERSION_LENGTH+1);
	ret=proc_i2c_read_bytes(0x55, version_data, GT80X_VERSION_LENGTH);
	if (ret < 0) 
		goto error_i2c_version;
	
	memcpy(page, version_data+1, GT80X_VERSION_LENGTH);
	version_data[GT80X_VERSION_LENGTH] = '\n';
	version[1] = 0x00;			
	proc_i2c_write_bytes(0x55, version, 2);
	return 0;
	
error_i2c_version:
	debug_printk("Warning: GT80X's firmware might be failure.\n");
	memcpy(page, error_msg, sizeof(error_msg));
	return ret;
}

/******************************************************
 *proc read data, copy data from kernel space to user space
 *Parameters:
 * 	*page: 	kernel cache,set to page size in common
 * 	**start: Set the data offset in the memory page, not set in common
 *return:
 *	The read bytes 
 *******************************************************/

 
static int goodix_update_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	
	int ret = 0;
	uint8_t type = read_command;
	
	if(type == READ_VERSION)
		count = GT80X_VERSION_LENGTH;
		
	if(count < 0 || count > MAX_LENGTH)
		return -EINVAL;

	switch(type)
	{
	case READ_VERSION:
		ret = goodix_read_version(page);
		ret = (ret == 0? GT80X_VERSION_LENGTH:-1);
		break;
	case SHUTDOWN_GET_STATE:
		ret = 1;
		ret = gpio_get_value(MX53_PCBA_TOUCH_RST);
		break;
	case I2C_READ_BYTES:
		page[0] = read_addr;
		ret = proc_i2c_read_bytes(slave_addr, page, count);
		ret = (ret == 2?count:-1);
		break;
	default:	
		return -ENOSYS;
	}

	*eof = true;
	*start = NULL;	
	/* reset read command. */
	read_command = READ_VERSION;
	return ret;
}


static int __init init_proc(void)
{
	int ret = -ENODEV;

	goodix_proc_entry = create_proc_entry(node_name, 0666, NULL);
	if(goodix_proc_entry == NULL)
	{
		debug_printk("Can't create proc entry!\n");
		ret = -ENOMEM;
		goto err_create_proc_entry;
	}
	
	debug_printk("Create proc entry success!\n");
	goodix_proc_entry->write_proc = goodix_update_write;
	goodix_proc_entry->read_proc = goodix_update_read;
	ret = 0;

err_create_proc_entry:
	return ret;
}

static void __exit exit_proc(void)
{
	debug_printk("Prepare to exit guitar-update module.\n");
	remove_proc_entry(node_name, NULL);
}

late_initcall(init_proc);
module_exit(exit_proc);

MODULE_AUTHOR("Eltonny");
MODULE_DESCRIPTION("A linux/Android kernel Module for GT80X updating.");
MODULE_LICENSE("GPL v2");


