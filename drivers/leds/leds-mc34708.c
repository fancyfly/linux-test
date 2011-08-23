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
/*
 * linux/drivers/leds-mc34708.c
 *
 * mc34708 PWM based LED control
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/mfd/mc34708/mc34708_pwm.h>
#include <linux/leds-mc34708.h>
#include <linux/slab.h>

struct led_mc34708_data {
	struct led_classdev	cdev;
	struct pwm_device	*pwm;
	unsigned int		period;
};

static void led_mc34708_set(struct led_classdev *led_cdev,
	enum led_brightness brightness)
{
	struct led_mc34708_data *led_dat =
		container_of(led_cdev, struct led_mc34708_data, cdev);
	unsigned int max = led_dat->cdev.max_brightness;
	unsigned int period =  led_dat->period;

	dev_dbg(led_cdev->dev, "set led brightness:%d\n", brightness);

	if (brightness == 0) {
		mc34708_pwm_config(led_dat->pwm, 0, period);
		mc34708_pwm_disable(led_dat->pwm);
	} else {
		mc34708_pwm_config(led_dat->pwm, brightness * period / max, period);
		mc34708_pwm_enable(led_dat->pwm);
	}
}

static int led_mc34708_probe(struct platform_device *pdev)
{
	struct led_mc34708_platform_data *pdata = pdev->dev.platform_data;
	struct led_mc34708 *cur_led;
	struct led_mc34708_data *leds_data, *led_dat;
	int i, ret = 0;

	if (!pdata)
		return -EBUSY;

	leds_data = kzalloc(sizeof(struct led_mc34708_data) * pdata->num_leds,
				GFP_KERNEL);
	if (!leds_data) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < pdata->num_leds; i++) {
		cur_led = &pdata->leds[i];
		led_dat = &leds_data[i];

		led_dat->pwm = mc34708_pwm_request(cur_led->pwm_id,
				cur_led->name);
		if (IS_ERR(led_dat->pwm)) {
			dev_err(&pdev->dev, "unable to request MC34708 PWM %d\n",
					cur_led->pwm_id);
			goto err;
		}

		led_dat->cdev.name = cur_led->name;
		led_dat->period = cur_led->pwm_period_ns;
		led_dat->cdev.brightness_set = led_mc34708_set;
		led_dat->cdev.brightness = LED_OFF;
		led_dat->cdev.max_brightness = cur_led->max_brightness;
		led_dat->cdev.flags |= LED_CORE_SUSPENDRESUME;

		ret = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (ret < 0) {
			dev_err(&pdev->dev, "unable to register led class device:%d\n",
					cur_led->pwm_id);
			mc34708_pwm_free(led_dat->pwm);
			goto err;
		}
	}

	platform_set_drvdata(pdev, leds_data);

	return 0;

err:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			led_classdev_unregister(&leds_data[i].cdev);
			mc34708_pwm_free(leds_data[i].pwm);
		}
	}

	kfree(leds_data);

	return ret;
}

static int __devexit led_mc34708_remove(struct platform_device *pdev)
{
	int i;
	struct led_mc34708_platform_data *pdata = pdev->dev.platform_data;
	struct led_mc34708_data *leds_data;

	leds_data = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		led_classdev_unregister(&leds_data[i].cdev);
		mc34708_pwm_free(leds_data[i].pwm);
	}

	kfree(leds_data);

	return 0;
}

static struct platform_driver led_mc34708_driver = {
	.probe		= led_mc34708_probe,
	.remove		= __devexit_p(led_mc34708_remove),
	.driver		= {
		.name	= "leds-mc34708",
		.owner	= THIS_MODULE,
	},
};

static int __init led_mc34708_init(void)
{
	return platform_driver_register(&led_mc34708_driver);
}

static void __exit led_mc34708_exit(void)
{
	platform_driver_unregister(&led_mc34708_driver);
}

module_init(led_mc34708_init);
module_exit(led_mc34708_exit);

MODULE_DESCRIPTION("MC34708 PWM LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:leds-mc34708");
