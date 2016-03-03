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
// zoe_module_base.h
//
// Description: 
//
//  ZOE base module
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_MODULE_BASE_H__
#define __ZOE_MODULE_BASE_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_cthread.h"
#include "zoe_cfifo.h"
#include "zoe_cdata_fifo.h"
#include "zoe_dbg.h"
#include "zoe_hal.h"
#include "zoe_module_connection_intf.h"
#include "zoe_module_data_intf.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __ZOE_MODULE_BASE_FWD_DEFINED__
#define __ZOE_MODULE_BASE_FWD_DEFINED__
typedef struct c_zoe_module_base c_zoe_module_base;
typedef struct c_zoe_port c_zoe_port;
#endif //__ZOE_MODULE_BASE_FWD_DEFINED__


/////////////////////////////////////////////////////////////////////////////
//
//

#ifdef ZOE_WINKER_BUILD
#include <wdm.h>
#define MODULE_PRINTF DbgPrint
#else // !ZOE_WINKER_BUILD
#ifdef ZOE_LINUXKER_BUILD
#ifdef __cplusplus
#define MODULE_PRINTF
#else // !__cplusplus
#include <linux/kernel.h>
#define MODULE_PRINTF printk
#endif //__cplusplus
#else // !ZOE_LINUXKER_BUILD
#include "stdio.h"
#define MODULE_PRINTF printf
#endif // ZOE_LINUXKER_BUILD
#endif // ZOE_WINKER_BUILD



/////////////////////////////////////////////////////////////////////////////
//
//

// data selector index
//
#define ZOE_MODULE_DATA_SEL_MODULE      0
#define ZOE_MODULE_DATA_SEL_PORT_START  1


// data type
//
#define ZOE_MODULE_DATA_TYPE_NONE       0
#define ZOE_MODULE_DATA_TYPE_STREAM     1
#define ZOE_MODULE_DATA_TYPE_VID        2
#define ZOE_MODULE_DATA_TYPE_AUD        3
#define ZOE_MODULE_DATA_TYPE_INDEX      4

// sub data type
//
#define ZOE_MODULE_DATA_SUB_TYPE_NONE   0
#define ZOE_MODULE_DATA_SUB_TYPE_COMP   1
#define ZOE_MODULE_DATA_SUB_TYPE_YUV    2


// data direction
//
#define ZOE_MODULE_DATA_IN              0
#define ZOE_MODULE_DATA_OUT             1


// memory type for data
//
#define ZOE_MODULE_MEM_TYPE_INTERNAL    0
#define ZOE_MODULE_MEM_TYPE_EXTERNAL    1
#define ZOE_MODULE_MEM_TYPE_SOC         2
#define ZOE_MODULE_MEM_TYPE_EXT_USER    4
#define ZOE_MODULE_MEM_TYPE_SOC_USER    5


// memory usage for data
//
#define ZOE_MODULE_MEM_USAGE_COPY       0
#define ZOE_MODULE_MEM_USAGE_IN_PLACE   1


// maximun number of additional events for base module thread
//
#define ZOE_MODULE_MAX_EXTRA_EVTS       12


// default input buffer descriptor fifo depth
//
#define ZOE_MODULE_BUF_DESC_FIFO_DEPTH  64


// data request flags
//
#define ZOE_MODULE_DATA_REQ_BLOCKING    0x00000001


// buffer descriptor flags
//
#define ZOE_BUF_DESC_FLAGS_EOS          0x00000001
#define ZOE_BUF_DESC_FLAGS_PTS          0x00000002
#define ZOE_BUF_DESC_FLAGS_DTS          0x00000004
#define ZOE_BUF_DESC_FLAGS_CONTINUE     0x00000008
#define ZOE_BUF_DESC_FLAGS_SRC_CHANGE   0x00010000
#define ZOE_BUF_DESC_FLAGS_ERROR        0x00020000
#define ZOE_BUF_DESC_FLAGS_META         0x00000010
#define ZOE_BUF_DESC_FLAGS_SEP_CHROMA   0x00000020
#define ZOE_BUF_DESC_FLAGS_MAP_KERNEL   0x00000100
#define ZOE_BUF_DESC_FLAGS_MAP_USER     0x00000200
#define ZOE_BUF_DESC_FLAGS_MAP_RTOS     0x00000300

#define ZOE_BUF_DESC_FLAGS_MAP_MASK     0x00000F00



STATIC_INLINE zoe_bool_t buf_desc_2_bufs(ZOE_BUFFER_DESCRIPTOR *p_buf_desc) 
{
    return (0 != (p_buf_desc->info.flags & (ZOE_BUF_DESC_FLAGS_META | ZOE_BUF_DESC_FLAGS_SEP_CHROMA)));
}


// buffer selection
//
#define ZOE_BUF_DATA                    0
#define ZOE_BUF_META                    1
#define ZOE_BUF_LUMA                    ZOE_BUF_DATA
#define ZOE_BUF_CHROMA                  ZOE_BUF_META


// write to port method
//
typedef zoe_errs_t (*WRITE_TO_PORT_FUNC)(c_zoe_module_base *This,
                                         uint32_t port_index,
                                         ZOE_BUFFER_DESCRIPTOR *p_buf_desc,
                                         uint32_t req_flag,
                                         uint32_t written[2],
                                         zoe_void_ptr_t p_private[2]
                                         );


// state change command
//
#define ZOE_BASE_MODULE_THREAD_CMD_SET_STATE    1
#define ZOE_BASE_MODULE_THREAD_CMD_STOP_PORT    2



#define ZOE_BASE_CHECK_CONTEXT(cnxt) \
    if (!cnxt) \
    { \
        zoe_dbg_printf_nc(ZOE_DBG_LVL_ERROR, \
                          "%s NO Context!!!\n", \
                          __FUNCTION__ \
	                      ); \
        return (ZOE_ERRS_INVALID); \
    }




/////////////////////////////////////////////////////////////////////////////
//
//

// zoe port
//
struct c_zoe_port
{
    // port management
    //
    int32_t                     m_id;
    c_zoe_module_base           *m_p_base_module;

    // common port properties
    //
    ZOE_MODULE_DATA_CONNECTOR   m_port_conn;    // the address of the port this port is connected to
    // port type and subtype are initialized by derived module
    int32_t                     m_port_type;    // initialized by derived module
    int32_t                     m_port_sub_type;// initialized by derived module
    zoe_bool_t                  m_port_set;
    uint32_t                    m_port_dir;
    zoe_bool_t                  m_need_dma;
    volatile zoe_state_t        m_state;        // port state
    // per port event
    zoe_sosal_obj_id_t          m_evt_port;
    // dma completion event 
    zoe_sosal_obj_id_t          m_evt_dma;

    // data fifos, created on connection time if fifo size is not zero
    zoe_bool_t                  m_has_meta;
    uint32_t                    m_port_fifo_size[2];    // initialized by derived module
    c_data_fifo                 *m_p_data_fifo[2];

    // output port properties
    //
    uint32_t                    m_down_stream_mem_type;
    uint32_t                    m_down_stream_mem_usage;
    // write to down stream port method
    WRITE_TO_PORT_FUNC          m_p_write_2_port_func;

    // input port properties
    //
    // input port memory usage and data fifo size are initialized by derived module
    uint32_t                    m_input_port_mem_usage; // initialized by derived module
    // input port buffer descriptor fifos
    c_fifo	                    *m_p_input_bufdesc_fifo;
};



/////////////////////////////////////////////////////////////////////////////
//
//

// zoe base module
//
struct c_zoe_module_base
{
    // c_object
    //
    c_object                    m_object;

    // c_thread
    //
    c_thread                    m_thread;

    // c_zoe_module_base
    //
    IZOEHALAPI                  *m_p_hal;
    zoe_dbg_comp_id_t           m_dbg_id;
    uint32_t                    m_cpu;
    uint32_t                    m_inst;
    void                        *m_p_module;    // derived module
    volatile zoe_state_t        m_state;        // module state
    volatile zoe_state_t        m_state_to;     // state transition to
    volatile zoe_bool_t         m_flushing;     // flushing
    uint32_t                    m_mem_type;     // memory type
    uint32_t                    m_mem_mapping_flag; // buffer mapping required for the module
    zoe_sosal_obj_id_t          m_evt_ack_state;// ack event for state change thread command
    zoe_errs_t                  m_err_cmd_play; // play thread command return status
    zoe_errs_t                  m_err_cmd_stop; // stop thread command return status
    zoe_errs_t                  m_err_cmd_pause;// pause thread command return status
    zoe_bool_t                  m_create_module_thread; // module has thread
    zoe_bool_t                  m_create_rpc_thread;    // rpc interface use thread

    // data ports
    c_zoe_port                  *m_ports;
    uint32_t                    m_num_inputs;
    uint32_t                    m_num_outputs;

    // additional thread events
    uint32_t                    m_num_extra_events;
    zoe_sosal_obj_id_t          m_evt_extra[ZOE_MODULE_MAX_EXTRA_EVTS];
    // data process routine outer loop, leave the fifo along
    zoe_errs_t (*processes)(c_zoe_module_base *This, 
                            uint32_t port_index
                            );
    // data process routine inner loop, take one buffer descripter out of the fifo
    zoe_errs_t (*process)(c_zoe_module_base *This, 
                          uint32_t port_index, 
                          ZOE_BUFFER_DESCRIPTOR *p_buf_desc
                          );
    // extra event notification
    zoe_errs_t (*notify)(c_zoe_module_base *This, 
                         uint32_t evt
                         );
    // state transition handler, executed from the calling thread
    zoe_errs_t (*do_play)(c_zoe_module_base *This);
    zoe_errs_t (*do_stop)(c_zoe_module_base *This);
    zoe_errs_t (*do_pause)(c_zoe_module_base *This);
    // state transition handler, synchronized with the main thread
    zoe_errs_t (*do_play_cmd)(c_zoe_module_base *This);
    zoe_errs_t (*do_stop_cmd)(c_zoe_module_base *This);
    zoe_errs_t (*do_pause_cmd)(c_zoe_module_base *This);
    // unblock function for stop
    zoe_errs_t (*unblock)(c_zoe_module_base *This);

    // interfaces
    zoe_bool_t                  m_intf_connection_registered;
    zoe_bool_t                  m_intf_data_registered;
    zoe_bool_t                  m_intf_control_registered;

    // module control interface
    //
    zoe_errs_t (*play)(c_zoe_module_base *This, 
                       uint32_t sel
                       );
    zoe_errs_t (*stop)(c_zoe_module_base *This, 
                       uint32_t sel
                       );
    zoe_errs_t (*flush)(c_zoe_module_base *This, 
                        uint32_t sel
                        );
    zoe_errs_t (*pause)(c_zoe_module_base *This, 
                        uint32_t sel
                        );
    zoe_state_t (*get_state)(c_zoe_module_base *This, 
                             uint32_t sel
                             );
    zoe_state_t (*evt_notify)(c_zoe_module_base *This, 
                              uint32_t sel,
                              uint32_t evt,
                              uint32_t evt_data[4]
                              );

    // module data interface
    //
    zoe_errs_t (*allocate_buffer)(c_zoe_module_base *This, 
                                  uint32_t sel, 
                                  uint32_t buf_sel,
                                  uint32_t size, 
                                  zoe_dev_mem_t *p_dev_mem, 
                                  uint32_t *p_size_got
                                  );
    zoe_errs_t (*release_buffer)(c_zoe_module_base *This, 
                                 uint32_t sel, 
                                 uint32_t buf_sel,
                                 zoe_dev_mem_t dev_mem,
                                 uint32_t size
                                 );
    zoe_errs_t (*release_buffer_with_info)(c_zoe_module_base *This, 
                                           uint32_t sel, 
                                           uint32_t buf_sel,
                                           zoe_dev_mem_t dev_mem,
                                           uint32_t size,
                                           ZOE_BUFFER_INFO* buf_info
                                           );
    zoe_errs_t (*allocate_yuv_buffer)(c_zoe_module_base *This, 
                                      uint32_t sel, 
                                      uint32_t num_planes,
                                      uint32_t size[3], 
                                      zoe_dev_mem_t dev_mem[3]
                                      );
    zoe_errs_t (*release_yuv_buffer)(c_zoe_module_base *This, 
                                     uint32_t sel, 
                                     uint32_t num_planes,
                                     zoe_dev_mem_t dev_mem[3],
                                     uint32_t size[3]
                                     );
    zoe_errs_t (*release_yuv_buffer_with_info)(c_zoe_module_base *This, 
                                               uint32_t sel, 
                                               uint32_t num_planes,
                                               zoe_dev_mem_t dev_mem[3],
                                               uint32_t size[3],
                                               ZOE_BUFFER_INFO* buf_info
                                               );
    zoe_errs_t (*get_mem_type)(c_zoe_module_base *This, 
                               uint32_t sel, 
                               uint32_t *p_mem_type
                               );
    zoe_errs_t (*get_mem_usage)(c_zoe_module_base *This, 
                                uint32_t sel, 
                                uint32_t *p_mem_usage
                                );
    zoe_errs_t (*write)(c_zoe_module_base *This, 
                        uint32_t sel, 
                        ZOE_BUFFER_DESCRIPTOR *p_buf_desc
                        );
    zoe_errs_t (*buffer_available)(c_zoe_module_base *This, 
                                   uint32_t sel,
                                   uint32_t buf_sel
                                   );
};



/////////////////////////////////////////////////////////////////////////////
//
//

// c_zoe_module_base
//
c_zoe_module_base * c_zoe_module_base_constructor(c_zoe_module_base *p_zoe_module_base,
							                      c_object *p_parent, 
                                                  uint32_t who_am_i,
							                      uint32_t attributes,
                                                  IZOEHALAPI *p_hal,
                                                  zoe_dbg_comp_id_t dbg_id,
                                                  uint32_t cpu,
                                                  uint32_t inst,
                                                  uint32_t num_inputs,
                                                  uint32_t num_outputs,
                                                  uint32_t num_extra_events,
                                                  int32_t timeout_us,
                                                  uint32_t priority,
                                                  uint32_t stack_words,
                                                  void * p_module,
                                                  char *p_thread_name,
                                                  zoe_bool_t create_module_thread,
                                                  zoe_bool_t create_rpc_thread
							                      );
void c_zoe_module_base_destructor(c_zoe_module_base *This);
#define zoe_module_base_write_to_port(This, \
                                      port_index, \
                                      p_buf_desc, \
                                      req_flag, \
                                      written, \
                                      p_private \
                                      ) \
    (This)->m_ports[port_index].m_p_write_2_port_func(This, \
                                                      port_index, \
                                                      p_buf_desc, \
                                                      req_flag, \
                                                      written, \
                                                      p_private \
                                                      );
zoe_errs_t zoe_module_base_flush(c_zoe_module_base *This,
                                 uint32_t sel
                                 );
zoe_errs_t zoe_module_base_release_buffer(c_zoe_module_base *This, 
                                          uint32_t sel, 
                                          uint32_t buf_sel,
                                          zoe_dev_mem_t dev_mem,
                                          uint32_t size
                                          );
STATIC_INLINE zoe_sosal_obj_id_t zoe_module_base_get_extra_event(c_zoe_module_base *This, 
                                                                 uint32_t evt_nb
                                                                 )
{
    return ((evt_nb < This->m_num_extra_events) ? This->m_evt_extra[evt_nb] : ZOE_NULL);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// module connection interface
//
uint32_t zoe_module_base_get_num_ports(c_zoe_module_base *This);
zoe_errs_t zoe_module_base_get_port_selector_from_index(c_zoe_module_base *This, 
                                                        uint32_t index, 
                                                        uint32_t *p_sel
                                                        );
zoe_errs_t zoe_module_base_get_port_dir(c_zoe_module_base *This, 
                                        uint32_t sel, 
                                        uint32_t *p_dir
                                        );
zoe_errs_t zoe_module_base_get_port_type(c_zoe_module_base *This, 
                                         uint32_t sel, 
                                         uint32_t *p_type, 
                                         uint32_t *p_sub_type
                                         );
zoe_errs_t zoe_module_base_is_port_set(c_zoe_module_base *This, 
                                       uint32_t sel, 
                                       zoe_bool_t *p_set
                                       );
zoe_errs_t zoe_module_base_port_set(c_zoe_module_base *This, 
                                    uint32_t sel, 
                                    ZOE_MODULE_DATA_CONNECTOR *p_port
                                    );
zoe_errs_t zoe_module_base_port_clear(c_zoe_module_base *This, 
                                      uint32_t sel
                                      );
zoe_errs_t zoe_module_base_port_get(c_zoe_module_base *This, 
                                    uint32_t sel, 
                                    ZOE_MODULE_DATA_CONNECTOR *p_port
                                    );

#ifdef __cplusplus
}
#endif

#endif //#define __ZOE_MODULE_BASE_H__



