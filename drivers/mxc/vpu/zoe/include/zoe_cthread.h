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
// zoe_cthread.h
//
// Description: 
//
//	Header for thread definition.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_CTHREAD_H__
#define __ZOE_CTHREAD_H__


#include "zoe_types.h"
#include "zoe_sosal.h"
#include "zoe_dbg.h"
#include "zoe_cfifo.h"

enum ThreadEventsWait
{   
    THREAD_EVENT_EXIT = 0,
    THREAD_EVENT_NEW_CMD,
    THREAD_EVENT_WAIT_END
};

#define NUM_THREAD_EVENT_WAIT       THREAD_EVENT_WAIT_END
#define MAX_NUM_THREAD_EVENT_WAIT_DERIVED   32
#define MAX_NUM_THREAD_EVENT_WAIT   (NUM_THREAD_EVENT_WAIT + MAX_NUM_THREAD_EVENT_WAIT_DERIVED)

enum ThreadEventsReply
{   
    THREAD_EVENT_EXIT_DONE = 0,
    THREAD_EVENTS_REPLY_END
};

#define NUM_THREAD_EVENT_REPLY      THREAD_EVENTS_REPLY_END


// thread command structure
//
typedef struct _THREAD_CMD
{
	zoe_void_ptr_t			pContext;   // cmd context
	uint32_t	            dwCmdCode;	// command code
    uint32_t                dwParam[4]; // cmd word parameters
	zoe_void_ptr_t			pParam[4];	// cmd pointer parameters
	zoe_sosal_obj_id_t      evtCmdAck;  // command acknowledge event
    zoe_bool_t              fBlocking;  // blocking call
	zoe_errs_t			    *pdwError;	// error code
} THREAD_CMD, *PTHREAD_CMD;


#define THREAD_CMD_FIFO_DEPTH   24

/////////////////////////////////////////////////////////////////////////////
//
//

// C defination, visible to both C and C++ code
//

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#ifndef __CTHREAD_FWD_DEFINED__
#define __CTHREAD_FWD_DEFINED__
typedef struct c_thread c_thread;
#endif // !__CTHREAD_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

struct c_thread
{
    // thread function
    //
    int (*m_thread_proc)(c_thread *This);

    // handle thread command
    //
	void (*m_do_command)(c_thread *This, THREAD_CMD *pCmd);

    // thread events
    //
	void (*m_init_events)(c_thread *This, zoe_sosal_event_wait_t *pWait);
	void (*m_handle_events)(c_thread *This, zoe_sosal_event_wait_t *pWait);

    // handle thread timeout
    //
	void (*m_handle_timeout)(c_thread *This);

    zoe_sosal_obj_id_t  m_threadID;

    zoe_void_ptr_t      m_context;
    uint32_t            m_priority;
    uint32_t            m_stackWords;         // size of stack in 32-bit words
    zoe_sosal_obj_id_t  m_EvtWait[NUM_THREAD_EVENT_WAIT];
    zoe_sosal_obj_id_t  m_EvtReply[NUM_THREAD_EVENT_REPLY];
    uint32_t            m_NbEventWaitDerived;
    int32_t             m_TimeoutUS;
    zoe_bool_t          m_bRemoveUserSpaceMapping;

	// thread commands
	//
	c_fifo	            *m_pCmdFifo;						// thread command fifo

    char                m_szThreadName[ZOE_SOSAL_OBJNAME_MAXLEN + 1];
    zoe_dbg_comp_id_t   m_dbgID;
};


/////////////////////////////////////////////////////////////////////////////
//
//

c_thread * c_thread_constructor(c_thread *pThread,
                                char *Name,
                                zoe_void_ptr_t context,
                                uint32_t priority,
                                uint32_t stackWords,
                                uint32_t NbEventWaitDerived,
                                int32_t TimeoutUS,
                                zoe_bool_t bRemoveUserSpaceMapping,
                                zoe_dbg_comp_id_t dbgID
                                );
void c_thread_destructor(c_thread *This);
// Thread creation and clean up
//
zoe_bool_t c_thread_thread_init(c_thread *This);
zoe_bool_t c_thread_thread_done(c_thread *This);
// thread command interface
//
zoe_errs_t c_thread_set_command(c_thread *This, 
                                THREAD_CMD *pCmd
                                );
// this is called from the thread_proc
zoe_bool_t c_thread_process_command(c_thread *This);
// helper
//
zoe_bool_t c_thread_is_thread_inited(c_thread *This);


#ifdef __cplusplus
}
#endif //__cplusplus


/////////////////////////////////////////////////////////////////////////////
//
//

// C++ defination, visible only to C++ code
//

#ifdef __cplusplus


/////////////////////////////////////////////////////////////////////////////
//
//

// hide the template syntax
//
typedef cpp_fifo<THREAD_CMD, THREAD_CMD_FIFO_DEPTH + 1> cpp_thread_cmd_fifo;


/////////////////////////////////////////////////////////////////////////////
//
//
class cpp_thread
{
public :
	cpp_thread(char *Name,
			   uint32_t priority,
               uint32_t stackWords,
               uint32_t NbEventWaitDerived,
               int32_t TimeoutUS,
               zoe_dbg_comp_id_t dbgID
			   )
		: m_threadID(ZOE_NULL)
		, m_priority(priority)
        , m_stackWords(stackWords)
        , m_NbEventWaitDerived(NbEventWaitDerived)
        , m_TimeoutUS(TimeoutUS)
		, m_pCmdFifo(ZOE_NULL)
        , m_dbgID(dbgID)
	{
        for (uint32_t i=0; i < sizeof(m_szThreadName); i++)
        {
            m_szThreadName[i] = Name[i];
            if ('\0' == Name[i])
            {
                break;
            }
        }
        m_szThreadName[sizeof(m_szThreadName) - 1] = '\0';
	}

	virtual ~cpp_thread()
	{
	    // make sure the thread is destroyed
	    //
        thread_done();
	}

	// thread command interface
	//
	zoe_errs_t set_command(THREAD_CMD& Cmd)
	{
		if (!m_pCmdFifo || 
			!m_threadID
			)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           m_dbgID,                           
						   "cpp_thread::set_command() NO THREAD YET!!!\n"
						   );
			return (ZOE_ERRS_INVALID);
		}

		// clear command error
		//
        if (Cmd.pdwError)
        {
		    *Cmd.pdwError = ZOE_ERRS_SUCCESS;
        }

		// clear command ack event
		//
		if (Cmd.evtCmdAck)
		{
			zoe_sosal_event_clear(Cmd.evtCmdAck);
		}
		
		// add the command to the thread command fifo
		//
		if (m_pCmdFifo->set_fifo(Cmd))
        {
            zoe_errs_t  err = ZOE_ERRS_SUCCESS;

		    // signal new command
		    //
		    zoe_sosal_event_set(m_EvtWait[THREAD_EVENT_NEW_CMD]);

		    // wait on command ack
		    //
		    if (Cmd.fBlocking && 
                Cmd.evtCmdAck
                )
		    {
				zoe_errs_t  waitErr;

				waitErr = zoe_sosal_event_wait(Cmd.evtCmdAck,
									           2000000
										       );
				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               m_dbgID,
							   "cpp_thread::set_command() waiting for cmd(0x%08lx) Status(%d)\n", 
							   Cmd.dwCmdCode,
							   waitErr
							   );

                if (Cmd.pdwError)
                {
                    err = ZOE_SUCCESS(waitErr) ? *Cmd.pdwError : waitErr;
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
			zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           m_dbgID,                           
						   "cpp_thread::set_command() m_pCmdFifo->set_fifo() Failed!!!\n"
						   );
			return (ZOE_ERRS_NOMEMORY);
		}
	}

	inline uint32_t get_thread_priority(void) {return (m_priority);}

protected:

	// Thread creation and clean up
	//
	virtual zoe_bool_t thread_init(void)
	{
		zoe_errs_t	err = ZOE_ERRS_SUCCESS;

		if (!m_threadID)
		{
			// create command fifo
			//
			m_pCmdFifo = new cpp_thread_cmd_fifo(ZOE_NULL, 
										         OBJECT_FIFO, 
										         OBJECT_CRITICAL_HEAVY
										         );
			if (!m_pCmdFifo)
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               m_dbgID,
							   "cpp_thread::thread_init() - Unable to create cpp_thread_cmd_fifo!\n"
							   );
				return (ZOE_FALSE);
			}

            // create build in synchronization objects
            //
            err = zoe_sosal_event_create(ZOE_NULL, &m_EvtWait[THREAD_EVENT_EXIT]);
            err = ZOE_FAIL(err) ? err : zoe_sosal_event_create(ZOE_NULL, &m_EvtWait[THREAD_EVENT_NEW_CMD]);
            err = ZOE_FAIL(err) ? err : zoe_sosal_event_create(ZOE_NULL, &m_EvtReply[THREAD_EVENT_EXIT_DONE]);

			// create thread
			//
			zoe_sosal_thread_parms_t    threadParam;
            zoe_sosal_obj_name_t        name;

			threadParam.proc = cpp_thread::static_thread_proc;
			threadParam.parm = (void *)this;
			threadParam.stack = m_stackWords;
			threadParam.priority = m_priority;
            threadParam.affinity.policy = ZOE_SOSAL_AFFINITY_POLICY_OS;
            threadParam.affinity.core = 0;
            threadParam.affinity.vpe = 0;
            for (uint32_t i=0; i < ZOE_SOSAL_OBJNAME_MAXLEN; i++)
            {
                name[i] = m_szThreadName[i];
                if ('\0' == m_szThreadName[i])
                {
                    break;
                }
            }
            name[ZOE_SOSAL_OBJNAME_MAXLEN - 1] = '\0';
			err = ZOE_FAIL(err) ? err : zoe_sosal_thread_create(&threadParam,
                                                                name,
                                                                &m_threadID
										                        );
		}

		return (ZOE_SUCCESS(err));
	}

	virtual zoe_bool_t thread_done()
	{
		zoe_errs_t	err;

		// terminate thread
		//
		if (m_threadID)
		{
			// Flush all the outstanding commands
			//
			flush_commands();

			// signal the thread that we are done
			//
			thread_exit();

			// call sosal delete thread function
			//
			err = zoe_sosal_thread_delete(m_threadID);

			if (!ZOE_SUCCESS(err))
			{
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               m_dbgID,
							   "cpp_thread::thread_done() zoe_sosal_thread_delete(0x%x) failed status(%d)!!!\n", 
                               m_threadID,
							   err
							   );
			}

			// nil the thread id regardless
			m_threadID = ZOE_NULL;
		}

		// destroy the thread command fifo
		//
		if (m_pCmdFifo)
		{
			delete m_pCmdFifo;
			m_pCmdFifo = ZOE_NULL;
		}

        // delete build in synchronization objects
        //
        if (m_EvtWait[THREAD_EVENT_EXIT])
        {
            zoe_sosal_event_delete(m_EvtWait[THREAD_EVENT_EXIT]);
            m_EvtWait[THREAD_EVENT_EXIT] = ZOE_NULL;
        }

        if (m_EvtWait[THREAD_EVENT_NEW_CMD])
        {
            zoe_sosal_event_delete(m_EvtWait[THREAD_EVENT_NEW_CMD]);
            m_EvtWait[THREAD_EVENT_NEW_CMD] = ZOE_NULL;
        }

        if (m_EvtReply[THREAD_EVENT_EXIT_DONE])
        {
            zoe_sosal_event_delete(m_EvtReply[THREAD_EVENT_EXIT_DONE]);
            m_EvtReply[THREAD_EVENT_EXIT_DONE] = ZOE_NULL;
        }

		return (ZOE_TRUE);
	}

	virtual zoe_bool_t thread_exit()
	{
		zoe_errs_t	err;

		zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       m_dbgID,
					   "cpp_thread::thread_exit() 0x%x\n",
                       m_threadID
					   );

		zoe_sosal_event_clear(m_EvtReply[THREAD_EVENT_EXIT_DONE]);
        zoe_sosal_event_set(m_EvtWait[THREAD_EVENT_EXIT]);
		err = zoe_sosal_event_wait(m_EvtReply[THREAD_EVENT_EXIT_DONE], 
								   1500000
								   );
		zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       m_dbgID,
					   "cpp_thread::thread_exit() Status(%d)\n", 
					   err
					   );

		return (ZOE_SUCCESS(err));
	}

	// thread command related functions
	//
	virtual zoe_bool_t process_command()
	{
		THREAD_CMD  Cmd;

		if (!m_threadID)
			return (ZOE_FALSE);

		// get and process command
		//
		if (m_pCmdFifo->get_fifo(Cmd))
		{
            // handle the command
			do_command(Cmd);

			// signal caller we have processed the command
		    if (Cmd.evtCmdAck)
			{
				zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               m_dbgID,
							   "cpp_thread::process_command() -- Signal Caller --\n"
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

	void flush_commands()
	{
		while (process_command());
	}

	virtual void do_command(THREAD_CMD& Cmd)
	{
        if (Cmd.pdwError)
        {
		    *Cmd.pdwError = ZOE_ERRS_SUCCESS;
        }
	}

    virtual void init_events(zoe_sosal_event_wait_t *)
    {
    }

    virtual void handle_events(zoe_sosal_event_wait_t *)
    {
    }

	virtual void handle_timeout()
    {
    }

	// thread proc
	//
	virtual int thread_proc()
    {
        zoe_sosal_event_wait_t    wait[MAX_NUM_THREAD_EVENT_WAIT];

		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       m_dbgID,                           
					   "cpp_thread::thread_proc() Enter\n"
					   );
        wait[THREAD_EVENT_EXIT].event_id = m_EvtWait[THREAD_EVENT_EXIT];
        wait[THREAD_EVENT_EXIT].fired = ZOE_FALSE;

        wait[THREAD_EVENT_NEW_CMD].event_id = m_EvtWait[THREAD_EVENT_NEW_CMD];
        wait[THREAD_EVENT_NEW_CMD].fired = ZOE_FALSE;

        // init derived events
        //
        init_events(&wait[THREAD_EVENT_WAIT_END]);

	    while (ZOE_TRUE)
	    {
            zoe_errs_t  err;

		    // wait on the IPC's
		    //
            err = zoe_sosal_events_wait(wait, 
                                        NUM_THREAD_EVENT_WAIT + m_NbEventWaitDerived,
                                        m_TimeoutUS,
                                        ZOE_FALSE
                                        );
		    if (ZOE_ERRS_SUCCESS == err)
		    {
                // thread exit
                //
                if (wait[THREAD_EVENT_EXIT].fired)
                {
                    wait[THREAD_EVENT_EXIT].fired = ZOE_FALSE;
		            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   m_dbgID,                           
					               "cpp_thread::thread_proc() THREAD_EVENT_EXIT\n"
					               );
		            zoe_sosal_event_set(m_EvtReply[THREAD_EVENT_EXIT_DONE]);
                    return (0);
                }

                // new command
                //
                if (wait[THREAD_EVENT_NEW_CMD].fired)
                {
                    wait[THREAD_EVENT_NEW_CMD].fired = ZOE_FALSE;
		            zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                                   m_dbgID,                           
					               "cpp_thread::thread_proc() THREAD_EVENT_NEW_CMD\n"
					               );
				    while (process_command());
                }

                // handle derived events
                //
                handle_events(&wait[THREAD_EVENT_WAIT_END]);
            }
            else if (ZOE_ERRS_TIMEOUT == err)
            {
                handle_timeout();
            }
            else
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               m_dbgID,                           
				               "cpp_thread::thread_proc() unhandle wait status(%d)!!!\n",
                               err
				               );
                break;
            }
		}
        return (0);
    }

	static void static_thread_proc(void *Context) {((cpp_thread *)Context)->thread_proc();}


	// thread management
	//
	zoe_sosal_obj_id_t	m_threadID;

	uint32_t	        m_priority;
    uint32_t            m_stackWords;   // size of stack in 32-bit words
    zoe_sosal_obj_id_t  m_EvtWait[NUM_THREAD_EVENT_WAIT];
    zoe_sosal_obj_id_t  m_EvtReply[NUM_THREAD_EVENT_REPLY];
    uint32_t            m_NbEventWaitDerived;
    int32_t             m_TimeoutUS;

	// thread commands
	//
	cpp_thread_cmd_fifo *m_pCmdFifo;    // thread command fifo

    char                m_szThreadName[ZOE_SOSAL_OBJNAME_MAXLEN + 1];
    zoe_dbg_comp_id_t   m_dbgID;
};

#endif //__cplusplus

#endif //__ZOE_CTHREAD_H__




