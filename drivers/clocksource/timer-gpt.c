/*
 *  linux/arch/arm/plat-mxc/time.c
 *
 *  Copyright (C) 2000-2001 Deep Blue Solutions
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright (C) 2006-2007 Pavel Pisa (ppisa@pikron.com)
 *  Copyright (C) 2008 Juergen Beisert (kernel@pengutronix.de)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/sched_clock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>

#if 0
#include <asm/mach/time.h>
#endif

/*
 * There are 4 versions of the timer hardware on Freescale MXC hardware.
 * Version 0: MX1/MXL
 * Version 1: MX21, MX27.
 * Version 2: MX25, MX31, MX35, MX37, MX51, MX6Q(rev1.0)
 * Version 3: MX6DL, MX6SX, MX6Q(rev1.1+)
 */

/* defines common for all i.MX */
#define MXC_TCTL		0x00
#define MXC_TCTL_TEN		(1 << 0) /* Enable module */
#define MXC_TPRER		0x04

/* MX1, MX21, MX27 */
#define MX1_2_TCTL_CLK_PCLK1	(1 << 1)
#define MX1_2_TCTL_IRQEN	(1 << 4)
#define MX1_2_TCTL_FRR		(1 << 8)
#define MX1_2_TCMP		0x08
#define MX1_2_TCN		0x10
#define MX1_2_TSTAT		0x14

/* MX21, MX27 */
#define MX2_TSTAT_CAPT		(1 << 1)
#define MX2_TSTAT_COMP		(1 << 0)

/* MX31, MX35, MX25, MX5, MX6 */
#define V2_TCTL_WAITEN		(1 << 3) /* Wait enable mode */
#define V2_TCTL_CLK_IPG		(1 << 6)
#define V2_TCTL_CLK_PER		(2 << 6)
#define V2_TCTL_CLK_OSC_DIV8	(5 << 6)
#define V2_TCTL_FRR		(1 << 9)
#define V2_TCTL_24MEN		(1 << 10)
#define V2_TPRER_PRE24M		12
#define V2_IR			0x0c
#define V2_TSTAT		0x08
#define V2_TSTAT_OF1		(1 << 0)
#define V2_TCN			0x24
#define V2_TCMP			0x10

#define V2_TIMER_RATE_OSC_DIV8	3000000


#define IMX_TIMER_V0         (0)
#define IMX_TIMER_V1         (1)
#define IMX_TIMER_V2         (2)
#define IMX_TIMER_V3         (3)

struct imx_timer {
	void __iomem *timer_base;
	int version;
	struct clock_event_device evt;
	struct irqaction act;
	void (*gpt_irq_enable)(struct imx_timer *);
	void (*gpt_irq_disable)(struct imx_timer *);
	void (*gpt_irq_acknowledge)(struct imx_timer *);
};

static void gpt_irq_disable_v0_v1(struct imx_timer *tm)
{
	unsigned int tmp;

	tmp = __raw_readl(tm->timer_base + MXC_TCTL);
	__raw_writel(tmp & ~MX1_2_TCTL_IRQEN, tm->timer_base + MXC_TCTL);

}

static void gpt_irq_disable_v2_v3(struct imx_timer *tm)
{
	__raw_writel(0, tm->timer_base + V2_IR);

}

static void gpt_irq_enable_v0_v1(struct imx_timer *tm)
{
	__raw_writel(__raw_readl(tm->timer_base + MXC_TCTL) | MX1_2_TCTL_IRQEN,
			tm->timer_base + MXC_TCTL);
}

static void gpt_irq_enable_v2_v3(struct imx_timer *tm)
{
	__raw_writel(1, tm->timer_base + V2_IR);
}

static void gpt_irq_acknowledge_v0(struct imx_timer *tm)
{
	__raw_readl(tm->timer_base + MX1_2_TSTAT);

	__raw_writel(0, tm->timer_base + MX1_2_TSTAT);
}

static void gpt_irq_acknowledge_v1(struct imx_timer *tm)
{
	__raw_readl(tm->timer_base + MX1_2_TSTAT);

	__raw_writel(MX2_TSTAT_CAPT | MX2_TSTAT_COMP,
			tm->timer_base + MX1_2_TSTAT);
}

static void gpt_irq_acknowledge_v2_v3(struct imx_timer *tm)
{
	__raw_readl(tm->timer_base + V2_TSTAT);

	__raw_writel(V2_TSTAT_OF1, tm->timer_base + V2_TSTAT);
}

static void __iomem *sched_clock_reg;

static u64 notrace mxc_read_sched_clock(void)
{
	return sched_clock_reg ? __raw_readl(sched_clock_reg) : 0;
}

static struct delay_timer imx_delay_timer;

static unsigned long imx_read_current_timer(void)
{
	return __raw_readl(sched_clock_reg);
}

static int __init mxc_clocksource_init(struct clk *timer_clk,
				void __iomem *addr)
{
	unsigned int c = clk_get_rate(timer_clk);

	BUG_ON(!addr);

#if 0
	imx_delay_timer.read_current_timer = &imx_read_current_timer;
	imx_delay_timer.freq = c;
	register_current_timer_delay(&imx_delay_timer);
#endif
	sched_clock_reg = addr;

	sched_clock_register(mxc_read_sched_clock, 32, c);
	return clocksource_mmio_init(addr, "mxc_timer1", c, 200, 32,
			clocksource_mmio_readl_up);
}

/* clock event */

static int mx1_2_set_next_event(unsigned long evt,
			      struct clock_event_device *evt_dev)
{
	unsigned long tcmp;
	struct imx_timer *tm = container_of(evt_dev, struct imx_timer, evt);

	tcmp = __raw_readl(tm->timer_base + MX1_2_TCN) + evt;

	__raw_writel(tcmp, tm->timer_base + MX1_2_TCMP);

	return (int)(tcmp - __raw_readl(tm->timer_base + MX1_2_TCN)) < 0 ?
				-ETIME : 0;
}

static int v2_set_next_event(unsigned long evt,
			      struct clock_event_device *evt_dev)
{
	unsigned long tcmp;
	struct imx_timer *tm = container_of(evt_dev, struct imx_timer, evt);

	tcmp = __raw_readl(tm->timer_base + V2_TCN) + evt;

	__raw_writel(tcmp, tm->timer_base + V2_TCMP);

	return evt < 0x7fffffff &&
		(int)(tcmp - __raw_readl(tm->timer_base + V2_TCN)) < 0 ?
				-ETIME : 0;
}

#ifdef DEBUG
static const char *clock_event_mode_label[] = {
	[CLOCK_EVT_MODE_PERIODIC] = "CLOCK_EVT_MODE_PERIODIC",
	[CLOCK_EVT_MODE_ONESHOT]  = "CLOCK_EVT_MODE_ONESHOT",
	[CLOCK_EVT_MODE_SHUTDOWN] = "CLOCK_EVT_MODE_SHUTDOWN",
	[CLOCK_EVT_MODE_UNUSED]   = "CLOCK_EVT_MODE_UNUSED",
	[CLOCK_EVT_MODE_RESUME]   = "CLOCK_EVT_MODE_RESUME",
};
#endif /* DEBUG */

static void mxc_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	unsigned long flags;
	struct imx_timer *tm = container_of(evt, struct imx_timer, evt);
	static enum clock_event_mode clockevent_mode = CLOCK_EVT_MODE_UNUSED;

	/*
	 * The timer interrupt generation is disabled at least
	 * for enough time to call mxc_set_next_event()
	 */
	local_irq_save(flags);

	/* Disable interrupt in GPT module */
	tm->gpt_irq_disable(tm);

	if (mode != clockevent_mode) {
		/* Set event time into far-far future */
		evt->set_next_event(-3, evt);

		/* Clear pending interrupt */
		tm->gpt_irq_acknowledge(tm);
	}

#ifdef DEBUG
	printk(KERN_INFO "mxc_set_mode: changing mode from %s to %s\n",
		clock_event_mode_label[clockevent_mode],
		clock_event_mode_label[mode]);
#endif /* DEBUG */

	/* Remember timer mode */
	clockevent_mode = mode;
	local_irq_restore(flags);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		printk(KERN_ERR"mxc_set_mode: Periodic mode is not "
				"supported for i.MX\n");
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	/*
	 * Do not put overhead of interrupt enable/disable into
	 * mxc_set_next_event(), the core has about 4 minutes
	 * to call mxc_set_next_event() or shutdown clock after
	 * mode switching
	 */
		local_irq_save(flags);
		tm->gpt_irq_enable(tm);
		local_irq_restore(flags);
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_RESUME:
		/* Left event sources disabled, no more interrupts appear */
		break;
	}
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t mxc_timer_interrupt(int irq, void *dev_id)
{
	struct imx_timer *tm = dev_id;
	void (*event_handler)(struct clock_event_device *);

	BUG_ON(!tm);

	tm->gpt_irq_acknowledge(tm);

	event_handler = ACCESS_ONCE(tm->evt.event_handler);
	if (event_handler)
		event_handler(&tm->evt);

	return IRQ_HANDLED;
}

static int __init mxc_clockevent_init(struct clk *timer_clk,
		struct imx_timer *tm)
{
	tm->evt.cpumask = cpumask_of(0);
	clockevents_config_and_register(&tm->evt,
					clk_get_rate(timer_clk),
					0xff, 0xfffffffe);

	return 0;
}

static void __init _mxc_timer_init_v0_v1(int irq, struct clk *clk_per,
		struct clk *clk_ipg, struct imx_timer *tm)
{
	uint32_t tctl_val;

	if (IS_ERR(clk_per)) {
		pr_err("i.MX timer: unable to get clk\n");
		return;
	}

	if (!IS_ERR(clk_ipg))
		clk_prepare_enable(clk_ipg);

	clk_prepare_enable(clk_per);

	/*
	 * Initialise to a known state (all timers off, and timing reset)
	 */

	__raw_writel(0, tm->timer_base + MXC_TCTL);
	__raw_writel(0, tm->timer_base + MXC_TPRER); /* see datasheet note */

	tctl_val = MX1_2_TCTL_FRR | MX1_2_TCTL_CLK_PCLK1 | MXC_TCTL_TEN;

	__raw_writel(tctl_val, tm->timer_base + MXC_TCTL);

	/* init and register the timer to the framework */
	mxc_clocksource_init(clk_per, tm->timer_base + MX1_2_TCN);

	tm->evt.set_next_event = mx1_2_set_next_event;
	mxc_clockevent_init(clk_per, tm);

	/* Make irqs happen */
	setup_irq(irq, &tm->act);
}

static void __init _mxc_timer_init_v2(int irq, struct clk *clk_per,
		struct clk *clk_ipg, struct imx_timer *tm)
{
	uint32_t tctl_val;

	if (IS_ERR(clk_per)) {
		pr_err("i.MX timer: unable to get clk\n");
		return;
	}

	if (!IS_ERR(clk_ipg))
		clk_prepare_enable(clk_ipg);

	clk_prepare_enable(clk_per);

	/*
	 * Initialise to a known state (all timers off, and timing reset)
	 */

	__raw_writel(0, tm->timer_base + MXC_TCTL);
	__raw_writel(0, tm->timer_base + MXC_TPRER); /* see datasheet note */

	tctl_val = V2_TCTL_FRR | V2_TCTL_WAITEN | MXC_TCTL_TEN;

	if (clk_get_rate(clk_per) == V2_TIMER_RATE_OSC_DIV8)
		tctl_val |= V2_TCTL_CLK_OSC_DIV8;
	else
		tctl_val |= V2_TCTL_CLK_PER;

	__raw_writel(tctl_val, tm->timer_base + MXC_TCTL);

	/* init and register the timer to the framework */
	mxc_clocksource_init(clk_per, tm->timer_base + V2_TCN);

	tm->evt.set_next_event = v2_set_next_event;
	mxc_clockevent_init(clk_per, tm);

	/* Make irqs happen */
	setup_irq(irq, &tm->act);
}

static void __init _mxc_timer_init_v3(int irq, struct clk *clk_per,
		struct clk *clk_ipg, struct imx_timer *tm)
{
	uint32_t tctl_val;

	if (IS_ERR(clk_per)) {
		pr_err("i.MX timer: unable to get clk\n");
		return;
	}

	if (!IS_ERR(clk_ipg))
		clk_prepare_enable(clk_ipg);

	clk_prepare_enable(clk_per);

	/*
	 * Initialise to a known state (all timers off, and timing reset)
	 */

	__raw_writel(0, tm->timer_base + MXC_TCTL);
	__raw_writel(0, tm->timer_base + MXC_TPRER); /* see datasheet note */

	tctl_val = V2_TCTL_FRR | V2_TCTL_WAITEN | MXC_TCTL_TEN;

	if (clk_get_rate(clk_per) == V2_TIMER_RATE_OSC_DIV8) {
		tctl_val |= V2_TCTL_CLK_OSC_DIV8;
		/* 24 / 8 = 3 MHz */
		__raw_writel(7 << V2_TPRER_PRE24M,
					tm->timer_base + MXC_TPRER);
		tctl_val |= V2_TCTL_24MEN;
	} else {
		tctl_val |= V2_TCTL_CLK_PER;
	}

	__raw_writel(tctl_val, tm->timer_base + MXC_TCTL);

	/* init and register the timer to the framework */
	mxc_clocksource_init(clk_per, tm->timer_base + V2_TCN);

	tm->evt.set_next_event = v2_set_next_event;
	mxc_clockevent_init(clk_per, tm);

	/* Make irqs happen */
	setup_irq(irq, &tm->act);
}

static void __init _mxc_timer_init(int irq, struct clk *clk_per,
		struct clk *clk_ipg, struct imx_timer *tm)
{

	switch (tm->version) {
	case IMX_TIMER_V0:
		tm->gpt_irq_enable = gpt_irq_enable_v0_v1;
		tm->gpt_irq_disable = gpt_irq_disable_v0_v1;
		tm->gpt_irq_acknowledge = gpt_irq_acknowledge_v0;
		_mxc_timer_init_v0_v1(irq, clk_per, clk_ipg, tm);
		break;

	case IMX_TIMER_V1:
		tm->gpt_irq_enable = gpt_irq_enable_v0_v1;
		tm->gpt_irq_disable = gpt_irq_disable_v0_v1;
		tm->gpt_irq_acknowledge = gpt_irq_acknowledge_v1;
		_mxc_timer_init_v0_v1(irq, clk_per, clk_ipg, tm);
		break;

	case IMX_TIMER_V2:
		tm->gpt_irq_enable = gpt_irq_enable_v2_v3;
		tm->gpt_irq_disable = gpt_irq_disable_v2_v3;
		tm->gpt_irq_acknowledge = gpt_irq_acknowledge_v2_v3;
		_mxc_timer_init_v2(irq, clk_per, clk_ipg, tm);
		break;

	case IMX_TIMER_V3:
		tm->gpt_irq_enable = gpt_irq_enable_v2_v3;
		tm->gpt_irq_disable = gpt_irq_disable_v2_v3;
		tm->gpt_irq_acknowledge = gpt_irq_acknowledge_v2_v3;
		_mxc_timer_init_v3(irq, clk_per, clk_ipg, tm);
		break;

	default:
		pr_err("<%s> timer device node is not supported\r\n", __func__);
		break;

	}
}

void __init mxc_timer_init(unsigned long pbase, int irq, int ver)
{
	struct imx_timer *timer;
	struct clk *clk_per = clk_get_sys("imx-gpt.0", "per");
	struct clk *clk_ipg = clk_get_sys("imx-gpt.0", "ipg");

	timer = kzalloc(sizeof(struct imx_timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");

	timer->timer_base = (void __iomem *)pbase;
	timer->version = ver;
	timer->evt.name = "mxc_timer1";
	timer->evt.rating = 99;
	timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.set_mode = mxc_set_mode;
	timer->evt.set_next_event = v2_set_next_event;
	timer->act.name = "i.MX Timer Tick";
	timer->act.flags = IRQF_TIMER | IRQF_IRQPOLL;
	timer->act.dev_id = timer;
	timer->act.handler = mxc_timer_interrupt;

	_mxc_timer_init(irq, clk_per, clk_ipg, timer);
}

struct imx_timer_ip_combo {
	const char      *compat;
	int             version;
};

static const struct imx_timer_ip_combo imx_timer_tables[] = {
	{"fsl,imx1-gpt",        IMX_TIMER_V0},
	{"fsl,imx25-gpt",       IMX_TIMER_V2},
	{"fsl,imx25-gpt",       IMX_TIMER_V2},
	{"fsl,imx50-gpt",       IMX_TIMER_V2},
	{"fsl,imx51-gpt",       IMX_TIMER_V2},
	{"fsl,imx53-gpt",       IMX_TIMER_V2},
	{"fsl,imx6q-gpt",       IMX_TIMER_V2},
	{"fsl,imx6sl-gpt",      IMX_TIMER_V3},
	{"fsl,imx6sx-gpt",      IMX_TIMER_V3},
	{"fsl,imx8dv-gpt",      IMX_TIMER_V3},
};

static void __init mxc_timer_init_dt(struct device_node *np)
{
	struct clk *clk_per, *clk_ipg;
	int irq, i, ret, ver;
	struct imx_timer *timer;

	if (sched_clock_reg)
		return;

	for (i =  0; i < sizeof(imx_timer_tables) /
			sizeof(struct imx_timer_ip_combo); i++) {
		ret = of_device_is_compatible(np, imx_timer_tables[i].compat);
		if (ret) {
			ver = imx_timer_tables[i].version;
			pr_err("<%s> compatible=%s timer_version=%d\r\n",
				__func__, imx_timer_tables[i].compat, ver);
			break;
		}
	}

	if (!ret) {
		pr_err("<%s> timer device node is not supported\r\n", __func__);
		return;
	}

	timer = kzalloc(sizeof(struct imx_timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");

	timer->timer_base = of_iomap(np, 0);
	WARN_ON(!timer->timer_base || !ret);

	irq = irq_of_parse_and_map(np, 0);

	clk_ipg = of_clk_get_by_name(np, "ipg");

	/* Try osc_per first, and fall back to per otherwise */
	clk_per = of_clk_get_by_name(np, "osc_per");
	if (IS_ERR(clk_per))
		clk_per = of_clk_get_by_name(np, "per");

	timer->version = ver;
	timer->evt.name = np->name;
	timer->evt.rating = 99;
	timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.set_mode = mxc_set_mode;
	timer->evt.set_next_event = v2_set_next_event;
	timer->act.name = np->name;
	timer->act.flags = IRQF_TIMER | IRQF_IRQPOLL;
	timer->act.dev_id = timer;
	timer->act.handler = mxc_timer_interrupt;

	_mxc_timer_init(irq, clk_per, clk_ipg, timer);
}

CLOCKSOURCE_OF_DECLARE(mx1_timer, "fsl,imx1-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx25_timer, "fsl,imx25-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx50_timer, "fsl,imx50-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx51_timer, "fsl,imx51-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx53_timer, "fsl,imx53-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx6q_timer, "fsl,imx6q-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx6sl_timer, "fsl,imx6sl-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx6sx_timer, "fsl,imx6sx-gpt", mxc_timer_init_dt);
CLOCKSOURCE_OF_DECLARE(mx8dv_timer, "fsl,imx8dv-gpt", mxc_timer_init_dt);
