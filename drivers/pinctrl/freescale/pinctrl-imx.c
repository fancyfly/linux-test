/*
 * Core driver for the imx pin controller
 *
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 * Copyright (C) 2012 Linaro Ltd.
 *
 * Author: Dong Aisheng <dong.aisheng@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/slab.h>

#include "../core.h"
#include "pinctrl-imx.h"

/* The bits in CONFIG cell defined in binding doc*/
#define IMX_NO_PAD_CTL	0x80000000	/* no pin config need */
#define IMX_PAD_SION 0x40000000		/* set SION */

static const inline struct imx_pin_group *imx_pinctrl_find_group_by_name(
				const struct imx_pinctrl_soc_info *info,
				const char *name)
{
	const struct imx_pin_group *grp = NULL;
	int i;

	for (i = 0; i < info->ngroups; i++) {
		if (!strcmp(info->groups[i].name, name)) {
			grp = &info->groups[i];
			break;
		}
	}

	return grp;
}

static int imx_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;

	return info->ngroups;
}

static const char *imx_get_group_name(struct pinctrl_dev *pctldev,
				       unsigned selector)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;

	return info->groups[selector].name;
}

static int imx_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector,
			       const unsigned **pins,
			       unsigned *npins)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;

	if (selector >= info->ngroups)
		return -EINVAL;

	*pins = info->groups[selector].pin_ids;
	*npins = info->groups[selector].npins;

	return 0;
}

static void imx_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
		   unsigned offset)
{
	seq_printf(s, "%s", dev_name(pctldev->dev));
}

static int imx_dt_node_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map **map, unsigned *num_maps)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	const struct imx_pin_group *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	int map_num = 1;
	int i, j;

	/*
	 * first find the group of this node and check if we need create
	 * config maps for pins
	 */
	grp = imx_pinctrl_find_group_by_name(info, np->name);
	if (!grp) {
		dev_err(info->dev, "unable to find group for node %s\n",
			np->name);
		return -EINVAL;
	}

	if (info->flags & IMX8_USE_SCU) {
		map_num += grp->npins;
	} else {
		for (i = 0; i < grp->npins; i++) {
			if (!(grp->pins[i].pin_conf.pin_memmap.config &
			    IMX_NO_PAD_CTL))
				map_num++;
		}
	}

	new_map = kmalloc(sizeof(struct pinctrl_map) * map_num, GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = map_num;

	/* create mux map */
	parent = of_get_parent(np);
	if (!parent) {
		kfree(new_map);
		return -EINVAL;
	}
	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = parent->name;
	new_map[0].data.mux.group = np->name;
	of_node_put(parent);

	/* create config map */
	new_map++;
	for (i = j = 0; i < grp->npins; i++) {
		if (info->flags & IMX8_USE_SCU) {
			new_map[j].type = PIN_MAP_TYPE_CONFIGS_PIN;
			new_map[j].data.configs.group_or_pin =
					pin_get_name(pctldev, grp->pins[i].pin);
			new_map[j].data.configs.configs =
				(unsigned long *)&grp->pins[i].pin_conf.pin_scu.flags;
			new_map[j].data.configs.num_configs = 6;
			j++;
		
		} else if (!(grp->pins[i].pin_conf.pin_memmap.config & IMX_NO_PAD_CTL)) {
			new_map[j].type = PIN_MAP_TYPE_CONFIGS_PIN;
			new_map[j].data.configs.group_or_pin =
					pin_get_name(pctldev, grp->pins[i].pin);
			new_map[j].data.configs.configs =
				&grp->pins[i].pin_conf.pin_memmap.config;
			new_map[j].data.configs.num_configs = 1;
			j++;
		}
	}

	dev_dbg(pctldev->dev, "maps: function %s group %s num %d\n",
		(*map)->data.mux.function, (*map)->data.mux.group, map_num);

	return 0;
}

static void imx_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	kfree(map);
}

static const struct pinctrl_ops imx_pctrl_ops = {
	.get_groups_count = imx_get_groups_count,
	.get_group_name = imx_get_group_name,
	.get_group_pins = imx_get_group_pins,
	.pin_dbg_show = imx_pin_dbg_show,
	.dt_node_to_map = imx_dt_node_to_map,
	.dt_free_map = imx_dt_free_map,
};

static int imx_pmx_set(struct pinctrl_dev *pctldev, unsigned selector,
		       unsigned group)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	unsigned int npins;
	int i, err;
	struct imx_pin_group *grp;

	/*
	 * Configure the mux mode for each pin in the group for a specific
	 * function.
	 */
	grp = &info->groups[group];
	npins = grp->npins;

	dev_dbg(ipctl->dev, "enable function %s group %s\n",
		info->functions[selector].name, grp->name);

	for (i = 0; i < npins; i++) {
		err = imx_pmx_set_one_pin(ipctl, &grp->pins[i]);
		if (err)
			return err;
	}

	return 0;
}

static int imx_pmx_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;

	return info->nfunctions;
}

static const char *imx_pmx_get_func_name(struct pinctrl_dev *pctldev,
					  unsigned selector)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;

	return info->functions[selector].name;
}

static int imx_pmx_get_groups(struct pinctrl_dev *pctldev, unsigned selector,
			       const char * const **groups,
			       unsigned * const num_groups)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;

	*groups = info->functions[selector].groups;
	*num_groups = info->functions[selector].num_groups;

	return 0;
}

int imx_pmx_gpio_request_enable(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range, unsigned offset)
{
	return imx_pmx_backend_gpio_request_enable(pctldev, range, offset);
}

int imx_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
	   struct pinctrl_gpio_range *range, unsigned offset, bool input)
{
	return imx_pmx_backend_gpio_set_direction(pctldev, range, offset,
						  input);
}

static const struct pinmux_ops imx_pmx_ops = {
	.get_functions_count = imx_pmx_get_funcs_count,
	.get_function_name = imx_pmx_get_func_name,
	.get_function_groups = imx_pmx_get_groups,
	.set_mux = imx_pmx_set,
	.gpio_request_enable = imx_pmx_gpio_request_enable,
	.gpio_set_direction = imx_pmx_gpio_set_direction,
};

static int imx_pinconf_get(struct pinctrl_dev *pctldev,
			     unsigned pin_id, unsigned long *config)
{
	return imx_pinconf_backend_get(pctldev, pin_id, config);
}

static int imx_pinconf_set(struct pinctrl_dev *pctldev,
			     unsigned pin_id, unsigned long *configs,
			     unsigned num_configs)
{
	return imx_pinconf_backend_set(pctldev, pin_id, configs, num_configs);
}

static void imx_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				   struct seq_file *s, unsigned pin_id)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	unsigned long config[6];
	int ret;
	ret = imx_pinconf_backend_get(pctldev, pin_id, config);
	if (ret) {
		seq_printf(s, "N/A");
		return;
	}

	if (info->flags &  IMX8_USE_SCU) {
		seq_printf(s, "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx", config[0],
			   config[1], config[2], config[3], config[4],
			   config[5]);
	} else {
		seq_printf(s, "0x%lx", config[0]);
	}
}

static void imx_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
					 struct seq_file *s, unsigned group)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	struct imx_pin_group *grp;
	unsigned long config[6];
	const char *name;
	int i, ret;

	if (group > info->ngroups)
		return;

	seq_printf(s, "\n");
	grp = &info->groups[group];
	for (i = 0; i < grp->npins; i++) {
		struct imx_pin *pin = &grp->pins[i];
		name = pin_get_name(pctldev, pin->pin);
		ret = imx_pinconf_get(pctldev, pin->pin, config);
		if (ret)
			return;
		if (info->flags &  IMX8_USE_SCU) {
			seq_printf(s, "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx",
				   config[0], config[1], config[2], config[3],
				   config[4], config[5]);
		} else {
			seq_printf(s, "0x%lx", config[0]);
		}
	}
}

static const struct pinconf_ops imx_pinconf_ops = {
	.pin_config_get = imx_pinconf_get,
	.pin_config_set = imx_pinconf_set,
	.pin_config_dbg_show = imx_pinconf_dbg_show,
	.pin_config_group_dbg_show = imx_pinconf_group_dbg_show,
};

static struct pinctrl_desc imx_pinctrl_desc = {
	.pctlops = &imx_pctrl_ops,
	.pmxops = &imx_pmx_ops,
	.confops = &imx_pinconf_ops,
	.owner = THIS_MODULE,
};

/*
 * Each pin represented in fsl,pins consists of 5 u32 PIN_FUNC_ID and
 * 1 u32 CONFIG, so 24 types in total for each pin.
 */
#define FSL_IMX8_PIN_SIZE 28
#define FSL_PIN_SIZE 24
#define SHARE_FSL_PIN_SIZE 20

static int imx_pinctrl_parse_groups(struct device_node *np,
				    struct imx_pin_group *grp,
				    struct imx_pinctrl_soc_info *info,
				    u32 index)
{
	int size, pin_size;
	const __be32 *list, **list_p;
	int i;

	dev_dbg(info->dev, "group(%d): %s\n", index, np->name);

	if (info->flags & IMX8_USE_SCU)
		pin_size = FSL_IMX8_PIN_SIZE;
	else if (info->flags & SHARE_MUX_CONF_REG)
		pin_size = SHARE_FSL_PIN_SIZE;
	else
		pin_size = FSL_PIN_SIZE;
	/* Initialise group */
	grp->name = np->name;

	/*
	 * the binding format is fsl,pins = <PIN_FUNC_ID CONFIG ...>,
	 * do sanity check and calculate pins number
	 */
	list = of_get_property(np, "fsl,pins", &size);
	if (!list) {
		dev_err(info->dev, "no fsl,pins property in node %s\n", np->full_name);
		return -EINVAL;
	}
	list_p = &list;

	/* we do not check return since it's safe node passed down */
	if (!size || size % pin_size) {
		dev_err(info->dev, "Invalid fsl,pins property in node %s\n", np->full_name);
		return -EINVAL;
	}

	grp->npins = size / pin_size;
	grp->pins = devm_kzalloc(info->dev, grp->npins * sizeof(struct imx_pin),
				GFP_KERNEL);
	grp->pin_ids = devm_kzalloc(info->dev, grp->npins * sizeof(unsigned int),
				GFP_KERNEL);
	if (!grp->pins || !grp->pin_ids)
		return -ENOMEM;

	for (i = 0; i < grp->npins; i++) {
		imx_pinctrl_parse_pin(info, &grp->pin_ids[i], &grp->pins[i],
				      list_p);
	}

	return 0;
}

static int imx_pinctrl_parse_functions(struct device_node *np,
				       struct imx_pinctrl_soc_info *info,
				       u32 index)
{
	struct device_node *child;
	struct imx_pmx_func *func;
	struct imx_pin_group *grp;
	static u32 grp_index;
	u32 i = 0;

	dev_dbg(info->dev, "parse function(%d): %s\n", index, np->name);

	func = &info->functions[index];

	/* Initialise function */
	func->name = np->name;
	func->num_groups = of_get_child_count(np);
	if (func->num_groups == 0) {
		dev_err(info->dev, "no groups defined in %s\n", np->full_name);
		return -EINVAL;
	}
	func->groups = devm_kzalloc(info->dev,
			func->num_groups * sizeof(char *), GFP_KERNEL);

	for_each_child_of_node(np, child) {
		func->groups[i] = child->name;
		grp = &info->groups[grp_index++];
		imx_pinctrl_parse_groups(child, grp, info, i++);
	}

	return 0;
}

static int imx_pinctrl_probe_dt(struct platform_device *pdev,
				struct imx_pinctrl_soc_info *info)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	u32 nfuncs = 0;
	u32 i = 0;

	if (!np)
		return -ENODEV;

	nfuncs = of_get_child_count(np);
	if (nfuncs <= 0) {
		dev_err(&pdev->dev, "no functions defined\n");
		return -EINVAL;
	}

	info->nfunctions = nfuncs;
	info->functions = devm_kzalloc(&pdev->dev, nfuncs * sizeof(struct imx_pmx_func),
					GFP_KERNEL);
	if (!info->functions)
		return -ENOMEM;

	info->ngroups = 0;
	for_each_child_of_node(np, child)
		info->ngroups += of_get_child_count(child);
	info->groups = devm_kzalloc(&pdev->dev, info->ngroups * sizeof(struct imx_pin_group),
					GFP_KERNEL);
	if (!info->groups)
		return -ENOMEM;

	for_each_child_of_node(np, child)
		imx_pinctrl_parse_functions(child, info, i++);

	return 0;
}

int imx_pinctrl_probe(struct platform_device *pdev,
		      struct imx_pinctrl_soc_info *info)
{
	struct imx_pinctrl *ipctl;
	struct resource *res;
	int ret, i;

	if (!info || !info->pins || !info->npins) {
		dev_err(&pdev->dev, "wrong pinctrl info\n");
		return -EINVAL;
	}
	info->dev = &pdev->dev;

	/* Create state holders etc for this driver */
	ipctl = devm_kzalloc(&pdev->dev, sizeof(*ipctl), GFP_KERNEL);
	if (!ipctl)
		return -ENOMEM;

	info->pin_regs = devm_kmalloc(&pdev->dev, sizeof(*info->pin_regs) *
				      info->npins, GFP_KERNEL);
	if (!info->pin_regs)
		return -ENOMEM;

	if (!(info->flags & IMX8_USE_SCU)) {
		for (i = 0; i < info->npins; i++) {
			info->pin_regs[i].mux_reg = -1;
			info->pin_regs[i].conf_reg = -1;
		}

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		ipctl->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(ipctl->base))
			return PTR_ERR(ipctl->base);
	}


	imx_pinctrl_desc.name = dev_name(&pdev->dev);
	imx_pinctrl_desc.pins = info->pins;
	imx_pinctrl_desc.npins = info->npins;

	ret = imx_pinctrl_probe_dt(pdev, info);
	if (ret) {
		dev_err(&pdev->dev, "fail to probe dt properties\n");
		return ret;
	}

	ipctl->info = info;
	ipctl->dev = info->dev;
	platform_set_drvdata(pdev, ipctl);
	ipctl->pctl = pinctrl_register(&imx_pinctrl_desc, &pdev->dev, ipctl);
	if (!ipctl->pctl) {
		dev_err(&pdev->dev, "could not register IMX pinctrl driver\n");
		return -EINVAL;
	}

	dev_info(&pdev->dev, "initialized IMX pinctrl driver\n");

	return 0;
}

int imx_pinctrl_remove(struct platform_device *pdev)
{
	struct imx_pinctrl *ipctl = platform_get_drvdata(pdev);

	pinctrl_unregister(ipctl->pctl);

	return 0;
}
