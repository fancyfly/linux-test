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
#include <linux/mfd/mc34708/mc34708_battery.h>
#include <linux/mfd/mc34708/mc34708_adc.h>
#include <linux/pmic_status.h>
#include <linux/pmic_external.h>
#include <linux/mfd/mc34708/mc34708.h>

#define CHRCMPLM_LSH	10
#define CHRCMPLM_WID	1

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

#define CHRTIMER_LSH  18
#define CHRTIMER_WID  4

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
#define ACC_CCFAULT	0x80

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

#define EOC_CURRENT_UA			50000
#define EOC_VOLTAGE_UV			4100000
#define EOC_CCOUT			5
#define LOW_VOLT_THRESHOLD		3400000
#define HIGH_VOLT_THRESHOLD		4200000
#define VOLT_THRESHOLD_DELTA		((HIGH_VOLT_THRESHOLD-LOW_VOLT_THRESHOLD)*3/100)
#define RECHARGING_VOLT_THRESHOLD	4100000
					/* larger than h/w recharging
					 * threshold 95.4% of the CHRCV */

#define ADC_MAX_CHANNEL		8

#define DEFAULT_INTER_RESISTOR_mOhm	150

static int suspend_flag;
static int power_supply_changed_flag;
static int power_change_flag;
static int second_charging_flag;
static int reset_cc_flag;
static int capacity_changed_flag;

static int last_batt_complete_id = -1;
bool can_calculate_capacity = false;

static int before_use_calculated_capacity = 1;

struct last_batt_rec {
	int voltage_uV;
	int current_uA;
};

#define NUM_BATT_RECORD		4
struct last_batt_rec last_batt_rec[NUM_BATT_RECORD];
static int batt_rec_index;

static unsigned int ccres_mC;
enum batt_complete_id {
	BATT_FULL = 1,
	BATT_EMPTY,
};

static int batt_ivolt_rec[NUM_BATT_RECORD];
static int batt_ivolt_index;

/*
usb_type = 0x4; USB host;
usb_type = 0x20; USB charger;
usb_type = 0x40; Dedicated charger;
*/
static int usb_type;
static struct mc34708_charger_setting_point ripley_charger_setting_point[] = {
	{
	 .microVolt = HIGH_VOLT_THRESHOLD,
	 .microAmp = 550000,//1550000,
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
	.toppingOffMicroAmp = 400000,	/* 400 mA */
	.maxChargingHour = 6,
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

static u32 ccres_mc[] =
{
	[CCRES_100_mC_PER_LSB] = 100,
	[CCRES_200_mC_PER_LSB] = 200,
	[CCRES_500_mC_PER_LSB] = 500,
	[CCRES_1000_mC_PER_LSB] = 1000,
};

enum integtime {
	INTEGTIME_4S = 0,
	INTEGTIME_8S,
	INTEGTIME_16S,
	INTEGTIME_32S,
};

struct ripley_dev_info;
static void ripley_battery_update_status(struct ripley_dev_info *);
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

static int get_coulomb_counter_cres_mC(void)
{
	int ret;
	unsigned int ccres;
	ret = pmic_read_reg(MC34708_REG_ACC1, &ccres, BITFMASK(CCRES));
	if (ret != 0)
		return -1;
	ccres = BITFEXT(ccres, CCRES);

	if (ccres < CCRES_100_mC_PER_LSB || ccres >= CCRES_INVALID)
		return -1;

	return ccres_mc[ccres];
}

static int convert_to_uAh(int cc)
{
	return cc * (ccres_mC * 1000 / 3600);
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

	/* See table: BATTCURRENT Resolution in RM */
	return 100000 * (2 << ccres) / ((2 << integtime) * 4);
}

static int enable_charger(int enable)
{
	pr_info("%s %d\n", __func__, enable);
	CHECK_ERROR(pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
				   BITFVAL(CHREN, enable ? 1 : 0),
				   BITFMASK(CHREN)));
	return 0;
}

static int enable_1p5(int enable)
{
	/* enable or disable 1P5 large current */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_DEBOUNCE,
				   BITFVAL(ILIM1P5, enable ? 1 : 0),
				   BITFMASK(ILIM1P5)));

	return 0;
}

static int reset_cc(void)
{
	unsigned int value;
	int ret;

	ret = pmic_read_reg(MC34708_REG_ACC0, &value, 0xff);
	if (ret)
		return -1;

	if (value & ACC_CCFAULT) {
		pr_warning("CCFAULT: may enlarge coulomb counter resolution\n");
	}

	value |= 1 << ACC_RSTCC_LSH;
	ret = pmic_write_reg(MC34708_REG_ACC0, value, PMIC_ALL_BITS);
	if (ret)
		return -1;

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

static int ripley_get_batt_volt_curr_raw(unsigned short *volt,
					unsigned short *curr)
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
static int ripley_get_batt_volt_curr(int *volt, int *curr)
{
	int retval;
	unsigned short voltage_raw;
	signed short current_raw;

#if 0
	retval = ripley_get_batt_voltage(&voltage_raw);
	if (retval == 0)
		*volt = voltage_raw * BAT_VOLTAGE_UNIT_UV;

	retval = ripley_get_batt_current(&(di->current_raw));
	if (retval == 0) {
		if (di->current_raw & 0x200)
			*curr =
			    (0x1FF - (di->current_raw & 0x1FF)) *
			    BAT_CURRENT_UNIT_UA * (-1);
		else
			*curr =
			    (di->current_raw & 0x1FF) * BAT_CURRENT_UNIT_UA;
	}
#else
	/* AMPD suggests to sample volt/curr alternately */
	retval = ripley_get_batt_volt_curr_raw(&voltage_raw, &current_raw);
	if (retval == 0) {
		*volt = voltage_raw * BAT_VOLTAGE_UNIT_UV;

		if (current_raw & 0x200)
			*curr =
			    (0x1FF - (current_raw & 0x1FF)) *
			    BAT_CURRENT_UNIT_UA * (-1);
		else
			*curr =
			    (current_raw & 0x1FF) * BAT_CURRENT_UNIT_UA;
	}
#endif

	return retval;
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

static int ripley_get_charger_coulomb(int *coulomb, int *ccfault)
{
	int ret;
	unsigned int value;

	ret = pmic_read_reg(MC34708_REG_ACC0, &value, PMIC_ALL_BITS);
	if (ret != 0)
		return -1;

	*ccfault = 0;
	if (value & ACC_CCFAULT) {
		pr_warning("CCFAULT: may enlarge coulomb counter resolution\n");
		*ccfault = 1;
		pmic_write_reg(MC34708_REG_ACC0, value, PMIC_ALL_BITS);
		ret = pmic_read_reg(MC34708_REG_ACC0, &value, PMIC_ALL_BITS);
		if (ret != 0)
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
	*coulomb = value;

	return 0;
}

struct ripley_dev_info {
	struct device *dev;

	unsigned short thermal_raw;
	int voltage_uV;
	int old_voltage_uV;
	int current_uA;
	int old_battery_status;
	int battery_status;
	int full_counter;
	int chargeTimeSeconds;
	int usb_charger_online;
	int aux_charger_online;
	int charger_voltage_uV;
	int accum_coulomb;
	int full_coulomb;
	int empty_coulomb;
	int real_capacity;
	int percent;
	int old_percent;
	int first_get_percent;
	int now_coulomb;
	int delta_coulomb;
	int internal_resistor_mOhm;
	int internal_voltage_uV;

	int currChargePoint;

	struct power_supply bat;
	struct power_supply usb_charger;
	struct power_supply aux_charger;

	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
	struct delayed_work ovp_mon_work;
	struct delayed_work calc_resistor_mon_work;
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
#define CHRIMER_HR_TO_BITS(hr)	(((hr) > 16) ? 15 : ((hr) - 1))


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

	/* set charging expire timer */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_SOURCE,
		BITFVAL(CHRTIMER, CHRIMER_HR_TO_BITS(config->maxChargingHour)),
		BITFMASK(CHRTIMER)));

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
				   BITFVAL(MSW, 1), BITFMASK(MSW)));
	 /* enable 1P5 large current */
	enable_1p5(true);

	/* enable ISO */
	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_DEBOUNCE,
				   BITFVAL(BATTISOEN, 0), BITFMASK(BATTISOEN)));

	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_SOURCE,
				   BITFVAL(AUXWEAKEN, 1), BITFMASK(AUXWEAKEN)));

	CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_SOURCE,
				   BITFVAL(VBUSWEAKEN, 1),
				   BITFMASK(VBUSWEAKEN)));

	return 0;
}

#define CHRCV_UV_TO_BITS(uv)	((uv - 3500000) / 20000)
#define CHRCC_UA_TO_BITS(ua)	((((ua < 250000) ? 250000 :	\
				(ua > 1550000) ? 1550000 : ua)	\
				- 250000) / 100000)
#define CHRCC_BITS_TO_UA(b)	((((b) > 13) ? 13 : (b)) * 100000 + 250000)

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
		/*
		 * NOTE: When set the ILIM_1P5 bit to 1, the USBCHR[1:0]
		 * and AUXILIM[1:0] settings are ignored.
		 *
		 * enable_1p5(true) called in init_charger()
		 */
		case USBHOST:
			/* disable 1P5 large current */
			enable_1p5(false);
			/*
			 * set current limit to 500mA
			 * Table 7.2-2, Buck input current limit settings
			 */
			CHECK_ERROR(pmic_write_reg(MC34708_REG_CHARGER_DEBOUNCE,
						   BITFVAL(AUXILIM, 6),
						   BITFMASK(AUXILIM)));
			CHECK_ERROR(pmic_write_reg(MC34708_REG_USB_CTL,
						   BITFVAL(MUSBCHRG, 2),
						   BITFMASK(MUSBCHRG)));
			val |= BITFVAL(CHRCC, CHRCC_UA_TO_BITS(250000));
			break;
		case USBCHARGER:
		case DEDICATEDCHARGER:
			/* set current limit to 950mA */
			CHECK_ERROR(pmic_write_reg(MC34708_REG_USB_CTL,
						   BITFVAL(MUSBCHRG, 3),
						   BITFMASK(MUSBCHRG)));
			val |= BITFVAL(CHRCC, CHRCC_UA_TO_BITS(1550000));
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

		if (usb_type != USBHOST)
			enable_charger(1);
		else
			enable_charger(0);

		di->currChargePoint = point;

		return 0;
	}

	return -EINVAL;
}

static int get_battcur(int *batt_curr)
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

	/* 2's complement */
	if (battcur & 0x800) {
		battcur = ~battcur;
		battcur += 1;
		battcur &= 0x7ff;
		battcur *= -1;
	}

	battcur *= battcur_res;
	pr_info("batt cur value = %d\n", battcur / 1000);

	*batt_curr = battcur;

	return 0;
}

static int check_eoc(struct ripley_dev_info *di)
{
	int ret;
	int voltage_uV;
	int battcur;
	unsigned short voltage_raw;

	ret = get_battcur(&battcur);
	if (ret) {
		pr_err("can not get battcur.\n");
		return -1;
	}

	ret = ripley_get_batt_voltage(&voltage_raw);
	if (ret == 0)
		voltage_uV = voltage_raw * BAT_VOLTAGE_UNIT_UV;

	if (voltage_uV >= EOC_VOLTAGE_UV &&
		battcur <= EOC_CURRENT_UA &&
		di->delta_coulomb < EOC_CCOUT)
		di->full_counter++;
	else
		di->full_counter = 0;

	if (di->full_counter > 2)
		di->battery_status = POWER_SUPPLY_STATUS_FULL;
	else
		di->battery_status = POWER_SUPPLY_STATUS_CHARGING;

	return 0;
}

static void _save_and_change_chrcc(int *old_chrcc, int chrcc)
{
	unsigned int value;

	pmic_read_reg(MC34708_REG_BATTERY_PROFILE, &value, BITFMASK(CHRCC));
	value = BITFEXT(value, CHRCC);
	if (old_chrcc != NULL)
		*old_chrcc = CHRCC_BITS_TO_UA(value);

	if (chrcc < 250000) {
		pr_warning("CHRCC can not be lower than 250000 uA.\n");
		return;
	}
	pmic_write_reg(MC34708_REG_BATTERY_PROFILE,
			BITFVAL(CHRCC, CHRCC_UA_TO_BITS(chrcc)),
			BITFMASK(CHRCC));
}

static int _record_last_batt_info(struct ripley_dev_info *di, bool force_read)
{
	int voltage_uV, current_uA;
	int ret;

	if (force_read) {
		ret = ripley_get_batt_volt_curr(&voltage_uV, &current_uA);
		if (ret) {
			pr_err("%s: ripley_get_batt_volt_curr() failed.\n",
				__func__);
			return ret;
		}

		last_batt_rec[batt_rec_index].voltage_uV = voltage_uV;
		last_batt_rec[batt_rec_index].current_uA = current_uA;
	} else {
		last_batt_rec[batt_rec_index].voltage_uV = di->voltage_uV;
		last_batt_rec[batt_rec_index].current_uA = di->current_uA;
	}

	batt_rec_index++;
	if (batt_rec_index >= NUM_BATT_RECORD)
		batt_rec_index = 0;

	return 0;
}

static int _is_valid_to_calc_internal_resistor(struct ripley_dev_info *di,
						bool power_supply_changed_flag)
{
	int curr_index, compared_index;

	if (batt_rec_index == 0)
		curr_index = NUM_BATT_RECORD - 1;
	else
		curr_index = batt_rec_index - 1;

	if (curr_index == 0)
		compared_index = NUM_BATT_RECORD - 1;
	else
		compared_index = curr_index - 1;

	if (power_supply_changed_flag) {
		if ((last_batt_rec[curr_index].current_uA > 0 &&
			last_batt_rec[compared_index].current_uA < 0) ||
			(last_batt_rec[curr_index].current_uA < 0 &&
			last_batt_rec[compared_index].current_uA > 0))
			return true;

		return false;
	} else {
		if ((last_batt_rec[curr_index].current_uA > 0 &&
			last_batt_rec[compared_index].current_uA > 0) &&
			abs(last_batt_rec[curr_index].current_uA -
			   last_batt_rec[compared_index].current_uA) > 100000)
			return true;

		return false;
	}
}

/* this is only used to calculate when battery is during learning phase */
static void _calculate_internal_resistor(struct ripley_dev_info *di)
{
	int curr_index, compared_index;

	int voltage0_uV, voltage1_uV;
	int current0_uA, current1_uA;
	int internal_resistor_mOhm = 0;

	if (batt_rec_index == 0)
		curr_index = NUM_BATT_RECORD - 1;
	else
		curr_index = batt_rec_index - 1;

	if (curr_index == 0)
		compared_index = NUM_BATT_RECORD - 1;
	else
		compared_index = curr_index - 1;

	voltage0_uV = last_batt_rec[compared_index].voltage_uV;
	current0_uA = last_batt_rec[compared_index].current_uA;

	voltage1_uV = last_batt_rec[curr_index].voltage_uV;
	current1_uA = last_batt_rec[curr_index].current_uA;

	pr_debug("voltage0_uV %d, current0_uA %d\n", voltage0_uV, current0_uA);
	pr_debug("voltage1_uV %d, current1_uA %d\n", voltage1_uV, current1_uA);

	if (current0_uA > 0 && current1_uA > 0) {
		if (current0_uA != current1_uA)
			internal_resistor_mOhm =
				abs(voltage0_uV - voltage1_uV) *
				1000 / abs(current0_uA - current1_uA);
		if (internal_resistor_mOhm != 0)
			di->internal_resistor_mOhm = internal_resistor_mOhm;

	} else if (current0_uA < 0 && current1_uA < 0) {
		;
	} else if ((current0_uA > 0 && current1_uA < 0) ||
		(current0_uA < 0 && current1_uA > 0))
		di->internal_resistor_mOhm = abs(voltage0_uV - voltage1_uV) *
				1000 / (abs(current0_uA) + abs(current1_uA));
	else
		pr_warning("%s: incorrect internal resistor calc.\n", __func__);

	pr_info("ORG: internal_resistor_mOhm %d\n", di->internal_resistor_mOhm);
#ifdef DEFAULT_INTER_RESISTOR_mOhm
	if (abs(di->internal_resistor_mOhm - DEFAULT_INTER_RESISTOR_mOhm) >
		(DEFAULT_INTER_RESISTOR_mOhm / 2))
		di->internal_resistor_mOhm = DEFAULT_INTER_RESISTOR_mOhm;
#endif

	pr_info("internal_resistor_mOhm %d\n", di->internal_resistor_mOhm);

}

static void _record_batt_ivolt(struct ripley_dev_info *di)
{
	batt_ivolt_rec[batt_ivolt_index] = di->internal_voltage_uV;

	batt_ivolt_index++;
	if (batt_ivolt_index >= NUM_BATT_RECORD)
		batt_ivolt_index = 0;
}

static void _recalc_batt_ivolt(struct ripley_dev_info *di)
{
	int accum_ivolt = 0, nr = 0, diff;
	int i;

	for (i = 0; i < NUM_BATT_RECORD; i++) {
		/* within 5% */
		diff = abs(batt_ivolt_rec[i] - di->internal_voltage_uV);
		if (batt_ivolt_rec[i] && (diff == 0 ||
			(di->internal_voltage_uV / diff >= 20))) {
			accum_ivolt += batt_ivolt_rec[i];
			nr++;
		}
	}

	if (nr)
		di->internal_voltage_uV = accum_ivolt / nr;
}

static void _adjust_batt_capacity(struct ripley_dev_info *di)
{
	static int counter;
	static int old_battery_status;

	if (old_battery_status != di->battery_status) {
		di->percent = di->old_percent;
		counter = 0;
	}

	old_battery_status = di->battery_status;

	if (di->battery_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		if (di->percent > di->old_percent) {
			di->percent = di->old_percent;
			counter = 0;
		} else if ((di->old_percent - di->percent) >= 1 &&
		/*	(di->old_percent - di->percent) < 5 &&	*/
			di->old_percent >= 0)
			counter++;

		if (counter > 2) {
			di->percent = di->old_percent - 1;
			counter = 0;
		} else
			di->percent = di->old_percent;
	} else if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING ||
		   di->battery_status == POWER_SUPPLY_STATUS_FULL) {
		if (di->percent < di->old_percent) {
			di->percent = di->old_percent;
			counter = 0;
		} else if ((di->percent - di->old_percent) >= 1 &&
		/*	(di->percent - di->old_percent) < 5 &&	*/
			di->old_percent >= 0)
			counter++;

		if (counter > 2) {
			di->percent = di->old_percent + 1;
			counter = 0;
		} else
			di->percent = di->old_percent;
	}
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
			msleep(50);
			di->aux_charger_online = auxOnline;
			dev_info(di->aux_charger.dev,
				 "aux charger status: %s\n",
				 auxOnline ? "online" : "offline");
			power_supply_changed(&di->aux_charger);
		}
		if (usbOnline != di->usb_charger_online) {
			/* need some delay to know the usb type */
			msleep(80);
			ret = pmic_read_reg(MC34708_REG_USB_DEVICE_TYPE,
					    &reg_usb_type, PMIC_ALL_BITS);
			usb_type = 0;
			if ((reg_usb_type & USBHOST) != 0) {
				usb_type = USBHOST;
				pr_info("USB host attached!!!\n");
				restartCharging = 0;
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
			pr_info("restartCharging\n");
			enable_charger(1);

			power_supply_changed_flag = 1;
			power_change_flag = 1;

			cancel_delayed_work(&di->calc_resistor_mon_work);
			queue_delayed_work(di->monitor_wqueue,
					   &di->calc_resistor_mon_work, HZ / 10);
			cancel_delayed_work(&di->monitor_work);
			queue_delayed_work(di->monitor_wqueue,
					   &di->monitor_work, HZ / 10);
		} else if (stopCharging) {
			pr_info("stopCharging\n");
			enable_charger(0);

			power_supply_changed_flag = 1;
			power_change_flag = 1;

			cancel_delayed_work(&di->calc_resistor_mon_work);
			queue_delayed_work(di->monitor_wqueue,
					   &di->calc_resistor_mon_work, HZ / 10);
			cancel_delayed_work(&di->monitor_work);
			queue_delayed_work(di->monitor_wqueue,
					   &di->monitor_work, HZ / 10);
		}
	}

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
	int coulomb, ccfault;
	static int old_delta_coulomb;
	static unsigned long last;

	/* Do not read info within 1/2 second */
	if (last && time_before(jiffies, last + HZ / 2))
		return 0;

	last = jiffies;

	retval = ripley_get_batt_volt_curr(&(di->voltage_uV),
					&(di->current_uA));
	if (retval)
		dev_err(di->dev, "ripley_get_batt_volt_curr() failed.\n");

	retval = ripley_get_charger_coulomb(&coulomb, &ccfault);
	if (retval == 0) {
		if (ccfault == 0) {
			di->delta_coulomb = coulomb - di->now_coulomb;
			di->now_coulomb = coulomb;
			old_delta_coulomb = di->delta_coulomb;
		} else {
			di->delta_coulomb = old_delta_coulomb; /* use latest */
			di->now_coulomb = coulomb;
		}

		di->accum_coulomb += di->delta_coulomb;
	}

	if (reset_cc_flag) {
		reset_cc();
		di->now_coulomb = 0;
		reset_cc_flag = false;
	}
	pr_info("readout: vol %d uV, cur %d uA, cc %d\n", di->voltage_uV,
							di->current_uA,
							di->now_coulomb);
	pr_info("calc-ed: delta cc %d ( %d mC ), accum_coulomb %d, "
		"accum_current_uAh %d\n",
					di->delta_coulomb,
					di->accum_coulomb,
					convert_to_uAh(di->accum_coulomb));
	return retval;
}

static void ripley_battery_update_capacity(struct ripley_dev_info *di)
{
	static bool can_adjust_percent = false;
	static int lowbatt_counter;
	int compared_batt_volt;

	if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN) {
		di->percent = (di->voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
			 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);

		if (di->percent < 0)
			di->percent = 0;
		else if (di->percent > 99)
			di->percent = 99;

	} else if (di->battery_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		/* determine discharging complete */
		if (di->internal_resistor_mOhm <= 0)
			compared_batt_volt = di->voltage_uV;
		else {
			compared_batt_volt = di->internal_voltage_uV =
				di->voltage_uV + abs(di->current_uA) *
				di->internal_resistor_mOhm / 1000;
		}

		/* trick: trigger before LOW_VOLT_THRESHOLD reached */
		if (compared_batt_volt <= (LOW_VOLT_THRESHOLD +
						VOLT_THRESHOLD_DELTA))
			lowbatt_counter++;
		else
			lowbatt_counter = 0;

		if (lowbatt_counter > 1) {/* discharging cmplt */
			pr_warning("Battery Low < %d uV\n", LOW_VOLT_THRESHOLD);
			if (last_batt_complete_id > 0 &&
				last_batt_complete_id != BATT_EMPTY) {
				can_calculate_capacity = true;
				before_use_calculated_capacity = 0;
			} else
				can_calculate_capacity = false;

			di->empty_coulomb = di->accum_coulomb;
			pr_info("DISCHR CMPL: di->empty_coulomb %d\n",
					di->empty_coulomb);
			last_batt_complete_id = BATT_EMPTY;	

			/* Reset CCOUT to avoid underflow */
			reset_cc_flag = true;
		}

		if (before_use_calculated_capacity == 1) {
			if (di->internal_resistor_mOhm <= 0)
				di->percent = (di->voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
					 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);
			else if (di->current_uA < 0) {
				di->internal_voltage_uV = di->voltage_uV +
					abs(di->current_uA) * di->internal_resistor_mOhm / 1000;
				_record_batt_ivolt(di);
				_recalc_batt_ivolt(di);
				di->percent = (di->internal_voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
					 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);
				pr_info("di->internal_voltage_uV %d, percent %d\n", di->internal_voltage_uV,
									di->percent);
				pr_info("di->internal_resistor_mOhm %d\n", di->internal_resistor_mOhm);
			} else {
				if (di->old_percent >= 0)
					di->percent = di->old_percent;
			}

			/* smooth the percent value, to avoid glitch */
			if (can_adjust_percent) {
				_adjust_batt_capacity(di);
			}

			if (di->percent < 0)
				di->percent = 0;
			else if (di->percent > 99)
				di->percent = 99;

			can_adjust_percent = true;

			goto out1;
		}

		/* otherwise use C-COUNTER */
		if (can_calculate_capacity) {
			/* recal capacity and get the real total capacity */
			di->real_capacity =
				di->full_coulomb - di->empty_coulomb;
			di->empty_coulomb = 0;
			di->accum_coulomb = 0;
			can_calculate_capacity = false;
			pr_debug("DISCHR can_cal real_capacity %d\n",
						di->real_capacity);
		}
		pr_debug("di->real_capacity %d\n", di->real_capacity);

		di->percent += di->delta_coulomb * 100 / di->real_capacity;
		di->percent = di->percent < 0 ? 0 : di->percent;

	} else if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING || 
		   di->battery_status == POWER_SUPPLY_STATUS_FULL) {

		lowbatt_counter = 0;

		if (di->battery_status == POWER_SUPPLY_STATUS_FULL) {
			if (last_batt_complete_id > 0 &&
				last_batt_complete_id != BATT_FULL) {
				can_calculate_capacity = true;
				before_use_calculated_capacity = 0;
			} else
				can_calculate_capacity = false;

			di->full_coulomb = di->accum_coulomb;
			pr_info("CHR CMPL: di->full_coulomb %d\n",
					di->full_coulomb);
			last_batt_complete_id = BATT_FULL;	

			/* Reset CCOUT to avoid overflow */
			reset_cc_flag = true;
		}

		if (before_use_calculated_capacity == 1) {
			if (di->internal_resistor_mOhm <= 0)
				di->percent = (di->voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
					 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);
			else if (di->current_uA > 0) {
				di->internal_voltage_uV = di->voltage_uV -
					di->current_uA * di->internal_resistor_mOhm / 1000;
				_record_batt_ivolt(di);
				_recalc_batt_ivolt(di);
				di->percent = (di->internal_voltage_uV - LOW_VOLT_THRESHOLD) * 100 /
					 (HIGH_VOLT_THRESHOLD - LOW_VOLT_THRESHOLD);
				pr_info("di->internal_voltage_uV %d, percent %d\n", di->internal_voltage_uV,
									di->percent);
				pr_info("di->internal_resistor_mOhm %d\n", di->internal_resistor_mOhm);
			} else {
				if (di->old_percent >= 0)
					di->percent = di->old_percent;
			}

			/* smooth the percent value, to avoid glitch */
			if (can_adjust_percent) {
				_adjust_batt_capacity(di);
			}

			if (di->percent < 0)
				di->percent = 0;
			else if (di->percent > 99)
				di->percent = 99;

			can_adjust_percent = true;

			goto out1;
		}

		/* otherwise use C-COUNTER */
		if (can_calculate_capacity) {
			/* recal capacity and get the real total capacity */
			di->real_capacity =
				di->full_coulomb - di->empty_coulomb;

			can_calculate_capacity = false;
			pr_debug("CHR can_cal real_capacity %d\n",
						di->real_capacity);
		}
		pr_debug("CHR di->real_capacity %d\n", di->real_capacity);

		di->percent += di->delta_coulomb * 100 / di->real_capacity;
		di->percent = di->percent > 99 ? 99 : di->percent;

	}
out1:
	/* re-consider special cases here */
	if (lowbatt_counter > 1) /* discharging cmplt */
		di->percent = 0;

	if (di->battery_status == POWER_SUPPLY_STATUS_FULL) {
		pr_info("POWER_SUPPLY_STATUS_FULL\n");
		di->percent = 100;

		pmic_write_reg(MC34708_REG_CHARGER_DEBOUNCE,
				BITFVAL(BATTISOEN, 1),
				BITFMASK(BATTISOEN));
		/* set as DISCHARGING WITH CHARGER */
		di->battery_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		second_charging_flag = false;
	}

	if (second_charging_flag)
		di->percent = 100;

	if (di->old_percent != di->percent) {
		if (di->battery_status != POWER_SUPPLY_STATUS_UNKNOWN)
			capacity_changed_flag = true;
		di->old_percent = di->percent;
	}

	pr_info("di->percent %d ...\n", di->percent);
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
			di->battery_status = POWER_SUPPLY_STATUS_CHARGING;
			if (usb_type == USBHOST)
				di->battery_status = POWER_SUPPLY_STATUS_DISCHARGING;
		} else if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING) {
			check_eoc(di);
		} else if (di->battery_status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			if (di->voltage_uV < RECHARGING_VOLT_THRESHOLD) {
				pr_info("Battery volt < %d uV, Re-Charging.\n",
					RECHARGING_VOLT_THRESHOLD);
				point = 0;
				init_charger(config);
				set_charging_point(di, point);
				di->battery_status = POWER_SUPPLY_STATUS_CHARGING;
				second_charging_flag = true;
			}
		}

	} else {
		di->battery_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->full_counter = 0;
		second_charging_flag = false;
	}

	if (power_change_flag) {
		pr_info("battery status: %d, old status: %d, point: %d\n",
			di->battery_status, old_battery_status,
			di->currChargePoint);
		power_change_flag = 0;
	}
	dev_info(di->bat.dev, "bat status: %d\n", di->battery_status);
	if ((old_battery_status != POWER_SUPPLY_STATUS_UNKNOWN &&
	    di->battery_status != old_battery_status) ||
	    capacity_changed_flag) {
		power_supply_changed(&di->bat);
		capacity_changed_flag = false;
	}
}

static void ripley_battery_work(struct work_struct *work)
{
	struct ripley_dev_info *di = container_of(work,
						  struct ripley_dev_info,
						  monitor_work.work);
	const int interval = HZ * 8;

	dev_dbg(di->dev, "%s\n", __func__);

	ripley_battery_update_status(di);
	_record_last_batt_info(di, false);
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

static void calc_resistor_work(struct work_struct *work)
{
	struct ripley_dev_info *di = container_of(work,
						  struct ripley_dev_info,
						  calc_resistor_mon_work.work);
	int old_chrcc;
	static bool flag_changed;
	static bool flag_captured_once;
	static int retry;

	if (power_supply_changed_flag) {
		/* wait until we can calc */
		if (!_is_valid_to_calc_internal_resistor(di,
				power_supply_changed_flag)) {
			queue_delayed_work(di->monitor_wqueue,
				   &di->calc_resistor_mon_work, HZ * 2);
			pr_warning("not valid to calc internal resistor.\n");
			retry++;
			if (retry > 5) {
				retry = 0;
				power_supply_changed_flag = 0;
			}

			return;
		}
		_calculate_internal_resistor(di);

		power_supply_changed_flag = 0;
		flag_changed = 1;
		flag_captured_once = 0;

		/* Postpone 30 Min */
		queue_delayed_work(di->monitor_wqueue,
				   &di->calc_resistor_mon_work, HZ * 1 * 60);
	} else {
		if (flag_changed) {
			flag_changed = 0;
			queue_delayed_work(di->monitor_wqueue,
					   &di->calc_resistor_mon_work,
					   HZ * 30 * 60);
		} else {
			if (di->battery_status == POWER_SUPPLY_STATUS_CHARGING) {
				if (flag_captured_once) {
					_record_last_batt_info(di, true);
					_save_and_change_chrcc(&old_chrcc, 350000);
					msleep(2000);
					_record_last_batt_info(di, true);
					_save_and_change_chrcc(NULL, old_chrcc);

					if (_is_valid_to_calc_internal_resistor(di, 0)) {
						_calculate_internal_resistor(di);
					}

					flag_captured_once = 0;
					queue_delayed_work(di->monitor_wqueue,
							   &di->calc_resistor_mon_work, HZ * 30 * 60);
				} else {
					flag_captured_once = 1;
					queue_delayed_work(di->monitor_wqueue,
							   &di->calc_resistor_mon_work, HZ * 5 * 60);
				}
			} else /* Not calc inter resistor when discharge */
				queue_delayed_work(di->monitor_wqueue,
						   &di->calc_resistor_mon_work,
						   HZ * 30 * 60);
		}
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

#if 1
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
	static unsigned long last;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = di->battery_status;
		/* Do not reveal re-charging status to end users */
		if (di->battery_status == POWER_SUPPLY_STATUS_NOT_CHARGING ||
			second_charging_flag) /* CHARGING again*/
			val->intval = POWER_SUPPLY_STATUS_FULL;
		return 0;
	default:
		break;
	}

	if (!last || time_after(jiffies, last + HZ / 2)) {
		last = jiffies;
		ripley_battery_read_status(di);
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = di->voltage_uV;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = di->current_uA;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = convert_to_uAh(di->accum_coulomb);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = HIGH_VOLT_THRESHOLD;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = LOW_VOLT_THRESHOLD;
		break;
        case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = di->percent;
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

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		retval = -ENOMEM;
		goto di_alloc_failed;
	}

	platform_set_drvdata(pdev, di);

	di->dev = &pdev->dev;
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

	di->chargeConfig = &ripley_charge_config;
	di->bat.name = "battery";
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = ripley_battery_props;
	di->bat.num_properties = ARRAY_SIZE(ripley_battery_props);
	di->bat.get_property = ripley_battery_get_property;
	di->bat.use_for_apm = 1;

	di->battery_status = POWER_SUPPLY_STATUS_UNKNOWN;
	di->accum_coulomb = 0;
	di->internal_resistor_mOhm = -1;
#ifdef DEFAULT_INTER_RESISTOR_mOhm
	di->internal_resistor_mOhm = DEFAULT_INTER_RESISTOR_mOhm;
#endif

	retval = power_supply_register(&pdev->dev, &di->bat);
	if (retval) {
		dev_err(di->dev, "failed to register battery\n");
		goto batt_failed;
	}

	init_battery_profile(di->chargeConfig);
	di->old_percent = -1;
//	enable_charger(false);

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
//	set_coulomb_counter_integtime(INTEGTIME_32S);
	ripley_calibrate_coulomb_counter();
	ccres_mC = get_coulomb_counter_cres_mC();
	printk("ccres_mC %d\n", ccres_mC);
	if (ccres_mC < 0)
		dev_err(di->dev, "can not get correct cc resolution.\n");

	INIT_DELAYED_WORK(&di->monitor_work, ripley_battery_work);
	INIT_DELAYED_WORK(&di->ovp_mon_work, battery_ovp_work);
	INIT_DELAYED_WORK(&di->calc_resistor_mon_work, calc_resistor_work);
	di->monitor_wqueue =
	    create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!di->monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}

	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ * 10);
	queue_delayed_work(di->monitor_wqueue, &di->calc_resistor_mon_work,
				HZ * 10);
	ripley_charger_update_status(di);

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
}

static int ripley_battery_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	struct ripley_dev_info *di = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&di->calc_resistor_mon_work);
	cancel_delayed_work_sync(&di->monitor_work);

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
	struct ripley_dev_info *di = platform_get_drvdata(pdev);

	suspend_flag = 0;
	CHECK_ERROR(pmic_write_reg
		    (MC34708_REG_INT_MASK0, BITFVAL(BATTOVP, 0),
		     BITFMASK(BATTOVP)));

	queue_delayed_work(di->monitor_wqueue,
			   &di->calc_resistor_mon_work, HZ / 10);
	queue_delayed_work(di->monitor_wqueue,
			   &di->monitor_work, HZ / 10);

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
