/*
 * Copyright 2017 NXP
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mixel-mipi.h>
#include <linux/platform_device.h>
#include <soc/imx8/sc/sci.h>

#define DPHY_PD_DPHY			0x00
#define DPHY_M_PRG_HS_PREPARE		0x04
#define DPHY_MC_PRG_HS_PREPARE		0x08
#define DPHY_M_PRG_HS_ZERO		0x0c
#define DPHY_MC_PRG_HS_ZERO		0x10
#define DPHY_M_PRG_HS_TRAIL		0x14
#define DPHY_MC_PRG_HS_TRAIL		0x18
#define DPHY_PD_PLL			0x1c
#define DPHY_TST			0x20
#define DPHY_CN				0x24
#define DPHY_CM				0x28
#define DPHY_CO				0x2c
#define DPHY_LOCK			0x30
#define DPHY_LOCK_BYP			0x34
#define DPHY_TX_RCAL			0x38
#define DPHY_AUTO_PD_EN			0x3c
#define DPHY_RXLPRP			0x40
#define DPHY_RXCDRP			0x44

#define CN_BUF	0xCB7A89C0
#define CO_BUF	0x63
#define CM(x)	( \
		(x <  32)?0xE0|(x-16) : \
		(x <  64)?0xC0|(x-32) : \
		(x < 128)?0x80|(x-64) : \
		(x - 128))
#define CN(x)	((x == 1)?0x1F : (((CN_BUF)>>(x-1))&0x1F))
#define CO(x)	((CO_BUF)>>(8-x)&0x3)

struct pll_divider {
	u32 cm;
	u32 cn;
	u32 co;
};

struct mixel_mipi_phy_priv {
	struct device *dev;
	void __iomem *base;
	bool have_sc;
	sc_rsrc_t mipi_id;
	struct pll_divider divider;
	struct mutex lock;
};

static inline u32 phy_read(struct phy *phy, unsigned int reg)
{
	struct mixel_mipi_phy_priv *priv = phy_get_drvdata(phy);

	return readl(priv->base + reg);
}

static inline void phy_write(struct phy *phy, u32 value, unsigned int reg)
{
	struct mixel_mipi_phy_priv *priv = phy_get_drvdata(phy);

	writel(value, priv->base + reg);
}

int mixel_phy_mipi_set_phy_speed(struct phy *phy,
				 unsigned long bit_clk,
				 unsigned long ref_clk)
{
	struct mixel_mipi_phy_priv *priv = dev_get_drvdata(phy->dev.parent);
	u32 div_rate;
	u32 numerator = 0;
	u32 denominator = 1;

	/* simulated fixed point with 3 decimals */
	div_rate = (bit_clk * 1000) / ref_clk;

	while (denominator <= 256) {
		if (div_rate % 1000 == 0) {
			numerator = div_rate / 1000;
			break;
		}
		denominator = denominator << 1;
		div_rate = div_rate << 1;
	}

	/* CM ranges between 16 and 255 */
	/* CN ranges between 1 and 32 */
	/* CO is power of 2: 1, 2, 4, 8 */

	if (numerator == 0 || numerator < 16 || numerator > 255) {
		dev_err(&phy->dev, "Failed to set phy speed to %lu\n",
			bit_clk);
		return -EINVAL;
	}

	priv->divider.cn = 1;
	if (denominator > 8) {
		priv->divider.cn = denominator >> 3;
		denominator = 8;
	}
	priv->divider.co = denominator;
	priv->divider.cm = numerator;

	return 0;
}
EXPORT_SYMBOL_GPL(mixel_phy_mipi_set_phy_speed);

static int mixel_mipi_phy_reset(struct phy *phy, u32 reset)
{
	struct mixel_mipi_phy_priv *priv = phy_get_drvdata(phy);
	sc_err_t sci_err = 0;
	sc_ipc_t ipc_handle = 0;
	u32 mu_id;

	sci_err = sc_ipc_getMuID(&mu_id);
	if (sci_err != SC_ERR_NONE) {
		dev_err(&phy->dev, "Failed to get MU ID (%d)\n", sci_err);
		return -ENODEV;
	}
	sci_err = sc_ipc_open(&ipc_handle, mu_id);
	if (sci_err != SC_ERR_NONE) {
		dev_err(&phy->dev, "Failed to open IPC (%d)\n", sci_err);
		return -ENODEV;
	}

	sci_err = sc_misc_set_control(ipc_handle,
				      priv->mipi_id,
				      SC_C_PHY_RESET,
				      reset);
	if (sci_err != SC_ERR_NONE) {
		dev_err(&phy->dev, "Failed to reset DPHY (%d)\n", sci_err);
		sc_ipc_close(ipc_handle);
		return -ENODEV;
	}

	sc_ipc_close(ipc_handle);

	return 0;
}

static int mixel_mipi_phy_power_on(struct phy *phy)
{
	struct mixel_mipi_phy_priv *priv = phy_get_drvdata(phy);
	u32 lock, timeout;
	int ret = 0;

	mutex_lock(&priv->lock);

	phy_write(phy, 0x00, DPHY_M_PRG_HS_PREPARE);
	phy_write(phy, 0x00, DPHY_MC_PRG_HS_PREPARE);
	phy_write(phy, 0x09, DPHY_M_PRG_HS_ZERO);
	phy_write(phy, 0x20, DPHY_MC_PRG_HS_ZERO);
	phy_write(phy, 0x05, DPHY_M_PRG_HS_TRAIL);
	phy_write(phy, 0x05, DPHY_MC_PRG_HS_TRAIL);
	phy_write(phy, 0x00, DPHY_LOCK_BYP);
	phy_write(phy, 0x01, DPHY_TX_RCAL);
	phy_write(phy, 0x00, DPHY_AUTO_PD_EN);
	phy_write(phy, 0x01, DPHY_RXLPRP);
	phy_write(phy, 0x01, DPHY_RXCDRP);
	phy_write(phy, 0x00, DPHY_PD_DPHY);

	/* VCO = REF_CLK * CM / CN * CO */
	if (priv->divider.cm < 16 || priv->divider.cm > 255 ||
		priv->divider.cn < 1 || priv->divider.cn > 32 ||
		priv->divider.co < 1 || priv->divider.co > 8) {
		dev_err(&phy->dev, "Invalid CM/CN/CO values! (%u/%u/%u)\n",
			priv->divider.cm, priv->divider.cn, priv->divider.co);
		return -EINVAL;
	}
	phy_write(phy, CM(priv->divider.cm), DPHY_CM);
	phy_write(phy, CN(priv->divider.cn), DPHY_CN);
	phy_write(phy, CO(priv->divider.co), DPHY_CO);
	phy_write(phy, 0x25, DPHY_TST);
	phy_write(phy, 0x00, DPHY_PD_PLL);

	timeout = 100;
	while (!(lock = phy_read(phy, DPHY_LOCK))) {
		udelay(10);
		if (--timeout == 0) {
			dev_err(&phy->dev, "Could not get DPHY lock!\n");
			mutex_unlock(&priv->lock);
			return -EINVAL;
		}
	}

	if (priv->have_sc)
		ret = mixel_mipi_phy_reset(phy, 1);

	mutex_unlock(&priv->lock);

	return ret;
}

static int mixel_mipi_phy_power_off(struct phy *phy)
{
	struct mixel_mipi_phy_priv *priv = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&priv->lock);

	phy_write(phy, 0x01, DPHY_PD_PLL);
	phy_write(phy, 0x01, DPHY_PD_DPHY);

	if (priv->have_sc)
		ret = mixel_mipi_phy_reset(phy, 0);

	mutex_unlock(&priv->lock);

	return ret;
}

static const struct phy_ops mixel_mipi_phy_ops = {
	.power_on = mixel_mipi_phy_power_on,
	.power_off = mixel_mipi_phy_power_off,
	.owner = THIS_MODULE,
};

static int mixel_mipi_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *res;
	struct phy_provider *phy_provider;
	struct mixel_mipi_phy_priv *priv;
	struct phy *phy;
	u32 phy_id = 0;

	if (!np)
		return -ENODEV;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	priv->base = devm_ioremap(dev, res->start, SZ_256);
	if (!priv->base)
		return -ENOMEM;

	priv->have_sc = of_property_read_bool(dev->of_node, "use_sc");
	if (priv->have_sc) {
		if (of_property_read_u32(np, "id", &phy_id) < 0) {
			dev_warn(dev,
				"Failed to get DPHY instance. Defaulting to 0\n");
			phy_id = 0;
		}
		if (phy_id > 1) {
			dev_err(dev, "Invalid DPHY id: %d\n", phy_id);
			return -EINVAL;
		}
		priv->mipi_id = phy_id?SC_R_MIPI_1:SC_R_MIPI_0;
	}

	priv->dev = dev;

	mutex_init(&priv->lock);
	dev_set_drvdata(dev, priv);

	phy = devm_phy_create(dev, np, &mixel_mipi_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "Failed to create phy\n");
		return PTR_ERR(phy);
	}
	phy_set_drvdata(phy, priv);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id mixel_mipi_phy_of_match[] = {
	{ .compatible = "mixel,mipi-phy" },
	{}
};
MODULE_DEVICE_TABLE(of, mixel_mipi_phy_of_match);

static struct platform_driver mixel_mipi_phy_driver = {
	.probe	= mixel_mipi_phy_probe,
	.driver = {
		.name = "mixel-mipi-phy",
		.of_match_table	= mixel_mipi_phy_of_match,
	}
};
module_platform_driver(mixel_mipi_phy_driver);

MODULE_AUTHOR("NXP Semiconductor");
MODULE_DESCRIPTION("Mixel MIPI PHY driver");
MODULE_LICENSE("GPL v2");
