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
// ccomponent.h
//
// Description: 
//
//  component declaration
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CCOMPONENT_H__
#define __CCOMPONENT_H__

#include <linux/types.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/poll.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ctrls.h>

#include "zoe_types.h"
#include "zoe_cobject.h"
#ifdef CONFIG_ZV4L2_USE_VB2
#include "cport_vb2.h"
#else // !CONFIG_ZV4L2_USE_VB2
#include "cport.h"
#endif // CONFIG_ZV4L2_USE_VB2


/////////////////////////////////////////////////////////////////////////////
//
//
typedef struct _ZPU_V4L_FMT 
{
    char                *name;
    uint32_t            fourcc;
    uint32_t            num_planes;
    uint32_t            vdec_std;
} ZPU_V4L_FMT, *PZPU_V4L_FMT;



typedef struct _ZPU_V4L_CONTROL 
{
    uint32_t            id;
    enum v4l2_ctrl_type type;
    int32_t             minimum;
    int32_t             maximum;
    int32_t             step;
    int32_t             default_value;
    uint32_t            menu_skip_mask;
    zoe_bool_t          is_volatile;
} ZPU_V4L_CONTROL, *PZPU_V4L_CONTROL;


/////////////////////////////////////////////////////////////////////////////
//
//


#ifndef __CDEVICE_FWD_DEFINED__
#define __CDEVICE_FWD_DEFINED__
typedef struct c_device c_device;
#endif // !__CDEVICE_FWD_DEFINED__


#ifndef __CCOMPONENT_FWD_DEFINED__
#define __CCOMPONENT_FWD_DEFINED__
typedef struct c_component c_component;
#endif // !__CCOMPONENT_FWD_DEFINED__

#define ZPU_V4L_MAX_CTRLS       12

#define v4l2_ctrl_to_component(__ctrl) \
    container_of((__ctrl)->handler, c_component, m_ctrl_handler)
#define v4l2_fh_to_component(__fh) container_of(__fh, c_component, m_fh)

struct c_component
{
	// public c_object
	//
	c_object				    m_Object;

	// c_component
	//
    COMPONENT_TYPE              m_type;
    struct video_device         *m_pVideoDev;
	c_device				    *m_pDevice;
    struct v4l2_fh              m_fh;
    struct v4l2_ctrl            *m_ctrls[ZPU_V4L_MAX_CTRLS];
    struct v4l2_ctrl_handler    m_ctrl_handler;
    zoe_bool_t                  m_ctrl_inited;
	ZOE_OBJECT_HANDLE	        m_hTask;	// codec lib task handle
	ZOE_OBJECT_HANDLE	        m_hObject;	// handle acquired from object manager
	c_port				        m_Ports[MAX_COMPONENT_PORT];
    zoe_bool_t                  m_user_mode_proxy;
    wait_queue_head_t           m_user_mode_event;
    uint32_t                    m_user_event;
};		


c_component * c_component_constructor(c_component *pComponent,
									  struct video_device *pVideoDev,
									  c_device *pDevice,
                                      COMPONENT_TYPE type
									  );
void c_component_destructor(c_component *This);
zoe_errs_t c_component_open(c_component *This,
                            struct file *filp
                            );
zoe_errs_t c_component_close(c_component *This);
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
long c_component_ioctl(struct file *filp, 
                       void *fh,
                       bool valid_prio,
                       unsigned int cmd, 
					   void *arg
					   );
#else //LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
long c_component_ioctl(struct file *filp, 
                       void *fh,
					   int cmd, 
					   void *arg
					   );
#endif //LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
int c_component_mmap(c_component *This, 
					 struct vm_area_struct *vma
					 );
unsigned int c_component_poll(c_component *This, 
                             struct file *filp,
							 poll_table *wait
							 );
zoe_errs_t c_component_set_user_event(c_component *This, 
                                      uint32_t event
                                      );
zoe_errs_t c_component_port_open(c_component *This,
							     PCOMPONENT_PORT_OPEN pOpenParam,
                                 zoe_bool_t b_format
							     );
zoe_errs_t c_component_port_close(c_component *This,
								  COMPONENT_PORT_TYPE type
								  );
zoe_errs_t c_component_start(c_component *This, 
                             COMPONENT_PORT_TYPE type
                             );
zoe_errs_t c_component_stop(c_component *This, 
                            COMPONENT_PORT_TYPE type
                            );
zoe_errs_t c_component_acquire(c_component *This, 
                               COMPONENT_PORT_TYPE type
                               );

// decoder functions
//
int c_component_ctrls_setup_decoder(c_component *This);
void c_component_ctrls_delete_decoder(c_component *This);

// encoder functions
//
int c_component_ctrls_setup_encoder(c_component *This);
void c_component_ctrls_delete_encoder(c_component *This);


// access functions
//
static inline ZOE_OBJECT_HANDLE c_component_get_task_handle(c_component *This) {return (This->m_hTask);}
static inline zoe_bool_t c_component_is_user_mode_proxy(c_component *This) {return (This->m_user_mode_proxy);}

#endif //__CCOMPONENT_H__




