/******************************************************************************
*                                                                             *
* All rights are reserved. This confidential and proprietary HDL/C/HVL soft   *
* description of a Hardware/Software component may be used only as authorized *
* by a licensing agreement from Zenverge Incorporated. In the event of        *
* publication, the following notice is applicable:                            *
*                                                                             *
*                       (C) COPYRIGHT 2011 Zenverge Inc.                      *
*                             ALL RIGHTS RESERVED                             *
* The entire notice above must be reproduced on all authorized copies of this *
* code. Reproduction in whole or in part is prohibited without the prior      *
* written consent of the Zenverge Incorporated.                               *
* Zenverge Incorporated reserves the right to make changes without notice at  *
* any time. Also, Zenverge Incorporated makes no warranty, expressed, implied *
* or statutory, including but not limited to any implied warranty of          *
* merchantability or fitness for any particular purpose, or that the use will *
* not infringe any third party patent, copyright or trademark. Zenverge       *
* Incorporated will not be liable for any loss or damage arising from its use *
******************************************************************************/


///////////////////////////////////////////////////////////////////////////////
//
// zoe_dbg.h  Debugging API
//
// Use this API to display debugging information that is automatically
// directed to the debug device of choice.
// It is essentially a printf-like interface with two additional features:
//  - component selection
//  - debug level selection
//
// Each debug printf call has a component and a debug level associated with it
// If that particular component has a debug level equal or higher than the one
// requested, then the printout is displayed on the current debug output
// device.
// An external debugging tool can select the debugging level for each
// registered component.  Modules register themselves as debug components
// with a given name and are assigned a unique ID; a special component
// ("Unspecified") is reserved to ID 0 and can be used for temporary printouts
// or if the module does not register its own component.
//
// There are 5 different debugging levels:
//  - Fatal: an unrecoverable error that will cause the death of the system
//  - Error: an error that could possibly be recovered
//  - Warning: a minor error, possibly a coding mistake or a normal occurrence
//  - Trace: just to show that the code is progressing normally
//  - eXtended trace: like trace above, but usually reserved for large amounts
//      of printout data (e.g. entering and exiting functions)
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_DBG_H__
#define __ZOE_DBG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "zoe_types.h"


typedef enum {
    ZOE_DBG_LVL_FATAL = 0,
    ZOE_DBG_LVL_ERROR,
    ZOE_DBG_LVL_WARNING,
    ZOE_DBG_LVL_TRACE,
    ZOE_DBG_LVL_XTRACE,

    ZOE_DBG_LVL_NUM
} zoe_dbg_levels_t;

#ifdef ZOE_DEBUG_DEFAULT_LEVEL
	#if ((ZOE_DEBUG_DEFAULT_LEVEL < 0) && (ZOE_DEBUG_DEFAULT_LEVEL >= ZOE_DBG_LVL_NUM))
		#error Invalid dbg_printf default value
	#endif
#else
	#define ZOE_DEBUG_DEFAULT_LEVEL ZOE_DBG_LVL_WARNING
#endif

// Type for the component ID
typedef void * zoe_dbg_comp_id_t;


// helper macros
#define ZOE_DBG_COMP_DECL(cID)  \
    zoe_dbg_comp_id_t cID = ZOE_NULL;

#define ZOE_DBG_COMP_EXT(cID)  \
    extern zoe_dbg_comp_id_t cID;


#ifdef ZOE_DEBUG

// Generic printf with level and component id
#define zoe_dbg_printf(lvl,c_id,fmt, ...) \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,lvl,c_id,fmt,##__VA_ARGS__)

// Specific macros for each debugging level
#define zoe_dbg_printf_f(c_id,fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_FATAL,c_id,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_e(c_id,fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_ERROR,c_id,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_w(c_id,fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_WARNING,c_id,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_t(c_id,fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_TRACE,c_id,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_x(c_id,fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_XTRACE,c_id,fmt,##__VA_ARGS__)

// Specific macros for the "Unspecified" component
#define zoe_dbg_printf_nc(lvl,fmt,...) _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,lvl,0,fmt,##__VA_ARGS__)

#define zoe_dbg_printf_nc_f(fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_FATAL,0,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_nc_e(fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_ERROR,0,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_nc_w(fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_WARNING,0,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_nc_t(fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_TRACE,0,fmt,##__VA_ARGS__)
#define zoe_dbg_printf_nc_x(fmt,...)  \
    _zoe_dbg_printf_(FILENAME_MACRO,__FUNCTION__,ZOE_DBG_LVL_XTRACE,0,fmt,##__VA_ARGS__)

// internal printf function for use with the macros above
void _zoe_dbg_printf_ (const char * fileName, const char * funcName, zoe_dbg_levels_t lvl, zoe_dbg_comp_id_t c_id, const char * fmt, ...);

// Register/unregister debug components
zoe_dbg_comp_id_t zoe_dbg_register (const char * name, zoe_dbg_levels_t lvl);
void zoe_dbg_unregister (zoe_dbg_comp_id_t c_id);

// Get/set debugging level
zoe_dbg_levels_t zoe_dbg_level_get (zoe_dbg_comp_id_t c_id);
void zoe_dbg_level_set (zoe_dbg_comp_id_t c_id, zoe_dbg_levels_t lvl);

#define ZOE_DBG_COMP_REGISTER(name,cID,lvl)    \
    {	cID = zoe_dbg_register(name,lvl);   \
        if (cID == ZOE_NULL) {  \
           zoe_dbg_printf_nc_e("Could not register debug component %s\n",name);    \
        }   \
    }

#define ZOE_DBG_COMP_REGISTER_DEF(name,cID)    ZOE_DBG_COMP_REGISTER(name,cID,ZOE_DEBUG_DEFAULT_LEVEL)

#define ZOE_DBG_COMP_UNREGISTER(cID) zoe_dbg_unregister(cID);

#else // !ZOE_DEBUG

#define zoe_dbg_printf(lvl,c_id,fmt,...)
#define zoe_dbg_printf_f(c_id,fmt,...)
#define zoe_dbg_printf_e(c_id,fmt,...)
#define zoe_dbg_printf_w(c_id,fmt,...)
#define zoe_dbg_printf_t(c_id,fmt,...)
#define zoe_dbg_printf_x(c_id,fmt,...)
#define zoe_dbg_printf_nc(lvl,fmt,...)
#define zoe_dbg_printf_nc_f(fmt,...)
#define zoe_dbg_printf_nc_e(fmt,...)
#define zoe_dbg_printf_nc_w(fmt,...)
#define zoe_dbg_printf_nc_t(fmt,...)
#define zoe_dbg_printf_nc_x(fmt,...)
#define zoe_dbg_register(name,lvl) ((zoe_dbg_comp_id_t) 0)
#define zoe_dbg_unregister(c_id)
#define zoe_dbg_level_get(c_id) ((zoe_dbg_levels_t) ZOE_DBG_LVL_NUM)
#define zoe_dbg_level_set(c_id,lvl)

#define ZOE_DBG_COMP_REGISTER(name,cID,lvl)
#define ZOE_DBG_COMP_REGISTER_DEF(name,cID)
#define ZOE_DBG_COMP_UNREGISTER(cID)

#endif



#ifdef __cplusplus
}
#endif

#endif //__ZOE_DBG_H__

