/*==========================================================================*/
/*!
 * @file  main/rpc.h
 *
 * Header file for the RPC implementation.
 */
/*==========================================================================*/

#ifndef _SC_RPC_H
#define _SC_RPC_H

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/ipc.h>

/* Defines */

#define SC_RPC_VERSION          1

#define SC_RPC_MAX_MSG          8

#define RPC_VER(MSG)            ((MSG)->version)
#define RPC_SIZE(MSG)           ((MSG)->size)
#define RPC_SVC(MSG)            ((MSG)->svc)
#define RPC_FUNC(MSG)           ((MSG)->func)
#define RPC_R8(MSG)             ((MSG)->func)
#define RPC_D32(MSG, IDX)       ((MSG)->DATA.d32[IDX / 4])
#define RPC_D16(MSG, IDX)       ((MSG)->DATA.d16[IDX / 2])
#define RPC_D8(MSG, IDX)        ((MSG)->DATA.d8[IDX])
                
/* Types */

typedef enum sc_rpc_svc_e
{
    SC_RPC_SVC_UNKNOWN          = 0,
    SC_RPC_SVC_RETURN           = 1,        
    SC_RPC_SVC_PM               = 2,
    SC_RPC_SVC_RM               = 3,
    SC_RPC_SVC_OTP              = 4,
    SC_RPC_SVC_TIMER            = 5,
    SC_RPC_SVC_PAD              = 6,
    SC_RPC_SVC_MISC             = 7,
    SC_RPC_SVC_ABORT            = 8        
} sc_rpc_svc_t;

typedef struct sc_rpc_msg_s
{
    uint8_t version;
    uint8_t size;
    uint8_t svc;
    uint8_t func;   
    union
    {
        uint32_t d32[(SC_RPC_MAX_MSG - 1)];
        uint16_t d16[(SC_RPC_MAX_MSG - 1) * 2];
        uint8_t d8[(SC_RPC_MAX_MSG - 1) * 4];
    } DATA;
} sc_rpc_msg_t;

typedef enum sc_rpc_async_state_e
{
    SC_RPC_ASYNC_STATE_RD_START     = 0,
    SC_RPC_ASYNC_STATE_RD_ACTIVE    = 1,
    SC_RPC_ASYNC_STATE_RD_DONE      = 2,
    SC_RPC_ASYNC_STATE_WR_START     = 3,
    SC_RPC_ASYNC_STATE_WR_ACTIVE    = 4,
    SC_RPC_ASYNC_STATE_WR_DONE      = 5,
} sc_rpc_async_state_t;

typedef struct sc_rpc_async_msg_s
{
    sc_rpc_async_state_t state;
    uint8_t wordIdx;
    sc_rpc_msg_t msg;
    uint32_t timeStamp;    
} sc_rpc_async_msg_t;

/* Functions */

void sc_call_rpc(sc_ipc_t ipc, sc_rpc_msg_t *msg, bool no_resp);
void sc_rpc_dispatch(sc_rsrc_t mu, sc_rpc_msg_t *msg);

#endif /* _SC_RPC_H */

