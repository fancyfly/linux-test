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
// zoe_cobject.h
//
// Description: 
//
//	Header file which defines the base object.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_COBJECT_H__
#define __ZOE_COBJECT_H__


#include "zoe_types.h"
#include "zoe_sosal.h"

/////////////////////////////////////////////////////////////////////////////
//
//

// this file only defines the base objects, users of the c_object class need to 
// define their own objects.
//
#define OBJECT_BASE				0x00000000L			// objects defined in the 
													// base classes start here
#define OBJECT_GENERIC			OBJECT_BASE			// generic object
#define OBJECT_FIFO				(OBJECT_BASE + 1)	// fifo
#define OBJECT_OBJECTMGR		(OBJECT_BASE + 2)	// object manager
#define OBJECT_QUEUE			(OBJECT_BASE + 3)	// queue
#define OBJECT_STACK			(OBJECT_BASE + 4)	// stack
#define OBJECT_DATA_FIFO	    (OBJECT_BASE + 5)	// data fifo
#define OBJECT_BUFFER_LIST	    (OBJECT_BASE + 6)	// buffer list

#define OBJECT_USER				0x0000FFFFL			// user defined objects

// the object attributes, crrently defined as either non-critical or critical.
//
#define OBJECT_DEFAULT			0x00000000L

#define OBJECT_CRITICAL_HEAVY	0x00000001L
#define OBJECT_CRITICAL_LIGHT	0x00000002L

#define OBJECT_CRITICAL			(OBJECT_CRITICAL_HEAVY | OBJECT_CRITICAL_LIGHT)


#ifndef ZOE_FIELD_OFFSET
#define ZOE_FIELD_OFFSET(type, field)   ((long)&(((type *)0)->field))
#endif // !ZOE_FIELD_OFFSET

#ifndef GET_INHERITED_OBJECT
#define GET_INHERITED_OBJECT(type, filed, pObject)  ((type *)((char *)pObject - ZOE_FIELD_OFFSET(type, filed)))
#endif // !GET_INHERITED_OBJECT


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/////////////////////////////////////////////////////////////////////////////
//
//

// C defination, visible to both C and C++ code
//

#ifndef __COBJECT_FWD_DEFINED__
#define __COBJECT_FWD_DEFINED__
typedef struct c_object c_object;
#endif // !__COBJECT_FWD_DEFINED__

struct c_object
{
    zoe_bool_t (*init)(c_object *This);
    zoe_bool_t (*done)(c_object *This);

    c_object			*m_pParent;				// who is my parent?
    zoe_bool_t		    m_fInitialized;			// am i initialized?
    uint32_t            m_dwWhoAmI;	            // who am i?
    uint32_t            m_dwObjectAttributes;	// object attributes

    zoe_sosal_obj_id_t	m_CriticalSection;
};


/////////////////////////////////////////////////////////////////////////////
//
//

//#define OBJECT_CS_USE_SEMAPHORE


#ifdef OBJECT_CS_USE_SEMAPHORE
#define INIT_CRITICAL(This) \
	(This)->m_CriticalSection = ZOE_NULL;\
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
        zoe_sosal_semaphore_parms_t sema_param;\
        sema_param.max_count = 1;\
        sema_param.init_count = 1;\
		zoe_sosal_semaphore_create(&sema_param, ZOE_NULL, &(This)->m_CriticalSection); \
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
        zoe_sosal_semaphore_parms_t sema_param;\
        sema_param.max_count = 1;\
        sema_param.init_count = 1;\
		zoe_sosal_semaphore_create(&sema_param, ZOE_NULL, &(This)->m_CriticalSection); \
	}\
	else \
	{\
		;\
	}

#define ENTER_CRITICAL(This) \
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_semaphore_take((This)->m_CriticalSection, -1);\
		}\
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_semaphore_take((This)->m_CriticalSection, -1);\
		}\
	}\
	else \
	{\
		;\
	}

#define LEAVE_CRITICAL(This) \
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_semaphore_give((This)->m_CriticalSection);\
		}\
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_semaphore_give((This)->m_CriticalSection);\
		}\
	}\
	else \
	{\
		;\
	}

#define DONE_CRITICAL(This) \
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_semaphore_delete((This)->m_CriticalSection);\
			(This)->m_CriticalSection = ZOE_NULL;\
		}\
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_semaphore_delete((This)->m_CriticalSection);\
			(This)->m_CriticalSection = ZOE_NULL;\
		}\
	}\
	else \
	{\
		;\
	}
#else // !OBJECT_CS_USE_SEMAPHORE
#define INIT_CRITICAL(This) \
	(This)->m_CriticalSection = ZOE_NULL;\
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		zoe_sosal_mutex_create(ZOE_NULL, &(This)->m_CriticalSection); \
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		zoe_sosal_mutex_create(ZOE_NULL, &(This)->m_CriticalSection); \
	}\
	else \
	{\
		;\
	}

#define ENTER_CRITICAL(This) \
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_mutex_get((This)->m_CriticalSection, -1);\
		}\
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_mutex_get((This)->m_CriticalSection, -1);\
		}\
	}\
	else \
	{\
		;\
	}

#define LEAVE_CRITICAL(This) \
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_mutex_release((This)->m_CriticalSection);\
		}\
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_mutex_release((This)->m_CriticalSection);\
		}\
	}\
	else \
	{\
		;\
	}

#define DONE_CRITICAL(This) \
	if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_mutex_delete((This)->m_CriticalSection);\
			(This)->m_CriticalSection = ZOE_NULL;\
		}\
	}\
	else if ((This)->m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
	{\
		if ((This)->m_CriticalSection) \
		{\
			zoe_sosal_mutex_delete((This)->m_CriticalSection);\
			(This)->m_CriticalSection = ZOE_NULL;\
		}\
	}\
	else \
	{\
		;\
	}
#endif //OBJECT_CS_USE_SEMAPHORE



/////////////////////////////////////////////////////////////////////////////
//
//

c_object * c_object_constructor(c_object *pObject,
                                c_object *pParent,
                                uint32_t dwWhoAmI,                              
                                uint32_t dwAttributes
							    );
void c_object_destructor(c_object *This);
#define c_object_init(This) \
    (This)->init(This)
#define c_object_done(This) \
    (This)->done(This)
c_object * c_object_get_parent(c_object *This);
zoe_bool_t c_object_is_initialized(c_object *This);
uint32_t c_object_who_am_i(c_object *This);

void c_object_enter_critical(c_object *This);
void c_object_leave_critical(c_object *This);

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

#define INIT_CRITICAL_CPP \
    m_CriticalSection = ZOE_NULL;\
    if (m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
    {\
        zoe_sosal_mutex_create(ZOE_NULL, &m_CriticalSection); \
    }\
    else if (m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
    {\
        zoe_sosal_mutex_create(ZOE_NULL, &m_CriticalSection); \
    }\
    else \
    {\
        ;\
    }\

#define ENTER_CRITICAL_CPP \
    if (m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
    {\
        if (m_CriticalSection) \
        {\
            zoe_sosal_mutex_get(m_CriticalSection, -1);\
        }\
    }\
    else if (m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
    {\
        if (m_CriticalSection) \
        {\
            zoe_sosal_mutex_get(m_CriticalSection, -1);\
        }\
    }\
    else \
    {\
        ;\
    }

#define LEAVE_CRITICAL_CPP \
    if (m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
    {\
        if (m_CriticalSection) \
        {\
            zoe_sosal_mutex_release(m_CriticalSection);\
        }\
    }\
    else if (m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
    {\
        if (m_CriticalSection) \
        {\
            zoe_sosal_mutex_release(m_CriticalSection);\
        }\
    }\
    else \
    {\
        ;\
    }

#define DONE_CRITICAL_CPP \
    if (m_dwObjectAttributes & OBJECT_CRITICAL_HEAVY) \
    {\
        if (m_CriticalSection) \
        {\
            zoe_sosal_mutex_delete(m_CriticalSection);\
            m_CriticalSection = ZOE_NULL; \
        }\
    }\
    else if (m_dwObjectAttributes & OBJECT_CRITICAL_LIGHT) \
    {\
        if (m_CriticalSection) \
        {\
            zoe_sosal_mutex_delete(m_CriticalSection);\
            m_CriticalSection = ZOE_NULL; \
        }\
    }\
    else \
    {\
        ;\
    }


/////////////////////////////////////////////////////////////////////////////
//
//


class cpp_object
{
public:
	cpp_object() 
	{
		m_pParent = ZOE_NULL; 
		m_fInitialized = ZOE_FALSE; 
		m_dwWhoAmI = OBJECT_GENERIC; 
		m_dwObjectAttributes = OBJECT_DEFAULT;
	}
	cpp_object(cpp_object *pParent, 
			   uint32_t dwWhoAmI,
			   uint32_t dwAttributes
			   )
	{
		m_pParent = pParent;
		m_fInitialized = ZOE_FALSE;
		m_dwWhoAmI = dwWhoAmI;
		m_dwObjectAttributes = dwAttributes;
		INIT_CRITICAL_CPP
	}
	virtual ~cpp_object() {DONE_CRITICAL_CPP}
	virtual zoe_bool_t init() {m_fInitialized = ZOE_TRUE; return (ZOE_TRUE);}
	virtual zoe_bool_t done() {m_fInitialized = ZOE_FALSE; return (ZOE_TRUE);}
	cpp_object * get_parent() {return (m_pParent);}
	zoe_bool_t is_initialized() {return (m_fInitialized);}
	uint32_t who_am_i() {return (m_dwWhoAmI);}

    void enter_critical() {ENTER_CRITICAL_CPP}
    void leave_critical() {LEAVE_CRITICAL_CPP}

protected:

    cpp_object          *m_pParent;             // who is my parent?
    zoe_bool_t          m_fInitialized;         // am i initialized?
    uint32_t            m_dwWhoAmI;	            // who am i?
    uint32_t            m_dwObjectAttributes;   // object attributes

    zoe_sosal_obj_id_t  m_CriticalSection;
};

#endif //__cplusplus

#endif //__ZOE_COBJECT_H__
