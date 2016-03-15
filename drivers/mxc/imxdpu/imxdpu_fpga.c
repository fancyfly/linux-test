/*
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/imxdpu.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/miscdevice.h>

#define TEST_BE	0
#define DEBUG

#include "imxdpu_registers.h"
#include "imxdpu_events.h"
#include "imxdpu_private.h"
#include "imxdpu_be.h"

#define DRIVER_NAME	"iris"
#include "lcdif_create_image.h"

void __iomem *iris_base;
#define IRIS_REGS_BASE	iris_base
#define IRIS_REGS_BASE_PHY	0x01000000

static unsigned int g_disp_addr;
static unsigned int g_display_addr_virt;
bool iris_probed = false;
unsigned int get_iris_display_base(void)
{
	return g_display_addr_virt;
}

EXPORT_SYMBOL(get_iris_display_base);

#ifdef DEBUG
#define iris_raw_writel(reg, val)	{__raw_writel(val, reg); printk("%s:%4d write reg 0x%08x <-- val 0x%08x\n", \
	__FILE__, __LINE__, (unsigned int)reg, (unsigned int)val);}
#else
#define iris_raw_writel(reg, val)	__raw_writel(val, reg);
#endif

#ifdef DEBUG
static u32 iris_raw_read(void *reg)
{
	u32 val;
	val = __raw_readl(reg);
	printk("%s:%4d read  reg 0x%08x --> val 0x%08x\n", __FILE__, __LINE__,
	       (unsigned int)reg, (unsigned int)val);
	return val;
}
#else
#define iris_raw_read(reg) __raw_readl(reg)
#endif

#if 0
#define BL_PRE_FW      88
#define BL_AFT_FW      40
#define BL_PRE_FH      23
#define BL_AFT_FH      1
#define HSYNC_PW       128
#define VSYNC_PW       4
#define FW             800
#define FH             600
#else
#define BL_PRE_FW      40
#define BL_AFT_FW      8
#define BL_PRE_FH      25
#define BL_AFT_FH      2
#define HSYNC_PW       96
#define VSYNC_PW       2
#define FW             640
#define FH             480
#endif

static DEFINE_PCI_DEVICE_TABLE(iris_pci_tbl) = {
	{
	0xabcd, 0x4321, PCI_ANY_ID, PCI_ANY_ID, 0x0, 0x0}, {
	0}
};

static inline void sleep(unsigned sec)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(sec * HZ);
}

/* Don't set bar0 here, done in FPGA RTL*/
/*
static void __init iris_pci_bar0(void)
{
	void __iomem *p;
	p = ioremap(0x01F00000, 0x1000);
	writel(0x8000000C, p+0x10);
	iounmap(p);
} */

void setup_channel(int8_t imxdpu_id,
		   imxdpu_chan_t chan,
		   uint32_t src_pixel_fmt,
		   uint16_t src_width,
		   uint16_t src_height,
		   int16_t clip_top,
		   int16_t clip_left,
		   uint16_t clip_width, uint16_t clip_height, uint16_t stride,
//      uint32_t dest_pixel_fmt,
//      uint8_t  blend_mode,
//      uint8_t  blend_layer,
		   uint8_t disp_id,
		   int16_t dest_top,
		   int16_t dest_left,
		   uint16_t dest_width,
		   uint16_t dest_height,
		   uint32_t const_color, unsigned int disp_addr)
{
	imxdpu_channel_params_t channel;
	channel.common.chan = chan;
	channel.common.src_pixel_fmt = src_pixel_fmt;
	channel.common.src_width = src_width;
	channel.common.src_height = src_height;
	channel.common.clip_top = clip_top;
	channel.common.clip_left = clip_left;
	channel.common.clip_width = clip_width;
	channel.common.clip_height = clip_height;
	channel.common.stride = stride;
//      channel.common.dest_pixel_fmt = dest_pixel_fmt;
//      channel.common.blend_mode =     blend_mode;
//      channel.common.blend_layer =    blend_layer;
	channel.common.disp_id = disp_id;
	channel.common.dest_top = dest_top;
	channel.common.dest_left = dest_left;
	channel.common.dest_width = dest_width;
	channel.common.dest_height = dest_height;
	channel.common.const_color = const_color;
	//channel.common.stride = FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	//channel.common.use_video_proc = IMXDPU_FALSE;
	//channel.common.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.common.chan, channel.common.stride, IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr,	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	imxdpu_disp_set_chan_crop(imxdpu_id,
				  channel.common.chan,
				  channel.common.clip_top,
				  channel.common.clip_left,
				  channel.common.clip_width,
				  channel.common.clip_height,
				  channel.common.dest_top,
				  channel.common.dest_left,
				  channel.common.dest_width,
				  channel.common.dest_height);
}

//static void iris_fetchdecode2_di0_board(unsigned int disp_addr){
static void imxdpu_test0(unsigned int disp_addr)
{
	int fail_count = 0;

	unsigned int read_data;

	unsigned int h_total, v_total;

	pr_info("Accessing the IRIS REG CONFIG\n");
	h_total = BL_PRE_FW + BL_AFT_FW + HSYNC_PW + FW - 1;

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_HTCFG1),
			(h_total << IMXDPU_FRAMEGEN0_HTCFG2_HSBP_SHIFT) + FW);
	read_data = iris_raw_read(IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_HTCFG1);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_HTCFG2),
			IMXDPU_FRAMEGEN0_HTCFG2_HSEN_MASK +
			((HSYNC_PW + BL_PRE_FW -
			  1) << IMXDPU_FRAMEGEN0_HTCFG2_HSBP_SHIFT) +
			(HSYNC_PW - 1));

	v_total = BL_PRE_FH + BL_AFT_FH + VSYNC_PW + FH - 1;
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_VTCFG1),
			(v_total << IMXDPU_FRAMEGEN0_VTCFG1_VTOTAL_SHIFT) + FH);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_VTCFG2),
			IMXDPU_FRAMEGEN0_VTCFG2_VSEN_MASK +
			((VSYNC_PW + BL_PRE_FH -
			  1) << IMXDPU_FRAMEGEN0_VTCFG2_VSBP_SHIFT) +
			(VSYNC_PW - 1));

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_SKICKCONFIG),
			IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKEN_MASK +
			((FH -
			  1) << IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKROW_SHIFT) +
			(FW - 40));
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_PIXENGCFG_EXTDST0_STATIC),
			(0x0080 <<
			 IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_DIV_SHIFT)
			|
			(IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SYNC_MODE__AUTO
			 <<
			 IMXDPU_PIXENGCFG_EXTDST0_STATIC_EXTDST0_SYNC_MODE_SHIFT)
	    );			//rId:FME_PIXENGCFG_EXTDST0_STATIC#  #iId:PIXENGCFG#  #mId:Static pixel engine configuration for extdst0#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_DISENGCFG_POLARITYCTRL0), IMXDPU_DISENGCFG_POLARITYCTRL0_POLEN0_MASK);	//rId:FME_DISENGCFG_POLARITYCTRL0#  #iId:DISENGCFG#  #mId:Polarity control for TCon#0 input and corresponding top-level output (TCon by-pass port).#

	iris_raw_writel((IRIS_REGS_BASE +
			 IMXDPU_FETCHDECODE2_BURSTBUFFERMANAGEMENT),
			(0x10 <<
			 IMXDPU_FETCHDECODE2_BURSTBUFFERMANAGEMENT_SETBURSTLENGTH_SHIFT)
			+
			(0x10 <<
			 IMXDPU_FETCHDECODE2_BURSTBUFFERMANAGEMENT_SETNUMBUFFERS_SHIFT));

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FETCHDECODE2_BASEADDRESS0),
			disp_addr);

	iris_raw_writel((IRIS_REGS_BASE +
			 IMXDPU_FETCHDECODE2_SOURCEBUFFERATTRIBUTES0),
			(0x0020 <<
			 IMXDPU_FETCHDECODE2_SOURCEBUFFERATTRIBUTES0_BITSPERPIXEL0_SHIFT)
			+
			((FW * 4 -
			  1) <<
			 IMXDPU_FETCHDECODE2_SOURCEBUFFERATTRIBUTES0_STRIDE0_SHIFT));

	iris_raw_writel((IRIS_REGS_BASE +
			 IMXDPU_FETCHDECODE2_SOURCEBUFFERDIMENSION0),
			((FH -
			  1) <<
			 IMXDPU_FETCHDECODE2_SOURCEBUFFERDIMENSION0_LINECOUNT0_SHIFT)
			+
			((FW -
			  1) <<
			 IMXDPU_FETCHDECODE2_SOURCEBUFFERDIMENSION0_LINEWIDTH0_SHIFT));

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FETCHDECODE2_COLORCOMPONENTSHIFT0), (0x10 << IMXDPU_FETCHDECODE2_COLORCOMPONENTSHIFT0_COMPONENTSHIFTRED0_SHIFT) + (0x08 << IMXDPU_FETCHDECODE2_COLORCOMPONENTSHIFT0_COMPONENTSHIFTGREEN0_SHIFT) + (0x00 << IMXDPU_FETCHDECODE2_COLORCOMPONENTSHIFT0_COMPONENTSHIFTBLUE0_SHIFT) + (0x18 << IMXDPU_FETCHDECODE2_COLORCOMPONENTSHIFT0_COMPONENTSHIFTALPHA0_SHIFT));	// color component shift

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT19_16), 0x05040302);	// tcon mapping
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT23_20),
			0x09080706);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT11_8), 0x0f0e0d0c);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT15_12),
			0x13121110);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT3_0), 0x19181716);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT7_4), 0x1d1c1b1a);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FETCHDECODE2_FRAMEDIMENSIONS), ((FH - 1) << IMXDPU_FETCHDECODE2_FRAMEDIMENSIONS_FRAMEHEIGHT_SHIFT) + ((FW - 1) << IMXDPU_FETCHDECODE2_FRAMEDIMENSIONS_FRAMEWIDTH_SHIFT));	// fetchdecode frame dimension
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_CONSTFRAME0_FRAMEDIMENSIONS), ((FH - 1) << IMXDPU_CONSTFRAME0_FRAMEDIMENSIONS_FRAMEHEIGHT_SHIFT) + ((FW - 1) << IMXDPU_CONSTFRAME0_FRAMEDIMENSIONS_FRAMEWIDTH_SHIFT));	// constframe frame dimension

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGINCTRL), (IMXDPU_FRAMEGEN0_FGINCTRL_FGDM__SEC << IMXDPU_FRAMEGEN0_FGINCTRL_FGDM_SHIFT));	//rId:FME_FRAMEGEN_FGINCTRL#  #iId:FRAMEGEN0#  #mId:FrameGen Input Control Register (shadowed)#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC), (0x0000002c << IMXDPU_PIXENGCFG_EXTDST0_DYNAMIC_EXTDST0_SRC_SEL_SHIFT));	//rId:FME_PIXENGCFG_EXTDST0_DYNAMIC#  #iId:PIXENGCFG#  #mId:Dynamic pixel engine configuration for extdst0#

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_LAYERBLEND0_STATICCONTROL),
			IMXDPU_LAYERBLEND0_STATICCONTROL_RESET_VALUE);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC), (IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_CLKEN__AUTOMATIC << IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_CLKEN_SHIFT) + (IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_SEC_SEL__FETCHDECODE2 << IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_SEC_SEL_SHIFT) + (IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_PRIM_SEL__CONSTFRAME0 << IMXDPU_PIXENGCFG_LAYERBLEND0_DYNAMIC_LAYERBLEND0_PRIM_SEL_SHIFT));	//rId:FME_PIXENGCFG_LAYERBLEND0_DYNAMIC#  #iId:PIXENGCFG#  #mId:Dynamic pixel engine configuration for layerblend0#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_PIXENGCFG_EXTDST0_TRIGGER), IMXDPU_PIXENGCFG_EXTDST0_TRIGGER_EXTDST0_SYNC_TRIGGER_MASK);	//rId:FME_PIXENGCFG_EXTDST0_TRIGGER#  #iId:PIXENGCFG#  #mId:Trigger bits for pixel engine configuration of extdst0#

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGENABLE), IMXDPU_FRAMEGEN0_FGENABLE_FGEN_MASK);	//rId:FME_FRAMEGEN_FGENABLE#  #iId:FRAMEGEN0#  #mId:FrameGen Enable Register#

	read_data = iris_raw_read(IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGCHSTAT);

	while (read_data != (IMXDPU_FRAMEGEN0_FGCHSTAT_SECSYNCSTAT_MASK |
			     IMXDPU_FRAMEGEN0_FGCHSTAT_PFIFOEMPTY_MASK)) {
		read_data =
		    iris_raw_read(IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGCHSTAT);
		printk("IMXDPU_FRAMEGEN0_FGCHSTAT = 0x%08x\n", read_data);
		sleep(1);
		if (fail_count++ > 10) {
			printk("IRIS TIMEOUT Error !!!!!!!!!!!!");
			break;
		}
	}			//#mId:FrameGen Channel Status Register#

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGCHSTATCLR), IMXDPU_FRAMEGEN0_FGCHSTATCLR_CLRSECSTAT_MASK);	//???? //rId:FME_FRAMEGEN_FGCHSTATCLR#  #iId:FRAMEGEN0#  #mId:FrameGen Channel Status Clear Register#
	//r 0x100 0x00000000    ); //rId:FME_COMCTRL_USERINTERRUPTENABLE0#  #iId:COMCTRL#  #mId:Interrupt Enable register 0 for user mode access#
	//r 0x104 0x00000000    ); //rId:FME_COMCTRL_USERINTERRUPTENABLE1#  #iId:COMCTRL#  #mId:Interrupt Enable register 1 for user mode access#
	//r 0x108 0x00000000    ); //rId:FME_COMCTRL_USERINTERRUPTENABLE2#  #iId:COMCTRL#  #mId:Interrupt Enable register 2 for user mode access#

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_COMCTRL_USERINTERRUPTMASK0), 0x20000000);	//rId:FME_COMCTRL_USERINTERRUPTENABLE0#  #iId:COMCTRL#  #mId:Interrupt Enable register 0 for user mode access#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_COMCTRL_USERINTERRUPTMASK1), 0x00000000);	//rId:FME_COMCTRL_USERINTERRUPTENABLE1#  #iId:COMCTRL#  #mId:Interrupt Enable register 1 for user mode access#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_COMCTRL_USERINTERRUPTMASK2), 0x00000000);	//rId:FME_COMCTRL_USERINTERRUPTENABLE2#  #iId:COMCTRL#  #mId:Interrupt Enable register 2 for user mode access#

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_COMCTRL_USERINTERRUPTENABLE0), 0x20000000);	//rId:FME_COMCTRL_USERINTERRUPTENABLE0#  #iId:COMCTRL#  #mId:Interrupt Enable register 0 for user mode access#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_COMCTRL_USERINTERRUPTENABLE1), 0x00000000);	//rId:FME_COMCTRL_USERINTERRUPTENABLE1#  #iId:COMCTRL#  #mId:Interrupt Enable register 1 for user mode access#
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_COMCTRL_USERINTERRUPTENABLE2), 0x00000000);	//rId:FME_COMCTRL_USERINTERRUPTENABLE2#  #iId:COMCTRL#  #mId:Interrupt Enable register 2 for user mode access#

	//iris_raw_writel((IRIS_REGS_BASE+IMXDPU_COMCTRL_INTERRUPTCLEAR0),0x20000000    ); //rId:FME_COMCTRL_INTERRUPTCLEAR0#  #iId:COMCTRL#  #mId:Interrupt Clear register 0#
	//iris_raw_writel((IRIS_REGS_BASE+IMXDPU_COMCTRL_INTERRUPTCLEAR1),0x00000000    ); //rId:FME_COMCTRL_INTERRUPTCLEAR1#  #iId:COMCTRL#  #mId:Interrupt Clear register 1#
	//iris_raw_writel((IRIS_REGS_BASE+IMXDPU_COMCTRL_INTERRUPTCLEAR2),0x00000000    ); //rId:FME_COMCTRL_INTERRUPTCLEAR2#  #iId:COMCTRL#  #mId:Interrupt Clear register 2#

	pr_info("IRIS display start ...\n");
}

void imxdpu_test_irq_handler(int irq, void *data)
{
	static int count = 0;

	if ((count++ % 1000) == 0) {
		pr_debug("%s() %d\n", __func__, count);
	}

	/* handle the irq */
}

static void imxdpu_test1(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_channel_params_t channel;
	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);

#if 1
	channel.fetch_decode.chan = IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = FW / 4;
	channel.fetch_decode.src_height = FH;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = FW / 4;
	channel.fetch_decode.dest_height = FH;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (0 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0x80);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_0, 0, 32);
	imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan, 0,
				      32);
#endif

#if 0
	channel.fetch_decode.chan = IMXDPU_CHAN_INTERGRAL_1;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;

	channel.fetch_decode.src_width = 128;
	channel.fetch_decode.src_height = 64;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 64;
	channel.fetch_decode.dest_height = 64;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = -64;

	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   ///0,//dma_addr_t phyaddr_1,
				   //0,//dma_addr_t phyaddr_2,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND0;	//IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_1);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_1, 0x90);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_1, 96, 32);
	//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan,32,32);

#endif

#if 0
	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_0;	//IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = 128;
	channel.fetch_decode.src_height = 128;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 128;
	channel.fetch_decode.dest_height = 128;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;

	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND1;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);
	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_2);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_2, 0xc0);
	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_2, 0, 128);
	//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan,128,128);
#endif

#if 0
	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_1;	//IMXDPU_CHAN_INTERGRAL_0;
	//channel.fetch_decode.chan = IMXDPU_CHAN_FRACTIONAL_1_1;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = 128;
	channel.fetch_decode.src_height = 128;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 128;
	channel.fetch_decode.dest_height = 128;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;

	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND2;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);
	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_3);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_3, 0xc0);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_3, 128 + 32,
				       128);
	//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan,256,256);
#endif

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test2(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_channel_params_t channel;
	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);

#if 1				// 8 layers
	memset(&channel, 0, sizeof(imxdpu_channel_params_t));
	channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_1;	//IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.src_width = FW;
	channel.fetch_layer.src_height = FH;
	channel.fetch_layer.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	channel.fetch_layer.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);
	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (0 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
				  0,	// clip_left,
				  32,	// clip_width,
				  32,	// clip_height,
				  8,	// dest_top,
				  8,	// dest_left,
				  32,	// dest_width,
				  32	// dest_height
	    );

#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_2;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;

		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;

		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  8,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  16,	// dest_top,
					  16,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );

	}
#endif
#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_3;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;

		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  16,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  32,	// dest_top,
					  32,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );

	}
#endif
#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_4;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  24,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  24,	// dest_top,
					  24,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );

	}
#endif
#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_5;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  32,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  32,	// dest_top,
					  32,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );

	}
#endif
#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_6;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  40,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  40,	// dest_top,
					  40,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );
	}
#endif
#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_7;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  48,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  48,	// dest_top,
					  48,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );

	}
#endif
#if 1
	{
		memset(&channel, 0, sizeof(imxdpu_channel_params_t));
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_8;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
					  56,	// clip_left,
					  32,	// clip_width,
					  32,	// clip_height,
					  56,	// dest_top,
					  56,	// dest_left,
					  32,	// dest_width,
					  32	// dest_height
		    );
	}
#endif

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_CONSTFRAME0;
	//layer.primary   = IMXDPU_ID_LAYERBLEND1;

	layer.secondary = get_channel_blk(IMXDPU_CHAN_FRACTIONAL_0_1);
	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0xff);
#endif

#if 1
	memset(&channel, 0, sizeof(imxdpu_channel_params_t));
	channel.fetch_decode.chan = IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = FW;
	channel.fetch_decode.src_height = FH;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	//layer.primary   = IMXDPU_ID_CONSTFRAME0;
	layer.primary = IMXDPU_ID_LAYERBLEND0;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;
	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_1);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_1, 0x40);
	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
				  0,	// clip_left,
				  32,	// clip_width,
				  32,	// clip_height,
				  128,	// dest_top,
				  0,	// dest_left,
				  32,	// dest_width,
				  32	// dest_height
	    );

#endif
#if 1
	memset(&channel, 0, sizeof(imxdpu_channel_params_t));
	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_0;	//IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = FW;
	channel.fetch_decode.src_height = FH;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;

	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND1;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_2);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_2, 0x40);

	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
				  0,	// clip_left,
				  32,	// clip_width,
				  32,	// clip_height,
				  128,	// dest_top,
				  64,	// dest_left,
				  32,	// dest_width,
				  32	// dest_height
	    );

#endif

#if 1
	memset(&channel, 0, sizeof(imxdpu_channel_params_t));
	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_1;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = FW;
	channel.fetch_decode.src_height = FH;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (3 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	//layer.primary   = IMXDPU_ID_CONSTFRAME0;
	layer.primary = IMXDPU_ID_LAYERBLEND2;

	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_3);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_3, 0x40);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_3, 196, 128);
	imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan, 0,
				      0);
	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
				  0,	// clip_left,
				  32,	// clip_width,
				  32,	// clip_height,
				  128,	// dest_top,
				  64,	// dest_left,
				  32,	// dest_width,
				  32	// dest_height
	    );

#endif

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test2_1(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_1,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      0,	//int16_t  dest_top,
		      0,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_2,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      16,	//int16_t  dest_top,
		      16,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_3,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      32,	//int16_t  dest_top,
		      32,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_4,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      48,	//int16_t  dest_top,
		      48,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_5,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      64,	//int16_t  dest_top,
		      64,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_6,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      80,	//int16_t  dest_top,
		      80,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_7,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      96,	//int16_t  dest_top,
		      96,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_0_8,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      112,	//int16_t  dest_top,
		      112,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_CONSTFRAME0;
	//layer.primary   = IMXDPU_ID_LAYERBLEND0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_FRACTIONAL_0_1);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0xff);

#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_1,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      64,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      0,	//int16_t  dest_top,
		      128,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );

#endif

#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_2,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      16,	//int16_t  dest_top,
		      128 + 16,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_3,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      32,	//int16_t  dest_top,
		      128 + 32,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_4,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      48,	//int16_t  dest_top,
		      128 + 48,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_5,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      64,	//int16_t  dest_top,
		      128 + 64,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_6,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      80,	//int16_t  dest_top,
		      128 + 80,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_7,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      96,	//int16_t  dest_top,
		      128 + 96,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_FRACTIONAL_1_8,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      112,	//int16_t  dest_top,
		      128 + 112,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND0;	//IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_FRACTIONAL_1_1);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_1);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_1, 0xff);

#if 1
	setup_channel(0, IMXDPU_CHAN_INTERGRAL_0,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      64,	//uint16_t clip_width,
		      64,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      128,	//int16_t  dest_top,
		      0,	//int16_t  dest_left,
		      64,	//uint16_t dest_width,
		      64,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (1 * FW * FH * 4)	//unsigned int disp_addr
	    );

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND1;	//IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_INTERGRAL_0);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_2);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_2, 0xff);
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_INTERGRAL_1,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      64,	//uint16_t clip_width,
		      64,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      128,	//int16_t  dest_top,
		      256,	//int16_t  dest_left,
		      64,	//uint16_t dest_width,
		      64,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (1 * FW * FH * 4)	//unsigned int disp_addr
	    );

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND2;	//IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_INTERGRAL_1);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_3);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_3, 0xff);
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_VIDEO_0,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      64,	//uint16_t clip_width,
		      64,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      256,	//int16_t  dest_top,
		      0,	//int16_t  dest_left,
		      64,	//uint16_t dest_width,
		      64,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (3 * FW * FH * 4)	//unsigned int disp_addr
	    );

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND3;	//IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_VIDEO_0);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_4);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_4, 0xff);
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_VIDEO_1,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      64,	//uint16_t clip_width,
		      64,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      256,	//int16_t  dest_top,
		      256,	//int16_t  dest_left,
		      64,	//uint16_t dest_width,
		      64,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (3 * FW * FH * 4)	//unsigned int disp_addr
	    );

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND4;	//IMXDPU_ID_CONSTFRAME0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_VIDEO_1);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_5);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_5, 0xff);
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_1,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      0,	//int16_t  dest_top,
		      256 + 0,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_2,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      16,	//int16_t  dest_top,
		      256 + 16,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_3,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      32,	//int16_t  dest_top,
		      256 + 32,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_4,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      48,	//int16_t  dest_top,
		      256 + 48,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_5,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      64,	//int16_t  dest_top,
		      256 + 64,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_6,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      80,	//int16_t  dest_top,
		      256 + 80,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_7,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      96,	//int16_t  dest_top,
		      256 + 96,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_8,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      112,	//int16_t  dest_top,
		      256 + 112,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif

	layer.enable = IMXDPU_TRUE;
	//layer.primary   = IMXDPU_ID_CONSTFRAME0;
	layer.primary = IMXDPU_ID_LAYERBLEND5;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_WARP_2_1);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_6);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_6, 0xff);

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test2_2(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_1,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      0,	//int16_t  dest_top,
		      0,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_2,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      16,	//int16_t  dest_top,
		      16,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_3,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      32,	//int16_t  dest_top,
		      32,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_4,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      48,	//int16_t  dest_top,
		      48,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_5,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      64,	//int16_t  dest_top,
		      64,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_6,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      80,	//int16_t  dest_top,
		      80,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_7,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      96,	//int16_t  dest_top,
		      96,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif
#if 1
	setup_channel(0, IMXDPU_CHAN_WARP_2_8,	//imxdpu_chan_t chan,
		      IMXDPU_PIX_FMT_RGB32,	//uint32_t src_pixel_fmt,
		      FW,	//uint16_t src_width,
		      FH,	//uint16_t src_height,
		      0,	//int16_t  clip_top,
		      0,	//int16_t  clip_left,
		      32,	//uint16_t clip_width,
		      32,	//uint16_t clip_height,
		      FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32),	//uint16_t stride,
		      0,	//uint8_t  disp_id,
		      112,	//int16_t  dest_top,
		      112,	//int16_t  dest_left,
		      32,	//uint16_t dest_width,
		      32,	//uint16_t dest_height,
		      IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTRED, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTGREEN, 0x80) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTBLUE, 0) | IMXDPU_SET_FIELD(IMXDPU_COLOR_CONSTALPHA, 0),	//uint32_t const_color,
		      disp_addr + (2 * FW * FH * 4)	//unsigned int disp_addr
	    );
#endif

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_CONSTFRAME0;
	//layer.primary   = IMXDPU_ID_LAYERBLEND0;
	layer.secondary = get_channel_blk(IMXDPU_CHAN_WARP_2_1);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0xff);

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test3(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_channel_params_t channel;
	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);

#if 1				// 8 layers
	channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_1;	//IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.src_width = 32;
	channel.fetch_layer.src_height = 32;

	channel.fetch_layer.clip_width = 0;
	channel.fetch_layer.clip_height = 0;
	channel.fetch_layer.clip_top = 0;
	channel.fetch_layer.clip_left = 0;

	channel.fetch_layer.dest_width = FW;
	channel.fetch_layer.dest_height = FH / 4;
	channel.fetch_layer.dest_top = 0;
	channel.fetch_layer.dest_left = 0;
	channel.fetch_layer.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
	channel.fetch_layer.disp_id = 0;

	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_2;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 32;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 64;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 0;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,0,128);

	}
#endif
#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_3;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 32;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 128;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 0;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);

	}
#endif
#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_4;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 32;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 192;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 0;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);

	}
#endif
#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_5;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 64;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 320;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 32;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);

	}
#endif
#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_6;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 64;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 384;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 32;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);

	}
#endif
#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_6;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 64;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 384;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 32;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);

	}
#endif

#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_7;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 64;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 448;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 32;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);

	}
#endif
#if 1
	{
		channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_0_8;	//IMXDPU_CHAN_INTERGRAL_0;
		channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
		channel.fetch_layer.src_width = FW;
		channel.fetch_layer.src_height = FH / 4;

		channel.fetch_layer.clip_width = 32;
		channel.fetch_layer.clip_height = 64;
		channel.fetch_layer.clip_top = 0;
		channel.fetch_layer.clip_left = 512;

		channel.fetch_layer.dest_width = FW;
		channel.fetch_layer.dest_height = FH / 4;
		channel.fetch_layer.dest_top = 32;
		channel.fetch_layer.dest_left = 0;
		channel.fetch_layer.stride =
		    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
		//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
		channel.fetch_layer.disp_id = 0;
		imxdpu_init_channel(imxdpu_id, &channel);
		imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
					   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
					   0,	//uint32_t u_offset,
					   0);	//uint32_t v_offset)
		//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan,128,0);
	}
#endif

	layer.enable = IMXDPU_TRUE;

	layer.primary = IMXDPU_ID_CONSTFRAME0;
	//layer.primary   = IMXDPU_ID_LAYERBLEND1;

	layer.secondary = get_channel_blk(channel.fetch_layer.chan);
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;
	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0xff);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_0, 0, 0);	////????
	imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_layer.chan, 0,
				      0);
#endif

#if 1
	channel.fetch_decode.chan = IMXDPU_CHAN_INTERGRAL_0;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = 64;
	channel.fetch_decode.src_height = 64;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 64;
	channel.fetch_decode.dest_height = 64;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	//layer.primary   = IMXDPU_ID_CONSTFRAME0;
	layer.primary = IMXDPU_ID_LAYERBLEND0;

	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_1);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_1, 0xff);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_1, 0, 64);
	//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan,0,32);

#endif
#if 1
	channel.fetch_decode.chan = IMXDPU_CHAN_INTERGRAL_1;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = 64;
	channel.fetch_decode.src_height = 64;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 64;
	channel.fetch_decode.dest_height = 64;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;

	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (0 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_LAYERBLEND1;
	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_2);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_2, 0xff);
	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_2, 0, 256);
	//imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan,128,128);
#endif

#if 1
	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_0;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = 128 + 196;
	channel.fetch_decode.src_height = 128;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 128 + 196;
	channel.fetch_decode.dest_height = 128;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (3 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	//layer.primary   = IMXDPU_ID_CONSTFRAME0;
	layer.primary = IMXDPU_ID_LAYERBLEND2;

	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	layer.stream = IMXDPU_DISPLAY_STREAM_NONE;
	//layer.stream    = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_3);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_3, 0xff);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_3, 196, 128);
	imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan, 0,
				      0);

#endif
#if 1
	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_1;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = 64;
	channel.fetch_decode.src_height = 64;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 64;
	channel.fetch_decode.dest_height = 64;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (0 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	//layer.primary   = IMXDPU_ID_CONSTFRAME0;
	layer.primary = IMXDPU_ID_LAYERBLEND3;

	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_4);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_4, 0xff);

	imxdpu_disp_set_layer_position(imxdpu_id, IMXDPU_LAYER_4, 196, 196);
	imxdpu_disp_set_chan_position(imxdpu_id, channel.fetch_decode.chan, 0,
				      0);

#endif

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test4(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_channel_params_t channel;
	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);

#if 1
	channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_1_2;
	channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.src_width = FW;
	channel.fetch_layer.src_height = FH;
	channel.fetch_layer.clip_width = 0;
	channel.fetch_layer.clip_height = 0;
	channel.fetch_layer.clip_top = 0;
	channel.fetch_layer.clip_left = 0;
	channel.fetch_layer.dest_width = 0;
	channel.fetch_layer.dest_height = 0;
	channel.fetch_layer.dest_top = 0;
	channel.fetch_layer.dest_left = 0;

	channel.fetch_layer.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
	channel.fetch_layer.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
				  0,	// clip_left,
				  128,	// clip_width,
				  128,	// clip_height,
				  128,	// dest_top,
				  0,	// dest_left,
				  256,	// dest_width,
				  256	// dest_height
	    );

#endif
#if 1
	channel.fetch_layer.chan = IMXDPU_CHAN_FRACTIONAL_1_1;
	channel.fetch_layer.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_layer.src_width = FW;
	channel.fetch_layer.src_height = FH;

	channel.fetch_layer.clip_width = FW;
	channel.fetch_layer.clip_height = FH;
	channel.fetch_layer.clip_top = 0;
	channel.fetch_layer.clip_left = 0;
	channel.fetch_layer.dest_width = FW;
	channel.fetch_layer.dest_height = FH;
	channel.fetch_layer.dest_top = 0;
	channel.fetch_layer.dest_left = 0;
	channel.fetch_layer.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	//channel.fetch_layer.use_video_proc = IMXDPU_FALSE;
	channel.fetch_layer.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_layer.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_CONSTFRAME0;
	//layer.primary   = IMXDPU_ID_LAYERBLEND0;

	layer.secondary = get_channel_blk(channel.fetch_layer.chan);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0xff);

	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_layer.chan, 0,	// clip_top,
				  0,	// clip_left,
				  128,	// clip_width,
				  128,	// clip_height,
				  0,	// dest_top,
				  0,	// dest_left,
				  128,	// dest_width,
				  128	// dest_height
	    );

#endif
	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test4_1(unsigned int disp_addr)
{
	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;
	struct imxdpu_soc *imxdpu_inst;

	imxdpu_channel_params_t channel;
	imxdpu_layer_t layer;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_DISABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_setup_constframe(imxdpu_id, imxdpu_display_id, 0, 0, 0x80,
				     0);

	channel.fetch_decode.chan = IMXDPU_CHAN_VIDEO_1;
	channel.fetch_decode.src_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.dest_pixel_fmt = IMXDPU_PIX_FMT_BGRA32;
	channel.fetch_decode.src_width = FW;
	channel.fetch_decode.src_height = FH;

	channel.fetch_decode.clip_width = 0;
	channel.fetch_decode.clip_height = 0;
	channel.fetch_decode.clip_top = 0;
	channel.fetch_decode.clip_left = 0;

	channel.fetch_decode.dest_width = 0;
	channel.fetch_decode.dest_height = 0;
	channel.fetch_decode.dest_top = 0;
	channel.fetch_decode.dest_left = 0;
	channel.fetch_decode.stride =
	    FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_BGRA32);
	channel.fetch_decode.use_video_proc = IMXDPU_FALSE;
	channel.fetch_decode.disp_id = 0;
	imxdpu_init_channel(imxdpu_id, &channel);

	imxdpu_init_channel_buffer(imxdpu_id, channel.fetch_decode.chan, FW * imxdpu_bytes_per_pixel(IMXDPU_PIX_FMT_RGB32), IMXDPU_ROTATE_NONE,	//imxdpu_rotate_mode_t rot_mode,
				   disp_addr + (2 * FW * FH * 4),	//dma_addr_t phyaddr_0,
				   0,	//uint32_t u_offset,
				   0);	//uint32_t v_offset)

	layer.enable = IMXDPU_TRUE;
	layer.primary = IMXDPU_ID_CONSTFRAME0;
	//layer.primary   = IMXDPU_ID_LAYERBLEND0;

	layer.secondary = get_channel_blk(channel.fetch_decode.chan);

	//layer.stream    = IMXDPU_DISPLAY_STREAM_NONE;
	layer.stream = IMXDPU_DISPLAY_STREAM_0;

	imxdpu_disp_setup_layer(imxdpu_id, &layer, IMXDPU_LAYER_0);
	imxdpu_disp_set_layer_global_alpha(imxdpu_id, IMXDPU_LAYER_0, 0xff);
	imxdpu_disp_set_chan_crop(imxdpu_id, channel.fetch_decode.chan, 32,	// clip_top,
				  32,	// clip_left,
				  64,	// clip_width,
				  64,	// clip_height,
				  128,	// dest_top,
				  128,	// dest_left,
				  64,	// dest_width,
				  64	// dest_height
	    );

	imxdpu_inst = imxdpu_get_soc(imxdpu_id);
	imxdpu_request_irq(imxdpu_id,
			   IMXDPU_FRAMEGEN0_INT0_IRQ,
			   (void *)imxdpu_test_irq_handler,
			   0, "test irq", imxdpu_inst);

	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);

	pr_info("IRIS display start ...\n");
}

static void imxdpu_test5(void)
{
/* two ways to setup test mode */
#if 0
	unsigned int h_total, v_total;
	/* horizontal setup */
	h_total = BL_PRE_FW + BL_AFT_FW + HSYNC_PW + FW - 1;
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_HTCFG1),
			(h_total << IMXDPU_FRAMEGEN0_HTCFG2_HSBP_SHIFT) + FW);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_HTCFG2),
			IMXDPU_FRAMEGEN0_HTCFG2_HSEN_MASK +
			((HSYNC_PW + BL_PRE_FW -
			  1) << IMXDPU_FRAMEGEN0_HTCFG2_HSBP_SHIFT) +
			(HSYNC_PW - 1));

	/* vertical setup */
	v_total = BL_PRE_FH + BL_AFT_FH + VSYNC_PW + FH - 1;
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_VTCFG1),
			(v_total << IMXDPU_FRAMEGEN0_VTCFG1_VTOTAL_SHIFT) + FH);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_VTCFG2),
			IMXDPU_FRAMEGEN0_VTCFG2_VSEN_MASK +
			((VSYNC_PW + BL_PRE_FH - 1) <<
			 IMXDPU_FRAMEGEN0_VTCFG2_VSBP_SHIFT) + (VSYNC_PW - 1));

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_SKICKCONFIG),
			IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKEN_MASK +
			((FH -
			  1) << IMXDPU_FRAMEGEN0_SKICKCONFIG_SKICKROW_SHIFT) +
			(FW - 40));

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_DISENGCFG_POLARITYCTRL0),
			IMXDPU_DISENGCFG_POLARITYCTRL0_POLEN0_MASK);

	/* timing controller setup */
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT19_16),
			0x05040302);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT23_20),
			0x09080706);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT11_8), 0x0f0e0d0c);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT15_12),
			0x13121110);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT3_0), 0x19181716);
	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_TCON0_MAPBIT7_4), 0x1d1c1b1a);

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGINCTRL),
			(IMXDPU_FRAMEGEN0_FGINCTRL_FGDM__TEST <<
			 IMXDPU_FRAMEGEN0_FGINCTRL_FGDM_SHIFT));

	iris_raw_writel((IRIS_REGS_BASE + IMXDPU_FRAMEGEN0_FGENABLE),
			IMXDPU_FRAMEGEN0_FGENABLE_FGEN_MASK);
#else

	struct imxdpu_videomode vmode;
	const int imxdpu_id = 0;
	const int imxdpu_display_id = 0;

	vmode.pixelclock = 25175000;
	vmode.hlen = 640;
	vmode.hfp = 16;		//16;
	vmode.hbp = 48;		//48;
	vmode.hsync = 96;

	vmode.vlen = 480;
	vmode.vfp = 10;		//2
	vmode.vbp = 33;
	vmode.vsync = 2;	//2;
	vmode.flags =
	    IMXDPU_DISP_FLAGS_HSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_VSYNC_HIGH |
	    IMXDPU_DISP_FLAGS_DE_HIGH | IMXDPU_DISP_FLAGS_POSEDGE;

	pr_info("Iris static test \n");

	imxdpu_init(imxdpu_id);
	imxdpu_disp_setup_frame_gen(imxdpu_id, imxdpu_display_id,
				    (const struct imxdpu_videomode *)&vmode,
				    0x80, 0, 0, 1, IMXDPU_ENABLE);
	imxdpu_disp_init(imxdpu_id, imxdpu_display_id);
	imxdpu_disp_enable_frame_gen(imxdpu_id, imxdpu_display_id,
				     IMXDPU_ENABLE);
#endif
	pr_info("IRIS display start ...\n");
}

//const color fill
static void imxdpu_be_test7(void)
{
	int i = 0, j = 0;
	unsigned int dst_addr_virt;
	struct be_blit_cfg blit_cfg = { 0 };
	dma_addr_t dst_addr_phy;
	dst_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(8 * 8 * 4),
					     (dma_addr_t *) & dst_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	dst_addr_phy -= 0x10000000;

	printk("%s\n", __FUNCTION__);
	//fetchpersp 4x4 const color 0x11223344 format RGBA
	blit_cfg.fetch_persp.in_pipeline = 1;
	blit_cfg.fetch_persp.control = 0x0;
	blit_cfg.fetch_persp.burst_buf = 0x00000404;
	blit_cfg.fetch_persp.buf_address = 0x0;
	blit_cfg.fetch_persp.buf_attributes = 0x0;
	blit_cfg.fetch_persp.buf_dimension = 0x0;
	blit_cfg.fetch_persp.color_bits = 0x0;
	blit_cfg.fetch_persp.color_shift = 0x0;
	blit_cfg.fetch_persp.layer_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_dimension = 0x00030003;
	blit_cfg.fetch_persp.const_color = 0x11223344;
	blit_cfg.fetch_persp.layer_property = 0x40000110;
	blit_cfg.fetch_persp.frame_dimension = 0x00030003;
	blit_cfg.fetch_persp.frame_resample = 0x00104000;

	//store 8x8 buffer format RGBA input frame 8x8
	blit_cfg.store.in_pipeline = 1;
	blit_cfg.store.control = 0x0;
	blit_cfg.store.burst_buf = 0x0000400;
	blit_cfg.store.buf_address = dst_addr_phy;
	blit_cfg.store.buf_attributes = 0x2000001f;
	blit_cfg.store.buf_dimension = 0x00070007;
	blit_cfg.store.frame_offset = 0x00000000;
	blit_cfg.store.color_bits = 0x08080808;
	blit_cfg.store.color_shift = 0x18100800;

	blit_cfg.engcfg.store9_dynamic = 0x2;
	imxdpu_be_init();
	imxdpu_be_blit(NULL, &blit_cfg);
	imxdpu_be_wait_shadow_load(NULL);
	imxdpu_be_wait_complete(NULL);

	printk("dst virt:%08x dst phy:%08x\n", dst_addr_virt, dst_addr_phy);
	for (i = 0; i < 8; i++) {
		printk("%d ", i);
		for (j = 0; j < 8; j++)
			printk(" %08x ",
			       *(((unsigned int *)dst_addr_virt) + 8 * i + j));
		printk("\n");
	}
}

//copy data
static void imxdpu_be_test8(void)
{
	int i = 0, j = 0;
	struct be_blit_cfg blit_cfg = { 0 };
	unsigned int src_addr_virt;
	dma_addr_t src_addr_phy;
	unsigned int dst_addr_virt;
	dma_addr_t dst_addr_phy;

	printk("%s\n", __FUNCTION__);
	src_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(4 * 4 * 4),
					     (dma_addr_t *) & src_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	dst_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(8 * 8 * 4),
					     (dma_addr_t *) & dst_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	src_addr_phy -= 0x10000000;
	dst_addr_phy -= 0x10000000;
	for (i = 0; i < 4; i++) {
		*(((unsigned int *)src_addr_virt) + 4 * i) = 0x11112222;
		*(((unsigned int *)src_addr_virt) + 4 * i + 1) = 0x33334444;
		*(((unsigned int *)src_addr_virt) + 4 * i + 2) = 0x55556666;
		*(((unsigned int *)src_addr_virt) + 4 * i + 3) = 0x77778888;
	}

	//fetchpersp 4x4 color 0x55667788 format RGBA
	blit_cfg.fetch_persp.in_pipeline = 1;
	blit_cfg.fetch_persp.control = 0x0;
	blit_cfg.fetch_persp.burst_buf = 0x00000404;
	blit_cfg.fetch_persp.buf_address = src_addr_phy;
	blit_cfg.fetch_persp.buf_attributes = 0x0020000f;
	blit_cfg.fetch_persp.buf_dimension = 0x00030003;
	blit_cfg.fetch_persp.color_bits = 0x08080808;
	blit_cfg.fetch_persp.color_shift = 0x18100800;
	blit_cfg.fetch_persp.layer_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_dimension = 0x00030003;
	blit_cfg.fetch_persp.const_color = 0x0;
	blit_cfg.fetch_persp.layer_property = 0xc0000100;
	blit_cfg.fetch_persp.frame_dimension = 0x00030003;
	blit_cfg.fetch_persp.frame_resample = 0x00104000;

	//store 8x8 buffer format RGBA input frame 8x8
	blit_cfg.store.in_pipeline = 1;
	blit_cfg.store.control = 0x0;
	blit_cfg.store.burst_buf = 0x0000400;
	blit_cfg.store.buf_address = dst_addr_phy;
	blit_cfg.store.buf_attributes = 0x2000001f;
	blit_cfg.store.buf_dimension = 0x00070007;
	blit_cfg.store.frame_offset = 0x00000000;
	blit_cfg.store.color_bits = 0x08080808;
	blit_cfg.store.color_shift = 0x18100800;

	blit_cfg.engcfg.store9_dynamic = 0x2;
	imxdpu_be_init();
	imxdpu_be_blit(NULL, &blit_cfg);
	imxdpu_be_wait_shadow_load(NULL);
	imxdpu_be_wait_complete(NULL);

	printk("dst virt:%08x dst phy:%08x\n", dst_addr_virt, dst_addr_phy);
	for (i = 0; i < 8; i++) {
		printk("%d ", i);
		for (j = 0; j < 8; j++)
			printk(" %08x ",
			       *(((unsigned int *)dst_addr_virt) + 8 * i + j));
		printk("\n");
	}
}

//rotation 90 clockwise
static void imxdpu_be_test9(void)
{
	int i = 0, j = 0;
	struct be_blit_cfg blit_cfg = { 0 };
	unsigned int src_addr_virt;
	dma_addr_t src_addr_phy;
	unsigned int dst_addr_virt;
	dma_addr_t dst_addr_phy;

	printk("%s\n", __FUNCTION__);
	src_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(4 * 4 * 4),
					     (dma_addr_t *) & src_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	dst_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(8 * 8 * 4),
					     (dma_addr_t *) & dst_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	src_addr_phy -= 0x10000000;
	dst_addr_phy -= 0x10000000;
	for (i = 0; i < 16; i++) {
		*(((unsigned int *)src_addr_virt) + i) = i;
	}

	//fetchpersp 4x4 format RGBA
	blit_cfg.fetch_persp.in_pipeline = 1;
	blit_cfg.fetch_persp.control = 0x0;
	blit_cfg.fetch_persp.burst_buf = 0x00000404;
	blit_cfg.fetch_persp.buf_address = src_addr_phy;
	blit_cfg.fetch_persp.buf_attributes = 0x0020000f;
	blit_cfg.fetch_persp.buf_dimension = 0x00030003;
	blit_cfg.fetch_persp.color_bits = 0x08080808;
	blit_cfg.fetch_persp.color_shift = 0x18100800;
	blit_cfg.fetch_persp.clip_dimension = 0x00030003;
	blit_cfg.fetch_persp.const_color = 0x0;
	blit_cfg.fetch_persp.layer_property = 0xc0000100;
	blit_cfg.fetch_persp.frame_dimension = 0x00030003;
	//rotation 90 clockwise
	blit_cfg.fetch_persp.layer_offset = 0x7ffd << 16;
	blit_cfg.fetch_persp.clip_offset = 0x7ffd << 16;
	blit_cfg.fetch_persp.frame_resample =
	    0x1 << 24 | 0x4 << 18 | 0x3c << 12;
	//horizontal flip
	//blit_cfg.fetch_persp.layer_offset=0x0<<16 | 0x7ffd;
	//blit_cfg.fetch_persp.clip_offset=0x0<<16 | 0x7ffd;
	//blit_cfg.fetch_persp.frame_resample=0x1<<24 | 0x4 <<18 | 0x3c <<12;

	//store 8x8 buffer format RGBA input frame 8x8
	blit_cfg.store.in_pipeline = 1;
	blit_cfg.store.control = 0x0;
	blit_cfg.store.burst_buf = 0x0000400;
	blit_cfg.store.buf_address = dst_addr_phy;
	blit_cfg.store.buf_attributes = 0x2000001f;
	blit_cfg.store.buf_dimension = 0x00070007;
	blit_cfg.store.frame_offset = 0x00000000;
	blit_cfg.store.color_bits = 0x08080808;
	blit_cfg.store.color_shift = 0x18100800;

	blit_cfg.engcfg.store9_dynamic = 0x2;
	imxdpu_be_init();
	imxdpu_be_blit(NULL, &blit_cfg);
	imxdpu_be_wait_shadow_load(NULL);
	imxdpu_be_wait_complete(NULL);

	printk("dst virt:%08x dst phy:%08x\n", dst_addr_virt, dst_addr_phy);
	for (i = 0; i < 8; i++) {
		printk("%d ", i);
		for (j = 0; j < 8; j++)
			printk(" %08x ",
			       *(((unsigned int *)dst_addr_virt) + 8 * i + j));
		printk("\n");
	}
}

//scale
static void imxdpu_be_test10(void)
{
	int i = 0, j = 0;
	struct be_blit_cfg blit_cfg = { 0 };
	unsigned int src_addr_virt;
	dma_addr_t src_addr_phy;
	unsigned int dst_addr_virt;
	dma_addr_t dst_addr_phy;

	printk("%s\n", __FUNCTION__);
	src_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(4 * 4 * 4),
					     (dma_addr_t *) & src_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	dst_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(8 * 8 * 4),
					     (dma_addr_t *) & dst_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	src_addr_phy -= 0x10000000;
	dst_addr_phy -= 0x10000000;
	for (i = 0; i < 4; i++) {
		*(((unsigned int *)src_addr_virt) + 4 * i) = 0x11112222;
		*(((unsigned int *)src_addr_virt) + 4 * i + 1) = 0x33334444;
		*(((unsigned int *)src_addr_virt) + 4 * i + 2) = 0x55556666;
		*(((unsigned int *)src_addr_virt) + 4 * i + 3) = 0x77778888;
	}

	//fetchpersp 4x4 color format RGBA
	blit_cfg.fetch_persp.in_pipeline = 1;
	blit_cfg.fetch_persp.control = 0x0;
	blit_cfg.fetch_persp.burst_buf = 0x00000404;
	blit_cfg.fetch_persp.buf_address = src_addr_phy;
	blit_cfg.fetch_persp.buf_attributes = 0x0020000f;
	blit_cfg.fetch_persp.buf_dimension = 0x00030003;
	blit_cfg.fetch_persp.color_bits = 0x08080808;
	blit_cfg.fetch_persp.color_shift = 0x18100800;
	blit_cfg.fetch_persp.layer_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_dimension = 0x00030003;
	blit_cfg.fetch_persp.const_color = 0x0;
	blit_cfg.fetch_persp.layer_property = 0xc0000100;
	blit_cfg.fetch_persp.frame_dimension = 0x00030003;
	blit_cfg.fetch_persp.frame_resample = 0x00104000;

	//rop bypass
	blit_cfg.rop.in_pipeline = 1;
	blit_cfg.rop.control = 0x0;
	//matrix bypass
	blit_cfg.matrix.in_pipeline = 1;
	blit_cfg.matrix.control = 0x0;
	//hscaler 2x
	blit_cfg.hscaler.in_pipeline = 1;
	blit_cfg.hscaler.control = 0x7 << 16 | 0x1 << 4 | 0x1;
	blit_cfg.hscaler.setup1 = 0x1 << 18;
	//vscaler 2x
	blit_cfg.vscaler.in_pipeline = 1;
	blit_cfg.vscaler.control = 0x7 << 16 | 0x1 << 8 | 0x1 << 4 | 0x1;
	blit_cfg.vscaler.setup1 = 0x1 << 18;

	//store 8x8 buffer format RGBA input frame 8x8
	blit_cfg.store.in_pipeline = 1;
	blit_cfg.store.control = 0x0;
	blit_cfg.store.burst_buf = 0x0000400;
	blit_cfg.store.buf_address = dst_addr_phy;
	blit_cfg.store.buf_attributes = 0x2000001f;
	blit_cfg.store.buf_dimension = 0x00070007;
	blit_cfg.store.frame_offset = 0x00000000;
	blit_cfg.store.color_bits = 0x08080808;
	blit_cfg.store.color_shift = 0x18100800;

	//cfg engine
	blit_cfg.engcfg.rop9_dynamic = 1 << 24 | 0x2;	//fetchpersp
	blit_cfg.engcfg.matrix9_dynamic = 1 << 24 | 0x4;	//rop
	//up-scaling horizontal  vscaler->hscaler
	//down-scaling horizontal  hscaler->vscaler
	blit_cfg.engcfg.vscaler9_dynamic = 1 << 24 | 0x6;	//matrix
	blit_cfg.engcfg.hscaler9_dynamic = 1 << 24 | 0x8;	//vscaler
	blit_cfg.engcfg.store9_dynamic = 0x7;	//hscaler

	imxdpu_be_init();
	imxdpu_be_blit(NULL, &blit_cfg);
	imxdpu_be_wait_shadow_load(NULL);
	imxdpu_be_wait_complete(NULL);

	printk("dst virt:%08x dst phy:%08x\n", dst_addr_virt, dst_addr_phy);
	for (i = 0; i < 8; i++) {
		printk("%d ", i);
		for (j = 0; j < 8; j++)
			printk(" %08x ",
			       *(((unsigned int *)dst_addr_virt) + 8 * i + j));
		printk("\n");
	}
}

//blend data
static void imxdpu_be_test11(void)
{
	int i = 0, j = 0;
	struct be_blit_cfg blit_cfg = { 0 };
	unsigned int prim_addr_virt;
	dma_addr_t prim_addr_phy;
	unsigned int sec_addr_virt;
	dma_addr_t sec_addr_phy;
	unsigned int dst_addr_virt;
	dma_addr_t dst_addr_phy;

	printk("%s\n", __FUNCTION__);
	prim_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(4 * 4 * 4),
					     (dma_addr_t *) & prim_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	sec_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(4 * 4 * 4),
					     (dma_addr_t *) & sec_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	dst_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL, PAGE_ALIGN(8 * 8 * 4),
					     (dma_addr_t *) & dst_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	prim_addr_phy -= 0x10000000;
	sec_addr_phy -= 0x10000000;
	dst_addr_phy -= 0x10000000;
	for (i = 0; i < 4; i++) {
		*(((unsigned int *)prim_addr_virt) + 4 * i) = 0x11111111;
		*(((unsigned int *)prim_addr_virt) + 4 * i + 1) = 0x22222222;
		*(((unsigned int *)prim_addr_virt) + 4 * i + 2) = 0x33333333;
		*(((unsigned int *)prim_addr_virt) + 4 * i + 3) = 0x44444444;
	}
	for (i = 0; i < 4; i++) {
		*(((unsigned int *)sec_addr_virt) + 4 * i) = 0x55555555;
		*(((unsigned int *)sec_addr_virt) + 4 * i + 1) = 0x66666666;
		*(((unsigned int *)sec_addr_virt) + 4 * i + 2) = 0x77777777;
		*(((unsigned int *)sec_addr_virt) + 4 * i + 3) = 0x88888888;
	}

	//fetchpersp 4x4 color 0x55667788 format RGBA
	blit_cfg.fetch_persp.in_pipeline = 1;
	blit_cfg.fetch_persp.control = 0x0;
	blit_cfg.fetch_persp.burst_buf = 0x00000404;
	blit_cfg.fetch_persp.buf_address = prim_addr_phy;
	blit_cfg.fetch_persp.buf_attributes = 0x0020000f;
	blit_cfg.fetch_persp.buf_dimension = 0x00030003;
	blit_cfg.fetch_persp.color_bits = 0x08080808;
	blit_cfg.fetch_persp.color_shift = 0x18100800;
	blit_cfg.fetch_persp.layer_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_offset = 0x00000000;
	blit_cfg.fetch_persp.clip_dimension = 0x00030003;
	blit_cfg.fetch_persp.const_color = 0x0;
	blit_cfg.fetch_persp.layer_property = 0xc0000100;
	blit_cfg.fetch_persp.frame_dimension = 0x00030003;
	blit_cfg.fetch_persp.frame_resample = 0x00104000;

	//fetchdecode 4x4 color 0x55667788 format RGBA
	blit_cfg.fetch_decode.in_pipeline = 1;
	blit_cfg.fetch_decode.control = 0x0;
	blit_cfg.fetch_decode.burst_buf = 0x00000404;
	blit_cfg.fetch_decode.buf_address = sec_addr_phy;
	blit_cfg.fetch_decode.buf_attributes = 0x0020000f;
	blit_cfg.fetch_decode.buf_dimension = 0x00030003;
	blit_cfg.fetch_decode.color_bits = 0x08080808;
	blit_cfg.fetch_decode.color_shift = 0x18100800;
	blit_cfg.fetch_decode.layer_offset = 0x00000000;
	blit_cfg.fetch_decode.clip_offset = 0x00000000;
	blit_cfg.fetch_decode.clip_dimension = 0x00030003;
	blit_cfg.fetch_decode.const_color = 0x0;
	blit_cfg.fetch_decode.layer_property = 0xc0000100;
	blit_cfg.fetch_decode.frame_dimension = 0x00030003;
	blit_cfg.fetch_decode.frame_resample = 0x00104000;

	//rop bypass
	blit_cfg.rop.in_pipeline = 1;
	blit_cfg.rop.control = 0x0;

	//blitblend
	blit_cfg.blitblend.in_pipeline = 1;
	blit_cfg.blitblend.control = 0x1;
	blit_cfg.blitblend.const_color = 0x00000080;
	blit_cfg.blitblend.red_func = 0x8003 << 16 | 0x8004;
	blit_cfg.blitblend.green_func = 0x8003 << 16 | 0x8004;
	blit_cfg.blitblend.blue_func = 0x8003 << 16 | 0x8004;
	blit_cfg.blitblend.alpha_func = 0x0 << 16 | 0x1;

	//store 8x8 buffer format RGBA input frame 8x8
	blit_cfg.store.in_pipeline = 1;
	blit_cfg.store.control = 0x0;
	blit_cfg.store.burst_buf = 0x0000400;
	blit_cfg.store.buf_address = dst_addr_phy;
	blit_cfg.store.buf_attributes = 0x2000001f;
	blit_cfg.store.buf_dimension = 0x00070007;
	blit_cfg.store.frame_offset = 0x00000000;
	blit_cfg.store.color_bits = 0x08080808;
	blit_cfg.store.color_shift = 0x18100800;

	//cfg engine
	blit_cfg.engcfg.rop9_dynamic = 1 << 24 | 0x2;	//fetchpersp
	blit_cfg.engcfg.blitblend9_dynamic = 1 << 24 | 0x1 << 8 | 0x4;	//vscaler
	blit_cfg.engcfg.store9_dynamic = 0xa;	//hscaler
	imxdpu_be_init();
	imxdpu_be_blit(NULL, &blit_cfg);
	imxdpu_be_wait_shadow_load(NULL);
	imxdpu_be_wait_complete(NULL);

	printk("dst virt:%08x dst phy:%08x\n", dst_addr_virt, dst_addr_phy);
	for (i = 0; i < 8; i++) {
		printk("%d ", i);
		for (j = 0; j < 8; j++)
			printk(" %08x ",
			       *(((unsigned int *)dst_addr_virt) + 8 * i + j));
		printk("\n");
	}
}

static void __init iris_pci_ep_init(void)
{
	void __iomem *p = NULL;
#define  PCIE_EP_CONF_ADDR  0x01F00000
#define  PCIE_RC_CONF_ADDR  0x01FFC000
#define ATU_R_BaseAddress 0x900
#define ATU_VIEWPORT_R (ATU_R_BaseAddress + 0x0)
#define ATU_REGION_CTRL1_R (ATU_R_BaseAddress + 0x4)
#define ATU_REGION_CTRL2_R (ATU_R_BaseAddress + 0x8)
#define ATU_REGION_LOWBASE_R (ATU_R_BaseAddress + 0xC)
#define ATU_REGION_UPBASE_R (ATU_R_BaseAddress + 0x10)
#define ATU_REGION_LIMIT_ADDR_R (ATU_R_BaseAddress + 0x14)
#define ATU_REGION_LOW_TRGT_ADDR_R (ATU_R_BaseAddress + 0x18)
#define ATU_REGION_UP_TRGT_ADDR_R (ATU_R_BaseAddress + 0x1C)
#if 1
	p = ioremap(PCIE_RC_CONF_ADDR, 0x2000);
	if (IS_ERR(p))
		printk("RC mem remap error\n");
	writel(1, p + ATU_VIEWPORT_R);
	writel(0, p + ATU_REGION_CTRL1_R);
	writel(0x01000000, p + ATU_REGION_LOWBASE_R);
	writel(0x0130FFFF, p + ATU_REGION_LIMIT_ADDR_R);
	writel(0, p + ATU_REGION_LOW_TRGT_ADDR_R);
	writel(0x80000000, p + ATU_REGION_CTRL2_R);
	iounmap(p);
#endif
	p = ioremap(PCIE_EP_CONF_ADDR, 0x2000);
	writel(0x00100147, p + 4);
	writel((1 << 31), p + ATU_VIEWPORT_R);
	writel((1 << 31), p + ATU_REGION_CTRL2_R);
	writel(0x7fffffff, p + ATU_REGION_LIMIT_ADDR_R);
	writel(0x08000000, p + ATU_REGION_LOW_TRGT_ADDR_R);
	iounmap(p);
#if 0
	p = ioremap(0x01200000, 0x2000);
	/* Enable EP controller interrupt */
	writel(0xffff, p + 0x50);

	/* For dynamic translate table */
	writel(0x40000000, p + 0x1008);
	iounmap(p);
#endif

	p = ioremap(PCIE_RC_CONF_ADDR, 0x2000);
	if (IS_ERR(p))
		printk("RC mem remap error\n");
	writel(2 | (1 << 31), p + ATU_VIEWPORT_R);
	writel(0, p + ATU_REGION_CTRL1_R);
	writel((1 << 31), p + ATU_REGION_CTRL2_R);
	writel(0x00000000, p + ATU_REGION_LOWBASE_R);
	writel(0x0, p + ATU_REGION_UPBASE_R);
	writel(0x7fffffff, p + ATU_REGION_LIMIT_ADDR_R);	//MEM SPACE: 512MB/1GB
	writel(0x10000000, p + ATU_REGION_LOW_TRGT_ADDR_R);	//NON CACHEABLE
	writel(0, p + ATU_REGION_UP_TRGT_ADDR_R);
	iounmap(p);
}

static irqreturn_t handle_irq(int irq, void *data)
{

	static int int_count = 0;

	int_count++;

#if 0
	{
		static int buff = 0;
		if ((int_count % 2000) == 0) {
			buff++;
			if (buff == 4) {
				buff = 0;
			}
			imxdpu_update_channel_buffer(0,
						     IMXDPU_CHAN_INTERGRAL_0,
						     g_disp_addr +
						     (buff * FW * FH * 4));
		}
	}
#endif
#if TEST_BE
	return imxdpu_isr(irq, data);
#else
	if (((void *)imxdpu_get_soc(0)) == data) {
		if (IMXDPU_TRUE == imxdpu_handle_irq(0)) {
			return IRQ_HANDLED;
		} else {
			pr_warn("%s() nothing to do!\n", __func__);
			return IRQ_NONE;
		}
	} else {
		pr_warn("%s() soc %p != data %p, instance mismatch!\n",
			__func__, imxdpu_get_soc(0), data);
		return IRQ_NONE;
	}
#endif
}

#if TEST_BE
static long imxdpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;
	switch (cmd) {
	case IMXDPU_IOC_BLIT:
		ret = imxdpu_be_load(NULL, argp);
		if (ret)
			return ret;
		break;
	case IMXDPU_IOC_WAIT:
		ret = imxdpu_be_wait(NULL);
		if (ret)
			return ret;
		break;
	default:
		printk(KERN_INFO "imxdpu unknown ioctl: %d\n", cmd);
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations imxdpu_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = imxdpu_ioctl,
};
#endif

static struct miscdevice imxdpu_dev;

static int iris_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int ret = 0;
	unsigned int display_addr_virt;
	dma_addr_t display_addr_phy;
	volatile int test = 1;
#if TEST_BE
	imxdpu_dev.minor = MISC_DYNAMIC_MINOR;
	imxdpu_dev.name = "imxdpu";
	imxdpu_dev.fops = &imxdpu_fops;
	imxdpu_dev.parent = NULL;
	ret = misc_register(&imxdpu_dev);
#endif
	iris_pci_ep_init();

	iris_base = ioremap(IRIS_REGS_BASE_PHY, 0x14000);
	if (!iris_base) {
		pr_err("can't ioremap IRIS PHY base \n");
		goto failed_ioremap;
	}

	display_addr_virt =
	    (unsigned int)dma_alloc_coherent(NULL,
					     PAGE_ALIGN(640 * 480 * 4 * 6),
					     (dma_addr_t *) & display_addr_phy,
					     GFP_DMA | GFP_KERNEL);
	if ((void *)display_addr_virt == NULL) {
		pr_err("dma_alloc_coherent error\n");
		iounmap(iris_base);
		goto failed_ioremap;
	}
	printk("display_addr_virt  %x, display_addr_phy %x\n",
	       display_addr_virt, display_addr_phy);

	/* create images */
	create_color_bar(FH, FW, 0x10, display_addr_virt);
	create_blocks(FH, FW, 0x10, display_addr_virt + (1 * FH * FW * 4));
	copy_image(display_addr_virt + (2 * FH * FW * 4));

	g_disp_addr = display_addr_phy - 0x10000000;
	g_display_addr_virt = display_addr_virt;

#if !TEST_BE
	switch (test) {
	case 0:
		imxdpu_test0(display_addr_phy - 0x10000000);
		break;
	case 1:
		imxdpu_test1(display_addr_phy - 0x10000000);
		break;
	case 2:
		imxdpu_test2(display_addr_phy - 0x10000000);
		break;
	case 21:
		imxdpu_test2_1(display_addr_phy - 0x10000000);
		break;
	case 22:
		imxdpu_test2_2(display_addr_phy - 0x10000000);
		break;
	case 3:
		imxdpu_test3(display_addr_phy - 0x10000000);
		break;
	case 4:
		imxdpu_test4(display_addr_phy - 0x10000000);
		break;
	case 41:
		imxdpu_test4_1(display_addr_phy - 0x10000000);
		break;
	case 7:
		imxdpu_be_test7();
		break;
	case 8:
		imxdpu_be_test8();
		break;
	case 9:
		imxdpu_be_test9();
		break;
	case 10:
		imxdpu_be_test10();
		break;
	case 11:
		imxdpu_be_test11();
		break;
	default:
		imxdpu_test5();
	}
#endif

	ret = request_irq(pdev->irq, handle_irq, IRQF_SHARED, "imx6fpga",
			  imxdpu_get_soc(0) /* instance id */ );
#if TEST_BE
	switch (test) {
	case 0:
		imxdpu_test0(display_addr_phy - 0x10000000);
		break;
	case 1:
		imxdpu_test1(display_addr_phy - 0x10000000);
		break;
	case 2:
		imxdpu_test2(display_addr_phy - 0x10000000);
		break;
	case 21:
		imxdpu_test2_1(display_addr_phy - 0x10000000);
		break;
	case 22:
		imxdpu_test2_2(display_addr_phy - 0x10000000);
		break;
	case 3:
		imxdpu_test3(display_addr_phy - 0x10000000);
		break;
	case 4:
		imxdpu_test4(display_addr_phy - 0x10000000);
		break;
	case 41:
		imxdpu_test4_1(display_addr_phy - 0x10000000);
		break;
	case 7:
		imxdpu_be_test7();
		break;
	case 8:
		imxdpu_be_test8();
		break;
	case 9:
		imxdpu_be_test9();
		break;
	case 10:
		imxdpu_be_test10();
		break;
	case 11:
		imxdpu_be_test11();
		break;
	case 100:
		imxdpu_be_init();
		break;
	default:
		imxdpu_test5();
	}
#endif
	if (ret)
		printk
		    ("Unable to allocate interrupt, Error: %d, pdev->irq %d\n",
		     ret, pdev->irq);

	return ret;

	iris_probed = true;

 failed_ioremap:
	return -1;
}

static struct pci_driver pci_iris_driver = {
	.name = DRIVER_NAME,
	.id_table = iris_pci_tbl,
	.probe = iris_pci_probe,
};

static int __init iris_init(void)
{
	int ret;
	pr_info("Iris Driver\n");

#ifndef  CONFIG_MXC_IMXDPU_FPGA	//CONFIG_IRIS_FPGA
	//ret = platform_driver_register(&iris_driver);
	if (ret)
		return ret;
#else
	pr_info("PCI Iris FPGA Driver\n");
	ret = pci_register_driver(&pci_iris_driver);
	return ret;
#endif
}

static void __exit iris_cleanup(void)
{
	//platform_driver_unregister(&iris_driver);
}

module_exit(iris_cleanup);
module_init(iris_init);

MODULE_LICENSE("GPL");
