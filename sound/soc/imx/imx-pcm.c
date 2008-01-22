/*
 * imx-pcm.c -- ALSA SoC interface for the Freescale i.MX3 CPU's
 *
 * Copyright 2006 Wolfson Microelectronics PLC.
 * Author: Liam Girdwood
 *         liam.girdwood@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * Based on imx31-pcm.c by	Nicolas Pitre, (C) 2004 MontaVista Software, Inc.
 * and on mxc-alsa-mc13783 (C) 2006 Freescale.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <asm/arch/dma.h>
#include <asm/arch/spba.h>
#include <asm/arch/clock.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>

#include "imx-pcm.h"

/* debug */
#define IMX_PCM_DEBUG 0
#if IMX_PCM_DEBUG
#define dbg(format, arg...) printk(format, ## arg)
#else
#define dbg(format, arg...)
#endif

/*
 * Coherent DMA memory is used by default, although Freescale have used 
 * bounce buffers in all their drivers for i.MX31 to date. If you have any 
 * issues, please select bounce buffers. 
 */
#define IMX31_DMA_BOUNCE 0

static const struct snd_pcm_hardware imx_pcm_hardware = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	.buffer_bytes_max = 32 * 1024,
	.period_bytes_min = 64,
	.period_bytes_max = 8 * 1024,
	.periods_min = 2,
	.periods_max = 255,
	.fifo_size = 0,
};

struct mxc_runtime_data {
	int dma_ch;
	struct imx_pcm_dma_param *dma_params;
	spinlock_t dma_lock;
	int active, period, periods;
	int dma_wchannel;
	int dma_active;
	int old_offset;
	int dma_alloc;
};

static void audio_stop_dma(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	unsigned long flags;
#if IMX31_DMA_BOUNCE
	unsigned int dma_size;
	unsigned int offset;

	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * prtd->periods;
#endif

	/* stops the dma channel and clears the buffer ptrs */
	spin_lock_irqsave(&prtd->dma_lock, flags);
	prtd->active = 0;
	prtd->period = 0;
	prtd->periods = 0;
	mxc_dma_disable(prtd->dma_wchannel);

#if IMX31_DMA_BOUNCE
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
				 DMA_TO_DEVICE);
	else
		dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
				 DMA_FROM_DEVICE);
#endif
	spin_unlock_irqrestore(&prtd->dma_lock, flags);
}

static int dma_new_period(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	unsigned int dma_size = frames_to_bytes(runtime, runtime->period_size);
	unsigned int offset = dma_size * prtd->period;
	int ret = 0;
	mxc_dma_requestbuf_t sdma_request;

	if (!prtd->active)
		return 0;
	//printk(KERN_EMERG"In func %s \n",__FUNCTION__);               
	memset(&sdma_request, 0, sizeof(mxc_dma_requestbuf_t));

	dbg("period pos  ALSA %x DMA %x\n", runtime->periods, prtd->period);
	dbg("period size ALSA %x DMA %x Offset %x dmasize %x\n",
	    (unsigned int)runtime->period_size, runtime->dma_bytes,
	    offset, dma_size);
	dbg("DMA addr %x\n", runtime->dma_addr + offset);

#if IMX31_DMA_BOUNCE
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		sdma_request.src_addr = (dma_addr_t) (dma_map_single(NULL,
								     runtime->
								     dma_area +
								     offset,
								     dma_size,
								     DMA_TO_DEVICE));
	else
		sdma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL,
								     runtime->
								     dma_area +
								     offset,
								     dma_size,
								     DMA_FROM_DEVICE));
#else
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		sdma_request.src_addr =
		    (dma_addr_t) (runtime->dma_addr + offset);
	else
		sdma_request.dst_addr =
		    (dma_addr_t) (runtime->dma_addr + offset);

#endif
	sdma_request.num_of_bytes = dma_size;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		mxc_dma_config(prtd->dma_wchannel, &sdma_request, 1,
			       MXC_DMA_MODE_WRITE);
		ret = mxc_dma_enable(prtd->dma_wchannel);
	} else {

		mxc_dma_config(prtd->dma_wchannel, &sdma_request, 1,
			       MXC_DMA_MODE_READ);
		ret = mxc_dma_enable(prtd->dma_wchannel);
	}
	prtd->dma_active = 1;
	prtd->period++;
	prtd->period %= runtime->periods;

	return ret;
}

static void audio_dma_irq(void *data)
{
	struct snd_pcm_substream *substream = (struct snd_pcm_substream *)data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
#if IMX31_DMA_BOUNCE
	unsigned int dma_size = frames_to_bytes(runtime, runtime->period_size);
	unsigned int offset = dma_size * prtd->periods;
#endif

	//printk(KERN_EMERG"In func %s \n",__FUNCTION__);               
	prtd->dma_active = 0;
	prtd->periods++;
	prtd->periods %= runtime->periods;

	dbg("irq per %d offset %x\n", prtd->periods,
	    frames_to_bytes(runtime, runtime->period_size) * prtd->periods);
#if IMX31_DMA_BOUNCE
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
				 DMA_TO_DEVICE);
	else
		dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
				 DMA_FROM_DEVICE);

#endif

	if (prtd->active)
		snd_pcm_period_elapsed(substream);
	dma_new_period(substream);
}

static int imx_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;

	prtd->period = 0;
	prtd->periods = 0;
	return 0;
}

static int imx_pcm_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_link *pcm_link = substream->private_data;
	struct snd_soc_dai *cpu_dai = pcm_link->cpu_dai;
	struct mxc_pcm_dma_params *dma = cpu_dai->dma_data;
	int ret = 0, channel = 0;

	/* only allocate the DMA chn once */
	if (!prtd->dma_alloc) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

			channel =
			    mxc_dma_request(MXC_DMA_SSI1_16BIT_TX0,
					    "ALSA TX SDMA");
			if (channel < 0) {
				printk(KERN_ERR
				       "imx-pcm: error requesting a write dma channel\n");
				return channel;
			}
			ret = mxc_dma_callback_set(channel, (mxc_dma_callback_t)
						   audio_dma_irq,
						   (void *)substream);

		} else {
			channel =
			    mxc_dma_request(MXC_DMA_SSI1_16BIT_RX0,
					    "ALSA RX SDMA");
			if (channel < 0) {
				printk(KERN_ERR
				       "imx-pcm: error requesting a read dma channel\n");
				return channel;
			}
		}
		prtd->dma_wchannel = channel;
		prtd->dma_alloc = 1;

		/* set up chn with params */
		//      dma->params.callback = audio_dma_irq;
		dma->params.arg = substream;

		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			dma->params.word_size = TRANSFER_16BIT;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
		case SNDRV_PCM_FORMAT_S24_LE:
			dma->params.word_size = TRANSFER_24BIT;
			break;
		}

	}
#if IMX31_DMA_BOUNCE
	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0) {
		printk(KERN_ERR "imx-pcm: failed to malloc pcm pages\n");
		if (channel)
			mxc_dma_free(channel);
		return ret;
	}
	runtime->dma_addr = virt_to_phys(runtime->dma_area);
#else
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
#endif
	return ret;
}

static int imx_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;

	if (prtd->dma_wchannel) {
		mxc_dma_free(prtd->dma_wchannel);
		prtd->dma_wchannel = 0;
		prtd->dma_alloc = 0;
	}
#if IMX31_DMA_BOUNCE
	snd_pcm_lib_free_pages(substream);
#endif
	return 0;
}

static int imx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct mxc_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		prtd->dma_active = 0;
		/* requested stream startup */
		prtd->active = 1;
		ret = dma_new_period(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		/* requested stream shutdown */
		audio_stop_dma(substream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		prtd->active = 0;
		prtd->periods = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		prtd->active = 1;
		prtd->dma_active = 0;
		ret = dma_new_period(substream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->active = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->active = 1;
		if (prtd->old_offset) {
			prtd->dma_active = 0;
			ret = dma_new_period(substream);
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static snd_pcm_uframes_t imx_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	unsigned int offset = 0;

	/* is a transfer active ? */
	if (prtd->dma_active) {
		offset = (runtime->period_size * (prtd->periods)) +
		    (runtime->period_size >> 1);
		if (offset >= runtime->buffer_size)
			offset = runtime->period_size >> 1;
	} else {
		offset = (runtime->period_size * (prtd->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
	}
	dbg("pointer offset %x\n", offset);

	return offset;
}

static int imx_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd;
	int ret;

	snd_soc_set_runtime_hwparams(substream, &imx_pcm_hardware);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		return ret;

	prtd = kzalloc(sizeof(struct mxc_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	runtime->private_data = prtd;
	return 0;
}

static int imx_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;

	kfree(prtd);
	return 0;
}

static int
imx_pcm_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr, runtime->dma_bytes);
}

struct snd_pcm_ops imx_pcm_ops = {
	.open = imx_pcm_open,
	.close = imx_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = imx_pcm_hw_params,
	.hw_free = imx_pcm_hw_free,
	.prepare = imx_pcm_prepare,
	.trigger = imx_pcm_trigger,
	.pointer = imx_pcm_pointer,
	.mmap = imx_pcm_mmap,
};

static int imx_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = imx_pcm_hardware.buffer_bytes_max;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;

	buf->bytes = size;
	return 0;
}

static void imx_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 imx_pcm_dmamask = 0xffffffff;

static int imx_pcm_new(struct snd_soc_platform *platform,
		       struct snd_card *card, int playback, int capture,
		       struct snd_pcm *pcm)
{
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &imx_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;
#if IMX31_DMA_BOUNCE
	ret = snd_pcm_lib_preallocate_pages_for_all(pcm,
						    SNDRV_DMA_TYPE_CONTINUOUS,
						    snd_dma_continuous_data
						    (GFP_KERNEL),
						    imx_pcm_hardware.
						    buffer_bytes_max * 2,
						    imx_pcm_hardware.
						    buffer_bytes_max * 2);
	if (ret < 0) {
		printk(KERN_ERR "imx-pcm: failed to preallocate pages\n");
		goto out;
	}
#else
	if (playback) {
		ret = imx_pcm_preallocate_dma_buffer(pcm,
						     SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (capture) {
		ret = imx_pcm_preallocate_dma_buffer(pcm,
						     SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
#endif
      out:
	return ret;
}

static const struct snd_soc_platform_ops imx_platform_ops = {
	.pcm_new = imx_pcm_new,
#if IMX31_DMA_BOUNCE
	.pcm_free = NULL,
#else
	.pcm_free = imx_pcm_free_dma_buffers,
#endif
};

static int imx_pcm_probe(struct device *dev)
{
	struct snd_soc_platform *platform = to_snd_soc_platform(dev);

	platform->pcm_ops = &imx_pcm_ops;
	platform->platform_ops = &imx_platform_ops;
	snd_soc_register_platform(platform);
	return 0;
}

static int imx_pcm_remove(struct device *dev)
{
	return 0;
}

const char imx_pcm[SND_SOC_PLATFORM_NAME_SIZE] = "imx-pcm";
EXPORT_SYMBOL_GPL(imx_pcm);

static struct snd_soc_device_driver imx_pcm_driver = {
	.type = SND_SOC_BUS_TYPE_DMA,
	.driver = {
		   .name = imx_pcm,
		   .owner = THIS_MODULE,
		   .bus = &asoc_bus_type,
		   .probe = imx_pcm_probe,
		   .remove = __devexit_p(imx_pcm_remove),
		   },
};

static __init int imx_pcm_init(void)
{
	return driver_register(&imx_pcm_driver.driver);
}

static __exit void imx_pcm_exit(void)
{
	driver_unregister(&imx_pcm_driver.driver);
}

module_init(imx_pcm_init);
module_exit(imx_pcm_exit);

MODULE_AUTHOR("Liam Girdwood");
MODULE_DESCRIPTION("Freescale i.MX3x PCM DMA module");
MODULE_LICENSE("GPL");
