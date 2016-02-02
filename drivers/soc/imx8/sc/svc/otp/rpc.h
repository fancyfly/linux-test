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
 * Header file for the OTP RPC implementation.
 *
 * @addtogroup OTP_SVC
 * @{
 */

#ifndef _SC_OTP_RPC_H
#define _SC_OTP_RPC_H

/* Includes */

/* Defines */

/* Types */

/*!
 * This type is used to indicate RPC OTP function calls.
 */
typedef enum otp_func_e
{
    OTP_FUNC_UNKNOWN, //!< Unknown function
    OTP_FUNC_READ, //!< Index for otp_read() RPC call
    OTP_FUNC_WRITE, //!< Index for otp_write() RPC call
    OTP_FUNC_SET_PERMISSIONS, //!< Index for otp_set_permissions() RPC call
} otp_func_t;

/* Functions */

/*!
 * @name Internal RPC Function
 * @{
 */

/*!
 * This function dispatches an incoming OTP RPC request.
 *
 * @param[in]     caller_pt   caller partition
 * @param[in]     msg         pointer to RPC message
 */
void otp_dispatch(sc_rm_pt_t caller_pt, sc_rpc_msg_t *msg);

/* @} */

#endif /* _SC_OTP_RPC_H */

/**@}*/

