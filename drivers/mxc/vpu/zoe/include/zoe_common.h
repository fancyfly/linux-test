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
*                   (C) COPYRIGHT 2012-2014 Zenverge Inc.                     *
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


///////////////////////////////////////////////////////////////////////////////
//
// Definitions for structures shared between firmware and other applications.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_COMMON_H__
#define __ZOE_COMMON_H__


#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

#include "zoe_types.h"

#ifdef __cplusplus
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Information structure at the start of a firmware executable image.
// All firmware executable images (.bin files) start with data organized
// according to this structure.
// This same structure is present in memory at the start of the firmware partition.
// The meaning of the sizes and addresses varies between the type of firmware.
// For some firmware types (ROM, boot code) the firmware is not all contained
// in a single contiguous block of memory and therefore the size parameter
// might not be meaningful.
// The 'link_addr' and 'size' values are set at build time.
// The 'load_addr' and 'move_addr' values are physical addresses set by the
// startup code at runtime and therefore they are meaningless in the binary
// file.
// Some firmware types will move things around in memory and therefore have
// a move_addr different from load_addr.  This is done to support the new
// requirements for position-independent code (MX8).  The firmware partition
// still takes the same physical memory block (defined by 'load_addr' and 'size')
// but the code is relocated to be aligned in some way that allows for more
// efficient virtual memory translation schemes.
// All fields in this structure must be aligned to their size.
//


typedef struct {
	uint32_t	jump;				// instruction to jump past this ID structure
	uint32_t	delaySlot;			// MIPS needs a nop after a jump
	uint32_t	signature;			// fourCC code with check value (ZOE_IMAGE_SIGNATURE)
	uint8_t		chipID;				// code to identify the target chip
	uint8_t		chipVarID;			// code to identify the target chip variant
	uint16_t	osID;				// code to identify the target OS
	uint32_t	puset;				// bitmap of PUs in this target
	uint32_t	part_size4k;		// size in 4kB pages used by the firmware partition (code, data, heap, ...)
	// Make sure these are 64-bit aligned
	uint64_t	link_addr;			// virtual address at which the binary image has been linked
	uint64_t	load_addr;			// phy address at which the binary image has been loaded
	uint64_t	move_addr;			// phy address at which the binary image has been moved
    char		strings[1];			// NULL-terminated strings with application name and revision numbers
} zoe_image_start_t, *zoe_image_start_ptr_t;


///////////////////////////////////////////////////////////////////////////////
//
//	Shared IO buffers
//
// Used to implement a UART-less stdio in the firmware; the buffers are embedded
// inside the firmware's RW data following a block of repeated tags (so they can
// be easily discovered by an application searching through the loaded firmware
// memory partition). The tags are 64-bit values.  The block of tags is followed
// by a different check tags to indicate the start of the shared buffers structure.
// Tags and structures must be stored at 64-bit aligned addresses.
//
//
// Generic shared memory IO buffer.  One byte should always be left available,
// when the read and write offsets are equal the buffer is empty.  This avoid
// ambiguities and allows for separate agents to modify the read and write
// offsets without need for synchronization mechanisms (assuming that reading
// and writing the read and write offsets is atomic).
// Buffer address must be 64-bit-aligned. Size is in 32-bit words.


#define ZOE_SHBUFIO_TAG				ZOE_EIGHTCC('Z','s','h','b','u','f','i','o')
#define ZOE_SHBUFIO_TAGS_NUM		8
#define ZOE_SHBUFIO_START_CHECK		ZOE_EIGHTCC('s','h','b','u','f','s','t','r')

typedef struct {
	uint64_t        start;		// physical address of first byte of the buffer
	uint32_t        words;		// size of buffer in words (zero means buffer IO not enabled)
	uint32_t	    read;		// offset of last byte read
	uint32_t	    write;		// offset of next byte to write
} zoe_shbufio_buffer_descr_t, *zoe_shbufio_buffer_descr_ptr_t;

typedef struct {
	uint64_t						tags[ZOE_SHBUFIO_TAGS_NUM];		// pattern to facilitate finding the buffers
	uint64_t						check;							// ZOE_SHBUFIO_START_CHECK when the structure is valid
	zoe_shbufio_buffer_descr_t		stdin_buffer_descr;
	zoe_shbufio_buffer_descr_t		stdout_buffer_descr;
	zoe_shbufio_buffer_descr_t		stderr_buffer_descr;
} zoe_shbufio_info_t, *zoe_shbufio_info_ptr_t;


////////////////////////////////////////////////////////////////////////////////
//
//	Legacy definitions for low memory common area
//

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
	#define ZOE_COMMON_DDR_START 0
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
	#define ZOE_COMMON_DDR_START 0
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
	#define ZOE_COMMON_DDR_START 0x10000000
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
	#define ZOE_COMMON_DDR_START 0x80000000
#else
	#error Unsupported CHIP
#endif
///////////////////////////////////////////////////////////////////////////////
// IPC
#define ZOE_COMMON_IPC_PRIVATE_AREA_START (0x80000 + ZOE_COMMON_DDR_START)
#define ZOE_COMMON_IPC_PRIVATE_AREA_SIZE 0x80000



#ifdef __cplusplus
}
#endif

#endif //!defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

#endif //__ZOE_COMMON_H__

