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
// zoe_module_control_intf_srv.c
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
#include "zoe_module_control_intf_srv.h"


// API zoe_module_control_intf Dispatch Function
//
zoe_errs_t zoe_module_control_intf_dispatch(
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
    if (pHdr->ulAPIVersion != zoe_module_control_intf_version)
        return (ZOE_ERRS_INVALID);

    // get input buffer size excluding ZOEXDR header
    inSize = pHdr->ulDataLength;
    // save dispatch ID
    ID = pHdr->ulFunctionID;

    switch (ID)
    {
        case 0:
        {
            zoe_errs_t zoe_module_base_play_ret; // return value
            uint32_t sel;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 0 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_base_play_ret = zoe_module_base_play_srv(sel,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_base_play_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 0 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 1:
        {
            zoe_errs_t zoe_module_base_stop_ret; // return value
            uint32_t sel;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 1 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_base_stop_ret = zoe_module_base_stop_srv(sel,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_base_stop_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 1 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 2:
        {
            zoe_errs_t zoe_module_base_flush_ret; // return value
            uint32_t sel;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 2 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_base_flush_ret = zoe_module_base_flush_srv(sel,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_base_flush_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 2 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 3:
        {
            zoe_errs_t zoe_module_base_pause_ret; // return value
            uint32_t sel;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 3 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_base_pause_ret = zoe_module_base_pause_srv(sel,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_base_pause_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 3 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 4:
        {
            zoe_state_t zoe_module_base_get_state_ret; // return value
            uint32_t sel;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 4 in size mismatch expect 4 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_base_get_state_ret = zoe_module_base_get_state_srv(sel,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_state_t *)curPtr) = (zoe_state_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_base_get_state_ret));
            curPtr += sizeof(zoe_state_t);
            curLen += sizeof(zoe_state_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 4 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 5:
        {
            zoe_errs_t zoe_module_base_notify_ret; // return value
            uint32_t sel;
            uint32_t evt;
            uint32_t evt_data[4];

            // validate input size
            if (inSize != 24)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 5 in size mismatch expect 24 got %d\r\n", inSize);
                break;
            }
            // pointer to the input data buffer
            curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack input ZOEXDR stream into paramaters
            // unpack one field
            (sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            (evt) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 4; _i1_0++)
                {
                    evt_data[_i1_0] = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);
                }
            }



            // call API
            zoe_module_base_notify_ret = zoe_module_base_notify_srv(sel,evt,evt_data,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_base_notify_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_control_intf function 5 out size mismatch expect 4 got %d\r\n", outSize);
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
    pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_control_intf_version);
    pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)ID);
    pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)outSize);

    // prepare header
    msg_size = (outSize + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
    ZOE_IPC_PREPARE_RESPONSE(&pMsg->hdr, &resp.hdr, msg_size);
    err = c_zoe_ipc_service_post_message(p_zoe_ipc_srv, from_cpu, &resp, ZOE_NULL);
    if (ZOE_FAIL(err))
    {
        zoe_dbg_printf_nc_e("zoe_module_control_intf_dispatch() c_zoe_ipc_service_post_message failed (%d)\r\n", err);
        return (err);
    }
    return (ZOE_ERRS_SUCCESS);

}

