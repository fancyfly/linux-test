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

int imx_pmx_set_one_pin(struct imx_pinctrl *ipctl, struct imx_pin *pin)
{
	unsigned int pin_id = pin->pin_conf.pin_memmap.pin;
	struct imx_pin_reg *pin_reg;
	pin_reg = &info->pin_regs[pin_id];

	if (pin_reg->mux_reg == -1) {
		dev_err(ipctl->dev, "Pin(%s) does not support mux function\n",
			info->pins[pin_id].name);
		return -EINVAL;
	}

	if (info->flags & SHARE_MUX_CONF_REG) {
		u32 reg;
		reg = readl(ipctl->base + pin_reg->mux_reg);
		reg &= ~(0x7 << 20);
		reg |= (pin->mux_mode << 20);
		writel(reg, ipctl->base + pin_reg->mux_reg);
	} else {
		writel(pin->mux_mode, ipctl->base + pin_reg->mux_reg);
	}
	dev_dbg(ipctl->dev, "write: offset 0x%x val 0x%x\n",
		pin_reg->mux_reg, pin->mux_mode);

	/*
	 * If the select input value begins with 0xff, it's a quirky
	 * select input and the value should be interpreted as below.
	 *     31     23      15      7        0
	 *     | 0xff | shift | width | select |
	 * It's used to work around the problem that the select
	 * input for some pin is not implemented in the select
	 * input register but in some general purpose register.
	 * We encode the select input value, width and shift of
	 * the bit field into input_val cell of pin function ID
	 * in device tree, and then decode them here for setting
	 * up the select input bits in general purpose register.
	 */
	if (pin->input_val >> 24 == 0xff) {
		u32 val = pin->input_val;
		u8 select = val & 0xff;
		u8 width = (val >> 8) & 0xff;
		u8 shift = (val >> 16) & 0xff;
		u32 mask = ((1 << width) - 1) << shift;
		/*
		 * The input_reg[i] here is actually some IOMUXC general
		 * purpose register, not regular select input register.
		 */
		val = readl(ipctl->base + pin->input_reg);
		val &= ~mask;
		val |= select << shift;
		writel(val, ipctl->base + pin->input_reg);
	} else if (pin->input_reg) {
		/*
		 * Regular select input register can never be at offset
		 * 0, and we only print register value for regular case.
		 */
		writel(pin->input_val, ipctl->base + pin->input_reg);
		dev_dbg(ipctl->dev,
			"==>select_input: offset 0x%x val 0x%x\n",
			pin->input_reg, pin->input_val);
	}

	return 0;
}

int imx_pmx_backend_gpio_request_enable(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range, unsigned offset)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	const struct imx_pin_reg *pin_reg;
	struct imx_pin_group *grp;
	struct imx_pin *imx_pin;
	unsigned int pin, group;
	u32 reg;

	/* Currently implementation only for shared mux/conf register */
	if (!(info->flags & SHARE_MUX_CONF_REG))
		return -EINVAL;

	pin_reg = &info->pin_regs[offset];
	if (pin_reg->mux_reg == -1)
		return -EINVAL;

	/* Find the pinctrl config with GPIO mux mode for the requested pin */
	for (group = 0; group < info->ngroups; group++) {
		grp = &info->groups[group];
		for (pin = 0; pin < grp->npins; pin++) {
			imx_pin = &grp->pins[pin];
			if (imx_pin->pin == offset && !imx_pin->mux_mode)
				goto mux_pin;
		}
	}

	return -EINVAL;

mux_pin:
	reg = readl(ipctl->base + pin_reg->mux_reg);
	reg &= ~(0x7 << 20);
	reg |= imx_pin->config;
	writel(reg, ipctl->base + pin_reg->mux_reg);

	return 0;
}

int imx_pmx_backend_gpio_set_direction(struct pinctrl_dev *pctldev,
	   struct pinctrl_gpio_range *range, unsigned offset, bool input)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	const struct imx_pin_reg *pin_reg;
	u32 reg;

	/*
	 * Only Vybrid has the input/output buffer enable flags (IBE/OBE)
	 * They are part of the shared mux/conf register.
	 */
	if (!(info->flags & SHARE_MUX_CONF_REG))
		return -EINVAL;

	pin_reg = &info->pin_regs[offset];
	if (pin_reg->mux_reg == -1)
		return -EINVAL;

	/* IBE always enabled allows us to read the value "on the wire" */
	reg = readl(ipctl->base + pin_reg->mux_reg);
	if (input)
		reg &= ~0x2;
	else
		reg |= 0x2;
	writel(reg, ipctl->base + pin_reg->mux_reg);

	return 0;
}

static int imx_pinconf_backend_get(struct pinctrl_dev *pctldev,
			     unsigned pin_id, unsigned long *config)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	const struct imx_pin_reg *pin_reg = &info->pin_regs[pin_id];

	if (!pin_reg || pin_reg->conf_reg == -1) {
		dev_err(info->dev, "Pin(%s) does not support config function\n",
			info->pins[pin_id].name);
		return -EINVAL;
	}

	*config = readl(ipctl->base + pin_reg->conf_reg);

	if (info->flags & SHARE_MUX_CONF_REG)
		*config &= 0xffff;

	return 0;
}
static int imx_pinconf_backend_set(struct pinctrl_dev *pctldev,
				   unsigned pin_id, unsigned long *configs,
				   unsigned num_configs)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	const struct imx_pin_reg *pin_reg = &info->pin_regs[pin_id];
	int i;

	if (pin_reg->conf_reg == -1) {
		dev_err(info->dev, "Pin(%s) does not support config function\n",
			info->pins[pin_id].name);
		return -EINVAL;
	}

	dev_dbg(ipctl->dev, "pinconf set pin %s\n",
		info->pins[pin_id].name);

	for (i = 0; i < num_configs; i++) {
		if (info->flags & SHARE_MUX_CONF_REG) {
			u32 reg;
			reg = readl(ipctl->base + pin_reg->conf_reg);
			reg &= ~0xffff;
			reg |= configs[i];
			writel(reg, ipctl->base + pin_reg->conf_reg);
		} else {
			writel(configs[i], ipctl->base + pin_reg->conf_reg);
		}
		dev_dbg(ipctl->dev, "write: offset 0x%x val 0x%lx\n",
			pin_reg->conf_reg, configs[i]);
	} /* for each config */

	return 0;
}

static int imx_pinctrl_parse_pin(struct imx_pinctrl_soc_info *info,
				 struct imx_pin *pin, const __be32 **list_p)
{
	const __be32 *list = *list_p;
	u32 mux_reg = be32_to_cpu(*list++);
	u32 conf_reg;
	unsigned int pin_id;
	struct imx_pin_reg *pin_reg;
	struct imx_pin *pin = &grp->pins[i];

	if (info->flags & SHARE_MUX_CONF_REG) {
		conf_reg = mux_reg;
	} else {
		conf_reg = be32_to_cpu(*list++);
		if (!conf_reg)
			conf_reg = -1;
	}

	pin_id = mux_reg ? mux_reg / 4 : conf_reg / 4;
	pin_reg = &info->pin_regs[pin_id];
	pin->pin = pin_id;
	grp->pin_ids[i] = pin_id;
	pin_reg->mux_reg = mux_reg;
	pin_reg->conf_reg = conf_reg;
	pin->input_reg = be32_to_cpu(*list++);
	pin->mux_mode = be32_to_cpu(*list++);
	pin->input_val = be32_to_cpu(*list++);

	/* SION bit is in mux register */
	config = be32_to_cpu(*list++);
	if (config & IMX_PAD_SION)
		pin->mux_mode |= IOMUXC_CONFIG_SION;
	pin->config = config & ~IMX_PAD_SION;

	dev_dbg(info->dev, "%s: 0x%x 0x%08lx", info->pins[pin_id].name,
			pin->mux_mode, pin->config);
}
