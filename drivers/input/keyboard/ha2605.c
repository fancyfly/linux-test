/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fsl_devices.h>
#include <mach/i2c.h>
#include <mach/irqs.h>
#include <mach/iomux-mx53.h>
#include <mach/gpio.h>

#define HA2605_MAX_KEY_COUNT	0x05
#define HA2605_CONFIG_LEN	0x0c
#define KEY_VALID_CODE_MIN	0x01
#define KEY_VALID_CODE_MAX	0x05
#define KEY_INVALID_CODE	0xff

struct ha2605_priv {
	struct i2c_client *client;
	struct input_dev *input;
	char phys[32];
	unsigned int irq;
	struct work_struct work;
	int old_keycode;
	struct workqueue_struct *workqueue;
	int keycount;
	u16 keycodes[HA2605_MAX_KEY_COUNT];
};

/*
 *Function as i2c_master_receive, and return 2 if operation is successful.
 */
static int i2c_read_bytes(struct i2c_client *client, uint8_t * buf,
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

static bool ha2605_init_panel(struct ha2605_priv *ha2605)
{
	int ret = -1;
	int count;

	uint8_t config_info[HA2605_CONFIG_LEN] = {
		0x88, 0x5,
		0x90, 0x90, 0x90, 0x90, 0x90,
		0xf8, 0x0, 0x40, 0x0, 0x0d
	};

	for (count = 5; count > 0; count--) {
		ret =
		    i2c_write_bytes(ha2605->client, config_info,
				    HA2605_CONFIG_LEN);
		if (ret == 1)	/*Initiall success */
			break;
		else
			msleep(10);
	}

	return ret == 1 ? true : false;
}

static void ha2605_work(struct work_struct *work)
{
	struct ha2605_priv *ha2605 =
	    container_of(work, struct ha2605_priv, work);
	struct i2c_client *client = ha2605->client;
	struct input_dev *input = ha2605->input;
	uint8_t key_value[2];
	uint8_t key_val;
	int ret = -1;
	int *old_keycode = &ha2605->old_keycode;

	ret = i2c_read_bytes(client, key_value, 2);
	if (ret <= 0) {
		dev_dbg(&(client->dev),
			"I2C transfer error. ERROR Number:%d\n ", ret);
		return;
	}

	key_val = key_value[1];
	if (key_val == KEY_INVALID_CODE){
		input_report_key(input, *old_keycode, 0);
		input_sync(input);
	} else if (key_val >= KEY_VALID_CODE_MIN &&
			key_val <= KEY_VALID_CODE_MAX){
		#if defined(CONFIG_AT070TN93)
		*old_keycode = ha2605->keycodes[key_val - 2];
		#endif
		#if defined(CONFIG_AT070TN2_WSVGA)
		*old_keycode = ha2605->keycodes[key_val - 1];
		#endif
		input_report_key(input, *old_keycode, 1);
		input_sync(input);
	}
}

static irqreturn_t ha2605_irq(int irq, void *handle)
{
	struct ha2605_priv *ha2605 = handle;
	queue_work(ha2605->workqueue, &ha2605->work);
	return IRQ_HANDLED;
}

static int __devinit ha2605_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	int i;
	int ret;
	struct ha2605_priv *ha2605;
	struct input_dev *input_dev;
	struct ha2605_platform_data *pdata = client->dev.platform_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C don't support enough function");
		return -EIO;
	}

	ha2605 = kzalloc(sizeof(struct ha2605_priv), GFP_KERNEL);
	if (!ha2605)
		return -ENOMEM;

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		goto err_free_mem;
	}

	ha2605->client = client;
	ha2605->irq = client->irq;
	ha2605->input = input_dev;
	ha2605->keycount = pdata->keycount;

	if (ha2605->keycount > HA2605_MAX_KEY_COUNT) {
		dev_err(&client->dev, "Too many key defined\n");
		ret = -EINVAL;
		goto err_free_mem;
	}

	ha2605->workqueue = create_singlethread_workqueue("ha2605");
	INIT_WORK(&ha2605->work, ha2605_work);

	if (ha2605->workqueue == NULL) {
		dev_err(&client->dev, "couldn't create workqueue\n");
		ret = -ENOMEM;
		goto err_free_dev;
	}

	ret = ha2605_init_panel(ha2605);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to init register\n");
		goto err_free_dev;
	}

	snprintf(ha2605->phys, sizeof(ha2605->phys),
		 "%s/input3", dev_name(&client->dev));

	input_dev->name = "Goodix ha2605 Keypad";
	input_dev->phys = ha2605->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->keycode = pdata->matrix;
	input_dev->keycodesize = sizeof(pdata->matrix[0]);
	input_dev->keycodemax = ha2605->keycount;
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	for (i = 0; i < input_dev->keycodemax; i++) {
		__set_bit(pdata->matrix[i], input_dev->keybit);
		ha2605->keycodes[i] = pdata->matrix[i];
		input_set_capability(input_dev, EV_KEY, pdata->matrix[i]);
	}

	ret = input_register_device(input_dev);
	if (ret)
		goto err_free_wq;

	i2c_set_clientdata(client, ha2605);

	ret = request_irq(ha2605->irq, ha2605_irq,
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			  client->dev.driver->name, ha2605);
	if (ret < 0) {
		dev_err(&client->dev, "failed to register irq %d!\n",
			ha2605->irq);
		goto err_unreg_dev;
	}

	return 0;
err_unreg_dev:
	input_unregister_device(input_dev);
err_free_wq:
	destroy_workqueue(ha2605->workqueue);
err_free_dev:
	input_free_device(input_dev);
err_free_mem:
	kfree(ha2605);
	return ret;
}

static int __devexit ha2605_remove(struct i2c_client *client)
{
	struct ha2605_priv *ha2605 = i2c_get_clientdata(client);
	free_irq(ha2605->irq, ha2605);
	cancel_work_sync(&ha2605->work);
	destroy_workqueue(ha2605->workqueue);
	input_unregister_device(ha2605->input);
	input_free_device(ha2605->input);
	kfree(ha2605);

	return 0;
}

static const struct i2c_device_id ha2605_idtable[] = {
	{"ha2605_touchkey", 0},
	{}
};

static struct i2c_driver ha2605_driver = {

	.driver = {
		   .owner = THIS_MODULE,
		   .name = "ha2605_touchkey",
		   },
	.id_table = ha2605_idtable,
	.probe = ha2605_probe,
	.remove = __devexit_p(ha2605_remove),
};

static int __init ha2605_init(void)
{
	return i2c_add_driver(&ha2605_driver);
}

static void __exit ha2605_exit(void)
{
	i2c_del_driver(&ha2605_driver);
}

module_init(ha2605_init);
module_exit(ha2605_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Goodix ha2605 touch key driver");
MODULE_LICENSE("GPL v2");
