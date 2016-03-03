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
// izvusbfw.h
//
// Description: 
//
//  ZOE USB firmware command interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __IZVUSBFW_H__
#define __IZVUSBFW_H__

#include "zoe_types.h"


// USB firmware end point index
//
enum
{
    ZVUSB_EP_DATA_IN = 0,
    ZVUSB_EP_DATA_OUT,
    ZVUSB_EP_CMD_RESP,
    ZVUSB_EP_CMD_OUT,
    ZVUSB_EP_UNBLOCK,
    ZVUSB_EP_RESERVE,
    ZVUSB_EP_NR_MAX
};


// USB firmware commands
//
typedef enum _ZVUSB_CMD_TYPE
{
    ZVUSB_CMD_WRITE_MEM_DIRECT  = 0,
    ZVUSB_CMD_READ_MEM_DIRECT   = 1,
    ZVUSB_CMD_WRITE_MEM_COPY    = 2,
    ZVUSB_CMD_READ_MEM_COPY     = 3,

} ZVUSB_CMD_TYPE;


#pragma pack(1)

// USB command
//
typedef struct _USBFW_COMMAND
{
    uint32_t    cmd_len;
    uint32_t    cmd_cnt;
    ZVUSB_CMD_TYPE  cmd_type;
    uint8_t     cmd_data[0];
} USBFW_COMMAND, *PUSBFW_COMMAND;


// USB reply
//
typedef struct _USBFW_REPLY
{
    uint32_t    reply_len;
    uint32_t    cmd_cnt;
    uint8_t     reply_data[0];
} USBFW_REPLY, *PUSBFW_REPLY;

#define USBFW_REPLY_DESCR_SIZE  sizeof(USBFW_REPLY)


// ZV_USB_WRITE_MEM_DIRECT, ZV_USB_READ_MEM_DIRECT
//
typedef struct _USBFW_CMD_DATA_MEM_DIRECT 
{
    uint32_t    mem_ptr;
    uint32_t    data_length;
} USBFW_CMD_DATA_MEM_DIRECT, *PUSBFW_CMD_DATA_MEM_DIRECT;


// ZV_USB_WRITE_MEM_COPY, ZV_USB_READ_MEM_COPY
//
typedef struct _USBFW_CMD_DATA_MEM_COPY
{
    uint32_t    mem_ptr;
    uint32_t    data_length;
    uint32_t    data_type;
    uint8_t     data[0];
} USBFW_CMD_DATA_MEM_COPY, *PUSBFW_CMD_DATA_MEM_COPY;


#pragma pack()


#endif //__IZVUSBFW_H__


