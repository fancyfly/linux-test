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

#ifndef _SC_SCFW_H
#define _SC_SCFW_H

#include <linux/types.h>

/*!
 * This type is used to declare a handle for an IPC communication
 * channel. Its meaning is specific to the IPC implementation.
 */
typedef uint32_t sc_ipc_t;

/*! 
 * This type is used to declare an ID for an IPC communication
 * channel. Its meaning is specific to the IPC implementation.
 */
typedef uint32_t sc_ipc_id_t;

/*!
 * This function returns the MU channel ID for this implementation
 *
 * @param[in]     ipc         pointer to Mu channel ID
 * @return Returns an error code (SC_ERR_NONE = success, SC_ERR_IPC
 *         otherwise).
 */
int sc_ipc_getMuID(uint32_t *mu_id);

#endif
