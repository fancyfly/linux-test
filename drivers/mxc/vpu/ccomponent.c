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
// ccomponent.c
//
// Description: 
//
//  component implementation
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


extern zoe_dbg_comp_id_t            g_ZVV4LDevDBGCompID;
extern struct vm_operations_struct	zvv4l_mmap_ops; 
extern struct semaphore             g_module_lock;

/////////////////////////////////////////////////////////////////////////////
//
//

zoe_errs_t c_component_port_open(c_component *This,
							     PCOMPONENT_PORT_OPEN pOpenParam,
                                 zoe_bool_t b_format
							     )
{
	zoe_errs_t					err;
	c_port						*pPort;
	PORT_DIRECTION				dir;
	uint32_t				    dwOpenType;
	PCOMPONENT_PORT_OPEN_FORMAT	pOpenFormat = ZOE_NULL;
	uint32_t				    frame_nbs;
	uint32_t				    frame_size;
	zoe_bool_t					bBufferPartialFill;
	zoe_bool_t					bBufferFrameAligned;
	zoe_bool_t					bSupportUserPtr;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s [\n", 
				   __FUNCTION__
				   );

	// validate parameters
	//
	if (!pOpenParam ||
		(pOpenParam->type >= MAX_COMPONENT_PORT) ||
		(pOpenParam->type < 0)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s pOpenParam(0x%x) type(%d)\n", 
					   __FUNCTION__,
					   pOpenParam,
					   pOpenParam ? pOpenParam->type : -1
					   );
		return (ZOE_ERRS_PARMS);
	}

	ENTER_CRITICAL(&This->m_Object)

	pPort = &This->m_Ports[pOpenParam->type];

	if (pPort->m_valid)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s port(%d) opened already\n", 
					   __FUNCTION__,
					   pOpenParam->type
					   );
		LEAVE_CRITICAL(&This->m_Object)
		return (ZOE_ERRS_INVALID);
	}

	// only allow user pointer for PCIe bus
	//
	bSupportUserPtr = (ZVV4LDEV_TYPE_PCIE == This->m_pDevice->m_type);

	// setup port specific parameters
	//
	switch (pOpenParam->type)
	{
		case COMPONENT_PORT_YUV_IN:
			dir = PORT_DIR_WRITE;
			dwOpenType = ZV_CODEC_YUV_IN;
            if (b_format)
            {
			    pOpenFormat = &pOpenParam->format;
			    frame_size = (pOpenParam->format.yuv.nWidth * 
						     pOpenParam->format.yuv.nHeight * 
						     pOpenParam->format.yuv.nBitCount) / 8
						     ;
			    frame_size = (frame_size + PAGE_SIZE - 1) & PAGE_MASK; 
                frame_nbs = 4;
            }
            else
            {
                pOpenFormat = ZOE_NULL;
                frame_size = 0;
                frame_nbs = 0;
            }
			bBufferPartialFill = ZOE_FALSE;
			bBufferFrameAligned = ZOE_TRUE;
			break;

		case COMPONENT_PORT_COMP_IN:
			dir = PORT_DIR_WRITE;
			dwOpenType = ZV_CODEC_VID_IN;
			frame_nbs = ZV_AVLIB_MAX_DATA_ENTRIES / 16;

			frame_size = 64 * 1024;
			bBufferFrameAligned = ZOE_FALSE;
			bBufferPartialFill = ZOE_TRUE;
			break;

		case COMPONENT_PORT_COMP_OUT:
			dir = PORT_DIR_READ;
			dwOpenType = ZV_CODEC_VID_OUT;
			frame_nbs = ZV_AVLIB_MAX_DATA_ENTRIES / 16;
			frame_size = 64 * 1024;
			bBufferFrameAligned = ZOE_FALSE;
			bBufferPartialFill = ZOE_TRUE;
			break;

		case COMPONENT_PORT_META_OUT:
			dir = PORT_DIR_READ;
			dwOpenType = ZV_CODEC_META_OUT;
			frame_nbs = 128;
			frame_size = 30 * 64;
			bBufferFrameAligned = ZOE_FALSE;
			bBufferPartialFill = ZOE_TRUE;
			break;

		case COMPONENT_PORT_YUV_OUT:
			dir = PORT_DIR_READ;
			dwOpenType = ZV_CODEC_YUV_OUT;
            if (b_format)
            {
			    pOpenFormat = &pOpenParam->format;
			    frame_size = (pOpenParam->format.yuv.nWidth * 
						     pOpenParam->format.yuv.nHeight * 
						     pOpenParam->format.yuv.nBitCount) / 8
						     ;
			    frame_size = (frame_size + PAGE_SIZE - 1) & PAGE_MASK; 
                frame_nbs = 4;
            }
            else
            {
                pOpenFormat = ZOE_NULL;
                frame_size = 0;
                frame_nbs = 0;
            }
			bBufferPartialFill = ZOE_FALSE;
			bBufferFrameAligned = ZOE_TRUE;
			break;

		case COMPONENT_PORT_VIRTUAL_IN:
		case COMPONENT_PORT_VIRTUAL_OUT:
			dir = PORT_DIR_NONE;
			dwOpenType = ZV_CODEC_VIRTUAL;
			frame_nbs = 0;
			frame_size = 0;
			bBufferFrameAligned = ZOE_FALSE;
			bBufferPartialFill = ZOE_TRUE;
			break;

		default:
			LEAVE_CRITICAL(&This->m_Object)
			return (ZOE_ERRS_PARMS);
	}

	// instantiate the port
	//
	if (!c_port_constructor(pPort, 
						    This, 
						    OBJECT_CRITICAL_HEAVY, 
						    pOpenParam->type, 
						    dir,
						    dwOpenType, 
						    pOpenFormat,
						    frame_nbs, 
						    frame_size, 
						    bBufferPartialFill,
						    bBufferFrameAligned,
						    bSupportUserPtr
						    ))
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_port_constructor() failed\n", 
					   __FUNCTION__
					   );
		LEAVE_CRITICAL(&This->m_Object)
		return (ZOE_ERRS_FAIL);
	}

	err = c_port_create(pPort);
    if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_port_create() failed status(%d)\n", 
					   __FUNCTION__,
					   err
					   );
		c_port_destructor(pPort);
	}

	LEAVE_CRITICAL(&This->m_Object)

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s status(%d) ]\n", 
				   __FUNCTION__,
				   err
				   );

	return (err);
}



zoe_errs_t c_component_port_close(c_component *This,
								  COMPONENT_PORT_TYPE type
								  )
{
	c_port	*pPort;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s\n", 
				   __FUNCTION__
				   );

	if ((type >= MAX_COMPONENT_PORT) ||
		(type < 0)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s invalid type(%d)\n", 
					   __FUNCTION__,
					   type
					   );
		return (ZOE_ERRS_PARMS);
	}

	ENTER_CRITICAL(&This->m_Object)

	pPort = &This->m_Ports[type];

	if (pPort->m_valid)
	{
		c_port_close(pPort);
		c_port_destructor(pPort);
	}

	LEAVE_CRITICAL(&This->m_Object)
	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_component_start(c_component *This, 
                             COMPONENT_PORT_TYPE type
                             )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_port		*pPort;
	int			i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s [\n", 
				   __FUNCTION__
				   );

	ENTER_CRITICAL(&This->m_Object)

    if (COMPONENT_PORT_END == type)
    {
	    // from stop to acquire 
	    //
	    for (i = 0; i < MAX_COMPONENT_PORT; i++)
	    {
		    pPort = &This->m_Ports[i];
		    if (pPort->m_valid)
		    {
			    if (PORT_STATE_STOP == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_ACQUIRE, 
									       PORT_STATE_STOP
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_start_Exit;
				    }
			    }
		    }
	    }

	    // from acquire to pause 
	    //
	    for (i = 0; i < MAX_COMPONENT_PORT; i++)
	    {
		    pPort = &This->m_Ports[i];
		    if (pPort->m_valid)
		    {
			    if (PORT_STATE_ACQUIRE == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_PAUSE,
									       PORT_STATE_ACQUIRE
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_start_Exit;
				    }
			    }
		    }
	    }

	    // from pause to run 
	    //
	    for (i = 0; i < MAX_COMPONENT_PORT; i++)
	    {
		    pPort = &This->m_Ports[i];
		    if (pPort->m_valid)
		    {
			    err = c_port_set_state(pPort, 
								       PORT_STATE_RUN,
								       pPort->m_State
								       );
			    if (!ZOE_SUCCESS(err))
			    {
				    goto c_component_start_Exit;
			    }
		    }
	    }
    }
    else
    {
        if (type < MAX_COMPONENT_PORT)
        {
		    pPort = &This->m_Ports[type];
		    if (pPort->m_valid)
		    {
			    if (PORT_STATE_STOP == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_ACQUIRE, 
									       PORT_STATE_STOP
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_start_Exit;
				    }
			    }

			    if (PORT_STATE_ACQUIRE == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_PAUSE,
									       PORT_STATE_ACQUIRE
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_start_Exit;
				    }
			    }

			    err = c_port_set_state(pPort, 
								       PORT_STATE_RUN,
								       pPort->m_State
								       );
			    if (!ZOE_SUCCESS(err))
			    {
				    goto c_component_start_Exit;
			    }
            }
        }
        else
        {
            err = ZOE_ERRS_PARMS;
        }
    }

c_component_start_Exit:

	if (!ZOE_SUCCESS(err) &&
        (COMPONENT_PORT_END == type)
        )
	{
		for (i = 0; i < MAX_COMPONENT_PORT; i++)
		{
			pPort = &This->m_Ports[i];
			if (pPort->m_valid)
			{
				c_port_set_state(pPort, 
		 					     PORT_STATE_STOP, 
		 					     pPort->m_State
		 					     );
			}
		}
	}

	LEAVE_CRITICAL(&This->m_Object)

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s status(%d) ]\n", 
				   __FUNCTION__,
				   err
				   );

	return (err);
}



zoe_errs_t c_component_stop(c_component *This, 
                            COMPONENT_PORT_TYPE type
                            )
{
	c_port	*pPort;
	int		i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s [\n", 
				   __FUNCTION__
				   );

	ENTER_CRITICAL(&This->m_Object)

    if (COMPONENT_PORT_END == type)
    {
	    // stop all ports 
	    //
	    for (i = 0; i < MAX_COMPONENT_PORT; i++)
	    {
		    pPort = &This->m_Ports[i];
		    if (pPort->m_valid)
		    {
			    c_port_set_state(pPort, 
		 				         PORT_STATE_STOP, 
		 				         pPort->m_State
		 				         );
		    }
	    }
    }
    else
    {
        if (type < MAX_COMPONENT_PORT)
        {
		    pPort = &This->m_Ports[type];
		    if (pPort->m_valid)
		    {
			    c_port_set_state(pPort, 
		 				         PORT_STATE_STOP, 
		 				         pPort->m_State
		 				         );
		    }
        }
    }

	LEAVE_CRITICAL(&This->m_Object)

	// always succeed
	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_component_acquire(c_component *This, 
                               COMPONENT_PORT_TYPE type
                               )
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;
	c_port		*pPort;
	int			i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s [\n", 
				   __FUNCTION__
				   );

	ENTER_CRITICAL(&This->m_Object)

    if (COMPONENT_PORT_END == type)
    {
	    // from stop to acquire 
	    //
	    for (i = 0; i < MAX_COMPONENT_PORT; i++)
	    {
		    pPort = &This->m_Ports[i];
		    if (pPort->m_valid)
		    {
			    if (PORT_STATE_STOP == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_ACQUIRE, 
									       PORT_STATE_STOP
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_acquire_Exit;
				    }
			    }
		    }
	    }

	    // from acquire to pause 
	    //
	    for (i = 0; i < MAX_COMPONENT_PORT; i++)
	    {
		    pPort = &This->m_Ports[i];
		    if (pPort->m_valid)
		    {
			    if (PORT_STATE_ACQUIRE == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_PAUSE,
									       PORT_STATE_ACQUIRE
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_acquire_Exit;
				    }
			    }
		    }
	    }
    }
    else
    {
        if (type < MAX_COMPONENT_PORT)
        {
		    pPort = &This->m_Ports[type];
		    if (pPort->m_valid)
		    {
			    if (PORT_STATE_STOP == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_ACQUIRE, 
									       PORT_STATE_STOP
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_acquire_Exit;
				    }
			    }

			    if (PORT_STATE_ACQUIRE == pPort->m_State)
			    {
				    err = c_port_set_state(pPort, 
									       PORT_STATE_PAUSE,
									       PORT_STATE_ACQUIRE
									       );
				    if (!ZOE_SUCCESS(err))
				    {
					    goto c_component_acquire_Exit;
				    }
			    }
		    }
        }
        else
        {
            err = ZOE_ERRS_PARMS;
        }
    }

c_component_acquire_Exit:

	if (!ZOE_SUCCESS(err) &&
        (COMPONENT_PORT_END == type)
        )
	{
		for (i = 0; i < MAX_COMPONENT_PORT; i++)
		{
			pPort = &This->m_Ports[i];
			if (pPort->m_valid)
			{
				c_port_set_state(pPort, 
		 					     PORT_STATE_STOP, 
		 					     pPort->m_State
		 					     );
			}
		}
	}

	LEAVE_CRITICAL(&This->m_Object)

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   g_ZVV4LDevDBGCompID,    
				   "%s status(%d) ]\n", 
				   __FUNCTION__,
				   err
				   );

	return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// constructor
//
c_component * c_component_constructor(c_component *pComponent,
									  struct video_device *pVideoDev,
									  c_device *pDevice,
                                      COMPONENT_TYPE type
									  )
{
	if (pComponent)
	{
		int	i;

        // zero init 
        //
		memset(pComponent, 
			   0, 
			   sizeof(c_component)
			   );

		// c_object
		//
		c_object_constructor(&pComponent->m_Object,
							 ZOE_NULL, 
                             OBJECT_ZOE_V4L2_COMPONENT,
		   					 OBJECT_CRITICAL_LIGHT
		   					 );

		// initialize members
		//
        pComponent->m_type = type;
		pComponent->m_pVideoDev = pVideoDev;
		pComponent->m_pDevice = pDevice;
        pComponent->m_ctrl_inited = ZOE_FALSE;
		pComponent->m_hTask = ZOE_NULL_HANDLE;
		pComponent->m_hObject = ZOE_NULL_HANDLE;
        pComponent->m_user_mode_proxy = ZOE_FALSE;
		
		for (i = 0; i < MAX_COMPONENT_PORT; i++)
		{
			pComponent->m_Ports[i].m_valid = ZOE_FALSE;
		}
	}

	return (pComponent);
}



// destructor
//
void c_component_destructor(c_component *This)
{
	// c_object
	//
    c_object_destructor(&This->m_Object);
}



zoe_errs_t c_component_open(c_component *This, 
                            struct file *filp
                            )
{
	i_zv_codec	*pMpegCodec = c_device_get_codec(This->m_pDevice);
	zoe_errs_t	err;

    v4l2_fh_init(&This->m_fh, 
                 This->m_pVideoDev
                 );
	filp->private_data = &This->m_fh;
    v4l2_fh_add(&This->m_fh);

	// allocate task
	//
	err = pMpegCodec->alloc_task(pMpegCodec, 
                                 (ZV_CODEC_TASK_TYPE)This->m_type,
								 &This->m_hTask
								 );
	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) alloc_task failed status(%d)\n", 
					   __FUNCTION__,
                       This->m_type,
					   err
					   );
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s(%d) alloc_task hTask(%d)\n", 
					   __FUNCTION__,
                       This->m_type,
					   This->m_hTask
					   );
		// add to the device's component manager
		//
		This->m_hObject = c_object_mgr_add_object(This->m_pDevice->m_pComponentMgr, 
											      This
											      );
        // open ports for decoder and encoder nodes
        //
        if (COMPONENT_TYPE_DECODER == This->m_type)
        {
            COMPONENT_PORT_OPEN open_param;
            int                 rc;

            // setup decoder controls
            rc = c_component_ctrls_setup_decoder(This);
            if (0 != rc)
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_component_ctrls_setup_decoder(%d) failed lerr(%d)\n", 
					   __FUNCTION__,
                       rc
					   );
                return (LinuxKerToZoeStatus(rc));
            }

            This->m_fh.ctrl_handler = &This->m_ctrl_handler;

            // open compressed data port
            open_param.type = COMPONENT_PORT_COMP_IN;

			err = c_component_port_open(This,
									    &open_param,
                                        ZOE_FALSE
									    );
            if (ZOE_FAIL(err))
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_component_port_open(%d) failed(%d)\n", 
					   __FUNCTION__,
					   open_param.type,
                       err
					   );
                return (err);
            }

            // open YUV port
            open_param.type = COMPONENT_PORT_YUV_OUT;

			err = c_component_port_open(This,
									    &open_param,
                                        ZOE_FALSE
									    );
            if (ZOE_FAIL(err))
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_component_port_open(%d) failed(%d)\n", 
					   __FUNCTION__,
					   open_param.type,
                       err
					   );
                return (err);
            }
        }
        else if (COMPONENT_TYPE_ENCODER == This->m_type)
        {
            COMPONENT_PORT_OPEN open_param;
            int                 rc;

            // setup encoder controls
            rc = c_component_ctrls_setup_encoder(This);
            if (0 != rc)
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_component_ctrls_setup_encoder(%d) failed lerr(%d)\n", 
					   __FUNCTION__,
                       rc
					   );
                return (LinuxKerToZoeStatus(rc));
            }

            This->m_fh.ctrl_handler = &This->m_ctrl_handler;

            // open compressed data port
            open_param.type = COMPONENT_PORT_COMP_OUT;

			err = c_component_port_open(This,
									    &open_param,
                                        ZOE_FALSE
									    );
            if (ZOE_FAIL(err))
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_component_port_open(%d) failed(%d)\n", 
					   __FUNCTION__,
					   open_param.type,
                       err
					   );
                return (err);
            }

            // open YUV port
            open_param.type = COMPONENT_PORT_YUV_IN;

			err = c_component_port_open(This,
									    &open_param,
                                        ZOE_FALSE
									    );
            if (ZOE_FAIL(err))
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s c_component_port_open(%d) failed(%d)\n", 
					   __FUNCTION__,
					   open_param.type,
                       err
					   );
                return (err);
            }
        }
	}

	return (err);

}



zoe_errs_t c_component_close(c_component *This)
{
	i_zv_codec	*pMpegCodec = c_device_get_codec(This->m_pDevice);
	int			i;

	// make sure close all the opened ports
	//
	for (i = 0; i < MAX_COMPONENT_PORT; i++)
	{
		c_component_port_close(This, 
							   (COMPONENT_PORT_TYPE)i
							   );
	}

	// release the allocated task
	//
	if (ZOE_NULL_HANDLE != This->m_hTask)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s release_task hTask(%d)\n",
					   __FUNCTION__,
					   This->m_hTask
					   );

		pMpegCodec->release_task(pMpegCodec,
								 This->m_hTask
								 );
		This->m_hTask = ZOE_NULL_HANDLE;
	}

	// remove ourself from the device's component manager
	//
	if (ZOE_NULL_HANDLE != This->m_hObject)
	{
		c_object_mgr_remove_object(This->m_pDevice->m_pComponentMgr, 
								   This->m_hObject
								   );
		This->m_hObject = ZOE_NULL_HANDLE;
	}

    if (COMPONENT_TYPE_DECODER == This->m_type)
    {
        c_component_ctrls_delete_decoder(This);
    }
    else if (COMPONENT_TYPE_ENCODER == This->m_type)
    {
        c_component_ctrls_delete_encoder(This);
    }

    v4l2_fh_del(&This->m_fh);
    v4l2_fh_exit(&This->m_fh);

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_component_set_user_event(c_component *This, 
                                      uint32_t event
                                      )
{
    if ((This->m_user_mode_proxy) &&
        (This->m_pDevice->m_h_component_user_proxy == This->m_hObject)
        )
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s(%d)\n", 
					   __FUNCTION__,
                       event
					   );
        This->m_user_event = event;
        wake_up_interruptible(&This->m_user_mode_event);
        return (ZOE_ERRS_SUCCESS);
    }
    else
    {
        return (ZOE_ERRS_INVALID);
    }
}



#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
long c_component_ioctl(struct file *filp, 
                       void *fh,
                       bool valid_prio,
                       unsigned int cmd, 
					   void *arg
					   )
#else //LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
long c_component_ioctl(struct file *filp, 
                       void *fh,
					   int cmd, 
					   void *arg
					   )
#endif //LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
{
    c_component			*pComponent = (c_component *)v4l2_fh_to_component(fh);
	unsigned int		nSize = _IOC_SIZE(cmd);
	unsigned int		nDir = _IOC_DIR(cmd);
	unsigned int		nType = _IOC_TYPE(cmd);
	unsigned int		nr = _IOC_NR(cmd);
	zoe_errs_t			err = ZOE_ERRS_PARMS;

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   g_ZVV4LDevDBGCompID,
				   "%s() cmd(0x%08x) dir(%d) type(%d) nr(%d) size(%d)\n", 
				   __FUNCTION__,
				   cmd,
                   nDir,
				   nType,
				   nr,
				   nSize
				   );

	switch (cmd)
	{

/////////////////////////////////////////////////////////////////////////////
//
//

		// stream extension interface
		//
		case ZVDEV_STRM_IOCTL_PORT_OPEN:
		{
			PCOMPONENT_PORT_OPEN	pOpen = (PCOMPONENT_PORT_OPEN)arg;

			err = c_component_port_open(pComponent,
									    pOpen,
                                        ZOE_TRUE
									    );
			break;
		}

		case ZVDEV_STRM_IOCTL_PORT_CLOSE:
		{
			PCOMPONENT_PORT_TYPE	pType = (PCOMPONENT_PORT_TYPE)arg;

			err = c_component_port_close(pComponent,
									     *pType
									     );
			break;
		}

		case ZVDEV_STRM_IOCTL_DEVICE_INFO:    
		{
			PZVDEV_INFO	pInfo = (PZVDEV_INFO)arg;

			switch (pComponent->m_pDevice->m_type)
			{
				case ZVV4LDEV_TYPE_USB:
					pInfo->busType = ZOEHAL_BUS_USB;
					break;

				case ZVV4LDEV_TYPE_PCIE:
					pInfo->busType = ZOEHAL_BUS_PCIe;
					break;

				case ZVV4LDEV_TYPE_DIRECT:
					pInfo->busType = ZOEHAL_BUS_HPU;
					break;

				default:
					pInfo->busType = -1;
					break;
			}
			pInfo->devInstance = pComponent->m_pDevice->m_id;
			pInfo->hTask = pComponent->m_hTask;
			pInfo->type = pComponent->m_type;
			err = ZOE_ERRS_SUCCESS;
			break;
		}

		case ZVDEV_STRM_IOCTL_ACQUIRE:
		{
			err = c_component_acquire(pComponent, 
                                      COMPONENT_PORT_END
                                      );
			break;
		}

		case ZVDEV_STRM_IOCTL_PROXY_SET_USER:
        {
	        down(&g_module_lock);
            if ((ZOE_NULL_HANDLE == pComponent->m_pDevice->m_h_component_user_proxy) &&
                !pComponent->m_user_mode_proxy
                )
            {
                init_waitqueue_head(&pComponent->m_user_mode_event);
                pComponent->m_user_event = 0;
                pComponent->m_pDevice->m_h_component_user_proxy = pComponent->m_hObject;
                pComponent->m_user_mode_proxy = ZOE_TRUE;
			    err = ZOE_ERRS_SUCCESS;
            }
            else
            {
                err = ZOE_ERRS_INVALID;
            }
	        up(&g_module_lock);
            break;
        }

		case ZVDEV_STRM_IOCTL_PROXY_TERM_USER:
        {
	        down(&g_module_lock);
            err = c_component_set_user_event(pComponent, 
                                             ZVDEV_PROXY_EVEVT_TERM
                                             );
            if (ZOE_SUCCESS(err))
            {
                pComponent->m_user_mode_proxy = ZOE_FALSE;
                pComponent->m_pDevice->m_h_component_user_proxy = ZOE_NULL_HANDLE;
                pComponent->m_pDevice->m_user_mode_launched = ZOE_FALSE;
            }
	        up(&g_module_lock);
            break;
        }

        case ZVDEV_STRM_IOCTL_PROXY_WAIT_EVENT:
        {
            PZVDEV_PROXY_EVENT	p_event = (PZVDEV_PROXY_EVENT)arg;

            if ((pComponent->m_user_mode_proxy) &&
                (pComponent->m_pDevice->m_h_component_user_proxy == pComponent->m_hObject)
                )
            {
			    if (!wait_event_interruptible_timeout(pComponent->m_user_mode_event, 
                                                      (pComponent->m_user_event != 0),
			                                          msecs_to_jiffies(p_event->timeout_ms)
                                                      ))
                {
	                down(&g_module_lock);
                    if (1 == c_object_mgr_get_number_of_objects(pComponent->m_pDevice->m_pComponentMgr))
                    {
                        p_event->event = ZVDEV_PROXY_EVEVT_TERM;
			            err = ZOE_ERRS_SUCCESS;
                    }
                    else
                    {
				        err = ZOE_ERRS_TIMEOUT;
                    }
	                up(&g_module_lock);
                }
                else
                {
                    p_event->event = pComponent->m_user_event;
                    pComponent->m_user_event = 0;
			        err = ZOE_ERRS_SUCCESS;
                }
            }
            else
            {
			    err = ZOE_ERRS_PARMS;
            }
            break;
        }

/////////////////////////////////////////////////////////////////////////////
//
//

		// device control interface
		//
		case ZVDEV_CNTL_IOCTL_SET_TEST_CONTROL:
		{
			i_zv_codec  *pMpegCodec = c_device_get_codec(pComponent->m_pDevice);

			err = pMpegCodec->set(pMpegCodec,
								  (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
								  pComponent->m_hTask,
								  nr - ZVDEV_CNTL_IOCTL_SET,
								  (zoe_void_ptr_t)arg,
								  ZOE_NULL,
								  nSize
								  );
			break;
		}

		case ZVDEV_CNTL_IOCTL_GET_TEST_CONTROL:
		{
			i_zv_codec	    *pMpegCodec = c_device_get_codec(pComponent->m_pDevice);
			uint32_t	dwGot = 0;

			err = pMpegCodec->get(pMpegCodec,
								  (ZOE_REFGUID)&PROPSETID_ZV_CODEC_CONTROL,
								  pComponent->m_hTask,
								  nr - ZVDEV_CNTL_IOCTL_GET,
								  (zoe_void_ptr_t)arg,
								  (zoe_void_ptr_t)arg,
								  &dwGot
								  );
			break;														
		}


/////////////////////////////////////////////////////////////////////////////
//
//
		// diagnostic interface
		//

		case ZVDEV_DIAG_IOCTL_DMA_WRITE_PTR:
		{
			PZV_CODEC_DIAG_DMA_PTR_STRUCT	pDMA = (PZV_CODEC_DIAG_DMA_PTR_STRUCT)arg;
			i_zv_av_lib						*pCodecLib = c_device_get_codec_lib(pComponent->m_pDevice);

            pDMA->ulXferMode = DMA_BUFFER_MODE_USERPTR;
            pDMA->bSwap = ZOE_FALSE;

			err = pCodecLib->set(pCodecLib,
								 (ZOE_REFGUID)&PROPSETID_ZV_CODEC_DIAG,
								 pComponent->m_hTask,
								 nr - ZVDEV_DIAG_IOCTL_BASE,
								 (zoe_void_ptr_t)pDMA,
								 ZOE_NULL,
								 nSize
								 );
			if (!ZOE_SUCCESS(err))
			{
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               g_ZVV4LDevDBGCompID,
			                   "ZVDEV_DIAG_IOCTL_DMA_WRITE_PTR failed(%d)\n", 
			                   err
			                   );
            }
			break;
		}

		case ZVDEV_DIAG_IOCTL_MEM_WRITE:
		case ZVDEV_DIAG_IOCTL_REG_WRITE:
        case ZVDEV_DIAG_IOCTL_REG_WRITE_EX:
        case ZVDEV_DIAG_IOCTL_ENABLE_WAIT_ISR:
        case ZVDEV_DIAG_IOCTL_DISABLE_WAIT_ISR:
        case ZVDEV_DIAG_IOCTL_SET_ISR:
	    case ZVDEV_DIAG_IOCTL_IPC_REG:
		case ZVDEV_DIAG_IOCTL_IPC_TEST:
		{
			i_zv_av_lib *pCodecLib = c_device_get_codec_lib(pComponent->m_pDevice);

			err = pCodecLib->set(pCodecLib,
								 (ZOE_REFGUID)&PROPSETID_ZV_CODEC_DIAG,
								 pComponent->m_hTask,
								 nr - ZVDEV_DIAG_IOCTL_BASE,
								 (zoe_void_ptr_t)arg,
								 ZOE_NULL,
								 nSize
								 );
			break;
		}

		case ZVDEV_DIAG_IOCTL_DMA_READ_PTR:
		{			
			PZV_CODEC_DIAG_DMA_PTR_STRUCT	pDMA = (PZV_CODEC_DIAG_DMA_PTR_STRUCT)arg;
			i_zv_av_lib						*pCodecLib = c_device_get_codec_lib(pComponent->m_pDevice);
			uint32_t					    dwGot = 0;

            pDMA->ulXferMode = DMA_BUFFER_MODE_USERPTR;
            pDMA->bSwap = ZOE_FALSE;

			err = pCodecLib->get(pCodecLib,
								 (ZOE_REFGUID)&PROPSETID_ZV_CODEC_DIAG,
								 pComponent->m_hTask,
								 nr - ZVDEV_DIAG_IOCTL_BASE,
								 (zoe_void_ptr_t)pDMA,
								 (zoe_void_ptr_t)pDMA,
								 &dwGot
								 );
			if (!ZOE_SUCCESS(err))
			{
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               g_ZVV4LDevDBGCompID,
			                   "ZVDEV_DIAG_IOCTL_DMA_READ_PTR failed(%d)\n", 
			                   err
			                   );
            }
			break;
		}

		case ZVDEV_DIAG_IOCTL_MEM_READ:
		case ZVDEV_DIAG_IOCTL_REG_READ:
		case ZVDEV_DIAG_IOCTL_REG_READ_EX:
        case ZVDEV_DIAG_IOCTL_WAIT_ISR:
		case ZV_CODEC_DIAG_CHIP_VERSION:
		{
			i_zv_av_lib	*pCodecLib = c_device_get_codec_lib(pComponent->m_pDevice);
			uint32_t	dwGot = 0;

			err = pCodecLib->get(pCodecLib,
								 (ZOE_REFGUID)&PROPSETID_ZV_CODEC_DIAG,
								 pComponent->m_hTask,
								 nr - ZVDEV_DIAG_IOCTL_BASE,
								 (zoe_void_ptr_t)arg,
								 (zoe_void_ptr_t)arg,
								 &dwGot
								 );
			break;
		}

		default:
			err = ZOE_ERRS_NOTSUPP;
			break;
	}

	return (ZoeToLinuxKerStatus(err));
}



int c_component_mmap(c_component *This, 
					 struct vm_area_struct *vma
					 )
{
	c_port				*pPort;
	COMPONENT_PORT_TYPE	type;
	long				lErr = 0;
	unsigned long		offset = vma->vm_pgoff << PAGE_SHIFT;
#ifndef CONFIG_ZV4L2_USE_VB2
	struct v4l2_buffer	*pV4l2Buf;
    int					nIndex;
    unsigned long		ulStart = 0;
    unsigned long		ulAddr = 0;
    unsigned long		ulSize = 0;
    unsigned int        buf_index = 0;
    int                 found = 0;
#endif //!CONFIG_ZV4L2_USE_VB2

	// find the port this buffer belongs to
	//
	type = offset >> PORT_MMAP_BUF_TYPE_SHIFT;
	if (type >= MAX_COMPONENT_PORT)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s vma->vm_pgoff(0x%x) offset(0x%x) type(%d) not found!!!\n", 
					   __FUNCTION__,
					   vma->vm_pgoff,
                       offset,
                       type
					   );
		return (-EINVAL);
	}

	pPort = &This->m_Ports[type];

#ifdef CONFIG_ZV4L2_USE_VB2
	offset &= ~PORT_MMAP_BUF_TYPE_MASK;
	offset = offset >> PAGE_SHIFT;
    vma->vm_pgoff = offset;
    lErr = vb2_mmap(&pPort->m_vb2_q, 
                    vma
                    );
#else //!CONFIG_ZV4L2_USE_VB2
	if (V4L2_MEMORY_MMAP != pPort->m_memoryType)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s type(%d) invalid memory type(%d)!!!\n", 
					   __FUNCTION__,
					   type,
					   pPort->m_memoryType
					   );
		return (-EINVAL);
	}

	offset &= ~PORT_MMAP_BUF_TYPE_MASK;
	offset = offset >> PAGE_SHIFT;

	for (nIndex = 0; nIndex < pPort->m_frame_nbs; nIndex++)
	{
		pV4l2Buf = &pPort->m_v4l2Buffers[nIndex];
        for (buf_index = 0; buf_index < pV4l2Buf->length; buf_index++)
        {
		    if ((pV4l2Buf->m.planes[buf_index].m.mem_offset >> PAGE_SHIFT) == offset)
		    {
                found = 1;
			    break;
		    }
        }
        if (found)
        {
            break;
        }
	}

	if (nIndex >= pPort->m_frame_nbs)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s vma->vm_pgoff(0x%x) not found!!!\n", 
					   __FUNCTION__,
					   vma->vm_pgoff
					   );
		return (-EINVAL);
	}

	// VM_IO marks the area as being an mmaped region for I/O to a
	// device. It also prevents the region from being core dumped.
	//
	vma->vm_flags |= VM_IO;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP); 
#else //LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	vma->vm_flags |= VM_RESERVED; 
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)

	ulStart = vma->vm_start;
	ulAddr = (unsigned long)(pPort->m_pBufMem + pV4l2Buf->m.planes[buf_index].m.mem_offset);
	ulSize = vma->vm_end - vma->vm_start;

	while (ulSize > 0) 
	{
		if ((lErr = vm_insert_page(vma, 
								   ulStart, 
								   vmalloc_to_page((void *)ulAddr)
								   )) < 0
								   ) 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s vm_insert_page() start(%d) addr(%d) failed(%d)!!!\n", 
						   __FUNCTION__,
						   ulStart,
						   ulAddr,
						   lErr
						   );
			return (lErr);
		}

		ulStart += PAGE_SIZE;
		ulAddr += PAGE_SIZE;
		ulSize -= PAGE_SIZE;
    }

	vma->vm_ops = &zvv4l_mmap_ops;
	vma->vm_private_data = pV4l2Buf;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s vma->vm_pgoff(0x%x) found buf(%d) lErr(%d)\n", 
				   __FUNCTION__,
				   vma->vm_pgoff,
				   nIndex,
				   lErr
				   );
#endif //CONFIG_ZV4L2_USE_VB2    
	return (lErr);
}



unsigned int c_component_poll(c_component *This,
                              struct file *filp,
							  poll_table *wait
							  ) 
{
	c_port          *pPort;
    unsigned int    rc = 0;
    int             i;
    zoe_bool_t      b_streaming = ZOE_FALSE;


	ENTER_CRITICAL(&This->m_Object)

    for (i = 0; i < MAX_COMPONENT_PORT; i++)
    {
		pPort = &This->m_Ports[i];
		if (pPort->m_valid &&
            (PORT_STATE_STOP != pPort->m_State)
            )
		{
            b_streaming = ZOE_TRUE;
            break;
        }
    }

    if (!b_streaming)
    {
        rc = POLLERR;
        goto poll_exit;
    }

    poll_wait(filp, &This->m_fh.wait, wait);

    for (i = 0; i < MAX_COMPONENT_PORT; i++)
    {
		pPort = &This->m_Ports[i];
		if (pPort->m_valid &&
            (PORT_DIR_NONE != pPort->m_dir)
            )
        {
            if (c_port_buf_rdy(pPort, filp, wait))
            {
                if (PORT_DIR_READ == pPort->m_dir)
                {
                    rc |= POLLIN | POLLRDNORM;
                }
                else
                {
                    rc |= POLLOUT | POLLWRNORM;
                }
            }
        }
    }
    
    if (v4l2_event_pending(&This->m_fh))
    {
        rc |= POLLPRI;
    }

poll_exit:
	LEAVE_CRITICAL(&This->m_Object)
    return (rc);
}


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
    c_component     *pComponent = (c_component *)v4l2_fh_to_component(fh);
    zoe_errs_t      err;
    unsigned int    port_index = (reqbuf->count & 0xFFFF0000) >> 16;
    unsigned int    count = reqbuf->count & 0x0000FFFF;
	c_port		    *pPort;

	if (!reqbuf ||
		(port_index >= MAX_COMPONENT_PORT)
		)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() VIDIOC_REQBUFS unknown sequence(0x%x)!!\n", 
					   __FUNCTION__,
					   reqbuf->count
					   );
		err = ZOE_ERRS_PARMS;
	}
	else
	{
        reqbuf->count = count;
		pPort = &pComponent->m_Ports[port_index];
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



const struct v4l2_ioctl_ops zvv4l_codec_ioctl_ops = 
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




