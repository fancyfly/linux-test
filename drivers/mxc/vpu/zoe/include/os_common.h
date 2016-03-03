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
*                  (C) COPYRIGHT 2011-2014 Zenverge Inc.                      *
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

///////////////////////////////////////////////////////////////////////////////
//
//  OS interface definitions common to Host and Device OSes
//
// AUTHORS: Enrico Cadorin (EC)
// CREATED: Sep 20, 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OS_COMMON_H__
#define __OS_COMMON_H__


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#ifdef ZOE_DEBUG
extern void OSDbgInit (void);
extern void OSDbgTerm (void);
#else
#define OSDbgInit()
#define OSDbgTerm()
#endif


// Lock: This is a low level lock (possibly same as a spinlock) which is guaranteed
// to protect code in threads and ISRs and is always available, even before sosal_init.
// It is used by the OS-independent stubs

typedef void * TOSLockID;
extern TOSLockID OSLockCreate (zoe_sosal_obj_name_t name);
extern void OSLockDelete (TOSLockID lockID);
extern void OSLockGet (TOSLockID lockID);
extern void OSLockRelease (TOSLockID lockID);


#ifdef __cplusplus
}
#endif

#endif //__OS_COMMON_H__
