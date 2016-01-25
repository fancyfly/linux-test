/*
 * imx8 power management support
 *
 * Copyright (C) 2013  NXP Inc

 *
 * based on pm-rmoblie.c
 *  Copyright (C) 2014 Glider bvba
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_clock.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <soc/imx8/sc/sci.h>

#include "pm-domain-imx8.h"

static sc_ipc_t pm_ipcHandle;

static int imx8_pd_power(struct generic_pm_domain *domain, bool power_on)
{
	struct imx8_pm_domain *pd;
	sc_err_t sciErr;

	pd = container_of(domain, struct imx8_pm_domain, pd);

	if (pd->rsrc_id == SC_R_LAST)
		return 0;

	sciErr = sc_pm_set_resource_power_mode(pm_ipcHandle, pd->rsrc_id,
									(power_on) ? SC_PM_PW_MODE_ON : SC_PM_PW_MODE_OFF);

	if (sciErr)
		printk("Failed power operation on resource %d\n", pd->rsrc_id);

	return 0;
}

static int imx8_pd_power_on(struct generic_pm_domain *domain)
{
	return imx8_pd_power(domain, true);
}

static int imx8_pd_power_off(struct generic_pm_domain *domain)
{
	return imx8_pd_power(domain, false);
}


static int __init imx8_add_pm_domains(struct device_node *parent,
					struct generic_pm_domain *genpd_parent)
{
	struct device_node *np;

	for_each_child_of_node(parent, np) {
		struct imx8_pm_domain *imx8_pd;
		sc_rsrc_t rsrc_id;

		imx8_pd = kzalloc(sizeof(*imx8_pd), GFP_KERNEL);
		if (!imx8_pd)
			return -ENOMEM;

		if (!of_property_read_string(np, "name", &imx8_pd->pd.name))
			imx8_pd->name = imx8_pd->pd.name;

		if (!of_property_read_u32(np, "reg", &rsrc_id))
			imx8_pd->rsrc_id = rsrc_id;

		imx8_pd->pd.power_off = imx8_pd_power_off;
		imx8_pd->pd.power_on = imx8_pd_power_on;

		pm_genpd_init(&imx8_pd->pd, NULL, true);

		if (genpd_parent)
			pm_genpd_add_subdomain(genpd_parent, &imx8_pd->pd);

		of_genpd_add_provider_simple(np, &imx8_pd->pd);

		imx8_add_pm_domains(np, &imx8_pd->pd);
	}
	return 0;
}

static int __init imx8_init_pm_domains(void)
{
	struct device_node *np;
	sc_err_t sciErr;
	sc_rsrc_t rsrc_id;
	uint32_t mu_id;

	pr_info("**** imx8_init_pm_domains\n");

	for_each_compatible_node(np, NULL, "nxp, imx8-pd") {
		struct imx8_pm_domain *imx8_pd;

		imx8_pd = kzalloc(sizeof(struct imx8_pm_domain), GFP_KERNEL);
		if (!imx8_pd) {
			pr_err("%s: failed to allocate memory for domain\n",
				__func__);
			return -ENOMEM;
		}
		if (!of_property_read_string(np, "name", &imx8_pd->pd.name))
			imx8_pd->name = imx8_pd->pd.name;

		if (!of_property_read_u32(np, "reg", &rsrc_id))
			imx8_pd->rsrc_id = rsrc_id;

		if (imx8_pd->rsrc_id != SC_R_LAST) {
			imx8_pd->pd.power_off = imx8_pd_power_off;
			imx8_pd->pd.power_on = imx8_pd_power_on;
		}
		pm_genpd_init(&imx8_pd->pd, NULL, true);
		of_genpd_add_provider_simple(np, &imx8_pd->pd);
		imx8_add_pm_domains(np, &imx8_pd->pd);
	}

#if 0
	/* Assign the child power domains to their parents */
	for_each_compatible_node(np, NULL, "nxp, imx8-pd") {
		struct generic_pm_domain *child_domain, *parent_domain;
		struct of_phandle_args args;
		struct imx8_pm_domain *pd;

		args.np = np;
		args.args_count = 0;
		child_domain = of_genpd_get_from_provider(&args);
		if (IS_ERR(child_domain))
			continue;

		if (of_parse_phandle_with_args(np, "power-domains",
				"#power-domain-cells", 0, &args) != 0)
			continue;

		parent_domain = of_genpd_get_from_provider(&args);
		if (IS_ERR(parent_domain))
			continue;

		if (pm_genpd_add_subdomain(parent_domain, child_domain))
				pr_warn("%s failed to add subdomain: %s\n",
				parent_domain->name, child_domain->name);
		else
			pr_info("%s has as child subdomain: %s.\n",
				parent_domain->name, child_domain->name);

		if (!of_property_read_u32(np, "reg", &rsrc_id)) {
			pd = container_of(child_domain, struct imx8_pm_domain, pd);
			pd->rsrc_id = rsrc_id;
		}
		of_node_put(np);
	}
#endif
	sciErr = sc_ipc_getMuID(&mu_id);
	if (sciErr != SC_ERR_NONE) {
		pr_info("Cannot obtain MU ID\n");
		return sciErr;
	}

	sciErr = sc_ipc_open(&pm_ipcHandle, mu_id);
	/* Inform the clock code that power domains init is complete. */
	notify_imx8_clk();
	return 0;
}

early_initcall(imx8_init_pm_domains);

