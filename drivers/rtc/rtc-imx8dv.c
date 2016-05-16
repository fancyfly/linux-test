/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <soc/imx8/sc/sci.h>

sc_ipc_t timer_ipcHandle;

struct imx8dv_rtc_data {
	struct rtc_device *rtc;
	spinlock_t lock;
};

struct imx8dv_rtc_data *data;

static int imx8dv_rtc_alarm_sc_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
	u32 events = 0;

	rtc_update_irq(data->rtc, 1, events);

	return 0;
}

static struct notifier_block imx8dv_rtc_alarm_sc_notifier = {
	.notifier_call = imx8dv_rtc_alarm_sc_notify,
};

static int imx8dv_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	u32 time = 0;
	sc_err_t sciErr = SC_ERR_NONE;

	if (!timer_ipcHandle) {
		return -1;
	}

	sciErr = sc_timer_get_rtc_sec1970(timer_ipcHandle, &time);
	if (sciErr) {
		dev_err(dev, "failed to read time: %d\n", sciErr);
		return -1;
	}

	rtc_time_to_tm(time, tm);

	return 0;
}

static int imx8dv_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	sc_err_t sciErr = SC_ERR_NONE;

	if (!timer_ipcHandle)
		return -1;

	sciErr = sc_timer_set_rtc_time(timer_ipcHandle,
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);

	if (sciErr) {
		dev_err(dev, "failed to set time: %d\n", sciErr);
		return -1;
	}

	return 0;
}

static int imx8dv_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	return 0;
}

static int imx8dv_rtc_alarm_irq_enable(struct device *dev, unsigned int enable)
{
	return 0;
}

static int imx8dv_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	sc_err_t sciErr = SC_ERR_NONE;
	struct rtc_time *alrm_tm = &alrm->time;

	if (!timer_ipcHandle)
		return -1;

	sciErr = sc_timer_set_rtc_alarm(timer_ipcHandle,
		alrm_tm->tm_year + 1900,
		alrm_tm->tm_mon + 1,
		alrm_tm->tm_mday,
		alrm_tm->tm_hour,
		alrm_tm->tm_min,
		alrm_tm->tm_sec);

	return 0;
}

static const struct rtc_class_ops imx8dv_rtc_ops = {
	.read_time = imx8dv_rtc_read_time,
	.set_time = imx8dv_rtc_set_time,
	.read_alarm = imx8dv_rtc_read_alarm,
	.set_alarm = imx8dv_rtc_set_alarm,
	.alarm_irq_enable = imx8dv_rtc_alarm_irq_enable,
};

static int imx8dv_rtc_probe(struct platform_device *pdev)
{
	int ret = 0;
	uint32_t mu_id;
	sc_err_t sciErr;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);

	device_init_wakeup(&pdev->dev, true);
	data->rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
					&imx8dv_rtc_ops, THIS_MODULE);
	if (IS_ERR(data->rtc)) {
		ret = PTR_ERR(data->rtc);
		dev_err(&pdev->dev, "failed to register rtc: %d\n", ret);
		goto error_rtc_device_register;
	}

	sciErr = sc_ipc_getMuID(&mu_id);
	if (sciErr != SC_ERR_NONE) {
		dev_err(&pdev->dev, "can not obtain mu id: %d\n", sciErr);
		return sciErr;
	}

	sciErr = sc_ipc_open(&timer_ipcHandle, mu_id);

	if (sciErr != SC_ERR_NONE) {
		dev_err(&pdev->dev, "can not open mu channel to scu: %d\n", sciErr);
		return sciErr;
	};

	register_scu_notifier(&imx8dv_rtc_alarm_sc_notifier);

error_rtc_device_register:

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int imx8dv_rtc_suspend(struct device *dev)
{
	return 0;
}

static int imx8dv_rtc_suspend_noirq(struct device *dev)
{
	return 0;
}

static int imx8dv_rtc_resume(struct device *dev)
{
	return 0;
}

static int imx8dv_rtc_resume_noirq(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops imx8dv_rtc_pm_ops = {
	.suspend = imx8dv_rtc_suspend,
	.suspend_noirq = imx8dv_rtc_suspend_noirq,
	.resume = imx8dv_rtc_resume,
	.resume_noirq = imx8dv_rtc_resume_noirq,
};

#define IMX8DV_RTC_PM_OPS	(&imx8dv_rtc_pm_ops)

#else

#define IMX8DV_RTC_PM_OPS	NULL

#endif

static const struct of_device_id imx8dv_dt_ids[] = {
	{ .compatible = "fsl,imx8dv-rtc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx8dv_dt_ids);

static struct platform_driver imx8dv_rtc_driver = {
	.driver = {
		.name	= "imx8dv_rtc",
		.pm	= IMX8DV_RTC_PM_OPS,
		.of_match_table = imx8dv_dt_ids,
	},
	.probe		= imx8dv_rtc_probe,
};
module_platform_driver(imx8dv_rtc_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Freescale i.MX8DV RTC Driver");
MODULE_LICENSE("GPL");
