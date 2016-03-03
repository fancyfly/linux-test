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

//----------------------------------------------------------------------------
// zoe_module_mgr_intf_srv.c
//
// Description:
//  ZOERPCGEN Generated Server Stub File.
//
// Author: 
//	ZOERPCGEN
//
//----------------------------------------------------------------------------

#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_sosal.h"
#include "zoe_ipc_srv.h"
#include "zoe_xdr.h"
#include "zoe_module_mgr_intf_srv.h"


// API zoe_module_mgr_intf Dispatch Function
//
zoe_errs_t zoe_module_mgr_intf_dispatch(
    void *pContext,
    ZOE_IPC_CPU from_cpu,
    PZOE_IPC_MSG pMsg
    )
{
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    zoe_errs_t     err;
    ZOE_IPC_MSG    resp; // rpc response
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    void           *inBuffer = (void *)&pMsg->param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize; // input buffer size
    uint32_t       outSize = 0;// output buffer size
    char           *curPtr; // data payload pointer 
    uint32_t       curLen;  // current data payload length
    uint32_t       ID; // saved ID

    // generate ZOEXDR stream header
    pHdr = (PZOEXDR_STREAM_HDR)inBuffer;

    // convert to host byte order
    pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
    pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
    pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
    pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

    // validate XDR stream header
    if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
        return (ZOE_ERRS_INVALID);
    if (pHdr->ulAPIVersion != zoe_module_mgr_intf_version)
        return (ZOE_ERRS_INVALID);

    // get input buffer size excluding ZOEXDR header
    inSize = pHdr->ulDataLength;
    // save dispatch ID
    ID = pHdr->ulFunctionID;

    switch (ID)
    {
        case 0:
        {
            zoe_errs_t zoe_module_mgr_create_module_ret; // return value
            uint32_t module_create;
            uint32_t inst_ret;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 0 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (module_create) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_mgr_create_module_ret = zoe_module_mgr_create_module_srv(module_create,&inst_ret,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_mgr_create_module_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream
            // pack one field
            *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(inst_ret));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // update output size
            outSize = curLen;
            if (outSize != 8)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 0 out size mismatch expect 8 got %d\r\n", outSize);
            }
            break;
        }
        case 1:
        {
            zoe_errs_t zoe_module_mgr_destroy_module_ret; // return value
            uint32_t module_destroy;
            uint32_t inst_destroy;

            // validate input size
            if (inSize != 8)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 1 in size mismatch expect 8 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (module_destroy) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            (inst_destroy) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_mgr_destroy_module_ret = zoe_module_mgr_destroy_module_srv(module_destroy,inst_destroy,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_mgr_destroy_module_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 1 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 2:
        {
            zoe_errs_t zoe_module_mgr_find_module_ret; // return value
            uint32_t module_find;
            uint32_t inst_find;

            // validate input size
            if (inSize != 8)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 2 in size mismatch expect 8 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (module_find) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            (inst_find) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_mgr_find_module_ret = zoe_module_mgr_find_module_srv(module_find,inst_find,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_mgr_find_module_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 2 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 3:
        {
            zoe_errs_t zoe_module_mgr_discover_module_ret; // return value
            uint32_t module_discover;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 3 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (module_discover) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_mgr_discover_module_ret = zoe_module_mgr_discover_module_srv(module_discover,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_mgr_discover_module_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_mgr_intf function 3 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        default:
            return (ZOE_ERRS_NOTFOUND);
    }

    // get output XDR header
    pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

    // update XDR header and convert to network order
    pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
    pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_mgr_intf_version);
    pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)ID);
    pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)outSize);

    // prepare header
    msg_size = (outSize + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
    ZOE_IPC_PREPARE_RESPONSE(&pMsg->hdr, &resp.hdr, msg_size);
    err = c_zoe_ipc_service_post_message(p_zoe_ipc_srv, from_cpu, &resp, ZOE_NULL);
    if (ZOE_FAIL(err))
    {
        zoe_dbg_printf_nc_e("zoe_module_mgr_intf_dispatch() c_zoe_ipc_service_post_message failed (%d)\r\n", err);
        return (err);
    }
    return (ZOE_ERRS_SUCCESS);

}

