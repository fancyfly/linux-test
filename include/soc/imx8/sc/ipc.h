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
 * Header file for the IPC implementation.
 */

#ifndef _SC_IPC_H
#define _SC_IPC_H

/* Includes */

#include <soc/imx8/sc/types.h>

/* Defines */

/* Types */

/* Functions */

/*!
 * This function opens an IPC channel.
 *
 * @param[out]    ipc         return pointer for ipc handle
 * @param[in]     id          id of channel to open
 *
 * @return Returns an error code (SC_ERR_NONE = success, SC_ERR_IPC
 *         otherwise).
 *
 * The \a id parameter is implementation specific. Could be an MU
 * address, pointer to a driver path, channel index, etc.
 */
sc_err_t sc_ipc_open(sc_ipc_t *ipc, sc_ipc_id_t id);

/*!
 * This function closes an IPC channel.
 *
 * @param[in]     ipc         pointer to id of channel to close
 */
void sc_ipc_close(sc_ipc_t *ipc);

/*!
 * This function returns the MU channel ID for this implementation
 *
 * @param[in]     ipc         pointer to Mu channel ID
 * @return Returns an error code (SC_ERR_NONE = success, SC_ERR_IPC
 *         otherwise).
 */
int sc_ipc_getMuID(uint32_t *mu_id);

#endif /* _SC_IPC_H */

