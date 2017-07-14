/*
 * i.MX drm driver - Northwest Logic MIPI DSI display driver
 *
 * Copyright (C) 2017 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/unaligned.h>

#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_of.h>

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mixel-mipi.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>
#include <video/mipi_display.h>
#include <video/videomode.h>
#include <soc/imx8/sc/sci.h>

#include "imx-drm.h"

#define DRIVER_NAME "imx-mipi-dsi"


#define MIPI_FIFO_TIMEOUT msecs_to_jiffies(250)

/* ------------------------ register offsets ------------------------ */

/* mScale MIPI DSI specific registers */
#define SRC_MIPIPHY_RCR				0x28
#define RESET_BYTE_N				0x2
#define RESET_N					0x4
#define DPI_RESET_N				0x8
#define ESC_RESET_N				0x10
#define PCLK_RESET_N				0x20

/* mScale GPR */
#define IOMUXC_GPR_GPR13			0x34
#define GPR_MIPI_MUX_SEL			0x4

/* 8QM MIPI DSI CSR */
#define MIPI_CSR_TX_ULPS			0x0
#define MIPI_CSR_TX_ULPS_VALUE			0x1F
#define MIPI_CSR_PXL2DPI			0x4

/* 8QXP MIPI DSI CSR */
#define MIPIv2_CSR_TX_ULPS			0x30
#define MIPIv2_CSR_PXL2DPI			0x40

#define MIPI_CSR_PXL2DPI_16_BIT_PACKED		0x0
#define MIPI_CSR_PXL2DPI_16_BIT_565_ALIGNED	0x1
#define MIPI_CSR_PXL2DPI_16_BIT_565_SHIFTED	0x2
#define MIPI_CSR_PXL2DPI_18_BIT_PACKED		0x3
#define MIPI_CSR_PXL2DPI_18_BIT_ALIGNED		0x4
#define MIPI_CSR_PXL2DPI_24_BIT			0x5

/* DSI host registers */
#define HOST_CFG_NUM_LANES		0x0
#define HOST_CFG_NONCONTINUOUS_CLK	0x4
#define HOST_CFG_T_PRE			0x8
#define HOST_CFG_T_POST			0xc
#define HOST_CFG_TX_GAP			0x10
#define HOST_CFG_AUTOINSERT_EOTP	0x14
#define HOST_CFG_EXTRA_CMDS_AFTER_EOTP	0x18
#define HOST_CFG_HTX_TO_COUNT		0x1c
#define HOST_CFG_LRX_H_TO_COUNT		0x20
#define HOST_CFG_BTA_H_TO_COUNT		0x24
#define HOST_CFG_TWAKEUP		0x28
#define HOST_CFG_STATUS_OUT		0x2c
#define HOST_RX_ERROR_STATUS		0x30

/* DSI dpi registers */
#define DPI_PIXEL_PAYLOAD_SIZE		0x200
#define DPI_PIXEL_FIFO_SEND_LEVEL	0x204
#define DPI_INTERFACE_COLOR_CODING	0x208
#define DPI_PIXEL_FORMAT		0x20c
#define DPI_VSYNC_POLARITY		0x210
#define DPI_HSYNC_POLARITY		0x214
#define DPI_VIDEO_MODE			0x218
#define DPI_HFP				0x21c
#define DPI_HBP				0x220
#define DPI_HSA				0x224
#define DPI_ENABLE_MULT_PKTS		0x228
#define DPI_VBP				0x22c
#define DPI_VFP				0x230
#define DPI_BLLP_MODE			0x234
#define DPI_USE_NULL_PKT_BLLP		0x238
#define DPI_VACTIVE			0x23c
#define DPI_VC				0x240

/* DSI APB PKT control */
#define HOST_TX_PAYLOAD			0x280

#define HOST_PKT_CONTROL		0x284
#define HOST_PKT_CONTROL_WC(x)		(((x) & 0xffff) << 0)
#define HOST_PKT_CONTROL_VC(x)		(((x) & 0x3) << 16)
#define HOST_PKT_CONTROL_DT(x)		(((x) & 0x3f) << 18)
#define HOST_PKT_CONTROL_VC_DT(x)	(((x) & 0xff) << 16)
#define HOST_PKT_CONTROL_HS_SEL(x)	(((x) & 0x1) << 24)
#define HOST_PKT_CONTROL_BTA_TX(x)	(((x) & 0x1) << 25)
#define HOST_PKT_CONTROL_BTA_NO_TX(x)	(((x) & 0x1) << 26)

#define HOST_SEND_PACKET		0x288
#define HOST_PKT_STATUS			0x28c
#define HOST_PKT_FIFO_WR_LEVEL		0x290
#define HOST_PKT_FIFO_RD_LEVEL		0x294
#define HOST_PKT_RX_PAYLOAD		0x298

#define HOST_PKT_RX_PKT_HEADER		0x29c
#define HOST_PKT_RX_PKT_HEADER_WC(x)	(((x) & 0xffff) << 0)
#define HOST_PKT_RX_PKT_HEADER_DT(x)	(((x) & 0x3f) << 16)
#define HOST_PKT_RX_PKT_HEADER_VC(x)	(((x) & 0x3) << 22)

/* DSI IRQ handling */
#define HOST_IRQ_STATUS					0x2a0
#define HOST_IRQ_STATUS_SM_NOT_IDLE			BIT(0)
#define HOST_IRQ_STATUS_TX_PKT_DONE			BIT(1)
#define HOST_IRQ_STATUS_DPHY_DIRECTION			BIT(2)
#define HOST_IRQ_STATUS_TX_FIFO_OVFLW			BIT(3)
#define HOST_IRQ_STATUS_TX_FIFO_UDFLW			BIT(4)
#define HOST_IRQ_STATUS_RX_FIFO_OVFLW			BIT(5)
#define HOST_IRQ_STATUS_RX_FIFO_UDFLW			BIT(6)
#define HOST_IRQ_STATUS_RX_PKT_HDR_RCVD			BIT(7)
#define HOST_IRQ_STATUS_RX_PKT_PAYLOAD_DATA_RCVD	BIT(8)
#define HOST_IRQ_STATUS_HOST_BTA_TIMEOUT		BIT(29)
#define HOST_IRQ_STATUS_LP_RX_TIMEOUT			BIT(30)
#define HOST_IRQ_STATUS_HS_TX_TIMEOUT			BIT(31)

#define HOST_IRQ_STATUS2				0x2a4
#define HOST_IRQ_STATUS2_SINGLE_BIT_ECC_ERR		BIT(0)
#define HOST_IRQ_STATUS2_MULTI_BIT_ECC_ERR		BIT(1)
#define HOST_IRQ_STATUS2_CRC_ERR			BIT(2)

#define HOST_IRQ_MASK					0x2a8
#define HOST_IRQ_MASK_SM_NOT_IDLE_MASK			BIT(0)
#define HOST_IRQ_MASK_TX_PKT_DONE_MASK			BIT(1)
#define HOST_IRQ_MASK_DPHY_DIRECTION_MASK		BIT(2)
#define HOST_IRQ_MASK_TX_FIFO_OVFLW_MASK		BIT(3)
#define HOST_IRQ_MASK_TX_FIFO_UDFLW_MASK		BIT(4)
#define HOST_IRQ_MASK_RX_FIFO_OVFLW_MASK		BIT(5)
#define HOST_IRQ_MASK_RX_FIFO_UDFLW_MASK		BIT(6)
#define HOST_IRQ_MASK_RX_PKT_HDR_RCVD_MASK		BIT(7)
#define HOST_IRQ_MASK_RX_PKT_PAYLOAD_DATA_RCVD_MASK	BIT(8)
#define HOST_IRQ_MASK_HOST_BTA_TIMEOUT_MASK		BIT(29)
#define HOST_IRQ_MASK_LP_RX_TIMEOUT_MASK		BIT(30)
#define HOST_IRQ_MASK_HS_TX_TIMEOUT_MASK		BIT(31)

#define HOST_IRQ_MASK2					0x2ac
#define HOST_IRQ_MASK2_SINGLE_BIT_ECC_ERR_MASK		BIT(0)
#define HOST_IRQ_MASK2_MULTI_BIT_ECC_ERR_MASK		BIT(1)
#define HOST_IRQ_MASK2_CRC_ERR_MASK			BIT(2)
/* ------------------------------ end ------------------------------- */

enum transfer_direction {
	DSI_PACKET_SEND,
	DSI_PACKET_RECEIVE
};

struct mipi_dsi_transfer {
	const struct mipi_dsi_msg *msg;
	struct mipi_dsi_packet packet;
	struct list_head list;
	struct completion hw_done;
	struct completion completed;

	int status;    /* status of transmission */
	enum transfer_direction direction;
	u16 rx_word_count;
	size_t tx_len; /* bytes sent */
	size_t rx_len; /* bytes received */
};

struct imx_mipi_dsi {
	struct drm_panel		*panel;
	struct drm_bridge		*bridge;
	struct phy			*phy;
	struct mipi_dsi_host		host;
	struct drm_encoder		encoder;
	struct drm_connector		connector;

	void __iomem			*base;
	struct regmap			*csr;
	struct regmap			*reset;
	struct regmap			*mux_sel;

	int				irq;
	struct clk_settings const	*clk_config;
	struct clk			*clk_pixel;
	struct clk			*clk_rx_esc;
	struct clk			*clk_tx_esc;
	struct clk			*clk_phy_ref;
	struct clk			*clk_bypass;
	struct clk			*clk_dbi;

	enum mipi_dsi_pixel_format	format;
	struct videomode		vm;

	spinlock_t			transfer_lock;
	struct mipi_dsi_transfer	*xfer;

	u32				instance;
	u32				data_rate;
	u32				lanes;
	u32				vc;
	unsigned long			mode_flags;
	bool				enabled;
};

struct clk_setting {
	bool present;
	u32 clk_rate;
};

struct clk_settings {
	struct clk_setting pixel;
	struct clk_setting rx_esc;
	struct clk_setting tx_esc;
	struct clk_setting phy_ref;
	struct clk_setting bypass;
	struct clk_setting dbi;
};

enum imx_ext_regs {
	IMX_REG_CSR = BIT(0),
	IMX_REG_SRC = BIT(1),
	IMX_REG_GPR = BIT(2),
};

struct devtype {
	int (*poweron)(struct imx_mipi_dsi *);
	int (*poweroff)(struct imx_mipi_dsi *);
	struct clk_settings clk_config;
	u32 ext_regs; /* required external register */
	u8 max_instances;
};

static int imx8mq_dsi_poweron(struct imx_mipi_dsi *dsi);
static int imx8mq_dsi_poweroff(struct imx_mipi_dsi *dsi);
static struct devtype imx8mq_dev = {
	.poweron = &imx8mq_dsi_poweron,
	.poweroff = &imx8mq_dsi_poweroff,
	.clk_config = {
		.pixel   = { .present = false },
		.rx_esc  = { .present = true, .clk_rate = 80000000 },
		.tx_esc  = { .present = true, .clk_rate = 20000000 },
		.phy_ref = { .present = true },
		.bypass  = { .present = false },
		.dbi     = { .present = true },
	},
	.ext_regs = IMX_REG_SRC | IMX_REG_GPR,
	.max_instances = 1,
};

static int imx8qm_dsi_poweron(struct imx_mipi_dsi *dsi);
static int imx8qm_dsi_poweroff(struct imx_mipi_dsi *dsi);
static struct devtype imx8qm_dev = {
	.poweron = &imx8qm_dsi_poweron,
	.poweroff = &imx8qm_dsi_poweroff,
	.clk_config = {
		.pixel   = { .present = true },
		.rx_esc  = { .present = true, .clk_rate = 72000000 },
		.tx_esc  = { .present = true, .clk_rate = 18000000 },
		.phy_ref = { .present = false },
		.bypass  = { .present = true, .clk_rate = 27000000 },
		.dbi     = { .present = false },
	},
	.ext_regs = IMX_REG_CSR,
	.max_instances = 2,
};

static int imx8qxp_dsi_poweron(struct imx_mipi_dsi *dsi);
static int imx8qxp_dsi_poweroff(struct imx_mipi_dsi *dsi);
static struct devtype imx8qxp_dev = {
	.poweron = &imx8qxp_dsi_poweron,
	.poweroff = &imx8qxp_dsi_poweroff,
	.clk_config = {
		.pixel   = { .present = true },
		.rx_esc  = { .present = true, .clk_rate = 60000000 },
		.tx_esc  = { .present = true, .clk_rate = 20000000 },
		.phy_ref = { .present = false },
		.bypass  = { .present = true, .clk_rate = 27000000 },
		.dbi     = { .present = false },
	},
	.ext_regs = IMX_REG_CSR,
	.max_instances = 2,
};

/* TODO: Currently enabled only on 8QM; uncomment next platforms after enabling */
static const struct of_device_id imx_dsi_dt_ids[] = {
	{ .compatible = "nxp,imx8qm-mipi-dsi", .data = &imx8qm_dev, },
	/*{ .compatible = "nxp,imx8qxp-mipi-dsi", .data = &imx8qxp_dev, },*/
	/*{ .compatible = "nxp,imx8mq-mipi-dsi", .data = &imx8mq_dev, },*/
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_dsi_dt_ids);

static const char IRQ_NAME[] = "MIPI-DSI";

static inline struct imx_mipi_dsi *encoder_to_dsi(struct drm_encoder *encoder)
{
	return container_of(encoder, struct imx_mipi_dsi, encoder);
}

static inline struct imx_mipi_dsi *connector_to_dsi(struct drm_connector *connector)
{
	return container_of(connector, struct imx_mipi_dsi, connector);
}

static inline struct imx_mipi_dsi *host_to_dsi(struct mipi_dsi_host *host)
{
	return container_of(host, struct imx_mipi_dsi, host);
}

static inline void imx_dsi_write(struct imx_mipi_dsi *dsi, u32 reg, u32 val)
{
	writel(val, dsi->base + reg);
}

static inline u32 imx_dsi_read(struct imx_mipi_dsi *dsi, u32 reg)
{
	return readl(dsi->base + reg);
}

static bool imx_dsi_encoder_mode_fixup(struct drm_encoder *encoder,
				       const struct drm_display_mode *mode,
				       struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void imx_dsi_encoder_mode_set(struct drm_encoder *encoder,
				     struct drm_display_mode *mode,
				     struct drm_display_mode *adjusted)
{
	struct imx_mipi_dsi *dsi = encoder_to_dsi(encoder);

	drm_display_mode_to_videomode(adjusted, &dsi->vm);

	DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "%ux%u @ %lukHz"
		" (HBP:%u HFP:%u HS:%u, VBP:%u VFP:%u VS:%u)\n",
		dsi->vm.hactive, dsi->vm.vactive, dsi->vm.pixelclock / 1000,
		dsi->vm.hback_porch, dsi->vm.hfront_porch, dsi->vm.hsync_len,
		dsi->vm.vback_porch, dsi->vm.vfront_porch, dsi->vm.vsync_len);
}

static void imx_dsi_config_host(struct imx_mipi_dsi *dsi)
{
	imx_dsi_write(dsi, HOST_CFG_NUM_LANES, dsi->lanes - 1);
	imx_dsi_write(dsi, HOST_CFG_NONCONTINUOUS_CLK, 0x00);
	imx_dsi_write(dsi, HOST_CFG_T_PRE, 0x01);
	imx_dsi_write(dsi, HOST_CFG_T_POST, 0x34);
	imx_dsi_write(dsi, HOST_CFG_TX_GAP, 0x0D);
	imx_dsi_write(dsi, HOST_CFG_AUTOINSERT_EOTP, 0x00);
	imx_dsi_write(dsi, HOST_CFG_EXTRA_CMDS_AFTER_EOTP, 0x00);
	imx_dsi_write(dsi, HOST_CFG_HTX_TO_COUNT, 0x00);
	imx_dsi_write(dsi, HOST_CFG_LRX_H_TO_COUNT, 0x00);
	imx_dsi_write(dsi, HOST_CFG_BTA_H_TO_COUNT, 0x00);
	imx_dsi_write(dsi, HOST_CFG_TWAKEUP, 0x3a98);
}

static void imx_dsi_config_dpi(struct imx_mipi_dsi *dsi)
{
	struct videomode *vm = &dsi->vm;
	u32 hfp, hbp, hsa;

	/* TODO: use dsi->format value */
	imx_dsi_write(dsi, DPI_INTERFACE_COLOR_CODING, MIPI_CSR_PXL2DPI_24_BIT);
	imx_dsi_write(dsi, DPI_PIXEL_FORMAT, 0x03);
	imx_dsi_write(dsi, DPI_VSYNC_POLARITY, 0x00);
	imx_dsi_write(dsi, DPI_HSYNC_POLARITY, 0x00);

	if ((dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) &&
	       !(dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)) {
		imx_dsi_write(dsi, DPI_VIDEO_MODE, 0x2);
		imx_dsi_write(dsi, DPI_PIXEL_FIFO_SEND_LEVEL, 8);
		hfp = vm->hfront_porch * 3 - 8;
		hbp = vm->hback_porch * 3 - 14;
		hsa = vm->hsync_len * 3 - 10;
	} else {
		imx_dsi_write(dsi, DPI_VIDEO_MODE, 0x0);
		imx_dsi_write(dsi, DPI_PIXEL_FIFO_SEND_LEVEL, vm->hactive);
		hfp = vm->hfront_porch;
		hbp = vm->hback_porch;
		hsa = vm->hsync_len;
	}

	imx_dsi_write(dsi, DPI_HFP, hfp);
	imx_dsi_write(dsi, DPI_HBP, hbp);
	imx_dsi_write(dsi, DPI_HSA, hsa);

	imx_dsi_write(dsi, DPI_ENABLE_MULT_PKTS, 0x0);
	imx_dsi_write(dsi, DPI_BLLP_MODE, 0x1);
	imx_dsi_write(dsi, DPI_ENABLE_MULT_PKTS, 0x0);
	imx_dsi_write(dsi, DPI_USE_NULL_PKT_BLLP, 0x0);
	imx_dsi_write(dsi, DPI_VC, 0x0);

	imx_dsi_write(dsi, DPI_PIXEL_PAYLOAD_SIZE, vm->hactive);
	imx_dsi_write(dsi, DPI_VACTIVE, vm->vactive - 1);
	imx_dsi_write(dsi, DPI_VBP, vm->vback_porch);
	imx_dsi_write(dsi, DPI_VFP, vm->vfront_porch);
}

static void imx_dsi_set_clocks(struct imx_mipi_dsi *dsi, bool enable)
{
	unsigned long rate;
	/* PHY_REF CLK */
	if (enable && dsi->clk_config->phy_ref.present) {
		clk_set_rate(dsi->clk_phy_ref,
			     dsi->clk_config->phy_ref.clk_rate);
		clk_enable(dsi->clk_phy_ref);
		rate = clk_get_rate(dsi->clk_phy_ref);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev,
			"Enabled PHY_REF clk (rate=%lu)!\n", rate);
	} else if (dsi->clk_config->phy_ref.present) {
		clk_disable(dsi->clk_phy_ref);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "Disabled PHY_REF clk!\n");
	}
	/* BYPASS CLK */
	if (enable && dsi->clk_config->bypass.present) {
		clk_set_rate(dsi->clk_bypass,
			     dsi->clk_config->bypass.clk_rate);
		clk_enable(dsi->clk_bypass);
		rate = clk_get_rate(dsi->clk_bypass);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev,
			"Enabled BYPASS clk (rate=%lu)!\n", rate);
	} else if (dsi->clk_config->bypass.present) {
		clk_disable(dsi->clk_bypass);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "Disabled BYPASS clk!\n");
	}
	/* PIXEL CLK */
	if (enable && dsi->clk_config->pixel.present) {
		clk_set_rate(dsi->clk_pixel,
			     dsi->clk_config->pixel.clk_rate);
		clk_enable(dsi->clk_pixel);
		rate = clk_get_rate(dsi->clk_pixel);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev,
			"Enabled PIXEL clk (rate=%lu)!\n", rate);
	} else if (dsi->clk_config->pixel.present) {
		clk_disable(dsi->clk_pixel);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "Disabled PIXEL clk!\n");
	}
	/* TX_ESC CLK */
	if (enable && dsi->clk_config->tx_esc.present) {
		clk_set_rate(dsi->clk_tx_esc,
			     dsi->clk_config->tx_esc.clk_rate);
		clk_enable(dsi->clk_tx_esc);
		rate = clk_get_rate(dsi->clk_tx_esc);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev,
			"Enabled TX_ESC clk (rate=%lu)!\n", rate);
	} else if (dsi->clk_config->tx_esc.present) {
		clk_disable(dsi->clk_tx_esc);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "Disabled TX_ESC clk!\n");
	}
	/* RX_ESC CLK */
	if (enable && dsi->clk_config->rx_esc.present) {
		clk_set_rate(dsi->clk_rx_esc,
			     dsi->clk_config->rx_esc.clk_rate);
		clk_enable(dsi->clk_rx_esc);
		rate = clk_get_rate(dsi->clk_rx_esc);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev,
			"Enabled TX_ESC clk (rate=%lu)!\n", rate);
	} else if (dsi->clk_config->rx_esc.present) {
		clk_disable(dsi->clk_rx_esc);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "Disabled RX_ESC clk!\n");
	}
	/* DBI CLK */
	if (enable && dsi->clk_config->dbi.present) {
		clk_set_rate(dsi->clk_dbi,
			     dsi->clk_config->dbi.clk_rate);
		clk_enable(dsi->clk_dbi);
		rate = clk_get_rate(dsi->clk_dbi);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev,
			"Enabled DBI clk (rate=%lu)!\n", rate);
	} else if (dsi->clk_config->dbi.present) {
		clk_disable(dsi->clk_dbi);
		DRM_DEV_DEBUG_DRIVER(dsi->host.dev, "Disabled DBI clk!\n");
	}
}

static int imx8mq_dsi_poweron(struct imx_mipi_dsi *dsi)
{
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   PCLK_RESET_N, PCLK_RESET_N);
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   ESC_RESET_N, ESC_RESET_N);
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   RESET_BYTE_N, RESET_BYTE_N);
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   DPI_RESET_N, DPI_RESET_N);

	return 0;
}

static int imx8mq_dsi_poweroff(struct imx_mipi_dsi *dsi)
{
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   PCLK_RESET_N, 0);
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   ESC_RESET_N, 0);
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   RESET_BYTE_N, 0);
	regmap_update_bits(dsi->reset, SRC_MIPIPHY_RCR,
			   DPI_RESET_N, 0);

	return 0;
}

/*
 * v2 is true for QXP
 * On QM, we have 2 DPUs, each one with a MIPI-DSI link
 * On QXP, we have 1 DPU with two MIPI-DSI links
 * Because of this, we will have different initialization
 * paths for MIPI0 and MIPI1 on QM vs QXP
 */
static int imx8q_dsi_poweron(struct imx_mipi_dsi *dsi, bool v2)
{
	struct device *dev = dsi->host.dev;
	int ret = 0;
	sc_err_t sci_err = 0;
	sc_ipc_t ipc_handle = 0;
	u32 inst = dsi->instance;
	u32 mu_id;
	sc_rsrc_t mipi_id, dc_id;
	sc_ctrl_t mipi_ctrl;

	sci_err = sc_ipc_getMuID(&mu_id);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev, "Failed to get MU ID (%d)\n", sci_err);
		ret = -ENODEV;
		goto err_mu;
	}
	sci_err = sc_ipc_open(&ipc_handle, mu_id);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev, "Failed to open IPC (%d)\n", sci_err);
		ret = -ENODEV;
		goto err_mu;
	}

	mipi_id = (inst)?SC_R_MIPI_1:SC_R_MIPI_0;
	if (v2)
		dc_id = SC_R_DC_0;
	else
		dc_id = (inst)?SC_R_DC_1:SC_R_DC_0;

	/* Initialize Pixel Link */
	if (v2)
		mipi_ctrl = inst?SC_C_PXL_LINK_MST2_VLD:SC_C_PXL_LINK_MST1_VLD;
	else
		mipi_ctrl = SC_C_PXL_LINK_MST1_VLD;
	sci_err = sc_misc_set_control(ipc_handle,
				      dc_id,
				      mipi_ctrl,
				      1);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev,
			"Failed to set SC_C_PXL_LINK_MST%d_VLD (%d)\n",
			inst + 1,
			sci_err);
		ret = -ENODEV;
		goto err_ipc;
	}

	if (v2)
		mipi_ctrl = inst?SC_C_SYNC_CTRL1:SC_C_SYNC_CTRL0;
	else
		mipi_ctrl = SC_C_SYNC_CTRL0;
	sci_err = sc_misc_set_control(ipc_handle,
				      dc_id,
				      mipi_ctrl,
				      1);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev,
			"Failed to set SC_C_SYNC_CTRL%d (%d)\n",
			inst,
			sci_err);
		ret = -ENODEV;
		goto err_ipc;
	}

	if (v2)
		mipi_ctrl = inst?SC_C_PXL_LINK_MST2_ADDR:SC_C_PXL_LINK_MST1_ADDR;
	else
		mipi_ctrl = SC_C_PXL_LINK_MST1_ADDR;
	sci_err = sc_misc_set_control(ipc_handle,
				      dc_id,
				      mipi_ctrl,
				      0);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev,
			"Failed to set SC_C_PXL_LINK_MST%d_ADDR (%d)\n",
			inst,
			sci_err);
		ret = -ENODEV;
		goto err_ipc;
	}

	if (v2) {
		sci_err = sc_misc_set_control(ipc_handle,
				mipi_id, SC_C_MODE, 0);
		if (sci_err != SC_ERR_NONE)
			DRM_WARN("Failed to set SC_C_MODE (%d)\n", sci_err);
		sci_err = sc_misc_set_control(ipc_handle,
				mipi_id, SC_C_DUAL_MODE, 0);
		if (sci_err != SC_ERR_NONE)
			DRM_WARN("Failed to set SC_C_DUAL_MODE (%d)\n", sci_err);
		sci_err = sc_misc_set_control(ipc_handle,
				mipi_id, SC_C_PXL_LINK_SEL, inst);
		if (sci_err != SC_ERR_NONE)
			DRM_WARN("Failed to set SC_C_PXL_LINK_SEL (%d)\n", sci_err);
	}

	/* Assert DPI and MIPI bits */
	sci_err = sc_misc_set_control(ipc_handle,
				      mipi_id,
				      SC_C_DPI_RESET,
				      1);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev,
			"Failed to assert DPI reset (%d)\n",
			sci_err);
		ret = -ENODEV;
		goto err_ipc;
	}

	sci_err = sc_misc_set_control(ipc_handle,
				      mipi_id,
				      SC_C_MIPI_RESET,
				      1);
	if (sci_err != SC_ERR_NONE) {
		DRM_DEV_ERROR(dev,
			"Failed to assert MIPI reset (%d)\n",
			sci_err);
		ret = -ENODEV;
		goto err_ipc;
	}

	/* TODO: maybe it's better to use the dsi->format value? */
	if (v2) {
		regmap_write(dsi->csr,
			     MIPIv2_CSR_TX_ULPS,
			     0);
		regmap_write(dsi->csr,
			     MIPIv2_CSR_PXL2DPI,
			     MIPI_CSR_PXL2DPI_24_BIT);
	} else {
		regmap_write(dsi->csr,
			     MIPI_CSR_TX_ULPS,
			     0);
		regmap_write(dsi->csr,
			     MIPI_CSR_PXL2DPI,
			     MIPI_CSR_PXL2DPI_24_BIT);
	}

	sc_ipc_close(ipc_handle);
	return ret;

err_ipc:
	sc_ipc_close(ipc_handle);
err_mu:
	return ret;
}

static int imx8q_dsi_poweroff(struct imx_mipi_dsi *dsi, bool v2)
{
	struct device *dev = dsi->host.dev;
	sc_err_t sci_err = 0;
	sc_ipc_t ipc_handle = 0;
	u32 mu_id;
	u32 inst = dsi->instance;
	sc_rsrc_t mipi_id, dc_id;

	mipi_id = (inst)?SC_R_MIPI_1:SC_R_MIPI_0;
	if (v2)
		dc_id = SC_R_DC_0;
	else
		dc_id = (inst)?SC_R_DC_1:SC_R_DC_0;

	/* Deassert DPI and MIPI bits */
	if (sc_ipc_getMuID(&mu_id) == SC_ERR_NONE &&
	    sc_ipc_open(&ipc_handle, mu_id) == SC_ERR_NONE) {
		sci_err = sc_misc_set_control(ipc_handle,
				mipi_id, SC_C_DPI_RESET, 0);
		if (sci_err != SC_ERR_NONE)
			DRM_DEV_ERROR(dev,
				"Failed to deassert DPI reset (%d)\n",
				sci_err);

		sci_err = sc_misc_set_control(ipc_handle,
				mipi_id, SC_C_MIPI_RESET, 0);
		if (sci_err != SC_ERR_NONE)
			DRM_DEV_ERROR(dev,
				"Failed to deassert MIPI reset (%d)\n",
				sci_err);

		sci_err = sc_misc_set_control(ipc_handle,
				dc_id, SC_C_SYNC_CTRL0, 0);
		if (sci_err != SC_ERR_NONE)
			DRM_DEV_ERROR(dev,
				"Failed to reset SC_C_SYNC_CTRL0 (%d)\n",
				sci_err);

		sci_err = sc_misc_set_control(ipc_handle,
				dc_id, SC_C_PXL_LINK_MST1_VLD, 0);
		if (sci_err != SC_ERR_NONE)
			DRM_DEV_ERROR(dev,
				"Failed to reset SC_C_SYNC_CTRL0 (%d)\n",
				sci_err);
	}

	return 0;
}

static int imx8qm_dsi_poweron(struct imx_mipi_dsi *dsi)
{
	return imx8q_dsi_poweron(dsi, false);
}

static int imx8qm_dsi_poweroff(struct imx_mipi_dsi *dsi)
{
	return imx8q_dsi_poweroff(dsi, false);
}

static int imx8qxp_dsi_poweron(struct imx_mipi_dsi *dsi)
{
	return imx8q_dsi_poweron(dsi, true);
}

static int imx8qxp_dsi_poweroff(struct imx_mipi_dsi *dsi)
{
	return imx8q_dsi_poweroff(dsi, true);
}

static void imx_dsi_init_interrupts(struct imx_mipi_dsi *dsi)
{
	u32 irq_enable;

	imx_dsi_write(dsi, HOST_IRQ_MASK, 0xffffffff);
	imx_dsi_write(dsi, HOST_IRQ_MASK2, 0x7);

	irq_enable = ~(u32)(HOST_IRQ_MASK_TX_PKT_DONE_MASK |
			HOST_IRQ_MASK_RX_PKT_HDR_RCVD_MASK);

	imx_dsi_write(dsi, HOST_IRQ_MASK, irq_enable);
}

static void imx_dsi_encoder_enable(struct drm_encoder *encoder)
{
	struct imx_mipi_dsi *dsi = encoder_to_dsi(encoder);
	struct device *dev = dsi->host.dev;
	const struct of_device_id *of_id = of_match_device(imx_dsi_dt_ids, dev);
	const struct devtype *devtype = of_id->data;
	unsigned long ref_clk, bit_clk;
	int bpp;
	int ret;

	DRM_DEV_INFO(dev, "instance = %u\n", dsi->instance);

	if (dsi->enabled)
		return;

	imx_dsi_set_clocks(dsi, true);

	if (dsi->clk_config->phy_ref.present)
		ref_clk = clk_get_rate(dsi->clk_phy_ref);
	else if (dsi->clk_config->bypass.present)
		ref_clk = clk_get_rate(dsi->clk_bypass);
	else
		ref_clk = 27000000;
	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	bit_clk = (dsi->vm.pixelclock / dsi->lanes) * bpp;
	ret = mixel_phy_mipi_set_phy_speed(dsi->phy, bit_clk, ref_clk);
	if (ret < 0) {
		DRM_ERROR("Cannot setup PHY for current mode: %ux%u @%lukHz\n",
			dsi->vm.hactive, dsi->vm.vactive,
			dsi->vm.pixelclock / 1000);
		return;
	}

	if (dsi->panel && drm_panel_prepare(dsi->panel)) {
		DRM_ERROR("Failed to setup panel\n");
		return;
	}

	ret = devtype->poweron(dsi);
	if (ret < 0) {
		DRM_ERROR("Failed to power on DSI controller (%d)\n", ret);
		return;
	}

	imx_dsi_config_host(dsi);
	imx_dsi_config_dpi(dsi);

	ret = phy_power_on(dsi->phy);
	if (ret < 0) {
		DRM_ERROR("Failed to power on DPHY (%d)\n", ret);
		return;
	}
	imx_dsi_init_interrupts(dsi);


	if (dsi->panel && drm_panel_enable(dsi->panel)) {
		DRM_ERROR("Failed to enable panel\n");
		drm_panel_unprepare(dsi->panel);
		return;
	}

	dsi->enabled = true;
}

static void imx_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct imx_mipi_dsi *dsi = encoder_to_dsi(encoder);
	struct device *dev = dsi->host.dev;
	const struct of_device_id *of_id = of_match_device(imx_dsi_dt_ids, dev);
	const struct devtype *devtype = of_id->data;

	DRM_DEV_INFO(dev, "instance = %u\n", dsi->instance);
	if (!dsi->enabled)
		return;

	if (dsi->panel) {
		if (drm_panel_disable(dsi->panel)) {
			DRM_DEV_ERROR(dev, "failed to disable panel\n");
			return;
		}
		drm_panel_unprepare(dsi->panel);
	}

	phy_power_off(dsi->phy);

	devtype->poweroff(dsi);
	imx_dsi_set_clocks(dsi, false);

	dsi->enabled = false;
}

static enum drm_connector_status imx_dsi_connector_detect(
	struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static int imx_dsi_connector_get_modes(struct drm_connector *connector)
{
	struct imx_mipi_dsi *dsi = connector_to_dsi(connector);

	return drm_panel_get_modes(dsi->panel);
}

static int imx_dsi_encoder_atomic_check(struct drm_encoder *encoder,
					struct drm_crtc_state *crtc_state,
					struct drm_connector_state *conn_state)
{
	struct imx_crtc_state *imx_crtc_state = to_imx_crtc_state(crtc_state);

	imx_crtc_state->bus_format = MEDIA_BUS_FMT_RGB101010_1X30;

	return 0;
}

static const struct drm_encoder_helper_funcs imx_dsi_encoder_helper_funcs = {
	.enable = imx_dsi_encoder_enable,
	.disable = imx_dsi_encoder_disable,
	.mode_fixup = imx_dsi_encoder_mode_fixup,
	.mode_set = imx_dsi_encoder_mode_set,
	.atomic_check = imx_dsi_encoder_atomic_check,
};

static const struct drm_connector_funcs imx_dsi_connector_funcs = {
	.dpms = drm_atomic_helper_connector_dpms,
	.detect = imx_dsi_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
	imx_dsi_connector_helper_funcs = {
	.get_modes = imx_dsi_connector_get_modes,
};

static int imx_dsi_create_connector(struct drm_device *drm,
				    struct imx_mipi_dsi *dsi)
{
	int ret;

	ret = drm_connector_init(drm, &dsi->connector,
				 &imx_dsi_connector_funcs,
				 DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		DRM_ERROR("Failed to init drm connector: %d\n", ret);
		return ret;
	}

	drm_connector_helper_add(&dsi->connector,
				 &imx_dsi_connector_helper_funcs);

	dsi->connector.dpms = DRM_MODE_DPMS_OFF;
	drm_mode_connector_attach_encoder(&dsi->connector, &dsi->encoder);

	if (dsi->panel) {
		ret = drm_panel_attach(dsi->panel, &dsi->connector);
		if (ret) {
			DRM_ERROR("Failed to attach panel: %d\n", ret);
			goto err_connector;
		}
	}

	return 0;

err_connector:
	drm_connector_cleanup(&dsi->connector);
	return ret;
}

static void imx_dsi_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs imx_dsi_encoder_funcs = {
	.destroy = imx_dsi_encoder_destroy,
};

static int imx_dsi_host_attach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	struct imx_mipi_dsi *dsi = host_to_dsi(host);
	struct device *dev = dsi->encoder.dev->dev;

	DRM_DEV_INFO(dev, "instance=%u, lanes=%u, format=0x%x\n",
		     dsi->instance, device->lanes, device->format);

	if (device->lanes < 1 || device->lanes > 4)
		return -EINVAL;

	dsi->lanes = device->lanes;
	dsi->format = device->format;
	dsi->mode_flags = device->mode_flags;

	if (dsi->connector.dev)
		drm_helper_hpd_irq_event(dsi->connector.dev);

	return 0;
}

static int imx_dsi_host_detach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	struct imx_mipi_dsi *dsi = host_to_dsi(host);
	struct device *dev = dsi->encoder.dev->dev;

	DRM_DEV_INFO(dev, "instance = %u\n", dsi->instance);
	if (dsi->connector.dev)
		drm_helper_hpd_irq_event(dsi->connector.dev);

	return 0;
}

static bool imx_dsi_read_packet(struct imx_mipi_dsi *dsi, u32 status)
{
	struct device *dev = dsi->encoder.dev->dev;
	struct mipi_dsi_transfer *xfer = dsi->xfer;
	u8 *payload = xfer->msg->rx_buf;
	u32 val;
	u16 word_count;
	u8 channel;
	u8 data_type;

	if (xfer->rx_word_count == 0) {
		if (!(status & HOST_IRQ_STATUS_RX_PKT_HDR_RCVD))
			return false;
		/* Get the RX header and parse it */
		val = imx_dsi_read(dsi, HOST_PKT_RX_PKT_HEADER);
		word_count = HOST_PKT_RX_PKT_HEADER_WC(val);
		channel	= HOST_PKT_RX_PKT_HEADER_VC(val);
		data_type = HOST_PKT_RX_PKT_HEADER_DT(val);

		if (channel != xfer->msg->channel) {
			DRM_WARN("MIPI-DSI Channel missmatch"
				 " (%u != %u) when reading packet: %u\n",
				 channel, xfer->msg->channel, data_type);
			return true;
		}

		switch (data_type) {
		case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE:
		case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE:
			if (xfer->msg->rx_len > 1) {
				/* read second byte */
				payload[1] = word_count >> 8;
				++xfer->rx_len;
			}
			/* Fall through */
		case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE:
			/* Fall through */
		case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE:
			if (xfer->msg->rx_len > 0) {
				/* read first byte */
				payload[0] = word_count & 0xff;
				++xfer->rx_len;
			}
			return true;
		case MIPI_DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT:
			xfer->status = 0;
			DRM_DEV_ERROR(dev,
				"MIPI-DSI packet error report: 0x%04x\n",
				word_count);
			break;
		}

		if (word_count > xfer->msg->rx_len) {
			DRM_WARN("MIPI-DSI receive buffer is too small:"
				 " %lu (needed: %u)\n",
				 xfer->msg->rx_len, word_count);
			return true;
		}

		xfer->rx_word_count = word_count;
	} else {
		/* Set word_count from previous header read */
		word_count = xfer->rx_word_count;
	}

	/* If RX payload is not yet received, wait for it */
	if (!(status & HOST_IRQ_STATUS_RX_PKT_PAYLOAD_DATA_RCVD))
		return false;

	/* Read the RX payload */
	while (word_count >= 4) {
		val = imx_dsi_read(dsi, HOST_PKT_RX_PAYLOAD);
		payload[0] = (val >>  0) & 0xff;
		payload[1] = (val >>  8) & 0xff;
		payload[2] = (val >> 16) & 0xff;
		payload[3] = (val >> 24) & 0xff;
		payload += 4;
		xfer->rx_len += 4;
		word_count -= 4;
	}

	if (word_count > 0) {
		val = imx_dsi_read(dsi, HOST_PKT_RX_PAYLOAD);
		switch (word_count) {
		case 3:
			payload[2] = (val >> 16) & 0xff;
			++xfer->rx_len;
			/* Fall through */
		case 2:
			payload[1] = (val >>  8) & 0xff;
			++xfer->rx_len;
			/* Fall through */
		case 0:
			payload[0] = (val >>  0) & 0xff;
			++xfer->rx_len;
			break;
		}
	}

	return true;
}

static void imx_dsi_finish_transmission(struct imx_mipi_dsi *dsi, u32 status)
{
	struct mipi_dsi_transfer *xfer = dsi->xfer;
	bool end_packet = false;

	if (!xfer)
		return;

	if (xfer->direction == DSI_PACKET_SEND &&
	    status & HOST_IRQ_STATUS_TX_PKT_DONE) {
		xfer->status = xfer->tx_len;
		end_packet = true;
	} else if (xfer->direction == DSI_PACKET_RECEIVE) {
		/* Read the RX_PKT_HEADER to get the response */
		end_packet = imx_dsi_read_packet(dsi, status);
		if (end_packet)
			xfer->status = xfer->rx_len;
	}

	if (end_packet)
		complete(&xfer->completed);
}

static void imx_dsi_begin_transmission(struct imx_mipi_dsi *dsi)
{
	struct mipi_dsi_transfer *xfer = dsi->xfer;
	struct mipi_dsi_packet *pkt = &xfer->packet;
	const u8 *payload;
	size_t length;
	u16 word_count;
	u8 lp_mode, need_bta;
	u32 val;

	/* Send the payload, if any */
	/* TODO: Need to check the TX FIFO overflow */
	length = pkt->payload_length;
	payload = pkt->payload;
	while (length >= 4) {
		val = get_unaligned_le32(payload);
		imx_dsi_write(dsi, HOST_TX_PAYLOAD, val);
		payload += 4;
		length -= 4;
	}
	/* Send the rest of the payload */
	val = 0;
	switch (length) {
	case 3:
		val |= payload[2] << 16;
		/* Fall through */
	case 2:
		val |= payload[1] << 8;
		/* Fall through */
	case 1:
		val |= payload[0];
		imx_dsi_write(dsi, HOST_TX_PAYLOAD, val);
		break;
	}
	xfer->tx_len = length;

	/*
	 * Now, send the header
	 * header structure is:
	 * header[0] = Virtual Channel + Data Type
	 * header[1] = Word Count LSB
	 * header[2] = Word Count MSB
	 */
	word_count = pkt->header[1] | (pkt->header[2] << 8);
	lp_mode = (xfer->msg->flags & MIPI_DSI_MSG_USE_LPM)?0:1;
	need_bta = (xfer->msg->flags & MIPI_DSI_MSG_REQ_ACK)?1:0;
	val = HOST_PKT_CONTROL_WC(word_count) |
		HOST_PKT_CONTROL_VC_DT(pkt->header[0]) |
		HOST_PKT_CONTROL_HS_SEL(lp_mode) |
		HOST_PKT_CONTROL_BTA_TX(need_bta);
	imx_dsi_write(dsi, HOST_PKT_CONTROL, val);

	/* Send packet command */
	imx_dsi_write(dsi, HOST_SEND_PACKET, 0x1);
}

static ssize_t imx_dsi_host_transfer(struct mipi_dsi_host *host,
				     const struct mipi_dsi_msg *msg)
{
	struct imx_mipi_dsi *dsi = host_to_dsi(host);
	struct mipi_dsi_transfer xfer;
	unsigned long flags;
	ssize_t ret = 0;

	/* Ensure transmission of one packet at a time */
	spin_lock_irqsave(&dsi->transfer_lock, flags);
	/* Create packet to be sent */
	dsi->xfer = &xfer;
	ret = mipi_dsi_create_packet(&xfer.packet, msg);
	if (ret < 0) {
		dsi->xfer = NULL;
		spin_unlock_irqrestore(&dsi->transfer_lock, flags);
		return ret;
	}

	if ((msg->type & MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM ||
	    msg->type & MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM ||
	    msg->type & MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM ||
	    msg->type & MIPI_DSI_DCS_READ) &&
	    msg->rx_len > 0 &&
	    msg->rx_buf != NULL)
		xfer.direction = DSI_PACKET_RECEIVE;
	else
		xfer.direction = DSI_PACKET_SEND;

	xfer.msg = msg;
	xfer.status = -ETIMEDOUT;
	xfer.rx_word_count = 0;
	xfer.rx_len = 0;
	init_completion(&xfer.completed);

	/* Initiate the DSI packet transmision */
	imx_dsi_begin_transmission(dsi);

	wait_for_completion_timeout(&xfer.completed, MIPI_FIFO_TIMEOUT);

	ret = xfer.status;
	if (xfer.status == -ETIMEDOUT)
		dev_err(host->dev, "DSI transfer timed out: 0x%X\n",
			msg->type);

	return ret;
}

static const struct mipi_dsi_host_ops imx_dsi_host_ops = {
	.attach = imx_dsi_host_attach,
	.detach = imx_dsi_host_detach,
	.transfer = imx_dsi_host_transfer,
};

static irqreturn_t imx_dsi_irq_handler(int irq, void *data)
{
	u32 irq_status;
	struct imx_mipi_dsi *dsi = data;

	irq_status = imx_dsi_read(dsi, HOST_IRQ_STATUS);

	if (irq_status & HOST_IRQ_STATUS_TX_PKT_DONE ||
	    irq_status & HOST_IRQ_STATUS_RX_PKT_HDR_RCVD ||
	    irq_status & HOST_IRQ_STATUS_RX_PKT_PAYLOAD_DATA_RCVD)
		imx_dsi_finish_transmission(dsi, irq_status);

	return IRQ_HANDLED;
}

static int imx_dsi_bind(struct device *dev,
			struct device *master,
			void *data)
{
	int ret;
	struct drm_device *drm = data;
	struct imx_mipi_dsi *dsi = dev_get_drvdata(dev);

	DRM_DEV_INFO(dev, "instance = %u\n", dsi->instance);

	ret = imx_drm_encoder_parse_of(drm, &dsi->encoder, dev->of_node);
	if (ret)
		return ret;

	ret = mipi_dsi_host_register(&dsi->host);
	if (ret < 0) {
		dev_err(dev, "failed to register DSI host: %d\n", ret);
		return ret;
	}

	ret = drm_encoder_init(drm, &dsi->encoder, &imx_dsi_encoder_funcs,
			       DRM_MODE_ENCODER_DSI, NULL);
	if (ret) {
		DRM_ERROR("Failed to init encoder to drm\n");
		goto err_host;
	}
	drm_encoder_helper_add(&dsi->encoder, &imx_dsi_encoder_helper_funcs);

	/*
	 * Create the connector. If we have a bridge, attach it and let the
	 * bridge create the connector.
	 */
	if (dsi->bridge != NULL) {
		dsi->bridge->encoder = &dsi->encoder;
		dsi->encoder.bridge = dsi->bridge;
		if (drm_bridge_attach(dsi->encoder.dev, dsi->bridge)) {
			DRM_ERROR("Failed to attach bridge to drm!\n");
			dsi->bridge->encoder = NULL;
			dsi->encoder.bridge = NULL;
		}
	} else {
		ret = imx_dsi_create_connector(drm, dsi);
		if (ret)
			goto err_encoder;
	}

	return 0;

err_encoder:
	drm_encoder_cleanup(&dsi->encoder);
err_host:
	mipi_dsi_host_unregister(&dsi->host);
	return ret;
}

static void imx_dsi_unbind(struct device *dev,
			   struct device *master,
			   void *data)
{
	struct imx_mipi_dsi *dsi = dev_get_drvdata(dev);

	DRM_DEV_INFO(dev, "instance = %u\n", dsi->instance);
	drm_encoder_cleanup(&dsi->encoder);
	/* Skip connector cleanup if creation was delegated to the bridge */
	if (dsi->connector.dev)
		drm_connector_cleanup(&dsi->connector);
	mipi_dsi_host_unregister(&dsi->host);
}

static const struct component_ops imx_dsi_component_ops = {
	.bind	= imx_dsi_bind,
	.unbind	= imx_dsi_unbind,
};

static int imx_dsi_probe(struct platform_device *pdev)
{
	struct imx_mipi_dsi *dsi;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *remote_node, *endpoint;
	const struct of_device_id *of_id = of_match_device(imx_dsi_dt_ids, dev);
	const struct devtype *devtype = of_id->data;
	struct clk *clk;
	u32 instance;
	int ret;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;

	dsi->host.ops = &imx_dsi_host_ops;
	dsi->host.dev = dev;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	while (endpoint) {
		remote_node = of_graph_get_remote_port_parent(endpoint);
		if (!remote_node) {
			dev_err(dev, "No endpoint found!\n");
			return -ENODEV;
		}

		dsi->bridge = of_drm_find_bridge(remote_node);
		dsi->panel = of_drm_find_panel(remote_node);
		of_node_put(remote_node);
		endpoint = of_graph_get_next_endpoint(dev->of_node, endpoint);
	};

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	dsi->base = devm_ioremap_resource(dev, res);
	if (!dsi->base)
		return -ENOMEM;

	dsi->clk_config = &devtype->clk_config;
	if (dsi->clk_config->pixel.present) {
		clk = devm_clk_get(dev, "pixel");
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_err(dev, "Failed to get pixel clock: %d\n", ret);
			return ret;
		}
		clk_prepare(clk);
		dsi->clk_pixel = clk;
	}

	if (dsi->clk_config->tx_esc.present) {
		clk = devm_clk_get(dev, "tx_esc");
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_err(dev, "Failed to get tx_esc clock: %d\n", ret);
			return ret;
		}
		clk_prepare(clk);
		dsi->clk_tx_esc = clk;
	}

	if (dsi->clk_config->rx_esc.present) {
		clk = devm_clk_get(dev, "rx_esc");
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_err(dev, "Failed to get rx_esc clock: %d\n", ret);
			return ret;
		}
		clk_prepare(clk);
		dsi->clk_rx_esc = clk;
	}

	if (dsi->clk_config->phy_ref.present) {
		clk = devm_clk_get(dev, "phy_ref");
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_err(dev, "Failed to get phy_ref clock: %d\n", ret);
			return ret;
		}
		clk_prepare(clk);
		dsi->clk_phy_ref = clk;
	}

	if (dsi->clk_config->bypass.present) {
		clk = devm_clk_get(dev, "bypass");
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_err(dev, "Failed to get bypass clock: %d\n", ret);
			return ret;
		}
		clk_prepare(clk);
		dsi->clk_bypass = clk;
	}

	if (dsi->clk_config->dbi.present) {
		clk = devm_clk_get(dev, "dbi");
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			dev_err(dev, "Failed to get dbi clock: %d\n", ret);
			return ret;
		}
		clk_prepare(clk);
		dsi->clk_dbi = clk;
	}

	dsi->csr = syscon_regmap_lookup_by_phandle(dev->of_node, "csr");
	if (IS_ERR(dsi->csr) && (devtype->ext_regs & IMX_REG_CSR)) {
		ret = PTR_ERR(dsi->csr);
		dev_err(dev, "Failed to get CSR regmap (%d)\n", ret);
		return ret;
	}
	dsi->reset = syscon_regmap_lookup_by_phandle(dev->of_node, "reset");
	if (IS_ERR(dsi->reset) && (devtype->ext_regs & IMX_REG_SRC)) {
		ret = PTR_ERR(dsi->reset);
		dev_err(dev, "Failed to get SRC regmap (%d)\n", ret);
		return ret;
	}
	dsi->mux_sel = syscon_regmap_lookup_by_phandle(dev->of_node, "mux-sel");
	if (IS_ERR(dsi->mux_sel) && (devtype->ext_regs & IMX_REG_GPR)) {
		ret = PTR_ERR(dsi->mux_sel);
		dev_err(dev, "Failed to get GPR regmap (%d)\n", ret);
		return ret;
	}
	if (!IS_ERR(dsi->mux_sel)) {
		dev_info(dev, "Selecting DCSS as input source...\n");
		regmap_update_bits(dsi->mux_sel,
				   IOMUXC_GPR_GPR13,
				   GPR_MIPI_MUX_SEL,
				   0x1);
	}

	dsi->phy = devm_phy_get(dev, "dphy");
	if (IS_ERR(dsi->phy)) {
		ret = PTR_ERR(dsi->phy);
		if (ret == -EPROBE_DEFER)
			dev_warn(dev, "Waiting for PHY...\n");
		else
			dev_err(dev, "Could not get PHY (%d)\n", ret);
		return ret;
	}

	if (devtype->max_instances > 1) {
		ret = of_property_read_u32(dev->of_node, "instance", &instance);
		if (ret < 0 && (devtype->max_instances > 1)) {
			dev_warn(dev,
				"Missing instance property! Default to 0!\n");
			instance = 0;
		}
		if (instance >= devtype->max_instances) {
			dev_warn(dev,
				"Invalid DSI instance: %d! Default to 0!\n",
				 instance);
			instance = 0;
		}
		dsi->instance = instance;
	}

	ret = of_property_read_u32(dev->of_node, "data_lanes", &dsi->lanes);
	if (ret < 0) {
		dev_warn(dev,
			"Failed to get DSI data_lanes. Defaulting to 4\n");
		dsi->lanes = 4;
	}

	dsi->irq = platform_get_irq(pdev, 0);
	if (dsi->irq < 0) {
		dev_err(dev, "Failed to get device IRQ!\n");
		return -EINVAL;
	}
	ret = devm_request_irq(dev, dsi->irq,
			imx_dsi_irq_handler, 0, IRQ_NAME, dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to request IRQ: %d (%d)\n", dsi->irq, ret);
		return ret;
	}
	platform_set_drvdata(pdev, dsi);
	dev_dbg(dev, "MIPI-DSI probed!\n");

	return component_add(&pdev->dev, &imx_dsi_component_ops);
}

static int imx_dsi_remove(struct platform_device *pdev)
{
	struct imx_mipi_dsi *dsi = platform_get_drvdata(pdev);

	imx_dsi_encoder_disable(&dsi->encoder);

	component_del(&pdev->dev, &imx_dsi_component_ops);
	return 0;
}

static struct platform_driver imx_dsi_driver = {
	.probe		= imx_dsi_probe,
	.remove		= imx_dsi_remove,
	.driver		= {
		.of_match_table = imx_dsi_dt_ids,
		.name	= DRIVER_NAME,
	},
};

module_platform_driver(imx_dsi_driver);

MODULE_AUTHOR("NXP Semiconductor");
MODULE_DESCRIPTION("i.MX Northwest Logic MIPI-DSI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
