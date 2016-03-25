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
 * File containing client-side RPC functions for the MISC service. These
 * function are ported to clients that communicate to the SC.
 *
 * @addtogroup MISC_SVC
 * @{
 */

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>
#include <soc/imx8/sc/svc/misc/api.h>
#include "../../main/rpc.h"
#include "rpc.h"

/* Local Defines */

/* Local Types */

/* Local Functions */

sc_err_t sc_misc_set_control(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_ctrl_t ctrl, uint32_t val)
{
    sc_rpc_msg_t msg;
    uint8_t result;

    RPC_VER(&msg) = SC_RPC_VERSION;
    RPC_SVC(&msg) = SC_RPC_SVC_MISC;
    RPC_FUNC(&msg) = MISC_FUNC_SET_CONTROL;
    RPC_D32(&msg, 0) = ctrl;
    RPC_D32(&msg, 4) = val;
    RPC_D16(&msg, 8) = resource;
    RPC_SIZE(&msg) = 4;

    sc_call_rpc(ipc, &msg, false);

    result = RPC_R8(&msg);
    return result;
}

sc_err_t sc_misc_get_control(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_ctrl_t ctrl, uint32_t *val)
{
    sc_rpc_msg_t msg;
    uint8_t result;

    RPC_VER(&msg) = SC_RPC_VERSION;
    RPC_SVC(&msg) = SC_RPC_SVC_MISC;
    RPC_FUNC(&msg) = MISC_FUNC_GET_CONTROL;
    RPC_D32(&msg, 0) = ctrl;
    RPC_D16(&msg, 4) = resource;
    RPC_SIZE(&msg) = 3;

    sc_call_rpc(ipc, &msg, false);

    if (val != NULL)
        *val = RPC_D32(&msg, 0);
    result = RPC_R8(&msg);
    return result;
}

sc_err_t sc_misc_set_ari(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_rsrc_t master, uint16_t ari, bool enable)
{
    sc_rpc_msg_t msg;
    uint8_t result;

    RPC_VER(&msg) = SC_RPC_VERSION;
    RPC_SVC(&msg) = SC_RPC_SVC_MISC;
    RPC_FUNC(&msg) = MISC_FUNC_SET_ARI;
    RPC_D16(&msg, 0) = resource;
    RPC_D16(&msg, 2) = master;
    RPC_D16(&msg, 4) = ari;
    RPC_D8(&msg, 6) = enable;
    RPC_SIZE(&msg) = 3;

    sc_call_rpc(ipc, &msg, false);

    result = RPC_R8(&msg);
    return result;
}

/**@}*/

