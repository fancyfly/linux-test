#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/types.h>

#include <soc/imx8/imx8dv/lpcg.h>
#include <soc/imx8/sc/sci.h>
#include "clk-imx8dv.h"

#define IMX8DV_SDHC_BUS_DIV 0
#define IMX8DV_SDHC_BUS_CLK 1
#define IMX8DV_SDHC1_DIV 	2
#define IMX8DV_SDHC1_CLK 	3
#define IMX8DV_I2C0_DIV		4
#define IMX8DV_I2C0_CLK		5
#define IMX8DV_I2C0_DIV_CLK	6
#define IMX8DV_I2C1_DIV		7
#define IMX8DV_I2C2_DIV		8
#define IMX8DV_I2C2_CLK		9
#define IMX8DV_I2C3_DIV		10
#define IMX8DV_I2C3_CLK		11
#define IMX8DV_END_CLK 		12

DEFINE_SPINLOCK(imx_ccm_lock);
sc_ipc_t ccm_ipcHandle;

static struct clk *clks[IMX8DV_END_CLK];
static struct clk_onecell_data clk_data;

static void __init imx8dv_clocks_init(struct device_node *ccm_node)
{
	sc_err_t sciErr;
	sc_pm_clock_rate_t rate;
	int i, mu_id;

	mu_id = imx8dv_mu_init();

	sciErr = sc_ipc_open(&ccm_ipcHandle, mu_id);

	if (sciErr != SC_ERR_NONE) {
		pr_info("Cannot open MU channel to SCU\n");
		return;
	};

	clks[IMX8DV_I2C0_DIV] = imx_clk_divider_scu("i2c0_div", SC_R_I2C_0, 2);
	clks[IMX8DV_I2C0_DIV_CLK] = imx_clk_gate_scu("i2c0_div_clk", "i2c0_div", SC_R_I2C_0, 2, (void __iomem *)(SC_R_I2C_0_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C0_CLK] = imx_clk_gate_scu("i2c0_clk", "i2c0_div_clk", SC_R_I2C_0, 2, (void __iomem *)(SC_R_I2C_0_LPCG + 0xc), 0, 0);
	clks[IMX8DV_SDHC_BUS_DIV] = imx_clk_divider_scu("sdhc_bus_div", SC_R_SDHC_0, 1);
	clks[IMX8DV_SDHC_BUS_CLK] = imx_clk_gate_scu("sdhc_bus_clk", "sdhc_bus_div", SC_R_SDHC_0, 1, NULL, 0, 0);
	clks[IMX8DV_SDHC1_DIV] = imx_clk_divider_scu("sdhc1_div", SC_R_SDHC_0, 2);
	clks[IMX8DV_SDHC1_CLK] = imx_clk_gate_scu("sdhc1_clk", "sdhc1_div", SC_R_SDHC_0, 2, NULL, 0, 0);

	for (i = 0; i < ARRAY_SIZE(clks); i++)
		if (IS_ERR(clks[i]))
			pr_err("i.MX8DV clk %d: register failed with %ld\n",
					i, PTR_ERR(clks[i]));


	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(ccm_node, of_clk_src_onecell_get, &clk_data);

	/* Following is an example for clock init in Linux. */
	clk_prepare(clks[IMX8DV_I2C0_CLK]);
	clk_set_rate(clks[IMX8DV_I2C0_CLK], 133000000);
	sciErr = sc_pm_get_clock_rate(ccm_ipcHandle, SC_R_I2C_0, 2, &rate);
	printk("In ****%s**********I2C0_clk rate = %d\n", __FILE__, rate);

	clk_enable(clks[IMX8DV_I2C0_CLK]);

	clk_disable(clks[IMX8DV_I2C0_CLK]);
	clk_enable(clks[IMX8DV_I2C0_CLK]);
	printk("*************** finished imx8dv_clocks_init\n");
}

CLK_OF_DECLARE(imx8dv,"fsl,imx8dv-clk", imx8dv_clocks_init);

