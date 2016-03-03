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
*                   (C) COPYRIGHT 2013-2014 Zenverge Inc.                     *
*                           ALL RIGHTS RESERVED                               *
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
// Assembly code can use configuration information as well and therefore all
// these header files must be assembly-friendly.  Basically, assembler can
// handle only C preprocessor directives; anything different from that must be
// surrounded by the following guards:
//
//	#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
//
//	#endif
//

#ifndef __ZOE_TARGET_H__
#define __ZOE_TARGET_H__


#define ZOE_IMAGE_SIGNATURE 0x69656F7A	// ZOE_FOURCC('z','o','e','i')


// Each individual target consists of a CHIP_VAR, a PU set, and an OS.
//


// ZOE_TARGET_CHIP takes one of these values (over 8 bit)
#define ZOE_TARGET_CHIP_CHISEL		0
#define ZOE_TARGET_CHIP_DAWN		1
#define ZOE_TARGET_CHIP_MX6			2
#define ZOE_TARGET_CHIP_MX8			3

#define ZOE_TARGET_CHIP_MAX			255		// not to exceed this value

// ZOE_TARGET_CHIP_VAR takes one of these values (over 8 bit)
#define ZOE_TARGET_CHIP_VAR_		0
#define ZOE_TARGET_CHIP_VAR_DV		1
#define ZOE_TARGET_CHIP_VAR_QM		2
#define ZOE_TARGET_CHIP_VAR_Z		3
#define ZOE_TARGET_CHIP_VAR_DQ		4

#define ZOE_TARGET_CHIP_VAR_MAX		255		// not to exceed this value

// ZOE_TARGET_OS takes one of these values (over 16 bit)
#define ZOE_TARGET_OS_BARE			0
#define ZOE_TARGET_OS_ZEOS			1
#define ZOE_TARGET_OS_FREERTOS		2
#define ZOE_TARGET_OS_LINUX_KER		3
#define ZOE_TARGET_OS_LINUX_USR		4
#define ZOE_TARGET_OS_WIN_KER		5
#define ZOE_TARGET_OS_WIN_USR		6
#define ZOE_TARGET_OS_ANDRO_KER		7
#define ZOE_TARGET_OS_ANDRO_USR		8

#define ZOE_TARGET_OS_MAX			65535	// not to exceed this value

// ZOE TARGET_PUS is a bit map of 32 bits
// Sets of more than 1 PU can be defined as long as they have the same ISA and OS
#define ZOE_TARGET_PU_SPU_BIT		0
#define ZOE_TARGET_PU_HPU_BIT		1
#define ZOE_TARGET_PU_FWPU_BIT		2
#define ZOE_TARGET_PU_EDPU_BIT		3
#define ZOE_TARGET_PU_EEPU_BIT		4
#define ZOE_TARGET_PU_MEPU_BIT		5
#define ZOE_TARGET_PU_DMAPU_BIT		6
#define ZOE_TARGET_PU_AUD0PU_BIT	7
#define ZOE_TARGET_PU_AUD1PU_BIT	8
#define ZOE_TARGET_PU_AUD2PU_BIT	9
#define ZOE_TARGET_PU_AUD3PU_BIT	10
#define ZOE_TARGET_PU_EXT_BIT		11
// Add PUs here and add string to the ZOE_TARGET_PU_STRING macro below
#define ZOE_TARGET_PU_END_BIT		32	// No PUs after this

// Single-PU sets simply have a single bit set
#define ZOE_TARGET_PUSET_SPU		(1<<ZOE_TARGET_PU_SPU_BIT)
#define ZOE_TARGET_PUSET_HPU		(1<<ZOE_TARGET_PU_HPU_BIT)
#define ZOE_TARGET_PUSET_FWPU		(1<<ZOE_TARGET_PU_FWPU_BIT)
#define ZOE_TARGET_PUSET_EDPU		(1<<ZOE_TARGET_PU_EDPU_BIT)
#define ZOE_TARGET_PUSET_EEPU		(1<<ZOE_TARGET_PU_EEPU_BIT)
#define ZOE_TARGET_PUSET_MEPU		(1<<ZOE_TARGET_PU_MEPU_BIT)
#define ZOE_TARGET_PUSET_DMAPU		(1<<ZOE_TARGET_PU_DMAPU_BIT)
#define ZOE_TARGET_PUSET_AUD0PU		(1<<ZOE_TARGET_PU_AUD0PU_BIT)
#define ZOE_TARGET_PUSET_AUD1PU		(1<<ZOE_TARGET_PU_AUD1PU_BIT)
#define ZOE_TARGET_PUSET_AUD2PU		(1<<ZOE_TARGET_PU_AUD2PU_BIT)
#define ZOE_TARGET_PUSET_AUD3PU		(1<<ZOE_TARGET_PU_AUD3PU_BIT)
#define ZOE_TARGET_PUSET_EXT		(1<<ZOE_TARGET_PU_EXT_BIT)
// Multi-PU sets are chip-dependent
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
#define ZOE_TARGET_PUSET_FWSET (ZOE_TARGET_PUSET_EDPU + ZOE_TARGET_PUSET_EEPU + ZOE_TARGET_PUSET_MEPU + ZOE_TARGET_PUSET_DMAPU + ZOE_TARGET_PUSET_AUD0PU + ZOE_TARGET_PUSET_AUD1PU)
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
// No PU sets defined
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
// No PU sets defined
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
// No PU sets defined
#else
	#error "Chip not supported"
#endif

#define ZOE_TARGET_PUSET_AUDPU_MASK (ZOE_TARGET_PUSET_AUD0PU + ZOE_TARGET_PUSET_AUD1PU + ZOE_TARGET_PUSET_AUD2PU + ZOE_TARGET_PUSET_AUD3PU)
#if ((ZOE_TARGET_PUSET & ZOE_TARGET_PUSET_AUDPU_MASK) != 0)
	#define ZOE_TARGET_PUSET_HAS_AUDIO
	#if ((ZOE_TARGET_PUSET & ~ZOE_TARGET_PUSET_AUDPU_MASK) == 0)
		#define ZOE_TARGET_PUSET_IS_AUDIOPUSET
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Define all image strings (C/C++ only)
//

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

#define ZOE_TARGET_PU_STRING(i) (	\
	(i == 0) ? "SPU" :				\
	(i == 1) ? "HPU" :				\
	(i == 2) ? "FWPU" :				\
	(i == 3) ? "EDPU" :				\
	(i == 4) ? "EEPU" :				\
	(i == 5) ? "MEPU" :				\
	(i == 6) ? "DMAPU" :			\
	(i == 7) ? "AUD0PU" :			\
	(i == 8) ? "AUD1PU" :			\
	(i == 9) ? "AUD2PU" :			\
	(i == 10) ? "AUD3PU" :			\
	(i == 11) ? "EXTPU" :			\
	"Invalid CPU")

#define ZOE_TARGET_PU_LC_STRING(i) (	\
	(i == 0) ? "spu" :				\
	(i == 1) ? "hpu" :				\
	(i == 2) ? "fwpu" :				\
	(i == 3) ? "edpu" :				\
	(i == 4) ? "eepu" :				\
	(i == 5) ? "mepu" :				\
	(i == 6) ? "dmapu" :			\
	(i == 7) ? "aud0pu" :			\
	(i == 8) ? "aud1pu" :			\
	(i == 9) ? "aud2pu" :			\
	(i == 10) ? "aud3pu" :			\
	(i == 11) ? "extpu" :			\
	"Invalid CPU")

#define ZOE_TARGET_OS_STRING(i) (	\
	(i == 0) ? "Bare metal" :		\
	(i == 1) ? "ZeOS" :				\
	(i == 2) ? "FreeRTOS" :			\
	(i == 3) ? "Linux kernel" :		\
	(i == 4) ? "Linux user" :		\
	(i == 5) ? "Windows kernel" :	\
	(i == 6) ? "Windows user" :		\
	(i == 7) ? "Android kernel" :	\
	(i == 8) ? "Android user" :		\
	"Invalid OS")

#define ZOE_TARGET_CHIP_STRING(i) (	\
	(i == 0) ? "Chisel" :			\
	(i == 1) ? "Dawn" :				\
	(i == 2) ? "iMX6" :				\
	(i == 3) ? "iMX8" :				\
	"Invalid CHIP")

#define ZOE_TARGET_CHIP_VAR_STRING(i) (	\
	(i == 0) ? "" :					\
	(i == 1) ? "DV" :				\
	(i == 2) ? "QM" :				\
	(i == 3) ? "Z" :				\
	(i == 4) ? "DQ" :				\
	"Invalid CHIP variant")

#endif // !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

///////////////////////////////////////////////////////////////////////////////
//
// Define legacy constants
//

#if (ZOE_TARGET_OS == ZOE_TARGET_OS_LINUX_KER)
	#ifndef ZOE_LINUXKER_BUILD
		#define ZOE_LINUXKER_BUILD
	#endif
#elif (ZOE_TARGET_OS == ZOE_TARGET_OS_LINUX_USR)
	#ifndef ZOE_LINUXUSER_BUILD
    	#define ZOE_LINUXUSER_BUILD
	#endif
#elif (ZOE_TARGET_OS == ZOE_TARGET_OS_WIN_KER)
	#ifndef ZOE_WINKER_BUILD
    	#define ZOE_WINKER_BUILD
	#endif
#elif (ZOE_TARGET_OS == ZOE_TARGET_OS_WIN_USR)
	#ifndef ZOE_WINUSER_BUILD
    	#define ZOE_WINUSER_BUILD
	#endif
#endif

#endif //__ZOE_TARGET_H__
