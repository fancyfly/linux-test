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
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>

#include <soc/imx8/sc/ipc.h>
#include <soc/imx8/sc/sci.h>
#include "../../mu/fsl_mu_hal.h"
#include "rpc.h"

/* Local Defines */
#define MU_SIZE 0x10000

/* Local Types */
unsigned int scu_mu_id;
static unsigned long mu_base_physaddr;
static void __iomem *mu_base_virtaddr;

/* Local functions */

/* Local variables */
static uint32_t gIPCport;
static bool scu_mu_init;

DEFINE_MUTEX(scu_mu_mutex);

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

static const char * const mu_data[] __initconst = {
	"fsl,imx8dv-mu",
	NULL
	};

static int __init imx8dv_dt_find_muaddr(unsigned long node, const char *uname,
    int depth, void *data)
{
	const __be64 *prop64;
	const __be32 *prop32;

	if (of_flat_dt_match(node, mu_data)) {
		prop64 = of_get_flat_dt_prop(node, (const char *)"reg", NULL);

		if (!prop64)
			return -EINVAL;
		mu_base_physaddr = be64_to_cpup(prop64);

		prop32 = of_get_flat_dt_prop(node, (const char *)"fsl,scu_ap_mu_id", NULL);
		if (!prop32)
			return -EINVAL;
		scu_mu_id = be32_to_cpup(prop32);
	}
	return 0;
}

/*Initialization of the MU code. */
int __init imx8dv_mu_init()
{
	/*
	 * Get the address of MU to be used for communication with the SCU
	 */
	WARN_ON(of_scan_flat_dt(imx8dv_dt_find_muaddr, NULL));

	if (!mu_base_physaddr) {
		pr_info("Cannot find MU ADDR in device tree \n");
		return -EINVAL;
	}
	mu_base_virtaddr = ioremap(mu_base_physaddr, SZ_64K);

	if (!scu_mu_init) {
		uint32_t i;
		/* Init MU */
		MU_HAL_Init(mu_base_virtaddr);

		/* Enable all RX interrupts */
		for (i = 0; i < MU_RR_COUNT; i++)
		    MU_HAL_EnableRxFullInt(mu_base_virtaddr, i);

		gIPCport = scu_mu_id;
		scu_mu_init = true;
	}

	pr_info("*****Initialized MU\n");
	return scu_mu_id;
}

early_initcall(imx8dv_mu_init);
