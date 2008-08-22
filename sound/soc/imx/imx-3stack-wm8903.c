/*
 * imx-3stack-wm8903.c  --  i.MX 3Stack Driver for Wolfson WM8903 Codec
 *
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Based on imx-3stack-wm8350 (c) 2007 Wolfson Microelectronics PLC.

 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <asm/arch/dma.h>
#include <asm/arch/spba.h>
#include <asm/arch/clock.h>
#include <asm/arch/mxc.h>
#include <asm/arch/gpio.h>

#include "imx-ssi.h"
#include "imx-pcm.h"
#include "../codecs/wm8903.h"

static void headphone_detect_handler(struct work_struct *work);
static DECLARE_WORK(hp_event, headphone_detect_handler);
void gpio_activate_audio_ports(void);
void wm8903_hp_initialize_hp_detect(struct snd_soc_codec *codec);
int wm8903_hp_status(struct snd_soc_codec *codec);

/* SSI BCLK and LRC master */
#define WM8903_SSI_MASTER	1

/* WM8903's interrupt out is connected to
   MX51_PIN_EIM_A26 = gpio2 GPIO[20] */
#define WM8903_IRQ_GPIO_PORT	1
#define WM8903_IRQ_GPIO_NUM		20

static int imx_3stack_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_dai *cpu_dai = pcm_link->cpu_dai;
	struct snd_soc_dai *codec_dai = pcm_link->codec_dai;
	unsigned int channels = params_channels(params);
	u32 dai_format;

#if WM8903_SSI_MASTER
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_SYNC;
#else
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_SYNC;
#endif
	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	codec_dai->ops->set_fmt(codec_dai, dai_format);

	/* set cpu DAI configuration */
	cpu_dai->ops->set_fmt(cpu_dai, dai_format);

	/* set i.MX active slot mask */
	cpu_dai->ops->set_tdm_slot(cpu_dai,
				   channels == 1 ? 0xfffffffe : 0xfffffffc,
				   channels);

	/* set the SSI system clock as input (unused) */
	cpu_dai->ops->set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);

	/* MCLK is 12MHz */
	codec_dai->ops->set_sysclk(codec_dai, 0, 12000000, 0);

	return 0;
}

/*
 * imx_3stack WM8903 HiFi DAI operations.
 */
static struct snd_soc_ops imx_3stack_hifi_ops = {
	.hw_params = imx_3stack_hifi_hw_params,
};

static void imx_3stack_init_dam(int ssi_port, int dai_port)
{
	/* WM8903 uses SSI2 via AUDMUX port dai_port for audio */

	/* reset port ssi_port & dai_port */
	DAM_PTCR(ssi_port) = 0;
	DAM_PDCR(ssi_port) = 0;
	DAM_PTCR(dai_port) = 0;
	DAM_PDCR(dai_port) = 0;

	/* set to synchronous */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_SYN;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_SYN;

#if WM8903_SSI_MASTER
	/* set Rx sources ssi_port <--> dai_port */
	DAM_PDCR(ssi_port) |= AUDMUX_PDCR_RXDSEL(dai_port);
	DAM_PDCR(dai_port) |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  dai_port--> ssi_port output */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TFSDIR;
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, dai_port);

	/* set Tx Clock direction and source dai_port--> ssi_port output */
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TCLKDIR;
	DAM_PTCR(ssi_port) |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, dai_port);
#else
	/* set Rx sources ssi_port <--> dai_port */
	DAM_PDCR(ssi_port) |= AUDMUX_PDCR_RXDSEL(dai_port);
	DAM_PDCR(dai_port) |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  ssi_port --> dai_port output */
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TFSDIR;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, ssi_port);

	/* set Tx Clock direction and source ssi_port--> dai_port output */
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TCLKDIR;
	DAM_PTCR(dai_port) |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, ssi_port);
#endif
}

static int imx_3stack_pcm_new(struct snd_soc_pcm_link *pcm_link)
{
	int ret;

	pcm_link->audio_ops = &imx_3stack_hifi_ops;

	ret = snd_soc_pcm_new(pcm_link, 1, 1);
	if (ret < 0) {
		pr_err("%s: failed to create hifi pcm\n", __func__);
		return ret;
	}

	return 0;
}

static const struct snd_soc_pcm_link_ops imx_3stack_pcm_ops = {
	.new = imx_3stack_pcm_new,
};

/* imx_3stack machine dapm widgets */
static const struct snd_soc_dapm_widget imx_3stack_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
	SND_SOC_DAPM_LINE("Line Out Jack", NULL),
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker Jack", NULL),
};

/* imx_3stack machine audio map */
static const char *audio_map[][3] = {
	/* Mic Jack --> IN1L and IN2L (with automatic bias) */
	{"IN1L", NULL, "Mic Bias"},
	{"IN2L", NULL, "Mic Jack"},
	{"Mic Bias", NULL, "Mic Jack"},

	/* WM8903 Line in Jack --> AUX (L+R) */
	{"IN3R", NULL, "Line In Jack"},
	{"IN3L", NULL, "Line In Jack"},

	/* WM8903 HPOUTS --> Headphone Jack */
	{"Headphone Jack", NULL, "HPOUTR"},
	{"Headphone Jack", NULL, "HPOUTL"},

	/* WM8903 LINEOUTS--> Line Out Jack */
	{"Line Out Jack", NULL, "LINEOUTR"},
	{"Line Out Jack", NULL, "LINEOUTL"},

	/* WM8903 Speaker --> Speaker Jack */
	{"Speaker Jack", NULL, "LOP"},
	{"Speaker Jack", NULL, "LON"},
	{"Speaker Jack", NULL, "ROP"},
	{"Speaker Jack", NULL, "RON"},

	{NULL, NULL, NULL},
};

#ifdef CONFIG_PM
static int imx_3stack_audio_suspend(struct platform_device *dev,
				    pm_message_t state)
{
	return 0;
}

static int imx_3stack_audio_resume(struct platform_device *dev)
{
	return 0;
}

#else
#define imx_3stack_audio_suspend	NULL
#define imx_3stack_audio_resume	NULL
#endif

static void *wm8903_3stack_pcm_link;

static void headphone_detect_handler(struct work_struct *work)
{
	struct snd_soc_pcm_link *pcm_link;
	struct snd_soc_codec *codec;
	struct snd_soc_machine *machine;
	u16 hp_status;

	pcm_link = (struct snd_soc_pcm_link *)wm8903_3stack_pcm_link;
	codec = pcm_link->codec;
	machine = pcm_link->machine;

	/* debounce for 200ms */
	schedule_timeout_interruptible(msecs_to_jiffies(200));

	/* determine whether hp is plugged in and clear interrupt status. */
	hp_status = wm8903_hp_status(codec);

	sysfs_notify(&machine->pdev->dev.driver->kobj, NULL, "headphone");
}

static irqreturn_t imx_headphone_detect_handler(int irq, void *dev_id)
{
	schedule_work(&hp_event);
	return IRQ_HANDLED;
}

static ssize_t show_headphone(struct device_driver *dev, char *buf)
{
	u16 hp_status;
	struct snd_soc_pcm_link *pcm_link;
	struct snd_soc_codec *codec;

	pcm_link = (struct snd_soc_pcm_link *)wm8903_3stack_pcm_link;
	codec = pcm_link->codec;

	/* determine whether hp is plugged in */
	hp_status = wm8903_hp_status(codec);

	if (hp_status == 0)
		strcpy(buf, "speaker\n");
	else
		strcpy(buf, "headphone\n");

	return strlen(buf);
}

DRIVER_ATTR(headphone, S_IRUGO | S_IWUSR, show_headphone, NULL);

int mach_probe(struct snd_soc_machine *machine)
{
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;
	int i, ret;

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);

	wm8903_3stack_pcm_link = pcm_link;

	codec = pcm_link->codec;

	gpio_activate_audio_ports();
	codec->ops->io_probe(codec, machine);

	/* set unused imx_3stack WM8903 codec pins */
	snd_soc_dapm_set_endpoint(machine, "IN2R", 0);
	snd_soc_dapm_set_endpoint(machine, "IN2L", 0);

	/* Add imx_3stack specific widgets */
	for (i = 0; i < ARRAY_SIZE(imx_3stack_dapm_widgets); i++) {
		snd_soc_dapm_new_control(machine, codec,
					 &imx_3stack_dapm_widgets[i]);
	}

	/* set up imx_3stack specific audio path audio map */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(machine, audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);
	}

	/* connect and enable all input jacks (for now) */
	snd_soc_dapm_set_endpoint(machine, "Mic Jack", 1);
	snd_soc_dapm_set_endpoint(machine, "Line In Jack", 1);

	snd_soc_dapm_set_policy(machine, SND_SOC_DAPM_POLICY_STREAM);
	snd_soc_dapm_sync_endpoints(machine);

	wm8903_hp_initialize_hp_detect(codec);
	wm8903_hp_status(codec);

	gpio_config(WM8903_IRQ_GPIO_PORT, WM8903_IRQ_GPIO_NUM,
		    false, GPIO_INT_RISE_EDGE);
	gpio_request_irq(WM8903_IRQ_GPIO_PORT, WM8903_IRQ_GPIO_NUM,
			 GPIO_LOW_PRIO,
			 imx_headphone_detect_handler, 0,
			 "headphone", pcm_link);

	/* register card with ALSA upper layers */
	ret = snd_soc_register_card(machine);
	if (ret < 0) {
		pr_err("%s: failed to register sound card\n", __func__);
		return ret;
	}

	return 0;
}

struct snd_soc_machine_ops machine_ops = {
	.mach_probe = mach_probe,
};

static int __init imx_3stack_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_machine *machine;
	struct mxc_audio_platform_data *dev_data = pdev->dev.platform_data;
	struct snd_soc_pcm_link *hifi;
	const char *ssi_port;
	int ret;

	machine = kzalloc(sizeof(struct snd_soc_machine), GFP_KERNEL);
	if (machine == NULL)
		return -ENOMEM;

	machine->owner = THIS_MODULE;
	machine->pdev = pdev;
	machine->name = "i.MX_3STACK";
	machine->longname = "WM8903";
	machine->ops = &machine_ops;
	pdev->dev.driver_data = machine;

	/* register card */
	ret = snd_soc_new_card(machine, 1, SNDRV_DEFAULT_IDX1,
			       SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		pr_err("%s: failed to create stereo sound card\n", __func__);
		goto err;
	}

	if (dev_data->src_port == 2)
		ssi_port = imx_ssi_3;
	else
		ssi_port = imx_ssi_1;

	hifi = snd_soc_pcm_link_new(machine, "imx_3stack-hifi",
				    &imx_3stack_pcm_ops, imx_pcm,
				    "wm8903-codec", "wm8903-hifi-dai",
				    ssi_port);

	if (hifi == NULL) {
		pr_err("Failed to create HiFi PCM link\n");
		snd_soc_machine_free(machine);
		goto err;
	}

	ret = snd_soc_pcm_link_attach(hifi);
	hifi->private_data = dev_data;
	if (ret < 0)
		goto link_err;

	imx_3stack_init_dam(dev_data->src_port, dev_data->ext_port);

	ret = driver_create_file(pdev->dev.driver, &driver_attr_headphone);
	if (ret < 0)
		goto sysfs_err;

	return ret;

sysfs_err:
	driver_remove_file(pdev->dev.driver, &driver_attr_headphone);

link_err:
	snd_soc_machine_free(machine);

err:
	kfree(machine);
	return ret;
}

static int __devexit imx_3stack_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_machine *machine = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	struct snd_soc_pcm_link *pcm_link;

	gpio_free_irq(WM8903_IRQ_GPIO_PORT, WM8903_IRQ_GPIO_NUM, GPIO_LOW_PRIO);

	pcm_link = list_first_entry(&machine->active_list,
				    struct snd_soc_pcm_link, active_list);

	codec = pcm_link->codec;
	codec->ops->io_remove(codec, machine);

	snd_soc_machine_free(machine);

	driver_remove_file(pdev->dev.driver, &driver_attr_headphone);
	kfree(machine);

	return 0;
}

const char imx_3stack_audio[32] = {
	"imx-3stack-wm8903"
};

static struct platform_driver imx_3stack_wm8903_audio_driver = {
	.probe = imx_3stack_audio_probe,
	.remove = __devexit_p(imx_3stack_audio_remove),
	.suspend = imx_3stack_audio_suspend,
	.resume = imx_3stack_audio_resume,
	.driver = {
		   .name = imx_3stack_audio,
		   .owner = THIS_MODULE,
		   },
};

static int __init imx_3stack_asoc_init(void)
{
	return platform_driver_register(&imx_3stack_wm8903_audio_driver);
}

static void __exit imx_3stack_asoc_exit(void)
{
	platform_driver_unregister(&imx_3stack_wm8903_audio_driver);
}

module_init(imx_3stack_asoc_init);
module_exit(imx_3stack_asoc_exit);

MODULE_DESCRIPTION("ALSA SoC WM8903 Driver for i.MX 3STACK");
MODULE_LICENSE("GPL");
