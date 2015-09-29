/*==========================================================================*/
/*!
 * @file  main/rpc.c
 *
 * Implementation of the RPC dispatch function.
 */
/*==========================================================================*/

/* Includes */

#include <soc/imx8/sc/svc/pm/rpc.h>
#include <soc/imx8/sc/svc/rm/rpc.h>
#include <soc/imx8/sc/svc/otp/rpc.h>
#include <soc/imx8/sc/svc/wdog/rpc.h>
#include <soc/imx8/sc/svc/misc/rpc.h>
#include "rpc.h"


/* Local Defines */

/* Local Types */

/* Local Functions */

/* Local Variables */

/*--------------------------------------------------------------------------*/
/* Dispatch RPC call                                                        */
/*--------------------------------------------------------------------------*/
void sc_rpc_dispatch(sc_rsrc_t mu, sc_rpc_msg_t *msg)
{
    sc_rm_pt_t caller_pt = 0;
    sc_rm_idx_t idx = 0;

    /* Map MU */
    if (rm_check_map_ridx(mu, &idx))
        rm_get_ridx_owner(idx, &caller_pt);
    else
        return;

    /* Check request size */
    if (RPC_SIZE(msg) == 0)
    {
        rpc_print(0, "ipc_error: bad size (%d)\n", RPC_SIZE(msg));

        RPC_VER(msg) = SC_RPC_VERSION;
        RPC_SIZE(msg) = 1;
        RPC_SVC(msg) = SC_RPC_SVC_RETURN;
        RPC_R8(msg) = SC_ERR_IPC;

        return;        
    }
    
    /* Check request API version */
    if (RPC_VER(msg) != SC_RPC_VERSION)
    {
        rpc_print(0, "ipc_error: bad version (%d)\n", RPC_VER(msg));

        RPC_VER(msg) = SC_RPC_VERSION;
        RPC_SIZE(msg) = 1;
        RPC_SVC(msg) = SC_RPC_SVC_RETURN;
        RPC_R8(msg) = SC_ERR_VERSION;

        return;
    }

    /* Call requested service dispatch */    
    switch (RPC_SVC(msg))
    {
        case SC_RPC_SVC_PM :
            pm_dispatch(caller_pt, msg);
            return; 
        case SC_RPC_SVC_RM :
            rm_dispatch(caller_pt, msg);
            return;
        case SC_RPC_SVC_OTP :
            otp_dispatch(caller_pt, msg);
            return;
        case SC_RPC_SVC_WDOG :
            wdog_dispatch(caller_pt, msg);
            return;
        case SC_RPC_SVC_PAD :
            pad_dispatch(caller_pt, msg);
            return;
        case SC_RPC_SVC_MISC :
            misc_dispatch(caller_pt, msg);
            return;
        default :
            break;
    }

    /* Service not found */
    rpc_print(0, "ipc_error: bad service (%d)\n", RPC_SVC(msg));
    
    RPC_VER(msg) = SC_RPC_VERSION;
    RPC_SIZE(msg) = 1;
    RPC_SVC(msg) = SC_RPC_SVC_RETURN;
    RPC_R8(msg) = SC_ERR_NOTFOUND;
}

