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
 * One Time Programmable (OTP) function.
 *
 * @addtogroup OTP_SVC (SVC) One Time Programmable Service
 *
 * Module for the One Time Programmable (OTP) service.
 *
 * @{
 */

#ifndef _SC_OTP_API_H
#define _SC_OTP_API_H

/* Includes */

#include <soc/imx8/sc/types.h>

/* Defines */

/* Types */

/*!
 * This type is used to declare OTP values in word lengths.
 */
typedef uint32_t sc_otp_word_t;

/*!
 * This type is used to declare OTP offset values (of OTP word lengths).
 */
typedef uint8_t sc_otp_offset_t;

/* Functions */

/*!
 * This function reads the OTP value. 
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     offset      offset into OTP region
 * @param[out]    data        data to read from the OTP 
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *         
 * Return errors:
 * - SC_ERR_UNAVAILABLE if caller's partition has no OTP resources,
 * - SC_ERR_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS No read allowed
 */
sc_err_t sc_otp_read(sc_ipc_t ipc, sc_otp_word_t *data,
    sc_otp_offset_t offset);

/*!
 * This function writes the OTP value. 
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     offset      offset into OTP region
 * @param[in]     data        data to write to the OTP 
 * @param[in]     bitmask     mask bits to program 
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *         
 * Return errors:
 * - SC_ERR_UNAVAILABLE if caller's partition has no OTP resources,
 * - SC_ERR_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS No write allowed
 */
sc_err_t sc_otp_write(sc_ipc_t ipc, sc_otp_word_t data,
    sc_otp_offset_t offset, sc_otp_word_t bitmask);

/*!
 * This function allows the owner of a partition to set and lock access
 * permissions. These permissions are carried forward if the OTP is
 * reassigned
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     offset      OTP word to access
 * @param[in]     readen      allows read access
 * @param[in]     writeen     allows write access
 * @param[in]     lock        no further changes to permissions during
 *                            power cycle is allowed
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_UNAVAILABLE if caller's partition has no OTP resources,
 * - SC_ERR_LOCKED if partition is already locked
 * 
 * Assigns some part of the OTP resource owned by the caller's partition
 * to another partition.
 */
sc_err_t sc_otp_set_permissions(sc_ipc_t ipc, sc_otp_offset_t offset,
    bool readen, bool writeen, bool lock);

#endif /* _SC_OTP_API_H */

/**@}*/

