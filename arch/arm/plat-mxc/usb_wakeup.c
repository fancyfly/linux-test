/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 *  The code contained herein is licensed under the GNU General Public
 *  License. You may obtain a copy of the GNU General Public License
 *  Version 2 or later at the following locations:
 *  *
 *  http://www.opensource.org/licenses/gpl-license.html
 *  http://www.gnu.org/copyleft/gpl.html
*/

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/fsl_devices.h>
#include <linux/suspend.h>
#include <mach/arc_otg.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <linux/wakelock.h>


struct wakeup_ctrl {
	int wakeup_irq;
	int usb_irq;
	struct fsl_usb2_wakeup_platform_data *pdata;
	struct task_struct *thread;
	struct completion  event;
};
static struct wakeup_ctrl *g_ctrl;

extern int usb_event_is_otg_wakeup(void);
extern void usb_debounce_id_vbus(void);
struct completion  otg_event;
static bool mhl_flag = false;
#define CHECK_MHL_TIME (msecs_to_jiffies(200)) /* 200 ms */
static bool mhl_connected = false;
static struct wake_lock detect_mhl_wake_lock;

bool is_host_mhl_connected()
{
	return mhl_connected || (!(UOG_OTGSC&(1<<8)));
}
EXPORT_SYMBOL(is_host_mhl_connected);

/*The fuction is called by mhl driver tell me the mhl host caled plug out */
void mhl_usb_connect( void )
{
	mhl_flag = false;
	complete(&otg_event);
	printk(KERN_DEBUG "FSL OTG:mhl_usb_connect\n");
}
EXPORT_SYMBOL(mhl_usb_connect);

/*The fuction is called by mhl driver tell me the mhl caled plug out */
void mhl_disconnect( void )
{
	mhl_flag = true;
	complete(&otg_event);
	printk(KERN_DEBUG "FSL OTG:mhl_disconnect\n");
}
EXPORT_SYMBOL(mhl_disconnect);

static void wakeup_clk_gate(struct fsl_usb2_wakeup_platform_data *pdata, bool on)
{
	if (pdata->usb_clock_for_pm)
		pdata->usb_clock_for_pm(on);
}

static bool usb2_is_in_lowpower(struct wakeup_ctrl *ctrl)
{
	int i;
	struct fsl_usb2_wakeup_platform_data *pdata = ctrl->pdata;
	/* all the usb module related the wakeup is in lowpower mode */
	for (i = 0; i < 3; i++) {
		if (pdata->usb_pdata[i]) {
			if (pdata->usb_pdata[i]->phy_lowpower_suspend && !pdata->usb_pdata[i]->lowpower)
				return false;
		}
	}
	return true;
}

static void delay_process_wakeup(struct wakeup_ctrl *ctrl)
{
	int i;
	struct fsl_usb2_wakeup_platform_data *pdata = ctrl->pdata;
	disable_irq_nosync(ctrl->wakeup_irq);
	if ((ctrl->usb_irq > 0) && (ctrl->wakeup_irq != ctrl->usb_irq))
		disable_irq_nosync(ctrl->usb_irq);

	for (i = 0; i < 3; i++) {
		if (pdata->usb_pdata[i]) {
			pdata->usb_pdata[i]->irq_delay = 1;
		}
	}
	pdata->usb_wakeup_is_pending = true;
	complete(&ctrl->event);
}

static irqreturn_t usb_wakeup_handler(int irq, void *_dev)
{
	struct wakeup_ctrl *ctrl = (struct wakeup_ctrl *)_dev;
	irqreturn_t ret = IRQ_NONE;

	if (usb2_is_in_lowpower(ctrl)) {
		printk(KERN_INFO "usb wakeup is here\n");
		delay_process_wakeup(ctrl);
		ret = IRQ_HANDLED;
	}
	return ret;
}

static enum usb_wakeup_event is_wakeup(struct fsl_usb2_platform_data *pdata)
{
	if (pdata->is_wakeup_event)
		return pdata->is_wakeup_event(pdata);
	else
		return WAKEUP_EVENT_INVALID;
}
#define MX53_PCBA_USB_OTG_PWR_EN	(1*32 + 4)

static void wakeup_event_handler(struct wakeup_ctrl *ctrl)
{
	struct fsl_usb2_wakeup_platform_data *pdata = ctrl->pdata;
	int already_waked = 0;
	enum usb_wakeup_event wakeup_evt;
	int i;
	u32 temp;

	//if(!mhl_flag)
	//	init_completion(&otg_event);
	mhl_flag = false;
	wakeup_clk_gate(ctrl->pdata, true);
	/* In order to get the real id/vbus value */
	if (usb_event_is_otg_wakeup())
		usb_debounce_id_vbus();

	for (i = 0; i < 3; i++) {
		struct fsl_usb2_platform_data *usb_pdata = pdata->usb_pdata[i];
		if (usb_pdata) {
			usb_pdata->irq_delay = 0;
			wakeup_evt = is_wakeup(usb_pdata);
			if (wakeup_evt != WAKEUP_EVENT_INVALID) {
				if( (wakeup_evt == WAKEUP_EVENT_ID)) {					
				//	if(usb_pdata->platform_driver_vbus)
				//		usb_pdata->platform_driver_vbus(1);
				            //we need handle only  otg cable plug in case
						if(UOG_OTGSC & OTGSC_STS_A_VBUS_VALID)
							mhl_usb_connect();
						gpio_set_value(MX53_PCBA_USB_OTG_PWR_EN, 1);
						mhl_connected=true;
						if(!strcmp(pdata->name,"DR wakeup")) {
							printk(KERN_DEBUG "wake_lock:%s\n",pdata->name);
							wake_lock(&detect_mhl_wake_lock);
						}
						/*we will wait for mhl driver until mhl driver tell me what cale plug in */
						printk(KERN_DEBUG "(wait_for_completion_interruptible ++)\n");
						wait_for_completion_interruptible(&otg_event);
						printk(KERN_DEBUG "(wait_for_completion_interruptible --OTGSC:0x%x)\n",UOG_OTGSC);
						//usb_pdata->platform_driver_vbus(0);
						gpio_set_value(MX53_PCBA_USB_OTG_PWR_EN, 0);
						if(!strcmp(pdata->name,"DR wakeup")) {
							printk(KERN_DEBUG "wake_unlock:%s\n",pdata->name);
							if (wake_lock_active(&detect_mhl_wake_lock))	
								wake_unlock(&detect_mhl_wake_lock);
						}
						/*if mhl cable insert we will clear all interrupt,USB host need keep ID interrupt */
						if(mhl_flag== true) {
							temp = UOG_OTGSC;
							temp |=(0x7f<<16);
							UOG_OTGSC = temp;
						}
						else {
							temp = UOG_OTGSC;
							temp &= ~(0x1<<16);
							temp |= (0x7e<<16);
							UOG_OTGSC = temp;
						}
						mhl_connected=false;
				}				
				if(mhl_flag == false) {
					if (usb_pdata->usb_clock_for_pm)
						usb_pdata->usb_clock_for_pm(true);
					usb_pdata->lowpower = 0;
					already_waked = 1;
					if (usb_pdata->wakeup_handler) {
						usb_pdata->wakeup_handler(usb_pdata);
					}
					init_completion(&otg_event);
				}
				if(mhl_flag== true) {
					wakeup_clk_gate(ctrl->pdata, false);
					return;
				}
			}
		}
	}
	/* If nothing to wakeup, clear wakeup event */
	if ((already_waked == 0) && pdata->usb_wakeup_exhandle)
		pdata->usb_wakeup_exhandle();

	wakeup_clk_gate(ctrl->pdata, false);
	pdata->usb_wakeup_is_pending = false;
	wake_up(&pdata->wq);
}

static int wakeup_event_thread(void *param)
{
	struct wakeup_ctrl *ctrl = (struct wakeup_ctrl *)param;
	struct sched_param sch_param = {.sched_priority = 1};

	sched_setscheduler(current, SCHED_RR, &sch_param);
	while (1) {
		wait_for_completion_interruptible(&ctrl->event);
		if (kthread_should_stop())
			break;
		wakeup_event_handler(ctrl);
		enable_irq(ctrl->wakeup_irq);
		if ((ctrl->usb_irq > 0) && (ctrl->wakeup_irq != ctrl->usb_irq))
			enable_irq(ctrl->usb_irq);
	}
	return 0;
}

static int wakeup_dev_probe(struct platform_device *pdev)
{
	struct fsl_usb2_wakeup_platform_data *pdata;
	struct wakeup_ctrl *ctrl = NULL;
	int status;

	printk(KERN_INFO "IMX usb wakeup probe\n");

	if (!pdev || !pdev->dev.platform_data)
		return -ENODEV;
	ctrl = kzalloc(sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return -ENOMEM;
	pdata = pdev->dev.platform_data;
	init_waitqueue_head(&pdata->wq);
	pdata->usb_wakeup_is_pending = false;
	if(!strcmp(pdata->name,"DR wakeup")) {
		printk(KERN_INFO "wake_lock_init:%s\n",pdata->name);
		wake_lock_init(&detect_mhl_wake_lock, WAKE_LOCK_SUSPEND, "mhl_usb_connect");
	}
	ctrl->pdata = pdata;
	init_completion(&ctrl->event);
	init_completion(&otg_event);
	ctrl->wakeup_irq = platform_get_irq(pdev, 0);
	status = request_irq(ctrl->wakeup_irq, usb_wakeup_handler, IRQF_SHARED, "usb_wakeup", (void *)ctrl);
	if (status)
		goto error1;
	ctrl->usb_irq = platform_get_irq(pdev, 1);

	ctrl->thread = kthread_run(wakeup_event_thread, (void *)ctrl, "usb_wakeup thread");
	status = IS_ERR(ctrl->thread) ? -1 : 0;
	if (status)
		goto error2;
	g_ctrl = ctrl;
	printk(KERN_DEBUG "the wakeup pdata is 0x%p\n", pdata);

	return 0;
error2:
	free_irq(ctrl->wakeup_irq, (void *)ctrl);
error1:
	kfree(ctrl);
	return status;
}

static int  wakeup_dev_exit(struct platform_device *pdev)
{
	if (g_ctrl->thread) {
		complete(&g_ctrl->event);
		kthread_stop(g_ctrl->thread);
	}
	free_irq(g_ctrl->wakeup_irq, (void *)g_ctrl);
	kfree(g_ctrl);
	return 0;
}
static struct platform_driver wakeup_d = {
	.probe   = wakeup_dev_probe,
	.remove  = wakeup_dev_exit,
	.driver = {
		.name = "usb_wakeup",
	},
};

static int __init wakeup_dev_init(void)
{
	return platform_driver_register(&wakeup_d);
}
static void __exit wakeup_dev_uninit(void)
{
	platform_driver_unregister(&wakeup_d);
}

subsys_initcall(wakeup_dev_init);
module_exit(wakeup_dev_uninit);

