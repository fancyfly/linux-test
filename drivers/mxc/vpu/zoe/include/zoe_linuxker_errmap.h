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
// zoe_linuxker_errmap.h
//
// Description: 
//
//  error code mapping between ZOE and Linux Kernel
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __ZOE_LINUXKER_ERRMAP_H__
#define __ZOE_LINUXKER_ERRMAP_H__

#include "zoe_types.h"
#include <linux/errno.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


STATIC_INLINE long ZoeToLinuxKerStatus(zoe_errs_t err)
{
	switch (err)
	{
		case ZOE_ERRS_SUCCESS:  return (0);
		case ZOE_ERRS_PENDING:  return (0);
		case ZOE_ERRS_TIMEOUT:  return (-ETIME);
		case ZOE_ERRS_NOMEMORY: return (-ENOMEM);
		case ZOE_ERRS_INVALID:	return (-EPERM);
		case ZOE_ERRS_PARMS:    return (-EINVAL);
		case ZOE_ERRS_NOTFOUND: return (-ENODEV);
		case ZOE_ERRS_FAIL:     return (-EIO);
		case ZOE_ERRS_NOTIMPL:  return (-EPERM);
		case ZOE_ERRS_NOTSUPP:  return (-EPERM);
        case ZOE_ERRS_BUSY:     return (-EBUSY);
		case ZOE_ERRS_CANCELLED:return (-EBUSY);
        case ZOE_ERRS_AGAIN:    return (-EAGAIN); 
        case ZOE_ERRS_INTERNAL: return (-EIO);
		default:                return (-EIO);
	}
}



STATIC_INLINE zoe_errs_t LinuxKerToZoeStatus(long lErr)
{
	switch (lErr)
	{
		case 0:         return (ZOE_ERRS_SUCCESS);
		case -EIO:      return (ZOE_ERRS_FAIL);
		case -ENOMEM:   return (ZOE_ERRS_NOMEMORY);
		case -EINVAL:   return (ZOE_ERRS_PARMS);
		case -EPERM:    return (ZOE_ERRS_NOTIMPL);
		case -EBUSY:    return (ZOE_ERRS_TIMEOUT);
		case -ETIME:    return (ZOE_ERRS_TIMEOUT);
		case -ETIMEDOUT:return (ZOE_ERRS_TIMEOUT);
		case -ENODEV:   return (ZOE_ERRS_NOTFOUND);
        case -EAGAIN:   return (ZOE_ERRS_AGAIN);
		default:        return (ZOE_ERRS_FAIL);
	}
}


#ifdef __cplusplus
}
#endif //__cplusplus


#endif //__ZOE_LINUXKER_ERRMAP_H__

