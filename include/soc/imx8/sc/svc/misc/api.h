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
 * Header file containing the public API for the System Controller (SC)
 * Miscellaneous (MISC) function.
 *
 * @addtogroup MISC_SVC (SVC) Miscellaneous Service
 *
 * Module for the Miscellaneous (MISC) service.
 *
 * @{
 */

#ifndef _SC_MISC_API_H
#define _SC_MISC_API_H

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>

/* Defines */

/* Types */

/* Functions */

/*!
 * This function sets a miscellaneous control value.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    resource the control is associated with
 * @param[in]     ctrl        control to change
 * @param[in]     val         value to apply to the control
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the resource owner or parent
 *   of the owner
 *
 * Refer to the [Control List](@ref CONTROLS) for valid control values.
 */
sc_err_t sc_misc_set_control(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_ctrl_t ctrl, uint32_t val);

/*!
 * This function gets a miscellaneous control value.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    resource the control is associated with
 * @param[in]     ctrl        control to get
 * @param[out]    val         pointer to return the control value
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the resource owner or parent
 *   of the owner
 *
 * Refer to the [Control List](@ref CONTROLS) for valid control values.
 */
sc_err_t sc_misc_get_control(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_ctrl_t ctrl, uint32_t *val);

/*!
 * This function configures the ARI match value for PCIe/SATA resources.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    match resource
 * @param[in]     master      PCIe/SATA master to match
 * @param[in]     ari         ARI to match
 * @param[in]     enable      enable match or not
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the owner or parent
 *   of the owner of the resource and translation
 *
 * For PCIe, the ARI is the 16-bit value that includes the bus number,
 * device number, and function number. For SATA, this value includes the
 * FISType and PM_Port.
 */
sc_err_t sc_misc_set_ari(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_rsrc_t master, uint16_t ari, bool enable);

#endif /* _SC_MISC_API_H */

/**@}*/

