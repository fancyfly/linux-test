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
// zoe_module_data_intf_srv.c
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
#include "zoe_module_data_intf_srv.h"


// API zoe_module_data_intf Dispatch Function
//
zoe_errs_t zoe_module_data_intf_dispatch(
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
    if (pHdr->ulAPIVersion != zoe_module_data_intf_version)
        return (ZOE_ERRS_INVALID);

    // get input buffer size excluding ZOEXDR header
    inSize = pHdr->ulDataLength;
    // save dispatch ID
    ID = pHdr->ulFunctionID;

    switch (ID)
    {
        case 0:
        {
            zoe_errs_t zoe_module_allocate_buffer_ret; // return value
            uint32_t sel;
            uint32_t buf_sel;
            uint32_t size;
            zoe_dev_mem_t dev_mem;
            uint32_t size_got;

            // validate input size
            if (inSize != 12)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 0 in size mismatch expect 12 got %d\r\n", inSize);
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
            (buf_sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            (size) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_allocate_buffer_ret = zoe_module_allocate_buffer_srv(sel,buf_sel,size,&dev_mem,&size_got,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_allocate_buffer_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream
            // pack one field
            *((zoe_dev_mem_t *)curPtr) = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_ENCODE, (int64_t)(dev_mem));
            curPtr += sizeof(zoe_dev_mem_t);
            curLen += sizeof(zoe_dev_mem_t);


            // pack one field
            *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(size_got));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // update output size
            outSize = curLen;
            if (outSize != 16)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 0 out size mismatch expect 16 got %d\r\n", outSize);
            }
            break;
        }
        case 1:
        {
            zoe_errs_t zoe_module_release_buffer_ret; // return value
            uint32_t sel;
            uint32_t buf_sel;
            zoe_dev_mem_t dev_mem;
            uint32_t size;

            // validate input size
            if (inSize != 20)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 1 in size mismatch expect 20 got %d\r\n", inSize);
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
            (buf_sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            (dev_mem) = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(zoe_dev_mem_t);
            curLen += sizeof(zoe_dev_mem_t);


            // unpack one field
            (size) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_release_buffer_ret = zoe_module_release_buffer_srv(sel,buf_sel,dev_mem,size,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_release_buffer_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 1 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 2:
        {
            zoe_errs_t zoe_module_release_buffer_with_info_ret; // return value
            uint32_t sel;
            uint32_t buf_sel;
            zoe_dev_mem_t dev_mem;
            uint32_t size;
            ZOE_BUFFER_INFO buf_info;

            // validate input size
            if (inSize != 48)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 2 in size mismatch expect 48 got %d\r\n", inSize);
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
            (buf_sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            (dev_mem) = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(zoe_dev_mem_t);
            curLen += sizeof(zoe_dev_mem_t);


            // unpack one field
            (size) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack structure begin
            // unpack one field
            (buf_info.pts) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack one field
            (buf_info.dts) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack one field
            (buf_info.flags) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (buf_info.context) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack structure end



            // call API
            zoe_module_release_buffer_with_info_ret = zoe_module_release_buffer_with_info_srv(sel,buf_sel,dev_mem,size,&buf_info,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_release_buffer_with_info_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 2 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 3:
        {
            zoe_errs_t zoe_module_allocate_yuv_buffer_ret; // return value
            uint32_t sel;
            uint32_t num_planes;
            uint32_t size[3];
            zoe_dev_mem_t dev_mem[3];

            // validate input size
            if (inSize != 20)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 3 in size mismatch expect 20 got %d\r\n", inSize);
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
            (num_planes) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 3; _i1_0++)
                {
                    size[_i1_0] = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);
                }
            }



            // call API
            zoe_module_allocate_yuv_buffer_ret = zoe_module_allocate_yuv_buffer_srv(sel,num_planes,size,dev_mem,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_allocate_yuv_buffer_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream
            // pack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 3; _i1_0++)
                {
                    *((zoe_dev_mem_t *)curPtr) = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_ENCODE, (int64_t)dev_mem[_i1_0]);
                    curPtr += sizeof(zoe_dev_mem_t);
                    curLen += sizeof(zoe_dev_mem_t);
                }
            }



            // update output size
            outSize = curLen;
            if (outSize != 28)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 3 out size mismatch expect 28 got %d\r\n", outSize);
            }
            break;
        }
        case 4:
        {
            zoe_errs_t zoe_module_release_yuv_buffer_ret; // return value
            uint32_t sel;
            uint32_t num_planes;
            zoe_dev_mem_t dev_mem[3];
            uint32_t size[3];

            // validate input size
            if (inSize != 44)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 4 in size mismatch expect 44 got %d\r\n", inSize);
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
            (num_planes) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 3; _i1_0++)
                {
                    dev_mem[_i1_0] = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
                    curPtr += sizeof(zoe_dev_mem_t);
                    curLen += sizeof(zoe_dev_mem_t);
                }
            }


            // unpack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 3; _i1_0++)
                {
                    size[_i1_0] = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);
                }
            }



            // call API
            zoe_module_release_yuv_buffer_ret = zoe_module_release_yuv_buffer_srv(sel,num_planes,dev_mem,size,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_release_yuv_buffer_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 4 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 5:
        {
            zoe_errs_t zoe_module_release_yuv_buffer_with_info_ret; // return value
            uint32_t sel;
            uint32_t num_planes;
            zoe_dev_mem_t dev_mem[3];
            uint32_t size[3];
            ZOE_BUFFER_INFO buf_info;

            // validate input size
            if (inSize != 72)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 5 in size mismatch expect 72 got %d\r\n", inSize);
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
            (num_planes) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 3; _i1_0++)
                {
                    dev_mem[_i1_0] = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
                    curPtr += sizeof(zoe_dev_mem_t);
                    curLen += sizeof(zoe_dev_mem_t);
                }
            }


            // unpack one field
            {
                int _i1_0;
                for (_i1_0 = 0; _i1_0 < 3; _i1_0++)
                {
                    size[_i1_0] = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);
                }
            }


            // unpack structure begin
            // unpack one field
            (buf_info.pts) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack one field
            (buf_info.dts) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack one field
            (buf_info.flags) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (buf_info.context) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack structure end



            // call API
            zoe_module_release_yuv_buffer_with_info_ret = zoe_module_release_yuv_buffer_with_info_srv(sel,num_planes,dev_mem,size,&buf_info,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_release_yuv_buffer_with_info_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 5 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 6:
        {
            zoe_errs_t zoe_module_get_mem_type_ret; // return value
            uint32_t sel;
            uint32_t mem_type;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 6 in size mismatch expect 4 got %d\r\n", inSize);
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
            zoe_module_get_mem_type_ret = zoe_module_get_mem_type_srv(sel,&mem_type,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_get_mem_type_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream
            // pack one field
            *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(mem_type));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // update output size
            outSize = curLen;
            if (outSize != 8)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 6 out size mismatch expect 8 got %d\r\n", outSize);
            }
            break;
        }
        case 7:
        {
            zoe_errs_t zoe_module_get_mem_usage_ret; // return value
            uint32_t sel;
            uint32_t mem_usage;

            // validate input size
            if (inSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 7 in size mismatch expect 4 got %d\r\n", inSize);
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
            zoe_module_get_mem_usage_ret = zoe_module_get_mem_usage_srv(sel,&mem_usage,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_get_mem_usage_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream
            // pack one field
            *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(mem_usage));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // update output size
            outSize = curLen;
            if (outSize != 8)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 7 out size mismatch expect 8 got %d\r\n", outSize);
            }
            break;
        }
        case 8:
        {
            zoe_errs_t zoe_module_write_ret; // return value
            uint32_t sel;
            ZOE_BUFFER_DESCRIPTOR buf_desc;

            // validate input size
            if (inSize != 104)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 8 in size mismatch expect 104 got %d\r\n", inSize);
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


            // unpack structure begin
            // unpack structure begin
            // unpack one field
            (buf_desc.info.pts) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack one field
            (buf_desc.info.dts) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack one field
            (buf_desc.info.flags) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (buf_desc.info.context) = (uint64_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
            curPtr += sizeof(uint64_t);
            curLen += sizeof(uint64_t);

            // unpack structure end

            // unpack structure begin
            // unpack structure begin
            // unpack one field
            (buf_desc.owner.addr.cpu) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (buf_desc.owner.addr.module) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (buf_desc.owner.addr.inst) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack structure end

            // unpack one field
            (buf_desc.owner.selector) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack structure end

            {
                int _i2_0;
                for (_i2_0 = 0; _i2_0 < 2; _i2_0++)
                {
                    // unpack structure begin
                    // unpack one field
                    (buf_desc.buffers[_i2_0].buf_ptr) = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
                    curPtr += sizeof(zoe_dev_mem_t);
                    curLen += sizeof(zoe_dev_mem_t);

                    // unpack one field
                    (buf_desc.buffers[_i2_0].buf_ptr_va) = (zoe_dev_mem_t)ZOEXDR_int_64(_ZOEXDR_DECODE, *((int64_t *)curPtr));
                    curPtr += sizeof(zoe_dev_mem_t);
                    curLen += sizeof(zoe_dev_mem_t);

                    // unpack one field
                    (buf_desc.buffers[_i2_0].size) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);

                    // unpack one field
                    (buf_desc.buffers[_i2_0].offset) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);

                    // unpack one field
                    (buf_desc.buffers[_i2_0].valid_size) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
                    curPtr += sizeof(uint32_t);
                    curLen += sizeof(uint32_t);

                    // unpack structure end

                }
            }
            // unpack structure end



            // call API
            zoe_module_write_ret = zoe_module_write_srv(sel,&buf_desc,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_write_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 8 out size mismatch expect 4 got %d\r\n", outSize);
            }
            break;
        }
        case 9:
        {
            zoe_errs_t zoe_module_buffer_available_ret; // return value
            uint32_t sel;
            uint32_t buf_sel;

            // validate input size
            if (inSize != 8)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 9 in size mismatch expect 8 got %d\r\n", inSize);
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
            (buf_sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);



            // call API
            zoe_module_buffer_available_ret = zoe_module_buffer_available_srv(sel,buf_sel,pContext);

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // pack return value into ZOEXDR stream */
            // pack one field
            *((zoe_errs_t *)curPtr) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(zoe_module_buffer_available_ret));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // pack output arguments into ZOEXDR stream

            // update output size
            outSize = curLen;
            if (outSize != 4)
            {
                zoe_dbg_printf_nc_e("zoe_module_data_intf function 9 out size mismatch expect 4 got %d\r\n", outSize);
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
    pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_data_intf_version);
    pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)ID);
    pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)outSize);

    // prepare header
    msg_size = (outSize + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
    ZOE_IPC_PREPARE_RESPONSE(&pMsg->hdr, &resp.hdr, msg_size);
    err = c_zoe_ipc_service_post_message(p_zoe_ipc_srv, from_cpu, &resp, ZOE_NULL);
    if (ZOE_FAIL(err))
    {
        zoe_dbg_printf_nc_e("zoe_module_data_intf_dispatch() c_zoe_ipc_service_post_message failed (%d)\r\n", err);
        return (err);
    }
    return (ZOE_ERRS_SUCCESS);

}

