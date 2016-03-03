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
// zoe_cobjectmgr.c
//
// Description: 
//
//	the object that manages multiple objects.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zoe_cobjectmgr.h"


static void c_object_mgr_remove_objects(c_object_mgr *This)
{
	OBJECT_ENTRY	*pthis_entry;
	OBJECT_ENTRY	*ptemp_entry;

	pthis_entry = This->m_pHead;	

	while (pthis_entry)
	{
		ptemp_entry = pthis_entry;
		pthis_entry = pthis_entry->pNext;
		if (This->m_pFuncCallback && (ptemp_entry->pObject))
			This->m_pFuncCallback(ptemp_entry->pObject);
		zoe_sosal_memory_free(ptemp_entry);
	}
	This->m_dwObjectNb = 0;
	This->m_pHead = ZOE_NULL;
}



c_object_mgr * c_object_mgr_constructor(c_object_mgr *pObjectMgr,
									    c_object *pParent, 
									    uint32_t dwAttributes,
									    OBJECTMGR_CALLBACK pFuncCallback
									    )
{
	if (pObjectMgr)
	{
		c_object_constructor(&pObjectMgr->m_Object, 
							 pParent, 
                             OBJECT_OBJECTMGR,
							 dwAttributes
							 );
		pObjectMgr->m_hCurObject = 0;
		pObjectMgr->m_pHead = ZOE_NULL;
		pObjectMgr->m_dwObjectNb = 0;
		pObjectMgr->m_pFuncCallback = pFuncCallback;
	}

	return (pObjectMgr);
}



void c_object_mgr_destructor(c_object_mgr *This)
{
	// release all the objects
	//
	c_object_mgr_remove_objects(This);

	c_object_destructor(&This->m_Object);
}



ZOE_OBJECT_HANDLE c_object_mgr_add_object(c_object_mgr *This,
									      zoe_void_ptr_t pObject
									      )
{
	ZOE_OBJECT_HANDLE	hRet = 0;
	OBJECT_ENTRY		*pNewEntry = (OBJECT_ENTRY *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                                            sizeof(OBJECT_ENTRY), 
                                                                            sizeof(void *)
                                                                            );
	ENTER_CRITICAL(&This->m_Object)

	if (ZOE_NULL == This->m_pHead)
	{
		This->m_pHead = pNewEntry;
		This->m_pHead->hObject = 1;
		This->m_pHead->pObject = pObject;
		This->m_pHead->pNext = (OBJECT_ENTRY *)ZOE_NULL;
		This->m_dwObjectNb++;
		hRet = This->m_pHead->hObject;
	}
	else
	{		
		OBJECT_ENTRY	*ptemp_entry;
		
		if (This->m_pHead->hObject != 1)
		{
			ptemp_entry = pNewEntry;
			ptemp_entry->hObject = 1;
			ptemp_entry->pObject = pObject;
			ptemp_entry->pNext = This->m_pHead;
			This->m_pHead = ptemp_entry;
			This->m_dwObjectNb++;
			hRet = This->m_pHead->hObject;
		}
		else
		{
			OBJECT_ENTRY	*pthis_entry = This->m_pHead;
			OBJECT_ENTRY	*pnext_entry = This->m_pHead->pNext;
            
			while (pnext_entry && 
				   (pnext_entry->hObject == (pthis_entry->hObject + 1))
				   )
			{
				pthis_entry = pnext_entry;
				pnext_entry = pnext_entry->pNext;
			}
			ptemp_entry = pNewEntry;
			ptemp_entry->hObject = pthis_entry->hObject + 1;
			ptemp_entry->pObject = pObject;
			ptemp_entry->pNext = pnext_entry;
			pthis_entry->pNext = ptemp_entry;
			This->m_dwObjectNb++;
			hRet = ptemp_entry->hObject;
		}
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (hRet);
}



zoe_void_ptr_t c_object_mgr_remove_object(c_object_mgr *This,
								          ZOE_OBJECT_HANDLE hObject
								          )
{
	OBJECT_ENTRY	*pthis_entry;
	OBJECT_ENTRY	*pprev_entry;
	OBJECT_ENTRY	*pRemove = ZOE_NULL;
	zoe_void_ptr_t  pObject = ZOE_NULL;

	ENTER_CRITICAL(&This->m_Object)

	pthis_entry = This->m_pHead;
	if (pthis_entry)
	{
		if (pthis_entry->hObject == hObject)
		{
			pObject = pthis_entry->pObject;
			This->m_pHead = pthis_entry->pNext;
			pRemove = pthis_entry;
			This->m_dwObjectNb--;
		}
		else
		{
			do	{
				pprev_entry = pthis_entry;
				pthis_entry = pthis_entry->pNext;
				if (pthis_entry && (pthis_entry->hObject == hObject))
				{
					pprev_entry->pNext = pthis_entry->pNext;
					pObject = pthis_entry->pObject;
					pRemove = pthis_entry;
					This->m_dwObjectNb--;
					break;
				}		
			} while (pthis_entry);
		}
	}

	LEAVE_CRITICAL(&This->m_Object)

	if (pRemove)
    {
		zoe_sosal_memory_free(pRemove);
    }

	return (pObject);
}



zoe_void_ptr_t c_object_mgr_get_object_by_handle(c_object_mgr *This,
									             ZOE_OBJECT_HANDLE hObject
									             )
{
	OBJECT_ENTRY	*pthis_entry;
	zoe_void_ptr_t  pObject = ZOE_NULL;
	uint32_t    i;

	ENTER_CRITICAL(&This->m_Object)

	pthis_entry = This->m_pHead;	

	for (i = 0; i < This->m_dwObjectNb; i++)
	{
		if (pthis_entry->hObject == hObject)
		{
			pObject = pthis_entry->pObject;
		}
		pthis_entry = pthis_entry->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (pObject);
}



ZOE_OBJECT_HANDLE c_object_mgr_get_handle_by_object(c_object_mgr *This,
											        zoe_void_ptr_t pObject
											        )
{
	OBJECT_ENTRY		*pthis_entry;
	ZOE_OBJECT_HANDLE	hObject = ZOE_NULL_HANDLE;
	uint32_t        i;

	ENTER_CRITICAL(&This->m_Object)

	pthis_entry = This->m_pHead;	

	for (i = 0; i < This->m_dwObjectNb; i++)
	{
		if (pthis_entry->pObject == pObject)
		{
			hObject = pthis_entry->hObject;
			break;
		}
		pthis_entry = pthis_entry->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (hObject);
}



zoe_void_ptr_t c_object_mgr_get_object_by_index(c_object_mgr *This,
									            uint32_t dwIndex
									            )
{
	OBJECT_ENTRY	*pthis_entry;
	zoe_void_ptr_t  pObject = ZOE_NULL;
	uint32_t    i;

	ENTER_CRITICAL(&This->m_Object)

	pthis_entry = This->m_pHead;	

	for (i = 0; i < This->m_dwObjectNb; i++)
	{
		if (i == dwIndex)
		{
			pObject = pthis_entry->pObject;
			break;
		}
		pthis_entry = pthis_entry->pNext;
	}

	LEAVE_CRITICAL(&This->m_Object)

	return (pObject);
}



ZOE_OBJECT_HANDLE c_object_mgr_set_current_object(c_object_mgr *This,
											      ZOE_OBJECT_HANDLE hObject
											      )
{
	ZOE_OBJECT_HANDLE	hold_object = This->m_hCurObject;

	This->m_hCurObject = hObject;
	return (hold_object);
}



ZOE_OBJECT_HANDLE c_object_mgr_get_current_object(c_object_mgr *This) 
{
	return (This->m_hCurObject);
}



uint32_t c_object_mgr_get_number_of_objects(c_object_mgr *This) 
{
	return (This->m_dwObjectNb);
}



