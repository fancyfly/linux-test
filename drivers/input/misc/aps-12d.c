/* linux/drivers/input/misc/aps-12d.c
 *
 * Copyright (C) 2009-2010 HUAWEI Corporation.
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/aps-12d.h>

#ifdef DEBUG
#define dbg(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define dbg(fmt, args...)
#endif

#define ABS_LIGHT		ABS_MISC
#define ALS_THRESHOLD		30
#define PROXIMITY_THRESHOLD	5

struct aps_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct mutex mlock;
	struct hrtimer timer;
	struct work_struct work;
	int (*power) (int on);
};

static struct workqueue_struct *aps_wq;
static struct aps_data *this_aps_data;
static int aps_12d_delay = APS_12D_TIMRER;	/*1s */
static uint16_t last_als_count;
static uint16_t last_ps_count;
static atomic_t l_flag;
static atomic_t p_flag;

static inline int aps_i2c_reg_read(struct aps_data *aps, int reg)
{
	int val;

	mutex_lock(&aps->mlock);

	val = i2c_smbus_read_byte_data(aps->client, reg);
	if (val < 0)
		printk(KERN_ERR "aps_i2c_reg_read failed\n");

	mutex_unlock(&aps->mlock);

	return val;
}

static inline int aps_i2c_reg_write(struct aps_data *aps, int reg, uint8_t val)
{
	int ret;

	mutex_lock(&aps->mlock);
	ret = i2c_smbus_write_byte_data(aps->client, reg, val);
	if (ret < 0) {
		printk(KERN_ERR "aps_i2c_reg_write failed\n");
	}
	mutex_unlock(&aps->mlock);

	return ret;
}

static int aps_12d_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int aps_12d_release(struct inode *inode, struct file *file)
{
	if (atomic_read(&l_flag) || atomic_read(&p_flag)) {
		hrtimer_cancel(&this_aps_data->timer);
		aps_12d_delay = APS_12D_TIMRER;
	}

	return 0;
}

static int
aps_12d_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	      unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	short flag;

	switch (cmd) {
	case APS_IOCTL_APP_SET_LFLAG:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		break;
	case APS_IOCTL_APP_SET_PFLAG:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		break;
	case APS_IOCTL_APP_SET_DELAY:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		break;

	default:
		break;
	}

	switch (cmd) {
	case APS_IOCTL_APP_SET_LFLAG:
		if (flag && !atomic_read(&l_flag) && !atomic_read(&p_flag))
			hrtimer_start(&this_aps_data->timer, ktime_set(1, 0),
				    HRTIMER_MODE_REL);
		else if (!flag && (atomic_read(&l_flag) || atomic_read(&p_flag)))
			hrtimer_cancel(&this_aps_data->timer);

		atomic_set(&l_flag, flag);
		break;

	case APS_IOCTL_APP_GET_LFLAG:	/*get open acceleration sensor flag */
		flag = atomic_read(&l_flag);
		break;

	case APS_IOCTL_APP_SET_PFLAG:
		if (flag && !atomic_read(&l_flag) && !atomic_read(&p_flag))
			hrtimer_start(&this_aps_data->timer, ktime_set(1, 0),
				    HRTIMER_MODE_REL);
		else if (!flag && (atomic_read(&l_flag) || atomic_read(&p_flag)))
			hrtimer_cancel(&this_aps_data->timer);

		atomic_set(&p_flag, flag);
		break;

	case APS_IOCTL_APP_GET_PFLAG:	/*get open acceleration sensor flag */
		flag = atomic_read(&p_flag);
		break;

	case APS_IOCTL_APP_SET_DELAY:
		if (flag)
			aps_12d_delay = flag;
		else
			aps_12d_delay = 200;	/*200ms */
		break;

	case APS_IOCTL_APP_GET_DELAY:
		flag = aps_12d_delay;
		break;

	default:
		break;
	}

	switch (cmd) {
	case APS_IOCTL_APP_GET_LFLAG:
		if (copy_to_user(argp, &flag, sizeof(flag)))
			return -EFAULT;
		break;

	case APS_IOCTL_APP_GET_PFLAG:
		if (copy_to_user(argp, &flag, sizeof(flag)))
			return -EFAULT;
		break;

	case APS_IOCTL_APP_GET_DELAY:
		if (copy_to_user(argp, &flag, sizeof(flag)))
			return -EFAULT;

		break;

	default:
		break;
	}
	return 0;

}

static struct file_operations aps_12d_fops = {
	.owner = THIS_MODULE,
	.open = aps_12d_open,
	.release = aps_12d_release,
	.ioctl = aps_12d_ioctl,
};

static struct miscdevice light_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "aps_light",
	.fops = &aps_12d_fops,
};

static struct miscdevice proximity_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "aps_proximity",
	.fops = &aps_12d_fops,
};

static void aps_12d_work_func(struct work_struct *work)
{
	int ret, lsb, msb;
	uint16_t ir_once_val = 0, ps_once_val = 0;
	uint16_t ps_count = 0, als_count = 0;
	struct aps_data *aps = container_of(work, struct aps_data, work);

	if (atomic_read(&p_flag)) {
		/* Command 1 register: IR once */
		ret = aps_i2c_reg_write(aps, APS_12D_REG_CMD1, APS_12D_IR_ONCE);
		msleep(60);
		lsb = aps_i2c_reg_read(aps, APS_12D_DATA_LSB);
		msb = aps_i2c_reg_read(aps, APS_12D_DATA_MSB);
		ir_once_val = (msb << 8) + lsb;

		ret =
		    aps_i2c_reg_write(aps, APS_12D_REG_CMD1,
				      APS_12D_PROXIMITY_ONCE);
		msleep(60);
		lsb = aps_i2c_reg_read(aps, APS_12D_DATA_LSB);
		msb = aps_i2c_reg_read(aps, APS_12D_DATA_MSB) & 0x0F;
		ps_once_val = (msb << 8) + lsb;

		ps_count = ps_once_val - ir_once_val;

		dbg("ps_count=%d, ps_once_val=%d, ir_once_val=%d\n",
			    ps_count, ps_once_val, ir_once_val);

		if (abs(ps_count - last_ps_count) >= PROXIMITY_THRESHOLD) {
			last_ps_count = ps_count;
			input_report_abs(aps->input_dev, ABS_DISTANCE, ps_count);
			input_sync(aps->input_dev);
		}
	}

	if (atomic_read(&p_flag) && atomic_read(&l_flag))
		msleep(120);

	if (atomic_read(&l_flag)) {
		ret =
		    aps_i2c_reg_write(aps, APS_12D_REG_CMD1, APS_12D_ALS_ONCE);
		msleep(60);
		lsb = aps_i2c_reg_read(aps, APS_12D_DATA_LSB);
		msb = aps_i2c_reg_read(aps, APS_12D_DATA_MSB) & 0x0F;
		als_count = (msb << 8) + lsb;

		dbg("als_count=%d \n", als_count);

		if (abs(als_count - last_als_count) >= ALS_THRESHOLD) {
			last_als_count = als_count;

			input_report_abs(aps->input_dev, ABS_LIGHT, als_count);
			input_sync(aps->input_dev);
		}
	}
}

static enum hrtimer_restart aps_timer_func(struct hrtimer *timer)
{
	struct aps_data *aps = container_of(timer, struct aps_data, timer);

	int sesc = aps_12d_delay / 1000;
	int nsesc = (aps_12d_delay % 1000) * 1000000;

	queue_work(aps_wq, &aps->work);

	hrtimer_start(&aps->timer, ktime_set(sesc, nsesc),
			    HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static int aps_12d_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;
	struct aps_data *aps;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "aps_12d_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	aps = kzalloc(sizeof(*aps), GFP_KERNEL);
	if (aps == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	mutex_init(&aps->mlock);

	INIT_WORK(&aps->work, aps_12d_work_func);
	aps->client = client;
	i2c_set_clientdata(client, aps);

	/* Command 2 register: 25mA,DC,12bit,Range1 */
	ret = aps_i2c_reg_write(aps, APS_12D_REG_CMD2,
				(uint8_t) (APS_12D_IRDR_SEL_25MA << 6 |
					   APS_12D_FREQ_SEL_DC << 4 |
					   APS_12D_RES_SEL_12 << 2 |
					   APS_12D_RANGE_SEL_ALS_16000));
	if (ret < 0) {
		goto err_detect_failed;
	}

	aps->input_dev = input_allocate_device();
	if (aps->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR
		       "aps_12d_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	aps->input_dev->name = "aps-12d sensor";
	aps->input_dev->id.bustype = BUS_I2C;
	input_set_drvdata(aps->input_dev, aps);

	ret = input_register_device(aps->input_dev);
	if (ret) {
		printk(KERN_ERR
		       "aps_probe: Unable to register %s input device\n",
		       aps->input_dev->name);
		goto err_input_register_device_failed;
	}

	set_bit(EV_ABS, aps->input_dev->evbit);
	input_set_abs_params(aps->input_dev, ABS_LIGHT, 0, 16000, 0, 0);
	input_set_abs_params(aps->input_dev, ABS_DISTANCE, 0, 4096, 0, 0);

	ret = misc_register(&light_dev);
	if (ret) {
		printk(KERN_ERR
		       "aps_12d_probe: light_dev register failed\n");
		goto err_light_misc_device_register_failed;
	}

	ret = misc_register(&proximity_dev);
	if (ret) {
		printk(KERN_ERR
		       "aps_12d_probe: proximity_dev register failed\n");
		goto err_proximity_misc_device_register_failed;
	}

	hrtimer_init(&aps->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aps->timer.function = aps_timer_func;

	aps_wq = create_singlethread_workqueue("aps_wq");

	if (!aps_wq) {
		ret = -ENOMEM;
		goto err_create_workqueue_failed;
	}

	this_aps_data = aps;

	printk(KERN_INFO "aps_12d_probe end\n");

	return 0;

err_create_workqueue_failed:
	misc_deregister(&proximity_dev);
err_proximity_misc_device_register_failed:
	misc_deregister(&light_dev);
err_light_misc_device_register_failed:
err_input_register_device_failed:
	input_free_device(aps->input_dev);
err_input_dev_alloc_failed:
err_detect_failed:
	kfree(aps);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;

}

static int aps_12d_remove(struct i2c_client *client)
{
	struct aps_data *aps = i2c_get_clientdata(client);

	hrtimer_cancel(&aps->timer);

	misc_deregister(&light_dev);
	misc_deregister(&proximity_dev);

	input_unregister_device(aps->input_dev);

	kfree(aps);
	return 0;
}

static int aps_12d_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct aps_data *aps = i2c_get_clientdata(client);

	hrtimer_cancel(&aps->timer);
	ret = cancel_work_sync(&aps->work);

	ret = aps_i2c_reg_write(aps, APS_12D_REG_CMD1, APS_12D_POWER_DOWN);

	if (aps->power) {
		ret = aps->power(0);
		if (ret < 0)
			printk(KERN_ERR "aps_12d_suspend power off failed\n");
	}
	return 0;
}

static int aps_12d_resume(struct i2c_client *client)
{
	int ret;
	struct aps_data *aps = i2c_get_clientdata(client);

	/* Command 2 register: 25mA,DC,12bit,Range2 */
	ret = aps_i2c_reg_write(aps, APS_12D_REG_CMD2,
				(uint8_t) (APS_12D_IRDR_SEL_25MA << 6 |
					   APS_12D_FREQ_SEL_DC << 4 |
					   APS_12D_RES_SEL_12 << 2 |
					   APS_12D_RANGE_SEL_ALS_16000));

	hrtimer_start(&aps->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	return 0;
}

static const struct i2c_device_id aps_id[] = {
	{"aps-12d", 0},
	{}
};

static struct i2c_driver aps_driver = {
	.probe = aps_12d_probe,
	.remove = aps_12d_remove,
	.suspend = aps_12d_suspend,
	.resume = aps_12d_resume,
	.id_table = aps_id,
	.driver = {
		   .name = "aps-12d",
		   },
};

static int __devinit aps_12d_init(void)
{
	return i2c_add_driver(&aps_driver);
}

static void __exit aps_12d_exit(void)
{
	i2c_del_driver(&aps_driver);
	if (aps_wq)
		destroy_workqueue(aps_wq);
}

device_initcall_sync(aps_12d_init);
module_exit(aps_12d_exit);

MODULE_DESCRIPTION("Proximity Driver");
MODULE_LICENSE("GPL");
