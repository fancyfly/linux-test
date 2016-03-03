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
// drventry.c
//
// Description: 
//
//  platform driver entry point
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_sosal.h"
#include "zv_v4ldevversion.h"
#include "cdevice.h"

#ifdef CONFIG_HOST_PLATFORM_X86_LINUX
#define ZPU_V4L2_REGISTER_PLAT_DEV
#endif //CONFIG_HOST_PLATFORM_X86_LINUX
#define MODULE_NAME "mxc_zpu"


/////////////////////////////////////////////////////////////////////////////
//
//

// globals
//
zoe_dbg_comp_id_t   g_ZVV4LDevDBGCompID = 0;
struct semaphore    g_module_lock;
int                 param_launch_user = 0;



/////////////////////////////////////////////////////////////////////////////
//
//
// statics
//
static ZVV4LDEV_VARIANT zpu_mx8dv_drvdata = 
{
    .fw_name    = "firmware.bin",
};


static ZVV4LDEV_VARIANT zpu_mx8x_drvdata = 
{
    .fw_name    = "firmware.bin",
};


/////////////////////////////////////////////////////////////////////////////
//
//

static int zpu_probe(struct platform_device *pdev)
{
    long				lErr = 0;
    c_device			*pDevice = ZOE_NULL;
    ZVV4LDEV_PARENT_DEV	parent_dev;
    zoe_errs_t			err;
    uint32_t		    i = 0;

	// register the component for debug output
	//
	ZOE_DBG_COMP_REGISTER("ZPUDev",
                          g_ZVV4LDevDBGCompID,
				          ZOE_DBG_LVL_ERROR //ZOE_DBG_LVL_TRACE
				          );

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   g_ZVV4LDevDBGCompID,
				   "%s() (platform_dev=%08x) driver v%d.%d.%d\n",
				   __FUNCTION__,
				   pdev,
				   ZV_V4L_VER_MAJOR,
				   ZV_V4L_VER_MINOR,
				   ZV_V4L_VER_BRANCH
				   );

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
//	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(64)))
//    {    
	    if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(32)))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           g_ZVV4LDevDBGCompID,
				           "cannot set 32 bit DMA \n"
				           );
        }
        else
        {
	        dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           g_ZVV4LDevDBGCompID,
				           "32 bit DMA \n"
				           );
        }
//    }
//    else
//    {
//	    dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64));
//	    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
//                       g_ZVV4LDevDBGCompID,
//			           "64 bit DMA \n"
//			           );
//    }
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
//	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(64)))
//    {    
	    if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(32)))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           g_ZVV4LDevDBGCompID,
				           "cannot set 32 bit DMA \n"
				           );
        }
        else
        {
	        dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           g_ZVV4LDevDBGCompID,
				           "32 bit DMA \n"
				           );
        }
//    }
//    else
//    {
//	    dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64));
//	    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
//                       g_ZVV4LDevDBGCompID,
//			           "64 bit DMA \n"
//			           );
//    }
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(32)))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       g_ZVV4LDevDBGCompID,
			           "cannot set 32 bit DMA \n"
			           );
    }
    else
    {
	    dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       g_ZVV4LDevDBGCompID,
			           "32 bit DMA \n"
			           );
    }
#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP

	parent_dev.p_platformdev = pdev;

	// find an empty slot for this device
	//
	down(&g_module_lock);

	for (i = 0; i < ZVV4LDEV_MAX_DEVICES; i++)
	{
		if (ZOE_NULL == g_pZvV4lDevices[i])
		{
			// create and init the device
			//
			pDevice = (c_device *)kmalloc(sizeof(c_device),
										  GFP_KERNEL
										  );
			if (pDevice)
			{
				memset(pDevice,
					   0,
					   sizeof(c_device)
					   );

				if (!c_device_constructor(pDevice,
										  i,
										  ZVV4LDEV_TYPE_DIRECT,
										  &parent_dev,
										  &pdev->dev
										  ))
				{
					kfree((void *)pDevice);
					pDevice = ZOE_NULL;
					lErr = -ENOMEM;
				}
				else
				{
					err = c_device_create(pDevice);
					if (ZOE_SUCCESS(err))
					{
						g_pZvV4lDevices[i] = pDevice;
						dev_set_drvdata(&pdev->dev,
										(void *)pDevice
										);
					}
					else
					{
						c_device_destructor(pDevice);
						kfree((void *)pDevice);
						pDevice = ZOE_NULL;
						lErr = -ENOMEM;
					}
				}
			}
			else
			{
				lErr = -ENOMEM;
			}
			break;
		}
	}

	if (ZVV4LDEV_MAX_DEVICES == i)
	{
		// no empty slot available
		//
		lErr = -ENOMEM;
	}

	up(&g_module_lock);

	if (lErr != 0)
	{
		goto ERROR1;
	}

END:
	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   g_ZVV4LDevDBGCompID,
				   "%s() lErr(%d)\n",
				   __FUNCTION__,
				   lErr
				   );
	return (lErr);

ERROR1:
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   g_ZVV4LDevDBGCompID,
				   "%s() Err1\n",
				   __FUNCTION__,
                   lErr
				   );
	goto END;
}


static int zpu_remove(struct platform_device *pdev)
{
    c_device	*pDevice = (c_device *)dev_get_drvdata(&pdev->dev);

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   g_ZVV4LDevDBGCompID,
				   "%s() (pdev=%08x) (pDevice=%08x)\n",
				   __FUNCTION__,
				   pdev,
				   pDevice
				   );

	down(&g_module_lock);

	if (pDevice)
	{
		g_pZvV4lDevices[pDevice->m_id] = ZOE_NULL;
		c_device_close(pDevice);
		c_device_destructor(pDevice);
		kfree((void *)pDevice);
    }

	up(&g_module_lock);

	dev_set_drvdata(&pdev->dev,
					NULL
					);
    return (0);
}



static int zpu_suspend(struct platform_device *pdev,
				       pm_message_t state
				       )
{
    c_device	*pDevice = (c_device *)dev_get_drvdata(&pdev->dev);

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   g_ZVV4LDevDBGCompID,
				   "%s() \n",
				   __FUNCTION__
				   );
	if (pDevice)
	{
		c_device_power_down(pDevice);
    }

    return 0;
}



static int zpu_resume(struct platform_device *pdev)
{
    c_device	*pDevice = (c_device *)dev_get_drvdata(&pdev->dev);

	zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                   g_ZVV4LDevDBGCompID,
				   "%s() \n",
				   __FUNCTION__
				   );

	if (pDevice)
	{
		c_device_power_up(pDevice);
	}
	return (0);
}



static struct platform_device_id zpu_driver_ids[] = 
{
	{
		.name = "zpu_mx8dv",
		.driver_data = (unsigned long)&zpu_mx8dv_drvdata,
	}, 
    {
		.name = "zpu_mx8x",
		.driver_data = (unsigned long)&zpu_mx8x_drvdata,
	}, 
	{},
};

MODULE_DEVICE_TABLE(platform, zpu_driver_ids);


static const struct of_device_id zpu_dev_match[] = 
{
	{
		.compatible = "nxp,imx8dv-vpu",
		.data = &zpu_mx8dv_drvdata,
	}, 
    {
		.compatible = "nxp,imx8x-vpu",
		.data = &zpu_mx8x_drvdata,
	},        
	{},
};

MODULE_DEVICE_TABLE(of, zpu_dev_match);



#ifdef ZPU_V4L2_REGISTER_PLAT_DEV

static void zpu_dev_release(struct device *dev)
{
	printk("%s() \n",
		   __FUNCTION__
		   );
}


static struct platform_device   zpu_dev = 
{
	.name       = MODULE_NAME,
	.dev.release= zpu_dev_release,
#ifndef ZPU_V4L2_REGISTER_PLAT_DEV
	.id_entry	= zpu_driver_ids,
#endif //!ZPU_V4L2_REGISTER_PLAT_DEV
};

#endif //ZPU_V4L2_REGISTER_PLAT_DEV



static struct platform_driver   zpu_driver =
{
	.driver = {
        .name = MODULE_NAME,
#ifndef ZPU_V4L2_REGISTER_PLAT_DEV
        .of_match_table = zpu_dev_match,
#endif //!ZPU_V4L2_REGISTER_PLAT_DEV
    },
	.probe		= zpu_probe,
#ifndef ZPU_V4L2_REGISTER_PLAT_DEV
	.id_table	= zpu_driver_ids,
#endif //!ZPU_V4L2_REGISTER_PLAT_DEV
	.suspend	= zpu_suspend,
	.resume		= zpu_resume,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	.remove		= zpu_remove,
#else //LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
	.remove		= __devexit_p(zpu_remove),
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
};



static int __init zpu_init_module(void)
{
	long	lErr;

	printk("%s() \n",
		   __FUNCTION__
		   );

    // init sosal
    //
    zoe_sosal_init();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
	sema_init(&g_module_lock, 1);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
	init_MUTEX(&g_module_lock);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)

#ifdef ZPU_V4L2_REGISTER_PLAT_DEV
	lErr = platform_device_register(&zpu_dev);
	if (lErr)
    {
	    printk("%s() platform_device_register failed(%d)\n",
		       __FUNCTION__,
               lErr
		       );
		return (lErr);
    }
#endif //ZPU_V4L2_REGISTER_PLAT_DEV

    lErr = platform_driver_register(&zpu_driver);
	if (lErr)
    {
	    printk("%s() platform_driver_register failed(%d)\n",
		       __FUNCTION__,
               lErr
		       );
		return (lErr);
    }
	return (lErr);
}


static void __exit zpu_fini_module(void)
{
	printk("%s() \n",
		   __FUNCTION__
		   );

	platform_driver_unregister(&zpu_driver);

#ifdef ZPU_V4L2_REGISTER_PLAT_DEV
    platform_device_unregister(&zpu_dev);
#endif //ZPU_V4L2_REGISTER_PLAT_DEV

	// exit sosal
	//
    zoe_sosal_term();
	return;
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Linux VPU driver for Freescale i.MX/MXC");
MODULE_LICENSE("Dual BSD/GPL");

module_init(zpu_init_module);
module_exit(zpu_fini_module);

module_param(param_launch_user, int, 0644);
MODULE_PARM_DESC(param_launch_user, "Launch User Mode Firmware Process. (Default 0)");




