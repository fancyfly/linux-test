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
// cdevice.c
//
// Description: 
//
//  Device class
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include <linux/version.h>
#include <linux/module.h>
#ifdef __LINUX24__
#include <linux/videodev.h>
#include "videodev2.h"
#else //!__LINUX24__
#include <linux/videodev2.h>
#endif //__LINUX24__
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
//#include <linux/video_decoder.h>
//#include <linux/video_encoder.h>
#include <linux/list.h>
#include <linux/unistd.h>
//#include <linux/byteorder/swab.h>
#include <linux/pagemap.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/page-flags.h>
#include <asm/uaccess.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
#include <asm/switch_to.h>
#else
#include <asm/system.h>
#endif
#include <asm/io.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <asm/semaphore.h>
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)

#include "zoe_dbg.h"
#include "zv_devioctl.h"
#include "ccomponent.h"
#include "cdevice.h"
#include "zoe_linuxker_errmap.h"
#include "zvfops.h"
#include "objids.h"

extern zoe_dbg_comp_id_t    g_ZVV4LDevDBGCompID;
extern struct semaphore     g_module_lock;
extern int                  param_launch_user;

#define BOOTCODE_FILENAME   "/sbin/sfw_boot_code.bin"
#define CUSTBOOT_FILENAME   "/sbin/sfw_custom_boot.bin"
#define SFW_FILENAME        "/sbin/sfw_spu.bin"
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
#define FW_FILENAME         "/sbin/firmware.bin"
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#define FW_FILENAME         "/sbin/firmware.bin"
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
#define FW_FILENAME         "/sbin/firmware.bin"
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

#define DRV_USE_KMALLOC     1

static char *   s_FILENAME[ZVAVLIB_IMAGE_MAX] =
{
    BOOTCODE_FILENAME,
    CUSTBOOT_FILENAME,
    SFW_FILENAME,
    FW_FILENAME
};

/////////////////////////////////////////////////////////////////////////////
//
//

c_device	*g_pZvV4lDevices[ZVV4LDEV_MAX_DEVICES] = 
{ 
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};



/////////////////////////////////////////////////////////////////////////////
//
//
static void lauch_user_mode_fw(c_device *p_device)
{
    static char *path = "/sbin/launch.sh";
    static char bus_type[16];
    static char dev_instance[16];
    char        *argv[4];
    static char *envp[] = {"HOME=/",
                           "TERM=linux",
                           "PATH=/sbin:/bin:/usr/sbin:/usr/bin", 
                           NULL
                           };
    int         ret;

    if (!p_device->m_user_mode_launched)
    {
        p_device->m_user_mode_launched = ZOE_TRUE;

        // setup parameters
        //
	    switch (p_device->m_type)
	    {
		    case ZVV4LDEV_TYPE_USB:
                sprintf(bus_type, "%d", ZOEHAL_BUS_USB);
			    break;

		    case ZVV4LDEV_TYPE_PCIE:
                sprintf(bus_type, "%d", ZOEHAL_BUS_PCIe);
			    break;

		    case ZVV4LDEV_TYPE_DIRECT:
		    default:
                sprintf(bus_type, "%d", ZOEHAL_BUS_HPU);
			    break;
	    }
        sprintf(dev_instance, "%d", p_device->m_id);

        argv[0] = path;
        argv[1] = bus_type;
        argv[2] = dev_instance;
        argv[3] = NULL;

        ret = call_usermodehelper(path, argv, envp, UMH_WAIT_EXEC);
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
				       "%s() launching %s ret(%d)\n", 
				       __FUNCTION__,
				       path,
                       ret
				       );
    }
};



/////////////////////////////////////////////////////////////////////////////
//
//

// V4L interfaces
//
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static int zvv4l_open(struct file *filp,
                      COMPONENT_TYPE type
                      ) 
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int zvv4l_open(struct inode *inode, 
					  struct file *filp,
                      COMPONENT_TYPE type
					  ) 
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
{
	long		        lErr = 0;
    int			        i;
	c_device		    *pDevice;
	c_component	        *pComponent;
    struct video_device *vdev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    int			        minor = video_devdata(filp)->minor;
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
	int			        minor = MINOR(inode->i_rdev);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    zoe_errs_t          err;

	down(&g_module_lock);

	for (i = 0; i < ZVV4LDEV_MAX_DEVICES; i++) 
	{
        pDevice = g_pZvV4lDevices[i];
        if (pDevice)
        {
            switch (type)
            {
                case COMPONENT_TYPE_CODEC:
                    vdev = pDevice->m_pVideoDevice;
                    break;
                case COMPONENT_TYPE_DECODER:
                    vdev = pDevice->m_pVideoDevice_decoder;
                    break;
                case COMPONENT_TYPE_ENCODER:
                    vdev = pDevice->m_pVideoDevice_encoder;
                    break;
                default:
                    vdev = NULL;
                    break;
            }
        }
        if (pDevice && 
            vdev &&
            (vdev->minor == minor) 
            ) 
		{
		 	// got it
		 	goto _do_open;
        }
    }

	up(&g_module_lock);

	// not found
	//
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
				   "%s() minor(%d) not found\n", 
				   __FUNCTION__,
				   minor
				   );
	return (-ENODEV);

_do_open:

	// create and open the component 
	//
#if (DRV_USE_KMALLOC == 1) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	pComponent = kmalloc(sizeof(c_component), 
						 GFP_KERNEL
						 );
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	pComponent = vmalloc(sizeof(c_component));
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	if (pComponent)
	{
		if (!c_component_constructor(pComponent, 
									 vdev,
									 pDevice,
                                     type
									 ))
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           g_ZVV4LDevDBGCompID,
						   "%s() minor(%d) c_component_constructor failed!!!\n", 
						   __FUNCTION__,
						   minor
						   );
#if (DRV_USE_KMALLOC == 1) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
			kfree((void *)pComponent);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
			vfree((void *)pComponent);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
			lErr = -ENOMEM;
		}
		else
		{
			// component open will add the component to the object manager
			//
			err = c_component_open(pComponent, 
                                   filp
                                   );
            if (ZOE_FAIL(err))
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                               g_ZVV4LDevDBGCompID,
						       "%s() minor(%d) c_component_open failed(%d)!!!\n", 
						       __FUNCTION__,
						       minor
						       );
                c_component_close(pComponent);

#if (DRV_USE_KMALLOC == 1) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
			    kfree((void *)pComponent);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
			    vfree((void *)pComponent);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
			    lErr = ZoeToLinuxKerStatus(err);
            }
            else
            {
			    // save to driver data
			    //
		        video_set_drvdata(vdev, 
						          pComponent
						          );

            }
		}
	}
	else
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       g_ZVV4LDevDBGCompID,
					   "%s() minor(%d) can not allocate pComponent\n", 
					   __FUNCTION__,
					   minor
					   );
		lErr = -ENOMEM;
	}

    if ((0 != param_launch_user) &&
        (0 == lErr)
        )
    {
        lauch_user_mode_fw(pDevice);
    }

	up(&g_module_lock);
	return (lErr);
}



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static int zvv4l_open_codec(struct file *filp) 
{
    return (zvv4l_open(filp,
                       COMPONENT_TYPE_CODEC
                       ));
}
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int zvv4l_open_codec(struct inode *inode, 
					        struct file *filp
					        ) 
{
    return (zvv4l_open(inode, 
					   filp,
                       COMPONENT_TYPE_CODEC
					   ));
}
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static int zvv4l_open_decoder(struct file *filp) 
{
    return (zvv4l_open(filp,
                       COMPONENT_TYPE_DECODER
                       ));
}
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int zvv4l_open_decoder(struct inode *inode, 
					          struct file *filp
					          ) 
{
    return (zvv4l_open(inode, 
					   filp,
                       COMPONENT_TYPE_DECODER
					   ));
}
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static int zvv4l_open_encoder(struct file *filp) 
{
    return (zvv4l_open(filp,
                       COMPONENT_TYPE_ENCODER
                       ));
}
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int zvv4l_open_encoder(struct inode *inode, 
					          struct file *filp
					          ) 
{
    return (zvv4l_open(inode, 
					   filp,
                       COMPONENT_TYPE_ENCODER
					   ));
}
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static int zvv4l_close(struct file *filp) 
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int zvv4l_close(struct inode *inode, 
					   struct file *filp
					   ) 
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
{
	long		lErr = -EPERM;
    int			i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    int			minor = video_devdata(filp)->minor;
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
    int			minor = MINOR(inode->i_rdev);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    c_device    *pDevice;
    c_component	*pComponent = (c_component *)v4l2_fh_to_component(filp->private_data);

	down(&g_module_lock);

    for (i = 0; i < ZVV4LDEV_MAX_DEVICES; i++) 
    {
        pDevice = g_pZvV4lDevices[i];
        if (pDevice && 
            pComponent &&
            pComponent->m_pVideoDev &&
            (pComponent->m_pVideoDev->minor == minor) 
            ) 
        {
            // got it
            goto _do_close;
        }
    }

	up(&g_module_lock);

    // not found
    //
    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                   g_ZVV4LDevDBGCompID,
				   "%s() minor(%d) not found\n", 
				   __FUNCTION__,
				   minor
				   );
    return (-ENODEV);

_do_close:
	if (pComponent)
	{
		c_component_close(pComponent);
		c_component_destructor(pComponent);
#if (DRV_USE_KMALLOC == 1) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
		kfree((void *)pComponent);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
		vfree((void *)pComponent);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		lErr = 0;
	}

    // close the proxy if it is the last one
    //
    if ((0 != param_launch_user) &&
        (1 == c_object_mgr_get_number_of_objects(pDevice->m_pComponentMgr)) &&
        (ZOE_NULL_HANDLE != pDevice->m_h_component_user_proxy)
        )
    {
        pComponent = (c_component *)c_object_mgr_get_object_by_handle(pDevice->m_pComponentMgr, 
                                                                      pDevice->m_h_component_user_proxy
                                                                      );
        if (pComponent && 
            c_component_is_user_mode_proxy(pComponent)
            )
        {
            c_component_set_user_event(pComponent, 
                                       ZVDEV_PROXY_EVEVT_TERM
                                       );
        }
    }

	up(&g_module_lock);
    return (lErr);
}



static unsigned int zvv4l_poll(struct file *filp, 
							   poll_table *wait
							   ) 
{
	unsigned int    rc = 0;
    c_component	    *pComponent = (c_component *)v4l2_fh_to_component(filp->private_data);

	if (pComponent)
	{
        return (c_component_poll(pComponent,
                                 filp,
								 wait
								 ));
	}
    else
    {
        rc = POLLERR;
    }

    return (rc);
}



static void zvv4l_mmap_open(struct vm_area_struct *vma) 
{
    return;
}


static void zvv4l_mmap_close(struct vm_area_struct *vma) 
{
    return;
}



struct vm_operations_struct zvv4l_mmap_ops = 
{
	open:	zvv4l_mmap_open,
	close:	zvv4l_mmap_close,
};



static int zvv4l_mmap(struct file *filp, 
					  struct vm_area_struct *vma
					  )
{
	long		lErr = -EPERM;
    c_component	*pComponent = (c_component *)v4l2_fh_to_component(filp->private_data);

	vma->vm_ops = &zvv4l_mmap_ops;
    
	if (pComponent)
	{
        return (c_component_mmap(pComponent, 
						         vma
						         ));
	}

    return (lErr);
}




#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static struct v4l2_file_operations zvv4l_fops =
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static struct file_operations zvv4l_fops = 
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
{
	owner:          THIS_MODULE,
	open:           zvv4l_open_codec,
	unlocked_ioctl: video_ioctl2,
	release:        zvv4l_close,
	poll:           zvv4l_poll,
	mmap:           zvv4l_mmap,
};


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static struct v4l2_file_operations zvv4l_fops_decoder =
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static struct file_operations zvv4l_fops_decoder = 
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
{
	owner:          THIS_MODULE,
	open:           zvv4l_open_decoder,
	unlocked_ioctl: video_ioctl2,
	release:        zvv4l_close,
	poll:           zvv4l_poll,
	mmap:           zvv4l_mmap,
};


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static struct v4l2_file_operations zvv4l_fops_encoder =
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static struct file_operations zvv4l_fops_encoder = 
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
{
	owner:          THIS_MODULE,
	open:           zvv4l_open_encoder,
	unlocked_ioctl: video_ioctl2,
	release:        zvv4l_close,
	poll:           zvv4l_poll,
	mmap:           zvv4l_mmap,
};


extern const struct v4l2_ioctl_ops zvv4l_codec_ioctl_ops;
extern const struct v4l2_ioctl_ops zvv4l_decoder_ioctl_ops;
extern const struct v4l2_ioctl_ops zvv4l_encoder_ioctl_ops;


static struct video_device zvv4l_videodevice = 
{
	.name	= "MX8 codec",
	.fops	= &zvv4l_fops,
	.ioctl_ops = &zvv4l_codec_ioctl_ops,
	.minor	= -1,
    .vfl_dir = VFL_DIR_M2M,
};


static struct video_device zvv4l_videodevice_decoder = 
{
	.name	= "MX8 decoder",
	.fops	= &zvv4l_fops_decoder,
	.ioctl_ops = &zvv4l_decoder_ioctl_ops,
	.minor	= -1,
    .vfl_dir = VFL_DIR_M2M,
};


static struct video_device zvv4l_videodevice_encoder = 
{
	.name	= "MX8 encoder",
	.fops	= &zvv4l_fops_encoder,
	.ioctl_ops = &zvv4l_encoder_ioctl_ops,
	.minor	= -1,
    .vfl_dir = VFL_DIR_M2M,
};


/////////////////////////////////////////////////////////////////////////////
//
//


#ifdef __LINUX24__
inline struct video_device *video_device_alloc(void) 
{
    struct video_device *vfd;

    vfd = kmalloc(sizeof(*vfd), GFP_KERNEL);
    if (NULL == vfd) 
    {
        return NULL;
    }
    memset(vfd, 0, sizeof(*vfd));
    return (vfd);
}


inline void video_device_release(struct video_device *vfd) 
{
	if (vfd) 
    {
        kfree(vfd);
    }
}
#endif //__LINUX24__


static zoe_errs_t c_device_callback(zoe_void_ptr_t Context, 
								    uint32_t dwCode, 
								    zoe_void_ptr_t pParam
								    ) 
{
	return (ZOE_ERRS_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////
//
//


// constructor
//
c_device * c_device_constructor(c_device *pDevice, 
							    uint32_t id,
							    ZVV4LDEV_TYPE type,
							    PZVV4LDEV_PARENT_DEV pDev,
							    struct device *pGenericDevice
							    )
{
	if (pDevice)
	{
        int     i;

		// c_object
		//
		c_object_constructor(&pDevice->m_Object,
							 ZOE_NULL,
                             OBJECT_ZOE_V4L2_DEVICE,
		   					 OBJECT_CRITICAL_LIGHT
		   					 );

		// initialize members
		//
		pDevice->m_id = id;
		if ((type > MAX_ZVV4LDEV_TYPE) ||
			(type <= ZVV4LDEV_TYPE_NONE)
			)
		{
			return (ZOE_NULL);
		}
		pDevice->m_type = type;
		pDevice->m_pVideoDevice = ZOE_NULL;
		pDevice->m_pVideoDevice_decoder = ZOE_NULL;
		pDevice->m_pVideoDevice_encoder = ZOE_NULL;


		pDevice->m_device.read = pDev->read;
		pDevice->m_pGenericDevice = pGenericDevice;

        // config init data
        //
        memset(&pDevice->m_initData, 
               0, 
               sizeof(pDevice->m_initData)
               );
		switch (type) 
		{
    		case ZVV4LDEV_TYPE_USB:
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_BOOT_CODE] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_SFW]  = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_TRUE;

				pDevice->m_initData.codecInitData.bDontInitHW       = ZOE_FALSE;
        		pDevice->m_initData.codecInitData.BusType           = ZOEHAL_BUS_USB;
        		pDevice->m_initData.codecInitData.Instance          = id;
        		pDevice->m_initData.codecInitData.MemSize           = 512;

        		pDevice->m_initData.codecInitData.BusData           = ZOE_NULL;
        		pDevice->m_initData.codecInitData.BusDataSize       = 0;
        		break;

    		case ZVV4LDEV_TYPE_PCIE:
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_BOOT_CODE] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_SFW]  = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_FALSE;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_BOOT_CODE] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_SFW]  = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_FALSE;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_BOOT_CODE] = ZOE_TRUE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] = ZOE_TRUE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_SFW]  = ZOE_TRUE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_TRUE;
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

				pDevice->m_initData.codecInitData.bDontInitHW       = ZOE_FALSE;
        		pDevice->m_initData.codecInitData.BusType           = ZOEHAL_BUS_PCIe;
        		pDevice->m_initData.codecInitData.Instance          = id;
        		pDevice->m_initData.codecInitData.MemSize           = 512;

        		pDevice->m_initData.codecInitData.BusData           = pDevice->m_device.p_pciedev;
        		pDevice->m_initData.codecInitData.BusDataSize       = sizeof(struct pci_dev);
        		break;

    		case ZVV4LDEV_TYPE_DIRECT:
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_BOOT_CODE] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] = ZOE_FALSE;
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_SFW]  = ZOE_FALSE;
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#ifdef CONFIG_HOST_PLATFORM_ARM64
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_TRUE;
#else //!CONFIG_HOST_PLATFORM_ARM64
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_FALSE;
#endif //CONFIG_HOST_PLATFORM_ARM64
#else // ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_MX8
				pDevice->m_initData.bDownloadFW[ZVAVLIB_IMAGE_FW]   = ZOE_FALSE;
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8
				pDevice->m_initData.codecInitData.bDontInitHW       = ZOE_TRUE;
        		pDevice->m_initData.codecInitData.BusType           = ZOEHAL_BUS_HPU;
        		pDevice->m_initData.codecInitData.Instance          = id;
        		pDevice->m_initData.codecInitData.MemSize           = 512;

        		pDevice->m_initData.codecInitData.BusData           = pDevice->m_device.p_platformdev;
        		pDevice->m_initData.codecInitData.BusDataSize       = sizeof(struct platform_device);
        		break;

			default:
				return (ZOE_NULL);
		}

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
   		pDevice->m_initData.codecInitData.ChipType          = ZOEHAL_CHIP_CAFE;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
   		pDevice->m_initData.codecInitData.ChipType          = ZOEHAL_CHIP_iMX8;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
   		pDevice->m_initData.codecInitData.ChipType          = ZOEHAL_CHIP_CHISEL;
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

        for (i = 0; i < ZVAVLIB_IMAGE_MAX; i++)
        {
		    if (pDevice->m_initData.bDownloadFW[i])
		    {
			    long    lErr;

			    lErr = zvf_load(s_FILENAME[i], 
							    &pDevice->m_initData.pFW[i], 
							    &pDevice->m_initData.FWSize[i]
							    );
			    if (lErr)
			    {
				    pDevice->m_initData.bDownloadFW[i] = ZOE_FALSE;

				    if (pDevice->m_initData.pFW[i])
				    {
					    vfree(pDevice->m_initData.pFW[i]);
					    pDevice->m_initData.pFW[i] = ZOE_NULL;
				    }
			    }
		    }
        }

		pDevice->m_PowerState = ZVV4LDEV_PWR_STATE_D0;

		pDevice->m_pComponentMgr = ZOE_NULL;

		pDevice->m_pCodecLib = ZOE_NULL;
		pDevice->m_pMpegCodec = ZOE_NULL;

        pDevice->m_user_mode_launched = ZOE_FALSE;
        pDevice->m_h_component_user_proxy = ZOE_NULL_HANDLE;
	}

	return (pDevice);
}



void c_device_destructor(c_device *This)
{
    int i;

    for (i = 0; i < ZVAVLIB_IMAGE_MAX; i++)
    {
	    if (This->m_initData.pFW[i])
	    {
		    vfree(This->m_initData.pFW[i]);
		    This->m_initData.pFW[i] = ZOE_NULL;
        }
	}

	if (This->m_pComponentMgr)
	{
		c_object_mgr_destructor(This->m_pComponentMgr);
		kfree((void *)This->m_pComponentMgr);
		This->m_pComponentMgr = ZOE_NULL;
	}

	if (This->m_pVideoDevice)
	{
		video_unregister_device(This->m_pVideoDevice);
		This->m_pVideoDevice = ZOE_NULL;
	}

	if (This->m_pVideoDevice_decoder)
	{
		video_unregister_device(This->m_pVideoDevice_decoder);
		This->m_pVideoDevice_decoder = ZOE_NULL;
	}

	if (This->m_pVideoDevice_encoder)
	{
		video_unregister_device(This->m_pVideoDevice_encoder);
		This->m_pVideoDevice_encoder = ZOE_NULL;
	}

	c_object_destructor(&This->m_Object);
}



zoe_errs_t c_device_create(c_device *This)
{
	zoe_errs_t	err;
    int         i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s\n", 
				   __FUNCTION__
				   );
	// V4L2 register device
	//
	if (v4l2_device_register(This->m_pGenericDevice, &This->m_v4l2_dev))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
			   "%s()-> Unable to register v4l\n", 
			   __FUNCTION__
			   );
		return (ZOE_ERRS_FAIL);
	}

	// allocate and register V4L codec device
	//
	This->m_pVideoDevice = video_device_alloc();
	if (This->m_pVideoDevice)
	{
#if 1
        strncpy(This->m_pVideoDevice->name, zvv4l_videodevice.name, sizeof(zvv4l_videodevice.name));
        This->m_pVideoDevice->fops = zvv4l_videodevice.fops;
        This->m_pVideoDevice->ioctl_ops = zvv4l_videodevice.ioctl_ops;
        This->m_pVideoDevice->minor = zvv4l_videodevice.minor;
        This->m_pVideoDevice->vfl_dir = zvv4l_videodevice.vfl_dir;
#else
		memcpy(This->m_pVideoDevice, 
			   &zvv4l_videodevice, 
			   sizeof(struct video_device)
			   );
#endif
#ifndef __LINUX24__
		video_set_drvdata(This->m_pVideoDevice, 
						  NULL
						  );
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
	    This->m_pVideoDevice->v4l2_dev = &This->m_v4l2_dev;
	    This->m_pVideoDevice->dev_parent = This->m_pGenericDevice;
#else //LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
	    This->m_pVideoDevice->parent = This->m_pGenericDevice;
#endif //LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
        This->m_pVideoDevice->dev = This->m_pGenericDevice;
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
        This->m_pVideoDevice->release = video_device_release;
#endif //!__LINUX24__

		if (video_register_device(This->m_pVideoDevice, 
								  VFL_TYPE_GRABBER, 
								  -1
								  )) 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s()-> Unable to register video device\n", 
						   __FUNCTION__
						   );
            video_device_release(This->m_pVideoDevice);
            This->m_pVideoDevice = ZOE_NULL;
	        v4l2_device_unregister(&This->m_v4l2_dev);
        } 
		else 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s()-> Registered v4l codec minor(%d)\n", 
						   __FUNCTION__, 
						   This->m_pVideoDevice->minor
						   );
        }       
	}

	// allocate and register V4L decoder device
	//
	This->m_pVideoDevice_decoder = video_device_alloc();
	if (This->m_pVideoDevice_decoder)
	{
#if 1
        strncpy(This->m_pVideoDevice_decoder->name, zvv4l_videodevice_decoder.name, sizeof(zvv4l_videodevice_decoder.name));
        This->m_pVideoDevice_decoder->fops = zvv4l_videodevice_decoder.fops;
        This->m_pVideoDevice_decoder->ioctl_ops = zvv4l_videodevice_decoder.ioctl_ops;
        This->m_pVideoDevice_decoder->minor = zvv4l_videodevice_decoder.minor;
        This->m_pVideoDevice_decoder->vfl_dir = zvv4l_videodevice_decoder.vfl_dir;
#else
		memcpy(This->m_pVideoDevice_decoder, 
			   &zvv4l_videodevice_decoder, 
			   sizeof(struct video_device)
			   );
#endif
#ifndef __LINUX24__
		video_set_drvdata(This->m_pVideoDevice_decoder, 
						  NULL
						  );
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
	    This->m_pVideoDevice_decoder->v4l2_dev = &This->m_v4l2_dev;
	    This->m_pVideoDevice_decoder->dev_parent = This->m_pGenericDevice;
#else //LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
	    This->m_pVideoDevice_decoder->parent = This->m_pGenericDevice;
#endif //LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
        This->m_pVideoDevice_decoder->dev = This->m_pGenericDevice;
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
        This->m_pVideoDevice_decoder->release = video_device_release;
#endif //!__LINUX24__

		if (video_register_device(This->m_pVideoDevice_decoder, 
								  VFL_TYPE_GRABBER, 
								  -1
								  )) 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s()-> Unable to register video decoder device\n", 
						   __FUNCTION__
						   );
            video_device_release(This->m_pVideoDevice_decoder);
            This->m_pVideoDevice_decoder = ZOE_NULL;
        } 
		else 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s()-> Registered v4l decoder minor(%d)\n", 
						   __FUNCTION__, 
						   This->m_pVideoDevice_decoder->minor
						   );
        }       
	}

	// allocate and register V4L encoder device
	//
	This->m_pVideoDevice_encoder = video_device_alloc();
	if (This->m_pVideoDevice_encoder)
	{
#if 1
        strncpy(This->m_pVideoDevice_encoder->name, zvv4l_videodevice_encoder.name, sizeof(zvv4l_videodevice_encoder.name));
        This->m_pVideoDevice_encoder->fops = zvv4l_videodevice_encoder.fops;
        This->m_pVideoDevice_encoder->ioctl_ops = zvv4l_videodevice_encoder.ioctl_ops;
        This->m_pVideoDevice_encoder->minor = zvv4l_videodevice_encoder.minor;
        This->m_pVideoDevice_encoder->vfl_dir = zvv4l_videodevice_encoder.vfl_dir;
#else
		memcpy(This->m_pVideoDevice_encoder, 
			   &zvv4l_videodevice_encoder, 
			   sizeof(struct video_device)
			   );
#endif
#ifndef __LINUX24__
		video_set_drvdata(This->m_pVideoDevice_decoder, 
						  NULL
						  );
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
	    This->m_pVideoDevice_encoder->v4l2_dev = &This->m_v4l2_dev;
	    This->m_pVideoDevice_encoder->dev_parent = This->m_pGenericDevice;
#else //LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
	    This->m_pVideoDevice_encoder->parent = This->m_pGenericDevice;
#endif //LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
        This->m_pVideoDevice_encoder->dev = This->m_pGenericDevice;
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
        This->m_pVideoDevice_encoder->release = video_device_release;
#endif //!__LINUX24__

		if (video_register_device(This->m_pVideoDevice_encoder, 
								  VFL_TYPE_GRABBER, 
								  -1
								  )) 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s()-> Unable to register video encoder device\n", 
						   __FUNCTION__
						   );
            video_device_release(This->m_pVideoDevice_encoder);
            This->m_pVideoDevice_encoder = ZOE_NULL;
        } 
		else 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           g_ZVV4LDevDBGCompID,
						   "%s()-> Registered v4l encoder minor(%d)\n", 
						   __FUNCTION__, 
						   This->m_pVideoDevice_encoder->minor
						   );
        }       
	}

	// create component manager
	//
	This->m_pComponentMgr = (c_object_mgr *)kmalloc(sizeof(c_object_mgr), 
												    GFP_KERNEL
												    );
	if (!c_object_mgr_constructor(This->m_pComponentMgr,
								  &This->m_Object,
								  OBJECT_CRITICAL_HEAVY,
								  ZOE_NULL
								  ))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s() c_object_mgr_constructor Failed!!!!\n",
					   __FUNCTION__
					   );
		c_device_close(This);
		return (ZOE_ERRS_NOMEMORY);
	}

	// initialize codec library
	//
	err = zv_init_av_library(This->m_device.read,
						     (void *)This->m_pGenericDevice,
						     &This->m_initData,
						     &This->m_pCodecLib
						     );

	// get pointers to library interfaces
	//
	err = !ZOE_SUCCESS(err) ? err : i_zv_av_lib_get_codec(This->m_pCodecLib, &This->m_pMpegCodec);

	if (!ZOE_SUCCESS(err))
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       g_ZVV4LDevDBGCompID,
					   "%s zv_init_av_library() Failed() Status(%d)\n", 
					   __FUNCTION__,
					   err
					   );
		c_device_close(This);
		return (err);
	}

	err = i_zv_generic_device_init_device(This->m_pCodecLib,
									      c_device_callback,
									      This,
									      ZOE_NULL
									      );

    for (i = 0; i < ZVAVLIB_IMAGE_MAX; i++)
    {
	    if (This->m_initData.pFW[i])
	    {
		    vfree(This->m_initData.pFW[i]);
		    This->m_initData.pFW[i] = ZOE_NULL;
        }
	}

	if (!ZOE_SUCCESS(err))
	{
		c_device_close(This);
		return (err);
	}
	c_object_init(&This->m_Object);
	return (err);
}



zoe_errs_t c_device_close(c_device *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   g_ZVV4LDevDBGCompID,
				   "%s\n", 
				   __FUNCTION__
				   );

	// close all the remaining components
	//
	while (0 != c_object_mgr_get_number_of_objects(This->m_pComponentMgr))
	{
		c_component	*pComponent = c_object_mgr_get_object_by_index(This->m_pComponentMgr, 
															       0
															       );
		if (pComponent)
		{
			c_component_close(pComponent);
			c_component_destructor(pComponent);
#if (DRV_USE_KMALLOC == 1) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
			kfree((void *)pComponent);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
			vfree((void *)pComponent);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		}
	}

	// unregister video codec device
	//
	if (This->m_pVideoDevice)
	{
		video_unregister_device(This->m_pVideoDevice);
		This->m_pVideoDevice = ZOE_NULL;
	}

	// unregister video decoder device
	//
	if (This->m_pVideoDevice_decoder)
	{
		video_unregister_device(This->m_pVideoDevice_decoder);
		This->m_pVideoDevice_decoder = ZOE_NULL;
	}

	// unregister video encoder device
	//
	if (This->m_pVideoDevice_encoder)
	{
		video_unregister_device(This->m_pVideoDevice_encoder);
		This->m_pVideoDevice_encoder = ZOE_NULL;
	}

	// unregister v4l2 device
	//
    v4l2_device_unregister(&This->m_v4l2_dev);

	// release all interfaces we have acquired
	//
	if (This->m_pMpegCodec)
	{
		i_zv_generic_device_release(This->m_pMpegCodec);
		This->m_pMpegCodec = ZOE_NULL;
	}

	if (This->m_pCodecLib)
	{
		i_zv_generic_device_release(This->m_pCodecLib);
		zv_done_av_library(This->m_pCodecLib);
		This->m_pCodecLib = ZOE_NULL;
	}

	c_object_done(&This->m_Object);

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_device_power_up(c_device *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	if (ZOE_NULL != This->m_pCodecLib)
	{
		err = This->m_pCodecLib->enable(This->m_pCodecLib);
	}
	This->m_PowerState = ZVV4LDEV_PWR_STATE_D0;
	return (err);
}



zoe_errs_t c_device_power_down(c_device *This)
{
	zoe_errs_t	err = ZOE_ERRS_SUCCESS;

	if (ZOE_NULL != This->m_pCodecLib)
	{
		err = This->m_pCodecLib->disable(This->m_pCodecLib);
	}
	This->m_PowerState = ZVV4LDEV_PWR_STATE_D3;
	return (err);
}



