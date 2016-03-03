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
// zoe_hal.h
//
// Description: 
//
//  ZOE Hardware Abstraction Layer
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ZOE_HAL_H__
#define __ZOE_HAL_H__


#define ZOEHAL_EMU_HANDLE_ISR

#include "zoe_types.h"
#include "zoe_dbg.h"
#include "zoe_sosal.h"
#include "zoe_sosal_priv.h"
#ifdef ZOEHAL_EMU_HANDLE_ISR
#include "zoe_cthread.h"
#endif //ZOEHAL_EMU_HANDLE_ISR
#include "cusbcntl.h"
#include "cpciecntl.h"
#include "chpucntl.h"
#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
//

// bus type
//
typedef enum _ZOEHAL_BUS
{
	ZOEHAL_BUS_USB = 0,
	ZOEHAL_BUS_PCIe,
	ZOEHAL_BUS_HPU,
	ZOEHAL_BUS_EMULATION_USB,
	ZOEHAL_BUS_EMULATION_PCIe,
	ZOEHAL_BUS_EMULATION_HPU,
	ZOEHAL_BUS_INTERNAL,

	ZOEHAL_BUS_TOTAL_BUSES

} ZOEHAL_BUS, *PZOEHAL_BUS;

#define ZOEHAL_BUS_FIRST    ZOEHAL_BUS_USB
#define ZOEHAL_BUS_LAST     ZOEHAL_BUS_INTERNAL


// chip id
//
typedef enum _ZOEHAL_CHIP_ID
{
	ZOEHAL_CHIP_HAMMER = 0,
	ZOEHAL_CHIP_CHISEL,
	ZOEHAL_CHIP_CAFE,
	ZOEHAL_CHIP_iMX8,

	ZOEHAL_CHIP_TOTAL,
	ZOEHAL_CHIP_INVALID

} ZOEHAL_CHIP_ID, *PZOEHAL_CHIP_ID;

#define ZOEHAL_CHIP_FIRST   ZOEHAL_CHIP_HAMMER
#define ZOEHAL_CHIP_LAST    ZOEHAL_CHIP_MX8


// dma buffer mode
//
#define DMA_BUFFER_MODE_KERNEL      0x00000000  // vmalloc
#define DMA_BUFFER_MODE_USERPTR     0x01000000  // user space buffer
#define DMA_BUFFER_MODE_COMMON      0x02000000  // dma common buffer, require copy
#define DMA_BUFFER_MODE_CONTIG      0x04000000  // dma contigious buffer

#define DMA_BUFFER_MODE_MASK        0xFF000000

/////////////////////////////////////////////////////////////////////////////
//
//

#ifndef __IZOEHALAPI_FWD_DEFINED__
#define __IZOEHALAPI_FWD_DEFINED__
typedef struct IZOEHALAPI IZOEHALAPI;
#endif //__IZOEHALAPI_FWD_DEFINED__

/////////////////////////////////////////////////////////////////////////////
//
//


#ifdef ZOEHAL_EMU_HANDLE_ISR
typedef struct _ZOEHAL_EMU_ISR_THREAD_CNXT
{
    volatile zoe_bool_t         b_isr_enabled;
    zoe_sosal_isr_sw_proc_t     isr_proc;
    zoe_sosal_isr_sw_numbers_t  from_num;
    void                        *isr_cnxt;
    IZOEHALAPI                  *p_zoehal;
} ZOEHAL_EMU_ISR_THREAD_CNXT, *PZOEHAL_EMU_ISR_THREAD_CNXT;
#endif //ZOEHAL_EMU_HANDLE_ISR


/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL per instance data
//
struct IZOEHALAPI
{
	// access functions
	//
	zoe_errs_t (*RegisterRead)(IZOEHALAPI * This,
							   uint32_t dwAddr,
							   uint32_t * pData
							   );
	zoe_errs_t (*RegisterWrite)(IZOEHALAPI * This,
								uint32_t dwAddr,
								uint32_t dwData 
								);
	zoe_errs_t (*RegisterReadEx)(IZOEHALAPI * This,
								 uint32_t dwAddr,
								 uint32_t * pData,
								 uint32_t numReg
								 );
	zoe_errs_t (*RegisterWriteEx)(IZOEHALAPI * This,
								  uint32_t dwAddr,
								  uint32_t * pData,
								  uint32_t numReg
								  );
	zoe_errs_t (*MemoryRead)(IZOEHALAPI * This,
							 zoe_dev_mem_t dwAddr,
							 uint32_t * pData,
                             zoe_bool_t bCached
							 );
	zoe_errs_t (*MemoryWrite)(IZOEHALAPI * This,
							  zoe_dev_mem_t dwAddr,
							  uint32_t dwData,
                              zoe_bool_t bCached
							  );
	zoe_errs_t (*MemoryReadEx)(IZOEHALAPI * This,
							   zoe_dev_mem_t dwAddr,
							   uint8_t * pData, 
							   uint32_t ulLength,   // in bytes
                               zoe_bool_t bCached
							   );
	zoe_errs_t (*MemoryWriteEx)(IZOEHALAPI * This,
							    zoe_dev_mem_t dwAddr,
							    uint8_t * pData,
							    uint32_t ulLength,  // in bytes
                                zoe_bool_t bCached
							    );
	zoe_errs_t (*DMARead)(IZOEHALAPI * This,
						  zoe_dev_mem_t ulAddr,
						  uint8_t * pHostAddr,
						  uint32_t ulLength,    // in bytes
                          uint32_t ulXferMode,
                          zoe_bool_t bSwap,
                          zoe_sosal_obj_id_t evt,
                          zoe_void_ptr_t p_private
						  );
	zoe_errs_t (*DMAWrite)(IZOEHALAPI * This,
					       zoe_dev_mem_t ulAddr,
						   uint8_t * pHostAddr,
						   uint32_t ulLength,   // in bytes
                           uint32_t ulXferMode,
                           zoe_bool_t bSwap,
                           zoe_sosal_obj_id_t evt,
                           zoe_void_ptr_t p_private
						   );
    zoe_errs_t (*WaitIsr)(IZOEHALAPI * This,
                          zoe_sosal_isr_sw_numbers_t from_num,
                          uint32_t timeout_ms
                          );
    zoe_errs_t (*EnableWaitIsr)(IZOEHALAPI * This, 
                                zoe_sosal_isr_sw_numbers_t from_num
                                );
    zoe_errs_t (*DisableWaitIsr)(IZOEHALAPI * This, 
                                 zoe_sosal_isr_sw_numbers_t from_num
                                 );
    zoe_errs_t (*isr_sw_handler_install)(IZOEHALAPI *This,
                                         zoe_sosal_isr_sw_numbers_t from_num,
                                         zoe_sosal_isr_sw_proc_t proc, 
                                         void * ctxt
                                         );
    zoe_errs_t (*isr_sw_enable_state_set)(IZOEHALAPI *This,
                                          zoe_sosal_isr_sw_numbers_t from_num,
                                          zoe_bool_t * enable_ptr
                                          );
    zoe_errs_t (*isr_sw_trigger)(IZOEHALAPI *This,
                                 zoe_sosal_isr_sw_numbers_t to_num,
                                 zoe_sosal_isr_sw_numbers_t from_num
                                 );
    zoe_errs_t (*isr_sw_clear)(IZOEHALAPI *This,
                               zoe_sosal_isr_sw_numbers_t from_num
                               );
    zoe_errs_t (*wait_proxy_event)(IZOEHALAPI * This,
                                   uint32_t *p_event,
                                   uint32_t timeout_ms
                                   );
    zoe_errs_t (*set_proxy)(IZOEHALAPI * This);
    zoe_errs_t (*term_proxy)(IZOEHALAPI * This);
    void * (*mmap)(IZOEHALAPI *This,
                   int64_t offset, 
                   int64_t size
                   );
    zoe_errs_t (*unmap)(IZOEHALAPI *This,
                        void *ptr, 
                        int64_t length
                        );
    uint32_t (*GetMaxDMASize)(IZOEHALAPI * This);
    zoe_bool_t (*CanSwapData)(IZOEHALAPI * This);

	
	ZOEHAL_BUS                  m_bus_type; // bus type
    uint32_t                    m_instance; // instance
    ZOE_OBJECT_HANDLE           m_hDevice;  // device handle for emulation mode
    zoe_dbg_comp_id_t           m_dbgID;

    // USB bus type
    //
    CUsbCntl                    *m_pUsbCntl;

    // PCIe bus interface
    //
    CPCIeCntl                   *m_pPCIeCntl;

    // hpu bus
    //
    CHPUCntl                    *m_pHPUCntl;

    // emulation mode
    //
#ifdef ZOEHAL_EMU_HANDLE_ISR
    // c_thread
    //
    c_thread                    m_EmuISRThread[ZOE_SOSAL_ISR_SW_NUM];
    ZOEHAL_EMU_ISR_THREAD_CNXT  m_EmuISRThreadCnxt[ZOE_SOSAL_ISR_SW_NUM];
    zoe_sosal_obj_id_t          m_EmuISREvtEnable[ZOE_SOSAL_ISR_SW_NUM];
#endif //ZOEHAL_EMU_HANDLE_ISR

    // user mode interrupt kernel proxy
    //
    zoe_bool_t                  m_b_enable_wait_isrs[ZOE_SOSAL_ISR_SW_NUM];
    zoe_sosal_obj_id_t          m_evt_wait_isrs[ZOE_SOSAL_ISR_SW_NUM];
};


// zoehal API
//
zoe_errs_t zoehal_init(IZOEHALAPI * This,
					   ZOEHAL_BUS busType,
                       uint32_t instance,
					   CUsbCntl *pUsbCntl,
					   CPCIeCntl *pPCIeCntl,
                       CHPUCntl *pHPUCntl,
                       zoe_dbg_comp_id_t dbgID
					   );
zoe_errs_t zoehal_done(IZOEHALAPI * This);

// singleton to get the HAL pointer
IZOEHALAPI * zoehal_get_hal_ptr(void);

// macros for accessing zoehal interface
//
#define ZOEHAL_REG_READ(This, dwAddr, pData) \
    (This)->RegisterRead(This, dwAddr, pData)

#define ZOEHAL_REG_WRITE(This, dwAddr, dwData) \
    (This)->RegisterWrite(This, dwAddr, dwData)

#define ZOEHAL_REG_READ_EX(This, dwAddr, pData, numReg) \
    (This)->RegisterReadEx(This, dwAddr, pData, numReg)

#define ZOEHAL_REG_WRITE_EX(This, dwAddr, pData, numReg) \
    (This)->RegisterWriteEx(This, dwAddr, pData, numReg)

#define ZOEHAL_MEM_READ(This, dwAddr, pData, bCached) \
    (This)->MemoryRead(This, dwAddr, pData, bCached)

#define ZOEHAL_MEM_WRITE(This, dwAddr, dwData, bCached) \
    (This)->MemoryWrite(This, dwAddr, dwData, bCached)

#define ZOEHAL_MEM_READ_EX(This, dwAddr, pData, ulLength, bCached) \
    (This)->MemoryReadEx(This, dwAddr, pData, ulLength, bCached)

#define ZOEHAL_MEM_WRITE_EX(This, dwAddr, dwData, ulLength, bCached) \
    (This)->MemoryWriteEx(This, dwAddr, dwData, ulLength, bCached)

#define ZOEHAL_DMA_READ(This, ulAddr, pHostAddr, ulLength, ulXferMode, bSwap, evt, p_private) \
    (This)->DMARead(This, ulAddr, pHostAddr, ulLength, ulXferMode, bSwap, evt, p_private)

#define ZOEHAL_DMA_WRITE(This, ulAddr, pHostAddr, ulLength, ulXferMode, bSwap, evt, p_private) \
    (This)->DMAWrite(This, ulAddr, pHostAddr, ulLength, ulXferMode, bSwap, evt, p_private)

#define ZOEHAL_WAIT_ISR(This, from_num, timeout_ms) \
    (This)->WaitIsr(This, from_num, timeout_ms)

#define ZOEHAL_ENABLE_WAIT_ISR(This, from_num) \
    (This)->EnableWaitIsr(This, from_num)

#define ZOEHAL_DISABLE_WAIT_ISR(This, from_num) \
    (This)->DisableWaitIsr(This, from_num)

#define ZOEHAL_ISR_SW_HANDLER_INSTALL(This, from_num, proc, ctxt) \
    (This)->isr_sw_handler_install(This, from_num, proc, ctxt)

#define ZOEHAL_ISR_SW_ENABLE_STATE_SET(This, from_num, enable_ptr) \
    (This)->isr_sw_enable_state_set(This, from_num, enable_ptr)

#define ZOEHAL_ISR_SW_TRIGGER(This, to_num, from_num) \
    (This)->isr_sw_trigger(This, to_num, from_num)

#define ZOEHAL_ISR_SW_CLEAR(This, from_num) \
    (This)->isr_sw_clear(This, from_num)

#define ZOEHAL_WAIT_PROXY_EVENT(This, p_event, timeout_ms) \
    (This)->wait_proxy_event(This, p_event, timeout_ms)

#define ZOEHAL_SET_PROXY(This) \
    (This)->set_proxy(This)

#define ZOEHAL_TERM_PROXY(This) \
    (This)->term_proxy(This)

#define ZOEHAL_MMAP(This, offset, size) \
    (This)->mmap(This, offset, size)

#define ZOEHAL_UNMAP(This, ptr, length) \
    (This)->unmap(This, ptr, length)

#define ZOEHAL_GetMaxDMASize(This) \
    (This)->GetMaxDMASize(This)

#define ZOEHAL_CanSwapData(This) \
    (This)->CanSwapData(This)

#ifdef __cplusplus
}
#endif

#endif //__ZOE_HAL_H__



