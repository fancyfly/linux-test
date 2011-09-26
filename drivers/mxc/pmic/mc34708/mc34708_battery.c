/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
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
 */

/*
 * Includes
 */
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/mach-types.h>
#include <linux/mfd/mc34708/mc34708_battery.h>
#include <linux/mfd/mc34708/mc34708_adc.h>
#include <linux/pmic_status.h>
#include <linux/pmic_external.h>
#include <linux/mfd/mc34708/mc34708.h>

#define CHRCV_LSH 6
#define CHRCV_WID 6

#define CHRCC_LSH 12
#define CHRCC_WID 4

#define CHRITERM_LSH 16
#define CHRITERM_WID 3

#define USBDETS_LSH 3
#define USBDETS_WID 1

#define AUXDETS_LSH 4
#define AUXDETS_WID 1

#define VBAT_TRKL_LSH 0
#define VBAT_TRKL_WID 2

#define LOWBATT_LSH 4
#define LOWBATT_WID 2

#define BATTEMPH_LSH 21
#define BATTEMPH_WID 2

#define EOCBUCKEN_LSH 23
#define EOCBUCKEN_WID 1

#define BATTEMPL_LSH 19
#define BATTEMPL_WID 2

#define CHRITERMEN_LSH 2
#define CHRITERMEN_WID 1

#define CHREN_LSH 3
#define CHREN_WID 1

#define CCRES_LSH	22
#define CCRES_WID	2

#define INTEGTIME_LSH	5
#define INTEGTIME_WID	2

#define BATTCURRENT_LSH	0
#define BATTCURRENT_WID	12

#define MUSBCHRG_LSH 13
#define MUSBCHRG_WID 2

#define MSW_LSH 1
#define MSW_WID 1

#define AUXWEAKEN_LSH 22
#define AUXWEAKEN_WID 1

#define VBUSWEAKEN_LSH 23
#define VBUSWEAKEN_WID 1

#define AUXILIM_LSH 11
#define AUXILIM_WID 3

#define ILIM1P5_LSH 20
#define ILIM1P5_WID 1

#define ACC_STARTCC_LSH		0
#define ACC_STARTCC_WID		1
#define ACC_RSTCC_LSH		1
#define ACC_RSTCC_WID		1
#define ACC_CCFAULT_LSH		7
#define ACC_CCFAULT_WID		7
#define ACC_CCOUT_LSH		8
#define ACC_CCOUT_WID		16
#define ACC1_ONEC_LSH		0
#define ACC1_ONEC_WID		15

#define ACC_CALIBRATION 0x17
#define ACC_START_COUNTER 0x07
#define ACC_STOP_COUNTER 0x2
#define ACC_CONTROL_BIT_MASK 0x1f
#define ACC_ONEC_VALUE 273
#define ACC_COULOMB_PER_LSB 1
#define ACC_CALIBRATION_DURATION_MSECS 20

#define BAT_VOLTAGE_UNIT_UV 4692
#define BAT_CURRENT_UNIT_UA 7813
#define CHG_VOLTAGE_UINT_UV 23474
#define CHG_MIN_CURRENT_UA 3500

#define COULOMB_TO_UAH(c) (10000 * c / 36)

#define BATTOVP_LSH 9
#define BATTOVP_WID 1

#define USBDETM_LSH 3
#define USBDETM_WID 1
#define AUXDETM_LSH 4
#define AUXDETM_WID 1
#define ATTACHM_LSH 15
#define ATTACHM_WID 1
#define DETACHM_LSH 16
#define DETACHM_WID 1
#define ADCCHANGEM_LSH 21
#define ADCCHANGEM_WID 1

#define BATTISOEN_LSH 23
#define BATTISOEN_WID 1

#define USBHOST 0x4
#define USBCHARGER 0x20
#define DEDICATEDCHARGER 0x40

#define EOC_CURRENT_UA		100000
#define LOW_VOLT_THRESHOLD	3300000
#define HIGH_VOLT_THRESHOLD	4200000

#define ADC_MAX_CHANNEL		8

static int suspend_flag;
static int charging_flag;
static int power_change_flag;

static int last_batt_complete_id = -1;
bool can_calculate_capacity = false;

static int before_use_calculated_capacity = 1;

#define CC_RECORD_STEP		100000
#define CC_RECORD_DEVIATION	6000
#define CC_RECORD_VOLT_IDX(x)	(((x) - LOW_VOLT_THRESHOLD) / CC_RECORD_STEP)
#define CC_RECORD_VOLT_NUMBERS	(CC_RECORD_VOLT_IDX(HIGH_VOLT_THRESHOLD) + 1)
#define CC_RECORD_INVALID_VAL	(-100000)
static int charging_cc_record [CC_RECORD_VOLT_NUMBERS];
static int discharging_cc_record [CC_RECORD_VOLT_NUMBERS];
enum batt_complete_id {
	BATT_FULL = 1,
	BATT_EMPTY,
};

/*
usb_type = 0x4; USB host;
usb_type = 0x20; USB charger;
usb_type = 0x40; Dedicated charger;
*/
static int usb_type;
static struct mc34708_charger_setting_point ripley_charger_setting_point[] = {
	{
	 .microVolt = 4200000,
	 .microAmp = 1150000,
	 },
};

static struct mc34708_charger_config ripley_charge_config = {
	.batteryTempLow = 0,
	.batteryTempHigh = 0,
	.hasTempSensor = 0,
	.trickleThreshold = 3000000,
	.vbusThresholdLow = 4600000,
	.vbusThresholdWeak = 4800000,
	.vbusThresholdHigh = 5000000,
	.vauxThresholdLow = 4600000,
	.vauxThresholdWeak = 4800000,
	.vauxThresholdHigh = 5000000,
	.lowBattThreshold = 3000000,
	.toppingOffMicroAmp = 50000,	/* 50mA */
	.chargingPoints = ripley_charger_setting_point,
	.pointsNumber = 1,
};

enum ccres {
	CCRES_100_mC_PER_LSB = 0,
	CCRES_200_mC_PER_LSB,
	CCRES_500_mC_PER_LSB,
	CCRES_1000_mC_PER_LSB,
	CCRES_INVALID,
};

enum integtime {
	INTEGTIME_4S = 0,
	INTEGTIME_8S,
	INTEGTIME_16S,
	INTEGTIME_32S,
};

#ifdef DEBUG
static int dump_register(int reg)
{
	int ret;
	int val;

	ret = pmic_read_reg(reg, &val, PMIC_ALL_BITS);
	if (ret == 0) {
		printk("reg %d:  %x\n", reg, val);
	} else
		printk("reg read error\n");
}
#endif

static int set_coulomb_counter_cres(enum ccres ccres)
{
	CHECK_ERROR(pmic_write_reg(MC34708_REG_ACC1,
				   BITFVAL(CCRES, ccres),
				   BITFMASK(CCRES)));

	return 0;
}

static int set_coulomb_counter_integtime(enum integtime integtime)
{
	CHECK_ERROR(pmic_write_reg(MC34708_REG_ACC0,
				   BITFVAL(INTEGTIME, integtime),
				   BITFMASK(INTEGTIME)));

	return 0;
}

/* uA */
static int get_columb_counter_battcur_res_ua_lsb()
{
	int ret;
	unsigned int ccres;
	unsigned int integtime;

	ret = pmic_read_reg(MC34708_REG_ACC1, &ccres, BITFMASK(CCRES));
	if (ret != 0)
		return -1;
	ccres = BITFEXT(ccres, CCRES);
	pr_debug("%s: ccres = %x\n", __func__, ccres);

	ret = pmic_read_reg(MC34708_REG_ACC0, &integtime, BITFMASK(INTEGTIME));
	if (ret != 0)
		return -1;
	integtime = BITFEXT(integtime, INTEGTIME);
	pr_debug("%s: integtime = %x\n", __func__, integtime);

	if (ccres < CCRES_100_mC_PER_LSB || ccres >= CCRES_INVALID)
		return -1;

	/* See table: BATTCURRENT Rsolution in RM */
	return 100000 * (2 << ccres) / ((2 << integtime) * 4);
}

static int enable_charger(int enable)
{
	charging_flag = enable ? 1 : 0;
	CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
				   BITFVAL(CHREN, enable ? 1 : 0),
				   BITFMASK(CHREN)));
	return 0;
}

static int ripley_get_batt_voltage(unsigned short *voltage)
{
	int channel[ADC_MAX_CHANNEL];
	unsigned short result[ADC_MAX_CHANNEL];
	unsigned short max, min;
	int i;

	for (i = 0; i < ADC_MAX_CHANNEL; i++)
		channel[i] = BATTERY_VOLTAGE;

	CHECK_ERROR(mc34708_pmic_adc_convert(channel, result,
						ADC_MAX_CHANNEL));

	*voltage = 0;
	min = max = result[0];
	for (i = 0; i < ADC_MAX_CHANNEL; i++) {
		if (min > result[i])
			min = result[i];
		if (max < result[i])
			max = result[i];
		*voltage += result[i];
	}
	/* abandon max/min, then get average */
	*voltage -= (max + min);
	*voltage /= (ADC_MAX_CHANNEL - 2);

	return 0;
}

static int ripley_get_batt_current(unsigned short *curr)
{
	int channel[ADC_MAX_CHANNEL];
	unsigned short result[ADC_MAX_CHANNEL];
	unsigned short max, min;
	int i;

	for (i = 0; i < ADC_MAX_CHANNEL; i++)
		channel[i] = BATTERY_CURRENT;

	CHECK_ERROR(mc34708_pmic_adc_convert(channel, result,
						ADC_MAX_CHANNEL));

	*curr = 0;
	min = max = result[0];
	for (i = 0; i < ADC_MAX_CHANNEL; i++) {
		if (min > result[i])
			min = result[i];
		if (max < result[i])
			max = result[i];
		*curr += result[i];
	}
	/* abandon max/min, then get average */
	*curr -= (max + min);
	*curr /= (ADC_MAX_CHANNEL - 2);

	return 0;
}

static int ripley_get_batt_volt_curr(unsigned short *volt, unsigned short *curr)
{
	int channel[ADC_MAX_CHANNEL];
	unsigned short result[ADC_MAX_CHANNEL];
	unsigned short max, min;
	int i;

	for (i = 0; i < ADC_MAX_CHANNEL; i++) {
		if (i % 2)
			channel[i] = BATTERY_CURRENT;
		else
			channel[i] = BATTERY_VOLTAGE;
	}

	CHECK_ERROR(mc34708_pmic_adc_convert(channel, result,
						ADC_MAX_CHANNEL));

	*volt = 0;
	*curr = 0;

	min = max = result[0];
	for (i = 0; i < ADC_MAX_CHANNEL; i += 2) {
		if (min > result[i])
			min = result[i];
		if (max < result[i])
			max = result[i];
		*volt += result[i];
	}
	/* abandon max/min, then get average */
	*volt -= (max + min);
	*volt /= ((ADC_MAX_CHANNEL / 2) - 2);

	min = max = result[1];
	for (i = 1; i < ADC_MAX_CHANNEL; i += 2) {
		if (min > result[i])
			min = result[i];
		if (max < result[i])
			max = result[i];
		*curr += result[i];
	}
	/* abandon max/min, then get average */
	*curr -= (max + min);
	*curr /= ((ADC_MAX_CHANNEL / 2) - 2);

	return 0;
}


static int coulomb_counter_calibration;
static unsigned int coulomb_counter_start_time_msecs;

static int ripley_calibrate_coulomb_counter(void)
{
	int ret;
	unsigned int value;

	/* set scaler */
	CHECK_ERROR(pmic_write_reg(REG_ACC1, 0x19e, BITFMASK(ACC1_ONEC)));

	CHECK_ERROR(pmic_write_reg
		    (REG_ACC0, ACC_CALIBRATION, ACC_CONTROL_BIT_MASK));
	msleep(ACC_CALIBRATION_DURATION_MSECS);

	ret = pmic_read_reg(REG_ACC0, &value, BITFMASK(ACC_CCOUT));
	if (ret != 0)
		return -1;
	value = BITFEXT(value, ACC_CCOUT);
	pr_debug("calibrate value = %x\n", value);
	coulomb_counter_calibration = (int)((s16) ((u16) value));
	pr_debug("coulomb_counter_calibration = %d\n",
		 coulomb_counter_calibration);
	CHECK_ERROR(pmic_write_reg
		    (REG_ACC0, 0x7, ACC_CONTROL_BIT_MASK));

	return 0;

}

static int ripley_get_charger_coulomb(int *coulomb)
{
	int ret;
	unsigned int value;

	ret = pmic_read_reg(MC34708_REG_ACC0, &value, PMIC_ALL_BITS);
	if (ret != 0)
		return -1;
#define CCFAULT	0x80
	if (value & CCFAULT) {
		pr_warning("CCFAULT: CCOUT contents no longer valid!\n");
		return -1;
	}
	value = BITFEXT(value, ACC_CCOUT);
	pr_debug("counter value = %x\n", value);

	/* 2's complement */
	if (value & 0x8000) {
		value = ~value;
		value += 1;
		value &= 0x7fff; 
		value *= -1;
	}
	pr_debug("cc %d\n", value);
	*coulomb = value; // TODO

	return 0;
}

struct ripley_dev_info {
	struct device *dev;

	unsigned short voltage_raw;
	int voltage_uV;
	unsigned short current_raw;
	int current_uA;
	int old_battery_status;
	int battery_status;
	int full_counter;
	int chargeTimeSeconds;
	int usb_charger_online;
	int aux_charger_online;
	int charger_voltage_uV;
	int accum_current_uAh;
	int percent;
	int real_capacity;
	int full_coulomb;
	int empty_coulomb;
	int now_coulomb;
	int delta_coulomb;

	int currChargePoint;

	struct power_supply bat;
	struct power_supply usb_charger;
	struct power_supply aux_charger;

	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
	struct delayed_work ovp_mon_work;
	struct mc34708_charger_config *chargeConfig;
};

#define ripley_SENSER	25
#define to_ripley_dev_info(x) container_of((x), struct ripley_dev_info, \
					      bat);

static enum power_supply_property ripley_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property ripley_aux_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property ripley_usb_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

#define BATTTEMPH_TO_BITS(temp)	((temp - 45) / 5)
#define BATTTEMPL_TO_BITS(temp)	(temp / 5)
#define VBAT_TRKL_UV_TO_BITS(uv)	((uv-2800000) / 100000)
#define LOWBATT_UV_TO_BITS(uv)	((uv - 3000000) / 100000)
#define CHRITEM_UV_TO_BITS(uv)	(((uv / 1000) - 50) / 50)

static int init_battery_profile(struct mc34708_charger_config *config)
{
	/* set charger current termination threshold */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
				   BITFVAL(CHRITERM,
					   CHRITEM_UV_TO_BITS
					   (config->toppingOffMicroAmp)),
				   BITFMASK(CHRITERM)));
	/* enable charger current termination */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
				   BITFVAL(CHRITERMEN, 1),
				   BITFMASK(CHRITERMEN)));

	/* enable EOC buck */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
				   BITFVAL(EOCBUCKEN, 1), BITFMASK(EOCBUCKEN)));

	CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
				   BITFVAL(VBAT_TRKL,
					   VBAT_TRKL_UV_TO_BITS
					   (config->trickleThreshold)),
				   BITFMASK(VBAT_TRKL)));

	CHECK_ERROR(pmic_write_reg
		    (MC34708_REG_BATTERY_PROFILE,
		     BITFVAL(LOWBATT,
			     LOWBATT_UV_TO_BITS(config->lowBattThreshold)),
		     BITFMASK(LOWBATT)));

	if (config->hasTempSensor) {
		CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
					   BITFVAL(BATTEMPH,
						   BATTTEMPH_TO_BITS
						   (config->batteryTempHigh)),
					   BITFMASK(BATTEMPH)));
		CHECK_ERROR(pmic_write_reg
			    (MC34708_REG_BATTERY_PROFILE,
			     BITFVAL(BATTEMPL,
				     BATTTEMPL_TO_BITS(config->batteryTempLow)),
			     BITFMASK(BATTEMPL)));
	}


	return 0;
}

static int init_charger(struct mc34708_charger_config *config)
{
	init_battery_profile(config);

	/* disable manual switch */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_USB_CTL,
				   BITFVAL(MSW, 0), BITFMASK(MSW)));
	/* enable 1P5 large current */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_DEBOUNCE,
				   BITFVAL(ILIM1P5, 1), BITFMASK(ILIM1P5)));

	/* enable ISO */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_DEBOUNCE,
				   BITFVAL(BATTISOEN, 1), BITFMASK(BATTISOEN)));

	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_SOURCE,
				   BITFVAL(AUXWEAKEN, 1), BITFMASK(AUXWEAKEN)));

	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_SOURCE,
				   BITFVAL(VBUSWEAKEN, 1),
				   BITFMASK(VBUSWEAKEN)));

	return 0;
}

#define CHRCV_UV_TO_BITS(uv)	((uv - 3500000) / 20000)
#define CHRCC_UA_TO_BITS(ua)	((ua - 250000) / 100000)

static int set_charging_point(struct ripley_dev_info *di, int point)
{
	unsigned int val, mask;
	if (point >= 0 && point < di->chargeConfig->pointsNumber) {

		val =
		    BITFVAL(CHRCV,
			    CHRCV_UV_TO_BITS(di->
					     chargeConfig->chargingPoints
					     [point].microVolt));
		switch (usb_type) {
		case USBHOST:
			/* set current limit to 500mA */
			CHECK_ERROR(pmic_write_reg(MC34708_REG_USB_CTL,
						   BITFVAL(MUSBCHRG, 1),
						   BITFMASK(MUSBCHRG)));
			val |= BITFVAL(CHRCC, CHRCC_UA_TO_BITS(250000));
			break;
		case USBCHARGER:
			/* set current limit to 950mA */
			CHECK_ERROR(pmic_write_reg(MC34708_REG_USB_CTL,
						   BITFVAL(MUSBCHRG, 3),
						   BITFMASK(MUSBCHRG)));
			val |= BITFVAL(CHRCC, CHRCC_UA_TO_BITS(350000));
			break;
		case DEDICATEDCHARGER:
			/* set current limit to 950mA */
			CHECK_ERROR(pmic_write_reg(MC34708_REG_USB_CTL,
						   BITFVAL(MUSBCHRG, 3),
						   BITFMASK(MUSBCHRG)));
			val |= BITFVAL(CHRCC, CHRCC_UA_TO_BITS(350000));
			break;
		default:
			val |=
			    BITFVAL(CHRCC,
				    CHRCC_UA_TO_BITS(di->
						     chargeConfig->chargingPoints
						     [point].microAmp));
			break;
		}

		mask = BITFMASK(CHRCV) | BITFMASK(CHRCC);
		CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
					   val, mask));

		di->currChargePoint = point;
		return 0;
	}
	return -EINVAL;
}

static int check_eoc(struct ripley_dev_info *di)
{
	int ret;
	int battcur_res, battcur;

	battcur_res = get_columb_counter_battcur_res_ua_lsb();
	pr_debug("battcur_res = %x\n", battcur_res);
	if (battcur_res < 0)
		return -1;

	ret = pmic_read_reg(MC34708_REG_ACC2, &battcur, BITFMASK(BATTCURRENT));
	if (ret != 0)
		return -1;
	battcur = BITFEXT(battcur, BATTCURRENT);
	pr_debug("batt cur reg value = %x\n", battcur);

	battcur *= battcur_res;
	pr_debug("batt cur value = %d\n", battcur/1000);

	ret = ripley_get_batt_voltage(&(di->voltage_raw));
	if (ret == 0)
		di->voltage_uV = di->voltage_raw * BAT_VOLTAGE_UNIT_UV;
	if (battcur <= EOC_CURRENT_UA && di->voltage_uV >= HIGH_VOLT_THRESHOLD)
		di->full_counter++;

	if (di->full_counter > 2)
		di->battery_status = POWER_SUPPLY_STATUS_FULL;
	else
		di->battery_status = POWER_SUPPLY_STATUS_CHARGING;

	return 0;
}

static int ripley_charger_update_status(struct ripley_dev_info *di)
{
	int ret;
	unsigned int value;
	unsigned int reg_usb_type;
	int usbOnline, auxOnline;
	int restartCharging = 0;
	int stopCharging = 0;

	ret = pmic_read_reg(MC34708_REG_INT_SENSE0, &value, PMIC_ALL_BITS);

	if (ret == 0) {
		usbOnline = BITFEXT(value, USBDETS);
		auxOnline = BITFEXT(value, AUXDETS);
		if (!(di->aux_charger_online || di->usb_charger_online) &&
		    (usbOnline || auxOnline))
			restartCharging = 1;
		if ((di->aux_charger_online || di->usb_charger_online) &&
		    !(usbOnline || auxOnline))
			stopCharging = 1;

		if (auxOnline != di->aux_charger_online) {
			msleep(500);
			di->aux_charger_online = auxOnline;
			dev_info(di->aux_charger.dev,
				 "aux charger status: %s\n",
				 auxOnline ? "online" : "offline");
			power_supply_changed(&di->aux_charger);
		}
		if (usbOnline != di->usb_charger_online) {
			/* need some delay to know the usb type */
			msleep(800);
			ret = pmic_read_reg(MC34708_REG_USB_DEVICE_TYPE,
					    &reg_usb_type, PMIC_ALL_BITS);
			usb_type = 0;
			if ((reg_usb_type & USBHOST) != 0) {
				usb_type = USBHOST;
				pr_info("USB host attached!!!\n");
			}
			if ((reg_usb_type & USBCHARGER) != 0) {
				usb_type = USBCHARGER;
				pr_info("USB charger attached!!!\n");
			}
			if ((reg_usb_type & DEDICATEDCHARGER) != 0) {
				usb_type = DEDICATEDCHARGER;
				pr_info("Dedicated charger attached!!!\n");
			}
			di->usb_charger_online = usbOnline;
			dev_info(di->usb_charger.dev, "usb cable status: %s\n",
				 usbOnline ? "online" : "offline");
			power_supply_changed(&di->usb_charger);
		}

		if (restartCharging) {
			enable_charger(1);
			cancel_delayed_work(&di->monitor_work);
			queue_delayed_work(di->monitor_wqueue,
					   &di->monitor_work, HZ / 10);
		} else if (stopCharging) {
			cancel_delayed_work(&di->monitor_work);
			queue_delayed_work(di->monitor_wqueue,
					   &di->monitor_work, HZ / 10);
		}
	}
	power_change_flag = 1;

	return ret;
}

static int ripley_aux_charger_get_property(struct power_supply *psy,
					   enum power_supply_property psp,
					   union power_supply_propval *val)
{
	struct ripley_dev_info *di =
	    container_of((psy), struct ripley_dev_info, aux_charger);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->aux_charger_online;
		return 0;
	default:
		break;
	}

	return -EINVAL;
}

static int ripley_usb_charger_get_property(struct power_supply *psy,
					   enum power_supply_property psp,
					   union power_supply_propval *val)
{
	struct ripley_dev_info *di =
	    container_of((psy), struct ripley_dev_info, usb_charger);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->usb_charger_online;
		return 0;
	default:
		break;
	}

	return -EINVAL;
}

static int ripley_battery_read_status(struct ripley_dev_info *di)
{
	int retval;
	int coulomb;
#if 0
	retval = ripley_get_batt_voltage(&(di->voltage_raw));
	if (retval == 0)
		di->voltage_uV = di->voltage_raw * BAT_VOLTAGE_UNIT_UV;

	retval = ripley_get_batt_current(&(di->current_raw));
	if (retval == 0) {
		if (di->current_raw & 0x200)
			di->current_uA =
			    (0x1FF - (di->current_raw & 0x1FF)) *
			    BAT_CURRENT_UNIT_UA * (-1);
		else
			di->current_uA =
			    (di->current_raw & 0x1FF) * BAT_CURRENT_UNIT_UA;
	}
#else
	/* AMPD suggest sample volt/curr alternately */
	retval = ripley_get_batt_volt_curr(&(di->voltage_raw),
					&(di->current_raw));
	if (retval == 0) {
		di->voltage_uV = di->voltage_raw * BAT_VOLTAGE_UNIT_UV;

		if (di->current_raw & 0x200)
			di->current_uA =
			    (0x1FF - (di->current_raw & 0x1FF)) *
			    BAT_CURRENT_UNIT_UA * (-1);
		else
			di->current_uA =
			    (di->current_raw & 0x1FF) * BAT_CURRENT_UNIT_UA;
	}
#endif
	retval = ripley_get_charger_coulomb(&coulomb);
	if (retval == 0) {
		di->delta_coulomb = coulomb - di->now_coulomb; /*TODO:check*/
		di->now_coulomb = coulomb;
//		di->accum_current_uAh = coulomb * 1000000 / 3600;
	}
	pr_debug("vol %d, cur %d, cc %d, delta cc %d\n", di->voltage_uV,
							di->current_uA,
							di->now_coulomb,
							di->delta_coulomb);
	return retval;
}

static void _gather_capacity_change_statistics(struct ripley_dev_info *di)
{
	int i, idx;
	if (di->voltage_uV < LOW_VOLT_THRESHOLD) //||
//	    di->voltage_uV > (HIGH_VOLT_THRESHOLD + CC_RECORD_DEVIATION))
		return;

	for (i = LOW_VOLT_THRESHOLD; i <= HIGH_VOLT_THRESHOLD;
		i += CC_RECORD_STEP) {
		if (abs(di->voltage_uV - i) < CC_RECORD_DEVIATION) {
			idx = CC_RECORD_VOLT_IDX(di->voltage_uV);
			if (di->battery_status ==
				POWER_SUPPLY_STATUS_CHARGING) {
				charging_cc_record[idx] = di->now_coulomb;
				pr_info("chr: idx %d cc %d\n", idx, charging_cc_record[idx]);
				if ((idx + 1) < CC_RECORD_VOLT_NUMBERS)
					charging_cc_record[idx + 1] =
						CC_RECORD_INVALID_VAL;
				if (idx > 1 && di->old_battery_status ==
					POWER_SUPPLY_STATUS_DISCHARGING) {
					charging_cc_record[idx - 1] =
						CC_RECORD_INVALID_VAL;
				}
			} else if (di->battery_status ==
				POWER_SUPPLY_STATUS_DISCHARGING) {
				discharging_cc_record[idx] = di->now_coulomb;
				pr_info("dischr: idx %d cc %d\n", idx, charging_cc_record[idx]);
				if ((idx + 1) < CC_RECORD_VOLT_NUMBERS &&
					di->old_battery_status ==
						POWER_SUPPLY_STATUS_CHARGING)
					discharging_cc_record[idx + 1] =
						CC_RECORD_INVALID_VAL;
				if (idx > 1) {
					discharging_cc_record[idx - 1] =
						CC_RECORD_INVALID_VAL;
				}
			}
		}
	}

#if 0
	pr_info ("chr: ");
	for (i = 0; i <= CC_RECORD_VOLT_NUMBERS;
		i ++) {
		pr_info(" %d", charging_cc_record[i]);
	}
	pr_info ("\ndischr: ");
	for (i = 0; i <= CC_RECORD_VOLT_NUMBERS;
		i ++) {
		pr_info(" %d", discharging_cc_record[i]);
	}
	pr_info ("\n");
#endif
}

static void _get_delta_cc_and_percent(struct ripley_dev_info *di,
					int *delta_cc, int *percent)
{
	int i, j;
	int min = 0, max = 0;

	if (di->battery_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		max = CC_RECORD_VOLT_IDX(di->voltage_uV);
		max = (max + 4) < CC_RECORD_VOLT_NUMBERS ?
				(max + 4) : (CC_RECORD_VOLT_NUMBERS - 1);

		for (i = max;
			i >= 0;
			i -= CC_RECORD_STEP) {

			if (charging_cc_record[i] != CC_RECORD_INVALID_VAL) {
				max = i;
				for (j = (i - 1); j > 0;
					j -= CC_RECORD_STEP) {
					if (charging_cc_record[j] !=
						CC_RECORD_INVALID_VAL) {
						min = j;
					} else
						break;
				}

				break;
			}

		}

		if (min == max) { /* Not found */
			*delta_cc = 0;
			*percent = 0;
		} else {
			*delta_cc = abs(discharging_cc_record[max] -
				charging_cc_record[min]);
			*percent = (max - min ) * CC_RECORD_STEP * 100 /
				(HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);
		}
	} else if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING) {
		min = CC_RECORD_VOLT_IDX(di->voltage_uV);
		min = (min - 4) > 0 ? (min - 4) : 0;

		for (i = min;
			i < CC_RECORD_VOLT_NUMBERS;
			i += CC_RECORD_STEP) {

			if (charging_cc_record[i] != CC_RECORD_INVALID_VAL) {
				min = i;
				for (j = (i + 1); j < CC_RECORD_VOLT_NUMBERS;
					j += CC_RECORD_STEP) {
					if (charging_cc_record[j] !=
						CC_RECORD_INVALID_VAL) {
						max = j;
					} else
						break;
				}

				break;
			}

		}

		if (min == max) { /* Not found */
			*delta_cc = 0;
			*percent = 0;
		} else {
			*delta_cc = abs(charging_cc_record[max] -
				charging_cc_record[min]);
			*percent = (max - min ) * CC_RECORD_STEP * 100 /
				(HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);
		}
	}
}

static void ripley_battery_update_capacity(struct ripley_dev_info *di)
{
	/* Gather the charging statistics */
	_gather_capacity_change_statistics(di);

	if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN) {
		di->percent = (di->voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
			 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);

		if (di->percent < 0)
			di->percent = 0;
		else if (di->percent > 99)
			di->percent = 99;

	} else if (di->battery_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		if (di->voltage_uV <= LOW_VOLT_THRESHOLD) {/* discharging cmplt */
			if (last_batt_complete_id > 0 &&
				last_batt_complete_id != BATT_EMPTY) {
				can_calculate_capacity = true;
				before_use_calculated_capacity = 0;
			} else
				can_calculate_capacity = false;

			di->empty_coulomb = di->now_coulomb;
			pr_info("DIS CMPL: di->empty_coulomb %d\n",
					di->empty_coulomb);
			last_batt_complete_id = BATT_EMPTY;	
		}

		if (before_use_calculated_capacity == 1) {
			/*
			 * Here to make it more accurate if base on
			 * the statistics of charging gathered before
			 */
			if (di->old_battery_status ==
				POWER_SUPPLY_STATUS_CHARGING) {
				int delta_cc, percent;
				_get_delta_cc_and_percent(di, &delta_cc,
							&percent);
				if (delta_cc != 0 && percent != 0) {
					pr_info("DIS delta_cc %d, percent %d\n",
						delta_cc, percent);
					di->percent -= di->delta_coulomb *
							percent / delta_cc;
					goto out1;
				} else
					goto out2;
			}

out2:
			di->percent = (di->voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
				 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);

			if (di->percent < 0)
				di->percent = 0;
			else if (di->percent > 99)
				di->percent = 99;

			/* re-consider the case */
			if (di->voltage_uV <= LOW_VOLT_THRESHOLD) /* discharging cmplt */
				di->percent = 0; /* TODO */

			goto out1;
		}

		if (can_calculate_capacity) {
			/* recal capacity and get the real total capacity */
			di->real_capacity = di->full_coulomb - di->empty_coulomb;
		} else {
			di->percent -= di->delta_coulomb * 100 / di->real_capacity;
			di->percent = di->percent < 0 ? 0 : di->percent;
		}

		/* re-consider the case */
		if (di->voltage_uV <= LOW_VOLT_THRESHOLD) /* discharging cmplt */
			di->percent = 0;
	} else if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING || 
		   di->battery_status == POWER_SUPPLY_STATUS_FULL) {

		if (di->battery_status == POWER_SUPPLY_STATUS_FULL) {
			if (last_batt_complete_id > 0 &&
				last_batt_complete_id != BATT_FULL) {
				can_calculate_capacity = true;
				before_use_calculated_capacity = 0;
			} else
				can_calculate_capacity = false;

			di->full_coulomb = di->now_coulomb;
			pr_info("CHR CMPL: di->full_coulomb %d\n",
					di->full_coulomb);
			last_batt_complete_id = BATT_FULL;	
		}

		if (before_use_calculated_capacity == 1) {
			/*
			 * Here to make it more accurate if base on
			 * the statistics of charging gathered before
			 */
			if (di->old_battery_status ==
				POWER_SUPPLY_STATUS_DISCHARGING) {
				int delta_cc, percent;
				_get_delta_cc_and_percent(di, &delta_cc,
							&percent);
				if (delta_cc != 0 && percent != 0) {
					pr_info("CHR delta_cc %d, percent %d\n",
						delta_cc, percent);
					di->percent += di->delta_coulomb *
							percent / delta_cc;
					goto out1;
				} else
					goto out3;
			}

out3:
			di->percent = (di->voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
				 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);

			if (di->percent < 0)
				di->percent = 0;
			else if (di->percent > 99)
				di->percent = 99;

			if (di->battery_status == POWER_SUPPLY_STATUS_FULL)
				di->percent = 100;

			goto out1;
		}

		if (can_calculate_capacity) {
			/* recal capacity and get the real total capacity */
			di->real_capacity = di->full_coulomb - di->empty_coulomb;
		} else {
			di->percent += di->delta_coulomb * 100 / di->real_capacity;
			di->percent = di->percent > 99 ? 99 : di->percent;
		}

		if (di->battery_status == POWER_SUPPLY_STATUS_FULL) {
			di->percent = 100;
		}
	}
out1:
	pr_debug("di->percent %d ...\n", di->percent);
	return;
}
static void ripley_battery_update_status(struct ripley_dev_info *di)
{
	unsigned int point = 0;
	struct mc34708_charger_config *config = di->chargeConfig;
	int old_battery_status = di->battery_status;
	di->old_battery_status = di->battery_status;

	if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN)
		di->full_counter = 0;

	ripley_battery_read_status(di);

	ripley_battery_update_capacity(di);

	if (di->usb_charger_online || di->aux_charger_online) {
		if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN ||
		    di->battery_status == POWER_SUPPLY_STATUS_DISCHARGING) {
			point = 0;
			init_charger(config);
			set_charging_point(di, point);
			enable_charger(1);
			di->battery_status = POWER_SUPPLY_STATUS_CHARGING;
		} else if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING) {
			check_eoc(di);
		}

	} else {
		di->battery_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->full_counter = 0;
	}

	if (power_change_flag) {
		pr_info("battery status: %d, old status: %d, point: %d\n",
			di->battery_status, old_battery_status,
			di->currChargePoint);
		power_change_flag = 0;
	}
	dev_dbg(di->bat.dev, "bat status: %d\n", di->battery_status);
	if (old_battery_status != POWER_SUPPLY_STATUS_UNKNOWN &&
	    di->battery_status != old_battery_status)
		power_supply_changed(&di->bat);
}

static void ripley_battery_work(struct work_struct *work)
{
	struct ripley_dev_info *di = container_of(work,
						  struct ripley_dev_info,
						  monitor_work.work);
	const int interval = HZ * 15;

	dev_dbg(di->dev, "%s\n", __func__);

	ripley_battery_update_status(di);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, interval);
}

/* Double-Check for OVP */
static void battery_ovp_work(struct work_struct *work)
{
	struct ripley_dev_info *di = container_of(work,
						  struct ripley_dev_info,
						  ovp_mon_work.work);
	const int interval = HZ;

	dev_dbg(di->dev, "%s\n", __func__);

	ripley_battery_update_status(di);
	if (di->voltage_uV >= 4250000) { /* No more than 4250000 */
		enable_charger(0);
		cancel_delayed_work_sync(&di->ovp_mon_work);
		pr_warning("more than 4.25v, disable charging\n");
	} else {
		queue_delayed_work(di->monitor_wqueue, &di->ovp_mon_work, interval);

		pr_debug("Re-checking the OVP\n");
	}
}

static void usb_charger_online_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;
	ripley_charger_update_status(di);
}

static void aux_charger_online_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;
	ripley_charger_update_status(di);
}

static void usb_over_voltage_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;
	ripley_charger_update_status(di);
}

static void aux_over_voltage_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;
	ripley_charger_update_status(di);
}

static void battery_over_voltage_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;
	ripley_charger_update_status(di);
}

static void battery_over_temp_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;
	ripley_charger_update_status(di);
}

static void battery_low_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;

	pr_info("\n\n low battery event\n");
	ripley_charger_update_status(di);
}

#if 0
static void battery_charge_complete_event_callback(void *para)
{
	struct ripley_dev_info *di = (struct ripley_dev_info *)para;

	pr_info("\n\n battery charge complete event, disable charging\n");
	queue_delayed_work(di->monitor_wqueue, &di->ovp_mon_work, HZ/10);
}
#endif

static int ripley_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct ripley_dev_info *di = to_ripley_dev_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN) {
			ripley_charger_update_status(di);
			ripley_battery_update_status(di);
		}
		val->intval = di->battery_status;
		return 0;
	default:
		break;
	}

	ripley_battery_read_status(di);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = di->voltage_uV;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = di->current_uA;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = di->accum_current_uAh; /*TODO*/
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = 4200000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = 3300000;
		break;
        case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = di->percent;	/*TODO*/
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = 30;	/*TODO*/
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ripley_battery_remove(struct platform_device *pdev)
{
	pmic_event_callback_t bat_event_callback;
	struct ripley_dev_info *di = platform_get_drvdata(pdev);

	bat_event_callback.func = usb_charger_online_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_unsubscribe(MC34708_EVENT_USBDET, bat_event_callback);
	bat_event_callback.func = aux_charger_online_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_unsubscribe(MC34708_EVENT_AUXDET, bat_event_callback);

	cancel_rearming_delayed_workqueue(di->monitor_wqueue,
					  &di->monitor_work);
	destroy_workqueue(di->monitor_wqueue);
	power_supply_unregister(&di->bat);
	power_supply_unregister(&di->usb_charger);
	power_supply_unregister(&di->aux_charger);

	kfree(di);

	return 0;
}

static int ripley_battery_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct ripley_dev_info *di;
	pmic_event_callback_t bat_event_callback;
	int i;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		retval = -ENOMEM;
		goto di_alloc_failed;
	}

	platform_set_drvdata(pdev, di);

	di->aux_charger.name = "ac";
	di->aux_charger.type = POWER_SUPPLY_TYPE_MAINS;
	di->aux_charger.properties = ripley_aux_charger_props;
	di->aux_charger.num_properties = ARRAY_SIZE(ripley_aux_charger_props);
	di->aux_charger.get_property = ripley_aux_charger_get_property;
	retval = power_supply_register(&pdev->dev, &di->aux_charger);
	if (retval) {
		dev_err(di->dev, "failed to register charger\n");
		goto charger_failed;
	}

	di->usb_charger.name = "usb";
	di->usb_charger.type = POWER_SUPPLY_TYPE_USB;
	di->usb_charger.properties = ripley_usb_charger_props;
	di->usb_charger.num_properties = ARRAY_SIZE(ripley_usb_charger_props);
	di->usb_charger.get_property = ripley_usb_charger_get_property;
	retval = power_supply_register(&pdev->dev, &di->usb_charger);
	if (retval) {
		dev_err(di->dev, "failed to register charger\n");
		goto charger_failed;
	}

	INIT_DELAYED_WORK(&di->monitor_work, ripley_battery_work);
	INIT_DELAYED_WORK(&di->ovp_mon_work, battery_ovp_work);
	di->monitor_wqueue =
	    create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!di->monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}

	for (i = 0; i < CC_RECORD_VOLT_NUMBERS; i++) {
		charging_cc_record[i] = CC_RECORD_INVALID_VAL;
		discharging_cc_record[i] = CC_RECORD_INVALID_VAL;
	}
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ * 10);

	di->dev = &pdev->dev;
	di->chargeConfig = &ripley_charge_config;
	di->bat.name = "battery";
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = ripley_battery_props;
	di->bat.num_properties = ARRAY_SIZE(ripley_battery_props);
	di->bat.get_property = ripley_battery_get_property;
	di->bat.use_for_apm = 1;

	di->battery_status = POWER_SUPPLY_STATUS_UNKNOWN;

	retval = power_supply_register(&pdev->dev, &di->bat);
	if (retval) {
		dev_err(di->dev, "failed to register battery\n");
		goto batt_failed;
	}

	init_battery_profile(di->chargeConfig);

	bat_event_callback.func = usb_over_voltage_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_USBOVP, bat_event_callback);

	bat_event_callback.func = aux_over_voltage_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_AUXOVP, bat_event_callback);

	bat_event_callback.func = usb_charger_online_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_USBDET, bat_event_callback);

	bat_event_callback.func = aux_charger_online_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_AUXDET, bat_event_callback);

	bat_event_callback.func = battery_over_voltage_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_BATTOVP, bat_event_callback);

	bat_event_callback.func = battery_over_temp_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_BATTOTP, bat_event_callback);

	bat_event_callback.func = battery_low_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_LOWBATT, bat_event_callback);

#if 0
	bat_event_callback.func = battery_charge_complete_event_callback;
	bat_event_callback.param = (void *)di;
	pmic_event_subscribe(MC34708_EVENT_CHRCMPL, bat_event_callback);
#endif

//	set_coulomb_counter_cres(CCRES_200_mC_PER_LSB);
//	set_coulomb_counter_integtime(INTEGTIME_8S);
	ripley_calibrate_coulomb_counter();

	goto success;

workqueue_failed:
	power_supply_unregister(&di->aux_charger);
	power_supply_unregister(&di->usb_charger);
charger_failed:
	power_supply_unregister(&di->bat);
batt_failed:
	kfree(di);
di_alloc_failed:
success:
	dev_dbg(di->dev, "%s battery probed!\n", __func__);
	return retval;

	return 0;
}

static int ripley_battery_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	suspend_flag = 1;
	CHECK_ERROR(pmic_write_reg
		    (MC34708_REG_INT_STATUS0, BITFVAL(BATTOVP, 0),
		     BITFMASK(BATTOVP)));
	CHECK_ERROR(pmic_write_reg
		    (MC34708_REG_INT_MASK0, BITFVAL(BATTOVP, 1),
		     BITFMASK(BATTOVP)));

	return 0;
};

static int ripley_battery_resume(struct platform_device *pdev)
{
	suspend_flag = 0;
	CHECK_ERROR(pmic_write_reg
		    (MC34708_REG_INT_MASK0, BITFVAL(BATTOVP, 0),
		     BITFMASK(BATTOVP)));

	return 0;
};

static struct platform_driver ripley_battery_driver_ldm = {
	.driver = {
		   .name = "mc34708_battery",
		   .bus = &platform_bus_type,
		   },
	.probe = ripley_battery_probe,
	.remove = ripley_battery_remove,
	.suspend = ripley_battery_suspend,
	.resume = ripley_battery_resume,
};

static int __init ripley_battery_init(void)
{
	pr_debug("Ripley Battery driver loading...\n");
	return platform_driver_register(&ripley_battery_driver_ldm);
}

static void __exit ripley_battery_exit(void)
{
	platform_driver_unregister(&ripley_battery_driver_ldm);
	pr_debug("Ripley Battery driver successfully unloaded\n");
}

module_init(ripley_battery_init);
module_exit(ripley_battery_exit);

MODULE_DESCRIPTION("ripley_battery driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
