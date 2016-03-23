/*
 * imxdpufb.h - Based on Simple Framebuffer Device
 *
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
 * Copyright (C) 2013 David Herrmann <dh.herrmann@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLATFORM_DATA_IMXDPUFB_H__
#define __PLATFORM_DATA_IMXDPUFB_H__

#include <drm/drm_fourcc.h>
#include <linux/fb.h>
#include <linux/kernel.h>

/* format array, use it to initialize a "struct imxdpufb_format" array */
#define IMXDPUFB_FORMATS \
{ \
	{ "a8r8g8b8", 32, {16, 8}, {8, 8}, {0, 8}, {24, 8}, DRM_FORMAT_ARGB8888 }, \
	{ "a8b8g8r8", 32, {0, 8}, {8, 8}, {16, 8}, {24, 8}, DRM_FORMAT_ABGR8888 }, \
	{ "r8b8g8a8", 32, {24, 8}, {16, 8}, {8, 8}, {0, 8}, DRM_FORMAT_RGBA8888}, \
	{ "nv12", 12, {0, 0}, {0, 0}, {0, 0}, {0, 0}, DRM_FORMAT_NV12 }, \
	{ "ayuv", 32, {16, 8}, {8, 8}, {0, 8}, {24, 8}, DRM_FORMAT_AYUV }, \
}

/*
 * Data-Format for Imxdpu-Framebuffers
 * @name: unique 0-terminated name that can be used to identify the mode
 * @red,green,blue: Offsets and sizes of the single RGB parts
 * @transp: Offset and size of the alpha bits. length=0 means no alpha
 * @fourcc: 32bit DRM four-CC code (see drm_fourcc.h)
 */
struct imxdpufb_format {
	const char *name;
	u32 bits_per_pixel;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
	u32 fourcc;
};

/*
 * Imxdpu-Framebuffer description
 * If the arch-boot code creates imxdpu-framebuffers without DT support, it
 * can pass the width, height, stride and format via this platform-data object.
 * The framebuffer location must be given as IORESOURCE_MEM resource.
 * @format must be a format as described in "struct imxdpufb_format" above.
 */
struct imxdpufb_platform_data {
	u32 width;
	u32 height;
	u32 stride;
	const char *format;
};

#endif /* __PLATFORM_DATA_IMXDPUFB_H__ */
