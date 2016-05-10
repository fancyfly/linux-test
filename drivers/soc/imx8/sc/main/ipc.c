/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* Includes */
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <soc/imx8/sc/svc/irq/api.h>
#include <soc/imx8/sc/ipc.h>
#include <soc/imx8/sc/sci.h>
#include "../../mu/fsl_mu_hal.h"
#include "rpc.h"

/* Local Defines */
#define MU_SIZE 0x10000

/* Local Types */
unsigned int scu_mu_id;
static void __iomem *mu_base_virtaddr;
static struct delayed_work scu_mu_work;
static sc_ipc_t mu_ipcHandle;

/* Local functions */

/* Local variables */
static uint32_t gIPCport;
static bool scu_mu_init;

DEFINE_MUTEX(scu_mu_mutex);

static BLOCKING_NOTIFIER_HEAD(SCU_notifier_chain);

EXPORT_SYMBOL(sc_pm_set_resource_power_mode);
EXPORT_SYMBOL(sc_pm_get_resource_power_mode);
EXPORT_SYMBOL(sc_pm_cpu_start);
EXPORT_SYMBOL(sc_ipc_getMuID);
EXPORT_SYMBOL(sc_ipc_open);
EXPORT_SYMBOL(sc_ipc_close);
EXPORT_SYMBOL(sc_call_rpc);
EXPORT_SYMBOL(sc_misc_set_control);
EXPORT_SYMBOL(sc_pm_clock_enable);
EXPORT_SYMBOL(sc_pm_set_clock_rate);
EXPORT_SYMBOL(sc_pad_set_gp_28lpp);
/*--------------------------------------------------------------------------*/
/* RPC command/response                                                     */
/*--------------------------------------------------------------------------*/
void sc_call_rpc(sc_ipc_t handle, sc_rpc_msg_t *msg, bool no_resp)
{
	if (in_interrupt()){
		pr_warn("Cannot make SC IPC calls from an interrupt context\n");
		dump_stack();
		return;
	}
	mutex_lock(&scu_mu_mutex);

	sc_ipc_write(handle, msg);
	if (!no_resp)
		sc_ipc_read(handle, msg);

	mutex_unlock(&scu_mu_mutex);
}

/*--------------------------------------------------------------------------*/
/* Get MU base address for specified IPC channel                            */
/*--------------------------------------------------------------------------*/
static MU_Type *sc_ipc_get_mu_base(uint32_t id)
{
	MU_Type *base;

	/* Check parameters */
	if (id >= SC_NUM_IPC)
		base = NULL;
	else
		base = (MU_Type*)(mu_base_virtaddr + (id * MU_SIZE));

	return base;
}

/*--------------------------------------------------------------------------*/
/* Get the MU ID used by Linux                                              */
/*--------------------------------------------------------------------------*/
int sc_ipc_getMuID(uint32_t *mu_id)
{
	if (scu_mu_init) {
		*mu_id = scu_mu_id;
		return SC_ERR_NONE;
	}
	return SC_ERR_UNAVAILABLE;
}

/*--------------------------------------------------------------------------*/
/* Open an IPC channel                                                      */
/*--------------------------------------------------------------------------*/
sc_err_t sc_ipc_requestInt(sc_ipc_t *handle, uint32_t id)
{
	return SC_ERR_NONE;
}

/*--------------------------------------------------------------------------*/
/* Open an IPC channel                                                      */
/*--------------------------------------------------------------------------*/
sc_err_t sc_ipc_open(sc_ipc_t *handle, uint32_t id)
{
	MU_Type *base;

	mutex_lock(&scu_mu_mutex);

	if (!scu_mu_init) {
		mutex_unlock(&scu_mu_mutex);
		return SC_ERR_UNAVAILABLE;
	}
	/* Get MU base associated with IPC channel */
	base = sc_ipc_get_mu_base(id);

	if (base == NULL) {
		mutex_unlock(&scu_mu_mutex);
		return SC_ERR_IPC;
	}

	*handle = (sc_ipc_t)task_pid_vnr(current);

	mutex_unlock(&scu_mu_mutex);

	return SC_ERR_NONE;
}

/*--------------------------------------------------------------------------*/
/* Close an IPC channel                                                     */
/*--------------------------------------------------------------------------*/
void sc_ipc_close(sc_ipc_t handle)
{
	MU_Type *base;

	mutex_lock(&scu_mu_mutex);

	if (!scu_mu_init) {
		mutex_unlock(&scu_mu_mutex);
		return;
	}

	/* Get MU base associated with IPC channel */
	base = sc_ipc_get_mu_base(gIPCport);

	/* TBD ***** What needs to be done here? */
	mutex_unlock(&scu_mu_mutex);
}

/*!
 * This function reads a message from an IPC channel.
 *
 * @param[in]     ipc         id of channel read from
 * @param[out]    data        pointer to message buffer to read
 *
 * This function will block if no message is available to be read.
 */
void sc_ipc_read(sc_ipc_t handle, void *data)
{
	MU_Type *base;
	uint8_t count = 0;
	sc_rpc_msg_t *msg = (sc_rpc_msg_t *)data;

	/* Get MU base associated with IPC channel */
	base = sc_ipc_get_mu_base(gIPCport);

	if ((base == NULL) || (msg == NULL))
		return;


	/* Read first word */
	MU_HAL_ReceiveMsg(base, 0, (uint32_t*) msg);
	count++;

	/* Check size */
	if (msg->size > SC_RPC_MAX_MSG) {
		*((uint32_t*) msg) = 0;
		return;
	}

	/* Read remaining words */
	while (count < msg->size) {
		MU_HAL_ReceiveMsg(base, count % MU_RR_COUNT,
			&(msg->DATA.d32[count - 1]));   
		count++;
	}
}

/*!
 * This function writes a message to an IPC channel.
 *
 * @param[in]     ipc         id of channel to write to
 * @param[in]     data        pointer to message buffer to write
 *
 * This function will block if the outgoing buffer is full.
 */
void sc_ipc_write(sc_ipc_t handle, void *data)
{
	MU_Type *base;
	uint8_t count = 0;
	sc_rpc_msg_t *msg = (sc_rpc_msg_t *)data;

	/* Get MU base associated with IPC channel */
	base = sc_ipc_get_mu_base(gIPCport);

	if ((base == NULL) || (msg == NULL))
		return;

	/* Check size */
	if (msg->size > SC_RPC_MAX_MSG)
		return;

	/* Write first word */
	MU_HAL_SendMsg(base, 0, *((uint32_t*) msg));
	count++;

	/* Write remaining words */
	while (count < msg->size)
	{
		MU_HAL_SendMsg(base, count % MU_TR_COUNT,
			msg->DATA.d32[count - 1]);   
		count++;
	}
}

int register_scu_notifier(struct notifier_block *nb)
{
		return blocking_notifier_chain_register(
		&SCU_notifier_chain, nb);
}
EXPORT_SYMBOL(register_scu_notifier);

int unregister_scu_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(
	&SCU_notifier_chain, nb);
}
EXPORT_SYMBOL(unregister_scu_notifier);

static int SCU_notifier_call_chain(unsigned long val)
{
	return (blocking_notifier_call_chain(
		&SCU_notifier_chain, val, NULL)
		== NOTIFY_BAD) ? -EINVAL : 0;
}


static void scu_mu_work_handler(struct work_struct *work)
{
	uint32_t irq_status;
	sc_err_t sciErr;
	int ret;

	/* Figure out what caused the interrupt. */
	sciErr = sc_irq_status(mu_ipcHandle, SC_R_MU_0A, SC_IRQ_GROUP_ALARM,
					&irq_status);

	if (irq_status & SC_IRQ_ALARM_PMIC0_TEMP)
		ret = SCU_notifier_call_chain(1);
}

static irqreturn_t imx8_scu_mu_isr(int irq, void *param)
{
	u32 irqs;

	irqs = (readl_relaxed(mu_base_virtaddr + 0x20) & (0xf << 28));
	if (irqs) {
		/* Clear the General Interrupt */
		writel_relaxed(irqs, mu_base_virtaddr + 0x20);
		/* Setup a bottom-half to handle the irq work. */
		schedule_delayed_work(&scu_mu_work, 0);
	}
	return IRQ_HANDLED;
}

/*Initialization of the MU code. */
int __init imx8dv_mu_init()
{
	struct device_node *np;
	u32 irq;
	int err;
	sc_err_t sciErr;

	/*
	 * Get the address of MU to be used for communication with the SCU
	 */
	np = of_find_compatible_node(NULL, NULL, "fsl,imx8dv-mu");
	if (!np)
		pr_info("Cannot find MU entry in device tree\n");
	mu_base_virtaddr = of_iomap(np, 0);
	WARN_ON(!mu_base_virtaddr);

	err = of_property_read_u32_index(np, "fsl,scu_ap_mu_id", 0, &scu_mu_id);
	if (err)
		pr_info("imx8dv_mu_init: Cannot get mu_id err = %d\n", err);
	
	irq = of_irq_get(np, 0);

	err = request_irq(irq, imx8_scu_mu_isr,
		IRQF_EARLY_RESUME, "imx8_mu_isr", NULL);

	if (err) {
		pr_info("imx8dv mu_init :request_irq failed %d, err = %d\n",
			irq, err);
	}

	if (!scu_mu_init) {
		uint32_t i;
		/* Init MU */
		MU_HAL_Init(mu_base_virtaddr);

		INIT_DELAYED_WORK(&scu_mu_work, scu_mu_work_handler);

		/* Enable all RX interrupts */
		for (i = 0; i < MU_RR_COUNT; i++)
		    MU_HAL_EnableGeneralInt(mu_base_virtaddr, i);

		gIPCport = scu_mu_id;
		scu_mu_init = true;
	}

	sciErr = sc_ipc_open(&mu_ipcHandle, scu_mu_id);
	if (sciErr != SC_ERR_NONE) {
		pr_info("Cannot open MU channel to SCU\n");
		return sciErr;
	};

	/* Request for the high temp interrupt. */
	sciErr = sc_irq_enable(mu_ipcHandle, SC_R_MU_0A, SC_IRQ_GROUP_ALARM,
					SC_IRQ_ALARM_PMIC0_TEMP, true);

	if (sciErr)
		pr_info("Cannot request PMIC0_TEMP interrupt\n");

		/* Request for the high temp interrupt. */
	sciErr = sc_irq_enable(mu_ipcHandle, SC_R_MU_0A, SC_IRQ_GROUP_ALARM,
					SC_IRQ_ALARM_PMIC1_TEMP, true);

	if (sciErr)
		pr_info("Cannot request PMIC1_TEMP interrupt\n");

	pr_info("*****Initialized MU\n");
	return scu_mu_id;
}

early_initcall(imx8dv_mu_init);
