/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/control.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/pinctrl/consumer.h>
#include "fsl_sai.h"

#define SUPPORT_RATE_NUM 10

struct imx_xtor_data {
	struct snd_soc_dai_link dai[3];
	struct snd_soc_card card;
	bool  is_stream_opened[2];
	struct platform_device *asrc_pdev;
	u32 asrc_rate;
	u32 asrc_format;
};

static int imx_xtor_startup(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	static struct snd_pcm_hw_constraint_list constraint_rates;
	static u32 support_rates[SUPPORT_RATE_NUM];
	int ret;


	support_rates[0] = 8000;
	support_rates[1] = 32000;
	support_rates[2] = 48000;
	support_rates[3] = 96000;
	support_rates[4] = 192000;
	constraint_rates.list = support_rates;
	constraint_rates.count = 5;

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
						&constraint_rates);
	if (ret)
		return ret;

	return 0;
}

static int imx_xtor_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_card *card = rtd->card;
	struct device *dev = card->dev;
	int ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
			SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS);
	if (ret) {
		dev_err(dev, "failed to set cpu dai fmt: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_OUT);
	if (ret) {
		dev_err(dev, "failed to set cpu sysclk: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0, 0, 2, params_width(params));
	if (ret) {
		dev_err(dev, "failed to set cpu dai tdm slot: %d\n", ret);
		return ret;
	}

	return 0;
}

static int imx_xtor_hw_free(struct snd_pcm_substream *substream)
{
	return 0;
}

static void imx_xtor_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct imx_xtor_data *data = snd_soc_card_get_drvdata(card);
	bool tx = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;

	data->is_stream_opened[tx] = false;
}

static int be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_card *card = rtd->card;
	struct imx_xtor_data *data = snd_soc_card_get_drvdata(card);
	struct snd_interval *rate;
	struct snd_mask *mask;

	if (!data->asrc_pdev)
		return -EINVAL;

	rate = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
	rate->max = rate->min = data->asrc_rate;

	mask = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
	snd_mask_none(mask);
	snd_mask_set(mask, data->asrc_format);

	return 0;
}

static const struct snd_soc_dapm_route audio_map[] = {
	/* Line out jack */
	{"CPU-Playback",  NULL, "ASRC-Playback"},
	{"Playback",  NULL, "CPU-Playback"},/* dai route for be and fe */
	{"ASRC-Capture",  NULL, "CPU-Capture"},
	{"CPU-Capture",  NULL, "Capture"},
};


static struct snd_soc_ops imx_xtor_ops = {
	.startup = imx_xtor_startup,
	.shutdown  = imx_xtor_shutdown,
	.hw_params = imx_xtor_hw_params,
	.hw_free = imx_xtor_hw_free,
};

static struct snd_soc_ops imx_xtor_be_ops = {
	.hw_params = imx_xtor_hw_params,
	.hw_free = imx_xtor_hw_free,
};

static int imx_xtor_probe(struct platform_device *pdev)
{
	struct device_node *cpu_np, *xtor_np = NULL;
	struct device_node *asrc_np = NULL;
	struct platform_device *asrc_pdev = NULL;
	struct platform_device *cpu_pdev;
	struct imx_xtor_data *data;
	int ret;
	u32 width;

	cpu_np = of_parse_phandle(pdev->dev.of_node, "cpu-dai", 0);
	if (!cpu_np) {
		dev_err(&pdev->dev, "cpu dai phandle missing or invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto fail;
	}

	asrc_np = of_parse_phandle(pdev->dev.of_node, "asrc-controller", 0);
	if (asrc_np) {
		asrc_pdev = of_find_device_by_node(asrc_np);
		data->asrc_pdev = asrc_pdev;
	}

	cpu_pdev = of_find_device_by_node(cpu_np);
	if (!cpu_pdev) {
		dev_err(&pdev->dev, "failed to find SAI platform device\n");
		ret = -EINVAL;
		goto fail;
	}

	data->dai[0].name = "xtor hifi";
	data->dai[0].stream_name = "xtor hifi";
	data->dai[0].codec_dai_name = "snd-soc-dummy-dai";
	data->dai[0].codec_name = "snd-soc-dummy";
	data->dai[0].cpu_dai_name = dev_name(&cpu_pdev->dev);
	data->dai[0].platform_of_node = cpu_np;
	data->dai[0].ops = &imx_xtor_ops;
	data->dai[0].playback_only = false;
	data->dai[0].capture_only = false;
	data->dai[0].dai_fmt = SND_SOC_DAIFMT_I2S |
			    SND_SOC_DAIFMT_NB_NF |
			    SND_SOC_DAIFMT_CBS_CFS;
	data->card.num_links = 1;
	data->card.dai_link = data->dai;

	/*if there is no asrc controller, we only enable one device*/
	if (asrc_pdev) {
		data->dai[1].name = "HiFi-ASRC-FE";
		data->dai[1].stream_name = "HiFi-ASRC-FE";
		data->dai[1].codec_dai_name = "snd-soc-dummy-dai";
		data->dai[1].codec_name = "snd-soc-dummy";
		data->dai[1].cpu_of_node    = asrc_np;
		data->dai[1].platform_of_node   = asrc_np;
		data->dai[1].dynamic   = 1;
		data->dai[1].dpcm_playback  = 1;
		data->dai[1].dpcm_capture   = 1;

		data->dai[2].name = "HiFi-ASRC-BE";
		data->dai[2].stream_name = "HiFi-ASRC-BE";
		data->dai[2].codec_dai_name  = "snd-soc-dummy-dai";
		data->dai[2].codec_name      = "snd-soc-dummy";
		data->dai[2].cpu_of_node     = cpu_np;
		data->dai[2].platform_name   = "snd-soc-dummy";
		data->dai[2].no_pcm   = 1;
		data->dai[2].dpcm_playback  = 1;
		data->dai[2].dpcm_capture   = 1;
		data->dai[2].ops = &imx_xtor_be_ops,
		data->dai[2].be_hw_params_fixup = be_hw_params_fixup,
		data->card.num_links = 3;
		data->card.dai_link = &data->dai[0];

		ret = of_property_read_u32(asrc_np, "fsl,asrc-rate",
						&data->asrc_rate);
		if (ret) {
			dev_err(&pdev->dev, "failed to get output rate\n");
			ret = -EINVAL;
			goto fail;
		}

		ret = of_property_read_u32(asrc_np, "fsl,asrc-width", &width);
		if (ret) {
			dev_err(&pdev->dev, "failed to get output rate\n");
			ret = -EINVAL;
			goto fail;
		}

		if (width == 24)
			data->asrc_format = SNDRV_PCM_FORMAT_S24_LE;
		else
			data->asrc_format = SNDRV_PCM_FORMAT_S16_LE;
	}

	data->card.dapm_routes = audio_map,
	data->card.num_dapm_routes = ARRAY_SIZE(audio_map),
	data->card.dev = &pdev->dev;
	data->card.owner = THIS_MODULE;
	ret = snd_soc_of_parse_card_name(&data->card, "model");
	if (ret)
		goto fail;

	platform_set_drvdata(pdev, &data->card);
	snd_soc_card_set_drvdata(&data->card, data);
	ret = devm_snd_soc_register_card(&pdev->dev, &data->card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
		goto fail;
	}

fail:
	if (cpu_np)
		of_node_put(cpu_np);
	if (xtor_np)
		of_node_put(xtor_np);
	return ret;
}

static const struct of_device_id imx_xtor_dt_ids[] = {
	{ .compatible = "fsl,imx-audio-xtor", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_xtor_dt_ids);

static struct platform_driver imx_xtor_driver = {
	.driver = {
		.name = "imx-xtor",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = imx_xtor_dt_ids,
	},
	.probe = imx_xtor_probe,
};
module_platform_driver(imx_xtor_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Freescale i.MX Dummy audio ASoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:imx-xtor");
