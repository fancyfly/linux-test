/*
 * Copyright (c) 2012-2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


///////////////////////////////////////////////////////////////////////////////
//
// zoe_ipc_srv.h
//
// Description: 
//
//  ZOE ipc service
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_IPC_SRV_H__
#define __ZOE_IPC_SRV_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_dbg.h"
#include "zoe_hal.h"
#include "zoe_ipc_def.h"
#include "zoe_cthread.h"
#include "zoe_cqueue.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __ZOE_IPC_SRV_FWD_DEFINED__
#define __ZOE_IPC_SRV_FWD_DEFINED__
typedef struct CZoeIPCService CZoeIPCService;
#endif //__ZOE_IPC_SRV_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

typedef struct _ZOE_IPC_ENTRY
{
    ZOE_IPC_MSG             msg;
    zoe_sosal_obj_id_t      evt;
    PZOE_IPC_OVERLAPPED     pOverlapped;
} ZOE_IPC_ENTRY, *PZOE_IPC_ENTRY;

#define ZOE_IPC_MAX_MSG_ENTRIES 256

typedef struct _ZOE_IPC_SRV_THREAD_CNXT
{
    ZOE_IPC_CPU             cpu_id;
    CZoeIPCService          *pZoeIPCService;
} ZOE_IPC_SRV_THREAD_CNXT, *PZOE_IPC_SRV_THREAD_CNXT;



typedef zoe_errs_t (*ZOE_IPC_DISPATCH_FUNC)(void *pCnxt,
                                            ZOE_IPC_CPU from_cpu,
                                            PZOE_IPC_MSG pMsg
                                            );


// dispatch module
//
typedef struct _ZOE_IPC_DISPATCH_MODULE
{
    uint32_t                module;
    ZOE_IPC_DISPATCH_FUNC   dispatch_func;
    uint32_t                inst_num;
    zoe_bool_t              inst_set[ZOE_IPC_MAX_INST];
    void                    *context[ZOE_IPC_MAX_INST];
    c_thread                *thread[ZOE_IPC_MAX_INST];
    struct _ZOE_IPC_DISPATCH_MODULE *pNext;
} ZOE_IPC_DISPATCH_MODULE, *PZOE_IPC_DISPATCH_MODULE;


// dispatch interface
//
typedef struct _ZOE_IPC_DISPATCH_INTF
{
    ZOE_IPC_DISPATCH_MODULE *pModuleHead;
} ZOE_IPC_DISPATCH_INTF, *PZOE_IPC_DISPATCH_INTF;


// direction
//
enum _ZOE_IPC_DIRECTION
{   
    ZOE_IPC_DIR_RX = 0,
    ZOE_IPC_DIR_TX,
    ZOE_IPC_DIR_END
};

#define NUM_ZOE_IPC_DIR ZOE_IPC_DIR_END


/////////////////////////////////////////////////////////////////////////////
//
//

// zoe ipc service instance data
//
struct CZoeIPCService
{
    // c_object
    //
    c_object                 m_Object;

    // c_thread
    //
    c_thread                m_Thread[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM];
    ZOE_IPC_SRV_THREAD_CNXT m_ThreadCnxt[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM];

    // CZoeIPCService
    //
    IZOEHALAPI              *m_pHal;
    zoe_dbg_comp_id_t       m_dbgID;
    ZOE_IPC_CPU             m_cpuID;
    uint32_t                m_msgID;
    void                    *m_device_object; // only valid in Linux kernel


    uint32_t                m_rxChannels[ZOE_IPC_CPU_NUM];
    uint32_t                m_txChannels[ZOE_IPC_CPU_NUM];

    // mailbox storage
    zoe_dev_mem_t           m_mb_space_phy;
    void                    *m_mb_space_vir;

    zoe_dev_mem_t           m_MBAddrs_phy[ZOE_IPC_CHANNEL_NUM];
    zoe_uintptr_t           m_MBAddrs[ZOE_IPC_CHANNEL_NUM];
    
    ZOE_IPC_ENTRY           m_Msgs[ZOE_IPC_MAX_MSG_ENTRIES];
    QUEUE_ENTRY             m_Entries[ZOE_IPC_MAX_MSG_ENTRIES];

    c_queue                 *m_pFreeQueue;
    c_queue                 *m_pRxQueue[ZOE_IPC_CPU_NUM];
    c_queue                 *m_pTxQueue[ZOE_IPC_CPU_NUM];
    c_queue                 *m_pWaitRespQueue[ZOE_IPC_CPU_NUM];
    c_queue                 *m_pResponseQueue[ZOE_IPC_CPU_NUM];

    // thread events
    zoe_sosal_obj_id_t      m_EvtMsg[NUM_ZOE_IPC_DIR][ZOE_IPC_CPU_NUM];

    // api dispatch table
    ZOE_IPC_DISPATCH_INTF   m_IPCDispatchTable[ZOE_IPC_MAX_INTF];
};


/////////////////////////////////////////////////////////////////////////////
//
//

CZoeIPCService * c_zoe_ipc_service_constructor(CZoeIPCService *pZoeIPCService,
											   c_object *pParent, 
							                   uint32_t dwAttributes,
                                               IZOEHALAPI *pHal,
                                               zoe_dbg_comp_id_t dbgID,
                                               zoe_sosal_isr_sw_numbers_t isr,
                                               void *device_object // only valid in Linux kernel
							                   );
void c_zoe_ipc_service_destructor(CZoeIPCService *This);

// zoe ipc open
//
zoe_errs_t c_zoe_ipc_service_open(CZoeIPCService *This);

// zoe ipc close
//
zoe_errs_t c_zoe_ipc_service_close(CZoeIPCService *This);

// zoe ipc service send message to cpu
//
zoe_errs_t c_zoe_ipc_service_send_message(CZoeIPCService *This, 
                                          ZOE_IPC_CPU cpu_id,
                                          PZOE_IPC_MSG pMsg,
                                          PZOE_IPC_MSG pResp,
                                          int32_t milliseconds
                                          );
// zoe ipc service post message to cpu
//
zoe_errs_t c_zoe_ipc_service_post_message(CZoeIPCService *This, 
                                          ZOE_IPC_CPU cpu_id,
                                          PZOE_IPC_MSG pMsg,
                                          PZOE_IPC_OVERLAPPED pOverlapped
                                          );
// zoe ipc register interface
//
zoe_errs_t c_zoe_ipc_service_register_interface(CZoeIPCService *This, 
                                                uint32_t intf,
                                                uint32_t module,
                                                uint32_t inst,
                                                void *pContext,
                                                ZOE_IPC_DISPATCH_FUNC dispatch_func,
                                                zoe_bool_t use_thread,
                                                uint32_t priority
                                                );
// zoe ipc unregister interface
//
zoe_errs_t c_zoe_ipc_service_unregister_interface(CZoeIPCService *This, 
                                                  uint32_t intf,
                                                  uint32_t module,
                                                  uint32_t inst
                                                  );

// zoe ipc debug queues
//
void c_zoe_ipc_service_debug_queues(CZoeIPCService *This, 
                                    ZOE_IPC_CPU cpu_id
                                    );

STATIC_INLINE ZOE_IPC_CPU c_zoe_ipc_service_get_cpu_id(CZoeIPCService *This) 
{
    return (This->m_cpuID);
}


// singleton used by zpcgen
//
CZoeIPCService * c_zoe_ipc_service_get_ipc_svc(void);

// get CPU id from interrupt number
//
ZOE_IPC_CPU c_zoe_ipc_service_get_cpu_from_interrupt(zoe_sosal_isr_sw_numbers_t isr);

// get per instance data for interface
//
void * c_zoe_ipc_service_get_interface_context(CZoeIPCService *This,
                                               uint32_t intf,
                                               uint32_t module,
                                               uint32_t inst
                                               );

// check target CPU mailbox open
//
zoe_bool_t c_zoe_ipc_service_target_opened(CZoeIPCService *This, 
                                           uint32_t cpu_id
                                           );

// clear target CPU mailbox open
//
void c_zoe_ipc_service_target_open_clr(CZoeIPCService *This, 
                                       uint32_t cpu_id
                                       );

#ifdef __cplusplus
}
#endif

#endif //__ZOE_IPC_SRV_H__



