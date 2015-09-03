#ifndef __IMX8DV_CLK_H
#define __IMX8DV_CLK_H

#include <linux/spinlock.h>
#include <linux/clk-provider.h>
#include <soc/imx8/sc/sci.h>

extern spinlock_t imx_ccm_lock;
extern sc_ipc_t ccm_ipcHandle;

struct clk *imx_clk_divider_scu(const char *name, 
					sc_rsrc_t rsrc_id, sc_pm_clk_t clk_type);

struct clk *clk_register_gate_scu(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		u8 clk_gate_scu_flags, spinlock_t *lock,
		sc_rsrc_t rsrc_id, sc_pm_clk_t clk_type,
		void __iomem *reg, u8 bit_idx, bool hw_gate);

static inline struct clk *imx_clk_gate_scu(const char *name, const char *parent,
		sc_rsrc_t rsrc_id, sc_pm_clk_t clk_type,
		void __iomem *reg, u8 bit_idx, bool hw_gate)
{
	return clk_register_gate_scu(NULL, name, parent,
			CLK_SET_RATE_PARENT /*| CLK_SET_RATE_GATE*/,
			0, &imx_ccm_lock, rsrc_id, clk_type, reg, bit_idx, hw_gate);
}

static inline void imx_clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = clk_set_rate(clk, rate);

	if (ret)
		pr_err("failed to set rate of clk %s to %ld: %d\n",
			__clk_get_name(clk), rate, ret);
}

#endif
