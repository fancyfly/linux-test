/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file fsl_csi.c, this file is derived from mx27_csi.c
 *
 * @brief mx25 CMOS Sensor interface functions
 *
 * @ingroup CSI
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/hardware.h>

#include "mxc_v4l2_capture.h"
#include "fsl_csi.h"

static struct csi_config_t g_csi_cfg;	/* csi hardware configuration */
static bool gcsi_mclk_on;
static csi_irq_callback_t g_callback;
static void *g_callback_data;
static struct clk csi_mclk;

static irqreturn_t csi_irq_handler(int irq, void *data)
{
	cam_data *cam = (cam_data *)data;
	unsigned long status = __raw_readl(CSI_CSISR);
	unsigned long cr3 = __raw_readl(CSI_CSICR3);

	cam->still_counter++;
	wake_up_interruptible(&cam->still_queue);
	__raw_writel(status, CSI_CSISR);
	/* reflash the embeded DMA controller */
	__raw_writel(cr3 | (1 << 14), CSI_CSICR3);
	if (g_callback)
		g_callback(g_callback_data, status);

	pr_debug("CSI status = 0x%08lX\n", status);

	return IRQ_HANDLED;
}

static void csihw_set_config(struct csi_config_t *cfg)
{
	unsigned int val = 0;

	/* control reg 1 */
	val |= cfg->swap16_en ? BIT_SWAP16_EN : 0;
	val |= cfg->ext_vsync ? BIT_EXT_VSYNC : 0;
	val |= cfg->eof_int_en ? BIT_EOF_INT_EN : 0;
	val |= cfg->prp_if_en ? BIT_PRP_IF_EN : 0;
	val |= cfg->ccir_mode ? BIT_CCIR_MODE : 0;
	val |= cfg->cof_int_en ? BIT_COF_INT_EN : 0;
	val |= cfg->sf_or_inten ? BIT_SF_OR_INTEN : 0;
	val |= cfg->rf_or_inten ? BIT_RF_OR_INTEN : 0;
	val |= cfg->sff_dma_done_inten ? BIT_SFF_DMA_DONE_INTEN : 0;
	val |= cfg->statff_inten ? BIT_STATFF_INTEN : 0;
	val |= cfg->fb2_dma_done_inten ? BIT_FB2_DMA_DONE_INTEN : 0;
	val |= cfg->fb1_dma_done_inten ? BIT_FB1_DMA_DONE_INTEN : 0;
	val |= cfg->rxff_inten ? BIT_RXFF_INTEN : 0;
	val |= cfg->sof_pol ? BIT_SOF_POL : 0;
	val |= cfg->sof_inten ? BIT_SOF_INTEN : 0;
	val |= cfg->mclkdiv << SHIFT_MCLKDIV;
	val |= cfg->hsync_pol ? BIT_HSYNC_POL : 0;
	val |= cfg->ccir_en ? BIT_CCIR_EN : 0;
	val |= cfg->mclken ? BIT_MCLKEN : 0;
	val |= cfg->fcc ? BIT_FCC : 0;
	val |= cfg->pack_dir ? BIT_PACK_DIR : 0;
	val |= cfg->gclk_mode ? BIT_GCLK_MODE : 0;
	val |= cfg->inv_data ? BIT_INV_DATA : 0;
	val |= cfg->inv_pclk ? BIT_INV_PCLK : 0;
	val |= cfg->redge ? BIT_REDGE : 0;

	__raw_writel(val, CSI_CSICR1);

	/* control reg 3 */
	val = 0x0;
	val |= cfg->dma_reflash_rff ? BIT_DMA_REFLASH_RFF : 0;
	val |= cfg->dma_reflash_sff ? BIT_DMA_REFLASH_SFF : 0;
	val |= cfg->dma_req_en_rff ? BIT_DMA_REQ_EN_RFF : 0;
	val |= cfg->dma_req_en_sff ? BIT_DMA_REQ_EN_SFF : 0;
	val |= cfg->statff_level ? BIT_STATFF_LEVEL : 0;
	val |= cfg->hresp_err_en ? BIT_HRESP_ERR_EN : 0;
	val |= cfg->rxff_level ? BIT_RXFF_LEVEL : 0;
	val |= cfg->two_8bit_sensor ? BIT_TWO_8BIT_SENSOR : 0;
	val |= cfg->zero_pack_en ? BIT_ZERO_PACK_EN : 0;
	val |= cfg->ecc_int_en ? BIT_ECC_INT_EN : 0;
	val |= cfg->ecc_auto_en ? BIT_ECC_AUTO_EN : 0;

	__raw_writel(val, CSI_CSICR3);

	/* rxfifo counter */
	__raw_writel(cfg->rxcnt, CSI_CSIRXCNT);

	/* update global config */
	memcpy(&g_csi_cfg, cfg, sizeof(struct csi_config_t));
}

static void csihw_reset_frame_count(void)
{
	__raw_writel(__raw_readl(CSI_CSICR3) | BIT_FRMCNT_RST, CSI_CSICR3);
}

static void csihw_reset(void)
{
	csihw_reset_frame_count();
	__raw_writel(CSICR1_RESET_VAL, CSI_CSICR1);
	__raw_writel(CSICR2_RESET_VAL, CSI_CSICR2);
	__raw_writel(CSICR3_RESET_VAL, CSI_CSICR3);
}

/*!
 * csi_init_interface
 *    Init csi interface
 */
void csi_init_interface(void)
{
	unsigned int val = 0;
	unsigned int imag_para;

	val |= 1 << 9;
	val |= 0x20000;
	val |= 0x10000;
	val |= 0x2;
	val |= 0x10;
	val |= 0x800;
	val |= (1 << 7);
	val |= 0x100;
	val |= 0x80000000;
	__raw_writel(val, CSI_CSICR1);

	imag_para = (640 << 16) | 960;
	__raw_writel(imag_para, CSI_CSIIMAG_PARA);

	val = 0x1010;
	val |= 0x4000;
	__raw_writel(val, CSI_CSICR3);
}

EXPORT_SYMBOL(csi_init_interface);

/*!
 * csi_enable_prpif
 *    Enable or disable CSI-PrP interface
 * @param       enable        Non-zero to enable, zero to disable
 */
void csi_enable_prpif(uint32_t enable)
{
	if (enable) {
		g_csi_cfg.prp_if_en = 1;
		g_csi_cfg.sof_inten = 0;
		g_csi_cfg.pack_dir = 0;
	} else {
		g_csi_cfg.prp_if_en = 0;
		g_csi_cfg.sof_inten = 1;
		g_csi_cfg.pack_dir = 1;
	}

	csihw_set_config(&g_csi_cfg);
}

EXPORT_SYMBOL(csi_enable_prpif);

/*!
 * csi_enable_mclk
 *
 * @param       src         enum define which source to control the clk
 *                          CSI_MCLK_VF CSI_MCLK_ENC CSI_MCLK_RAW CSI_MCLK_I2C
 * @param       flag        true to enable mclk, false to disable mclk
 * @param       wait        true to wait 100ms make clock stable, false not wait
 *
 * @return      0 for success
 */
int32_t csi_enable_mclk(int src, bool flag, bool wait)
{
	if (flag == true) {
		clk_enable(&csi_mclk);
		if (wait == true)
			msleep(10);
		pr_debug("Enable csi clock from source %d\n", src);
		gcsi_mclk_on = true;
	} else {
		clk_disable(&csi_mclk);
		pr_debug("Disable csi clock from source %d\n", src);
		gcsi_mclk_on = false;
	}

	return 0;
}

EXPORT_SYMBOL(csi_enable_mclk);

/*!
 * csi_read_mclk_flag
 *
 * @return  gcsi_mclk_source
 */
int csi_read_mclk_flag(void)
{
	return 0;
}

EXPORT_SYMBOL(csi_read_mclk_flag);

void csi_start_callback(void *data)
{
	cam_data *cam = (cam_data *)data;

	if (request_irq(MXC_INT_CSI, csi_irq_handler, 0, "csi", cam) < 0)
		pr_debug("CSI error: irq request fail\n");

}
EXPORT_SYMBOL(csi_start_callback);

void csi_stop_callback(void *data)
{
	cam_data *cam = (cam_data *)data;

	free_irq(MXC_INT_CSI, cam);
}
EXPORT_SYMBOL(csi_stop_callback);

static void _mclk_recalc(struct clk *clk)
{
	u32 div;

	div = (__raw_readl(CSI_CSICR1) & BIT_MCLKDIV) >> SHIFT_MCLKDIV;
	div = (div + 1) * 2;

	clk->rate = clk->parent->rate / div;
}

static unsigned long _mclk_round_rate(struct clk *clk, unsigned long rate)
{
	/* Keep CSI divider and change parent clock */
	if (clk->parent->round_rate)
		return clk->parent->round_rate(clk->parent, rate * 2);

	return 0;
}

static int _mclk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = -EINVAL;

	/* Keep CSI divider and change parent clock */
	if (clk->parent->set_rate) {
		ret = clk->parent->set_rate(clk->parent, rate * 2);
		if (ret == 0)
			clk->rate = clk->parent->rate / 2;
	}

	return ret;
}

static int _mclk_enable(struct clk *clk)
{
	__raw_writel(__raw_readl(CSI_CSICR1) | BIT_MCLKEN, CSI_CSICR1);
	return 0;
}

static void _mclk_disable(struct clk *clk)
{
	__raw_writel(__raw_readl(CSI_CSICR1) & ~BIT_MCLKEN, CSI_CSICR1);
}

static struct clk csi_mclk = {
	.name = "csi_clk",
	.recalc = _mclk_recalc,
	.round_rate = _mclk_round_rate,
	.set_rate = _mclk_set_rate,
	.enable = _mclk_enable,
	.disable = _mclk_disable,
};

int32_t __init csi_init_module(void)
{
	int ret = 0;
	struct clk *per_clk;

	per_clk = clk_get(NULL, "csi_clk");
	if (IS_ERR(per_clk))
		return PTR_ERR(per_clk);

	clk_put(per_clk);
	csi_mclk.parent = per_clk;
	clk_register(&csi_mclk);
	clk_enable(per_clk);
	csi_mclk.recalc(&csi_mclk);

	csihw_reset();
	csi_init_interface();

	return ret;
}

void __exit csi_cleanup_module(void)
{
	clk_disable(&csi_mclk);
	clk_unregister(&csi_mclk);
}

module_init(csi_init_module);
module_exit(csi_cleanup_module);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("fsl CSI driver");
MODULE_LICENSE("GPL");
