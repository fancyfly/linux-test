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

	/* Fixed clocks. */
	/* Megawrap */
	clks[IMX8DV_SDHC_BUS_CLK] = imx_clk_fixed("sdhc_bus_clk_fixed", SC_266MHZ);
	clks[IMX8DV_ENET_BUS_CLK] = imx_clk_fixed("enet_bus_clk_fixed", SC_266MHZ);
	clks[IMX8DV_ANATOP_CLK] = imx_clk_fixed("anatop_bus_clk_fixed", SC_125MHZ);

	/* Clocks dividers. */
	/* Megawrap */
	clks[IMX8DV_SDHC0_DIV] = imx_clk_divider_scu("sdhc0_div", SC_R_SDHC_0, 2);
	clks[IMX8DV_SDHC1_DIV] = imx_clk_divider_scu("sdhc1_div", SC_R_SDHC_1, 2);
	clks[IMX8DV_SDHC2_DIV] = imx_clk_divider_scu("sdhc2_div", SC_R_SDHC_2, 2);
	clks[IMX8DV_ENET0_DIV] = imx_clk_divider_scu("enet0_div", SC_R_ENET_0, 2);
	clks[IMX8DV_ENET1_DIV] = imx_clk_divider_scu("enet1_div", SC_R_ENET_1, 2);
	clks[IMX8DV_ENET0_TIME_DIV] = imx_clk_divider_scu("enet0_time_div", SC_R_ENET_0, 4);
	clks[IMX8DV_ENET1_TIME_DIV] = imx_clk_divider_scu("enet1_time_div", SC_R_ENET_1, 4);
	clks[IMX8DV_QSPI_DIV]  = imx_clk_divider_scu("qspi_div", SC_R_QSPI_0, 2);
	clks[IMX8DV_SAI0_DIV]  = imx_clk_divider_scu("sai0_div", SC_R_SAI_0, 2);
	clks[IMX8DV_SAI1_DIV]  = imx_clk_divider_scu("sai1_div", SC_R_SAI_1, 2);
	clks[IMX8DV_SAI2_DIV]  = imx_clk_divider_scu("sai2_div", SC_R_SAI_2, 2);
	clks[IMX8DV_UART0_DIV] = imx_clk_divider_scu("uart0_div", SC_R_UART_0, 2);
	clks[IMX8DV_UART1_DIV] = imx_clk_divider_scu("uart1_div", SC_R_UART_1, 2);
	clks[IMX8DV_UART2_DIV] = imx_clk_divider_scu("uart2_div", SC_R_UART_2, 2);
	clks[IMX8DV_SPI0_DIV]  = imx_clk_divider_scu("spi0_div", SC_R_SPI_0, 2);
	clks[IMX8DV_SPI1_DIV]  = imx_clk_divider_scu("spi1_div", SC_R_SPI_1, 2);
	clks[IMX8DV_SPI2_DIV]  = imx_clk_divider_scu("spi2_div", SC_R_SPI_2, 2);
	clks[IMX8DV_FTM0_DIV]  = imx_clk_divider_scu("ftm0_div", SC_R_FTM_0, 2);
	clks[IMX8DV_FTM1_DIV]  = imx_clk_divider_scu("ftm1_div", SC_R_FTM_1, 2);
	clks[IMX8DV_CAN0_DIV]  = imx_clk_divider_scu("can0_div", SC_R_CAN_0, 2);
	clks[IMX8DV_CAN1_DIV]  = imx_clk_divider_scu("can1_div", SC_R_CAN_1, 2);
	clks[IMX8DV_CAN2_DIV]  = imx_clk_divider_scu("can2_div", SC_R_CAN_2, 2);

	/*LSIO */
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

	/* Display controller */
	clks[IMX8DV_DC0_DISP0_DIV] = imx_clk_divider_scu("dc0_disp0_div", SC_R_DC_0, 0);
	clks[IMX8DV_DC0_DISP1_DIV] = imx_clk_divider_scu("dc0_disp1_div", SC_R_DC_0, 1);
	clks[IMX8DV_DC0_CAPT0_DIV] = imx_clk_divider_scu("dc0_capt0_div", SC_R_DC_0, 2);
	clks[IMX8DV_DC0_CAPT1_DIV] = imx_clk_divider_scu("dc0_capt1_div", SC_R_DC_0, 3);
	clks[IM8DV_DC0_GPU_2_DIV]  = imx_clk_divider_scu("dc0_gpu2_div", SC_R_GPU_2_PID0, 2);
	clks[IMX8DV_DC1_DISP0_DIV] = imx_clk_divider_scu("dc1_disp0_div", SC_R_DC_1, 0);
	clks[IMX8DV_DC1_DISP1_DIV] = imx_clk_divider_scu("dc1_disp1_div", SC_R_DC_1, 1);
	clks[IMX8DV_DC1_CAPT0_DIV] = imx_clk_divider_scu("dc1_capt0_div", SC_R_DC_1, 2);
	clks[IMX8DV_DC1_CAPT1_DIV] = imx_clk_divider_scu("dc1_capt1_div", SC_R_DC_1, 3);
	clks[IM8DV_DC1_GPU_3_DIV]  = imx_clk_divider_scu("dc1_gpu3_div", SC_R_GPU_3_PID0, 2);

	/* HSIO subsystem */
	clks[IMX8DV_PCIE_PER_DIV] = imx_clk_divider_scu("pcie_per_div", SC_R_PCIE_A, 2);
	clks[IMX8DV_PCIE_PHY_REF_DIV]  = imx_clk_divider_scu("pcie_phy_ref_div", SC_R_PCIE_PHY, 3);
	clks[IMX8DV_PCIE_PHY_PIPE_DIV] = imx_clk_divider_scu("pcie_phy_pipe_div", SC_R_PCIE_PHY, 4);

	/* LCD Subsystem */
	clks[IMX8DV_LCD_PIXEL_DIV] = imx_clk_divider_scu("lcd_pixel_div", SC_R_LCD_0, 2);
	clks[IMX8DV_LCD_I2C0_DIV] = imx_clk_divider_scu("lcd_i2c0_div", SC_R_LCD_0_I2C_0, 2);
	clks[IMX8DV_LCD_I2C1_DIV] = imx_clk_divider_scu("lcd_i2c1_div", SC_R_LCD_0_I2C_1, 2);
	clks[IMX8DV_LCD_PWM_DIV] = imx_clk_divider_scu("lcd_pwm_div", SC_R_LCD_0_PWM_0, 2);

	/* LVDS subsystem */
	clks[IMX8DV_LVDS_0_PIXEL_DIV] = imx_clk_divider_scu("lvds0_pixel_div", SC_R_LVDS_0, 2);
	clks[IMX8DV_LVDS_0_PHY_DIV] = imx_clk_divider_scu("lvds0_phy_div", SC_R_LVDS_0, 3);
	clks[IM8DV_LVDS_0_PWM_DIV] = imx_clk_divider_scu("lvds0_pwm_div", SC_R_LVDS_0_PWM_0, 2);
	clks[IM8DV_LVDS_0_I2C0_DIV] = imx_clk_divider_scu("lvds0_i2c0_div", SC_R_LVDS_0_I2C_0, 2);
	clks[IM8DV_LVDS_0_I2C1_DIV] = imx_clk_divider_scu("lvds0_i2c1_div", SC_R_LVDS_0_I2C_1, 2);
	clks[IMX8DV_LVDS_1_PIXEL_DIV] = imx_clk_divider_scu("lvds1_pixel_div", SC_R_LVDS_1, 2);
	clks[IMX8DV_LVDS_1_PHY_DIV] = imx_clk_divider_scu("lvds1_phy_div", SC_R_LVDS_1, 3);
	clks[IM8DV_LVDS_1_PWM_DIV] = imx_clk_divider_scu("lvds1_pwm_div", SC_R_LVDS_1_PWM_0, 2);
	clks[IM8DV_LVDS_1_I2C0_DIV] = imx_clk_divider_scu("lvds1_i2c0_div", SC_R_LVDS_1_I2C_0, 2);
	clks[IM8DV_LVDS_1_I2C1_DIV] = imx_clk_divider_scu("lvds1_i2c1_div", SC_R_LVDS_1_I2C_1, 2);
	clks[IMX8DV_LVDS_2_PIXEL_DIV] = imx_clk_divider_scu("lvds2_pixel_div", SC_R_LVDS_2, 2);
	clks[IMX8DV_LVDS_2_PHY_DIV] = imx_clk_divider_scu("lvds2_phy_div", SC_R_LVDS_2, 3);
	clks[IM8DV_LVDS_2_PWM_DIV] = imx_clk_divider_scu("lvds2_pwm_div", SC_R_LVDS_2_PWM_0, 2);
	clks[IM8DV_LVDS_2_I2C0_DIV] = imx_clk_divider_scu("lvds2_i2c0_div", SC_R_LVDS_2_I2C_0, 2);
	clks[IM8DV_LVDS_2_I2C1_DIV] = imx_clk_divider_scu("lvds2_i2c1_div", SC_R_LVDS_2_I2C_1, 2);

	/* Pixel Link Subsystem. */
	clks[IMX8DV_PL_0_PIXEL_DIV] = imx_clk_divider_scu("pl_0_pixel_div", SC_R_PI_0, 2);
	clks[IMX8DV_PL_0_MCLK_DIV] = imx_clk_divider_scu("pl_0_mclk_div", SC_R_PI_0, 4);
	clks[IMX8DV_PL_0_I2C_DIV] = imx_clk_divider_scu("pl_0_i2c_div", SC_R_PI_0_I2C_0, 2);
	clks[IMX8DV_PL_0_PWM0_DIV] = imx_clk_divider_scu("pl_0_pwm_0_div", SC_R_PI_0_PWM_0, 2);
	clks[IMX8DV_PL_0_PWM1_DIV] = imx_clk_divider_scu("pl_0_pwm_1_div", SC_R_PI_0_PWM_1, 2);
	clks[IMX8DV_PL_1_PIXEL_DIV] = imx_clk_divider_scu("pl_1_pixel_div", SC_R_PI_1, 2);
	clks[IMX8DV_PL_1_MCLK_DIV] = imx_clk_divider_scu("pl_1_mclk_div", SC_R_PI_1, 4);
	clks[IMX8DV_PL_1_I2C_DIV] = imx_clk_divider_scu("pl_1_i2c_div", SC_R_PI_1_I2C_0, 2);
	clks[IMX8DV_PL_1_PWM0_DIV] = imx_clk_divider_scu("pl_1_pwm_0_div", SC_R_PI_1_PWM_0, 2);
	clks[IMX8DV_PL_1_PWM1_DIV] = imx_clk_divider_scu("pl_1_pwm_1_div", SC_R_PI_1_PWM_1, 2);

	/* M4 subsystem. */
	clks[IMX8DV_M4_0_CORE_DIV] = imx_clk_divider_scu("m4_0_core_div", SC_R_M4_0_PID0, 2);
	clks[IMX8DV_M4_0_TPM_DIV] = imx_clk_divider_scu("m4_0_tpm_div", SC_R_M4_0_TPM, 2);
	clks[IMX8DV_M4_0_LPIT_DIV] = imx_clk_divider_scu("m4_0_lpit_div", SC_R_M4_0_PIT, 2);
	clks[IMX8DV_M4_0_I2C_DIV] = imx_clk_divider_scu("m4_0_i2c_div", SC_R_M4_0_I2C, 2);
	clks[IMX8DV_M4_0_UART_DIV] = imx_clk_divider_scu("m4_0_uart_div", SC_R_M4_0_UART, 2);
	clks[IMX8DV_M4_1_CORE_DIV] = imx_clk_divider_scu("m4_1_core_div", SC_R_M4_1_PID0, 2);
	clks[IMX8DV_M4_1_TPM_DIV] = imx_clk_divider_scu("m4_1_tpm_div", SC_R_M4_1_TPM, 2);
	clks[IMX8DV_M4_1_LPIT_DIV] = imx_clk_divider_scu("m4_1_lpit_div", SC_R_M4_1_PIT, 2);
	clks[IMX8DV_M4_1_I2C_DIV] = imx_clk_divider_scu("m4_1_i2c_div", SC_R_M4_1_I2C, 2);
	clks[IMX8DV_M4_1_UART_DIV] = imx_clk_divider_scu("m4_1_uart_div", SC_R_M4_1_UART, 2);

	/* VPU subsystem. */
	clks[IMX8DV_VPU_DDR_DIV] = imx_clk_divider_scu("vpu_ddr_div", SC_R_VPU_PID0, 0);
	clks[IMX8DV_VPU_CABAC_DIV] = imx_clk_divider_scu("vpu_cabac_div", SC_R_VPU_PID0, 1);
	clks[IMX8DV_VPU_XUVI_DIV] = imx_clk_divider_scu("vpu_xuvi_div", SC_R_VPU_PID0, 2);
	clks[IMX8DV_VPU_UART_DIV] = imx_clk_divider_scu("vpu_uart_div", SC_R_VPU_UART, 2);
	clks[IMX8DV_VPU_CORE_DIV] = imx_clk_divider_scu("vpu_core_div", SC_R_VPUCORE, 2);

	/* GPU */
	clks[IMX8DV_GPU0_CORE_DIV] = imx_clk_divider_scu("gpu0_core_div", SC_R_GPU_0_PID0, 2);
	clks[IMX8DV_GPU0_SHADER_DIV] = imx_clk_divider_scu("gpu0_shader_div", SC_R_GPU_0_PID0, 4);
	clks[IMX8DV_GPU1_CORE_DIV] = imx_clk_divider_scu("gpu1_core_div", SC_R_GPU_1_PID0, 2);
	clks[IMX8DV_GPU1_SHADER_DIV] = imx_clk_divider_scu("gpu1_shader_div", SC_R_GPU_1_PID0, 4);

	/* Gate-able clocks. */
	/* LSIO */
	clks[IMX8DV_I2C0_DIV_CLK] = imx_clk_gate_scu("i2c0_div_clk", "i2c0_div", SC_R_I2C_0, 2, (void __iomem *)(I2C_0_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C1_DIV_CLK] = imx_clk_gate_scu("i2c1_div_clk", "i2c0_div", SC_R_I2C_1, 2, (void __iomem *)(I2C_1_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C2_DIV_CLK] = imx_clk_gate_scu("i2c2_div_clk", "i2c0_div", SC_R_I2C_2, 2, (void __iomem *)(I2C_2_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C3_DIV_CLK] = imx_clk_gate_scu("i2c3_div_clk", "i2c0_div", SC_R_I2C_3, 2, (void __iomem *)(I2C_3_LPCG + 0x8), 0, 0);
	clks[IMX8DV_I2C0_CLK] = imx_clk_gate_scu("i2c0_clk", "i2c0_div_clk", SC_R_I2C_0, 2, (void __iomem *)(I2C_0_LPCG + 0xc), 0, 0);
	clks[IMX8DV_I2C1_CLK] = imx_clk_gate_scu("i2c1_clk", "i2c1_div_clk", SC_R_I2C_1, 2, (void __iomem *)(I2C_1_LPCG + 0xc), 0, 0);
	clks[IMX8DV_I2C2_CLK] = imx_clk_gate_scu("i2c2_clk", "i2c2_div_clk", SC_R_I2C_2, 2, (void __iomem *)(I2C_2_LPCG + 0xc), 0, 0);
	clks[IMX8DV_I2C3_CLK] = imx_clk_gate_scu("i2c3_clk", "i2c3_div_clk", SC_R_I2C_3, 2, (void __iomem *)(I2C_3_LPCG + 0xc), 0, 0);
	clks[IMX8DV_PWM0_CLK] = imx_clk_gate_scu("pwm0_clk", "pwm0_div", SC_R_PWM_0, 2, (void __iomem *)(PWM_0_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM1_CLK] = imx_clk_gate_scu("pwm1_clk", "pwm1_div", SC_R_PWM_1, 2, (void __iomem *)(PWM_1_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM2_CLK] = imx_clk_gate_scu("pwm2_clk", "pwm2_div", SC_R_PWM_2, 2, (void __iomem *)(PWM_2_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM3_CLK] = imx_clk_gate_scu("pwm3_clk", "pwm3_div", SC_R_PWM_3, 2, (void __iomem *)(PWM_3_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM4_CLK] = imx_clk_gate_scu("pwm4_clk", "pwm4_div", SC_R_PWM_4, 2, (void __iomem *)(PWM_4_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM5_CLK] = imx_clk_gate_scu("pwm5_clk", "pwm5_div", SC_R_PWM_5, 2, (void __iomem *)(PWM_5_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM6_CLK] = imx_clk_gate_scu("pwm6_clk", "pwm6_div", SC_R_PWM_6, 2, (void __iomem *)(PWM_6_LPCG + 0x4), 0, 0);
	clks[IMX8DV_PWM7_CLK] = imx_clk_gate_scu("pwm7_clk", "pwm7_div", SC_R_PWM_7, 2, (void __iomem *)(PWM_7_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT0_CLK] = imx_clk_gate_scu("gpt0_clk", "gpt0_div", SC_R_GPT_0, 2, (void __iomem *)(GPT_0_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT1_CLK] = imx_clk_gate_scu("gpt1_clk", "gpt1_div", SC_R_GPT_1, 2, (void __iomem *)(GPT_1_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT2_CLK] = imx_clk_gate_scu("gpt2_clk", "gpt2_div", SC_R_GPT_2, 2, (void __iomem *)(GPT_2_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT3_CLK] = imx_clk_gate_scu("gpt3_clk", "gpt3_div", SC_R_GPT_3, 2, (void __iomem *)(GPT_3_LPCG + 0x4), 0, 0);
	clks[IMX8DV_GPT4_CLK] = imx_clk_gate_scu("gpt4_clk", "gpt4_div", SC_R_GPT_4, 2, (void __iomem *)(GPT_4_LPCG + 0x4), 0, 0);

	/* Megawrap */
	clks[IMX8DV_SDHC_BUS_CLK] = imx_clk_gate_scu("sdhc_bus_clk", "sdhc_bus_clk_fixed", SC_R_SDHC_0, 1, NULL, 0, 0);
	clks[IMX8DV_ENET_BUS_CLK] = imx_clk_gate_scu("enet_bus_clk", "enet_bus_clk_fixed", SC_R_ENET_0, 1, NULL, 0, 0);
	clks[IMX8DV_ANATOP_CLK] = imx_clk_gate_scu("enet_anatop_clk", "anatop_bus_clk_fixed", SC_R_ENET_0, 1, NULL, 0, 0);
	clks[IMX8DV_CAN_OSC_CLK]  = imx_clk_gate_scu("can_osc_clk", NULL, SC_R_CAN_0, 4, NULL, 0, 0);
	clks[IMX8DV_USB_PHY_CLK]  = imx_clk_gate_scu("usb_phy_clk", NULL, SC_R_USB_0, 3, NULL, 0, 0);
	clks[IMX8DV_SDHC0_CLK] = imx_clk_gate_scu("sdhc0_clk", "sdhc0_div", SC_R_SDHC_0, 2, NULL, 0, 0);
	clks[IMX8DV_SDHC1_CLK] = imx_clk_gate_scu("sdhc1_clk", "sdhc1_div", SC_R_SDHC_1, 2, NULL, 0, 0);
	clks[IMX8DV_SDHC2_CLK] = imx_clk_gate_scu("sdhc2_clk", "sdhc2_div", SC_R_SDHC_2, 2, NULL, 0, 0);
	clks[IMX8DV_ENET0_CLK] = imx_clk_gate_scu("enet0_clk", "enet0_div", SC_R_ENET_0, 2, NULL, 0, 0);
	clks[IMX8DV_ENET1_CLK] = imx_clk_gate_scu("enet1_clk", "enet1_div", SC_R_ENET_1, 2, NULL, 0, 0);
	clks[IMX8DV_ENET0_TIME_CLK] = imx_clk_gate_scu("enet0_time_clk", "enet0_time_div", SC_R_ENET_0, 4, NULL, 0, 0);
	clks[IMX8DV_ENET1_TIME_CLK] = imx_clk_gate_scu("enet1_time_clk", "enet1_time_div", SC_R_ENET_1, 4, NULL, 0, 0);
	clks[IMX8DV_QSPI_CLK]  = imx_clk_gate_scu("qspi_clk", "qspi_div", SC_R_QSPI_0, 2, NULL, 0, 0);
	clks[IMX8DV_SAI0_CLK]  = imx_clk_gate_scu("sai0_clk", "sai0_div", SC_R_SAI_0, 2, NULL, 0, 0);
	clks[IMX8DV_SAI1_CLK]  = imx_clk_gate_scu("sai1_clk", "sai1_div", SC_R_SAI_1, 2, NULL, 0, 0);
	clks[IMX8DV_SAI2_CLK]  = imx_clk_gate_scu("sai2_clk", "sai2_div", SC_R_SAI_2, 2, NULL, 0, 0);
	clks[IMX8DV_UART0_CLK] = imx_clk_gate_scu("uart0_clk", "uart0_div", SC_R_UART_0, 2, NULL, 0, 0);
	clks[IMX8DV_UART1_CLK] = imx_clk_gate_scu("uart1_clk", "uart1_div", SC_R_UART_1, 2, NULL, 0, 0);
	clks[IMX8DV_UART2_CLK] = imx_clk_gate_scu("uart2_clk", "uart2_div", SC_R_UART_2, 2, NULL, 0, 0);
	clks[IMX8DV_SPI0_CLK]  = imx_clk_gate_scu("SPI0_clk", "spi0_div", SC_R_SPI_0, 2, NULL, 0, 0);
	clks[IMX8DV_SPI1_CLK]  = imx_clk_gate_scu("SPI1_clk", "spi1_div", SC_R_SPI_1, 2, NULL, 0, 0);
	clks[IMX8DV_SPI2_CLK]  = imx_clk_gate_scu("SPI2_clk", "spi2_div", SC_R_SPI_2, 2, NULL, 0, 0);
	clks[IMX8DV_FTM0_CLK]  = imx_clk_gate_scu("FTM0_clk", "ftm0_div", SC_R_FTM_0, 2, NULL, 0, 0);
	clks[IMX8DV_FTM1_CLK]  = imx_clk_gate_scu("FTM1_clk", "ftm1_div", SC_R_FTM_1, 2, NULL, 0, 0);
	clks[IMX8DV_CAN0_CLK]  = imx_clk_gate_scu("can0_clk", "can0_div", SC_R_CAN_0, 2, NULL, 0, 0);
	clks[IMX8DV_CAN1_CLK]  = imx_clk_gate_scu("can1_clk", "can1_div", SC_R_CAN_1, 2, NULL, 0, 0);
	clks[IMX8DV_CAN2_CLK]  = imx_clk_gate_scu("can2_clk", "can2_div", SC_R_CAN_2, 2, NULL, 0, 0);

	/* Display controller */
	clks[IMX8DV_DC0_DISP0_CLK] = imx_clk_gate_scu("dc0_disp0_clk", "dc0_disp0_div", SC_R_DC_0, 0, (void __iomem *)(DC_0_LPCG + 0x14), 0, 0);
	clks[IMX8DV_DC0_DISP1_CLK] = imx_clk_gate_scu("dc0_disp1_clk", "dc0_disp1_div", SC_R_DC_0, 1, (void __iomem *)(DC_0_LPCG + 0x18), 0, 0);
	clks[IMX8DV_DC0_CAPT0_CLK] = imx_clk_gate_scu("dc0_capt0_clk", "dc0_capt0_div", SC_R_DC_0, 2, NULL, 0, 0);
	clks[IMX8DV_DC0_CAPT1_CLK] = imx_clk_gate_scu("dc0_capt1_clk", "dc0_capt1_div", SC_R_DC_0, 3, NULL, 0, 0);
	clks[IM8DV_DC0_GPU_2_CLK] = imx_clk_gate_scu("dc0_gpu2_clk", "dc0_gpu2_div", SC_R_GPU_2_PID0, 2, (void __iomem *)(DC_0_LPCG + 0x0), 0, 0);
	clks[IMX8DV_DC1_DISP0_CLK] = imx_clk_gate_scu("dc1_disp0_clk", "dc1_disp0_div", SC_R_DC_1, 0, (void __iomem *)(DC_1_LPCG + 0x14), 0, 0);
	clks[IMX8DV_DC1_DISP1_CLK] = imx_clk_gate_scu("dc1_disp1_clk", "dc1_disp1_div", SC_R_DC_1, 1, (void __iomem *)(DC_0_LPCG + 0x18), 0, 0);
	clks[IMX8DV_DC1_CAPT0_CLK] = imx_clk_gate_scu("dc1_capt0_clk", "dc1_capt0_div", SC_R_DC_1, 2, NULL, 0, 0);
	clks[IMX8DV_DC1_CAPT1_CLK] = imx_clk_gate_scu("dc1_capt1_clk", "dc1_capt1_div", SC_R_DC_1, 3, NULL, 0, 0);
	clks[IM8DV_DC1_GPU_3_CLK] = imx_clk_gate_scu("dc1_gpu2_clk", "dc1_gpu2_div", SC_R_GPU_3_PID0, 2, (void __iomem *)(DC_1_LPCG + 0x0), 0, 0);

	/* HSIO subsystem */
	clks[IMX8DV_PCIE_PER_CLK] = imx_clk_gate_scu("pcie_per_clk", "pcie_per_div", SC_R_PCIE_A, 2, (void __iomem *)(PCIE_PER_LPCG + 0x0), 0, 0);
	clks[IMX8DV_PCIE_PHY_REF_CLK]  = imx_clk_gate_scu("pcie_phy_ref_clk", "pcie_phy_ref_div", SC_R_PCIE_PHY, 3, (void __iomem *)(PCIE_PHY_LPCG + 0x0), 0, 0);
	clks[IMX8DV_PCIE_PHY_PIPE_CLK] = imx_clk_gate_scu("pcie_phy_pipe_clk", "pcie_phy_pipe_div", SC_R_PCIE_PHY, 4, NULL, 0, 0);

	/* LCD Subsystem. */
	clks[IMX8DV_LCD_I2C0_CLK] = imx_clk_gate_scu("lcd_i2c0_clk", "lcd_i2c0_div", SC_R_LCD_0_I2C_0, 2,(void __iomem *)(LCD_LPCG + 0x1c), 0, 0);
	clks[IMX8DV_LCD_I2C1_CLK] = imx_clk_gate_scu("lcd_i2c1_clk", "lcd_i2c1_div", SC_R_LCD_0_I2C_1, 2,(void __iomem *)(LCD_LPCG + 0x2c), 0, 0);
	clks[IMX8DV_LCD_PWM_CLK] = imx_clk_gate_scu("lcd_pwm_clk", "lcd_pwm_div", SC_R_LCD_0_PWM_0, 2, (void __iomem *)(LCD_LPCG + 0x10), 0, 0);

	/* LVDS subsystem */
	clks[IMX8DV_LVDS_0_PHY_CLK] = imx_clk_gate_scu("lvds0_phy_clk", "lvds0_phy_div", SC_R_LVDS_0, 3, NULL, 0, 0);
	clks[IM8DV_LVDS_0_PWM_CLK] = imx_clk_gate_scu("lvds0_pwm_clk", "lvds0_pwm_div", SC_R_LVDS_0_PWM_0, 2, (void __iomem *)(LVDS_0_LPCG + 0x10), 0, 0);
	clks[IM8DV_LVDS_0_I2C0_CLK] = imx_clk_gate_scu("lvds0_ic20_clk", "lvds0_i2c0_div", SC_R_LVDS_0_I2C_0, 2, (void __iomem *)(LVDS_0_LPCG + 0x1c), 0, 0);
	clks[IM8DV_LVDS_0_I2C1_CLK] = imx_clk_gate_scu("lvds0_ic21_clk", "lvds0_i2c1_div", SC_R_LVDS_0_I2C_1, 2, (void __iomem *)(LVDS_0_LPCG + 0x2c), 0, 0);
	clks[IMX8DV_LVDS_1_PHY_CLK] = imx_clk_gate_scu("lvds1_phy_clk", "lvds1_phy_div", SC_R_LVDS_1, 3, NULL, 0, 0);
	clks[IM8DV_LVDS_1_PWM_CLK] = imx_clk_gate_scu("lvds1_pwm_clk", "lvds1_pwm_div", SC_R_LVDS_1_PWM_0, 2, (void __iomem *)(LVDS_1_LPCG + 0x10), 0, 0);
	clks[IM8DV_LVDS_1_I2C0_CLK] = imx_clk_gate_scu("lvds1_ic20_clk", "lvds1_i2c0_div", SC_R_LVDS_1_I2C_0, 2, (void __iomem *)(LVDS_1_LPCG + 0x1c), 0, 0);
	clks[IM8DV_LVDS_1_I2C1_CLK] = imx_clk_gate_scu("lvds1_ic21_clk", "lvds1_i2c1_div", SC_R_LVDS_1_I2C_1, 2, (void __iomem *)(LVDS_1_LPCG + 0x2c), 0, 0);
	clks[IMX8DV_LVDS_2_PHY_CLK] = imx_clk_gate_scu("lvds2_phy_clk", "lvds2_phy_div", SC_R_LVDS_2, 3, NULL, 0, 0);
	clks[IM8DV_LVDS_2_PWM_CLK] = imx_clk_gate_scu("lvds2_pwm_clk", "lvds2_pwm_div", SC_R_LVDS_2_PWM_0, 2, (void __iomem *)(LVDS_2_LPCG + 0x10), 0, 0);
	clks[IM8DV_LVDS_2_I2C0_CLK] = imx_clk_gate_scu("lvds2_ic20_clk", "lvds2_i2c0_div", SC_R_LVDS_2_I2C_0, 2, (void __iomem *)(LVDS_2_LPCG + 0x1c), 0, 0);
	clks[IM8DV_LVDS_2_I2C1_CLK] = imx_clk_gate_scu("lvds2_ic21_clk", "lvds2_i2c1_div", SC_R_LVDS_2_I2C_1, 2, (void __iomem *)(LVDS_2_LPCG + 0x2c), 0, 0);

	/* Capture Pixel Link Subsystem. Need to fix the LPCG stuff once the offsets are known. Know the base address of LPCG.*/
	clks[IMX8DV_PL_0_MCLK_CLK] = imx_clk_gate_scu("pl_0_mclk_clk", "pl_0_mclk_div", SC_R_PI_0, 4, NULL, 0, 0);
	clks[IMX8DV_PL_0_I2C_CLK] = imx_clk_gate_scu("pl_0_i2c_clk", "pl_0_i2c_div", SC_R_PI_0_I2C_0, 2, NULL, 0, 0);
	clks[IMX8DV_PL_0_PWM0_CLK] = imx_clk_gate_scu("pl_0_pwm_0_clk", "pl_0_pwm_0_div", SC_R_PI_0_PWM_0, 2, NULL, 0, 0);
	clks[IMX8DV_PL_0_PWM1_CLK] = imx_clk_gate_scu("pl_0_pwm_1_clk", "pl_0_pwm_1_div", SC_R_PI_0_PWM_1, 2, NULL, 0, 0);
	clks[IMX8DV_PL_1_MCLK_CLK] = imx_clk_gate_scu("pl_1_mclk_clk", "pl_1_mclk_div", SC_R_PI_1, 4, NULL, 0, 0);
	clks[IMX8DV_PL_1_I2C_CLK] = imx_clk_gate_scu("pl_1_i2c_clk", "pl_1_i2c_div", SC_R_PI_1_I2C_0, 2, NULL, 0, 0);
	clks[IMX8DV_PL_1_PWM0_CLK] = imx_clk_gate_scu("pl_1_pwm_0_clk", "pl_1_pwm_0_div", SC_R_PI_1_PWM_0, 2, NULL, 0, 0);
	clks[IMX8DV_PL_1_PWM1_CLK] = imx_clk_gate_scu("pl_1_pwm_1_clk", "pl_1_pwm_1_div", SC_R_PI_1_PWM_1, 2, NULL, 0, 0);

	/* M4 Clocks. */
	clks[IMX8DV_M4_0_TPM_CLK] = imx_clk_gate_scu("m4_0_tpm_clk", "m4_0_tpm_div", SC_R_M4_0_TPM, 2,NULL, 0, 0);
	clks[IMX8DV_M4_0_LPIT_CLK] = imx_clk_gate_scu("m4_0_lpit_clk", "m4_0_lpit_div", SC_R_M4_0_PIT, 2, (void __iomem *)(M4_0_LPIT_LPCG), 0, 0);
	clks[IMX8DV_M4_0_I2C_CLK] = imx_clk_gate_scu("m4_0_i2c_clk", "m4_0_i2c_div", SC_R_M4_0_I2C, 2, (void __iomem *)(M4_0_I2C_LPCG), 0, 0);
	clks[IMX8DV_M4_0_UART_CLK] = imx_clk_gate_scu("m4_0_uart_clk", "m4_0_uart_div", SC_R_M4_0_UART, 2, (void __iomem *)(M4_0_LPUART_LPCG), 0, 0);
	clks[IMX8DV_M4_1_TPM_CLK] = imx_clk_gate_scu("m4_1_tpm_clk", "m4_1_tpm_div", SC_R_M4_1_TPM, 2, NULL, 0, 0);
	clks[IMX8DV_M4_1_LPIT_CLK] = imx_clk_gate_scu("m4_1_lpit_clk", "m4_1_lpit_div", SC_R_M4_1_PIT, 2, (void __iomem *)(M4_1_LPIT_LPCG), 0, 0);
	clks[IMX8DV_M4_1_I2C_CLK] = imx_clk_gate_scu("m4_1_i2c_clk", "m4_1_i2c_div", SC_R_M4_1_I2C, 2, (void __iomem *)(M4_1_I2C_LPCG), 0, 0);
	clks[IMX8DV_M4_1_UART_CLK] = imx_clk_gate_scu("m4_1_uart_clk", "m4_1_uart_div", SC_R_M4_1_UART, 2, (void __iomem *)(M4_1_LPUART_LPCG), 0, 0);

	/* VPU subsystem */
	clks[IMX8DV_VPU_DDR_CLK] = imx_clk_gate_scu("vpu_ddr_clk", "vpu_ddr_div", SC_R_VPU_PID0, 0, NULL, 0, 0);
	clks[IMX8DV_VPU_CABAC_CLK] = imx_clk_gate_scu("vpu_cabac_clk", "vpu_cabac_div", SC_R_VPU_PID0, 1, NULL, 0, 0);
	clks[IMX8DV_VPU_XUVI_CLK] = imx_clk_gate_scu("vpu_xuvi_clk", "vpu_xuvi_div", SC_R_VPU_PID0, 2, NULL, 0, 0);
	clks[IMX8DV_VPU_UART_CLK] = imx_clk_gate_scu("vpu_uart_clk", "vpu_uart_div", SC_R_VPU_UART, 2, NULL, 0, 0);

	/* GPU susbsystem  */
	clks[IMX8DV_GPU0_CORE_CLK] = imx_clk_gate_scu("gpu_core0_clk", "gpu0_core_div", SC_R_GPU_0_PID0, 2, NULL, 0, 0);
	clks[IMX8DV_GPU0_SHADER_CLK] = imx_clk_gate_scu("gpu_shader0_clk", "gpu0_shader_div", SC_R_GPU_0_PID0, 4, NULL, 0, 0);
	clks[IMX8DV_GPU1_CORE_CLK] = imx_clk_gate_scu("gpu_core1_clk", "gpu1_core_div", SC_R_GPU_1_PID0, 2, NULL, 0, 0);
	clks[IMX8DV_GPU1_SHADER_CLK] = imx_clk_gate_scu("gpu_shader1_clk", "gpu1_shader_div", SC_R_GPU_1_PID0, 4, NULL, 0, 0);

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

