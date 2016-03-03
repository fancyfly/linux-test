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
// zoe_cobject.c
//
// Description: 
//
//	Header file which defines the base object.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zoe_cobject.h"


static zoe_bool_t _c_object_init(c_object *This) 
{
	This->m_fInitialized = ZOE_TRUE; 
	return (ZOE_TRUE);
}



static zoe_bool_t _c_object_done(c_object *This) 
{
	This->m_fInitialized = ZOE_FALSE; 
	return (ZOE_TRUE);
}



c_object * c_object_constructor(c_object *pObject,
							    c_object *pParent,
                                uint32_t dwWhoAmI,                                                            
							    uint32_t dwAttributes
							    )
{
	if (pObject)
	{
		// fill in function table
		//
		pObject->init = _c_object_init;
		pObject->done = _c_object_done;

		pObject->m_pParent = pParent;
		pObject->m_fInitialized = ZOE_FALSE;
        pObject->m_dwWhoAmI = dwWhoAmI; 
		pObject->m_dwObjectAttributes = dwAttributes;

		INIT_CRITICAL(pObject)
	}

	return (pObject);
}


void c_object_destructor(c_object *This) 
{
	DONE_CRITICAL(This);
}



c_object * c_object_get_parent(c_object *This) 
{
	return (This->m_pParent);
}



zoe_bool_t c_object_is_initialized(c_object *This) 
{
	return (This->m_fInitialized);
}



uint32_t c_object_who_am_i(c_object *This)
{
    return (This->m_dwWhoAmI);
}



void c_object_enter_critical(c_object *This) 
{
	ENTER_CRITICAL(This)
}



void c_object_leave_critical(c_object *This) 
{
	LEAVE_CRITICAL(This)
}

