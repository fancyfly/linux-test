/*
 * drivers/i2c/busses/i2c-lp-imx.c
 *
 * Copyright 2016, Freescale Semiconductots Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/module.h>

#define LPI2C_VERID	0x00
#define LPI2C_PARAM	0x04
#define LPI2C_MCR	0x10
#define LPI2C_MSR	0x14
#define LPI2C_MIER	0x18
#define LPI2C_MDER	0x1C
#define LPI2C_MCFGR0	0x20
#define LPI2C_MCFGR1	0x24
#define LPI2C_MCFGR2	0x28
#define LPI2C_MCFGR3	0x2C
#define LPI2C_MDMR	0x40
#define LPI2C_MCCR0	0x48
#define LPI2C_MCCR1	0x50
#define LPI2C_MFCR	0x58
#define LPI2C_MFSR	0x5C
#define LPI2C_MTDR	0x60
#define LPI2C_MRDR	0x70

#define LPI2C_MSR_BBF		(1u << 25)
#define LPI2C_MSR_MBF		(1u << 24)
#define LPI2C_MSR_DMF		(1u << 14)
#define LPI2C_MSR_PLTF		(1u << 13)
#define LPI2C_MSR_FEF		(1u << 12)
#define LPI2C_MSR_ALF		(1u << 11)
#define LPI2C_MSR_NDF		(1u << 10)
#define LPI2C_MSR_SDF		(1u << 9)
#define LPI2C_MSR_EPF		(1u << 8)
#define LPI2C_MSR_RDF		(1u << 1)
#define LPI2C_MSR_TDF		(1u << 0)
#define LPI2C_MCR_RRF		(1u << 9)
#define LPI2C_MCR_RTF		(1u << 8)
#define LPI2C_MCR_DBGEN		(1u << 3)
#define LPI2C_MCR_DOZEN		(1u << 2)
#define LPI2C_MCR_RST		(1u << 1)
#define LPI2C_MCR_MEN		(1u << 0)
#define LPI2C_MCFGR0_HRSEL	(1u << 2)
#define LPI2C_MCFGR0_HRPOL	(1u << 1)
#define LPI2C_MCFGR0_HREN	(1u << 0)
#define LPI2C_MCFGR1_IGNACK	(1u << 9)
#define LPI2C_MCFGR1_AUTOSTOP	(1u << 8)
#define LPI2C_MIER_DMIE		(1u << 14)
#define LPI2C_MIER_PLTIE	(1u << 13)
#define LPI2C_MIER_FEIE		(1u << 12)
#define LPI2C_MIER_ALIE		(1u << 11)
#define LPI2C_MIER_NDIE		(1u << 10)
#define LPI2C_MIER_SDIE		(1u << 9)
#define LPI2C_MIER_EPIE		(1u << 8)
#define LPI2C_MIER_RDIE		(1u << 1)
#define LPI2C_MIER_TDIE		(1u << 0)
#define LPI2C_MRDR_RXEMPTY	(1u << 14)

enum lpi2c_err_type {
	LPI2C_ERR_NONE,
	LPI2C_ERR_BUSY,
	LPI2C_ERR_TDF,
	LPI2C_ERR_RDF,
	LPI2C_ERR_EPF,
	LPI2C_ERR_SDF,
	LPI2C_ERR_NDF,
	LPI2C_ERR_ALF,
	LPI2C_ERR_FEF,
	LPI2C_ERR_PLTF,
	LPI2C_ERR_DMF,
	LPI2C_ERR_MBF,
	LPI2C_ERR_BBF,
	LPI2C_ERR_TIMEOUT,
};

enum lpi2c_msg_type {
	LPI2C_MSG_STOP,
	LPI2C_MSG_REPEAT_STAR,
	LPI2C_MSG_CONTINUE,
};

#define LPI2C_BITRATE		100000 /* 100KHz */
#define LPI2C_TIMEOUT		(msecs_to_jiffies(1000))
#define LPI2C_FIFO_SIZE 4
#define LPI2C_MTDR_CMD(x) \
	(((u32)(((u32)(x))) << 8u)) & 0x700u
#define LPI2C_MTDR_DATA(x) \
	(((u32)(((u32)(x))) << 0u)) & 0xffu
#define LPI2C_MCFGR1_PINCFG(x) \
	(((u32)(((u32)(x))) << 24u)) & 0x7000000u

struct lpi2c_imx_dev {
	struct device *dev;
	struct i2c_adapter adapter;
	struct clk *clk;
	void __iomem *base;
	unsigned int bitrate;
	int irq;
	bool irq_disabled;
	struct completion complete;
	int status;
	u8 *msg_buf;
	size_t msg_left;
	int msg_read;
};

static int lpi2c_imx_init(struct lpi2c_imx_dev *i2c_dev);

static inline void i2c_writel(struct lpi2c_imx_dev *i2c_dev, u32 val,
	unsigned long reg)
{
	writel(val, i2c_dev->base + reg);
}

static inline unsigned char i2c_readl(struct lpi2c_imx_dev *i2c_dev,
	unsigned long reg)
{
	return readl(i2c_dev->base + reg);
}

static void lpi2c_imx_unmask_irq(struct lpi2c_imx_dev *i2c_dev, u32 mask)
{
	u32 reg;

	reg = i2c_readl(i2c_dev, LPI2C_MIER);
	reg |= mask;
	i2c_writel(i2c_dev, reg, LPI2C_MIER);
}

static void lpi2c_imx_mask_irq(struct lpi2c_imx_dev *i2c_dev, u32 mask)
{
	u32 reg;

	reg = i2c_readl(i2c_dev, LPI2C_MIER);
	reg &= ~mask;
	i2c_writel(i2c_dev, reg, LPI2C_MIER);
}


static int lpi2c_imx_empty_rx_fifo(struct lpi2c_imx_dev *i2c_dev)
{
	dev_info(i2c_dev->dev, "i2c: on %s\n", __func__);
	return 0;
}

static int lpi2c_imx_fill_tx_fifo(struct lpi2c_imx_dev *i2c_dev)
{
	dev_info(i2c_dev->dev, "i2c: on %s\n", __func__);
	return 0;
}

static irqreturn_t lpi2c_imx_isr(int irq, void *dev_id)
{
	u32 status;
	const u32 irq_error = LPI2C_MSR_NDF | LPI2C_MSR_ALF;
	struct lpi2c_imx_dev *i2c_dev = dev_id;

	status = i2c_readl(i2c_dev, LPI2C_MSR);

	if (unlikely(status & irq_error)) {
		if (status & LPI2C_MSR_NDF)
			i2c_dev->status |= LPI2C_ERR_NDF;
		if (status & LPI2C_MSR_ALF)
			i2c_dev->status |= LPI2C_ERR_ALF;
		goto error;
	}

	/* receive */
	if ((status & LPI2C_MSR_RDF) && i2c_dev->msg_read) {
		if (i2c_dev->msg_left)
			lpi2c_imx_empty_rx_fifo(i2c_dev);
	}

	/* transmit */
	if ((status & LPI2C_MSR_TDF) && !i2c_dev->msg_read) {
		if (i2c_dev->msg_left)
			lpi2c_imx_fill_tx_fifo(i2c_dev);
		else
			lpi2c_imx_mask_irq(i2c_dev, LPI2C_MIER_TDIE);
	}

	/* clear irq */

	i2c_writel(i2c_dev, status, LPI2C_MSR);
	complete(&i2c_dev->complete);

	return IRQ_HANDLED;

error:
	/* mask all interrupts */
	lpi2c_imx_mask_irq(i2c_dev, LPI2C_MIER_DMIE | LPI2C_MIER_PLTIE |
			LPI2C_MIER_FEIE | LPI2C_MIER_ALIE | LPI2C_MIER_NDIE |
			LPI2C_MIER_SDIE | LPI2C_MIER_EPIE | LPI2C_MIER_RDIE |
			LPI2C_MIER_TDIE);
	i2c_writel(i2c_dev, status, LPI2C_MSR);
	complete(&i2c_dev->complete);

	return IRQ_HANDLED;
}

static int lpi2c_imx_check_busy_bus(struct lpi2c_imx_dev *i2c_dev)
{
	u32 reg;

	reg = i2c_readl(i2c_dev, LPI2C_MSR);
	if ((reg & LPI2C_MSR_BBF) && !(reg & LPI2C_MSR_MBF))
		return LPI2C_ERR_BUSY;

	return 0;
}

static int lpi2c_imx_check_clear_error(struct lpi2c_imx_dev *i2c_dev)
{
	u32 status, reg;
	int result = LPI2C_ERR_NONE;

	status = i2c_readl(i2c_dev, LPI2C_MSR);
	/* errors to check for */
	status &= LPI2C_MSR_NDF | LPI2C_MSR_ALF |
		LPI2C_MSR_FEF | LPI2C_MSR_PLTF;

	if (status) {
		if (status & LPI2C_MSR_PLTF)
			result = LPI2C_ERR_PLTF;
		else if (status & LPI2C_MSR_ALF)
			result = LPI2C_ERR_ALF;
		else if (status & LPI2C_MSR_NDF)
			result = LPI2C_ERR_NDF;
		else if (status & LPI2C_MSR_FEF)
			result = LPI2C_ERR_FEF;
		/* clear status flags */
		i2c_writel(i2c_dev, 0x7F00, LPI2C_MSR);
		/* reset fifos */
		reg = i2c_readl(i2c_dev, LPI2C_MCR);
		reg |= LPI2C_MCR_RRF | LPI2C_MCR_RTF;
		i2c_writel(i2c_dev, reg, LPI2C_MCR);
	}

	return result;
}

static int lpi2c_imx_wait_for_tx_ready(struct lpi2c_imx_dev *i2c_dev)
{
	u32 irq_mask, txcount;
	unsigned long time_left;
	int result = LPI2C_ERR_NONE, count = 100000;

#if 0
	irq_mask = i2c_readl(i2c_dev, LPI2C_MFSR);
	lpi2c_imx_unmask_irq(i2c_dev, LPI2C_MIER_TDIE);
#endif
	do {
		txcount = i2c_readl(i2c_dev, LPI2C_MFSR) & 0xff;
		if (!txcount)
			break;
		txcount = LPI2C_FIFO_SIZE - txcount;
		result = lpi2c_imx_check_clear_error(i2c_dev);
		if (result) {
			dev_info(i2c_dev->dev, "wait for tx ready: result 0x%x\n", result);
			return result;
		}
#if 0
		time_left = wait_for_completion_timeout(&i2c_dev->complete,
			LPI2C_TIMEOUT);
		lpi2c_imx_mask_irq(i2c_dev, LPI2C_MIER_TDIE);

		if (time_left == 0) {
			dev_err(i2c_dev->dev, "wait for tx ready time out\n");
			return -ETIMEDOUT;
		}
#endif
		count--;
	} while(!txcount && count <= 0);

	return 0;
}

static int lpi2c_imx_start(struct lpi2c_imx_dev *i2c_dev,
		u8 address, u8 direction)
{
	int result;
	u32 reg;

	result = lpi2c_imx_check_busy_bus(i2c_dev);
	if (result) {
		dev_info(&i2c_dev->adapter.dev, "start check busy bus\n");
		return result;
	}

	/* clear all flags */
	i2c_writel(i2c_dev, 0x7f00, LPI2C_MSR);
	/* turn off auto stop condition */
	reg = i2c_readl(i2c_dev, LPI2C_MCFGR1);
	reg &= ~LPI2C_MCFGR1_AUTOSTOP;
	i2c_writel(i2c_dev, reg, LPI2C_MCFGR1);
	/* wait for tx fifo ready */
	result = lpi2c_imx_wait_for_tx_ready(i2c_dev);
	if (result) {
		dev_info(&i2c_dev->adapter.dev, "start wait for tx ready\n");
		return result;
	}
	/* issue start command */
	reg = LPI2C_MTDR_CMD(0x4);
	reg |= (address << 0x1);
	reg |= direction;
	i2c_writel(i2c_dev, reg, LPI2C_MTDR);

	return 0;
}

static int lpi2c_imx_stop(struct lpi2c_imx_dev *i2c_dev)
{
	u32 reg;
	int result = LPI2C_ERR_NONE;
	int count = 100000;
	/* unsigned long time_left; */

	result = lpi2c_imx_wait_for_tx_ready(i2c_dev);
	if (result) {
		dev_err(i2c_dev->dev, "stop wait for tx ready\n");
		return result;
	}

	/* issue stop commad */
	reg = LPI2C_MTDR_CMD(0x2);
	i2c_writel(i2c_dev, reg, LPI2C_MTDR);

	while (result == LPI2C_ERR_NONE)
	{
		count--;
		reg = i2c_readl(i2c_dev, LPI2C_MSR);
		result = lpi2c_imx_check_clear_error(i2c_dev);
		/* clear stop detect flag */
		if (reg & LPI2C_MSR_SDF) {
			reg &= LPI2C_MSR_SDF;
			i2c_writel(i2c_dev, reg, LPI2C_MSR);
			break;
		}

		if (count <= 0)
			result = LPI2C_ERR_TIMEOUT;
	}
#if 0
	lpi2c_imx_unmask_irq(i2c_dev, LPI2C_MIER_SDIE);
	time_left = wait_for_completion_timeout(&i2c_dev->complete,
			LPI2C_TIMEOUT);
	lpi2c_imx_mask_irq(i2c_dev, LPI2C_MIER_SDIE);

	if (time_left == 0) {
		dev_err(i2c_dev->dev, "wait for stop detect time out\n");
		return -ETIMEDOUT;
	}
#endif

	return result;
}

static int lpi2c_imx_send(struct lpi2c_imx_dev *i2c_dev, u8 *txbuf, int len)
{
	u32 reg;
	int result = LPI2C_ERR_NONE;

	/* empty tx */
	if(!len)
		return result;

	while(len--) {
		result = lpi2c_imx_wait_for_tx_ready(i2c_dev);
		if (result) {
			dev_err(i2c_dev->dev,
				"send wait fot tx ready: %d\n", result);
			return result;
		}
		i2c_writel(i2c_dev, *txbuf++, LPI2C_MTDR);
	}

	return 0;
}

static int lpi2c_imx_receive(struct lpi2c_imx_dev *i2c_dev, u8 *rxbuf, int len)
{
	u32 reg;
	int result = LPI2C_ERR_NONE;

	/* empty rx */
	if(!len)
		return result;

	result = lpi2c_imx_wait_for_tx_ready(i2c_dev);
	if (result) {
		dev_err(i2c_dev->dev, "receive wait fot tx ready: %d\n", result);
		return result;
	}

	/* clear all status flags */
	i2c_writel(i2c_dev, 0x7f00, LPI2C_MSR);
	/* send receive command */
	reg = LPI2C_MTDR_CMD(0x1);
	reg |= LPI2C_MTDR_DATA(len - 1);
	i2c_writel(i2c_dev, reg, LPI2C_MTDR);

	while(len--) {
		do {
			result = lpi2c_imx_check_clear_error(i2c_dev);
			if (result) {
				dev_err(i2c_dev->dev, "receive check clear error: %d\n", result);
				return result;
			}
			reg = i2c_readl(i2c_dev, LPI2C_MRDR);
		} while (reg & LPI2C_MRDR_RXEMPTY);
		*rxbuf++ = LPI2C_MTDR_DATA(reg);
	}

	return 0;
}

static int lpi2c_imx_detect(struct lpi2c_imx_dev *i2c_dev,  u8 chip)
{
	int result = LPI2C_ERR_NONE;

	result = lpi2c_imx_start(i2c_dev, chip, 0);
	if (result) {
		dev_info(i2c_dev->dev, "detect: start error: %d\n", result);
		lpi2c_imx_stop(i2c_dev);
		lpi2c_imx_init(i2c_dev);
		return result;
	}

	result = lpi2c_imx_stop(i2c_dev);
	if (result) {
		dev_info(i2c_dev->dev, "detect: stop error: %d\n", result);
		lpi2c_imx_init(i2c_dev);
		return result;
	}

	return 0;
}

static int lpi2c_imx_read(struct lpi2c_imx_dev *i2c_dev, u8 chip, u32 addr,
		int alen, u8 *buf, int len)
{
	int result = LPI2C_ERR_NONE;

	result = lpi2c_imx_start(i2c_dev, chip, 0);
	if (result)
		goto read_err;
	result = lpi2c_imx_send(i2c_dev, (u8 *)&addr, 1);
	if (result)
		goto read_err;
	result = lpi2c_imx_start(i2c_dev, chip, 1);
	if (result)
		goto read_err;
	result = lpi2c_imx_receive(i2c_dev, buf, len);
	if (result)
		goto read_err;
	result = lpi2c_imx_stop(i2c_dev);
	if (result)
		goto read_err;

	return 0;

read_err:
	dev_info(&i2c_dev->adapter.dev, "read error: %d\n", result);
	lpi2c_imx_init(i2c_dev);
	lpi2c_imx_stop(i2c_dev);
	return result;
}

static int lpi2c_imx_write(struct lpi2c_imx_dev *i2c_dev, u8 chip, u32 addr,
		int alen, u8 *buf, int len)
{
	int result = LPI2C_ERR_NONE;

	result = lpi2c_imx_start(i2c_dev, chip, 0);
	if (result)
		return result;
	result = lpi2c_imx_send(i2c_dev, (u8 *)&addr, 1);
	if (result)
		return result;
	result = lpi2c_imx_send(i2c_dev, buf, len);
	if (result)
		return result;
	result = lpi2c_imx_stop(i2c_dev);
	if (result)
		return result;

	return 0;
}

static int lpi2c_imx_xfer_msg(struct lpi2c_imx_dev *i2c_dev,
		struct i2c_msg *msg, enum lpi2c_msg_type end_msg)
{

	return 0;
}

static int lpi2c_imx_xfer(struct i2c_adapter *adap,
		struct i2c_msg *msgs, int num)
{
	struct lpi2c_imx_dev *i2c_dev = i2c_get_adapdata(adap);
	enum lpi2c_msg_type end_msg = LPI2C_MSG_STOP;
	int i, ret = 0;
	int result = LPI2C_ERR_NONE;
	u32 addr = 0;

#if 0
	ret = clk_prepare_enable(i2c_dev->clk);

	if (ret < 0) {
		dev_err(i2c_dev->dev, "Clock enable failed: %d\n", ret);
		return ret;
	}
	/* start i2c transfer */
	ret = lpi2c_imx_start(i2c_dev, (u8)msgs[0].addr, 1);
#endif

	dev_info(&i2c_dev->adapter.dev, "msgs num: %d\n", num);

	for (i = 0; i < num; i++) {
		ret = lpi2c_imx_detect(i2c_dev, msgs[i].addr);
		if (ret) {
			dev_info(&i2c_dev->adapter.dev, "txfer detect 0x%x error: %d\n", msgs[i].addr, ret);
			return -ret;
		}
	}
#if 0
	ret = lpi2c_imx_read(i2c_dev, (u8)msgs[0].addr, msgs->buf[0],
		0, (u8 *)&msgs[1].buf, msgs[1].len);
	if (ret) {
		dev_info(&i2c_dev->adapter.dev, "read error: 0x%x\n", ret);
	}
#endif
#if 0
		ret = lpi2c_imx_xfer_msg(i2c_dev, &msgs[i], end_msg);
		if (ret)
			return result;
	}
	/* stop i2c transfer */
	ret = lpi2c_imx_stop(i2c_dev);
xfer_error:
	clk_disable_unprepare(i2c_dev->clk);
#endif
	return 0;
}

static u32 lpi2c_imx_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL |
		I2C_FUNC_SMBUS_READ_BLOCK_DATA;
}

static const struct i2c_algorithm lpi2c_imx_algo = {
	.master_xfer	= lpi2c_imx_xfer,
	.functionality	= lpi2c_imx_func,
};

static const struct of_device_id lpi2c_imx_of_match[] = {
	{ .compatible = "fsl,imx8dv-lpi2c" },
	{ },
};
MODULE_DEVICE_TABLE(of, lpi2c_imx_of_match);

static void lpi2c_imx_reset(struct lpi2c_imx_dev *i2c_dev)
{
	u32 reg;
	/* set and clear for peripherial reset */
	reg = i2c_readl(i2c_dev, LPI2C_MCR);
	/* reset fifos and master reset */
	reg |= LPI2C_MCR_RRF | LPI2C_MCR_RTF | LPI2C_MCR_RST;
	i2c_writel(i2c_dev, reg, LPI2C_MCR);
	/* wait for controller */
	udelay(50);
	/* disable dozen mode */
	reg = LPI2C_MCR_DOZEN;
	i2c_writel(i2c_dev, reg, LPI2C_MCR);
}

static int lpi2c_imx_init(struct lpi2c_imx_dev *i2c_dev)
{
	u32 reg;

	/* reset i2c controller */
	lpi2c_imx_reset(i2c_dev);
	/* host request disable, active high, external pin */
	reg = i2c_readl(i2c_dev, LPI2C_MCFGR0);
	reg &= ~LPI2C_MCFGR0_HREN;
	reg &= ~LPI2C_MCFGR0_HRSEL;
	reg |= LPI2C_MCFGR0_HRPOL;
	i2c_writel(i2c_dev, reg, LPI2C_MCFGR0);

	/* pincfg 2 pin open drain and ignore nack */
	reg = i2c_readl(i2c_dev, LPI2C_MCFGR1);
	reg &= ~LPI2C_MCFGR1_IGNACK;
	reg |= LPI2C_MCFGR1_PINCFG(0x0);
	i2c_writel(i2c_dev, reg, LPI2C_MCFGR1);

	/* hardcode bus speed */
        i2c_writel(i2c_dev, 0x09131326, LPI2C_MCCR0);
	/* enable i2c controller in master mode */
	reg = i2c_readl(i2c_dev, LPI2C_MCR);
	reg |= LPI2C_MCR_MEN;
	i2c_writel(i2c_dev, reg, LPI2C_MCR);

	return 0;
}

static int lpi2c_imx_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id = of_match_device(lpi2c_imx_of_match,
			&pdev->dev);
	struct lpi2c_imx_dev *i2c_dev;
	struct resource *res;
	void __iomem *base;
	int ret, irq;
	u32 reg;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(base))
		return PTR_ERR(base);

	i2c_dev = devm_kzalloc(&pdev->dev, sizeof(*i2c_dev), GFP_KERNEL);

	if (!i2c_dev)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "can't get I2C irq number id\n");
		return irq;
	}

	i2c_dev->dev = &pdev->dev;
	i2c_dev->base = base;
	i2c_dev->irq = irq;

	strlcpy(i2c_dev->adapter.name, pdev->name,
			sizeof(i2c_dev->adapter.name));
	i2c_dev->adapter.owner = THIS_MODULE;
	i2c_dev->adapter.algo = &lpi2c_imx_algo;
	i2c_dev->adapter.dev.parent = &pdev->dev;
	i2c_dev->adapter.nr = pdev->id;
	i2c_dev->adapter.dev.of_node = pdev->dev.of_node;

	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency",
			&i2c_dev->bitrate);
	if (ret) {
		i2c_dev->bitrate = LPI2C_BITRATE;
		dev_info(i2c_dev->dev, "using default bitrate: %d\n",
				i2c_dev->bitrate);
	}

	i2c_dev->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2c_dev->clk)) {
		dev_err(&pdev->dev, "can't get I2C clock root\n");
		return PTR_ERR(i2c_dev->clk);
	}

#if 0
	ret = clk_prepare_enable(i2c_dev->clk);
	if (ret) {
		dev_err(&pdev->dev, "can't enable I2C clock root\n");
		clk_disable_unprepare(i2c_dev->clk);
		return ret;
	}
#endif

	i2c_set_adapdata(&i2c_dev->adapter, i2c_dev);

	ret = lpi2c_imx_init(i2c_dev);
	if (ret) {
		dev_err(&pdev->dev, "Fail to init I2C interface\n");
		return ret;
	}

	ret = devm_request_irq(&pdev->dev, i2c_dev->irq, lpi2c_imx_isr, 0,
			dev_name(&pdev->dev), i2c_dev);
	if (ret) {
		dev_err(&pdev->dev, "Fail to request irq %i\n", i2c_dev->irq);
#if 0
		clk_disable_unprepare(i2c_dev->clk);
#endif
		return ret;
	}
#if 0
	/* init tranfer complete notifier */
	init_completion(&i2c_dev->complete);
#endif
	reg = i2c_readl(i2c_dev, LPI2C_VERID);
	dev_info(&pdev->dev,  "verid: 0x%x\n", reg);

	/* add numered adapter */
	ret = i2c_add_numbered_adapter(&i2c_dev->adapter);
	if (ret) {
		dev_err(&pdev->dev, "Fail to add I2C adapter\n");
		return ret;
	}

	return 0;
}

static int lpi2c_imx_remove(struct platform_device *pdev)
{
	struct lpi2c_imx_dev *i2c_dev = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c_dev->adapter);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int lpi2c_imx_suspend(struct device *dev)
{
	return 0;
}

static int lpi2c_imx_resume(struct device *dev)
{
	return 0;
}

static SIMPLE_DEV_PM_OPS(lpi2c_imx_pm, lpi2c_imx_suspend, lpi2c_imx_resume);
#endif

static struct platform_driver lpi2c_imx_driver = {
	.probe = lpi2c_imx_probe,
	.remove = lpi2c_imx_remove,
	.driver = {
		.name = "i2c-lp-imx",
		.of_match_table = lpi2c_imx_of_match,
#ifdef CONFIG_PM_SLEEP
		.pm = &lpi2c_imx_pm,
#endif
	},
};

static int __init lpi2c_imx_init_driver(void)
{
	return platform_driver_register(&lpi2c_imx_driver);
}

static void __exit lpi2c_imx_exit_driver(void)
{
	platform_driver_unregister(&lpi2c_imx_driver);
}

subsys_initcall(lpi2c_imx_init_driver);
module_exit(lpi2c_imx_exit_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Adrian Alonso");
MODULE_DESCRIPTION("IMX Low Power I2C Bus Controller driver");
