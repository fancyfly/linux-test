/*
 * Copyright 2012-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


///////////////////////////////////////////////////////////////////////////////
//
// cport.h
//
// Description: 
//
//  base port declaration
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CPORT_H__
#define __CPORT_H__


#include "zv_devioctl.h"

#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_cqueue.h"
#include "zv_avlib.h"
#include "zoe_util.h"


typedef enum _PORT_STATE
{
    PORT_STATE_STOP,
    PORT_STATE_ACQUIRE,
    PORT_STATE_PAUSE,
    PORT_STATE_RUN
} PORT_STATE, *PPORT_STATE;



typedef enum _PORT_DIRECTION
{
	PORT_DIR_READ = 0,
	PORT_DIR_WRITE,
	PORT_DIR_NONE

} PORT_DIRECTION, *PPORT_DIRECTION;

#define MAX_PORT_DIRECTION	PORT_DIR_NONE


#define PORT_MMAP_BUF_TYPE_SHIFT	28
#define PORT_MMAP_BUF_TYPE_MASK		0xF0000000

/////////////////////////////////////////////////////////////////////////////
//
//

// data request structure
//
typedef struct _PORT_DATA_REQ
{
	struct v4l2_buffer		*pV4L2Buf;
	PZV_BUFFER_DESCRIPTOR	pBufDesc;
	int						nId;
} PORT_DATA_REQ, *PPORT_DATA_REQ;


/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __CCOMPONENT_FWD_DEFINED__
#define __CCOMPONENT_FWD_DEFINED__
typedef struct c_component c_component;
#endif // !__CCOMPONENT_FWD_DEFINED__


#ifndef __CPORT_FWD_DEFINED__
#define __CPORT_FWD_DEFINED__
typedef struct c_port c_port;
#endif // !__CPORT_FWD_DEFINED__


struct c_port
{
	// public c_object
	//
	c_object					m_Object;

	// c_port
	//
	zoe_bool_t					m_valid;

	c_component					*m_pComponent;
	COMPONENT_PORT_TYPE			m_id;

	ZOE_OBJECT_HANDLE			m_hStreamLib;	// codec lib stream handle

	PORT_DIRECTION				m_dir;
	uint32_t				    m_dwOpenType;
	COMPONENT_PORT_OPEN_FORMAT	m_openFormat;
    zoe_bool_t                  m_format_valid;
    uint32_t                    m_pixel_format;
    uint32_t                    m_vdec_std;
	int							m_memoryType;
	uint32_t				    m_frame_nbs;
	uint32_t				    m_frame_size;
	zoe_bool_t					m_bBufferPartialFill;
	zoe_bool_t					m_bBufferFrameAligned;
	zoe_bool_t					m_bSupportUserPtr;

	volatile PORT_STATE			m_State;
	volatile zoe_bool_t			m_EOS;
	uint64_t				    m_picture_num;
    zoe_bool_t					m_discontinuity;

	zoe_bool_t					m_bDisabled;

    uint8_t                     *m_pBufMem;
	QUEUE_ENTRY					m_Entries[ZV_AVLIB_MAX_DATA_ENTRIES];
	PORT_DATA_REQ				m_PortReqs[ZV_AVLIB_MAX_DATA_ENTRIES];
	ZV_BUFFER_DESCRIPTOR		m_BufDesces[ZV_AVLIB_MAX_DATA_ENTRIES];
	struct v4l2_buffer			m_v4l2Buffers[ZV_AVLIB_MAX_DATA_ENTRIES];
	c_queue						*m_pQueueFree;
	c_queue						*m_pQueueData;
	c_queue						*m_pQueueLib;
	c_queue						*m_pQueueUser;
    zoe_bool_t                  m_free_queue_inited;
    wait_queue_head_t		    m_buf_ready_wq; // used for poll/select
};



// c_port interfaces
//
c_port * c_port_constructor(c_port *pPort,
						    c_component *pComponent,
						    uint32_t dwAttributes,
						    COMPONENT_PORT_TYPE id,
						    PORT_DIRECTION dir,
						    uint32_t dwOpenType,
						    PCOMPONENT_PORT_OPEN_FORMAT pOpenFormat,
						    uint32_t frame_nbs,
						    uint32_t frame_size,
						    zoe_bool_t bBufferPartialFill,
						    zoe_bool_t bBufferFrameAligned,
						    zoe_bool_t bSupportUserPtr
						    );
void c_port_destructor(c_port *This);
zoe_errs_t c_port_create(c_port *This);
zoe_errs_t c_port_close(c_port *This);
zoe_errs_t c_port_enable(c_port *This);
zoe_errs_t c_port_disable(c_port *This);
zoe_errs_t c_port_set_state(c_port *This,
						    PORT_STATE to_state, 
						    PORT_STATE from_state
						    );
zoe_errs_t c_port_req_buf(c_port *This, 
                          struct file *file,
						  struct v4l2_requestbuffers *reqbuf
						  );
zoe_errs_t c_port_query_buf(c_port *This, 
                            struct file *file,
						    struct v4l2_buffer *buf
						    );
zoe_errs_t c_port_dqbuf(c_port *This, 
                        struct file *file,
					    struct v4l2_buffer *pv4lbufapp
					    );
zoe_errs_t c_port_qbuf(c_port *This, 
                       struct file *file,
					   struct v4l2_buffer *pv4lbufapp
					   );
zoe_bool_t c_port_buf_rdy(c_port *This,
                          struct file *file,
                          poll_table *wait
                          );

#endif //__CPORT_H__



