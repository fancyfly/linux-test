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
// zoe_objids.h
//
// Description: 
//
//	object id's for everything inherited from c_object
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_OBJIDS_H__
#define __ZOE_OBJIDS_H__


#include "zoe_cobject.h"

#define OBJECT_ZOE_IPC_SRV          (OBJECT_USER + 1)
#define OBJECT_ZOE_TASK             (OBJECT_USER + 2)
#define OBJECT_ZOE_CHANNEL          (OBJECT_USER + 3)
#define OBJECT_ZOE_ZVCODEC          (OBJECT_USER + 4)
#define OBJECT_ZOE_ZVAVLIB          (OBJECT_USER + 5)
#define OBJECT_ZOE_USB_CNTL         (OBJECT_USER + 6)
#define OBJECT_ZOE_USB_INTF         (OBJECT_USER + 7)
#define OBJECT_ZOE_PCIE_CNTL        (OBJECT_USER + 8)
#define OBJECT_ZOE_PCIE_INTF        (OBJECT_USER + 9)
#define OBJECT_ZOE_HPU_CNTL         (OBJECT_USER + 10)
#define OBJECT_ZOE_HPU_INTF         (OBJECT_USER + 11)
#define OBJECT_ZOE_BUF_DESC_QUEUE   (OBJECT_USER + 12)


// avstream object base
//
#define OBJECT_ZOE_AVSTREAM         (OBJECT_USER + 50)

// v4l2 object base
//
#define OBJECT_ZOE_V4L2             (OBJECT_USER + 100)

// sfw object base
//
#define OBJECT_ZOE_SFW              (OBJECT_USER + 150)

// vfw object base
//
#define OBJECT_ZOE_VFW              (OBJECT_USER + 200)

// afw object base
//
#define OBJECT_ZOE_AFW              (OBJECT_USER + 250)

// system object base
//
#define OBJECT_ZOE_SYS              (OBJECT_USER + 300)

// module object base
//
#define OBJECT_ZOE_MODULE           (OBJECT_USER + 350)


#endif //__ZOE_OBJIDS_H__



