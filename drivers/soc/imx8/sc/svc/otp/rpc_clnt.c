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
 * File containing client-side RPC functions for the OTP service. These
 * function are ported to clients that communicate to the SC.
 *
 * @addtogroup OTP_SVC
 * @{
 */

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>
#include <soc/imx8/sc/svc/otp/api.h>
#include "../../main/rpc.h"
#include "rpc.h"

/* Local Defines */

/* Local Types */

/* Local Functions */

sc_err_t sc_otp_read(sc_ipc_t ipc, sc_otp_word_t *data,
    sc_otp_offset_t offset)
{
    sc_rpc_msg_t msg;
    uint8_t result;

    RPC_VER(&msg) = SC_RPC_VERSION;
    RPC_SVC(&msg) = SC_RPC_SVC_OTP;
    RPC_FUNC(&msg) = OTP_FUNC_READ;
    RPC_D8(&msg, 0) = offset;
    RPC_SIZE(&msg) = 2;

    sc_call_rpc(ipc, &msg, false);

    if (data != NULL)
        *data = RPC_D32(&msg, 0);
    result = RPC_R8(&msg);
    return result;
}

sc_err_t sc_otp_write(sc_ipc_t ipc, sc_otp_word_t data,
    sc_otp_offset_t offset, sc_otp_word_t bitmask)
{
    sc_rpc_msg_t msg;
    uint8_t result;

    RPC_VER(&msg) = SC_RPC_VERSION;
    RPC_SVC(&msg) = SC_RPC_SVC_OTP;
    RPC_FUNC(&msg) = OTP_FUNC_WRITE;
    RPC_D32(&msg, 0) = data;
    RPC_D32(&msg, 4) = bitmask;
    RPC_D8(&msg, 8) = offset;
    RPC_SIZE(&msg) = 4;

    sc_call_rpc(ipc, &msg, false);

    result = RPC_R8(&msg);
    return result;
}

sc_err_t sc_otp_set_permissions(sc_ipc_t ipc, sc_otp_offset_t offset,
    bool readen, bool writeen, bool lock)
{
    sc_rpc_msg_t msg;
    uint8_t result;

    RPC_VER(&msg) = SC_RPC_VERSION;
    RPC_SVC(&msg) = SC_RPC_SVC_OTP;
    RPC_FUNC(&msg) = OTP_FUNC_SET_PERMISSIONS;
    RPC_D8(&msg, 0) = offset;
    RPC_D8(&msg, 1) = readen;
    RPC_D8(&msg, 2) = writeen;
    RPC_D8(&msg, 3) = lock;
    RPC_SIZE(&msg) = 2;

    sc_call_rpc(ipc, &msg, false);

    result = RPC_R8(&msg);
    return result;
}

/**@}*/

