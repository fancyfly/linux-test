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
// zoe_objectmgr.h
//
// Description: 
//
//	the object that manages multiple objects.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_COBJECTMGR_H__
#define __ZOE_COBJECTMGR_H__

#include "zoe_types.h"
#include "zoe_cobject.h"


typedef zoe_errs_t (*OBJECTMGR_CALLBACK)(zoe_void_ptr_t pParam);


/////////////////////////////////////////////////////////////////////////////
//
//

// C defination, visible to both C and C++ code
//

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#ifndef __COBJECTMGR_FWD_DEFINED__
#define __COBJECTMGR_FWD_DEFINED__
typedef struct c_object_mgr c_object_mgr;
typedef struct OBJECT_ENTRY OBJECT_ENTRY;
#endif // !__COBJECTMGR_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

struct OBJECT_ENTRY
{
	ZOE_OBJECT_HANDLE	hObject;	// handle
	zoe_void_ptr_t		*pObject;	// pointer to object
	OBJECT_ENTRY		*pNext;		// next object entry
};



struct c_object_mgr
{
	// c_object
	//
	c_object			m_Object;

	// c_object_mgr
	//
	ZOE_OBJECT_HANDLE	m_hCurObject;
	OBJECT_ENTRY		*m_pHead;			// head of object list.
	uint32_t		    m_dwObjectNb;		// number of objects in the list.
	OBJECTMGR_CALLBACK	m_pFuncCallback;	// callback
};


c_object_mgr * c_object_mgr_constructor(c_object_mgr *pObjectMgr,
									    c_object *pParent, 
									    uint32_t dwAttributes,
									    OBJECTMGR_CALLBACK pFuncCallback
									    );
void c_object_mgr_destructor(c_object_mgr *This);
ZOE_OBJECT_HANDLE c_object_mgr_add_object(c_object_mgr *This,
									      zoe_void_ptr_t pObject
									      );
zoe_void_ptr_t c_object_mgr_remove_object(c_object_mgr *This,
								          ZOE_OBJECT_HANDLE hObject
								          );
zoe_void_ptr_t c_object_mgr_get_object_by_handle(c_object_mgr *This,
									             ZOE_OBJECT_HANDLE hObject
									             );
ZOE_OBJECT_HANDLE c_object_mgr_get_handle_by_object(c_object_mgr *This,
											        zoe_void_ptr_t pObject
											        );
zoe_void_ptr_t c_object_mgr_get_object_by_index(c_object_mgr *This,
									            uint32_t dwIndex
									            );
ZOE_OBJECT_HANDLE c_object_mgr_set_current_object(c_object_mgr *This,
											      ZOE_OBJECT_HANDLE hObject
											      );
ZOE_OBJECT_HANDLE c_object_mgr_get_current_object(c_object_mgr *This);
uint32_t c_object_mgr_get_number_of_objects(c_object_mgr *This);


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

template <class T> class OBJECT_ENTRY_CPP
{
public:
	OBJECT_ENTRY_CPP() 
		: pNext(ZOE_NULL)
		, hObject(0)
		, pObject(ZOE_NULL) 
	{
	}
	virtual ~OBJECT_ENTRY_CPP() {}

	ZOE_OBJECT_HANDLE	hObject;	// handle
	T					*pObject;	// pointer to object
	OBJECT_ENTRY_CPP	*pNext;		// next object entry
};

/////////////////////////////////////////////////////////////////////////////
//
//

template <class T> class cpp_object_mgr : public cpp_object
{
public:
	cpp_object_mgr(cpp_object *pParent, 
				   OBJECTMGR_CALLBACK pFuncCallback
				   );
	cpp_object_mgr(cpp_object *pParent, 
				   uint32_t dwWhoAmI, 
				   uint32_t dwAttributes, 
				   OBJECTMGR_CALLBACK pFuncCallback
				   );
	virtual ~cpp_object_mgr();

	ZOE_OBJECT_HANDLE add_object(T *pObject);
	T * remove_object_by_handle(ZOE_OBJECT_HANDLE hObject);
	T * remove_object(T *pObject);

	T * get_object_by_handle(ZOE_OBJECT_HANDLE hObject);
	ZOE_OBJECT_HANDLE get_handle_by_object(T *pObject);

	ZOE_OBJECT_HANDLE set_current_object(ZOE_OBJECT_HANDLE hObject);
	ZOE_OBJECT_HANDLE get_current_object() {return (m_hCurObject);}
	uint32_t get_number_of_objects() {return (m_dwObjectNb);}

protected:
	void remove_objects();
	T * get_object_by_index(uint32_t dwIndex);

	ZOE_OBJECT_HANDLE	m_hCurObject;
	OBJECT_ENTRY_CPP<T>	*m_pHead;           // head of object list.
	uint32_t        m_dwObjectNb;       // number of objects in the list.
	OBJECTMGR_CALLBACK	m_pFuncCallback;    // callback
};


/////////////////////////////////////////////////////////////////////////////
//
//

template <class T>
cpp_object_mgr<T>::cpp_object_mgr(cpp_object *pParent, 
							      OBJECTMGR_CALLBACK pFuncCallback
							      )
	: cpp_object(pParent, 
				 OBJECT_OBJECTMGR, 
				 OBJECT_CRITICAL_LIGHT
				 )
	, m_hCurObject(ZOE_NULL_HANDLE)
	, m_pHead((OBJECT_ENTRY_CPP<T> *)ZOE_NULL)
	, m_dwObjectNb(0)
	, m_pFuncCallback(pFuncCallback)
{
}



template <class T>
cpp_object_mgr<T>::cpp_object_mgr(cpp_object *pParent, 
							      uint32_t dwWhoAmI, 
							      uint32_t dwAttributes, 
							      OBJECTMGR_CALLBACK pFuncCallback
							      )
	: cpp_object(pParent, 
				 dwWhoAmI, 
				 dwAttributes
				 )
	, m_hCurObject(0)
	, m_pHead((OBJECT_ENTRY_CPP<T> *)ZOE_NULL)
	, m_dwObjectNb(0)
	, m_pFuncCallback(pFuncCallback)
{
}



template <class T>
cpp_object_mgr<T>::~cpp_object_mgr()
{
	// release all the objects
	//
	remove_objects();
}



template <class T>
ZOE_OBJECT_HANDLE cpp_object_mgr<T>::add_object(T *pObject)
{
	ZOE_OBJECT_HANDLE	hRet = 0;
	OBJECT_ENTRY_CPP<T>	*pNewEntry = new OBJECT_ENTRY_CPP<T>;

	ENTER_CRITICAL_CPP

	if (ZOE_NULL == m_pHead)
	{
		m_pHead = pNewEntry;
		m_pHead->hObject = 1;
		m_pHead->pObject = pObject;
		m_pHead->pNext = (OBJECT_ENTRY_CPP<T> *)ZOE_NULL;
		m_dwObjectNb++;
		hRet = m_pHead->hObject;
	}
	else
	{		
		OBJECT_ENTRY_CPP<T>	*ptemp_entry;
		
		if (m_pHead->hObject != 1)
		{
			ptemp_entry = pNewEntry;
			ptemp_entry->hObject = 1;
			ptemp_entry->pObject = pObject;
			ptemp_entry->pNext = m_pHead;
			m_pHead = ptemp_entry;
			m_dwObjectNb++;
			hRet = m_pHead->hObject;
		}
		else
		{
			OBJECT_ENTRY_CPP<T>	*pthis_entry = m_pHead;
			OBJECT_ENTRY_CPP<T>	*pnext_entry = m_pHead->pNext;
            
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
			m_dwObjectNb++;
			hRet = ptemp_entry->hObject;
		}
	}

	LEAVE_CRITICAL_CPP

	return (hRet);
}



template <class T>
T * cpp_object_mgr<T>::remove_object_by_handle(ZOE_OBJECT_HANDLE hObject)
{
	OBJECT_ENTRY_CPP<T>	*pthis_entry;
	OBJECT_ENTRY_CPP<T>	*pprev_entry;
	OBJECT_ENTRY_CPP<T>	*pRemove = ZOE_NULL;
	T					*pObject = ZOE_NULL;

	ENTER_CRITICAL_CPP

	pthis_entry = m_pHead;
	if (pthis_entry)
	{
		if (pthis_entry->hObject == hObject)
		{
			pObject = pthis_entry->pObject;
			m_pHead = pthis_entry->pNext;
			pRemove = pthis_entry;
			m_dwObjectNb--;
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
					m_dwObjectNb--;
					break;
				}		
			} while (pthis_entry);
		}
	}

	LEAVE_CRITICAL_CPP

	if (pRemove)
		delete pRemove;

	return (pObject);
}



template <class T>
T * cpp_object_mgr<T>::remove_object(T *pObject)
{
	OBJECT_ENTRY_CPP<T>	*pthis_entry;
	OBJECT_ENTRY_CPP<T>	*pprev_entry;
	OBJECT_ENTRY_CPP<T>	*pRemove = ZOE_NULL;
	T					*pObjectRet = ZOE_NULL;

	ENTER_CRITICAL_CPP

	pthis_entry = m_pHead;
	if (pthis_entry)
	{
		if (pthis_entry->pObject == pObject)
		{
			pObjectRet = pthis_entry->pObject;
			m_pHead = pthis_entry->pNext;
			pRemove = pthis_entry;
			m_dwObjectNb--;
		}
		else
		{
			do	{
				pprev_entry = pthis_entry;
				pthis_entry = pthis_entry->pNext;
				if (pthis_entry && (pthis_entry->pObject == pObject))
				{
					pprev_entry->pNext = pthis_entry->pNext;
					pObjectRet = pthis_entry->pObject;
					pRemove = pthis_entry;
					m_dwObjectNb--;
					break;
				}		
			} while (pthis_entry);
		}
	}

	LEAVE_CRITICAL_CPP

	if (pRemove)
		delete pRemove;

	return (pObjectRet);
}



template <class T>
T * cpp_object_mgr<T>::get_object_by_handle(ZOE_OBJECT_HANDLE hObject)
{
	OBJECT_ENTRY_CPP<T>	*pthis_entry;
	T					*pObject = ZOE_NULL;
	uint32_t		    i;

	ENTER_CRITICAL_CPP

	pthis_entry = m_pHead;	

	for (i = 0; i < m_dwObjectNb; i++)
	{
		if (pthis_entry->hObject == hObject)
		{
			pObject = pthis_entry->pObject;
		}
		pthis_entry = pthis_entry->pNext;
	}

	LEAVE_CRITICAL_CPP

	return (pObject);
}



template <class T>
ZOE_OBJECT_HANDLE cpp_object_mgr<T>::get_handle_by_object(T *pObject)
{
	OBJECT_ENTRY_CPP<T>	*pthis_entry;
	ZOE_OBJECT_HANDLE	hObject = ZOE_NULL_HANDLE;
	uint32_t		    i;

	ENTER_CRITICAL_CPP

	pthis_entry = m_pHead;	

	for (i = 0; i < m_dwObjectNb; i++)
	{
		if (pthis_entry->pObject == pObject)
		{
			hObject = pthis_entry->hObject;
			break;
		}
		pthis_entry = pthis_entry->pNext;
	}

	LEAVE_CRITICAL_CPP

	return (hObject);
}



template <class T>
ZOE_OBJECT_HANDLE cpp_object_mgr<T>::set_current_object(ZOE_OBJECT_HANDLE hObject)
{
	ZOE_OBJECT_HANDLE	hold_object = m_hCurObject;

	m_hCurObject = hObject;
	return (hold_object);
}



template <class T>
void cpp_object_mgr<T>::remove_objects()
{
	OBJECT_ENTRY_CPP<T>	*pthis_entry;
	OBJECT_ENTRY_CPP<T>	*ptemp_entry;

	pthis_entry = m_pHead;	

	while (pthis_entry)
	{
		ptemp_entry = pthis_entry;
		pthis_entry = pthis_entry->pNext;
		if (m_pFuncCallback && (ptemp_entry->pObject))
			m_pFuncCallback((PVOID)ptemp_entry->pObject);
		delete ptemp_entry;
	}
	m_dwObjectNb = 0;
	m_pHead = ZOE_NULL;
}



template <class T>
T * cpp_object_mgr<T>::get_object_by_index(uint32_t dwIndex)
{
	OBJECT_ENTRY_CPP<T>	*pthis_entry;
	T					*pObject = NULL;
	uint32_t		    i;

	ENTER_CRITICAL_CPP

	pthis_entry = m_pHead;	

	for (i = 0; i < m_dwObjectNb; i++)
	{
		if (i == dwIndex)
		{
			pObject = pthis_entry->pObject;
			break;
		}
		pthis_entry = pthis_entry->pNext;
	}

	LEAVE_CRITICAL_CPP

	return (pObject);
}

#endif //__cplusplus

#endif //__ZOE_COBJECTMGR_H__



