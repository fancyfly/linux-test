/*
 * include/linux/goodix_touch.h
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
 */

#ifndef _LINUX_GOODIX_TOUCH_H
#define	_LINUX_GOODIX_TOUCH_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

#define GOODIX_I2C_NAME "Goodix-TS"
#define GUITAR_GT80X

#define TOUCH_MAX_HEIGHT 	7680
#define TOUCH_MAX_WIDTH	 	5120

#define SCREEN_MAX_HEIGHT	480
#define SCREEN_MAX_WIDTH	272

#define GT80X_ADD            0xaa
#define MX53_PCBA_TOUCH_RST		(6*32 + 3)	/* GPIO7_3 */


#define GOODIX_MULTI_TOUCH
#ifndef GOODIX_MULTI_TOUCH
#define MAX_FINGER_NUM 1
#else
#define MAX_FINGER_NUM 5
#endif
#if defined(INT_PORT)
#if MAX_FINGER_NUM <= 3
#define READ_BYTES_NUM (1 + 2 + MAX_FINGER_NUM * 5)
#elif MAX_FINGER_NUM == 4
#define READ_BYTES_NUM (1 + 28)
#elif MAX_FINGER_NUM == 5
#define READ_BYTES_NUM (1 + 34)
#endif
#else
#define READ_BYTES_NUM (1 + 34)
#endif

enum finger_state {
#define FLAG_MASK 0x01
	FLAG_UP = 0,
	FLAG_DOWN = 1,
	FLAG_INVALID = 2,
};

struct point_node {
	uint8_t id;
	enum finger_state state;
	uint8_t pressure;
	unsigned int x;
	unsigned int y;
};

struct goodix_ts_data {
	int retry;
	int panel_type;
	char phys[32];
	struct i2c_client *client;
	struct input_dev *input_dev;
	uint8_t use_irq;
	uint8_t use_shutdown;
	uint32_t gpio_shutdown;
	uint32_t gpio_irq;
	uint32_t screen_width;
	uint32_t screen_height;
	struct hrtimer timer;
	struct work_struct work;
	struct early_suspend early_suspend;
	int (*power) (struct goodix_ts_data *ts, int on);
};

/* Notice: This definition used by platform_data.
 * It should be move this struct info to platform head file such as plat/ts.h.
 * If not used in client, it will be NULL in function of goodix_ts_probe.
 */
struct goodix_i2c_platform_data {
	uint32_t gpio_irq;	/*IRQ port, use macro such as "gpio_to_irq" to
				   get Interrupt Number. */
	uint32_t irq_cfg;	/*IRQ port config, must refer to master's
				   Datasheet. */
	uint32_t gpio_shutdown;	/*Shutdown port number */
	uint32_t shutdown_cfg;	/*Shutdown port config */
	uint32_t screen_width;	/*screen width */
	uint32_t screen_height;	/*screen height */
};

#endif				/* _LINUX_GOODIX_TOUCH_H */
