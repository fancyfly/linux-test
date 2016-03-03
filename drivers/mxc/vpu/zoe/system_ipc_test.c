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
// ipc_test.c
//
// Description: 
//
//  zoe ipc test
// 
// Authors: (DT)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOE_LINUXKER_BUILD
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#endif //!ZOE_LINUXKER_BUILD
#include "zoe_dbg.h"
#include "zoe_types.h"
#include "zoe_sosal.h"
#include "zoe_ipc_srv.h"


/////////////////////////////////////////////////////////////////////////////
//
//

#define _IPC_TEST_DONT_SLEEP
#define _IPC_SEND_TIMEOUT   5000

/////////////////////////////////////////////////////////////////////////////
//
//

static void test_ipc(CZoeIPCService *p_zoe_ipc_srv, 
                     uint32_t id,
                     ZOE_IPC_CPU to_cpu
                     );

typedef void (*test_func)(CZoeIPCService *p_zoe_ipc_srv, uint32_t id, ZOE_IPC_CPU to_cpu);

typedef struct _ipc_test_thread_info_t
{
    uint32_t            id;
    uint32_t            wait_ms;
    zoe_sosal_obj_id_t  thread_id;
    zoe_sosal_obj_id_t  event_id;
    test_func           func;
    char                *p_name;
    CZoeIPCService      *p_zoe_ipc_srv;
    ZOE_IPC_CPU         to_cpu;
    volatile zoe_bool_t test_running;
} ipc_test_thread_info_t, *p_ipc_test_thread_info_t;


static ipc_test_thread_info_t s_agent_threads[] = 
{
    {0, 10,  ZOE_NULL, ZOE_NULL, test_ipc,  "SPU"    ,ZOE_NULL},
    {1, 33,  ZOE_NULL, ZOE_NULL, test_ipc,  "FWPU"   ,ZOE_NULL},
    {2, 20,  ZOE_NULL, ZOE_NULL, test_ipc,  "AUD0PU" ,ZOE_NULL},
    {3, 200, ZOE_NULL, ZOE_NULL, test_ipc,  "AUD1PU" ,ZOE_NULL},
    {4, 50,  ZOE_NULL, ZOE_NULL, test_ipc,  "AUD2PU" ,ZOE_NULL},
    {5, 300, ZOE_NULL, ZOE_NULL, test_ipc,  "MEPU"   ,ZOE_NULL},
    {6, 66,  ZOE_NULL, ZOE_NULL, test_ipc,  "HPU"    ,ZOE_NULL},
    {7, 500, ZOE_NULL, ZOE_NULL, test_ipc,  "EXT"    ,ZOE_NULL}
};

#define AGENTS_NUM  (sizeof(s_agent_threads) / sizeof(s_agent_threads[0]))

#define IPC_TEST_NUM_MODULE 12

static CZoeIPCService       *s_p_zoe_ipc_srv = ZOE_NULL;
volatile static uint32_t    s_num_ipc_calls = 0;
static zoe_bool_t           s_ipc_test_registered = ZOE_FALSE;
static uint32_t             s_ipc_test_num_modules = IPC_TEST_NUM_MODULE;
static uint32_t             s_ipc_test_num_instances = ZOE_IPC_MAX_INST;

static void agent_thread_proc(void *);
static void wait_for_all(p_ipc_test_thread_info_t *params);

#ifdef ZOE_WINKER_BUILD
#include <wdm.h>
#define IPC_TEST_PRINTF DbgPrint
#else //!ZOE_WINKER_BUILD
#ifdef ZOE_LINUXKER_BUILD
#include <linux/kernel.h>
#include <linux/string.h>
#define IPC_TEST_PRINTF printk
#else //!ZOE_LINUXKER_BUILD
#define IPC_TEST_PRINTF printf
#endif //ZOE_LINUXKER_BUILD
#endif //ZOE_WINKER_BUILD

/////////////////////////////////////////////////////////////////////////////
//
//

// ipc interfaces
//

zoe_errs_t zoe_ipc_test_intf_x(void *pCnxt,
                               ZOE_IPC_CPU from_cpu,
                               PZOE_IPC_MSG p_msg
                               )
{
    zoe_errs_t      err;
    ZOE_IPC_MSG     resp;
    zoe_uintptr_t   context = (zoe_uintptr_t)pCnxt;
    uint32_t        intf = (uint32_t)(context >> 16);
    uint32_t        mod = (uint32_t)((context & 0xFFFF) >> 4);
    uint32_t        inst = (uint32_t)(context & 0xF);

//    IPC_TEST_PRINTF("intf_%d_%d_%d : msg(%d) intf(%d) mod(%d) inst(%d) (%d)args\r\n", 
//            intf, mod, inst, p_msg->hdr.msg.Reg.message, p_msg->hdr.msg.Reg.intf, p_msg->hdr.msg.Reg.module, p_msg->hdr.msg.Reg.inst, p_msg->hdr.status.Reg.msg_size);
    IPC_TEST_PRINTF("+%d", intf);

    if ((intf != ZOE_IPC_MSG_INTF_GET(p_msg->hdr.msg)) || 
        (mod != ZOE_IPC_MSG_MOD_GET(p_msg->hdr.msg)) ||
        (inst != ZOE_IPC_MSG_INST_GET(p_msg->hdr.msg))
        )
    {
        IPC_TEST_PRINTF("\r\nERR! =>intf_%d_%d_%d : msg(%d) intf(%d) mod(%d) inst(%d) (%d)args\r\n", 
                intf, mod, inst, ZOE_IPC_MSG_MSG_GET(p_msg->hdr.msg), ZOE_IPC_MSG_INTF_GET(p_msg->hdr.msg), ZOE_IPC_MSG_MOD_GET(p_msg->hdr.msg), ZOE_IPC_MSG_INST_GET(p_msg->hdr.msg), ZOE_IPC_STS_SIZE_GET(p_msg->hdr.status));
    }

    if (ZOE_IPC_STS_RSVP_GET(p_msg->hdr.status))
    {
        memset(&resp, 0, sizeof(resp));
        // prepare parameters
        if (ZOE_IPC_STS_SIZE_GET(p_msg->hdr.status))
        {
            memcpy(&resp.param[0], &p_msg->param[0], ZOE_IPC_STS_SIZE_GET(p_msg->hdr.status) << 2);
        }
        // prepare header
        ZOE_IPC_PREPARE_RESPONSE(&p_msg->hdr, &resp.hdr, ZOE_IPC_STS_SIZE_GET(p_msg->hdr.status));

        // post response
        err = c_zoe_ipc_service_post_message(s_p_zoe_ipc_srv, from_cpu, &resp, ZOE_NULL);
        if (ZOE_FAIL(err)) 
        {
            IPC_TEST_PRINTF("zoe_ipc_test_intf_x c_zoe_ipc_service_post_message failed (%d)\r\n", err);
        }
    }

    return (ZOE_ERRS_SUCCESS);
}


#define _IPC_TEST_RSVP  1
#define _IPC_CMP_RESULT 1
//#define IPC_TEST_ONESHOT


/////////////////////////////////////////////////////////////////////////////
//
//

void test_ipc(CZoeIPCService *p_zoe_ipc_srv,
              uint32_t id,
              ZOE_IPC_CPU to_cpu
              )
{
    uint32_t    i, j;
    zoe_errs_t  err;
    ZOE_IPC_MSG msg;
    ZOE_IPC_MSG resp;
    uint32_t    cmd;
    uint32_t    msg_size;

    IPC_TEST_PRINTF("\r\n.%d\r\n",id);

    if (p_zoe_ipc_srv)
    {
        memset(&msg, 0, sizeof(msg));

#ifndef IPC_TEST_ONESHOT
        for (i = 0; i < s_ipc_test_num_modules; i++)
        {
            for (j = 0; j < s_ipc_test_num_instances; j++)
            {
#else //IPC_TEST_ONESHOT
                i = j = 0;
#endif //!IPC_TEST_ONESHOT
                memset(&resp, 0, sizeof(resp));

                msg_size = (i + j + 1) % (ZOE_IPC_MSG_PARAM_SIZE_WORD + 1);
                if (0 == msg_size)
                {
                    msg_size =  1;
                }
                cmd = id + i + j;
                // prepare parameter
                memset(&msg.param[0], 0x38 + i + j + id, msg_size << 2);
                // prepare header
                ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 
                                        cmd,// message
                                        id, // intf
                                        i,  // module
                                        j,  // inst
                                        _IPC_TEST_RSVP,  // rsvp
                                        msg_size   // msg_size
                                        );
                // send message
                err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, _IPC_SEND_TIMEOUT/*-1*//*5000*/);
                if (ZOE_FAIL(err)) 
                {		    
                    IPC_TEST_PRINTF("test_ipc c_zoe_ipc_service_send_message failed (%d)\r\n", err);
                }
                else
                {
                    s_num_ipc_calls++;

#if (_IPC_TEST_RSVP & _IPC_CMP_RESULT)
                    if (ZOE_IPC_STS_SIZE_GET(resp.hdr.status) != msg_size)
                    {
                        IPC_TEST_PRINTF("test_ipc(%d:%d:%d) resp bad size(%d) expected(%d)!\r\n", id, i, j, ZOE_IPC_STS_SIZE_GET(resp.hdr.status), msg_size);
                        IPC_TEST_PRINTF("msg  msg(%04x) id(%d) cnxt(0x%p)\r\n", ZOE_IPC_MSG_MSG_GET(msg.hdr.msg), ZOE_IPC_STS_ID_GET(msg.hdr.status), msg.hdr.context.Reg.context);
                        IPC_TEST_PRINTF("resp msg(%04x) id(%d) cnxt(0x%p)\r\n", ZOE_IPC_MSG_MSG_GET(resp.hdr.msg), ZOE_IPC_STS_ID_GET(resp.hdr.status), resp.hdr.context.Reg.context);
                        IPC_TEST_PRINTF("mag  param(%08x)\r\n", msg.param[0]);
                        IPC_TEST_PRINTF("resp param(%08x)\r\n", resp.param[0]);
                    }
                    else
                    {
                        if (memcmp(&msg.param[0], &resp.param[0], msg_size << 2))
                        {
                            IPC_TEST_PRINTF("test_ipc(%d:%d:%d) bad response(0x%x)!\r\n", id, i, j, resp.param[0]);
                            IPC_TEST_PRINTF("msg  msg(%04x) id(%d) cnxt(0x%p)\r\n", ZOE_IPC_MSG_MSG_GET(msg.hdr.msg), ZOE_IPC_STS_ID_GET(msg.hdr.status), msg.hdr.context.Reg.context);
                            IPC_TEST_PRINTF("resp msg(%04x) id(%d) cnxt(0x%p)\r\n", ZOE_IPC_MSG_MSG_GET(resp.hdr.msg), ZOE_IPC_STS_ID_GET(resp.hdr.status), resp.hdr.context.Reg.context);
                        }
                    }
#endif //_IPC_TEST_RSVP & _IPC_CMP_RESULT
                }
#ifndef IPC_TEST_ONESHOT
            }
        }
#endif //!IPC_TEST_ONESHOT
    }
}



void zoe_ipc_test_register_srv(CZoeIPCService *p_zoe_ipc_srv, 
                               zoe_bool_t use_thread
                               )
{
    uint32_t        i, j, k;
    zoe_errs_t      err;
    zoe_uintptr_t   context;

    if (s_ipc_test_registered)
    {
        IPC_TEST_PRINTF("registered already");
        return;
    }

    if (p_zoe_ipc_srv)
    {
        if (use_thread)
        {
            s_ipc_test_num_modules = 4;
            s_ipc_test_num_instances = 4;
        }
        else
        {
            s_ipc_test_num_modules = IPC_TEST_NUM_MODULE;
            s_ipc_test_num_instances = ZOE_IPC_MAX_INST;
        }

        // set the registered to true
        s_ipc_test_registered = ZOE_TRUE;

        // save the ipc service for the test interface
        s_p_zoe_ipc_srv = p_zoe_ipc_srv;

        // register interfaces
        for (i = 0; i < AGENTS_NUM; i++)
        {
            for (j = 0; j < s_ipc_test_num_modules; j++)
            {
                for (k = 0; k < s_ipc_test_num_instances; k++)
                {
                    context = (i << 16) | (j << 4) | k;
                    err = c_zoe_ipc_service_register_interface(p_zoe_ipc_srv, 
                                                               i,       // intf
                                                               j,       // module
                                                               k,       // inst
                                                               (void *)context, // context
                                                               zoe_ipc_test_intf_x,  // dispatch function
                                                               use_thread,
                                                               0
                                                               );
                    if (ZOE_FAIL(err)) 
                    {
                        IPC_TEST_PRINTF("c_zoe_ipc_service_register_interface(%d:%d:%d) failed (%d)\n", i, j, k, err);
                    }
                    else
                    {
                        ////IPC_TEST_PRINTF("c_zoe_ipc_service_register_interface(%d:%d:%d) succeed\r\n", i, j, k);
                    }
                }
            }
        }
    }
}




void zoe_ipc_test_unregister_srv(CZoeIPCService *p_zoe_ipc_srv)
{
    uint32_t    i, j, k;
    zoe_errs_t  err;

    if (!s_ipc_test_registered)
    {
        IPC_TEST_PRINTF("not registered");
        return;
    }

    s_ipc_test_registered = ZOE_FALSE;

    // unregister interfaces
    for (i = 0; i < AGENTS_NUM; i++)
    {
        for (j = 0; j < s_ipc_test_num_modules; j++)
        {
            for (k = 0; k < s_ipc_test_num_instances; k++)
            {
                err = c_zoe_ipc_service_unregister_interface(p_zoe_ipc_srv, 
                                                             i, // intf
                                                             j, // module
                                                             k  // inst
                                                             );
                if (ZOE_FAIL(err)) 
                {
                    IPC_TEST_PRINTF("c_zoe_ipc_service_unregister_interface(%d:%d:%d) failed (%d)\r\n", i, j, k, err);
                }
            }
        }
    }
}


typedef struct _IPC_TEST_THREAD_PARAM
{
    CZoeIPCService  *p_zoe_ipc_srv;
    ZOE_IPC_CPU     to_cpu;
    int             iteration;
    int             seconds;
} IPC_TEST_THREAD_PARAM, *PIPC_TEST_THREAD_PARAM;


static void ipc_test_thread_proc(void * parm)
{
    PIPC_TEST_THREAD_PARAM      p_info = (PIPC_TEST_THREAD_PARAM)parm;
    int                         i, n;
    zoe_sosal_thread_parms_t    thr_parms;
    uint32_t                    max_pri;
    zoe_errs_t                  err = ZOE_ERRS_SUCCESS;
    zoe_sosal_ticks_t           ticks = 0;
    CZoeIPCService              *p_zoe_ipc_srv = p_info->p_zoe_ipc_srv;
    ZOE_IPC_CPU                 to_cpu = p_info->to_cpu;
    int                         iteration = p_info->iteration;
    int                         seconds = p_info->seconds;
    p_ipc_test_thread_info_t    p_agent_thread_param[AGENTS_NUM];

    for (i = 0; i < AGENTS_NUM; i++)
    {
        p_agent_thread_param[i] = ZOE_NULL;
    }

    for (n = 0; n < iteration; n++)
    {
        IPC_TEST_PRINTF("\r\n   ipc_test (%d) \r\n", n + 1);

        if (p_zoe_ipc_srv)
        {
            // reset counter
            s_num_ipc_calls = 0;

            // setup thread parameters
            max_pri = zoe_sosal_thread_maxpriorities_get();

            thr_parms.proc = agent_thread_proc;
            thr_parms.stack = 8192;
            thr_parms.priority = max_pri / 2;
//          thr_parms.priority = max_pri - 1;
            thr_parms.affinity.policy = ZOE_SOSAL_AFFINITY_POLICY_OS;
            thr_parms.affinity.core = 0;
            thr_parms.affinity.vpe = 0;

            for (i = 0; i < AGENTS_NUM; i++) 
            {
                p_agent_thread_param[i] = zoe_sosal_memory_local_alloc(sizeof(ipc_test_thread_info_t));
                if (!p_agent_thread_param[i])
                {
                    IPC_TEST_PRINTF("zoe_sosal_memory_local_alloc failed (%d) for thread %s\r\n", err, s_agent_threads[i].p_name);
                    goto zoe_ipc_test_exit;
                }
                p_agent_thread_param[i]->thread_id = ZOE_NULL;
                p_agent_thread_param[i]->event_id = ZOE_NULL;
                p_agent_thread_param[i]->to_cpu = to_cpu;
                p_agent_thread_param[i]->p_zoe_ipc_srv = p_zoe_ipc_srv;
                p_agent_thread_param[i]->id = s_agent_threads[i].id;
                p_agent_thread_param[i]->wait_ms = s_agent_threads[i].wait_ms;
                p_agent_thread_param[i]->func = s_agent_threads[i].func;
                // set test running flag
                p_agent_thread_param[i]->test_running = ZOE_TRUE;

                err = zoe_sosal_event_create(s_agent_threads[i].p_name, &p_agent_thread_param[i]->event_id);
                if (ZOE_FAIL(err)) 
                {
                    IPC_TEST_PRINTF("zoe_sosal_event_create failed (%d) for thread %s\r\n", err, s_agent_threads[i].p_name);
                    goto zoe_ipc_test_exit;
                } 
            }

            for (i = 0; i < AGENTS_NUM; i++) 
            {
                thr_parms.parm = p_agent_thread_param[i];

                err = zoe_sosal_thread_create(&thr_parms, s_agent_threads[i].p_name, &p_agent_thread_param[i]->thread_id);
                if (ZOE_FAIL(err)) 
                {
                    IPC_TEST_PRINTF("zoe_sosal_thread_create failed (%d) for thread %s\r\n", err, s_agent_threads[i].p_name);
                    goto zoe_ipc_test_exit;
                }
            }

            ticks = zoe_sosal_time_sys_ticks();
            IPC_TEST_PRINTF("zoe_ipc_test: started, sys ticks %llu\r\n", ticks);

            // wait for all threads to be alive and running
            wait_for_all(p_agent_thread_param);

            // wait for a while before terminating the test
            zoe_sosal_thread_sleep_ms(seconds * 1000);

            // signal the end of the test
            for (i = 0; i < AGENTS_NUM; i++) 
            {
                p_agent_thread_param[i]->test_running = ZOE_FALSE;
            }

            // wait for all threads to be done
            wait_for_all(p_agent_thread_param);

            // nil the thread id
            for (i = 0; i < AGENTS_NUM; i++) 
            {
                p_agent_thread_param[i]->thread_id = ZOE_NULL;
            }

            //zoe_sosal_thread_sleep_ms(500);
        }

zoe_ipc_test_exit:

        for (i = 0; i < AGENTS_NUM; i++) 
        {
            if (p_agent_thread_param[i])
            {
                if (p_agent_thread_param[i]->thread_id != ZOE_NULL) 
                {
                    err = zoe_sosal_thread_delete(p_agent_thread_param[i]->thread_id);
                    if (ZOE_FAIL(err)) 
                    {
                        if (ZOE_ERRS_INVALID == err) 
                        {
                            IPC_TEST_PRINTF("zoe_sosal_thread_delete(%s) already deleted\r\n", s_agent_threads[i].p_name);
                        }
                        else
                        {
                            IPC_TEST_PRINTF("zoe_sosal_thread_delete(%s) failed (%d)\r\n", s_agent_threads[i].p_name,err);
                        }
                    }
                    p_agent_thread_param[i]->thread_id = ZOE_NULL;
                }

                if (p_agent_thread_param[i]->event_id != ZOE_NULL) 
                {
                    err = zoe_sosal_event_delete(p_agent_thread_param[i]->event_id);
                    if (ZOE_FAIL(err)) 
                    {
                        IPC_TEST_PRINTF("zoe_sosal_event_delete failed (%d) for thread %s\r\n", err, s_agent_threads[i].p_name);
                    }
                    p_agent_thread_param[i]->event_id = ZOE_NULL;
                }

                zoe_sosal_memory_free(p_agent_thread_param[i]);
                p_agent_thread_param[i] = ZOE_NULL;
            }
        }

        ticks = zoe_sosal_time_sys_ticks() - ticks;
        IPC_TEST_PRINTF("zoe_ipc_test: exiting, total sys ticks %llu, ticks per second(%llu) calls(%d)\r\n", ticks, zoe_sosal_time_ticks_per_second(), s_num_ipc_calls);

        if (p_zoe_ipc_srv)
        {
            c_zoe_ipc_service_debug_queues(p_zoe_ipc_srv, to_cpu);
        }
    }

    zoe_sosal_memory_free(parm);

    return;
}



void zoe_ipc_test(CZoeIPCService *p_zoe_ipc_srv, 
                  ZOE_IPC_CPU to_cpu,
                  int iteration,
                  int seconds
                  )
{
    zoe_sosal_thread_parms_t    thr_parms;
    uint32_t                    max_pri;
    PIPC_TEST_THREAD_PARAM      p_info = (PIPC_TEST_THREAD_PARAM)zoe_sosal_memory_local_alloc(sizeof(IPC_TEST_THREAD_PARAM));
    zoe_sosal_obj_id_t          thread_id;
    zoe_errs_t                  err;

    if (p_info)
    {
        p_info->p_zoe_ipc_srv = p_zoe_ipc_srv;
        p_info->to_cpu = to_cpu;
        p_info->iteration = iteration;
        p_info->seconds = seconds;

        max_pri = zoe_sosal_thread_maxpriorities_get();

        thr_parms.proc = ipc_test_thread_proc;
        thr_parms.stack = 8192;
        thr_parms.priority = max_pri / 2;
        thr_parms.affinity.policy = ZOE_SOSAL_AFFINITY_POLICY_OS;
        thr_parms.affinity.core = 0;
        thr_parms.affinity.vpe = 0;
        thr_parms.parm = p_info;

        err = zoe_sosal_thread_create(&thr_parms, "ipc_test", &thread_id);
        if (ZOE_FAIL(err)) 
        {
            IPC_TEST_PRINTF("zoe_sosal_thread_create failed (%d) for ipc_test\r\n", err);
            zoe_sosal_memory_free(p_info);
            return;
        }
    }
}



static void agent_thread_proc(void * parm)
{
    p_ipc_test_thread_info_t    p_info = (p_ipc_test_thread_info_t)parm;
    zoe_errs_t                  err;

    // notify the main thread that we are ready to go
    //
    err = zoe_sosal_event_set(p_info->event_id);
    if (ZOE_FAIL(err)) 
    {
        IPC_TEST_PRINTF("zoe_sosal_event_set failed (%d) (%s)\r\n", err, s_agent_threads[p_info->id].p_name);
    }

    // enter the test loop
    while (p_info->test_running)
    {
        (*p_info->func)(p_info->p_zoe_ipc_srv, p_info->id, p_info->to_cpu);

#ifndef _IPC_TEST_DONT_SLEEP
        if (p_info->wait_ms > 0)
        {
            zoe_sosal_thread_sleep_ms(p_info->wait_ms);
        }
#endif //!_IPC_TEST_DONT_SLEEP
    }

    IPC_TEST_PRINTF("\r\n-%d\r\n", p_info->id);

    // notify the main thread that we are done
    //
    err = zoe_sosal_event_set(p_info->event_id);
    if (ZOE_FAIL(err)) 
    {
        IPC_TEST_PRINTF("zoe_sosal_event_set(done) failed (%d) (%s)\r\n", err, s_agent_threads[p_info->id].p_name);
    }

    return;
}



// wait for all threads to signal their event (multi event wait, all)
//
static void wait_for_all(p_ipc_test_thread_info_t *params)
{
    zoe_errs_t              err;
    int                     i;    

#if 0
    for (i = 0 ; i < AGENTS_NUM; i++) 
    {
        if (!params[i])
        {
            return;
        }
    }
    for (i = 0 ; i < AGENTS_NUM; i++) 
    {
        err = zoe_sosal_event_wait(params[i]->event_id, -1);
        if (ZOE_FAIL(err)) 
        {
            IPC_TEST_PRINTF("zoe_sosal_event_wait(%d) failed (%d)\r\n", i, err);
        }
    }
#else
    zoe_sosal_event_wait_t  waiter[AGENTS_NUM];

    for (i = 0 ; i < AGENTS_NUM; i++) 
    {
        if (!params[i])
        {
            return;
        }
    }

    for (i = 0 ; i < AGENTS_NUM; i++) 
    {
        waiter[i].event_id = params[i]->event_id;
        waiter[i].fired = ZOE_FALSE;
    }
    err = zoe_sosal_events_wait(waiter, AGENTS_NUM, -1, ZOE_TRUE);
    if (ZOE_FAIL(err)) 
    {
        IPC_TEST_PRINTF("sosal_synch_events_wait failed (%d)\r\n",err);
    }
    else
    {
        for (i = 0 ; i < AGENTS_NUM; i++) 
        {
            IPC_TEST_PRINTF("event(%d) (%s)\r\n", i, waiter[i].fired ? "FIRED" : "NOT FIRED");
        }
    }

#if 0
    // clear all the events anyway
    for (i = 0 ; i < AGENTS_NUM; i++) 
    {
        zoe_sosal_event_clear(params[i]->event_id);
    }
#endif

#endif
}

