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
 * Header file for the PAD RPC implementation.
 *
 * @addtogroup PAD_SVC
 * @{
 */

#ifndef _SC_PAD_RPC_H
#define _SC_PAD_RPC_H

/* Includes */

/* Defines */

/* Types */

/*!
 * This type is used to indicate RPC PAD function calls.
 */
typedef enum pad_func_e
{
    PAD_FUNC_UNKNOWN, //!< Unknown function
    PAD_FUNC_SET_MUX, //!< Index for pad_set_mux() RPC call
    PAD_FUNC_SET_GP, //!< Index for pad_set_gp() RPC call
    PAD_FUNC_SET_GP_28LPP, //!< Index for pad_set_gp_28lpp() RPC call
    PAD_FUNC_SET_WAKEUP, //!< Index for pad_set_wakeup() RPC call
    PAD_FUNC_SET_ALL, //!< Index for pad_set_all() RPC call
    PAD_FUNC_GET_MUX, //!< Index for pad_get_mux() RPC call
    PAD_FUNC_GET_GP, //!< Index for pad_get_gp() RPC call
    PAD_FUNC_GET_GP_28LPP, //!< Index for pad_get_gp_28lpp() RPC call
    PAD_FUNC_GET_WAKEUP, //!< Index for pad_get_wakeup() RPC call
    PAD_FUNC_GET_ALL, //!< Index for pad_get_all() RPC call
    PAD_FUNC_SET_GP_28FDSOI, //!< Index for pad_set_gp_28fdsoi() RPC call
    PAD_FUNC_GET_GP_28FDSOI, //!< Index for pad_get_gp_28fdsoi() RPC call
    PAD_FUNC_SET_GP_28FDSOI_COMP, //!< Index for pad_set_gp_28fdsoi_comp() RPC call
    PAD_FUNC_GET_GP_28FDSOI_COMP, //!< Index for pad_get_gp_28fdsoi_comp() RPC call
} pad_func_t;

/* Functions */

/*!
 * @name Internal RPC Function
 * @{
 */

/*!
 * This function dispatches an incoming PAD RPC request.
 *
 * @param[in]     caller_pt   caller partition
 * @param[in]     msg         pointer to RPC message
 */
void pad_dispatch(sc_rm_pt_t caller_pt, sc_rpc_msg_t *msg);

/* @} */

#endif /* _SC_PAD_RPC_H */

/**@}*/

