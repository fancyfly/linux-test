/*
 * imx-3stack-wm8994.c  --  SoC 5.1 audio for imx_3stack
 *
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
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
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/fsl_devices.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/jack.h>
#include <mach/dma.h>
#include <mach/clock.h>

#include "imx-pcm.h"
#include "imx-ssi.h"
#include "../codecs/wm8994.h"
#include <linux/mfd/wm8994/registers.h>

#define WM8994_CODEC_MASTER	1

struct imx_3stack_priv {
	int sysclk;         /*mclk from the outside*/
	int codec_sysclk;
	int dai_hifi;
	int dai_voice;
	int dai_bt;
	struct platform_device *pdev;
	struct wm8994 *wm8994;
};
static struct imx_3stack_priv card_priv;
static struct snd_soc_jack jack;
static struct snd_soc_card snd_soc_card_imx_3stack;

static int imx_3stack_hifi_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static void imx_3stack_hifi_shutdown(struct snd_pcm_substream *substream)
{

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *machine = rtd->dai;
	struct snd_soc_dai *codec_dai = machine->codec_dai;
	struct imx_3stack_priv *priv = &card_priv;
	int ret = 0;
	unsigned int pll_out;
	pr_debug("imx_3stack_hifi_shutdown direction=%d\n", substream->stream);

	priv->dai_hifi = 0;
	pll_out = 2822400*8;
	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_MCLK1, priv->sysclk, pll_out);
	if (ret < 0)
		return;
}

static int imx_3stack_voice_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static void imx_3stack_voice_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *machine = rtd->dai;
	struct snd_soc_dai *codec_dai = machine->codec_dai;
	struct imx_3stack_priv *priv = &card_priv;
	priv->dai_voice = 0;
}

static int imx_3stack_bt_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static void imx_3stack_bt_shutdown(struct snd_pcm_substream *substream)
{
	struct imx_3stack_priv *priv = &card_priv;
	priv->dai_bt = 0;
}

static void imx_3stack_init_dam(int ssi_port, int dai_port)
{
	unsigned int ssi_ptcr = 0;
	unsigned int dai_ptcr = 0;
	unsigned int ssi_pdcr = 0;
	unsigned int dai_pdcr = 0;
	/* WM8350 uses SSI1 or SSI2 via AUDMUX port dai_port for audio */

	/* reset port ssi_port & dai_port */
	__raw_writel(0, DAM_PTCR(ssi_port));
	__raw_writel(0, DAM_PTCR(dai_port));
	__raw_writel(0, DAM_PDCR(ssi_port));
	__raw_writel(0, DAM_PDCR(dai_port));

	/* set to synchronous */
	ssi_ptcr |= AUDMUX_PTCR_SYN;
	dai_ptcr |= AUDMUX_PTCR_SYN;

#if WM8994_CODEC_MASTER
	/* set Rx sources ssi_port <--> dai_port */
	ssi_pdcr |= AUDMUX_PDCR_RXDSEL(dai_port);
	dai_pdcr |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  dai_port--> ssi_port output */
	ssi_ptcr |= AUDMUX_PTCR_TFSDIR;
	ssi_ptcr |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, dai_port);

	/* set Tx Clock direction and source dai_port--> ssi_port output */
	ssi_ptcr |= AUDMUX_PTCR_TCLKDIR;
	ssi_ptcr |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, dai_port);
#else
	/* set Rx sources ssi_port <--> dai_port */
	ssi_pdcr |= AUDMUX_PDCR_RXDSEL(dai_port);
	dai_pdcr |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  ssi_port --> dai_port output */
	dai_ptcr |= AUDMUX_PTCR_TFSDIR;
	dai_ptcr |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, ssi_port);

	/* set Tx Clock direction and source ssi_port--> dai_port output */
	dai_ptcr |= AUDMUX_PTCR_TCLKDIR;
	dai_ptcr |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, ssi_port);
#endif

	__raw_writel(ssi_ptcr, DAM_PTCR(ssi_port));
	__raw_writel(dai_ptcr, DAM_PTCR(dai_port));
	__raw_writel(ssi_pdcr, DAM_PDCR(ssi_port));
	__raw_writel(dai_pdcr, DAM_PDCR(dai_port));
}

static int imx_3stack_hifi_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *machine = rtd->dai;
	struct snd_soc_dai *cpu_dai = machine->cpu_dai;
	struct snd_soc_dai *codec_dai = machine->codec_dai;
	struct imx_3stack_priv *priv = &card_priv;
	unsigned int channels = params_channels(params);
	struct imx_ssi *ssi_mode = (struct imx_ssi *)cpu_dai->private_data;
	int ret = 0;
	unsigned int pll_out;
	u32 dai_format;
	/* only need to do this once as capture and playback are sync */

	if (priv->dai_hifi)
		return 0;
	priv->dai_hifi = 1;

	pr_debug("imx_3stack_hifi_hw_params direction =%d\n",substream->stream);
/*
	if (params_rate(params) == 8000 || params_rate(params) == 11025)
		pll_out = params_rate(params) * 512;
	else
*/
	pll_out = params_rate(params) * 256;

	ret =
	    snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_MCLK1,
				priv->sysclk, pll_out);
	if (ret < 0)
		return ret;

	ret =
	    snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1, pll_out,
				   SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	priv->codec_sysclk = pll_out;

#if WM8994_CODEC_MASTER
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM;

	ssi_mode->sync_mode = 1;
	if (channels == 1)
		ssi_mode->network_mode = 0;
	else
		ssi_mode->network_mode = 1;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_format);
	if (ret < 0)
		return ret;

	/* set i.MX active slot mask */
	snd_soc_dai_set_tdm_slot(cpu_dai,
				 channels == 1 ? 0xfffffffe : 0xfffffffc,
				 channels == 1 ? 0xfffffffe : 0xfffffffc,
				 2, 32);

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_format);
	if (ret < 0)
		return ret;
#else

	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBS_CFS;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_format);
	if (ret < 0)
		return ret;

	/* set i.MX active slot mask */
	snd_soc_dai_set_tdm_slot(cpu_dai,
				 channels == 1 ? 0xfffffffe : 0xfffffffc,
				 channels == 1 ? 0xfffffffe : 0xfffffffc,
				 2, 32);

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_format);
	if (ret < 0)
		return ret;
#endif

	/* set the SSI system clock as input (unused) */
	snd_soc_dai_set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);
	return 0;
}

static int imx_3stack_voice_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params)
{

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *machine = rtd->dai;
	struct snd_soc_dai *codec_dai = machine->codec_dai;
	struct imx_3stack_priv *priv = &card_priv;
	int ret = 0;
	unsigned int pll_out;

	/* only need to do this once as capture and playback are sync */

	if (priv->dai_voice)
		return 0;
	priv->dai_voice = 1;

	pll_out = params_rate(params) * 256;
	/* set the codec system clock */
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL2, WM8994_FLL_SRC_BCLK, 2048000, pll_out);
	if (ret < 0)
		return ret;
        	
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL2, pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	return 0;
}

static int imx_3stack_bt_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *machine = rtd->dai;
	struct snd_soc_dai *codec_dai = machine->codec_dai;
	struct imx_3stack_priv *priv = &card_priv;
	int ret = 0;

	/* only need to do this once as capture and playback are sync */

	if (priv->dai_bt)
		return 0;
	priv->dai_bt = 1;
	return 0;
}

static struct snd_soc_ops imx_3stack_hifi_ops = {
	.startup = imx_3stack_hifi_startup,
	.shutdown = imx_3stack_hifi_shutdown,
	.hw_params = imx_3stack_hifi_hw_params,
};

static struct snd_soc_ops imx_3stack_voice_ops = {
	.startup = imx_3stack_voice_startup,
	.shutdown = imx_3stack_voice_shutdown,
	.hw_params = imx_3stack_voice_hw_params,
};

static struct snd_soc_ops imx_3stack_bt_ops = {
	.hw_params = imx_3stack_bt_hw_params,
};

static void headphone_detect_handler(struct work_struct *work)
{
	struct imx_3stack_priv *priv = &card_priv;
	struct platform_device *pdev = priv->pdev;
	struct mxc_audio_platform_data *plat = pdev->dev.platform_data;
	int hp_status;
	char *envp[3];
	char *buf;
	int mic_status;

	sysfs_notify(&pdev->dev.kobj, NULL, "headphone");
	hp_status = plat->hp_status();

	if (jack.status & SND_JACK_MICROPHONE)
		mic_status = 1;
	else
		mic_status = 0;
	/* setup a message for userspace headphone in */
	buf = kmalloc(32, GFP_ATOMIC);
	if (!buf) {
		pr_err("%s kmalloc failed\n", __func__);
		return;
	}

	if (hp_status == 1 && mic_status == 1) {
		envp[0] = "NAME=headset";
		snprintf(buf, 32, "STATE=%d", 1);
	} else if (hp_status == 1 && mic_status == 0) {
		envp[0] = "NAME=headphone";
		snprintf(buf, 32, "STATE=%d", 2);
	} else {
		envp[0] = "NAME=headphone";
		snprintf(buf, 32, "STATE=%d", 0);
	}
	envp[1] = buf;
	envp[2] = NULL;
	kobject_uevent_env(&pdev->dev.kobj, KOBJ_CHANGE, envp);
	kfree(buf);

	if (hp_status)
		set_irq_type(plat->hp_irq, IRQ_TYPE_EDGE_FALLING);
	else
		set_irq_type(plat->hp_irq, IRQ_TYPE_EDGE_RISING);
	enable_irq(plat->hp_irq);
}

static DECLARE_DELAYED_WORK(hp_event, headphone_detect_handler);

static irqreturn_t imx_headphone_detect_handler(int irq, void *data)
{
	disable_irq_nosync(irq);
	schedule_delayed_work(&hp_event, msecs_to_jiffies(200));
	return IRQ_HANDLED;
}

/* 3.5 pie jack detection DAPM pins */
static struct snd_soc_jack_pin jack_pins[] = {
	{
	 .pin = "Headset Mic",
	 .mask = SND_JACK_MICROPHONE,
	 },
};

static ssize_t show_headphone(struct device_driver *dev, char *buf)
{
	struct imx_3stack_priv *priv = &card_priv;
	struct platform_device *pdev = priv->pdev;
	struct mxc_audio_platform_data *plat = pdev->dev.platform_data;
	u16 hp_status;

	/* determine whether hp is plugged in */
	hp_status = plat->hp_status();

	if (hp_status == 0)
		strcpy(buf, "speaker\n");
	else {
		if (jack.status & SND_JACK_MICROPHONE)
			strcpy(buf, "headset\n");
		else
			strcpy(buf, "headphone\n");
	}

	return strlen(buf);
}

static DRIVER_ATTR(headphone, S_IRUGO | S_IWUSR, show_headphone, NULL);

static int spk_amp_event(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol, int event)
{
	struct imx_3stack_priv *priv = &card_priv;
	struct platform_device *pdev = priv->pdev;
	struct mxc_audio_platform_data *plat = pdev->dev.platform_data;

	if (plat->amp_enable == NULL)
		return 0;

	if (SND_SOC_DAPM_EVENT_ON(event))
		plat->amp_enable(1);
	else
		plat->amp_enable(0);

	return 0;
}

/* imx_3stack card dapm widgets */
static const struct snd_soc_dapm_widget imx_3stack_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Main Mic", NULL),
	SND_SOC_DAPM_LINE("Modem voice downlink", NULL),
	SND_SOC_DAPM_LINE("FM OUT", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", spk_amp_event),
	SND_SOC_DAPM_HP("Headset Phone", NULL),
	SND_SOC_DAPM_LINE("FM IN", NULL),
	SND_SOC_DAPM_LINE("Modem voice uplink", NULL),
};

/* imx_3stack machine connections to the codec pins */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ----input ------------------- */
	/* Mic Jack --> MIC_IN (with automatic bias) */
	{"MICBIAS2", NULL, "Headset Mic"},
	{"IN1LP", NULL, "MICBIAS2"},
	{"IN1LN", NULL, "Headset Mic"},

	{"MICBIAS1", NULL, "Main Mic"},
	{"IN1RP", NULL, "MICBIAS1"},
	{"IN1RN", NULL, "Main Mic"},

	/* Line in Jack --> LINE_IN */
	{"IN2LP:VXRN", NULL, "Modem voice downlink"},
	{"IN2RP:VXRP", NULL, "Modem voice downlink"},

	{"IN2LN", NULL, "FM OUT"},
	{"IN2RN", NULL, "FM OUT"},

	/* ----output------------------- */
	/* HP_OUT --> Headphone Jack */
	{"Headset Phone", NULL, "HPOUT1L"},
	{"Headset Phone", NULL, "HPOUT1R"},

	/* LINE_OUT --> Ext Speaker */
	{"Ext Spk", NULL, "SPKOUTLP"},
	{"Ext Spk", NULL, "SPKOUTLN"},
	{"Ext Spk", NULL, "SPKOUTRP"},
	{"Ext Spk", NULL, "SPKOUTRN"},

	{"FM IN", NULL, "LINEOUT1P"},
	{"FM IN", NULL, "LINEOUT1N"},

	{"Modem voice uplink", NULL, "LINEOUT2P"},
	{"Modem voice uplink", NULL, "LINEOUT2N"},
};

static int imx_3stack_wm8994_init(struct snd_soc_codec *codec)
{
	int ret;

	snd_soc_dapm_new_controls(codec, imx_3stack_dapm_widgets,
				  ARRAY_SIZE(imx_3stack_dapm_widgets));

	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

	snd_soc_dapm_sync(codec);

	/* Headset jack detection */
	ret = snd_soc_jack_new(&snd_soc_card_imx_3stack, "Headset Jack",
			       SND_JACK_HEADSET | SND_JACK_MECHANICAL |
			       SND_JACK_AVOUT, &jack);
	if (ret)
		return ret;

	ret = snd_soc_jack_add_pins(&jack, ARRAY_SIZE(jack_pins), jack_pins);
	if (ret)
		return ret;

	wm8958_mic_detect(codec, &jack, NULL, NULL);

	return 0;
}

static struct snd_soc_dai voice_dai = {
	.name = "Voice",
	.id = 0,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
        },
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
        }, 
};

static struct snd_soc_dai bt_dai = {
	.name = "BT",
	.id = 1,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
        },
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
        }, 
};

static struct snd_soc_dai_link imx_3stack_dai[] = {
{
	.name = "WM8994 HiFi",
	.stream_name = "WM8994 HiFi",
	.codec_dai = &wm8994_dai[0],
	.init = imx_3stack_wm8994_init,
	.ops = &imx_3stack_hifi_ops,
},
{
	.name = "WM8994 Voice",
	.stream_name = "WM8994 Voice",
	.cpu_dai = &voice_dai,
	.codec_dai = &wm8994_dai[1],
	.ops = &imx_3stack_voice_ops,
},
{
	.name = "WM8994 BT",
	.stream_name = "WM8994 BT",
	.cpu_dai = &bt_dai,
	.codec_dai = &wm8994_dai[2],
	.ops = &imx_3stack_bt_ops,
}
};

static int imx_3stack_card_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	kfree(socdev->codec_data);
	return 0;
}

static struct snd_soc_card snd_soc_card_imx_3stack = {
	.name = "imx-3stack",
	.platform = &imx_soc_platform,
	.dai_link = imx_3stack_dai,
	.num_links = ARRAY_SIZE(imx_3stack_dai),
	.remove = imx_3stack_card_remove,
};

static struct snd_soc_device imx_3stack_snd_devdata = {
	.card = &snd_soc_card_imx_3stack,
	.codec_dev = &soc_codec_dev_wm8994,
};

/*
 * This function will register the snd_soc_pcm_link drivers.
 */
static int __devinit imx_3stack_wm8994_probe(struct platform_device *pdev)
{

	struct mxc_audio_platform_data *plat = pdev->dev.platform_data;
	struct imx_3stack_priv *priv = &card_priv;
	struct snd_soc_dai *wm8994_cpu_dai = 0;
	struct wm8994 *wm8994 = plat->priv;
	int ret = 0;

	priv->pdev = pdev;
	priv->wm8994 = wm8994;

	gpio_activate_audio_ports();
	imx_3stack_init_dam(plat->src_port, plat->ext_port);

	if (plat->src_port == 2)
		wm8994_cpu_dai = imx_ssi_dai[2];
	else if (plat->src_port == 1)
		wm8994_cpu_dai = imx_ssi_dai[0];
	else if (plat->src_port == 7)
		wm8994_cpu_dai = imx_ssi_dai[4];

	imx_3stack_dai[0].cpu_dai = wm8994_cpu_dai;

	/* get mxc_audio_platform_data for pcm */
	imx_3stack_dai[0].cpu_dai->dev = &pdev->dev;
	imx_3stack_dai[1].cpu_dai->dev = &pdev->dev;
	imx_3stack_dai[2].cpu_dai->dev = &pdev->dev;
    	
	ret = driver_create_file(pdev->dev.driver, &driver_attr_headphone);
	if (ret < 0) {
		pr_err("%s:failed to create driver_attr_headphone\n", __func__);
		goto sysfs_err;
	}

	ret = -EINVAL;
	if (plat->init && plat->init())
		goto err_plat_init;

	priv->sysclk = plat->sysclk;

	if (plat->hp_status())
		ret = request_irq(plat->hp_irq,
				  imx_headphone_detect_handler,
				  IRQ_TYPE_EDGE_FALLING, pdev->name, priv);
	else
		ret = request_irq(plat->hp_irq,
				  imx_headphone_detect_handler,
				  IRQ_TYPE_EDGE_RISING, pdev->name, priv);

	if (ret < 0) {
		pr_err("%s: request irq failed\n", __func__);
		goto err_card_reg;
	}

	return 0;

err_card_reg:
	if (plat->finit)
		plat->finit();
err_plat_init:
	driver_remove_file(pdev->dev.driver, &driver_attr_headphone);
sysfs_err:
	return ret;
}

static int __devexit imx_3stack_wm8994_remove(struct platform_device *pdev)
{
	struct mxc_audio_platform_data *plat = pdev->dev.platform_data;
	struct imx_3stack_priv *priv = &card_priv;

	free_irq(plat->hp_irq, priv);

	if (plat->finit)
		plat->finit();

	driver_remove_file(pdev->dev.driver, &driver_attr_headphone);
	return 0;
}

static struct platform_driver imx_3stack_wm8994_driver = {
	.probe = imx_3stack_wm8994_probe,
	.remove = __devexit_p(imx_3stack_wm8994_remove),
	.driver = {
		   .name = "imx-3stack-wm8994",
		   .owner = THIS_MODULE,
		   },
};

static struct platform_device *imx_3stack_snd_device;

static int __init imx_3stack_asoc_init(void)
{
	int ret;
	/* register voice DAI here */
	ret = snd_soc_register_dai(&voice_dai);
	if (ret)
		return ret;
	
	/* register voice DAI here */
	ret = snd_soc_register_dai(&bt_dai);
	if (ret)
		return ret;

	ret = platform_driver_register(&imx_3stack_wm8994_driver);
	if (ret < 0)
		goto exit;

	imx_3stack_snd_device = platform_device_alloc("soc-audio", 5);
	if (!imx_3stack_snd_device)
		goto err_device_alloc;

	platform_set_drvdata(imx_3stack_snd_device, &imx_3stack_snd_devdata);
	imx_3stack_snd_devdata.dev = &imx_3stack_snd_device->dev;
	ret = platform_device_add(imx_3stack_snd_device);

	if (0 == ret)
		goto exit;

	platform_device_put(imx_3stack_snd_device);

err_device_alloc:
	platform_driver_unregister(&imx_3stack_wm8994_driver);
exit:
	return ret;
}

static void __exit imx_3stack_asoc_exit(void)
{
	platform_driver_unregister(&imx_3stack_wm8994_driver);
	platform_device_unregister(imx_3stack_snd_device);
}

module_init(imx_3stack_asoc_init);
module_exit(imx_3stack_asoc_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC wm8994 imx_3stack");
MODULE_LICENSE("GPL");
