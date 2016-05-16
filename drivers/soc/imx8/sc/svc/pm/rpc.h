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

/*!
 * Header file for the PM RPC implementation.
 *
 * @addtogroup PM_SVC
 * @{
 */

#ifndef _SC_PM_RPC_H
#define _SC_PM_RPC_H

/* Includes */

/* Defines */

/* Types */

/*!
 * This type is used to indicate RPC PM function calls.
 */
typedef enum pm_func_e
{
    PM_FUNC_UNKNOWN, //!< Unknown function
    PM_FUNC_SET_SYS_POWER_MODE, //!< Index for pm_set_sys_power_mode() RPC call
    PM_FUNC_GET_SYS_POWER_MODE, //!< Index for pm_get_sys_power_mode() RPC call
    PM_FUNC_SET_RESOURCE_POWER_MODE, //!< Index for pm_set_resource_power_mode() RPC call
    PM_FUNC_GET_RESOURCE_POWER_MODE, //!< Index for pm_get_resource_power_mode() RPC call
    PM_FUNC_SET_CLOCK_RATE, //!< Index for pm_set_clock_rate() RPC call
    PM_FUNC_GET_CLOCK_RATE, //!< Index for pm_get_clock_rate() RPC call
    PM_FUNC_CLOCK_ENABLE, //!< Index for pm_clock_enable() RPC call
    PM_FUNC_BOOT, //!< Index for pm_boot() RPC call
    PM_FUNC_REBOOT, //!< Index for pm_reboot() RPC call
    PM_FUNC_RESET_REASON, //!< Index for pm_reset_reason() RPC call
    PM_FUNC_CPU_START, //!< Index for pm_cpu_start() RPC call
    PM_FUNC_REBOOT_PARTITION, //!< Index for pm_reboot_partition() RPC call
    PM_FUNC_RESET, //!< Index for pm_reset() RPC call
} pm_func_t;

/* Functions */

/*!
 * @name Internal RPC Function
 * @{
 */

/*!
 * This function dispatches an incoming PM RPC request.
 *
 * @param[in]     caller_pt   caller partition
 * @param[in]     msg         pointer to RPC message
 */
void pm_dispatch(sc_rm_pt_t caller_pt, sc_rpc_msg_t *msg);

/* @} */

#endif /* _SC_PM_RPC_H */

/**@}*/

