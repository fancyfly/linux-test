/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file pmic/core/mc13892.c
 * @brief This file contains MC13892 specific PMIC code. This implementaion
 * may differ for each PMIC chip.
 *
 * @ingroup PMIC_CORE
 */

/*
 * Includes
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>

#include <asm/uaccess.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_status.h>

#include "pmic.h"

/*
 * Defines
 */

int pmic_i2c_24bit_read(struct i2c_client *client, unsigned int reg_num)
{
	char buf[3];
	int ret;

	memset(buf, 0, 3);
	ret = i2c_smbus_read_i2c_block_data(client, reg_num, 3, buf);

	if (ret == 3) {
		ret = buf[0] << 16 | buf[1] << 8 | buf[2];
		return ret;
	} else {
		printk(KERN_ERR "24bit read error, ret = %d\n", ret);
		return -1;	/* return -1 on failure */
	}
}

int pmic_i2c_24bit_write(struct i2c_client *client,
			 unsigned int reg_num, unsigned int reg_val)
{
	char buf[3];

	buf[0] = (reg_val >> 16) & 0xff;
	buf[1] = (reg_val >> 8) & 0xff;
	buf[2] = (reg_val) & 0xff;

	return i2c_smbus_write_i2c_block_data(client, reg_num, 3, buf);
}

int pmic_read(int reg_num, unsigned int *reg_val)
{
	if (mc13892_client == NULL)
		return PMIC_ERROR;

	*reg_val = pmic_i2c_24bit_read(mc13892_client, reg_num);

	if (*reg_val == -1)
		return PMIC_ERROR;

	return PMIC_SUCCESS;
}

int pmic_write(int reg_num, const unsigned int reg_val)
{
	if (mc13892_client == NULL)
		return PMIC_ERROR;

	return pmic_i2c_24bit_write(mc13892_client, reg_num, reg_val);
}

int pmic_init_registers(void)
{
	return PMIC_SUCCESS;
}

unsigned int pmic_get_active_events(unsigned int *active_events)
{
	return 0;
}

int pmic_event_unmask(type_event event)
{
	return 0;
}

int pmic_event_mask(type_event event)
{
	return 0;
}
