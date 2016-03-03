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
// zoe_module_connection_intf_clnt.c
//
// Description:
//  ZOERPCGEN Generated Client Stub File.
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
#include "zoe_module_connection_intf_clnt.h"
#include "zoe_module_connection_intf_srv.h"




// API zoe_module_connection_intf Client Function : 0
//
uint32_t zoe_module_base_get_num_ports_clnt(
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 16; // input size including XDR header
    uint32_t       outSize = 20; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    uint32_t   zoe_module_base_get_num_ports_ret = (uint32_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_get_num_ports_srv(pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 16 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_num_ports_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_num_ports_exit;
        }

        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)0);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)0);

        // prepare header
        msg_size = (sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 0, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_get_num_ports_ret) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);


            // unpack output arguments from ZOEXDR stream
        }
zoe_module_base_get_num_ports_exit:
        return (zoe_module_base_get_num_ports_ret);
    }
}

// API zoe_module_connection_intf Client Function : 1
//
zoe_errs_t zoe_module_base_get_port_selector_from_index_clnt(
    uint32_t index,
    uint32_t* sel,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 20; // input size including XDR header
    uint32_t       outSize = 24; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_get_port_selector_from_index_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_get_port_selector_from_index_srv( index, sel, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_port_selector_from_index_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 24 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_port_selector_from_index_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(index));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)1);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 1, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_get_port_selector_from_index_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
            // unpack one field
            *(sel) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

        }
zoe_module_base_get_port_selector_from_index_exit:
        return (zoe_module_base_get_port_selector_from_index_ret);
    }
}

// API zoe_module_connection_intf Client Function : 2
//
zoe_errs_t zoe_module_base_get_port_dir_clnt(
    uint32_t sel,
    uint32_t* dir,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 20; // input size including XDR header
    uint32_t       outSize = 24; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_get_port_dir_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_get_port_dir_srv( sel, dir, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_port_dir_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 24 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_port_dir_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(sel));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)2);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 2, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_get_port_dir_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
            // unpack one field
            *(dir) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

        }
zoe_module_base_get_port_dir_exit:
        return (zoe_module_base_get_port_dir_ret);
    }
}

// API zoe_module_connection_intf Client Function : 3
//
zoe_errs_t zoe_module_base_get_port_type_clnt(
    uint32_t sel,
    uint32_t* type,
    uint32_t* sub_type,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 20; // input size including XDR header
    uint32_t       outSize = 28; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_get_port_type_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_get_port_type_srv( sel, type, sub_type, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_port_type_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 28 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_get_port_type_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(sel));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)3);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 3, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_get_port_type_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
            // unpack one field
            *(type) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            *(sub_type) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

        }
zoe_module_base_get_port_type_exit:
        return (zoe_module_base_get_port_type_ret);
    }
}

// API zoe_module_connection_intf Client Function : 4
//
zoe_errs_t zoe_module_base_is_port_set_clnt(
    uint32_t sel,
    zoe_bool_t* set,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 20; // input size including XDR header
    uint32_t       outSize = 24; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_is_port_set_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_is_port_set_srv( sel, set, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_is_port_set_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 24 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_is_port_set_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(sel));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)4);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 4, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_is_port_set_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
            // unpack one field
            *(set) = (zoe_bool_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_bool_t);
            curLen += sizeof(zoe_bool_t);

        }
zoe_module_base_is_port_set_exit:
        return (zoe_module_base_is_port_set_ret);
    }
}

// API zoe_module_connection_intf Client Function : 5
//
zoe_errs_t zoe_module_base_port_set_clnt(
    uint32_t sel,
    ZOE_MODULE_DATA_CONNECTOR* port,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 36; // input size including XDR header
    uint32_t       outSize = 20; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_port_set_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_port_set_srv( sel, port, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 36 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_port_set_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_port_set_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(sel));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);


        // pack structure begin
        // pack structure begin
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(port->addr.cpu));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);

        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(port->addr.module));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);

        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(port->addr.inst));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);

        // pack structure end

        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(port->selector));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);

        // pack structure end



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)5);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 5, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_port_set_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
        }
zoe_module_base_port_set_exit:
        return (zoe_module_base_port_set_ret);
    }
}

// API zoe_module_connection_intf Client Function : 6
//
zoe_errs_t zoe_module_base_port_clear_clnt(
    uint32_t sel,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 20; // input size including XDR header
    uint32_t       outSize = 20; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_port_clear_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_port_clear_srv( sel, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_port_clear_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_port_clear_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(sel));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)6);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 6, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_port_clear_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
        }
zoe_module_base_port_clear_exit:
        return (zoe_module_base_port_clear_ret);
    }
}

// API zoe_module_connection_intf Client Function : 7
//
zoe_errs_t zoe_module_base_port_get_clnt(
    uint32_t sel,
    ZOE_MODULE_DATA_CONNECTOR* port,
    ZOE_IPC_CPU cpu_id,
    uint32_t module,
    uint32_t inst
    )
{
    uint32_t       msg_size; // entrie message size including XDR header in unit of 32 bit word
    CZoeIPCService *p_zoe_ipc_srv = c_zoe_ipc_service_get_ipc_svc();
    ZOE_IPC_CPU    to_cpu;
    zoe_sosal_isr_sw_numbers_t  isr_num = zoe_sosal_isr_sw_my_isr_num();
    ZOE_IPC_CPU    from_cpu = c_zoe_ipc_service_get_cpu_from_interrupt(isr_num);
    void           *pContext = c_zoe_ipc_service_get_interface_context(p_zoe_ipc_srv, zoe_module_connection_intf_interface, module, inst);
    zoe_errs_t     err;
    ZOE_IPC_MSG    msg; // rpc request
    ZOE_IPC_MSG    resp; // rpc response
    void           *inBuffer = (void *)&msg.param[0]; // input buffer
    void           *outBuffer = (void *)&resp.param[0]; // output buffer
    uint32_t       inSize = 20; // input size including XDR header
    uint32_t       outSize = 36; // output size including XDR header
    char           *curPtr; // data payload pointer
    uint32_t       curLen = 0; // current data payload length
    PZOEXDR_STREAM_HDR pHdr; // ZOEXDR header pointer
    zoe_errs_t   zoe_module_base_port_get_ret = (zoe_errs_t)0; // return value

    to_cpu = (ZOE_IPC_CPU)cpu_id;
    // check to and from cpu
    if (to_cpu == from_cpu)
    {
        return zoe_module_base_port_get_srv( sel, port, pContext);
    }
    else
    {

        // check buffer size
        //
        if (inSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("inSize 20 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_port_get_exit;
        }
        if (outSize > ZOE_IPC_MSG_PARAM_SIZE)
        {
            zoe_dbg_printf_nc_e("outSize 36 exceeding zoe_ipc buffer size 252\n ");
            goto zoe_module_base_port_get_exit;
        }

        // pointer to the input data buffer
        curPtr = (char *)inBuffer + sizeof(ZOEXDR_STREAM_HDR);

        // pack input data into ZOEXDR stream
        // pack one field
        *((uint32_t *)curPtr) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)(sel));
        curPtr += sizeof(uint32_t);
        curLen += sizeof(uint32_t);



        // generate ZOEXDR stream header
        pHdr = (PZOEXDR_STREAM_HDR)inBuffer;
        pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)_ZOEXDR_VERSION);
        pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)zoe_module_connection_intf_version);
        pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)7);
        pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_ENCODE, (int32_t)curLen);

        // prepare header
        msg_size = (curLen + sizeof(ZOEXDR_STREAM_HDR) + 3) >> 2;
        ZOE_IPC_PREPARE_REQUEST(&msg.hdr, 7, zoe_module_connection_intf_interface, module, inst, ZOE_TRUE, msg_size);
        // call the zoe ipc transport function
        err = c_zoe_ipc_service_send_message(p_zoe_ipc_srv, to_cpu, &msg, &resp, 5000);
        if (ZOE_FAIL(err))
        {
            // send messgae failed
            zoe_dbg_printf_nc_e("c_zoe_ipc_service_send_message FAILED err(%d)\n ", err);
        }
        else
        {
            // get output XDR header
            pHdr = (PZOEXDR_STREAM_HDR)outBuffer;

            pHdr->ulXDRVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulXDRVersion);
            pHdr->ulAPIVersion = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulAPIVersion);
            pHdr->ulFunctionID = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulFunctionID);
            pHdr->ulDataLength = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, (int32_t)pHdr->ulDataLength);

            // validate output XDR header
            if (pHdr->ulXDRVersion != _ZOEXDR_VERSION)
                zoe_dbg_printf_nc_w("Invalid XDR version received.\n ");
            if (pHdr->ulAPIVersion != zoe_module_connection_intf_version)
                zoe_dbg_printf_nc_w("Invalid API version received.\n ");

            // pointer to the output data buffer
            curPtr = (char *)outBuffer + sizeof(ZOEXDR_STREAM_HDR);
            curLen = 0;

            // unpack return value from ZOEXDR stream
            // unpack one field
            (zoe_module_base_port_get_ret) = (zoe_errs_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(zoe_errs_t);
            curLen += sizeof(zoe_errs_t);


            // unpack output arguments from ZOEXDR stream
            // unpack structure begin
            // unpack structure begin
            // unpack one field
            (port->addr.cpu) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (port->addr.module) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack one field
            (port->addr.inst) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack structure end

            // unpack one field
            (port->selector) = (uint32_t)ZOEXDR_int_32(_ZOEXDR_DECODE, *((int32_t *)curPtr));
            curPtr += sizeof(uint32_t);
            curLen += sizeof(uint32_t);

            // unpack structure end

        }
zoe_module_base_port_get_exit:
        return (zoe_module_base_port_get_ret);
    }
}

