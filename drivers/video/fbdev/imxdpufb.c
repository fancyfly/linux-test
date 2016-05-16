/*
 * IMXDPU Framebuffer based upon simplefb (simple frame-buffer driver),
 * as a platform device
 *
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2013, Stephen Warren
 *
 * Based on q40fb.c, which was:
 * Copyright (C) 2001 Richard Zidlicky <rz@linux-m68k.org>
 *
 * Also based on offb.c, which was:
 * Copyright (C) 1997 Geert Uytterhoeven
 * Copyright (C) 1996 Paul Mackerras
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_data/imxdpufb.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk-provider.h>
#include <linux/of_platform.h>

#include <linux/imxdpu.h>

static struct fb_fix_screeninfo imxdpufb_fix = {
	.id = "imxdpufb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.accel = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo imxdpufb_var = {
	.height = -1,
	.width = -1,
	.activate = FB_ACTIVATE_NOW,
	.vmode = FB_VMODE_NONINTERLACED,
};

#define PSEUDO_PALETTE_SIZE 16

static uint32_t imxdpu_convert_format(uint32_t format);

static int imxdpufb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			      u_int transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;
	u32 cr = red >> (16 - info->var.red.length);
	u32 cg = green >> (16 - info->var.green.length);
	u32 cb = blue >> (16 - info->var.blue.length);
	u32 value;

	if (regno >= PSEUDO_PALETTE_SIZE)
		return -EINVAL;

	value = (cr << info->var.red.offset) |
	    (cg << info->var.green.offset) | (cb << info->var.blue.offset);
	if (info->var.transp.length > 0) {
		u32 mask = (1 << info->var.transp.length) - 1;
		mask <<= info->var.transp.offset;
		value |= mask;
	}
	pal[regno] = value;

	return 0;
}

static void imxdpufb_destroy(struct fb_info *info)
{
	if (info->screen_base)
		iounmap(info->screen_base);
}

static struct fb_ops imxdpufb_ops = {
	.owner = THIS_MODULE,
	.fb_destroy = imxdpufb_destroy,
	.fb_setcolreg = imxdpufb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

static struct imxdpufb_format imxdpufb_formats[] = IMXDPUFB_FORMATS;

struct imxdpufb_params {
	u32 width;
	u32 height;
	u32 stride;
	struct imxdpufb_format *format;
	s8 instance;
	int current_mode;
};

#define NUM_MODES 4
struct imxdpu_videomode mode_db[NUM_MODES] = {
	{
		"480P60",  // name[64]
		27000000,  //pixelclock Hz
		720,       // hlen;
		16,        // hfp;
		60,        // hbp;
		62,        // hsync;
		/* field0  */
		480,       // vlen;
		9,         // vfp;
		30,        // vbp;
		6,         // vsync;
		/* field1  */
		0,         // vlen;
		0,         // vfp;
		0,         // vbp;
		0,         // vsync;
		IMXDPU_DISP_FLAGS_HSYNC_LOW | IMXDPU_DISP_FLAGS_VSYNC_LOW | IMXDPU_DISP_FLAGS_DE_HIGH, //flags;
	},
	{
		"576P50",  // name[64]
		27000000,  //pixelclock Hz
		720,       // hlen;
		12,        // hfp;
		68,        // hbp;
		64,        // hsync;
		/* field0  */
		576,       // vlen;
		5,         // vfp;
		39,        // vbp;
		5,         // vsync;
		/* field1  */
		0,         // vlen;
		0,         // vfp;
		0,         // vbp;
		0,         // vsync;
		IMXDPU_DISP_FLAGS_HSYNC_LOW | IMXDPU_DISP_FLAGS_VSYNC_LOW | IMXDPU_DISP_FLAGS_DE_HIGH, //flags;
	},
	{
		"720P60",  // name[64]
		74250000,  //pixelclock Hz
		1280,      // hlen;
		110,       // hfp;
		220,       // hbp;
		40,        // hsync;
		/* field0  */
		720,       // vlen;
		5,         // vfp;
		20,        // vbp;
		5,         // vsync;
		/* field1  */
		0,         // vlen;
		0,         // vfp;
		0,         // vbp;
		0,         // vsync;
		IMXDPU_DISP_FLAGS_HSYNC_HIGH | IMXDPU_DISP_FLAGS_VSYNC_HIGH | IMXDPU_DISP_FLAGS_DE_HIGH, //flags;
	},
	{
		"1080P60", // name[64]
		148500000, //pixelclock Hz
		1920,      // hlen;
		88,        // hfp;
		148,       // hbp;
		44,        // hsync;
		/* field0  */
		1080,      // vlen;
		4,         // vfp;
		36,        // vbp;
		5,         // vsync;
		/* field1  */
		0,         // vlen;
		0,         // vfp;
		0,         // vbp;
		0,         // vsync;
		IMXDPU_DISP_FLAGS_HSYNC_HIGH | IMXDPU_DISP_FLAGS_VSYNC_HIGH | IMXDPU_DISP_FLAGS_DE_HIGH, //flags;
	},
};

static int imxdpufb_parse_dt(struct platform_device *pdev,
			     struct imxdpufb_params *params)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;
	const char *format;
	const char *temp;
	int i;

	temp = of_node_full_name(np);
	dev_dbg(&pdev->dev, "%s(): full name %s\n", __func__, temp);
	if (strncmp(temp, "/framebuffer@0", 14) == 0) {
		params->instance = 0;
	} else if (strncmp(temp, "/framebuffer@1", 14) == 0) {
		params->instance = 1;
	} else if (strncmp(temp, "/framebuffer@2", 14) == 0) {
		params->instance = 2;
	} else if (strncmp(temp, "/framebuffer@3", 14) == 0) {
		params->instance = 3;
	} else {
		dev_err(&pdev->dev, "Can't parse full name property\n");
		return -EINVAL;
	}
	dev_dbg(&pdev->dev, "%s(): instance %d\n", __func__, params->instance);

	ret = of_property_read_u32(np, "width", &params->width);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse width property\n");
		return ret;
	}

	ret = of_property_read_u32(np, "height", &params->height);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse height property\n");
		return ret;
	}

	ret = of_property_read_u32(np, "stride", &params->stride);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse stride property - using default\n");
		params->stride = 0;
	}

	ret = of_property_read_string(np, "format", &format);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse format property\n");
		return ret;
	}
	params->format = NULL;
	for (i = 0; i < ARRAY_SIZE(imxdpufb_formats); i++) {
		if (strcmp(format, imxdpufb_formats[i].name))
			continue;
		params->format = &imxdpufb_formats[i];
		break;
	}

	for (i = 0; i < ARRAY_SIZE(mode_db); i++) {
		if ((params->width == mode_db[i].hlen) &&
		    (params->height == mode_db[i].vlen)) {
			params->current_mode = i;
			break;
		}
	}

	if (i == ARRAY_SIZE(mode_db)) {
		dev_err(&pdev->dev, "Can't find mode - using default\n");
		params->current_mode = 2;
	}
	dev_dbg(&pdev->dev, "Using video mode: %s\n", &(mode_db[params->current_mode].name[0]));

	if (!params->format) {
		dev_err(&pdev->dev, "Invalid format value\n");
		return -EINVAL;
	} else {
		dev_err(&pdev->dev, "format name %s fourcc 0x%08x\n", params->format->name, params->format->fourcc);
	}
	dev_dbg(&pdev->dev, "%s(): %s registered.\n", __func__, temp);
	return 0;
}

static int imxdpufb_parse_pd(struct platform_device *pdev,
			     struct imxdpufb_params *params)
{
	struct imxdpufb_platform_data *pd = dev_get_platdata(&pdev->dev);
	int i;
	dev_err(&pdev->dev, "%s()\n",__func__);
	params->width = pd->width;
	params->height = pd->height;
	params->stride = pd->stride;

	params->format = NULL;
	for (i = 0; i < ARRAY_SIZE(imxdpufb_formats); i++) {
		if (strcmp(pd->format, imxdpufb_formats[i].name))
			continue;

		params->format = &imxdpufb_formats[i];
		break;
	}

	if (!params->format) {
		dev_err(&pdev->dev, "Invalid format value\n");
		return -EINVAL;
	}

	return 0;
}

struct imxdpufb_par {
	u32 palette[PSEUDO_PALETTE_SIZE];
#if defined CONFIG_OF && defined CONFIG_COMMON_CLK
	int clk_count;
	struct clk **clks;
#endif
};

#if defined CONFIG_OF && defined CONFIG_COMMON_CLK
/*
 * Clock handling code.
 *
 * Here we handle the clocks property of our "imxdpu-framebuffer" dt node.
 * This is necessary so that we can make sure that any clocks needed by
 * the display engine that the bootloader set up for us (and for which it
 * provided a imxdpufb dt node), stay up, for the life of the imxdpufb
 * driver.
 *
 * When the driver unloads, we cleanly disable, and then release the clocks.
 *
 * We only complain about errors here, no action is taken as the most likely
 * error can only happen due to a mismatch between the bootloader which set
 * up imxdpufb, and the clock definitions in the device tree. Chances are
 * that there are no adverse effects, and if there are, a clean teardown of
 * the fb probe will not help us much either. So just complain and carry on,
 * and hope that the user actually gets a working fb at the end of things.
 */
static int imxdpufb_clocks_init(struct imxdpufb_par *par,
				struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct clk *clock;
	int i, ret;

	if (dev_get_platdata(&pdev->dev) || !np)
		return 0;

	par->clk_count = of_clk_get_parent_count(np);
	if (par->clk_count <= 0)
		return 0;

	par->clks = kcalloc(par->clk_count, sizeof(struct clk *), GFP_KERNEL);
	if (!par->clks)
		return -ENOMEM;

	for (i = 0; i < par->clk_count; i++) {
		clock = of_clk_get(np, i);
		if (IS_ERR(clock)) {
			if (PTR_ERR(clock) == -EPROBE_DEFER) {
				while (--i >= 0) {
					if (par->clks[i])
						clk_put(par->clks[i]);
				}
				kfree(par->clks);
				return -EPROBE_DEFER;
			}
			dev_err(&pdev->dev, "%s: clock %d not found: %ld\n",
				__func__, i, PTR_ERR(clock));
			continue;
		}
		par->clks[i] = clock;
	}

	for (i = 0; i < par->clk_count; i++) {
		if (par->clks[i]) {
			ret = clk_prepare_enable(par->clks[i]);
			if (ret) {
				dev_err(&pdev->dev,
					"%s: failed to enable clock %d: %d\n",
					__func__, i, ret);
				clk_put(par->clks[i]);
				par->clks[i] = NULL;
			}
		}
	}

	return 0;
}

static void imxdpufb_clocks_destroy(struct imxdpufb_par *par)
{
	int i;

	if (!par->clks)
		return;

	for (i = 0; i < par->clk_count; i++) {
		if (par->clks[i]) {
			clk_disable_unprepare(par->clks[i]);
			clk_put(par->clks[i]);
		}
	}

	kfree(par->clks);
}
#else
static int imxdpufb_clocks_init(struct imxdpufb_par *par,
				struct platform_device *pdev)
{
	return 0;
}

static void imxdpufb_clocks_destroy(struct imxdpufb_par *par)
{
}
#endif

void fb_irq_handler(int irq, void *data)
{
	static int count = 0;

	if ((count++ % 1000) == 0) {
		pr_debug("%s() %d\n", __func__, count);
	}

	/* handle the irq */
}

static void imxdpu_start(uint64_t disp_phys_addr,
			 const struct imxdpu_videomode *vmode,
			 const struct imxdpufb_params *params)
{
	struct imxdpu_soc *imxdpu_inst;
	imxdpu_layer_t layer;
	imxdpu_chan_t chan;
	const int stride = params->stride;
	const int imxdpu_id = params->instance/2;
	const int imxdpu_display_id = params->instance%2;

	pr_debug("Starting display imxdpu id %d, display %d\n",
		imxdpu_id, imxdpu_display_id);

	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);
#if 1
	if (imxdpu_display_id == 0) {
		chan = IMXDPU_CHAN_VIDEO_0;
	} else {
		chan = IMXDPU_CHAN_VIDEO_1;
	}

	imxdpu_disp_setup_channel(
		imxdpu_id,
		chan,	//imxdpu_chan_t chan,
		imxdpu_convert_format(params->format->fourcc),  //uint32_t src_pixel_fmt,
		vmode->hlen,	//uint16_t src_width,
		vmode->vlen,	//uint16_t src_height,
		0,	//int16_t  clip_top,
		0,	//int16_t  clip_left,
		0,	//uint16_t clip_width,
		0,	//uint16_t clip_height,
		stride, //t16_t stride,
		imxdpu_display_id,
		0,	//int16_t  dest_top,
		0,	//int16_t  dest_left,
		vmode->hlen,	//uint16_t dest_width,
		vmode->vlen,	//uint16_t dest_height,
		IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) |
		IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0) |
		IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) |
		IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),//uint32_t const_color,
		disp_phys_addr);

#endif

	layer.enable = IMXDPU_TRUE;
	layer.secondary = get_channel_blk(chan);

	if (imxdpu_display_id == 0) {
		layer.stream = IMXDPU_DISPLAY_STREAM_0;
		layer.primary = IMXDPU_ID_CONSTFRAME0;
		imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
		imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0,
						   0xff);
	} else {
		layer.stream = IMXDPU_DISPLAY_STREAM_1;
		layer.primary = IMXDPU_ID_CONSTFRAME1;
		imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_1);
		imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_1,
						   0xff);
	}

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
#if 0
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)fb_irq_handler, 0, "imxdpufb", imxdpu_inst);
#endif
	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);
	imxdpu_disp_check_shadow_loads(imxdpu_id, imxdpu_display_id);
	pr_debug("imxdpufb display start ...\n");
}

static uint32_t imxdpu_convert_format(uint32_t format)
{
	uint32_t ret = 0;
	switch (format) {
		case DRM_FORMAT_NV12: ret = IMXDPU_PIX_FMT_NV12; break;
		case DRM_FORMAT_AYUV: ret = IMXDPU_PIX_FMT_AYUV; break;
		case DRM_FORMAT_RGB888: ret = IMXDPU_PIX_FMT_RGB24; break;
		case DRM_FORMAT_XRGB8888: ret = IMXDPU_PIX_FMT_RGB32; break;
		case DRM_FORMAT_ARGB8888: ret = IMXDPU_PIX_FMT_ARGB32; break;
		case DRM_FORMAT_ABGR8888: ret = IMXDPU_PIX_FMT_ABGR32; break;
		case DRM_FORMAT_RGBA8888: ret = IMXDPU_PIX_FMT_RGBA32; break;
		case DRM_FORMAT_BGRA8888: ret = IMXDPU_PIX_FMT_BGRA32; break;
		case DRM_FORMAT_RGB565: ret = IMXDPU_PIX_FMT_RGB565; break;

		case DRM_FORMAT_XRGB1555:
		case DRM_FORMAT_ARGB1555:
		case DRM_FORMAT_XRGB2101010:
		case DRM_FORMAT_ARGB2101010:
		default:
			ret = 0;
	}
	pr_debug("%s(): format in 0x%08x out 0x%08x\n", __func__, format, ret);
 	return ret;
}
static int imxdpufb_probe(struct platform_device *pdev)
{
	int ret;
	struct imxdpufb_params params;
	struct fb_info *info;
	struct imxdpufb_par *par;
	struct resource *mem;

	if (fb_get_options("imxdpufb", NULL))
		return -ENODEV;

	ret = -ENODEV;
	if (dev_get_platdata(&pdev->dev))
		ret = imxdpufb_parse_pd(pdev, &params);
	else if (pdev->dev.of_node)
		ret = imxdpufb_parse_dt(pdev, &params);

	if (ret)
		return ret;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "No memory resource\n");
		return -EINVAL;
	}

	dev_dbg(&pdev->dev, "mem->start %p size %u\n", (void *)mem->start,
		(uint32_t) resource_size(mem));

	info = framebuffer_alloc(sizeof(struct imxdpufb_par), &pdev->dev);
	if (!info) {
		dev_err(&pdev->dev,
			"framebuffer_alloc failed to allocate memory\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, info);

	par = info->par;
	info->fix = imxdpufb_fix;

	if (params.format->fourcc == DRM_FORMAT_AYUV) {
		if (params.stride == 0 ) {
			params.stride = params.width * 4;
		}
		info->var.nonstd = DRM_FORMAT_AYUV;
	} else if (params.format->fourcc == DRM_FORMAT_NV12) {
		if (params.stride == 0 ) {
			params.stride = params.width;
		}
		info->fix.smem_len =
			params.width * (params.height + params.height/2);
		info->var.nonstd = DRM_FORMAT_NV12;
	} else {
		if (params.stride == 0 ) {
			params.stride = params.width * 4;
		}
		info->fix.smem_len = params.height * params.stride;
	}
	info->fix.line_length = params.stride;
	info->screen_size = info->fix.smem_len;
	info->screen_base = dma_alloc_coherent(&pdev->dev, info->fix.smem_len,
					       (dma_addr_t *) & info->fix.
					       smem_start,
					       GFP_DMA | GFP_KERNEL);
	if (info->fix.smem_start == 0) {
		dev_err(&pdev->dev, "failed to allocate memory for DMA\n");
		ret = -ENOMEM;
		goto error_fb_release;
	}

	dev_dbg(&pdev->dev, "info->fix.smem_start %p info->fix.smem_len %u\n",
		(void *)info->fix.smem_start, info->fix.smem_len);
#if 0
	for (i = 0; i < info->screen_size / 4; i++) {
		((uint32_t *) info->screen_base)[i] = 0x00ffff00;
	}
#endif
	info->var = imxdpufb_var;
	info->var.xres = params.width;
	info->var.yres = params.height;
	info->var.xres_virtual = params.width;
	info->var.yres_virtual = params.height;
        info->var.bits_per_pixel = params.format->bits_per_pixel;

        info->var.red = params.format->red;
        info->var.green = params.format->green;
        info->var.blue = params.format->blue;
        info->var.transp = params.format->transp;

	info->apertures = alloc_apertures(1);
	if (!info->apertures) {
		ret = -ENOMEM;
		dev_err(&pdev->dev,
			"falloc_apertures failed to allocate memory\n");
		goto error_fb_release;
	}
	info->apertures->ranges[0].base = info->fix.smem_start;
	info->apertures->ranges[0].size = info->fix.smem_len;

	info->fbops = &imxdpufb_ops;
	info->flags = FBINFO_DEFAULT | FBINFO_MISC_FIRMWARE;
	info->pseudo_palette = par->palette;

	ret = imxdpufb_clocks_init(par, pdev);
	if (ret < 0)
		goto error_unmap;

	dev_dbg(&pdev->dev,
		 "framebuffer at 0x%lx, 0x%x bytes, mapped to 0x%p\n",
		 info->fix.smem_start, info->fix.smem_len, info->screen_base);
	dev_dbg(&pdev->dev, "format=%s, mode=%dx%dx%d, linelength=%d\n",
		 params.format->name, info->var.xres, info->var.yres,
		 info->var.bits_per_pixel, info->fix.line_length);

	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Unable to register imxdpufb: %d\n", ret);
		goto error_clocks;
	}

	imxdpu_start((uint64_t) info->fix.smem_start,
		     &mode_db[params.current_mode],
		     &params);

	dev_info(&pdev->dev, "fb%d imxdpufb registered!\n", info->node);

	return 0;

 error_clocks:
	imxdpufb_clocks_destroy(par);
 error_unmap:
	iounmap(info->screen_base);
 error_fb_release:
	framebuffer_release(info);
	return ret;
}

static int imxdpufb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	struct imxdpufb_par *par = info->par;

	unregister_framebuffer(info);
	imxdpufb_clocks_destroy(par);
	framebuffer_release(info);

	return 0;
}

static const struct of_device_id imxdpufb_of_match[] = {
	{.compatible = "imxdpu-framebuffer",},
	{},
};

MODULE_DEVICE_TABLE(of, imxdpufb_of_match);

static struct platform_driver imxdpufb_driver = {
	.driver = {
		   .name = "imxdpu-framebuffer",
		   .of_match_table = imxdpufb_of_match,
		   },
	.probe = imxdpufb_probe,
	.remove = imxdpufb_remove,
};

static int __init imxdpufb_init(void)
{
	int ret;
	struct device_node *np;

	ret = platform_driver_register(&imxdpufb_driver);
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_OF_ADDRESS) && of_chosen) {
		for_each_child_of_node(of_chosen, np) {
			if (of_device_is_compatible(np, "imxdpu-framebuffer"))
				of_platform_device_create(np, NULL, NULL);
		}
	}

	return 0;
}

//fs_initcall(imxdpufb_init);
late_initcall(imxdpufb_init);

MODULE_AUTHOR("Oliver Brown <oliver.brown@nxp.com>");
MODULE_DESCRIPTION("IMXDPU framebuffer driver");
MODULE_LICENSE("GPL v2");
