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
// ccomponent_decoder.c
//
// Description: 
//
//  component implementation for decoder
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
#include <media/v4l2-event.h>

#include "zoe_dbg.h"
#include "zoe_cobjectmgr.h"
#include "ccomponent.h"
#include "cdevice.h"
#include "zv_devioctl.h"
#include "zoe_linuxker_errmap.h"
#include "objids.h"
#include "zoe_module_vdec_intf.h"
#include "zv_pixel_fmt.h"
#include "vpu_if.h"

//#define _FAKE_RES_CHANGE

extern zoe_dbg_comp_id_t            g_ZVV4LDevDBGCompID;
extern struct vm_operations_struct	zvv4l_mmap_ops; 
extern struct semaphore             g_module_lock;


/////////////////////////////////////////////////////////////////////////////
//
//
static ZPU_V4L_FMT  formats_compressed_dec[] = 
{
	{
		.name		= "H264 Encoded Stream",
		.fourcc		= V4L2_PIX_FMT_H264,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_AVC,

	},
	{
		.name		= "VC1 Encoded Stream",
		.fourcc		= V4L2_PIX_FMT_VC1_ANNEX_G,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_VC1,
	},
	{
		.name		= "VC1 RCV Encoded Stream",
		.fourcc		= V4L2_PIX_FMT_VC1_ANNEX_L,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_VC1,
	},
	{
		.name		= "MPEG2 Encoded Stream",
		.fourcc		= V4L2_PIX_FMT_MPEG2,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_MPEG2,
	},

	{
		.name		= "AVS Encoded Stream",
		.fourcc		= VPU_PIX_FMT_AVS,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_AVS,
	},
	{
		.name		= "MPEG4 ASP Encoded Stream",
		.fourcc		= VPU_PIX_FMT_ASP,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_ASP,
	},
	{
		.name		= "JPEG stills",
		.fourcc		= V4L2_PIX_FMT_JPEG,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_JPEG,
	},
	{
		.name		= "RV8 Encoded Stream",
		.fourcc		= VPU_PIX_FMT_RV8,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_RV8,
	},
	{
		.name		= "RV9 Encoded Stream",
		.fourcc		= VPU_PIX_FMT_RV9,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_RV9,
	},
	{
		.name		= "VP6 Encoded Stream",
		.fourcc		= VPU_PIX_FMT_VP6,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_VP6,
	},
	{
		.name		= "VP7 Encoded Stream",
		.fourcc		= VPU_PIX_FMT_VP7,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_VP7,
	},
	{
		.name		= "VP6 SPK Encoded Stream",
		.fourcc		= VPU_PIX_FMT_SPK,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_SPK,
	},
	{
		.name		= "VP8 Encoded Stream",
		.fourcc		= V4L2_PIX_FMT_VP8,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_VP8,
	},
	{
		.name		= "H264/MVC Encoded Stream",
		.fourcc		= V4L2_PIX_FMT_H264_MVC,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_AVC_MVC,
	},
	{
		.name		= "H265 HEVC Encoded Stream",
		.fourcc		= VPU_PIX_FMT_HEVC,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_HEVC,
	},
	{
		.name		= "VP9 Encoded Stream",
		.fourcc		= VPU_PIX_FMT_VP9,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_VP9,
	},
	{
		.name		= "Logo",
		.fourcc		= VPU_PIX_FMT_LOGO,
		.num_planes	= 1,
        .vdec_std   = VPU_VIDEO_UNDEFINED,
	},
};


#define ZPU_NUM_FORMATS_COMPRESSED_DEC  ARRAY_SIZE(formats_compressed_dec)


static ZPU_V4L_FMT  formats_yuv_dec[] = 
{
	{
		.name		= "4:2:0 2 Planes Y/CbCr",
		.fourcc		= V4L2_PIX_FMT_NV12,
		.num_planes	= 2,
        .vdec_std   = VPU_PF_YUV420_SEMIPLANAR,
	},
};


#define ZPU_NUM_FORMATS_YUV_DEC ARRAY_SIZE(formats_yuv_dec)


static ZPU_V4L_FMT *find_format(ZPU_V4L_FMT *p_format_table, 
                                unsigned int table_size,
                                struct v4l2_format *f
                                )
{
	unsigned int    i;

	for (i = 0; i < table_size; i++) 
    {
        if (p_format_table[i].fourcc == f->fmt.pix_mp.pixelformat)
        {
            return (&p_format_table[i]);
        }
	}
	return NULL;
}


static ZPU_V4L_CONTROL  zpu_controls_dec[] = 
{
	{
		.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 1,
		.maximum = 32,
		.step = 1,
		.default_value = 4,
		.is_volatile = ZOE_TRUE,
	},
};


#define NUM_CTRLS_DEC   ARRAY_SIZE(zpu_controls_dec)


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
	        strlcpy(cap->bus_info, "usb-", sizeof(cap->bus_info));
		    break;

		case ZVV4LDEV_TYPE_PCIE:
	        strlcpy(cap->bus_info, "PCIe:", sizeof(cap->bus_info));
		    break;

		case ZVV4LDEV_TYPE_DIRECT:
		default:
	        strlcpy(cap->bus_info, "platform:", sizeof(cap->bus_info));
		    break;
    }
    cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | 
                       V4L2_CAP_STREAMING
                       ;
    cap->capabilities = cap->device_caps | 
                        V4L2_CAP_DEVICE_CAPS
                        ;
	return (0);
}



static int v4l2_ioctl_enum_fmt_vid_cap_mplane(struct file *file, 
                                              void *fh,
							                  struct v4l2_fmtdesc *f
                                              )
{
    ZPU_V4L_FMT *fmt;

    if (f->index >= ZPU_NUM_FORMATS_YUV_DEC)
    {
        return (-EINVAL);
    }

	fmt = &formats_yuv_dec[f->index];
	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return (0);
}



static int v4l2_ioctl_enum_fmt_vid_out_mplane(struct file *file, 
                                              void *fh,
							                  struct v4l2_fmtdesc *f
                                              )
{
    ZPU_V4L_FMT *fmt;

    if (f->index >= ZPU_NUM_FORMATS_COMPRESSED_DEC)
    {
        return (-EINVAL);
    }

	fmt = &formats_compressed_dec[f->index];
	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return (0);
}



static int v4l2_ioctl_g_fmt(struct file *file, 
                            void *fh, 
                            struct v4l2_format *f
                            )
{
    c_component                     *pComponent = (c_component *)v4l2_fh_to_component(fh);
    struct v4l2_pix_format_mplane   *pix_mp = &f->fmt.pix_mp;
	c_port		                    *pPort;
    zoe_errs_t                      err;
    i_zv_codec                      *pCodec;
    uint32_t                        got = 0;
	
    if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
    {
        uint32_t    i;

        pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];

        // get the video format from the decoder 
        //
		pCodec = c_device_get_codec(pComponent->m_pDevice);
		err = pCodec->get(pCodec,
						  (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
						  pComponent->m_hTask,
						  ZV_CODEC_PROP_DEC_YUV_FMT,
						  (zoe_void_ptr_t)&pPort->m_pic_format,
						  (zoe_void_ptr_t)&pPort->m_pic_format,
						  &got
						  );
        if (ZOE_SUCCESS(err))
        {
            pPort->m_openFormat.yuv.nWidth = pPort->m_pic_format.width_disp;
            pPort->m_openFormat.yuv.nHeight = pPort->m_pic_format.height_disp;
            pPort->m_frame_size = 0;
            pPort->m_format_valid = ZOE_TRUE;

		    pix_mp->width = pPort->m_pic_format.width_disp;
		    pix_mp->height = pPort->m_pic_format.height_disp;
		    pix_mp->field = V4L2_FIELD_ANY;
		    pix_mp->num_planes = pPort->m_pic_format.num_planes;
		    pix_mp->pixelformat = V4L2_PIX_FMT_NV12;
            pix_mp->colorspace = V4L2_COLORSPACE_REC709;
            for (i = 0; i < pix_mp->num_planes; i++)
            {
		        pix_mp->plane_fmt[i].bytesperline = pPort->m_pic_format.planes[i].stride;
		        pix_mp->plane_fmt[i].sizeimage = pPort->m_pic_format.planes[i].sizeimage;
                pPort->m_frame_size += pPort->m_pic_format.planes[i].sizeimage;
            }
        }
        else
        {
            return (ZoeToLinuxKerStatus(err));
        }
	}
    else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) 
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];
		pix_mp->width = 0;
		pix_mp->height = 0;
		pix_mp->field = V4L2_FIELD_ANY;
		pix_mp->plane_fmt[0].bytesperline = 0;
		pix_mp->plane_fmt[0].sizeimage = pPort->m_frame_size;
		pix_mp->pixelformat = pPort->m_pixel_format;
		pix_mp->num_planes = 1;
	} 
    else 
    {
        return (-EINVAL);
	}
	return (0);
}



static int v4l2_ioctl_try_fmt(struct file *file, 
                              void *fh, 
                              struct v4l2_format *f
                              )
{
    ZPU_V4L_FMT *fmt;

    if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) 
    {
        fmt = find_format(formats_compressed_dec, 
                          ZPU_NUM_FORMATS_COMPRESSED_DEC, 
                          f
                          );
		if (!fmt) 
        {
            return (-EINVAL);
		}
	} 
    else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) 
    {
        fmt = find_format(formats_yuv_dec, 
                          ZPU_NUM_FORMATS_YUV_DEC, 
                          f
                          );
		if (!fmt) 
        {
			return (-EINVAL);
		}
	}
    else
    {
		return (-EINVAL);
    }

    return (0);
}



static int v4l2_ioctl_s_fmt(struct file *file, 
                            void *fh, 
                            struct v4l2_format *f
                            )
{
    c_component                     *pComponent = (c_component *)v4l2_fh_to_component(fh);
    int                             ret = 0;
	struct v4l2_pix_format_mplane   *pix_mp = &f->fmt.pix_mp;
	c_port		                    *pPort;
    ZPU_V4L_FMT                     *fmt;
    i_zv_codec                      *pCodec;
    zoe_errs_t                      err;

    if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) 
    {
        fmt = find_format(formats_yuv_dec, 
                          ZPU_NUM_FORMATS_YUV_DEC, 
                          f
                          );
		if (!fmt) 
        {
			return (-EINVAL);
		}

        pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];

        if (PORT_STATE_RUN == pPort->m_State)
        {
            return (-EBUSY);
        }

        // set the format to the decoder
		pCodec = c_device_get_codec(pComponent->m_pDevice);
		err = pCodec->set(pCodec,
						  (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
						  pComponent->m_hTask,
						  ZV_CODEC_PROP_DEC_YUV_PIXEL_FMT,
						  (zoe_void_ptr_t)&fmt->vdec_std,
						  ZOE_NULL,
						  sizeof(uint32_t)
						  );
        if (ZOE_SUCCESS(err))
        {
            // remember it
            pPort->m_vdec_std = fmt->vdec_std;
            pPort->m_pixel_format = pix_mp->pixelformat;
            pPort->m_frame_size = pix_mp->plane_fmt[0].sizeimage;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
				           "%s set ZV_CODEC_PROP_DEC_YUV_PIXEL_FMT err(%d)\n",
				           __FUNCTION__,
                           err
				           );
            return (ZoeToLinuxKerStatus(err));
        }
    }
    else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) 
    {
        fmt = find_format(formats_compressed_dec, 
                          ZPU_NUM_FORMATS_COMPRESSED_DEC, 
                          f
                          );
		if (!fmt) 
        {
            return (-EINVAL);
		}

        pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];

        if (PORT_STATE_RUN == pPort->m_State)
        {
            return (-EBUSY);
        }

	    pix_mp->height = 0;
	    pix_mp->width = 0;

        // set the format to the decoder
		pCodec = c_device_get_codec(pComponent->m_pDevice);
		err = pCodec->set(pCodec,
						  (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
						  pComponent->m_hTask,
						  ZV_CODEC_PROP_DEC_FMT,
						  (zoe_void_ptr_t)&fmt->vdec_std,
						  ZOE_NULL,
						  sizeof(uint32_t)
						  );
        if (ZOE_SUCCESS(err))
        {
            // remember it
            pPort->m_vdec_std = fmt->vdec_std;
            pPort->m_pixel_format = pix_mp->pixelformat;
            pPort->m_frame_size = pix_mp->plane_fmt[0].sizeimage;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
				           "%s set ZV_CODEC_PROP_DEC_FMT err(%d)\n",
				           __FUNCTION__,
                           err
				           );
            return (ZoeToLinuxKerStatus(err));
        }
    }
    else
    {
        ret = -EINVAL;
    }
    return (ret);
}



static int v4l2_ioctl_expbuf(struct file *file, 
                             void *fh,
	                         struct v4l2_exportbuffer *eb
                             )
{
#ifdef CONFIG_ZV4L2_USE_VB2
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
	c_port		*pPort;

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == eb->type)
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];

    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == eb->type)
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        return (-EINVAL);
    }

    return (vb2_expbuf(&pPort->m_vb2_q, 
                       eb
                       ));

#else //!CONFIG_ZV4L2_USE_VB2
    return (-EINVAL);
#endif //CONFIG_ZV4L2_USE_VB2
}



static int v4l2_ioctl_g_crop(struct file *file, 
                             void *fh,
		                     struct v4l2_crop *cr
                             )
{
    c_component     *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t      err;
    i_zv_codec      *pCodec;
    uint32_t        got = 0;
    VPU_PICTURE     pic_format;
    VPU_CROP_WINDOW crop;

    // get the video format from the decoder 
    //
	pCodec = c_device_get_codec(pComponent->m_pDevice);
	err = pCodec->get(pCodec,
					  (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
					  pComponent->m_hTask,
					  ZV_CODEC_PROP_DEC_YUV_FMT,
					  (zoe_void_ptr_t)&pic_format,
					  (zoe_void_ptr_t)&pic_format,
					  &got
					  );
    if (ZOE_SUCCESS(err))
    {
	    err = pCodec->get(pCodec,
					      (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
					      pComponent->m_hTask,
					      ZV_CODEC_PROP_DEC_CROP,
					      (zoe_void_ptr_t)&crop,
					      (zoe_void_ptr_t)&crop,
					      &got
					      );
    }

    if (ZOE_SUCCESS(err))
    {
        cr->c.left = crop.left;
        cr->c.top = crop.top;
        cr->c.width = pic_format.width_disp - (crop.left + crop.right);
        cr->c.height = pic_format.height_disp - (crop.top + crop.bottom);
        return (0);
    }
    else
    {
        return (ZoeToLinuxKerStatus(err));
    }
}



static int v4l2_ioctl_decoder_cmd(struct file *file, 
                                  void *fh,
			                      struct v4l2_decoder_cmd *cmd
                                  )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;

    switch (cmd->cmd) 
    {
        case V4L2_DEC_CMD_START:
            err = c_component_start(pComponent, 
                                    COMPONENT_PORT_END
                                    );
            break;
        case V4L2_DEC_CMD_STOP:
            err = c_component_stop(pComponent, 
                                   COMPONENT_PORT_END
                                   );
            break;
        case V4L2_DEC_CMD_PAUSE:
            err = c_component_stop(pComponent, 
                                   COMPONENT_PORT_END
                                   );
            break;
        case V4L2_DEC_CMD_RESUME:
            err = c_component_start(pComponent, 
                                    COMPONENT_PORT_END
                                    );
            break;
        default:
            return (-EINVAL);
    }
    return (ZoeToLinuxKerStatus(err));
}



static int v4l2_ioctl_subscribe_event(struct v4l2_fh *fh,
				                      const struct v4l2_event_subscription *sub
                                      )
{
    switch (sub->type) 
    {
	    case V4L2_EVENT_EOS:
            return (v4l2_event_subscribe(fh, sub, 0, NULL));
	    case V4L2_EVENT_SOURCE_CHANGE:
            return (v4l2_src_change_event_subscribe(fh, sub));
    	default:
            return (-EINVAL);
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//

static int v4l2_ioctl_streamon(struct file *file, 
                               void *fh, 
                               enum v4l2_buf_type i
                               )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
#ifdef CONFIG_ZV4L2_USE_VB2
    int         ret = -EINVAL;
	c_port		*pPort;

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == i)
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];

    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == i)
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        return (ret);
    }
    ret = vb2_streamon(&pPort->m_vb2_q, 
                       i
                       );
#ifdef _FAKE_RES_CHANGE
    if ((0 == ret) &&
        (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == i)
        )
    {
        const struct v4l2_event ev = 
        {
            .type = V4L2_EVENT_SOURCE_CHANGE,
            .u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION
        };
		v4l2_event_queue_fh(fh, &ev);
    }
#endif //_FAKE_RES_CHANGE
    return (ret);
#else //!CONFIG_ZV4L2_USE_VB2
    zoe_errs_t  err;

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == i)
    {
        err = c_component_start(pComponent, 
                                COMPONENT_PORT_COMP_IN
                                ); 
#ifdef _FAKE_RES_CHANGE
        if (ZOE_SUCCESS(err))
        {
            const struct v4l2_event ev = 
            {
                .type = V4L2_EVENT_SOURCE_CHANGE,
                .u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION
            };
			v4l2_event_queue_fh(fh, &ev);
        }
#endif //_FAKE_RES_CHANGE
    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == i)
    {
        err = c_component_start(pComponent, 
                                COMPONENT_PORT_YUV_OUT
                                ); 
    }
    else
    {
        err = ZOE_ERRS_INVALID;
    }
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
				   "%s buf(%d) err(%d)\n",
				   __FUNCTION__,
                   i,
                   err
				   );

	return (ZoeToLinuxKerStatus(err));
#endif //CONFIG_ZV4L2_USE_VB2
}



static int v4l2_ioctl_streamoff(struct file *file, 
                                void *fh, 
                                enum v4l2_buf_type i
                                )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
#ifdef CONFIG_ZV4L2_USE_VB2
    int         ret = -EINVAL;
	c_port		*pPort;

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == i)
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];

    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == i)
    {
        pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        return (ret);
    }
    ret = vb2_streamoff(&pPort->m_vb2_q, 
                        i
                        );
    return (ret);
#else //!CONFIG_ZV4L2_USE_VB2
    zoe_errs_t  err;

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == i)
    {
	    err = c_component_stop(pComponent, 
                               COMPONENT_PORT_COMP_IN
                               ); 
    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == i)
    {
	    err = c_component_stop(pComponent, 
                               COMPONENT_PORT_YUV_OUT
                               ); 
    }
    else
    {
        err = ZOE_ERRS_INVALID;
    }

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
				   "%s buf(%d) err(%d)\n",
				   __FUNCTION__,
                   i,
                   err
				   );

	return (ZoeToLinuxKerStatus(err));
#endif //CONFIG_ZV4L2_USE_VB2
}



static int v4l2_ioctl_qbuf(struct file *file, 
                           void *fh, 
                           struct v4l2_buffer *pv4lbuf
                           )
{
    c_component *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t  err;
	c_port		*pPort;

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pv4lbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];
    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pv4lbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        pPort = ZOE_NULL;
    }

	if (!pPort)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_QBUF unknown type(%d)!!\n", 
					   __FUNCTION__,
					   pv4lbuf->type
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
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

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pv4lbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];
    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pv4lbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        pPort = ZOE_NULL;
    }

	if (!pPort)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_DQBUF unknown type(%d)!!\n", 
					   __FUNCTION__,
					   pv4lbuf->type
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
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

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == reqbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];
    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == reqbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        pPort = ZOE_NULL;
    }

	if (!pPort)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_REQBUFS unknown type(%d)!!\n", 
					   __FUNCTION__,
					   reqbuf->type
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
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

    if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == pv4lbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_COMP_IN];
    }
    else if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == pv4lbuf->type)
    {
		pPort = &pComponent->m_Ports[COMPONENT_PORT_YUV_OUT];
    }
    else
    {
        pPort = ZOE_NULL;
    }

	if (!pPort)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_QUERYBUF unknown type(%d)!!\n", 
					   __FUNCTION__,
					   pv4lbuf->type
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
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



const struct v4l2_ioctl_ops zvv4l_decoder_ioctl_ops = 
{
	.vidioc_querycap                = v4l2_ioctl_querycap,
	.vidioc_enum_fmt_vid_cap_mplane = v4l2_ioctl_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out_mplane = v4l2_ioctl_enum_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane    = v4l2_ioctl_g_fmt,
	.vidioc_g_fmt_vid_out_mplane    = v4l2_ioctl_g_fmt,
	.vidioc_try_fmt_vid_cap_mplane  = v4l2_ioctl_try_fmt,
	.vidioc_try_fmt_vid_out_mplane  = v4l2_ioctl_try_fmt,
	.vidioc_s_fmt_vid_cap_mplane    = v4l2_ioctl_s_fmt,
	.vidioc_s_fmt_vid_out_mplane    = v4l2_ioctl_s_fmt,
	.vidioc_expbuf                  = v4l2_ioctl_expbuf,
	.vidioc_g_crop                  = v4l2_ioctl_g_crop,
	.vidioc_decoder_cmd             = v4l2_ioctl_decoder_cmd,
	.vidioc_subscribe_event         = v4l2_ioctl_subscribe_event,
	.vidioc_unsubscribe_event       = v4l2_event_unsubscribe,
	.vidioc_reqbufs		            = v4l2_ioctl_reqbufs,
	.vidioc_querybuf	            = v4l2_ioctl_querybuf,
	.vidioc_qbuf		            = v4l2_ioctl_qbuf,
	.vidioc_dqbuf		            = v4l2_ioctl_dqbuf,
	.vidioc_streamon	            = v4l2_ioctl_streamon,
	.vidioc_streamoff	            = v4l2_ioctl_streamoff,
	.vidioc_default		            = c_component_ioctl,
};



/////////////////////////////////////////////////////////////////////////////
//
//

// Set/Get controls - v4l2 control framework
//
static int v4l2_dec_s_ctrl(struct v4l2_ctrl *ctrl)
{
    switch (ctrl->id) 
    {
        default:
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
				           "%s Invalid control(%d)\n",
				           __FUNCTION__,
                           ctrl->id
				           );
		    return (-EINVAL);
	}
    return (0);
}



static int v4l2_dec_g_v_ctrl(struct v4l2_ctrl *ctrl)
{
    c_component *This;
    zoe_errs_t  err;
    i_zv_codec  *pCodec;
    uint32_t    got = 0;

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
		           "%s control(%d)\n",
		           __FUNCTION__,
                   ctrl->id
		           );

    This = v4l2_ctrl_to_component(ctrl);

    switch (ctrl->id) 
    {
	    case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
        {
            uint32_t    nb;

            // call decoder to get the minimum frame buffer requirement
		    pCodec = c_device_get_codec(This->m_pDevice);
		    err = pCodec->get(pCodec,
						      (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
						      This->m_hTask,
						      ZV_CODEC_PROP_DEC_MIN_FRAME_BUF,
						      (zoe_void_ptr_t)&nb,
						      (zoe_void_ptr_t)&nb,
						      &got
						      );
            if (ZOE_SUCCESS(err))
            {
                ctrl->val = nb;
            }
            else
            {
                return (ZoeToLinuxKerStatus(err));
            }
			break;
        }

        default:
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
				           "%s Invalid control(%d)\n",
				           __FUNCTION__,
                           ctrl->id
				           );
		    return (-EINVAL);
    }
	return (0);
}



static const struct v4l2_ctrl_ops   zpu_dec_ctrl_ops = 
{
    .s_ctrl             = v4l2_dec_s_ctrl,
    .g_volatile_ctrl    = v4l2_dec_g_v_ctrl,
};



int c_component_ctrls_setup_decoder(c_component *This)
{
    int i;

    v4l2_ctrl_handler_init(&This->m_ctrl_handler, 
                           NUM_CTRLS_DEC + 1
                           );
    if (This->m_ctrl_handler.error) 
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
				       "%s v4l2_ctrl_handler_init failed(%d)\n",
				       __FUNCTION__,
                       This->m_ctrl_handler.error
				       );
        return (This->m_ctrl_handler.error);
    }
    else
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
				       "%s v4l2_ctrl_handler_init ctrls(%d)\n",
				       __FUNCTION__,
                       NUM_CTRLS_DEC
				       );
        This->m_ctrl_inited = ZOE_TRUE;
    }

    for (i = 0; i < NUM_CTRLS_DEC; i++) 
    {
        This->m_ctrls[i] = v4l2_ctrl_new_std(&This->m_ctrl_handler,
                                             &zpu_dec_ctrl_ops,
					                         zpu_controls_dec[i].id, 
                                             zpu_controls_dec[i].minimum,
					                         zpu_controls_dec[i].maximum, 
                                             zpu_controls_dec[i].step,
					                         zpu_controls_dec[i].default_value
                                             );
        if (This->m_ctrl_handler.error ||
            !This->m_ctrls[i]
            ) 
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
				           "%s v4l2_ctrl_new_std failed(%d) This->m_ctrls[%d](%p)\n",
				           __FUNCTION__,
                           This->m_ctrl_handler.error,
                           i,
                           This->m_ctrls[i]
				           );
            return (This->m_ctrl_handler.error);
		}
        
        if (zpu_controls_dec[i].is_volatile && 
            This->m_ctrls[i]
            )
        {
            This->m_ctrls[i]->flags |= V4L2_CTRL_FLAG_VOLATILE;
        }
    }

    v4l2_ctrl_handler_setup(&This->m_ctrl_handler);

    return (0);
}



void c_component_ctrls_delete_decoder(c_component *This)
{
	int i;

    if (This->m_ctrl_inited)
    {
        v4l2_ctrl_handler_free(&This->m_ctrl_handler);
        This->m_ctrl_inited = ZOE_FALSE;
    }
    for (i = 0; i < NUM_CTRLS_DEC; i++)
    {
        This->m_ctrls[i] = ZOE_NULL;
    }
}







