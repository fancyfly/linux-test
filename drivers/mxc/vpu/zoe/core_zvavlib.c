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
// zvavlib.c
//
// Description: 
//
//	zpu codec library entry point.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zv_avlib.h"
#include "czvavlib.h"
#include "zoe_sosal.h"
#include "zoe_dbg.h"

/////////////////////////////////////////////////////////////////////////////
//
//

zoe_dbg_comp_id_t	g_ZVAVLibDBGCompID = 0;
c_zv_av_lib         *g_zvavlib = ZOE_NULL;

/////////////////////////////////////////////////////////////////////////////
//
//

zoe_errs_t zv_init_av_library(zoe_void_ptr_t pContext1,	//PDEVICE_OBJECT
						      zoe_void_ptr_t pContext2,	//PDEVICE_OBJECT
						      PZV_AVLIB_INITDATA pInitData,
						      i_zv_av_lib **pp_i_zv_av_lib
						      )
{
	c_zv_av_lib *pZVAVLib;
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	// register the debug component for core library
	//
	if (0 == g_ZVAVLibDBGCompID)
    {
		ZOE_DBG_COMP_REGISTER("ZV Core Lib", 
                              g_ZVAVLibDBGCompID, 
                              ZOE_DBG_LVL_ERROR/*ZOE_DBG_LVL_TRACE*/
                              );
    }

    // there can be only one
    //
    if (g_zvavlib)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVAVLibDBGCompID,
				       "zv_init_av_library() g_zvavlib existed!!!\n" 
				       );
        return (ZOE_ERRS_INVALID);
    }

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVAVLibDBGCompID,
				   "zv_init_av_library()\n" 
				   );

	if (!pInitData || 
		!pp_i_zv_av_lib
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       g_ZVAVLibDBGCompID,
					   "zv_init_av_library() pInitData == NULL or pp_i_zv_av_lib == NULL!!!\n"
					   );
		err = ZOE_ERRS_PARMS;
		goto ZVInitAVLibrary_Err;
	}

    // allocate memory for our core library object
    //
	pZVAVLib = (c_zv_av_lib *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                     sizeof(c_zv_av_lib),
                                                     0
                                                     );
	if (!pZVAVLib)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       g_ZVAVLibDBGCompID,
					   "zv_init_av_library() pZVAVLib == NULL!!!\n"
					   );
		err = ZOE_ERRS_NOMEMORY;
		goto ZVInitAVLibrary_Err;
	}

	if (ZOE_NULL == c_zv_av_lib_constructor(pZVAVLib, 
										    pContext1, 
										    pContext2,
										    pInitData,
                                            g_ZVAVLibDBGCompID
										    ))
	{
		zoe_sosal_memory_free((void *)pZVAVLib);
		zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       g_ZVAVLibDBGCompID,
					   "zv_init_av_library() FAILED - unable to create c_zv_av_lib!!!\n"
					   );
		err = ZOE_ERRS_FAIL;
		goto ZVInitAVLibrary_Err;
	}

	if (!c_object_init(&pZVAVLib->m_Object))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_FATAL,
                       g_ZVAVLibDBGCompID,
					   "zv_init_av_library() FAILED - c_zv_av_lib Init Failed!!!\n"
					   );
		c_object_done(&pZVAVLib->m_Object);
		c_zv_av_lib_destructor(pZVAVLib);
		zoe_sosal_memory_free((void *)pZVAVLib);
		err = ZOE_ERRS_FAIL;
		goto ZVInitAVLibrary_Err;
	}

    // save avlib object pointer
    //
    g_zvavlib = pZVAVLib;

	// return only the i_zv_av_lib interface
	//
	*pp_i_zv_av_lib = &pZVAVLib->m_iZVAVLib;
	return (err);

ZVInitAVLibrary_Err:
	// unregister the component from the debug manager
	//
	ZOE_DBG_COMP_UNREGISTER(g_ZVAVLibDBGCompID);
	g_ZVAVLibDBGCompID = 0;
	*pp_i_zv_av_lib = ZOE_NULL;
	return (err);
}



zoe_errs_t zv_done_av_library(i_zv_av_lib *p_i_zv_av_lib)
{
	c_zv_av_lib	*pZVAVLib;
	zoe_errs_t	err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   g_ZVAVLibDBGCompID,
				   "zv_done_av_library() p_i_zv_av_lib(0x%x)\n",
				   p_i_zv_av_lib
				   );

	if (p_i_zv_av_lib)
	{
		pZVAVLib = GET_INHERITED_OBJECT(c_zv_av_lib, m_iZVAVLib, p_i_zv_av_lib);

		if (pZVAVLib &&
            (pZVAVLib == g_zvavlib)
            )
		{
		    c_object_done(&pZVAVLib->m_Object);
		    c_zv_av_lib_destructor(pZVAVLib);
		    zoe_sosal_memory_free((void *)pZVAVLib);
            g_zvavlib = ZOE_NULL;
		}

		err = ZOE_ERRS_SUCCESS;
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVAVLibDBGCompID,
					   "zv_done_av_library() pZVAVLib == NULL!!!\n"
					   );
		err = ZOE_ERRS_PARMS;
	}

    if (ZOE_SUCCESS(err))
    {
	    // unregister the component from the debug manager 
	    //
	    if (g_ZVAVLibDBGCompID)
	    {
	        ZOE_DBG_COMP_UNREGISTER(g_ZVAVLibDBGCompID);
	        g_ZVAVLibDBGCompID = 0;
	    }
    }

	return (err);
}



