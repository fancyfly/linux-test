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
// cdevice.h
//
// Description: 
//
//  Device class
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CDEVICE_H__
#define __CDEVICE_H__


#include <linux/types.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#include <media/v4l2-dev.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <media/v4l2-ioctl.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#ifdef __LINUX24__
#include "videodev2.h"
#else
#include <linux/videodev2.h>
#endif
#include <media/v4l2-device.h>
#include <linux/firmware.h>

#include "zoe_types.h"
#include "zv_avlib.h"
#include "zoe_cobject.h"
#include "zoe_cobjectmgr.h"
#include "ccomponent.h"


/////////////////////////////////////////////////////////////////////////////
//
//


typedef enum _ZVV4LDEV_PowerStates
{   
    ZVV4LDEV_PWR_STATE_D0	= 0,	// on
    ZVV4LDEV_PWR_STATE_D1	,		// stand by
    ZVV4LDEV_PWR_STATE_D2	,		// hibernate
    ZVV4LDEV_PWR_STATE_D3			// off
} ZVV4LDEV_PowerStates, *PZVV4LDEV_PowerStates;


typedef enum _ZVV4LDEV_TYPE
{
	ZVV4LDEV_TYPE_NONE		= 0,
	ZVV4LDEV_TYPE_USB,
	ZVV4LDEV_TYPE_PCIE,
	ZVV4LDEV_TYPE_DIRECT,
	ZVV4LDEV_TYPE_END

} ZVV4LDEV_TYPE, *PZVV4LDEV_TYPE;

#define MAX_ZVV4LDEV_TYPE	ZVV4LDEV_TYPE_END


typedef union _ZVV4LDEV_PARENT_DEV
{
#ifndef __LINUX24__
	struct usb_interface	*p_usbintf;
#else //__LINUX24__
	struct usb_device		*p_usbdev;
#endif //__LINUX24__
	struct pci_dev			*p_pciedev;
    struct platform_device  *p_platformdev;
	struct device			*p_dev;
	void					*read;

} ZVV4LDEV_PARENT_DEV, *PZVV4LDEV_PARENT_DEV;


typedef struct _ZVV4LDEV_VARIANT
{
    char	                *fw_name;
} ZVV4LDEV_VARIANT, *PZVV4LDEV_VARIANT;


/////////////////////////////////////////////////////////////////////////////
//
//


#ifndef __CDEVICE_FWD_DEFINED__
#define __CDEVICE_FWD_DEFINED__
typedef struct c_device c_device;
#endif // !__CDEVICE_FWD_DEFINED__


struct c_device 
{
	// public c_object
	//
	c_object				m_Object;

	// c_device
	//
	uint32_t			    m_id;
	ZVV4LDEV_TYPE			m_type;
	ZVV4LDEV_PARENT_DEV		m_device;
	struct device			*m_pGenericDevice;

	// v4L2 dev
	//
	struct v4l2_device	    m_v4l2_dev;

	// V4L video devices
	//
    struct video_device		*m_pVideoDevice;
    struct video_device		*m_pVideoDevice_decoder;
    struct video_device		*m_pVideoDevice_encoder;

	// init data
	//
	ZV_AVLIB_INITDATA		m_initData;
    struct firmware         *m_pFW[ZVAVLIB_IMAGE_MAX];

	// device power state
	//
	ZVV4LDEV_PowerStates	m_PowerState;

	// component manager
	//
	c_object_mgr			*m_pComponentMgr;

	// codec interface objects goes here
	//
	i_zv_av_lib				*m_pCodecLib;
	i_zv_codec				*m_pMpegCodec;

    // user mode firmware init flag
    //
    zoe_bool_t              m_user_mode_launched;
    ZOE_OBJECT_HANDLE       m_h_component_user_proxy;
}; 



c_device * c_device_constructor(c_device *pDevice, 
							    uint32_t id,
							    ZVV4LDEV_TYPE type,
							    PZVV4LDEV_PARENT_DEV pDev,
							    struct device *pGenericDevice
							    );
void c_device_destructor(c_device *This);
zoe_errs_t c_device_create(c_device *This);
zoe_errs_t c_device_close(c_device *This);
zoe_errs_t c_device_power_up(c_device *This);
zoe_errs_t c_device_power_down(c_device *This);


// access functions
//
static inline i_zv_av_lib * c_device_get_codec_lib(c_device *This) {return (This->m_pCodecLib);}
static inline i_zv_codec * c_device_get_codec(c_device *This) {return (This->m_pMpegCodec);}


#define ZVV4LDEV_MAX_DEVICES	24
extern c_device *g_pZvV4lDevices[ZVV4LDEV_MAX_DEVICES];

#endif //__CDEVICE_H__



