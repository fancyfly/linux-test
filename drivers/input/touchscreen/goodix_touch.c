/**
 * driver/input/touchscreen/goodix_touch.c
 *
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Change Date:
 *		2010.11.11, add point_queue's definiens.
 *		2011.03.09, rewrite point_queue's definiens.
 *		2011.05.12, delete point_queue for
 *		Android 2.2/Android 2.3 and so on.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <mach/i2c.h>
#include <mach/irqs.h>
#include <mach/iomux-mx53.h>
#include <mach/gpio.h>

#include <linux/goodix_touch.h>

#ifndef GUITAR_GT80X
#error The code does not match the hardware version.
#endif

const char *imx_ts_name = "Goodix TouchScreen of GT80X";
static struct workqueue_struct *goodix_wq;

/*used by GT80X-IAP module */
struct i2c_client *i2c_connect_client;
EXPORT_SYMBOL(i2c_connect_client);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif

/*
 *Function as i2c_master_receive, and return 2 if operation is successful.
 */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf,
			  uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret = -1;
	/*Send write address */
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr = client->addr;
	msgs[0].len = 1;	/*data address */
	msgs[0].buf = buf;
	/*Receive data */
	msgs[1].flags = I2C_M_RD;	/*Read message */
	msgs[1].addr = client->addr;
	msgs[1].len = len - 1;
	msgs[1].buf = buf + 1;

	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret;
}

/*
 *Function as i2c_master_send, and return 1 if operation is successful.
 */
static int i2c_write_bytes(struct i2c_client *client, uint8_t * data,
			   uint16_t len)
{
	struct i2c_msg msg;
	int ret = -1;

	msg.flags = !I2C_M_RD;	/*Write message */
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);
	return ret;
}

static bool goodix_init_panel(struct goodix_ts_data *ts)
{
	int ret = -1;
	int count;

	uint8_t config_info[54] = {
		0x30, 0x19, 0x05, 0x06, 0x28, 0x02, 0x14, 0x20,
		0x10, 0x32, 0xB0, TOUCH_MAX_WIDTH >> 8,
		TOUCH_MAX_WIDTH & 0xFF, TOUCH_MAX_HEIGHT >> 8,
		TOUCH_MAX_HEIGHT & 0xFF, 0xED, 0xCB, 0xA9,
		0x87, 0x65, 0x43, 0x21, 0x00, 0x00, 0x00, 0x37, 0x2E,
		0x4D, 0xC1, 0x20, 0x03, 0x03, 0x83, 0x50, 0x3C, 0x1E,
		0xB4, 0x00, 0x34, 0x2C, 0x01, 0xEC, 0x00, 0x3C, 0x00, 0x00,
		    0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};

	for (count = 5; count > 0; count--) {
		ret = i2c_write_bytes(ts->client, config_info, 54);
		if (ret == 1)	/*Initiall success */
			break;
		else
			msleep(10);
	}

	return ret == 1 ? true : false;
}

/*Read and print the GT80X version message*/
static int goodix_read_version(struct goodix_ts_data *ts)
{
#define GT80X_VERSION_LENGTH	40
	int ret;
	uint8_t version[2] = { 0x69, 0xff };
	uint8_t version_data[GT80X_VERSION_LENGTH + 1] = { 0x6A };
	memset(version_data + 1, 0, GT80X_VERSION_LENGTH);
	ret = i2c_write_bytes(ts->client, version, 2);
	if (ret != 1)
		return ret;

	msleep(50);
	ret = i2c_read_bytes(ts->client, version_data, GT80X_VERSION_LENGTH);
	if (ret != 2)
		strncpy(version_data + 1, "NULL", 4);
	dev_info(&ts->client->dev, "GT80X Version: %s\n", version_data + 1);
	version[1] = 0x00;	/*cancel the command */
	i2c_write_bytes(ts->client, version, 2);
	return 0;
}

static void goodix_ts_work_func(struct work_struct *work)
{
	static struct point_node pointer[MAX_FINGER_NUM];
	static uint8_t finger_last;

	struct point_node *p = NULL;
	uint8_t read_position = 0;
	uint8_t point_data[READ_BYTES_NUM] = { 0 };
	uint8_t finger, finger_current;
	uint8_t check_sum = 0;
	unsigned int x, y;
	int count = 0;
	int ret = -1;

	struct goodix_ts_data *ts =
	    container_of(work, struct goodix_ts_data, work);

	if (ts->use_shutdown && gpio_get_value(ts->gpio_shutdown))
		goto NO_ACTION;	/*The data is invalid. */

	/* if i2c-transfer is failed,
	   let it restart less than 10 times. FOR DEBUG. */
	ret = i2c_read_bytes(ts->client, point_data, sizeof(point_data));
	if (ret <= 0) {
		dev_dbg(&(ts->client->dev),
			"I2C transfer error. ERROR Number:%d\n ", ret);
		ts->retry++;
		if (ts->retry >= 100) {	/*It's not normal for too much i2c-error. */
			dev_err(&(ts->client->dev),
				"Reset the chip for i2c error.\n ");
			ts->retry = 0;
			if (ts->power) {
				ts->power(ts, 0);
				ts->power(ts, 1);
			} else {
				goodix_init_panel(ts);
				msleep(200);
			}
		}
		goto XFER_ERROR;
	}

	if (!ts->use_irq) {
		switch (point_data[1] & 0x1f) {
		case 0:
			break;
		case 1:
			for (count = 1; count < 8; count++)
				check_sum += (int)point_data[count];
			read_position = 8;
			break;
		case 2:
		case 3:
			for (count = 1; count < 13; count++)
				check_sum += (int)point_data[count];
			read_position = 13;
			break;
		default:
			for (count = 1; count < 34; count++)
				check_sum += (int)point_data[count];
			read_position = 34;
		}
		if (check_sum != point_data[read_position])
			goto XFER_ERROR;
	}

	/*The bits indicate which fingers pressed down */
	finger_current = point_data[1] & 0x1f;
	finger = finger_current ^ finger_last;
	if (finger == 0 && finger_current == 0)
		goto NO_ACTION;	/*no action */
	else if (finger == 0)
		goto BIT_NO_CHANGE;	/*the same as last time */

	/*check which point(s) DOWN or UP */
	for (count = 0; count < MAX_FINGER_NUM; count++) {
		p = &pointer[count];
		p->id = count;
		if ((finger_current & FLAG_MASK) != 0)
			p->state = FLAG_DOWN;
		else {
			if ((finger & FLAG_MASK) != 0)	/*send press release. */
				p->state = FLAG_UP;
			else
				p->state = FLAG_INVALID;
		}

		finger >>= 1;
		finger_current >>= 1;
	}
	finger_last = point_data[1] & 0x1f;	/*restore last presse state. */

      BIT_NO_CHANGE:
	for (count = 0; count < MAX_FINGER_NUM; count++) {
		p = &pointer[count];
		if (p->state == FLAG_INVALID)
			continue;

		if (p->state == FLAG_UP) {
			x = y = 0;
			p->pressure = 0;
			continue;
		}

		if (p->id < 3)
			read_position = p->id * 5 + 3;
		else
			read_position = 29;

		if (p->id != 3) {
			x = (unsigned int)(point_data[read_position] << 8) +
			    (unsigned int)(point_data[read_position + 1]);
			y = (unsigned int)(point_data[read_position + 2] << 8) +
			    (unsigned int)(point_data[read_position + 3]);
			p->pressure = point_data[read_position + 4];
		}
#if MAX_FINGER_NUM > 3
		else {
			x = (unsigned int)(point_data[18] << 8) +
			    (unsigned int)(point_data[25]);
			y = (unsigned int)(point_data[26] << 8) +
			    (unsigned int)(point_data[27]);
			p->pressure = point_data[28];
		}
#endif
		#if defined(CONFIG_AT070TN93)
		x = x * SCREEN_MAX_WIDTH / TOUCH_MAX_WIDTH;	/*y */
		y = (TOUCH_MAX_HEIGHT - y) * SCREEN_MAX_HEIGHT / TOUCH_MAX_HEIGHT;	/*x */
		#endif
		#if defined(CONFIG_AT070TN2_WSVGA)
		x = (TOUCH_MAX_WIDTH - x) * SCREEN_MAX_WIDTH / TOUCH_MAX_WIDTH;   /*y */
		y = y * SCREEN_MAX_HEIGHT / TOUCH_MAX_HEIGHT;    /*x */
		#endif
		swap(x, y);

		p->x = x;
		p->y = y;
	}

#ifndef GOODIX_MULTI_TOUCH
	if (pointer[0].state == FLAG_DOWN) {
		input_report_abs(ts->input_dev, ABS_X, pointer[0].x);
		input_report_abs(ts->input_dev, ABS_Y, pointer[0].y);
	}
	input_report_abs(ts->input_dev, ABS_PRESSURE, pointer[0].pressure);
	input_report_key(ts->input_dev, BTN_TOUCH,
			 pointer[0].state ==
			 FLAG_INVALID ? FLAG_UP : pointer[0].state);
#else

	/* ABS_MT_TOUCH_MAJOR is used as ABS_MT_PRESSURE in android. */
	for (count = 0; count < MAX_FINGER_NUM; count++) {
		p = &pointer[count];

		if (p->state == FLAG_INVALID)
			continue;

		if (p->state == FLAG_DOWN) {
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
					 p->x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
					 p->y);
			dev_dbg(&(ts->client->dev), "Id:%d, x:%d, y:%d\n",
				p->id, p->x, p->y);
		}
		input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, p->id);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
				 p->pressure);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
				 p->pressure);

		input_mt_sync(ts->input_dev);
	}
#endif
	input_sync(ts->input_dev);

      XFER_ERROR:
      NO_ACTION:
	if (ts->use_irq)
		enable_irq(ts->client->irq);

}

/*******************************************************
Function:
	The timer function.
Parameters:
	timer: the timer used to count the time.
Return:
	The timer work mode, HRTIMER_NORESTART means no reboot.
********************************************************/
static enum hrtimer_restart goodix_ts_timer_func(struct hrtimer *timer)
{
	struct goodix_ts_data *ts =
	    container_of(timer, struct goodix_ts_data, timer);

	queue_work(goodix_wq, &ts->work);
	if (ts->timer.state != HRTIMER_STATE_INACTIVE)
		hrtimer_start(&ts->timer, ktime_set(0, 16000000),
			      HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	interrupt request function
********************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
	struct goodix_ts_data *ts = dev_id;

	dev_dbg(&ts->client->dev, "Touch:Get irq!\n");

	disable_irq_nosync(ts->client->irq);
	queue_work(goodix_wq, &ts->work);

	return IRQ_HANDLED;
}

/*******************************************************
Function:
	GT80x power management
Parameters:
	on: set GT80x running mode, 0 means sleep
Return:
	If the set succeed, lease then 0 means fail
********************************************************/
static int goodix_ts_power(struct goodix_ts_data *ts, int on)
{
	int ret = 0;
	if (!ts->use_shutdown)
		return -1;

	switch (on) {
	case 0:
		dev_info(&ts->client->dev, "Touch:power off!\n");
		gpio_set_value(ts->gpio_shutdown, 1);
		msleep(5);
		if (gpio_get_value(ts->gpio_shutdown))	/*has been suspended */
			ret = 0;
		break;
	case 1:
		dev_info(&ts->client->dev, "Touch:power on!\n");
		gpio_set_value(ts->gpio_shutdown, 0);
		msleep(5);
		if (gpio_get_value(ts->gpio_shutdown))	/*has been waked up */
			ret = -1;
		else
			msleep(200);
		break;
	}
	dev_dbg(&ts->client->dev, "Set Guitar's Shutdown %s. Ret:%d.\n",
		on ? "HIGH" : "LOW", ret);
	return ret;
}

static bool goodix_i2c_test(struct i2c_client *client)
{
	int ret, retry;
	uint8_t test_data[1] = { 0 };	/*only write a data address. */

	for (retry = 0; retry < 5; retry++) {
		ret = i2c_write_bytes(client, test_data, 1);
		if (ret == 1)
			break;
		msleep(5);
	}

	return ret == 1 ? true : false;
}

/*******************************************************
Function:
	Touch probe function
Parameters:
	client: the device struc to be driven.
	id: the device id.
return:
	The action result, 0 means ok.
********************************************************/
static int goodix_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct goodix_ts_data *ts;
	int ret = 0;

	struct goodix_i2c_platform_data *pdata;
	dev_dbg(&client->dev, "Install touchscreen driver for guitar.\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "System need I2C function.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	/*Get the resource pre-defined */
	pdata = client->dev.platform_data;
	if (pdata != NULL) {	/* use this pdata such as that: */
		ts->gpio_shutdown = pdata->gpio_shutdown;
		ts->gpio_irq = pdata->gpio_irq;
		client->irq = gpio_to_irq(ts->gpio_irq);
		dev_dbg(&client->dev, "Touch:probe irq %d!\n", client->irq);
		set_irq_type(gpio_to_irq(ts->gpio_irq), IRQF_TRIGGER_RISING);

		dev_dbg(&client->dev,
			"Touch:Get gpio_shutdown %d,gpio_irq %d, from the probe.",
			ts->gpio_shutdown, ts->gpio_irq);
	} else {		/*If pdata is not used, get value from head files. */
		dev_err(&client->dev, "No platform data defined\n");
	}

	if (ts->gpio_shutdown) {
		gpio_direction_output(ts->gpio_shutdown, 0);
		msleep(2);
		ret = gpio_get_value(ts->gpio_shutdown);
		if (ret) {
			dev_warn(&client->dev,
				 "Can't set touchscreen to work.\n");
			goto err_i2c_failed;
		}
		ts->use_shutdown = true;
		msleep(25);	/*waiting for initialization of Guitar. */
	} else
		ts->use_shutdown = false;

	i2c_connect_client = client;	/*used by Guitar Updating. */

	ret = goodix_i2c_test(client);
	if (!ret) {
		dev_err(&client->dev,
			"Warnning: I2C connection might be something wrong!\n");
		goto err_i2c_failed;
	}

	dev_dbg(&client->dev, "Touch:Test i2c ok!");

	if (ts->use_shutdown)
		gpio_set_value(ts->gpio_shutdown, 1);	/*suspend */

	INIT_WORK(&ts->work, goodix_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		dev_dbg(&client->dev, "Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->evbit[0] =
	    BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

#ifndef GOODIX_MULTI_TOUCH
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	input_set_abs_params(ts->input_dev, ABS_X, 0, SCREEN_MAX_HEIGHT, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, SCREEN_MAX_WIDTH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
#else
	ts->input_dev->absbit[0] = BIT_MASK(ABS_MT_TRACKING_ID) |
	    BIT_MASK(ABS_MT_TOUCH_MAJOR) | BIT_MASK(ABS_MT_WIDTH_MAJOR) |
	    BIT_MASK(ABS_MT_POSITION_X) | BIT_MASK(ABS_MT_POSITION_Y);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0,
			     SCREEN_MAX_HEIGHT, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0,
			     SCREEN_MAX_WIDTH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0,
			     MAX_FINGER_NUM, 0, 0);
#endif

	sprintf(ts->phys, "input/goodix-ts");
	ts->input_dev->name = imx_ts_name;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0xDEAD;
	ts->input_dev->id.product = 0xBEEF;
	ts->input_dev->id.version = 0x1105;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev, "Unable to register %s input device\n",
			ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	if (client->irq) {
		dev_dbg(&client->dev, "Touch:request irq %d!\n", client->irq);
		ret =
		    request_irq(client->irq, goodix_ts_irq_handler,
				IRQ_TYPE_EDGE_RISING, client->name, ts);
		if (ret != 0) {
			dev_err(&client->dev,
				"Can't allocate touchscreen's interrupt! ERRNO:%d\n",
				ret);
			gpio_direction_input(ts->gpio_irq);
			gpio_free(ts->gpio_irq);
			ts->use_irq = false;
			goto err_int_request_failed;
		} else {
			dev_dbg(&client->dev, "Touch:request irq ok!");
			disable_irq(client->irq);
			ts->use_irq = true;
			dev_dbg(&client->dev,
				"Reques EIRQ %d succesd on GPIO:%d\n",
				client->irq, ts->gpio_irq);
		}
	} else
		ts->use_irq = false;

      err_int_request_failed:
	if (!ts->use_irq) {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = goodix_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	flush_workqueue(goodix_wq);
	if (ts->use_shutdown) {
		dev_info(&client->dev, "Touch:set shutdown to 0!\n");
		msleep(100);
		gpio_set_value(ts->gpio_shutdown, 0);
		dev_info(&client->dev, "Touch:The shutdown is %d.\n",
			 gpio_get_value(ts->gpio_shutdown));
		ts->power = goodix_ts_power;
		msleep(100);
	}

	ret = goodix_init_panel(ts);
	ret = 1;
	if (!ret)
		goto err_init_godix_ts;
	else
		dev_info(&client->dev, "Touch:init panel ok!\n");

	if (ts->use_irq)
		enable_irq(client->irq);

	goodix_read_version(ts);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = goodix_ts_early_suspend;
	ts->early_suspend.resume = goodix_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	dev_dbg(&client->dev, "Start  %s in %s mode\n",
		ts->input_dev->name, ts->use_irq ? "Interrupt" : "Polling");
	return 0;

      err_init_godix_ts:
	if (ts->use_irq) {
		free_irq(client->irq, ts);
		gpio_free(ts->gpio_irq);
	}

      err_input_register_device_failed:
	input_free_device(ts->input_dev);

      err_input_dev_alloc_failed:
	i2c_set_clientdata(client, NULL);
      err_i2c_failed:
	if (ts->use_shutdown) {
		gpio_direction_input(ts->gpio_shutdown);
		gpio_free(ts->gpio_shutdown);
	}
	kfree(ts);
      err_alloc_data_failed:
      err_check_functionality_failed:
	return ret;
}

/*******************************************************
Function:
	Release the driver resource
Parameters:
	client: the device struct
return:
	The action result code, 0 means ok.
********************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	if (ts->use_irq) {
		free_irq(client->irq, ts);
		gpio_free(ts->gpio_irq);
	} else
		hrtimer_cancel(&ts->timer);

	if (ts->use_shutdown) {
		gpio_direction_input(ts->gpio_shutdown);
		gpio_free(ts->gpio_shutdown);
	}

	dev_notice(&client->dev, "The driver is removing...\n");
	i2c_set_clientdata(client, NULL);
	input_unregister_device(ts->input_dev);
	if (ts->input_dev)
		kfree(ts->input_dev);
	kfree(ts);
	return 0;
}

/*Suspend the device*/
static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	if (ts->use_irq)
		disable_irq(client->irq);
	else if (ts->timer.state)
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq)	/*irq was disabled twice. */
		enable_irq(client->irq);

	if (ts->power) {
		ret = ts->power(ts, 0);
		if (ret < 0)
			dev_warn(&client->dev, "%s power off failed\n",
				 imx_ts_name);
	}
	return 0;
}

/*Resume the device*/
static int goodix_ts_resume(struct i2c_client *client)
{
	int ret;
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	if (ts->power) {
		ret = ts->power(ts, 1);
		if (ret < 0)
			dev_warn(&client->dev, "%s power on failed\n",
				 imx_ts_name);
	}

	if (ts->use_irq)
		enable_irq(client->irq);
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
	struct goodix_ts_data *ts;
	ts = container_of(h, struct goodix_ts_data, early_suspend);
	goodix_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void goodix_ts_late_resume(struct early_suspend *h)
{
	struct goodix_ts_data *ts;
	ts = container_of(h, struct goodix_ts_data, early_suspend);
	goodix_ts_resume(ts->client);
}
#endif

/*only one client*/
static const struct i2c_device_id goodix_ts_id[] = {
	{GOODIX_I2C_NAME, 0},
};

static struct i2c_driver goodix_ts_driver = {
	.probe = goodix_ts_probe,
	.remove = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = goodix_ts_suspend,
	.resume = goodix_ts_resume,
#endif
	.id_table = goodix_ts_id,
	.driver = {
		   .name = GOODIX_I2C_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __devinit goodix_ts_init(void)
{
	int ret;
	goodix_wq = create_singlethread_workqueue("goodix_wq");
	if (!goodix_wq) {
		pr_err("Creat %s workqueue failed.\n", imx_ts_name);
		return -ENOMEM;
	}
	ret = i2c_add_driver(&goodix_ts_driver);
	return ret;
}

static void __exit goodix_ts_exit(void)
{
	i2c_del_driver(&goodix_ts_driver);
	if (goodix_wq)
		destroy_workqueue(goodix_wq);
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("Goodix Touchscreen Driver");
MODULE_LICENSE("GPL v2");
