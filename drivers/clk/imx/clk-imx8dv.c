/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <dt-bindings/clock/imx8dv-clock.h>
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

DEFINE_SPINLOCK(imx_ccm_lock);
sc_ipc_t ccm_ipcHandle;

static struct clk *clks[IMX8DV_CLK_END];
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

	clks[IMX8DV_SDHC_BUS] = imx_clk_fixed("sdhc_bus_clk_fixed", SC_266MHZ);
	clks[IMX8DV_ENET_BUS] = imx_clk_fixed("enet_bus_clk_fixed", SC_266MHZ);
	clks[IMX8DV_ANATOP_BUS] = imx_clk_fixed("anatop_bus_clk_fixed", SC_125MHZ);

	clks[IMX8DV_SDHC_BUS_CLK] = imx_clk_gate_scu("sdhc_bus_clk", "sdhc_bus_clk_fixed", SC_R_SDHC_0, 1, NULL, 0, 0);
	clks[IMX8DV_ENET_BUS_CLK] = imx_clk_gate_scu("enet_bus_clk", "enet_bus_clk_fixed", SC_R_ENET_0, 1, NULL, 0, 0);
	clks[IMX8DV_ANATOP_BUS_CLK] = imx_clk_gate_scu("enet_anatop_clk", "anatop_bus_clk_fixed", SC_R_ENET_0, 1, NULL, 0, 0);

	clks[IMX8DV_SDHC0_DIV] = imx_clk_divider_scu("sdhc0_div", SC_R_SDHC_0, 2);
	clks[IMX8DV_SDHC1_DIV] = imx_clk_divider_scu("sdhc1_div", SC_R_SDHC_1, 2);
	clks[IMX8DV_SDHC2_DIV] = imx_clk_divider_scu("sdhc2_div", SC_R_SDHC_2, 2);
	clks[IMX8DV_ENET0_DIV] = imx_clk_divider_scu("enet0_div", SC_R_ENET_0, 2);
	clks[IMX8DV_ENET0_DIV] = imx_clk_divider_scu("enet1_div", SC_R_ENET_1, 2);

	clks[IMX8DV_I2C0_DIV] = imx_clk_divider_scu("i2c0_div", SC_R_I2C_0, 2);
	clks[IMX8DV_I2C1_DIV] = imx_clk_divider_scu("i2c1_div", SC_R_I2C_1, 2);
	clks[IMX8DV_I2C2_DIV] = imx_clk_divider_scu("i2c2_div", SC_R_I2C_2, 2);
	clks[IMX8DV_I2C3_DIV] = imx_clk_divider_scu("i2c3_div", SC_R_I2C_3, 2);
	clks[IMX8DV_PWM0_DIV] = imx_clk_divider_scu("pwm0_div", SC_R_PWM_0, 2);
	clks[IMX8DV_PWM1_DIV] = imx_clk_divider_scu("pwm1_div", SC_R_PWM_1, 2);
	clks[IMX8DV_PWM2_DIV] = imx_clk_divider_scu("pwm2_div", SC_R_PWM_2, 2);
	clks[IMX8DV_PWM3_DIV] = imx_clk_divider_scu("pwm3_div", SC_R_PWM_3, 2);
	clks[IMX8DV_PWM4_DIV] = imx_clk_divider_scu("pwm4_div", SC_R_PWM_4, 2);
	clks[IMX8DV_PWM5_DIV] = imx_clk_divider_scu("pwm5_div", SC_R_PWM_5, 2);
	clks[IMX8DV_PWM6_DIV] = imx_clk_divider_scu("pwm6_div", SC_R_PWM_6, 2);
	clks[IMX8DV_PWM7_DIV] = imx_clk_divider_scu("pwm7_div", SC_R_PWM_7, 2);
	clks[IMX8DV_GPT0_DIV] = imx_clk_divider_scu("gpt0_div", SC_R_GPT_0, 2);
	clks[IMX8DV_GPT1_DIV] = imx_clk_divider_scu("gpt1_div", SC_R_GPT_1, 2);
	clks[IMX8DV_GPT2_DIV] = imx_clk_divider_scu("gpt2_div", SC_R_GPT_2, 2);
	clks[IMX8DV_GPT3_DIV] = imx_clk_divider_scu("gpt3_div", SC_R_GPT_3, 2);
	clks[IMX8DV_GPT4_DIV] = imx_clk_divider_scu("gpt4_div", SC_R_GPT_4, 2);

	clks[IMX8DV_GPU0_AXI_DIV] = imx_clk_divider_scu("gpu0_axi_div", SC_R_GPU_0_PID0, 1);
	clks[IMX8DV_GPU0_AHB_DIV] = imx_clk_divider_scu("gpu0_ahb_div", SC_R_GPU_0_PID0, 2);
	clks[IMX8DV_GPU1_AXI_DIV] = imx_clk_divider_scu("gpu1_axi_div", SC_R_GPU_1_PID0, 1);
	clks[IMX8DV_GPU1_AHB_DIV] = imx_clk_divider_scu("gpu1_ahb_div", SC_R_GPU_1_PID0, 2);

	clks[IMX8DV_GPU0_CORE_DIV] = imx_clk_divider_scu("gpu0_core_div", SC_R_GPU_0_PID0, 4);
	clks[IMX8DV_GPU0_SHADER_DIV] = imx_clk_divider_scu("gpu0_shader_div", SC_R_GPU_0_PID0, 3);
	clks[IMX8DV_GPU1_CORE_DIV] = imx_clk_divider_scu("gpu1_core_div", SC_R_GPU_1_PID0, 4);
	clks[IMX8DV_GPU1_SHADER_DIV] = imx_clk_divider_scu("gpu1_shader_div", SC_R_GPU_1_PID0, 3);

	clks[IMX8DV_I2C0_DIV_CLK] = imx_clk_gate_scu("i2c0_div_clk", "i2c0_div", SC_R_I2C_0, 2, (void __iomem *)(SC_R_I2C_0_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C1_DIV_CLK] = imx_clk_gate_scu("i2c1_div_clk", "i2c0_div", SC_R_I2C_1, 2, (void __iomem *)(SC_R_I2C_1_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C2_DIV_CLK] = imx_clk_gate_scu("i2c2_div_clk", "i2c0_div", SC_R_I2C_2, 2, (void __iomem *)(SC_R_I2C_2_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C3_DIV_CLK] = imx_clk_gate_scu("i2c3_div_clk", "i2c0_div", SC_R_I2C_3, 2, (void __iomem *)(SC_R_I2C_3_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C0_CLK] = imx_clk_gate_scu("i2c0_clk", "i2c0_div_clk", SC_R_I2C_0, 2, (void __iomem *)(SC_R_I2C_0_LPCG + 0xc), 0, 0);
	clks[IMX8DV_I2C1_CLK] = imx_clk_gate_scu("i2c1_clk", "i2c1_div_clk", SC_R_I2C_1, 2, (void __iomem *)(SC_R_I2C_1_LPCG + 0xc), 0, 0);
	clks[IMX8DV_I2C2_CLK] = imx_clk_gate_scu("i2c2_clk", "i2c2_div_clk", SC_R_I2C_2, 2, (void __iomem *)(SC_R_I2C_2_LPCG + 0xc), 0, 0);
	clks[IMX8DV_I2C3_CLK] = imx_clk_gate_scu("i2c3_clk", "i2c3_div_clk", SC_R_I2C_3, 2, (void __iomem *)(SC_R_I2C_3_LPCG + 0xc), 0, 0);
	clks[IMX8DV_PWM0_CLK] = imx_clk_gate_scu("pwm0_clk", "pwm0_div", SC_R_PWM_0, 2, (void __iomem *)(SC_R_PWM_0_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM1_CLK] = imx_clk_gate_scu("pwm1_clk", "pwm1_div", SC_R_PWM_1, 2, (void __iomem *)(SC_R_PWM_1_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM2_CLK] = imx_clk_gate_scu("pwm2_clk", "pwm2_div", SC_R_PWM_2, 2, (void __iomem *)(SC_R_PWM_2_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM3_CLK] = imx_clk_gate_scu("pwm3_clk", "pwm3_div", SC_R_PWM_3, 2, (void __iomem *)(SC_R_PWM_3_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM4_CLK] = imx_clk_gate_scu("pwm4_clk", "pwm4_div", SC_R_PWM_4, 2, (void __iomem *)(SC_R_PWM_4_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM5_CLK] = imx_clk_gate_scu("pwm5_clk", "pwm5_div", SC_R_PWM_5, 2, (void __iomem *)(SC_R_PWM_5_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM6_CLK] = imx_clk_gate_scu("pwm6_clk", "pwm6_div", SC_R_PWM_6, 2, (void __iomem *)(SC_R_PWM_6_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM7_CLK] = imx_clk_gate_scu("pwm7_clk", "pwm7_div", SC_R_PWM_7, 2, (void __iomem *)(SC_R_PWM_7_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT0_CLK] = imx_clk_gate_scu("gpt0_clk", "gpt0_div", SC_R_GPT_0, 2, (void __iomem *)(SC_R_GPT_0_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT1_CLK] = imx_clk_gate_scu("gpt1_clk", "gpt1_div", SC_R_GPT_1, 2, (void __iomem *)(SC_R_GPT_1_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT2_CLK] = imx_clk_gate_scu("gpt2_clk", "gpt2_div", SC_R_GPT_2, 2, (void __iomem *)(SC_R_GPT_2_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT3_CLK] = imx_clk_gate_scu("gpt3_clk", "gpt3_div", SC_R_GPT_3, 2, (void __iomem *)(SC_R_GPT_3_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT4_CLK] = imx_clk_gate_scu("gpt4_clk", "gpt4_div", SC_R_GPT_4, 2, (void __iomem *)(SC_R_GPT_4_LPCG + 0x4), 0, 0);

	clks[IMX8DV_SDHC0_CLK] = imx_clk_gate_scu("sdhc0_clk", "sdhc0_div", SC_R_SDHC_0, 2, NULL, 0, 0);
	clks[IMX8DV_SDHC1_CLK] = imx_clk_gate_scu("sdhc1_clk", "sdhc1_div", SC_R_SDHC_1, 2, NULL, 0, 0);
	clks[IMX8DV_SDHC2_CLK] = imx_clk_gate_scu("sdhc2_clk", "sdhc2_div", SC_R_SDHC_2, 2, NULL, 0, 0);

	clks[IMX8DV_ENET0_CLK] = imx_clk_gate_scu("enet0_clk", "enet0_div", SC_R_ENET_0, 2, NULL, 0, 0);
	clks[IMX8DV_ENET1_CLK] = imx_clk_gate_scu("enet1_clk", "enet1_div", SC_R_ENET_1, 2, NULL, 0, 0);

	clks[IMX8DV_GPU0_AXI_CLK] = imx_clk_gate_scu("gpu0_axi_clk", "gpu0_axi_div", SC_R_GPU_0_PID0, 1, NULL, 0, 0);
	clks[IMX8DV_GPU0_AHB_CLK] = imx_clk_gate_scu("gpu0_ahb_clk", "gpu0_ahb_div", SC_R_GPU_0_PID0, 2, NULL, 0, 0);
	clks[IMX8DV_GPU1_AXI_CLK] = imx_clk_gate_scu("gpu1_axi_clk", "gpu1_axi_div", SC_R_GPU_1_PID0, 1, NULL, 0, 0);
	clks[IMX8DV_GPU1_AHB_CLK] = imx_clk_gate_scu("gpu1_ahb_clk", "gpu1_ahb_div", SC_R_GPU_1_PID0, 2, NULL, 0, 0);

	clks[IMX8DV_GPU0_CORE_CLK] = imx_clk_gate_scu("gpu0_core_clk", "gpu0_core_div", SC_R_GPU_0_PID0, 4, NULL, 0, 0);
	clks[IMX8DV_GPU0_SHADER_CLK] = imx_clk_gate_scu("gpu0_shader_clk", "gpu0_shader_div", SC_R_GPU_0_PID0, 3, NULL, 0, 0);
	clks[IMX8DV_GPU1_CORE_CLK] = imx_clk_gate_scu("gpu1_core_clk", "gpu1_core_div", SC_R_GPU_1_PID0, 4, NULL, 0, 0);
	clks[IMX8DV_GPU1_SHADER_CLK] = imx_clk_gate_scu("gpu1_shader_clk", "gpu1_shader_div", SC_R_GPU_1_PID0, 3, NULL, 0, 0);

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

    clk_prepare(clks[IMX8DV_GPU0_AXI_CLK]);
    clk_enable(clks[IMX8DV_GPU0_AXI_CLK]);

    clk_prepare(clks[IMX8DV_GPU0_AHB_CLK]);
    clk_enable(clks[IMX8DV_GPU0_AHB_CLK]);

    clk_prepare(clks[IMX8DV_GPU1_AXI_CLK]);
    clk_enable(clks[IMX8DV_GPU1_AXI_CLK]);

    clk_prepare(clks[IMX8DV_GPU1_AHB_CLK]);
    clk_enable(clks[IMX8DV_GPU1_AHB_CLK]);

    clk_prepare(clks[IMX8DV_GPU0_CORE_CLK]);
    clk_set_rate(clks[IMX8DV_GPU0_CORE_CLK], 800000000);
    clk_enable(clks[IMX8DV_GPU0_CORE_CLK]);

    clk_prepare(clks[IMX8DV_GPU0_SHADER_CLK]);
    clk_enable(clks[IMX8DV_GPU0_SHADER_CLK]);

    clk_prepare(clks[IMX8DV_GPU1_CORE_CLK]);
    clk_set_rate(clks[IMX8DV_GPU1_CORE_CLK], 800000000);
    clk_enable(clks[IMX8DV_GPU1_CORE_CLK]);

    clk_prepare(clks[IMX8DV_GPU1_SHADER_CLK]);
    clk_enable(clks[IMX8DV_GPU1_SHADER_CLK]);

	printk("*************** finished imx8dv_clocks_init\n");
}

CLK_OF_DECLARE(imx8dv,"fsl,imx8dv-clk", imx8dv_clocks_init);

