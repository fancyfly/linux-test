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
#include <soc/imx8/sc/sci.h>

#include "../core.h"
#include "pinctrl-imx.h"

extern sc_ipc_t pinctrl_ipcHandle;

int imx_pmx_set_one_pin(struct imx_pinctrl *ipctl, struct imx_pin *pin)
{
	unsigned int pin_flags;
	sc_err_t err = SC_ERR_NONE;
	sc_ipc_t ipc = pinctrl_ipcHandle;
	unsigned int pin_id = pin->pin;
	struct imx_pin_scu *pin_scu = &pin->pin_conf.pin_scu;

	if (ipc == -1) {
		printk("IPC handle not initialized!\n");
		return -EIO;
	}

	pin_flags = pin_scu->flags;

	if (pin_flags & IMX_SCU_PIN_MUX_SET_MASK) {
		err = sc_pad_set_mux(ipc, pin_id, pin_scu->mux,
				     pin_scu->config, pin_scu->iso);
	}

	if (err != SC_ERR_NONE)
		return -EIO;

	return 0;
}

int imx_pmx_backend_gpio_request_enable(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range, unsigned offset)
{
	return -EINVAL;
}

int imx_pmx_backend_gpio_set_direction(struct pinctrl_dev *pctldev,
	   struct pinctrl_gpio_range *range, unsigned offset, bool input)
{
	return -EINVAL;
}

int imx_pinconf_backend_get(struct pinctrl_dev *pctldev, unsigned pin_id,
			    unsigned long *config)
{
	/* TODO */

	return -EINVAL;
}
int imx_pinconf_backend_set(struct pinctrl_dev *pctldev, unsigned pin_id,
			    unsigned long *configs, unsigned num_configs)
{
	struct imx_pinctrl *ipctl = pinctrl_dev_get_drvdata(pctldev);
	const struct imx_pinctrl_soc_info *info = ipctl->info;
	unsigned int *conf = (unsigned int *)configs;
	unsigned int flags, wakeup, gp;
	sc_err_t err = SC_ERR_NONE;
	sc_ipc_t ipc = pinctrl_ipcHandle;

	if (ipc == -1) {
		printk("IPC handle not initialized!\n");
		return -EIO;
	}

	dev_dbg(ipctl->dev, "pinconf set pin %s\n", info->pins[pin_id].name);

	flags = conf[0];
	wakeup = conf[4];
	gp = conf[5];

	if (flags & IMX_SCU_PIN_WAKEUP_SET_MASK) {
		err = sc_pad_set_wakeup(ipc, pin_id, wakeup); 
		if (err != SC_ERR_NONE)
			return -EIO;
	}

	if (flags & IMX_SCU_PIN_GP_SET_MASK) {
		err = sc_pad_set_gp(ipc, pin_id, gp);
		if (err != SC_ERR_NONE)
			return -EIO;
	}

	return 0;
}

int imx_pinctrl_parse_pin(struct imx_pinctrl_soc_info *info,
			  unsigned int *pin_id, struct imx_pin *pin,
			  const __be32 **list_p)
{
	const __be32 *list = *list_p;
	struct imx_pin_scu *pin_scu = &pin->pin_conf.pin_scu;

	pin->pin = be32_to_cpu(*list++);
	*pin_id = pin->pin;
	pin_scu->flags = be32_to_cpu(*list++);
	pin_scu->mux = be32_to_cpu(*list++);
	pin_scu->config = be32_to_cpu(*list++);
	pin_scu->iso = be32_to_cpu(*list++);
	pin_scu->wakeup = be32_to_cpu(*list++);
	pin_scu->gp = be32_to_cpu(*list++);

	*list_p = list;

	dev_dbg(info->dev, "%s: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		info->pins[pin->pin].name, pin->pin, pin_scu->flags,
		pin_scu->mux, pin_scu->config, pin_scu->iso, pin_scu->wakeup,
		pin_scu->gp);

	return 0;
}
