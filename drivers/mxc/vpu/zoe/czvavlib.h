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
// czvavlib.h
//
// Description: 
//
//  Header file for zpu library.
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CZVAVLIB_H__
#define __CZVAVLIB_H__


#include "zoe_types.h"
#include "zoe_cobject.h"
#include "zoe_hal.h"
#include "zv_avlib.h"
#include "czvcodec.h"
#include "zoe_ipc_srv.h"
#include "zoe_module_mgr.h"

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#ifndef CONFIG_HOST_PLATFORM_ARM64
#define _NO_SCU_API
#else // CONFIG_HOST_PLATFORM_ARM64
#undef _NO_SCU_API
#include <soc/imx8/sc/sci.h>
#endif // !CONFIG_HOST_PLATFORM_ARM64
#else // !ZOE_TARGET_CHIP_MX8
#define _NO_SCU_API
#endif // ZOE_TARGET_CHIP_MX8



#ifdef __cplusplus
extern "C" {
#endif


#ifndef __CZVAVLIB_FWD_DEFINED__
#define __CZVAVLIB_FWD_DEFINED__
typedef struct c_zv_av_lib c_zv_av_lib;
#endif //__CZVAVLIB_FWD_DEFINED__


struct c_zv_av_lib 
{
	// public c_object
	//
	c_object					m_Object;

	// public i_zv_av_lib
	//
	i_zv_av_lib				    m_iZVAVLib;

	// public IZOEHALAPI
	//
	IZOEHALAPI			        m_iHal;
    zoe_bool_t                  m_bHALInited;

	// c_zv_av_lib
	//
	zoe_void_ptr_t			    m_pDO;				// device PDO
	zoe_void_ptr_t			    m_PDOLayered;		// PDO
	ZV_AVLIB_INITDATA		    m_InitData;
	ZV_DEVICE_CALLBACK		    m_pDeviceCallback;	// codec client call back function
	zoe_void_ptr_t			    m_callbackContext;	// codec client call back function context
    zoe_dbg_comp_id_t           m_dbgID;            // debug ID

	// hardware components
	//
	CPCIeCntl				    *m_pPCIeCntl;       // PCIe bus interface
	CUsbCntl				    *m_pUsbCntl;        // USB bus interface
    CHPUCntl                    *m_pHPUCntl;        // HPU bus interface
	c_zv_codec				    *m_pZVCodec;	    // VPU

    // software components
    //
    CZoeIPCService              *m_pZoeIPCService;  // IPC service
    c_zoe_module_mgr            *m_p_zoe_module_mgr;// module manager

    // firmware space
    //
    void                        *m_p_fw_space_vir;
    zoe_dev_mem_t               m_p_fw_space_phy;

	// Error code
	//
	uint32_t			        m_dwDeviceError;	// internal error code

#ifndef _NO_SCU_API
    // scu ipc handle
    //
    sc_ipc_t                    m_ipcHndl;
#endif //!_NO_SCU_API
};


// c_zv_av_lib
//

// constructor
//
c_zv_av_lib * c_zv_av_lib_constructor(c_zv_av_lib *pZVAVLib,
							          zoe_void_ptr_t pPDO,
								      zoe_void_ptr_t pPDOLayered,
								      PZV_AVLIB_INITDATA pInitData,
                                      zoe_dbg_comp_id_t dbgID
								      );

// destructor
//
void c_zv_av_lib_destructor(c_zv_av_lib *This);

// firmware download
//
zoe_errs_t c_zv_av_lib_firmware_download(c_zv_av_lib *This);

// vpu power management
//
zoe_errs_t c_zv_av_lib_open_scu(c_zv_av_lib *This);
zoe_errs_t c_zv_av_lib_close_scu(c_zv_av_lib *This);
zoe_errs_t c_zv_av_lib_do_vpu_start(c_zv_av_lib *This);
zoe_errs_t c_zv_av_lib_do_vpu_stop(c_zv_av_lib *This);

// error
//
void c_zv_av_lib_set_error(c_zv_av_lib *This, 
					       uint32_t dwError
					       );
void c_zv_av_lib_clr_error(c_zv_av_lib *This);
uint32_t c_zv_av_lib_get_error(c_zv_av_lib *This);
zoe_bool_t c_zv_av_lib_is_error(c_zv_av_lib *This);

// callback helper
//
zoe_errs_t c_zv_av_lib_device_callback(c_zv_av_lib *This, 
								       uint32_t dwCode,
								       zoe_void_ptr_t pParam
								       );


#ifdef __cplusplus
}
#endif


#endif //__CZVAVLIB_H__





