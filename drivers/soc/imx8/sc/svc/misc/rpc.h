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
 * Header file for the MISC RPC implementation.
 *
 * @addtogroup MISC_SVC
 * @{
 */

#ifndef _SC_MISC_RPC_H
#define _SC_MISC_RPC_H

/* Includes */

/* Defines */

/* Types */

/*!
 * This type is used to indicate RPC MISC function calls.
 */
typedef enum misc_func_e
{
    MISC_FUNC_UNKNOWN, //!< Unknown function
    MISC_FUNC_SET_CONTROL, //!< Index for misc_set_control() RPC call
    MISC_FUNC_GET_CONTROL, //!< Index for misc_get_control() RPC call
    MISC_FUNC_SET_ARI, //!< Index for misc_set_ari() RPC call
} misc_func_t;

/* Functions */

/*!
 * @name Internal RPC Function
 * @{
 */

/*!
 * This function dispatches an incoming MISC RPC request.
 *
 * @param[in]     caller_pt   caller partition
 * @param[in]     msg         pointer to RPC message
 */
void misc_dispatch(sc_rm_pt_t caller_pt, sc_rpc_msg_t *msg);

/* @} */

#endif /* _SC_MISC_RPC_H */

/**@}*/

