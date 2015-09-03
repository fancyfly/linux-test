/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 * Copyright 2012 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <soc/imx8/sc/sci.h>
#include "clk-imx8dv.h"


struct clk_divider_scu {
	struct clk_divider div;
	const struct clk_ops *div_ops;
	sc_rsrc_t	rsrc_id;
	sc_pm_clk_t	clk_type;
};

static inline struct clk_divider_scu *to_clk_divider_scu(struct clk_hw *hw)
{
	struct clk_divider *div = container_of(hw, struct clk_divider, hw);

	return container_of(div, struct clk_divider_scu, div);
}

static unsigned long clk_divider_scu_recalc_rate(struct clk_hw *hw,
						  unsigned long parent_rate)
{
	struct clk_divider_scu *clk = to_clk_divider_scu(hw);
	sc_err_t sciErr;
	sc_pm_clock_rate_t rate;

	sciErr = sc_pm_get_clock_rate(ccm_ipcHandle, clk->rsrc_id,
									clk->clk_type, &rate);
	if (!sciErr)
		return rate;
	return sciErr;
}

static long clk_divider_scu_round_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *prate)
{
	/********* TBD Finish up round_rate function*******/
	*prate = rate;
	return rate;
}

static int clk_divider_scu_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_divider_scu *clk = to_clk_divider_scu(hw);
	sc_err_t sciErr;

	sciErr = sc_pm_set_clock_rate(ccm_ipcHandle, clk->rsrc_id,
									clk->clk_type, (sc_pm_clock_rate_t *)&rate);

	return sciErr;
}

static struct clk_ops clk_divider_scu_ops = {
	.recalc_rate = clk_divider_scu_recalc_rate,
	.round_rate = clk_divider_scu_round_rate,
	.set_rate = clk_divider_scu_set_rate,
};

struct clk *imx_clk_divider_scu(const char *name,
				sc_rsrc_t rsrc_id, sc_pm_clk_t clk_type)
{
	struct clk_divider_scu *div_clk;
	struct clk *clk;
	struct clk_init_data init;

	div_clk = kzalloc(sizeof(*div_clk), GFP_KERNEL);
	if (!div_clk)
		return ERR_PTR(-ENOMEM);

	div_clk->rsrc_id = rsrc_id;
	div_clk->clk_type = clk_type;

	div_clk->div.lock = &imx_ccm_lock;
	div_clk->div_ops = &clk_divider_ops;

	init.name = name;
	init.ops = &clk_divider_scu_ops;
	init.flags = CLK_IS_ROOT;
	init.num_parents = 0;
	div_clk->div.hw.init = &init;

	clk = clk_register(NULL, &div_clk->div.hw);
	if (IS_ERR(clk))
		kfree(div_clk);

	return clk;
}


