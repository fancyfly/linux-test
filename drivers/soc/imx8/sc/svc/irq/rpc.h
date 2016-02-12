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
 * Header file for the IRQ RPC implementation.
 *
 * @addtogroup IRQ_SVC
 * @{
 */

#ifndef _SC_IRQ_RPC_H
#define _SC_IRQ_RPC_H

/* Includes */

/* Defines */

/* Types */

/*!
 * This type is used to indicate RPC IRQ function calls.
 */
typedef enum irq_func_e
{
    IRQ_FUNC_UNKNOWN, //!< Unknown function
    IRQ_FUNC_ENABLE, //!< Index for irq_enable() RPC call
    IRQ_FUNC_STATUS, //!< Index for irq_status() RPC call
} irq_func_t;

/* Functions */

/*!
 * @name Internal RPC Function
 * @{
 */

/*!
 * This function dispatches an incoming IRQ RPC request.
 *
 * @param[in]     caller_pt   caller partition
 * @param[in]     msg         pointer to RPC message
 */
void irq_dispatch(sc_rm_pt_t caller_pt, sc_rpc_msg_t *msg);

/* @} */

#endif /* _SC_IRQ_RPC_H */

/**@}*/

