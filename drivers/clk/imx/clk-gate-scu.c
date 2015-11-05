/*
 * Copyright (C) 2010-2011 Canonical Ltd <jeremy.kerr@canonical.com>
 * Copyright (C) 2011-2012 Mike Turquette, Linaro Ltd <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Gated clock implementation
 */

#include <linux/clk-provider.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>
#include <soc/imx8/sc/sci.h>
#include "clk-imx8dv.h"

/**
 * DOC: basic gatable clock which can gate and ungate it's ouput
 *
 * Traits of this clock:
 * prepare - clk_(un)prepare only ensures parent is (un)prepared
 * enable - clk_enable and clk_disable are functional & control gating
 * rate - inherits rate from parent.  No clk_set_rate support
 * parent - fixed parent.  No clk_set_parent support
 */

struct clk_gate_scu {
	struct clk_hw hw;
	void __iomem	*reg;
	u8		bit_idx;
	bool	hw_gate;
	u8		flags;
	spinlock_t	*lock;
	sc_rsrc_t	rsrc_id;
	sc_pm_clk_t	clk_type;
};

#define to_clk_gate_scu(_hw) container_of(_hw, struct clk_gate_scu, hw)

/* Write to the LPCG bits. */
static int clk_gate_scu_enable(struct clk_hw *hw)
{
	struct clk_gate_scu *gate = to_clk_gate_scu(hw);
	u32 reg;
	unsigned long flags = 0;
	sc_err_t sciErr;

	spin_lock_irqsave(gate->lock, flags);

	if (gate->reg) {
		reg = readl(gate->reg);
		if (gate->hw_gate)
			reg |= (0x3 << gate->bit_idx);
		else
			reg |= (0x2 << gate->bit_idx);
		writel(reg, gate->reg);
	} else
		sciErr = sc_pm_clock_enable(ccm_ipcHandle, gate->rsrc_id,
										gate->clk_type, true, false);
	spin_unlock_irqrestore(gate->lock, flags);
	return 0;
}

/* Write to the LPCG bits. */
static void clk_gate_scu_disable(struct clk_hw *hw)
{
	struct clk_gate_scu *gate = to_clk_gate_scu(hw);
	u32 reg;
	unsigned long flags = 0;
	sc_err_t sciErr;

	/* Need to implement LPCG code here. */
	spin_lock_irqsave(gate->lock, flags);

	if (gate->reg) {
		reg = readl(gate->reg);
		if (gate->hw_gate)
			reg &= ~(0x3 << gate->bit_idx);
		else
			reg &= ~(0x2 << gate->bit_idx);
		writel(reg, gate->reg);
	} else 
		sciErr = sc_pm_clock_enable(ccm_ipcHandle, gate->rsrc_id,
										gate->clk_type, false, false);

	spin_unlock_irqrestore(gate->lock, flags);
}

static int clk_gate_scu_prepare(struct clk_hw *hw)
{
	struct clk_gate_scu *gate = to_clk_gate_scu(hw);
	unsigned long flags = 0;
	sc_err_t sciErr;

	spin_lock_irqsave(gate->lock, flags);

	if (gate->reg) {
		u32 reg;

		/* Disable clock at LPCG level before enabling the clock slice. */
		reg = readl(gate->reg);
		if (gate->hw_gate)
			reg &= ~(0x3 << gate->bit_idx);
		else
			reg &= ~(0x2 << gate->bit_idx);
		writel(reg, gate->reg);

		sciErr = sc_pm_clock_enable(ccm_ipcHandle, gate->rsrc_id,
										gate->clk_type, true, false);
	}

	sciErr = sc_pm_set_resource_power_mode(ccm_ipcHandle, gate->rsrc_id,
											SC_PM_PW_MODE_ON);
	spin_unlock_irqrestore(gate->lock, flags);
	return sciErr;
}

static void clk_gate_scu_unprepare(struct clk_hw *hw)
{
	struct clk_gate_scu *gate = to_clk_gate_scu(hw);
	unsigned long flags = 0;
	sc_err_t sciErr;

	spin_lock_irqsave(gate->lock, flags);

	sciErr = sc_pm_set_resource_power_mode(ccm_ipcHandle, gate->rsrc_id,
												SC_PM_PW_MODE_OFF);
	if (gate->reg)
		sciErr = sc_pm_clock_enable(ccm_ipcHandle, gate->rsrc_id,
										gate->clk_type, true,false);

	spin_unlock_irqrestore(gate->lock, flags);
}

static struct clk_ops clk_gate_scu_ops = {
	.prepare = clk_gate_scu_prepare,
	.unprepare = clk_gate_scu_unprepare,
	.enable = clk_gate_scu_enable,
	.disable = clk_gate_scu_disable,
};

struct clk *clk_register_gate_scu(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		u8 clk_gate_scu_flags, spinlock_t *lock,
		sc_rsrc_t rsrc_id, sc_pm_clk_t clk_type,
		void __iomem *reg, u8 bit_idx, bool hw_gate)
{
	struct clk_gate_scu *gate;
	struct clk *clk;
	struct clk_init_data init;

	gate = kzalloc(sizeof(struct clk_gate_scu), GFP_KERNEL);
	if (!gate)
		return ERR_PTR(-ENOMEM);

	/* struct clk_gate_scu assignments */
	gate->flags = clk_gate_scu_flags;
	gate->lock = lock;
	gate->rsrc_id = rsrc_id;
	gate->clk_type = clk_type;
	if (reg != NULL)
		gate->reg = ioremap((phys_addr_t)reg, SZ_64K);
	else
		gate->reg = NULL;
	gate->bit_idx = bit_idx;
	gate->hw_gate = hw_gate;

	init.name = name;
	init.ops = &clk_gate_scu_ops;
	init.flags = flags;
	init.parent_names = parent_name ? &parent_name : NULL;
	init.num_parents = parent_name ? 1 : 0;

	gate->hw.init = &init;

	clk = clk_register(dev, &gate->hw);
	if (IS_ERR(clk))
		kfree(gate);

	return clk;
}
