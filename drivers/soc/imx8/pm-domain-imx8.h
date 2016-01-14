/*
 * Copyright (C) 2012 Renesas Solutions Corp.
 *
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef PM_DOMAIN_IMX8_H
#define PM_DOMAIN_IMX8_H

#include <linux/pm_domain.h>
#include <soc/imx8/sc/sci.h>

#define DEFAULT_DEV_LATENCY_NS	250000

struct platform_device;

struct imx8_pm_domain {
	const char * name;
	struct generic_pm_domain pd;
	struct dev_power_governor *gov;
	int (*suspend)(void);
	void (*resume)(void);
	sc_rsrc_t rsrc_id;
};

static inline
struct imx8_pm_domain *to_imx8_pd(struct generic_pm_domain *d)
{
	return container_of(d, struct imx8_pm_domain, pd);
}

struct pm_domain_device {
	const char *domain_name;
	struct platform_device *pdev;
};

#endif /* PM_DOMAIN_IMX8_H */
