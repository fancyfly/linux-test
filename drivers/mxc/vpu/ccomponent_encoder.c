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
// ccomponent_encoder.c
//
// Description: 
//
//  component implementation for encoder
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#include "zoe_dbg.h"
#include "zoe_cobjectmgr.h"
#include "ccomponent.h"
#include "cdevice.h"
#include "zv_devioctl.h"
#include "zoe_linuxker_errmap.h"
#include "objids.h"


extern zoe_dbg_comp_id_t            g_ZVV4LDevDBGCompID;
extern struct vm_operations_struct	zvv4l_mmap_ops; 
extern struct semaphore             g_module_lock;

/////////////////////////////////////////////////////////////////////////////
//
//

static int v4l2_ioctl_querycap(struct file *file, 
                               void *fh,
			                   struct v4l2_capability *cap
                               )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);

    strncpy(cap->driver, pComponent->m_pVideoDev->name, sizeof(cap->driver) - 1);
    strncpy(cap->card, pComponent->m_pVideoDev->name, sizeof(cap->card) - 1);
    switch (pComponent->m_pDevice->m_type)
    {
		case ZVV4LDEV_TYPE_USB:
	        strlcpy(cap->bus_info, "Usb", sizeof(cap->bus_info));
		    break;

		case ZVV4LDEV_TYPE_PCIE:
	        strlcpy(cap->bus_info, "PCIe", sizeof(cap->bus_info));
		    break;

		case ZVV4LDEV_TYPE_DIRECT:
		default:
	        strlcpy(cap->bus_info, "Platform", sizeof(cap->bus_info));
		    break;
    }
    cap->version = KERNEL_VERSION(0, 0, 1);
    cap->device_caps = V4L2_CAP_STREAMING |
			           V4L2_CAP_VIDEO_CAPTURE_MPLANE
                       ;
    cap->capabilities = cap->device_caps | 
                        V4L2_CAP_DEVICE_CAPS
                        ;
	return (0);
}



static int v4l2_ioctl_g_fmt(struct file *file, 
                            void *fh, 
                            struct v4l2_format *f
                            )
{
    struct v4l2_pix_format_mplane   *pix_mp = &f->fmt.pix_mp;

    if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
    {
		pix_mp->width = 0;
		pix_mp->height = 0;
		pix_mp->field = V4L2_FIELD_NONE;
		pix_mp->plane_fmt[0].bytesperline = 0;
		pix_mp->plane_fmt[0].sizeimage = 0x100000;
		pix_mp->pixelformat = V4L2_PIX_FMT_H264;
		pix_mp->num_planes = 1;
    }
    else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
    {
	    pix_mp->width = 1920;
	    pix_mp->height = 1080;
	    pix_mp->field = V4L2_FIELD_NONE;
	    pix_mp->pixelformat = V4L2_PIX_FMT_NV12;
	    pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
	    pix_mp->plane_fmt[0].sizeimage = pix_mp->height * pix_mp->plane_fmt[0].bytesperline;
	    pix_mp->plane_fmt[1].bytesperline = pix_mp->width >> 1;
	    pix_mp->plane_fmt[1].sizeimage = pix_mp->height * pix_mp->plane_fmt[0].bytesperline;
        pix_mp->num_planes = 2;
    }
    else
    {
        return (-EINVAL);
    }
	return (0);
}



static int v4l2_ioctl_streamon(struct file *file, 
                               void *fh, 
                               enum v4l2_buf_type i
                               )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;

    err = c_component_start(pComponent, 
                            COMPONENT_PORT_END
                            ); 

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
				   "%s err(%d)\n",
				   __FUNCTION__,
                   err
				   );

	return (ZoeToLinuxKerStatus(err));
}



static int v4l2_ioctl_streamoff(struct file *file, 
                                void *fh, 
                                enum v4l2_buf_type i
                                )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;

	err = c_component_stop(pComponent,
                           COMPONENT_PORT_END
                           ); 

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
				   "%s err(%d)\n",
				   __FUNCTION__,
                   err
				   );

	return (ZoeToLinuxKerStatus(err));
}



static int v4l2_ioctl_qbuf(struct file *file, 
                           void *fh, 
                           struct v4l2_buffer *pv4lbuf
                           )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;
	c_port		*pPort;

	if (!pv4lbuf ||
		(pv4lbuf->sequence >= MAX_COMPONENT_PORT)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_QBUF unknown sequence(%d)!!\n", 
					   __FUNCTION__,
					   pv4lbuf->sequence
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
		pPort = &pComponent->m_Ports[pv4lbuf->sequence];
		err = c_port_qbuf(pPort, 
                          fh,
						  pv4lbuf
						  ); 
        if (err != ZOE_ERRS_AGAIN)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           g_ZVV4LDevDBGCompID,
				           "%s c_port_qbuf err(%d)\n",
				           __FUNCTION__,
                           err
				           );
        }
	}
	return (ZoeToLinuxKerStatus(err));
}



static int v4l2_ioctl_dqbuf(struct file *file, 
                            void *fh, 
                            struct v4l2_buffer *pv4lbuf
                            )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;
	c_port      *pPort;

	if (!pv4lbuf ||
		(pv4lbuf->sequence >= MAX_COMPONENT_PORT)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_DQBUF unknown sequence(%d)!!\n", 
					   __FUNCTION__,
					   pv4lbuf->sequence
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
		pPort = &pComponent->m_Ports[pv4lbuf->sequence];
		err = c_port_dqbuf(pPort, 
                           fh,
						   pv4lbuf
						   ); 
        if (err != ZOE_ERRS_AGAIN)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           g_ZVV4LDevDBGCompID,
				           "%s c_port_dqbuf err(%d)\n",
				           __FUNCTION__,
                           err
				           );
        }
	}
	return (ZoeToLinuxKerStatus(err));
}



static int v4l2_ioctl_reqbufs(struct file *file, 
                              void *fh,
				              struct v4l2_requestbuffers *reqbuf
                              )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;
	c_port		*pPort;

	if (!reqbuf ||
		(reqbuf->count >= MAX_COMPONENT_PORT)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_REQBUFS unknown sequence(%d)!!\n", 
					   __FUNCTION__,
					   reqbuf->count
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
		pPort = &pComponent->m_Ports[reqbuf->count];
		err = c_port_req_buf(pPort, 
                             fh,
						     reqbuf
						     ); 
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   g_ZVV4LDevDBGCompID,
				   "%s c_port_req_buf err(%d)\n",
				   __FUNCTION__,
                   err
				   );

	}
	return (ZoeToLinuxKerStatus(err));
}



static int v4l2_ioctl_querybuf(struct file *file, 
                               void *fh, 
                               struct v4l2_buffer *pv4lbuf
                               )
{		
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;
	c_port		*pPort;

	if (!pv4lbuf ||
		(pv4lbuf->sequence >= MAX_COMPONENT_PORT)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_QUERYBUF unknown sequence(%d)!!\n", 
					   __FUNCTION__,
					   pv4lbuf->sequence
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
		pPort = &pComponent->m_Ports[pv4lbuf->sequence];
		err = c_port_query_buf(pPort, 
                               fh,
							   pv4lbuf
							   );
	    zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                       g_ZVV4LDevDBGCompID,
				       "%s c_port_query_buf err(%d)\n",
				       __FUNCTION__,
                       err
				       );        
	}
	return (ZoeToLinuxKerStatus(err));
}



const struct v4l2_ioctl_ops zvv4l_encoder_ioctl_ops = 
{
	.vidioc_querycap                = v4l2_ioctl_querycap,
	.vidioc_g_fmt_vid_cap_mplane    = v4l2_ioctl_g_fmt,
    .vidioc_g_fmt_vid_out_mplane    = v4l2_ioctl_g_fmt,
	.vidioc_reqbufs		            = v4l2_ioctl_reqbufs,
	.vidioc_querybuf	            = v4l2_ioctl_querybuf,
	.vidioc_qbuf		            = v4l2_ioctl_qbuf,
	.vidioc_dqbuf		            = v4l2_ioctl_dqbuf,
	.vidioc_streamon	            = v4l2_ioctl_streamon,
	.vidioc_streamoff	            = v4l2_ioctl_streamoff,
	.vidioc_default		            = c_component_ioctl,
};


int c_component_ctrls_setup_encoder(c_component *This)
{
    return (0);
}



void c_component_ctrls_delete_encoder(c_component *This)
{
}


