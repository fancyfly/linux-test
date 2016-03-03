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
// zoe_cthread.c
//
// Description: 
//
//	Generic thread functions
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zoe_cthread.h"

#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#endif //!ZOE_LINUXKER_BUILD
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/version.h>
#endif //ZOE_LINUXKER_BUILD


// thread proc
//
static void c_thread_static_thread_proc(void * Context)
{
	c_thread    *pThread = (c_thread *)Context;

	if (pThread->m_bRemoveUserSpaceMapping)
	{
#ifdef ZOE_LINUXKER_BUILD
#ifdef __LINUX24__  
		sprintf(current->comm, "%s", pThread->m_szThreadName);
		daemonize();
#else //!__LINUX24__
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
		daemonize(pThread->m_szThreadName);
		current->flags |= PF_NOFREEZE;
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
#endif //__LINUX24__
		set_user_nice(current, -10);
#endif //ZOE_LINUXKER_BUILD
	}

    // thread function
    //
    pThread->m_thread_proc(pThread);
}



static zoe_bool_t c_thread_thread_exit(c_thread *This)
{
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_thread_thread_exit() 0x%x [\n",
                   This->m_threadID
				   );

	zoe_sosal_event_clear(This->m_EvtReply[THREAD_EVENT_EXIT_DONE]);
    zoe_sosal_event_set(This->m_EvtWait[THREAD_EVENT_EXIT]);
	err = zoe_sosal_event_wait(This->m_EvtReply[THREAD_EVENT_EXIT_DONE], 
							   2000000
							   );
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
				   "c_thread_thread_exit() Status(%d) ]\n", 
				   err
				   );

	return (ZOE_SUCCESS(err));
}



static zoe_bool_t c_thread_throw_away_command(c_thread *This)
{
	THREAD_CMD  Cmd;

	if (!This->m_threadID || 
        !This->m_pCmdFifo
        )
    {
		return (ZOE_FALSE);
    }

	// get and process command
	//
	if (c_fifo_get_fifo(This->m_pCmdFifo, 
                        &Cmd
                        ))
	{
		// signal caller we have processed the command
	    if (Cmd.evtCmdAck)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           This->m_dbgID,
						   "c_thread_throw_away_command() -- Signal Caller --\n"
						   );
			zoe_sosal_event_set(Cmd.evtCmdAck);
		}

		return (ZOE_TRUE);
	}
	else
	{
		return (ZOE_FALSE);
	}
}



static void c_thread_flush_commands(c_thread *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
	               "%s() cmd nb(%d)[\n",
                   __FUNCTION__,
                   This->m_pCmdFifo ? c_fifo_get_fifo_level(This->m_pCmdFifo) : 0
	               );
	while (c_thread_throw_away_command(This));
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
	               "%s() ]\n",
                   __FUNCTION__
	               );
}



static void c_thread_do_command(c_thread *This,
                                THREAD_CMD *pCmd
                                )
{
    if (pCmd->pdwError)
    {
	    *pCmd->pdwError = ZOE_ERRS_SUCCESS;
    }
}



static void c_thread_init_events(c_thread *This, 
                                 zoe_sosal_event_wait_t *pWait
                                 )
{
}



static void c_thread_handle_events(c_thread *This, 
                                   zoe_sosal_event_wait_t *pWait
                                   )
{
}



static void c_thread_handle_timeout(c_thread *This)
{
}



// thread proc
//
static int c_thread_thread_proc(c_thread *This)
{
    zoe_sosal_event_wait_t  wait[MAX_NUM_THREAD_EVENT_WAIT];

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   This->m_dbgID,                           
				   "c_thread_thread_proc() %s : Enter\n",
                   This->m_szThreadName
				   );
    wait[THREAD_EVENT_EXIT].event_id = This->m_EvtWait[THREAD_EVENT_EXIT];
    wait[THREAD_EVENT_EXIT].fired = ZOE_FALSE;

    wait[THREAD_EVENT_NEW_CMD].event_id = This->m_EvtWait[THREAD_EVENT_NEW_CMD];
    wait[THREAD_EVENT_NEW_CMD].fired = ZOE_FALSE;

    // init derived events
    //
    This->m_init_events(This,
                        &wait[THREAD_EVENT_WAIT_END]
                        );

    while (ZOE_TRUE)
    {
        zoe_errs_t  err;

	    // wait on the IPC's
	    //
        err = zoe_sosal_events_wait(wait, 
                                    NUM_THREAD_EVENT_WAIT + This->m_NbEventWaitDerived, 
                                    This->m_TimeoutUS,
                                    ZOE_FALSE
                                    );
	    if ((ZOE_ERRS_SUCCESS == err) ||
            (ZOE_ERRS_TIMEOUT == err)
            )
	    {
            // thread exit
            //
            if (wait[THREAD_EVENT_EXIT].fired)
            {
                wait[THREAD_EVENT_EXIT].fired = ZOE_FALSE;
	            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,                           
				               "c_thread_thread_proc() %s : THREAD_EVENT_EXIT\n",
                               This->m_szThreadName
				               );
	            zoe_sosal_event_set(This->m_EvtReply[THREAD_EVENT_EXIT_DONE]);
                return (0);
            }

            // new command
            //
            if (wait[THREAD_EVENT_NEW_CMD].fired)
            {
                wait[THREAD_EVENT_NEW_CMD].fired = ZOE_FALSE;
	            zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               This->m_dbgID,                           
				               "c_thread_thread_proc() THREAD_EVENT_NEW_CMD\n"
				               );
			    while (c_thread_process_command(This));
            }

            // handle derived events
            //
            This->m_handle_events(This, 
                                  &wait[THREAD_EVENT_WAIT_END]
                                  );

            // handle timeout
            //
            if (ZOE_ERRS_TIMEOUT == err)
            {
                This->m_handle_timeout(This);
            }
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,                           
			               "c_thread_thread_proc(%s) unhandle wait status(%d)!!!\n",
                           This->m_szThreadName,
                           err
			               );
            break;
        }
	}
    return (0);
}



c_thread * c_thread_constructor(c_thread *pThread,
                                char *Name,
                                zoe_void_ptr_t context,
                                uint32_t priority,
                                uint32_t stackWords,
                                uint32_t NbEventWaitDerived,
                                int32_t TimeoutUS,
                                zoe_bool_t bRemoveUserSpaceMapping,
                                zoe_dbg_comp_id_t dbgID
                                )
{
	if (pThread)
	{
		pThread->m_threadID = ZOE_NULL;

		pThread->m_context = context;
		pThread->m_priority = priority;
        pThread->m_stackWords = stackWords;
        pThread->m_NbEventWaitDerived = NbEventWaitDerived;
        pThread->m_TimeoutUS = TimeoutUS;
		pThread->m_bRemoveUserSpaceMapping = bRemoveUserSpaceMapping;

		strncpy(pThread->m_szThreadName, 
			    Name,
                sizeof(pThread->m_szThreadName)
			    );
        pThread->m_szThreadName[sizeof(pThread->m_szThreadName) - 1] = '\0';
        pThread->m_dbgID = dbgID;
        pThread->m_pCmdFifo = ZOE_NULL;

        pThread->m_thread_proc = c_thread_thread_proc;
		pThread->m_do_command = c_thread_do_command;
        pThread->m_init_events = c_thread_init_events;
        pThread->m_handle_events = c_thread_handle_events;
        pThread->m_handle_timeout = c_thread_handle_timeout;
	}
	return (pThread);
}



void c_thread_destructor(c_thread *This)
{
	// make sure the thread is destroyed
	//
	c_thread_thread_done(This);
}



// Thread creation and clean up
//
zoe_bool_t c_thread_thread_init(c_thread *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	if (!This->m_threadID)
	{
        zoe_sosal_obj_name_t        name;
		zoe_sosal_thread_parms_t    threadParam;
        c_fifo                       *pFifo;

		// create command fifo
		//
		pFifo = (c_fifo *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                sizeof(c_fifo), 
                                                0
                                                );
		if (pFifo)
		{
			This->m_pCmdFifo = c_fifo_constructor(pFifo,
                                                  ZOE_NULL,
												  OBJECT_CRITICAL_HEAVY,
												  THREAD_CMD_FIFO_DEPTH + 1,
											      sizeof(THREAD_CMD)
												  );
		}

		if (!This->m_pCmdFifo)
		{
            if (pFifo)
            {
			    zoe_sosal_memory_free((void *)pFifo);
            }

		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
					       "c_thread_thread_init() - Unable to create cmd fifo!\n"
					       );
		    return (ZOE_FALSE);
		}

        // create build in synchronization objects
        //
        err = zoe_sosal_event_create(ZOE_NULL, &This->m_EvtWait[THREAD_EVENT_EXIT]);
        err = ZOE_FAIL(err) ? err : zoe_sosal_event_create(ZOE_NULL, &This->m_EvtWait[THREAD_EVENT_NEW_CMD]);
        err = ZOE_FAIL(err) ? err : zoe_sosal_event_create(ZOE_NULL, &This->m_EvtReply[THREAD_EVENT_EXIT_DONE]);

		// create thread
		//
		threadParam.proc = c_thread_static_thread_proc;
		threadParam.parm = (void *)This;
		threadParam.stack = This->m_stackWords;
		threadParam.priority = This->m_priority;
        threadParam.affinity.policy = ZOE_SOSAL_AFFINITY_POLICY_OS;
        threadParam.affinity.core = 0;
        threadParam.affinity.vpe = 0;
        strncpy(name, This->m_szThreadName, ZOE_SOSAL_OBJNAME_MAXLEN);
		err = ZOE_FAIL(err) ? err : zoe_sosal_thread_create(&threadParam,
                                                            name,
                                                            &This->m_threadID
									                        );
	}

	return (ZOE_SUCCESS(err));
}



zoe_bool_t c_thread_thread_done(c_thread *This)
{
	zoe_errs_t	err;

	if (This->m_threadID)
	{
		// Flush all the outstanding commands
		//
		c_thread_flush_commands(This);

		// signal the thread that we are done
		//
		if (!c_thread_thread_exit(This))
        {
		    // call zoe_sosal delete thread function
		    //
		    err = zoe_sosal_thread_delete(This->m_threadID);

		    if (!ZOE_SUCCESS(err))
		    {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_thread_thread_done() zoe_sosal_thread_delete(0x%x) failed status(%d)!!!\n", 
                               This->m_threadID,
						       err
						       );
		    }
        }

		// nil the thread id regardless
		This->m_threadID = ZOE_NULL;
	}

	// destroy the thread command fifo
	//
	if (This->m_pCmdFifo)
	{
		c_fifo_destructor(This->m_pCmdFifo);
		zoe_sosal_memory_free((void *)This->m_pCmdFifo);
		This->m_pCmdFifo = ZOE_NULL;
	}

    // delete build in synchronization objects
    //
    if (This->m_EvtWait[THREAD_EVENT_EXIT])
    {
        zoe_sosal_event_delete(This->m_EvtWait[THREAD_EVENT_EXIT]);
        This->m_EvtWait[THREAD_EVENT_EXIT] = ZOE_NULL;
    }

    if (This->m_EvtWait[THREAD_EVENT_NEW_CMD])
    {
        zoe_sosal_event_delete(This->m_EvtWait[THREAD_EVENT_NEW_CMD]);
        This->m_EvtWait[THREAD_EVENT_NEW_CMD] = ZOE_NULL;
    }

    if (This->m_EvtReply[THREAD_EVENT_EXIT_DONE])
    {
        zoe_sosal_event_delete(This->m_EvtReply[THREAD_EVENT_EXIT_DONE]);
        This->m_EvtReply[THREAD_EVENT_EXIT_DONE] = ZOE_NULL;
    }

	// succeed it anyway
	//
	return (ZOE_TRUE);
}



// thread command interface
//
zoe_bool_t c_thread_process_command(c_thread *This)
{
	THREAD_CMD  Cmd;

	if (!This->m_threadID || 
        !This->m_pCmdFifo
        )
    {
		return (ZOE_FALSE);
    }

	// get and process command
	//
	if (c_fifo_get_fifo(This->m_pCmdFifo, 
                        &Cmd
                        ))
	{
        // handle the command
        //
		This->m_do_command(This, 
                           &Cmd
                           );

		// signal caller we have processed the command
	    if (Cmd.evtCmdAck)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           This->m_dbgID,
						   "c_thread_process_command() -- Signal Caller --\n"
						   );
			zoe_sosal_event_set(Cmd.evtCmdAck);
		}

		return (ZOE_TRUE);
	}
	else
	{
		return (ZOE_FALSE);
	}
}



zoe_errs_t c_thread_set_command(c_thread *This,
                                THREAD_CMD *pCmd
                                )
{
	if (!This->m_pCmdFifo || 
		!This->m_threadID
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
					   "c_thread_set_command() NO THREAD YET!!!\n"
					   );
		return (ZOE_ERRS_INVALID);
	}

	// clear command error
	//
    if (pCmd->pdwError)
    {
	    *pCmd->pdwError = ZOE_ERRS_SUCCESS;
    }

	// clear command ack event
	//
	if (pCmd->evtCmdAck)
	{
		zoe_sosal_event_clear(pCmd->evtCmdAck);
	}
	
	// add the command to the thread command fifo
	//
	if (c_fifo_set_fifo(This->m_pCmdFifo, 
                        pCmd
                        ))
    {
        zoe_errs_t  err = ZOE_ERRS_SUCCESS;

	    // signal new command
	    //
	    zoe_sosal_event_set(This->m_EvtWait[THREAD_EVENT_NEW_CMD]);

	    // wait on command ack
	    //
	    if (pCmd->fBlocking && 
            pCmd->evtCmdAck
            )
	    {
			zoe_errs_t  waitErr;

			waitErr = zoe_sosal_event_wait(pCmd->evtCmdAck,
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
								           12000000  // 12 seconds
#else // ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_DAWN
								           3000000  // 3 seconds
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN
									       );
            if (ZOE_FAIL(waitErr))
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
						       "c_thread_set_command() waiting for cmd(0x%08lx) Status(%d)\n", 
						       pCmd->dwCmdCode,
						       waitErr
						       );
            }

            if (pCmd->pdwError)
            {
                err = ZOE_SUCCESS(waitErr) ? *pCmd->pdwError : waitErr;
            }
            else
            {
                err = waitErr;
            }
	    }
        return (err);
    }	
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,                           
					   "c_thread_set_command() m_pCmdFifo->set_fifo() Failed!!!\n"
					   );
		return (ZOE_ERRS_NOMEMORY);
	}
}



// helper
//
zoe_bool_t c_thread_is_thread_inited(c_thread *This)
{
	return (ZOE_NULL != This->m_threadID);
}





