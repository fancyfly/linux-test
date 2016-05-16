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
 * Header file for the TIMER RPC implementation.
 *
 * @addtogroup TIMER_SVC
 * @{
 */

#ifndef _SC_TIMER_RPC_H
#define _SC_TIMER_RPC_H

/* Includes */

/* Defines */

/* Types */

/*!
 * This type is used to indicate RPC TIMER function calls.
 */
typedef enum timer_func_e
{
    TIMER_FUNC_UNKNOWN, //!< Unknown function
    TIMER_FUNC_SET_WDOG_TIMEOUT, //!< Index for timer_set_wdog_timeout() RPC call
    TIMER_FUNC_START_WDOG, //!< Index for timer_start_wdog() RPC call
    TIMER_FUNC_STOP_WDOG, //!< Index for timer_stop_wdog() RPC call
    TIMER_FUNC_PING_WDOG, //!< Index for timer_ping_wdog() RPC call
    TIMER_FUNC_GET_WDOG_STATUS, //!< Index for timer_get_wdog_status() RPC call
    TIMER_FUNC_SET_RTC_TIME, //!< Index for timer_set_rtc_time() RPC call
    TIMER_FUNC_GET_RTC_TIME, //!< Index for timer_get_rtc_time() RPC call
    TIMER_FUNC_SET_RTC_ALARM, //!< Index for timer_set_rtc_alarm() RPC call
    TIMER_FUNC_GET_RTC_SEC1970, //!< Index for timer_get_rtc_sec1970() RPC call
} timer_func_t;

/* Functions */

/*!
 * @name Internal RPC Function
 * @{
 */

/*!
 * This function dispatches an incoming TIMER RPC request.
 *
 * @param[in]     caller_pt   caller partition
 * @param[in]     msg         pointer to RPC message
 */
void timer_dispatch(sc_rm_pt_t caller_pt, sc_rpc_msg_t *msg);

/* @} */

#endif /* _SC_TIMER_RPC_H */

/**@}*/

