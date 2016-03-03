/*
 * Copyright (c) 2015-2016, NXP Semiconductors N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the NXP Semiconductors N.V. nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND  CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF  MERCHANTABILITY  AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NXP Semiconductors N.V. BE LIABLE FOR ANY
 * DIRECT, INDIRECT,  INCIDENTAL,  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY  THEORY  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR  OTHERWISE)  ARISING  IN  ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2014-2015, Freescale Semiconductor, Inc.
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
*                  (C) COPYRIGHT 2012-2014 Zenverge Inc.                      *
*                         ALL RIGHTS RESERVED                                 *
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


//
// May 11, 2012	(EC) Created
//
// Authors:
//	(EC)	Enrico Cadorin
//

//
// Header wrapper for XREG accesses
//

#ifndef __ZOE_XREG_H__
#define __ZOE_XREG_H__

#ifdef __cplusplus
extern "C" {
#endif

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
	#error Chisel not supported
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
	// All ZeOS modules map XREG uncached to 'kseg1
	#define XREG_ARB_BASE    	0xB0000000
	// Physical address of XREG BASE (for external agents)
	#define XREG_ARB_BASE_PHY   0x10000000
	#if (ZOE_TARGET_CHIP_VAR == ZOE_TARGET_CHIP_VAR_) || (ZOE_TARGET_CHIP_VAR == ZOE_TARGET_CHIP_VAR_DV)
		#include "xreg_64M/zv_xreg_defines.h"
		#define XREG_MAP_64M
	#else
		#include "xreg_16M/zv_xreg_defines.h"
		#define XREG_MAP_16M
	#endif
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
	#if (ZOE_TARGET_CHIP_VAR == ZOE_TARGET_CHIP_VAR_)
		// Use the no-variant chip name for the ARM emulator
		#if (ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS) || (ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS)
			#define XREG_ARB_BASE    	0x200C000000
		#else
			#define XREG_ARB_BASE    	0x0C000000
		#endif
		// Physical address of XREG BASE (for external agents)
		#define XREG_ARB_BASE_PHY   0x2C000000
		#include "xreg_64M/zv_xreg_defines.h"
		#define XREG_MAP_64M
	#elif (ZOE_TARGET_CHIP_VAR == ZOE_TARGET_CHIP_VAR_DV)
		#if (ZOE_TARGET_OS == ZOE_TARGET_OS_ZEOS) || (ZOE_TARGET_OS == ZOE_TARGET_OS_FREERTOS)
			#define XREG_ARB_BASE    	0x202C000000
		#else
			#define XREG_ARB_BASE    	0x2C000000
		#endif
		// Physical address of XREG BASE (for external agents)
		#define XREG_ARB_BASE_PHY   0x2C000000
		#include "xreg_64M/zv_xreg_defines.h"
		#define XREG_MAP_64M
	#else
// TODO: find base address for the 16MB register map
		#define XREG_ARB_BASE    	0x2C000000
		// Physical address of XREG BASE (for external agents)
		#define XREG_ARB_BASE_PHY   0x2C000000
		#include "xreg_16M/zv_xreg_defines.h"
		#define XREG_MAP_16M
	#endif
#else
	#error Unsupported CHIP
#endif

#if !defined(XREG_MAP_16M) && !defined(XREG_MAP_64M)
	#error No register map defined
#endif

// Absolute address interface
#if defined(__NIOS2)
	#define ZOE_READ_REG_ADDR(addr) ((uint32_t) __builtin_ldwio((void *)(addr)))
	#define ZOE_WRITE_REG_ADDR(addr,val) (__builtin_stwio((void *)(addr),val))
	#define ZOE_WRITE_REG_ADDR_SYNCH(addr,val) {	\
		volatile int tmp __attribute__ ((unused));	\
		__builtin_stwio((void *)(addr),val);		\
		tmp = __builtin_ldwio((void *)(addr));		\
		}
#else //ISA_NIOS
	#define ZOE_READ_REG_ADDR(addr) *((volatile uint32_t *) (uintptr_t) (addr))
	#define ZOE_WRITE_REG_ADDR(addr,val) (*((volatile uint32_t *) (uintptr_t) (addr)) = val)
	#define ZOE_WRITE_REG_ADDR_SYNCH(addr,val) {		\
		volatile uint32_t tmp __attribute__ ((unused));						\
		(*((volatile uint32_t *) (uintptr_t) (addr)) = val);	\
		tmp = *((volatile uint32_t *) (uintptr_t) (addr));		\
		}
#endif //ISA_NIOS

// XREG Offset interface
#define ZOE_READ_REG_XO(xoffset) ZOE_READ_REG_ADDR(XREG_ARB_BASE+((uintptr_t) (xoffset)))
#define ZOE_WRITE_REG_XO(xoffset,val) ZOE_WRITE_REG_ADDR(XREG_ARB_BASE+((uintptr_t) (xoffset)),val)
#define ZOE_WRITE_REG_XO_SYNCH(xoffset,val) ZOE_WRITE_REG_ADDR_SYNCH(XREG_ARB_BASE+((uintptr_t) (xoffset)),val)


// Block/Offset interface
#define ZOE_READ_REG_BO(block,offset) ZOE_READ_REG_ADDR(XREG_ARB_BASE+block+(offset))
#define ZOE_WRITE_REG_BO(block,offset,val) ZOE_WRITE_REG_XO(XREG_ARB_BASE+block+(offset),val)
#define ZOE_WRITE_REG_BO_SYNCH(block,offset,val) ZOE_WRITE_REG_XO_SYNCH(XREG_ARB_BASE+(block)+(offset),val)


#ifdef __cplusplus
}
#endif

#endif //__ZOE_XREG_H__

