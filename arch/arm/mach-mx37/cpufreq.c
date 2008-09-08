/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file cpufreq.c
 *
 * @brief A driver for the Freescale Semiconductor i.MX37 CPUfreq module.
 *
 * The CPUFREQ driver is for controling CPU frequency. It allows you to change
 * the CPU clock speed on the fly.
 *
 * @ingroup PM
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/regulator/regulator.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/arch/clock.h>
#include <asm/cacheflush.h>

/*
 * Frequencys can be set for CPU.
 */
#define CPU_FREQUENCY_200000_KHZ		200000
#define CPU_FREQUENCY_532000_KHZ		532000
#define ARM_LPM_CLK  200000000
#define ARM_NORMAL_CLK  532000000

#define LP_LPM_CLK  24000000
#define LP_NORMAL_CLK  133000000
#define GP_LPM_VOLTAGE 850000
#define LP_LPM_VOLTAGE 1000000
#define GP_NORMAL_VOLTAGE 1000000
#define LP_NORMAL_VOLTAGE 1200000

int low_bus_freq_mode;
int high_bus_freq_mode;
static struct clk *cpu_clk;
static struct clk *main_bus_clk;
static struct clk *pll2;
static struct clk *axi_a_clk;
static struct clk *axi_b_clk;
static struct clk *axi_c_clk;
static struct clk *emi_core_clk;
static struct clk *nfc_clk;
static struct clk *ahb_clk;
static struct clk *vpu_clk;
static struct clk *vpu_core_clk;
static struct clk *arm_axi_clk;
static struct clk *ddr_clk;
static struct clk *ipu_clk;
static struct clk *periph_apm_clk;
static struct clk *lp_apm;
static struct clk *osc;
static struct regulator *gp_regulator;
static struct regulator *lp_regulator;

/* does need to be in ascending order for calc_frequency_khz() below */
static struct cpufreq_frequency_table imx37_freq_table[] = {
	{0x01, CPU_FREQUENCY_200000_KHZ},
	{0x02, CPU_FREQUENCY_532000_KHZ},
	{0, CPUFREQ_TABLE_END},
};

int set_cpu_freq(int freq)
{
	int ret = 0;
	int org_cpu_rate;

	org_cpu_rate = clk_get_rate(cpu_clk);
	if (org_cpu_rate == freq)
		return ret;

	if (freq == ARM_NORMAL_CLK) {
		/*Set the voltage to 1.00v for the GP domain. */

		ret = regulator_set_voltage(gp_regulator, GP_NORMAL_VOLTAGE);
		if (ret < 0) {
			printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!\n");
			return ret;
		}

		ret = clk_set_rate(cpu_clk, ARM_NORMAL_CLK);
		if (ret != 0) {
			printk(KERN_DEBUG "cannot set CPU clock rate\n");
			return ret;
		}
	} else if (freq == ARM_LPM_CLK) {
		ret = clk_set_rate(cpu_clk, ARM_LPM_CLK);
		if (ret != 0) {
			printk(KERN_DEBUG "cannot set CPU clock rate\n");
			return ret;
		}

		/* Set the voltage to 0.8v for the GP domain. */
		ret = regulator_set_voltage(gp_regulator, GP_LPM_VOLTAGE);
		if (ret < 0) {
			printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!!\n");
			return ret;
		}
	}
	return ret;
}

int set_low_bus_freq(void)
{
	int ret = 0;

	struct clk *p_clk;
	struct clk *amode_parent_clk;

	amode_parent_clk = lp_apm;
	p_clk = clk_get_parent(periph_apm_clk);

	/* Make sure osc_clk is the parent of lp_apm. */
	if (clk_get_parent(amode_parent_clk) != osc)
		clk_set_parent(amode_parent_clk, osc);

	/* Set the parent of periph_apm_clk to be lp_apm */
	clk_set_parent(periph_apm_clk, amode_parent_clk);
	amode_parent_clk = periph_apm_clk;

	p_clk = clk_get_parent(main_bus_clk);
	/* Set the parent of main_bus_clk to be periph_apm_clk */
	clk_set_parent(main_bus_clk, amode_parent_clk);
	/*disable the old parent. */
	clk_disable(p_clk);

	clk_set_rate(axi_a_clk, LP_LPM_CLK);
	clk_set_rate(axi_b_clk, LP_LPM_CLK);
	clk_set_rate(axi_c_clk, LP_LPM_CLK);
	clk_set_rate(emi_core_clk, LP_LPM_CLK);
	clk_set_rate(nfc_clk, 4800000);
	clk_set_rate(ahb_clk, LP_LPM_CLK);

	amode_parent_clk = emi_core_clk;

	p_clk = clk_get_parent(arm_axi_clk);
	if (p_clk != amode_parent_clk) {
		clk_set_parent(arm_axi_clk, amode_parent_clk);
		/* Disable the old parent */
		clk_disable(p_clk);
	}

	p_clk = clk_get_parent(vpu_clk);
	if (p_clk != amode_parent_clk) {
		clk_set_parent(vpu_clk, amode_parent_clk);
		/* Disable the old parent */
		clk_disable(p_clk);
	}

	p_clk = clk_get_parent(vpu_core_clk);
	if (p_clk != amode_parent_clk) {
		clk_set_parent(vpu_core_clk, amode_parent_clk);
		/* Disable the old parent */
		clk_disable(p_clk);
	}

	/* Set the voltage to 1.0v for the LP domain. */
	ret = regulator_set_voltage(lp_regulator, LP_LPM_VOLTAGE);
	if (ret < 0) {
		printk(KERN_DEBUG "COULD NOT SET GP VOLTAGE!!!!!!\n");
		return ret;
	}

	low_bus_freq_mode = 1;
	high_bus_freq_mode = 0;
	return ret;
}

int set_high_bus_freq(void)
{
	struct clk *p_clk;
	struct clk *rmode_parent_clk;
	int ret = 0;

	if (!low_bus_freq_mode)
		return ret;

	low_bus_freq_mode = 0;

	/* Set the voltage to 1.2v for the LP domain. */
	ret = regulator_set_voltage(lp_regulator, LP_NORMAL_VOLTAGE);
	if (ret < 0) {
		printk(KERN_DEBUG "COULD NOT SET LP VOLTAGE!!!!!!\n");
		return ret;
	}

	rmode_parent_clk = pll2;
	clk_enable(rmode_parent_clk);

	/* Set the dividers before setting the parent clock. */
	clk_set_rate(axi_a_clk, 4800000);
	clk_set_rate(axi_b_clk, 4000000);
	clk_set_rate(axi_c_clk, 6000000);

	clk_set_rate(emi_core_clk, 4800000);
	clk_set_rate(ahb_clk, 4800000);

	/* Set the parent of main_bus_clk to be pll2 */
	p_clk = clk_get_parent(main_bus_clk);
	clk_set_parent(main_bus_clk, rmode_parent_clk);
	/* Need to increase the count */
	clk_enable(rmode_parent_clk);
	udelay(5);
	high_bus_freq_mode = 1;
	return ret;
}

int low_freq_bus_used(void)
{
	if ((clk_get_usecount(ipu_clk) == 0)
	    && (clk_get_usecount(vpu_clk) == 0))
		return 1;
	else
		return 0;
}

static int mx37_verify_speed(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, imx37_freq_table);
}

static unsigned int mx37_get_speed(unsigned int cpu)
{
	if (cpu)
		return 0;

	return clk_get_rate(cpu_clk) / 1000;
}

static int calc_frequency_khz(int target, unsigned int relation)
{
	int i;

	if (relation == CPUFREQ_RELATION_H) {
		for (i = ARRAY_SIZE(imx37_freq_table) - 1; i > 0; i--) {
			if (imx37_freq_table[i].frequency <= target)
				return imx37_freq_table[i].frequency;
		}
	} else if (relation == CPUFREQ_RELATION_L) {
		for (i = 0; i < ARRAY_SIZE(imx37_freq_table) - 1; i++) {
			if (imx37_freq_table[i].frequency >= target)
				return imx37_freq_table[i].frequency;
		}
	}
	printk(KERN_ERR "Error: No valid cpufreq relation\n");
	return CPU_FREQUENCY_532000_KHZ;
}

static int mx37_set_target(struct cpufreq_policy *policy,
			   unsigned int target_freq, unsigned int relation)
{
	struct cpufreq_freqs freqs;
	long freq_Hz;
	int low_freq_bus_ready = 0;
	int ret = 0;

	/*
	 * Some governors do not respects CPU and policy lower limits
	 * which leads to bad things (division by zero etc), ensure
	 * that such things do not happen.
	 */
	if (target_freq < policy->cpuinfo.min_freq)
		target_freq = policy->cpuinfo.min_freq;

	if (target_freq < policy->min)
		target_freq = policy->min;

	freq_Hz = calc_frequency_khz(target_freq, relation) * 1000;

	freqs.old = clk_get_rate(cpu_clk) / 1000;
	freqs.new = freq_Hz / 1000;
	freqs.cpu = 0;
	freqs.flags = 0;

	low_freq_bus_ready = low_freq_bus_used();

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	if ((freq_Hz == ARM_LPM_CLK) && (!low_bus_freq_mode)
	    && (low_freq_bus_ready)) {
		set_low_bus_freq();
		ret = set_cpu_freq(freq_Hz);
	} else {
		if (!high_bus_freq_mode)
			set_high_bus_freq();

		ret = set_cpu_freq(freq_Hz);
		if (low_bus_freq_mode) {
			if (ret == 0)
				set_high_bus_freq();
		}
	}

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

static int __init mx37_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	int ret;

	printk(KERN_INFO "i.MX37 CPU frequency driver\n");

	if (policy->cpu != 0)
		return -EINVAL;

	cpu_clk = clk_get(NULL, "cpu_clk");
	if (IS_ERR(cpu_clk)) {
		printk(KERN_ERR "%s: failed to get cpu clock\n", __func__);
		return PTR_ERR(cpu_clk);
	}

	main_bus_clk = clk_get(NULL, "main_bus_clk");
	if (IS_ERR(main_bus_clk)) {
		printk(KERN_ERR "%s: failed to get main_bus_clk\n", __func__);
		return PTR_ERR(main_bus_clk);
	}

	pll2 = clk_get(NULL, "pll2");
	if (IS_ERR(pll2)) {
		printk(KERN_ERR "%s: failed to get pll2\n", __func__);
		return PTR_ERR(pll2);
	}

	axi_a_clk = clk_get(NULL, "axi_a_clk");
	if (IS_ERR(axi_a_clk)) {
		printk(KERN_ERR "%s: failed to get axi_a_clk\n", __func__);
		return PTR_ERR(axi_a_clk);
	}

	axi_b_clk = clk_get(NULL, "axi_b_clk");
	if (IS_ERR(axi_b_clk)) {
		printk(KERN_ERR "%s: failed to get axi_b_clk\n", __func__);
		return PTR_ERR(axi_b_clk);
	}

	axi_c_clk = clk_get(NULL, "axi_c_clk");
	if (IS_ERR(axi_c_clk)) {
		printk(KERN_ERR "%s: failed to get axi_c_clk\n", __func__);
		return PTR_ERR(axi_c_clk);
	}

	emi_core_clk = clk_get(NULL, "emi_core_clk");
	if (IS_ERR(emi_core_clk)) {
		printk(KERN_ERR "%s: failed to get emi_core_clk\n", __func__);
		return PTR_ERR(emi_core_clk);
	}

	nfc_clk = clk_get(NULL, "nfc_clk");
	if (IS_ERR(nfc_clk)) {
		printk(KERN_ERR "%s: failed to get nfc_clk\n", __func__);
		return PTR_ERR(nfc_clk);
	}

	ahb_clk = clk_get(NULL, "ahb_clk");
	if (IS_ERR(ahb_clk)) {
		printk(KERN_ERR "%s: failed to get ahb_clk\n", __func__);
		return PTR_ERR(ahb_clk);
	}

	vpu_core_clk = clk_get(NULL, "vpu_core_clk");
	if (IS_ERR(vpu_core_clk)) {
		printk(KERN_ERR "%s: failed to get vpu_core_clk\n", __func__);
		return PTR_ERR(vpu_core_clk);
	}

	arm_axi_clk = clk_get(NULL, "arm_axi_clk");
	if (IS_ERR(arm_axi_clk)) {
		printk(KERN_ERR "%s: failed to get arm_axi_clk\n", __func__);
		return PTR_ERR(arm_axi_clk);
	}

	ddr_clk = clk_get(NULL, "ddr_clk");
	if (IS_ERR(ddr_clk)) {
		printk(KERN_ERR "%s: failed to get ddr_clk\n", __func__);
		return PTR_ERR(ddr_clk);
	}

	ipu_clk = clk_get(NULL, "ipu_clk");
	if (IS_ERR(ipu_clk)) {
		printk(KERN_ERR "%s: failed to get ipu_clk\n", __func__);
		return PTR_ERR(ipu_clk);
	}

	vpu_clk = clk_get(NULL, "vpu_clk");
	if (IS_ERR(vpu_clk)) {
		printk(KERN_ERR "%s: failed to get vpu_clk\n", __func__);
		return PTR_ERR(vpu_clk);
	}

	periph_apm_clk = clk_get(NULL, "periph_apm_clk");
	if (IS_ERR(periph_apm_clk)) {
		printk(KERN_ERR "%s: failed to get periph_apm_clk\n", __func__);
		return PTR_ERR(periph_apm_clk);
	}

	lp_apm = clk_get(NULL, "lp_apm");
	if (IS_ERR(lp_apm)) {
		printk(KERN_ERR "%s: failed to get lp_apm\n", __func__);
		return PTR_ERR(lp_apm);
	}

	osc = clk_get(NULL, "osc");
	if (IS_ERR(osc)) {
		printk(KERN_ERR "%s: failed to get osc\n", __func__);
		return PTR_ERR(osc);
	}

	gp_regulator = regulator_get(NULL, "DCDC1");
	if (IS_ERR(gp_regulator)) {
		clk_put(cpu_clk);
		printk(KERN_ERR "%s: failed to get gp regulator\n", __func__);
		return PTR_ERR(gp_regulator);
	}

	lp_regulator = regulator_get(NULL, "DCDC4");
	if (IS_ERR(lp_regulator)) {
		clk_put(ahb_clk);
		printk(KERN_ERR "%s: failed to get lp regulator\n", __func__);
		return PTR_ERR(lp_regulator);
	}

	policy->cur = policy->min = policy->max = clk_get_rate(cpu_clk) / 1000;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;
	policy->cpuinfo.min_freq = CPU_FREQUENCY_200000_KHZ;
	policy->cpuinfo.max_freq = CPU_FREQUENCY_532000_KHZ;

	/* Manual states, that PLL stabilizes in two CLK32 periods */
	policy->cpuinfo.transition_latency = 10;

	ret = cpufreq_frequency_table_cpuinfo(policy, imx37_freq_table);
	if (ret < 0) {
		clk_put(cpu_clk);
		clk_put(main_bus_clk);
		clk_put(pll2);
		clk_put(axi_a_clk);
		clk_put(axi_b_clk);
		clk_put(axi_c_clk);
		clk_put(emi_core_clk);
		clk_put(nfc_clk);
		clk_put(ahb_clk);
		clk_put(vpu_core_clk);
		clk_put(arm_axi_clk);
		clk_put(ddr_clk);
		clk_put(ipu_clk);
		clk_put(vpu_clk);
		clk_put(periph_apm_clk);
		clk_put(lp_apm);
		clk_put(osc);
		regulator_put(gp_regulator, NULL);
		regulator_put(lp_regulator, NULL);
		printk(KERN_ERR "%s: failed to register i.MX37 CPUfreq\n",
		       __func__);
		return ret;
	}
	cpufreq_frequency_table_get_attr(imx37_freq_table, policy->cpu);

	low_bus_freq_mode = 0;
	high_bus_freq_mode = 0;
	return 0;
}

static int mx37_cpufreq_driver_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);

	/* reset CPU to 532MHz */
	set_cpu_freq(ARM_NORMAL_CLK);
	if (low_bus_freq_mode)
		set_high_bus_freq();

	clk_put(cpu_clk);
	clk_put(main_bus_clk);
	clk_put(pll2);
	clk_put(axi_a_clk);
	clk_put(axi_b_clk);
	clk_put(axi_c_clk);
	clk_put(emi_core_clk);
	clk_put(nfc_clk);
	clk_put(ahb_clk);
	clk_put(vpu_core_clk);
	clk_put(arm_axi_clk);
	clk_put(ddr_clk);
	clk_put(ipu_clk);
	clk_put(periph_apm_clk);
	clk_put(lp_apm);
	clk_put(osc);
	regulator_put(gp_regulator, NULL);
	regulator_put(lp_regulator, NULL);
	return 0;
}

static struct cpufreq_driver mx37_driver = {
	.flags = CPUFREQ_STICKY,
	.verify = mx37_verify_speed,
	.target = mx37_set_target,
	.get = mx37_get_speed,
	.init = mx37_cpufreq_driver_init,
	.exit = mx37_cpufreq_driver_exit,
	.name = "imx37",
};

static int __devinit mx37_cpufreq_init(void)
{
	return cpufreq_register_driver(&mx37_driver);
}

static void mx37_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&mx37_driver);
}

module_init(mx37_cpufreq_init);
module_exit(mx37_cpufreq_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("CPUfreq driver for i.mx37");
MODULE_LICENSE("GPL");
