/*
 * Copyright (c) 2011-2015, Freescale Semiconductor, Inc.
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
/******************************************************************************
*                                                                             *
* All rights are reserved. This confidential and proprietary HDL/C/HVL soft   *
* description of a Hardware/Software component may be used only as authorized *
* by a licensing agreement from Zenverge Incorporated. In the event of        *
* publication, the following notice is applicable:                            *
*                                                                             *
*                  (C) COPYRIGHT 2011-2014 Zenverge Inc.                      *
*                          ALL RIGHTS RESERVED                                *
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


#ifndef __ZOE_TYPES_H__
#define __ZOE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif


#undef ZOE_POINTERS64

#if defined(__GNUC__)

	#ifdef _MIPS_ARCH
		#if defined(__MIPSEL__) || defined(MIPSEL)
			#define ZOE_LITTLE_ENDIAN
		#elif defined(__MIPSEB__) || defined(MIPSEB)
			#undef ZOE_LITTLE_ENDIAN
		#else
			#error MIPS_ARCH: Unsupported endianness
		#endif

    #elif defined(__xtensa__)
	    #ifdef __XTENSA_EL__
		    #define ZOE_LITTLE_ENDIAN
	    #else
		    #undef ZOE_LITTLE_ENDIAN
		#endif
	#elif defined(ZOE_LINUXKER_BUILD)
        #ifdef __cplusplus
	        #define ZOE_LITTLE_ENDIAN
        #else // !__cplusplus
		    #include <asm/byteorder.h>
		    #ifdef __LITTLE_ENDIAN
			    #define ZOE_LITTLE_ENDIAN
		    #elif defined(__BIG_ENDIAN)
			    #undef ZOE_LITTLE_ENDIAN
		    #else
			    #error Linux Kernel: Unsupported endianness
		    #endif
        #endif //__cplusplus
    #else
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			#define ZOE_LITTLE_ENDIAN
        #else
			#undef ZOE_LITTLE_ENDIAN
		#endif        
	#endif

    typedef signed char zoe_int8_t;
    typedef signed char * zoe_int8_ptr_t;
    typedef unsigned char zoe_uint8_t;
    typedef unsigned char * zoe_uint8_ptr_t;
    typedef signed short zoe_int16_t;
    typedef signed short * zoe_int16_ptr_t;
    typedef unsigned short zoe_uint16_t;
    typedef unsigned short * zoe_uint16_ptr_t;
    typedef signed int zoe_int32_t;
    typedef signed int * zoe_int32_ptr_t;
    typedef unsigned int zoe_uint32_t;
    typedef unsigned int * zoe_uint32_ptr_t;
    typedef signed long long zoe_int64_t;
    typedef signed long long * zoe_int64_ptr_t;
    typedef unsigned long long zoe_uint64_t;
    typedef unsigned long long * zoe_uint64_ptr_t;

#if (__SIZEOF_POINTER__ == 4)
	typedef zoe_uint32_t zoe_uintptr_t;
#elif (__SIZEOF_POINTER__ == 8)
	typedef zoe_uint64_t zoe_uintptr_t;
	#define ZOE_POINTERS64
#else
	#error "Unsupported pointer size"
#endif

    #define STATIC_INLINE static inline
    #define FILENAME_MACRO __BASE_FILE__

#elif defined(_WIN32)

    typedef signed __int8 zoe_int8_t;
    typedef signed __int8 * zoe_int8_ptr_t;
    typedef unsigned __int8 zoe_uint8_t;
    typedef unsigned __int8 * zoe_uint8_ptr_t;
    typedef signed __int16 zoe_int16_t;
    typedef signed __int16 * zoe_int16_ptr_t;
    typedef unsigned __int16 zoe_uint16_t;
    typedef unsigned __int16 * zoe_uint16_ptr_t;
    typedef signed __int32 zoe_int32_t;
    typedef signed __int32 * zoe_int32_ptr_t;
    typedef unsigned __int32 zoe_uint32_t;
    typedef unsigned __int32 * zoe_uint32_ptr_t;
    typedef signed __int64 zoe_int64_t;
    typedef signed __int64 * zoe_int64_ptr_t;
    typedef unsigned __int64 zoe_uint64_t;
    typedef unsigned __int64 * zoe_uint64_ptr_t;

#if defined(_WIN64)
	typedef unsigned __int64 zoe_uintptr_t;
	#define ZOE_POINTERS64
#else
	typedef unsigned int zoe_uintptr_t;
#endif

    #define STATIC_INLINE static __inline
    #define FILENAME_MACRO __FILE__

    // Microsoft C library has _snprintf instead of snprintf
    #define snprintf _snprintf

    // Assume that Win32 machines are always little endian
	#define ZOE_LITTLE_ENDIAN

#else
    #error Unsupported compiler
#endif

typedef void * zoe_void_ptr_t;


// device memory
typedef zoe_uint64_t zoe_dev_mem_t;



#if defined(ZOE_LINUXKER_BUILD) || defined(ZOE_WINKER_BUILD)
typedef zoe_uint8_t     uint8_t;
typedef zoe_uint16_t    uint16_t;
typedef zoe_uint32_t    uint32_t;
typedef zoe_uint64_t    uint64_t;
typedef zoe_int8_t      int8_t;
typedef zoe_int16_t     int16_t;
typedef zoe_int32_t     int32_t;
typedef zoe_int64_t     int64_t;
#else // !ZOE_LINUXKER_BUILD && !ZOE_WINKER_BUILD
#if !(_MSC_VER == 1700)
#include <stdint.h> //VS2005 does not have this header file
#else
	typedef signed char        int8_t;
	typedef short              int16_t;
	typedef int                int32_t;
	typedef long long          int64_t;
	typedef unsigned char      uint8_t;
	typedef unsigned short     uint16_t;
	typedef unsigned int       uint32_t;
	typedef unsigned long long uint64_t;

	typedef signed char        int_least8_t;
	typedef short              int_least16_t;
	typedef int                int_least32_t;
	typedef long long          int_least64_t;
	typedef unsigned char      uint_least8_t;
	typedef unsigned short     uint_least16_t;
	typedef unsigned int       uint_least32_t;
	typedef unsigned long long uint_least64_t;

	typedef signed char        int_fast8_t;
	typedef int                int_fast16_t;
	typedef int                int_fast32_t;
	typedef long long          int_fast64_t;
	typedef unsigned char      uint_fast8_t;
	typedef unsigned int       uint_fast16_t;
	typedef unsigned int       uint_fast32_t;
	typedef unsigned long long uint_fast64_t;
#endif
#endif // ZOE_LINUXKER_BUILD || ZOE_WINKER_BUILD


#define ZOE_TRUE 1
#define ZOE_FALSE 0
typedef zoe_uint32_t zoe_bool_t;

#define ZOE_NULL 0

#define ZOE_BAD_VIR_PTR ((void *) ZOE_NULL)
#define ZOE_BAD_PHY_PTR ((void *) -1)


enum {
    ZOE_ERRS_SUCCESS = 0,
    ZOE_ERRS_PENDING = 1,   // requested operation pending
    ZOE_ERRS_TIMEOUT = -1,
    ZOE_ERRS_NOMEMORY = -2,
    ZOE_ERRS_INVALID = -3,
    ZOE_ERRS_PARMS = -4,
    ZOE_ERRS_NOTFOUND = -5,
    ZOE_ERRS_FAIL = -6,     // generic error
    ZOE_ERRS_NOTIMPL = -7,  // not implemented
    ZOE_ERRS_NOTSUPP = -8,  // not supported
    ZOE_ERRS_BUSY = -9,
    ZOE_ERRS_CANCELLED = -10,
    ZOE_ERRS_AGAIN = -11,   // try again
    ZOE_ERRS_INTERNAL = -12 // internal error
};

typedef zoe_int32_t zoe_errs_t;

#define zoe_ret_code(rv) \
	((rv==ZOE_ERRS_SUCCESS)		? "ZOE_ERRS_SUCCESS" : \
	 (rv==ZOE_ERRS_PENDING)		? "ZOE_ERRS_PENDING" : \
	 (rv==ZOE_ERRS_TIMEOUT)		? "ZOE_ERRS_TIMEOUT" : \
	 (rv==ZOE_ERRS_NOMEMORY)	? "ZOE_ERRS_NOMEMORY" : \
	 (rv==ZOE_ERRS_INVALID)		? "ZOE_ERRS_INVALID" : \
	 (rv==ZOE_ERRS_PARMS)		? "ZOE_ERRS_PARMS" : \
	 (rv==ZOE_ERRS_NOTFOUND)	? "ZOE_ERRS_NOTFOUND" : \
     (rv==ZOE_ERRS_FAIL)		? "ZOE_ERRS_FAIL" : \
     (rv==ZOE_ERRS_NOTIMPL)		? "ZOE_ERRS_NOTIMPL" : \
	 (rv==ZOE_ERRS_NOTSUPP)		? "ZOE_ERRS_NOTSUPP" : \
	 (rv==ZOE_ERRS_BUSY)		? "ZOE_ERRS_BUSY" : \
     (rv==ZOE_ERRS_CANCELLED)	? "ZOE_ERRS_CANCELLED" : \
     (rv==ZOE_ERRS_AGAIN)		? "ZOE_ERRS_AGAIN" : \
     (rv==ZOE_ERRS_INTERNAL)	? "ZOE_ERRS_INTERNAL" : \
	 "UNKNOWN")


#define ZOE_SUCCESS(Status) ((Status) >= 0)
#define ZOE_FAIL(Status) ((Status) < 0)


// zoe state 
enum {
    ZOE_STATE_STOPPED  = 0,
    ZOE_STATE_PLAYING  = 1,
    ZOE_STATE_PAUSED   = 2,

    ZOE_STATE_NUM
};

typedef zoe_uint32_t zoe_state_t;

#define zoe_state_string(rv) \
		((rv==ZOE_STATE_STOPPED)	? "STOPPED" : \
		 (rv==ZOE_STATE_PLAYING)	? "PLAYING" : \
	     (rv==ZOE_STATE_PAUSED)		? "PAUSED" : \
		 "UNKNOWN")

typedef struct _ZOE_GUID 
{
    zoe_uint32_t    Data1;
    zoe_uint16_t    Data2;
    zoe_uint16_t    Data3;
    zoe_uint8_t     Data4[8];
} ZOE_GUID, *ZOE_REFGUID, *PZOE_GUID;


// generic object ID
typedef void * zoe_obj_id_t;

typedef zoe_uint32_t    ZOE_OBJECT_HANDLE, *PZOE_OBJECT_HANDLE; //handle of an object
#define ZOE_NULL_HANDLE ((ZOE_OBJECT_HANDLE)(-1))

// byte-swapping macros
#ifdef _MIPS_ARCH
	#define ZOE_BYTE_SWAP_16(x) __extension__({					\
		zoe_uint16_t __swap16md_x = (x);					\
		zoe_uint16_t __swap16md_v;						\
		__asm__ ("wsbh %0,%1" 						\
			 : "=d" (__swap16md_v) 					\
			 : "d" (__swap16md_x)); 					\
		__swap16md_v; 							\
	})
	#define ZOE_BYTE_SWAP_32(x) __extension__({					\
		zoe_uint32_t __swap32md_x = (x);					\
		zoe_uint32_t __swap32md_v;						\
		__asm__ ("wsbh %0,%1; rotr %0,16" 					\
			 : "=d" (__swap32md_v) 					\
			 : "d" (__swap32md_x)); 					\
		__swap32md_v; 							\
	})
#else
	#define ZOE_BYTE_SWAP_16(v) ((v << 8) | (v >> 8))
	#define ZOE_BYTE_SWAP_32(v) ((ZOE_BYTE_SWAP_16(v) << 16) | ZOE_BYTE_SWAP_16(v >> 16))
#endif

#ifdef ZOE_LITTLE_ENDIAN
	#define ZOE_FOURCC(a,b,c,d)  ((zoe_uint32_t) \
		(((zoe_uint8_t) a) | (((zoe_uint8_t) b) << 8) |	\
		(((zoe_uint8_t) c) << 16) | (((zoe_uint8_t) d) << 24)))
	#define ZOE_EIGHTCC(a,b,c,d,e,f,g,h)  ((uint64_t) \
		((((uint64_t) ZOE_FOURCC(e,f,g,h)) << 32) | (ZOE_FOURCC(a,b,c,d))))
	#define ZOE_CPU_TO_BE16(v) ZOE_BYTE_SWAP_16(v)
	#define ZOE_CPU_TO_BE32(v) ZOE_BYTE_SWAP_32(v)
#else
	#define ZOE_FOURCC(a,b,c,d)  ((zoe_uint32_t) \
		(((zoe_uint8_t) d) | (((zoe_uint8_t) c) << 8) |	\
		(((zoe_uint8_t) b) << 16) | (((zoe_uint8_t) a) << 24)))
	#define ZOE_EIGHTCC(a,b,c,d,e,f,g,h)  ((uint64_t) \
		((((uint64_t) ZOE_FOURCC(a,b,c,d)) << 32) | (ZOE_FOURCC(e,f,g,h))))
	#define ZOE_CPU_TO_BE16(v) v
	#define ZOE_CPU_TO_BE32(v) v
#endif



#ifndef ZOE_MAX
#define ZOE_MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef ZOE_MIN
#define ZOE_MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(ar)    (sizeof(ar) / sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)


// Macros to calculate an aligned address relative to a given address.
// al2 is the log2 of the alignment (e.g. 4 for 16B alignment);
// the result is a uint32.
#define NEXT_ALIGNED(p,al2) ((((zoe_uintptr_t) (p)) + ((1<<(al2))-1)) & ~((1<<(al2))-1))
#define PREV_ALIGNED(p,al2) (((zoe_uintptr_t) (p)) & ~((1<<(al2))-1))

//
// Macro to generate a compiler error if a certain predicate is not TRUE
// The predicate can be an expression that uses some C/C++ expression.
// Example:
//	CCASSERT(sizeof(globalStruct) <= MEMORY_REGION)
//	Will stop building if the structure globalStruct is larger than the value of MEMORY_REGION
//
#define CCASSERT(predicate) typedef char ASSERT_CONCAT(ccassert_,__COUNTER__)[2*((predicate)!=0)-1];
#define ASSERT_CONCAT(a,b) ASSERT_CONCAT_(a,b)
#define ASSERT_CONCAT_(a,b) a##b


// Macro to return a pointer to the structure that contains a given field at a given address.
//  fieldAddr is the actual address of the field
//  fieldName is the name of the field
//  structType is the typedef name of the structure
//
#define ZOE_GET_STRUCT_PTR(fieldAddr,fieldName,structType) \
        ((structType *) (((char *) fieldAddr) - ((char *) &((structType *) 0)->fieldName)))


#ifdef __cplusplus
}
#endif

#endif //__ZOE_TYPES_H__

