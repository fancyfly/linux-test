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
// zoe_hal.c
//
// Description: 
//
//  ZOE Hardware Abstraction Layer
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "zoe_hal.h"
#include "zoe_sosal.h"
#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#include "stdlib.h"
#else //ZOE_LINUXKER_BUILD
#include <linux/types.h>
#include <linux/string.h>
#endif //!ZOE_LINUXKER_BUILD
#include "zvdrv_interface.h"
#include "zoe_xreg.h"

#ifdef ZOE_TARGET_CHIP_VAR
#if (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_) && (ZOE_TARGET_CHIP_VAR != ZOE_TARGET_CHIP_VAR_DV)
#define SCB_BLK_CTRL            (SCB_XREG_SLV_BASE + MED_SCB_SCB_BLK_CTRL)
#define INT_CTRL_XREG_SLV_BASE  (SCB_XREG_SLV_BASE + MED_SCB_INT_CTRL)
#define MDMA_XREG_SLV_BASE      (SCB_XREG_SLV_BASE + MED_SCB_MDMA)
#define MISC_XREG_SLV_BASE      (SCB_XREG_SLV_BASE + MED_SCB_MISC_CTRL)
#endif // 16MB
#endif //ZOE_TARGET_CHIP_VAR

//#define DEBUG_SW_INT

/////////////////////////////////////////////////////////////////////////////
//
//

static IZOEHALAPI   *s_p_hal = ZOE_NULL;

// singleton to get the HAL pointer
IZOEHALAPI * zoehal_get_hal_ptr(void)
{
    return (s_p_hal);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL_BUS ZOEHAL_BUS_INTERNAL access functions
//

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
#define CACHED_XREG_INTERNAL(x)     ((zoe_uintptr_t)(x) & 0x7FFFFFFF)
#define UNCACHED_XREG_INTERNAL(x)   ((zoe_uintptr_t)(x) | 0x80000000)
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
#define CACHED_XREG_INTERNAL(x)     (0x90000000 + ((x) & 0x0FFFFFFF))
#define UNCACHED_XREG_INTERNAL(x)   (0xB0000000 + ((x) & 0x0FFFFFFF))
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
#if (ZOE_TARGET_CHIP_VAR == ZOE_TARGET_CHIP_VAR_QM) || (ZOE_TARGET_CHIP_VAR == ZOE_TARGET_CHIP_VAR_Z)
#define CACHED_XREG_INTERNAL(x)     (0x202C000000 + ((x) & 0x00FFFFFF))
#define UNCACHED_XREG_INTERNAL(x)   (0x202C000000 + ((x) & 0x00FFFFFF))
#else // 64MB
#define CACHED_XREG_INTERNAL(x)     (0x202C000000 + ((x) & 0x03FFFFFF))
#define UNCACHED_XREG_INTERNAL(x)   (0x202C000000 + ((x) & 0x03FFFFFF))
#endif // 16MB
#else //ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP





static zoe_errs_t Internal_RegisterRead(IZOEHALAPI *This,
							            uint32_t dwAddr,
							            uint32_t * pData
							            )
{
	*pData = *((volatile uint32_t *)((zoe_uintptr_t)UNCACHED_XREG_INTERNAL(dwAddr)));
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_RegisterWrite(IZOEHALAPI *This,
								         uint32_t dwAddr,
								         uint32_t dwData 
								         )
{
	*((volatile uint32_t *)((zoe_uintptr_t)UNCACHED_XREG_INTERNAL(dwAddr))) = dwData;
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_RegisterReadEx(IZOEHALAPI * This,
								          uint32_t dwAddr,
								          uint32_t * pData,
								          uint32_t numReg
								          )
{
    uint32_t    i;
    for (i = 0; i < numReg; i++, dwAddr += 4)
    {
	    *(pData + i) = *((volatile uint32_t *)((zoe_uintptr_t)UNCACHED_XREG_INTERNAL(dwAddr)));
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_RegisterWriteEx(IZOEHALAPI * This,
								           uint32_t dwAddr,
								           uint32_t * pData,
								           uint32_t numReg
								           )
{
    uint32_t    i;
    for (i = 0; i < numReg; i++, dwAddr += 4)
    {
	    *((volatile uint32_t *)((zoe_uintptr_t)UNCACHED_XREG_INTERNAL(dwAddr))) = *(pData + i);
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_MemoryRead(IZOEHALAPI * This,
							          zoe_dev_mem_t dwAddr,
							          uint32_t * pData,
                                      zoe_bool_t bCached
							          )
{
    void    *virt_addr = zoe_sosal_memory_get_vir((void *)((zoe_uintptr_t)dwAddr));

    if (ZOE_BAD_VIR_PTR == virt_addr)
    {
        return (ZOE_ERRS_INVALID);
    }

    if (bCached)
    {
	    *pData = *((volatile uint32_t *)(ZOE_SOSAL_MEMORY_CACHED(virt_addr)));
    }
    else
    {
	    *pData = *((volatile uint32_t *)virt_addr);
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_MemoryWrite(IZOEHALAPI * This,
							           zoe_dev_mem_t dwAddr,
							           uint32_t dwData,
                                       zoe_bool_t bCached
							           )
{
    void    *virt_addr = zoe_sosal_memory_get_vir((void *)((zoe_uintptr_t)dwAddr));

    if (ZOE_BAD_VIR_PTR == virt_addr)
    {
        return (ZOE_ERRS_INVALID);
    }

    if (bCached)
    {
	    *((volatile uint32_t *)(ZOE_SOSAL_MEMORY_CACHED(virt_addr))) = dwData;
    }
    else
    {
	    *((volatile uint32_t *)virt_addr) = dwData;
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_MemoryReadEx(IZOEHALAPI * This,
							            zoe_dev_mem_t dwAddr,
							            uint8_t * pData, 
							            uint32_t ulLength,  // in bytes
                                        zoe_bool_t bCached
							            )
{
    uint8_t *pSrc;
    void    *virt_addr = zoe_sosal_memory_get_vir((void *)((zoe_uintptr_t)dwAddr));

    if (ZOE_BAD_VIR_PTR == virt_addr)
    {
        return (ZOE_ERRS_INVALID);
    }

    pSrc = bCached ? 
           (uint8_t *)(ZOE_SOSAL_MEMORY_CACHED(virt_addr)) :
           (uint8_t *)virt_addr;
    memcpy(pData, pSrc, ulLength);
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_MemoryWriteEx(IZOEHALAPI * This,
							             zoe_dev_mem_t dwAddr,
							             uint8_t * pData,
							             uint32_t ulLength, // in bytes
                                         zoe_bool_t bCached
							             )
{
    uint8_t *pDest; 
    void    *virt_addr = zoe_sosal_memory_get_vir((void *)((zoe_uintptr_t)dwAddr));

    if (ZOE_BAD_VIR_PTR == virt_addr)
    {
        return (ZOE_ERRS_INVALID);
    }

    pDest = bCached ? 
            (uint8_t *)(ZOE_SOSAL_MEMORY_CACHED(virt_addr)) :
            (uint8_t *)virt_addr;
    memcpy(pDest, pData, ulLength);
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_DMARead(IZOEHALAPI * This,
						           zoe_dev_mem_t ulAddr,
						           uint8_t * pHostAddr,
						           uint32_t ulLength,   // in bytes
                                   uint32_t ulXferMode,
                                   zoe_bool_t bSwap,
                                   zoe_sosal_obj_id_t evt,
                                   zoe_void_ptr_t p_private
						           )
{
	Internal_MemoryReadEx(This, ulAddr, (uint8_t *)(ZOE_SOSAL_MEMORY_NONCACHED((void *)pHostAddr)), ulLength, ZOE_FALSE);
    if (evt)
    {
        zoe_sosal_event_set(evt);
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_DMAWrite(IZOEHALAPI * This,
					                zoe_dev_mem_t ulAddr,
						            uint8_t * pHostAddr,
						            uint32_t ulLength,  // in bytes
                                    uint32_t ulXferMode,
                                    zoe_bool_t bSwap,
                                    zoe_sosal_obj_id_t evt,
                                    zoe_void_ptr_t p_private
						            )
{
	Internal_MemoryWriteEx(This, ulAddr, (uint8_t *)(ZOE_SOSAL_MEMORY_NONCACHED((void *)pHostAddr)), ulLength, ZOE_FALSE);
    if (evt)
    {
        zoe_sosal_event_set(evt);
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t Internal_isr_sw_handler_install(IZOEHALAPI *This,
                                                  zoe_sosal_isr_sw_numbers_t from_num,
                                                  zoe_sosal_isr_sw_proc_t proc, 
                                                  void * ctxt
                                                  )
{
    return (zoe_sosal_isr_sw_handler_install(from_num,
                                             proc, 
                                             ctxt
                                             ));
}



static zoe_errs_t Internal_isr_sw_enable_state_set(IZOEHALAPI *This,
                                                   zoe_sosal_isr_sw_numbers_t from_num,
                                                   zoe_bool_t * enable_ptr
                                                   )
{
    return (zoe_sosal_isr_sw_enable_state_set(from_num,
                                              enable_ptr
                                              ));
}



static zoe_errs_t Internal_isr_sw_trigger(IZOEHALAPI *This,
                                          zoe_sosal_isr_sw_numbers_t to_num,
                                          zoe_sosal_isr_sw_numbers_t from_num
                                          )
{
    return (zoe_sosal_isr_sw_trigger(to_num));
}



static zoe_errs_t Internal_isr_sw_clear(IZOEHALAPI *This,
                                        zoe_sosal_isr_sw_numbers_t from_num
                                        )
{
    return (zoe_sosal_isr_sw_clear(from_num));
}



static uint32_t Internal_GetMaxDMASize(IZOEHALAPI * This)
{
    return (64 * 1048);
}



static zoe_bool_t Internal_CanSwapData(IZOEHALAPI * This)
{
    return (ZOE_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL_BUS EMULATION access functions
//


#ifdef ZOEHAL_EMU_HANDLE_ISR

// c_thread
//
static void Emu_InitEvents(c_thread *This_p, 
                           zoe_sosal_event_wait_t *pWait
                           )
{
    PZOEHAL_EMU_ISR_THREAD_CNXT pCnxt = (PZOEHAL_EMU_ISR_THREAD_CNXT)This_p->m_context;
    IZOEHALAPI                  *This = (IZOEHALAPI *)pCnxt->p_zoehal;

    pWait[0].event_id = This->m_EmuISREvtEnable[pCnxt->from_num];
    pWait[0].fired = ZOE_FALSE;
}



static void Emu_HandleEvents(c_thread *This_p, 
                             zoe_sosal_event_wait_t *pWait
                             )
{
    PZOEHAL_EMU_ISR_THREAD_CNXT pCnxt = (PZOEHAL_EMU_ISR_THREAD_CNXT)This_p->m_context;
    IZOEHALAPI                  *This = (IZOEHALAPI *)pCnxt->p_zoehal;
    zoe_errs_t                  err;

    while (ZOE_TRUE)
    {
        err = ZOEHAL_WAIT_ISR(This,
                              pCnxt->from_num,
                              10000
                              );
        if (ZOE_ERRS_SUCCESS == err)
        {
            if (pCnxt->isr_proc)
            {
                pCnxt->isr_proc(pCnxt->isr_cnxt, 
                                pCnxt->from_num
                                );
            }

        }
        else if (ZOE_ERRS_TIMEOUT == err)
        {
            if (!pCnxt->b_isr_enabled)
            {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
				               "Emu_HandleEvents() ZOEHAL_WAIT_ISR TIMEOUT and ISR Disabled\n"
				               );
                break;
            }
        }
        else
        {
            zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                           This->m_dbgID,
				           "Emu_HandleEvents() ZOEHAL_WAIT_ISR Failed(%d)!!\n",
				           err
				           );
            break;
        }
    }
}

#endif //ZOEHAL_EMU_HANDLE_ISR



static zoe_errs_t Emu_RegisterRead(IZOEHALAPI *This,
							       uint32_t dwAddr,
							       uint32_t * pData
							       )
{
	return (zvdrv_read_reg(This->m_hDevice, dwAddr, pData));
}



static zoe_errs_t Emu_RegisterWrite(IZOEHALAPI *This,
								    uint32_t dwAddr,
								    uint32_t dwData 
								    )
{
	return (zvdrv_write_reg(This->m_hDevice, dwAddr, dwData));
}




static zoe_errs_t Emu_RegisterReadEx(IZOEHALAPI * This,
								     uint32_t dwAddr,
								     uint32_t * pData,
								     uint32_t numReg
								     )
{
	return (zvdrv_read_reg_block(This->m_hDevice, dwAddr, pData, numReg));
}



static zoe_errs_t Emu_RegisterWriteEx(IZOEHALAPI * This,
								      uint32_t dwAddr,
								      uint32_t * pData,
								      uint32_t numReg
								      )
{
	return (zvdrv_write_reg_block(This->m_hDevice, dwAddr, pData, numReg));
}



static zoe_errs_t Emu_MemoryRead(IZOEHALAPI * This,
							     zoe_dev_mem_t dwAddr,
							     uint32_t * pData,
                                 zoe_bool_t bCached
							     )
{
	return (zvdrv_read_mem(This->m_hDevice, dwAddr, (uint8_t *)pData, sizeof(uint32_t)));
}



static zoe_errs_t Emu_MemoryWrite(IZOEHALAPI * This,
							      zoe_dev_mem_t dwAddr,
							      uint32_t dwData,
                                  zoe_bool_t bCached
							      )
{
	return (zvdrv_write_mem(This->m_hDevice, dwAddr, (uint8_t *)&dwData, sizeof(uint32_t)));
}



static zoe_errs_t Emu_MemoryReadEx(IZOEHALAPI * This,
							       zoe_dev_mem_t dwAddr,
							       uint8_t * pData, 
							       uint32_t ulLength,  // in bytes
                                   zoe_bool_t bCached
							       )
{
	return (zvdrv_read_mem(This->m_hDevice, dwAddr, pData, ulLength));
}



static zoe_errs_t Emu_MemoryWriteEx(IZOEHALAPI * This,
							        zoe_dev_mem_t dwAddr,
							        uint8_t * pData,
							        uint32_t ulLength, // in bytes
                                    zoe_bool_t bCached
							        )
{
	return (zvdrv_write_mem(This->m_hDevice, dwAddr, pData, ulLength));
}



static zoe_errs_t Emu_DMARead(IZOEHALAPI * This,
						      zoe_dev_mem_t ulAddr,
						      uint8_t * pHostAddr,
						      uint32_t ulLength,    // in bytes
                              uint32_t ulXferMode,
                              zoe_bool_t bSwap,
                              zoe_sosal_obj_id_t evt,
                              zoe_void_ptr_t p_private
						      )
{
	return (zvdrv_read_mem_direct(This->m_hDevice, ulAddr, pHostAddr, ulLength, ulXferMode, bSwap, evt));
}



static zoe_errs_t Emu_DMAWrite(IZOEHALAPI * This,
					           zoe_dev_mem_t ulAddr,
						       uint8_t * pHostAddr,
						       uint32_t ulLength,   // in bytes
                               uint32_t ulXferMode,
                               zoe_bool_t bSwap,
                               zoe_sosal_obj_id_t evt,
                               zoe_void_ptr_t p_private
						       )
{
	return (zvdrv_write_mem_direct(This->m_hDevice, ulAddr, pHostAddr, ulLength, ulXferMode, bSwap, evt));
}



static zoe_errs_t Emu_WaitIsr(IZOEHALAPI * This,
                              zoe_sosal_isr_sw_numbers_t from_num,
                              uint32_t timeout_ms
                              )
{
    return (zvdrv_wait_isr(This->m_hDevice, from_num, timeout_ms));
}



static zoe_errs_t Emu_EnableWaitIsr(IZOEHALAPI * This, 
                                    zoe_sosal_isr_sw_numbers_t from_num
                                    )
{
    return (zvdrv_enable_wait_isr(This->m_hDevice, from_num));
}



static zoe_errs_t Emu_DisableWaitIsr(IZOEHALAPI * This,
                                     zoe_sosal_isr_sw_numbers_t from_num
                                     )
{
    return (zvdrv_disable_wait_isr(This->m_hDevice, from_num));
}



static zoe_errs_t Emu_isr_sw_handler_install(IZOEHALAPI *This,
                                             zoe_sosal_isr_sw_numbers_t from_num,
                                             zoe_sosal_isr_sw_proc_t proc, 
                                             void * ctxt
                                             )
{
#ifdef ZOEHAL_EMU_HANDLE_ISR
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;

    if (proc)
    {

        This->m_EmuISRThreadCnxt[from_num].isr_proc = proc;
        This->m_EmuISRThreadCnxt[from_num].isr_cnxt = ctxt;
        This->m_EmuISRThreadCnxt[from_num].b_isr_enabled = ZOE_FALSE;

        // create the emulation IRQ thread
        //
		if (!c_thread_thread_init(&This->m_EmuISRThread[from_num]))
		{
            This->m_EmuISRThreadCnxt[from_num].isr_proc = ZOE_NULL;
            This->m_EmuISRThreadCnxt[from_num].isr_cnxt = ZOE_NULL;

	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
				           "Emu_isr_sw_handler_install c_thread_thread_init() FAILED\n"
				           );
            err = ZOE_ERRS_FAIL;
		}
    }
    else
    {
        // disbale wait isr so thread can be unblocked
        //
        if (This->m_EmuISRThreadCnxt[from_num].b_isr_enabled)
        {
            ZOEHAL_DISABLE_WAIT_ISR(This, 
                                    from_num
                                    );
        }

	    // destroy the emulation irq thread
	    //
        c_thread_thread_done(&This->m_EmuISRThread[from_num]);

        // nil the ISR proc
        //
        This->m_EmuISRThreadCnxt[from_num].isr_proc = ZOE_NULL;
        This->m_EmuISRThreadCnxt[from_num].isr_cnxt = ZOE_NULL;
    }

    return (err);
#else //!ZOEHAL_EMU_HANDLE_ISR
    return (zoe_sosal_isr_sw_handler_install(from_num,
                                             proc, 
                                             ctxt
                                             ));
#endif //ZOEHAL_EMU_HANDLE_ISR
}



static zoe_errs_t Emu_isr_sw_enable_state_set(IZOEHALAPI *This,
                                              zoe_sosal_isr_sw_numbers_t from_num,
                                              zoe_bool_t * enable_ptr
                                              )
{
#ifdef ZOEHAL_EMU_HANDLE_ISR
    zoe_errs_t  err = ZOE_ERRS_SUCCESS;
    zoe_bool_t  set = *enable_ptr;

    if (set != This->m_EmuISRThreadCnxt[from_num].b_isr_enabled)
    {
        *enable_ptr = This->m_EmuISRThreadCnxt[from_num].b_isr_enabled;
        This->m_EmuISRThreadCnxt[from_num].b_isr_enabled = set;

        if (ZOE_TRUE == set)
        {
            err = ZOEHAL_ENABLE_WAIT_ISR(This, 
                                         from_num
                                         );
            This->m_EmuISRThreadCnxt[from_num].b_isr_enabled = ZOE_SUCCESS(err);
            if (ZOE_SUCCESS(err) &&
                This->m_EmuISREvtEnable[from_num]
                )
            {
                zoe_sosal_event_set(This->m_EmuISREvtEnable[from_num]);
            }
        }
        else
        {
            err = ZOEHAL_DISABLE_WAIT_ISR(This, 
                                          from_num
                                          );
        }
    }

    return (err);
#else //!ZOEHAL_EMU_HANDLE_ISR
    return (zoe_sosal_isr_sw_enable_state_set(from_num,
                                              enable_ptr
                                              ));
#endif //ZOEHAL_EMU_HANDLE_ISR
}



static zoe_errs_t Emu_isr_sw_trigger(IZOEHALAPI *This,
                                     zoe_sosal_isr_sw_numbers_t to_num,
                                     zoe_sosal_isr_sw_numbers_t from_num
                                     )
{
    return (zvdrv_isr_sw_trigger(This->m_hDevice, to_num, from_num));
}



static zoe_errs_t Emu_isr_sw_clear(IZOEHALAPI *This,
                                   zoe_sosal_isr_sw_numbers_t from_num
                                   )
{
#ifdef ZOEHAL_EMU_HANDLE_ISR
    return (ZOE_ERRS_SUCCESS);
#else //!ZOEHAL_EMU_HANDLE_ISR
    return (zoe_sosal_isr_sw_clear(from_num));
#endif //ZOEHAL_EMU_HANDLE_ISR
}



static zoe_errs_t Emu_wait_proxy_event(IZOEHALAPI * This,
                                       uint32_t *p_event,
                                       uint32_t timeout_ms
                                       )
{
    return (zvdrv_wait_proxy_event(This->m_hDevice, p_event, timeout_ms));
}



static zoe_errs_t Emu_set_proxy(IZOEHALAPI * This)
{
    return (zvdrv_set_proxy(This->m_hDevice));
}



static zoe_errs_t Emu_term_proxy(IZOEHALAPI * This)
{
    return (zvdrv_term_proxy(This->m_hDevice));
}



static void * Emu_mmap(IZOEHALAPI *This,
                       int64_t offset, 
                       int64_t size
                       )
{
    return (zvdrv_mmap(This->m_hDevice, (size_t)offset, (size_t)size));
}



static zoe_errs_t Emu_unmap(IZOEHALAPI *This,
                            void *ptr, 
                            int64_t length
                            )
{
    return (zvdrv_unmap(This->m_hDevice, ptr, (size_t)length));
}


static uint32_t Emu_GetMaxDMASize(IZOEHALAPI * This)
{
    return (zvdrv_get_max_dma_size(This->m_hDevice));
}



static zoe_bool_t Emu_CanSwapData(IZOEHALAPI * This)
{
    return (ZOE_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL_BUS ZOEHAL_BUS_USB access functions
//

static zoe_errs_t USB_RegisterRead(IZOEHALAPI * This,
							       uint32_t dwAddr,
							       uint32_t * pData
							       )
{
    return (CUsbCntl_RegisterRead(This->m_pUsbCntl, dwAddr, pData));
}



static zoe_errs_t USB_RegisterWrite(IZOEHALAPI * This,
							        uint32_t dwAddr,
							        uint32_t dwData 
							        )
{
    return (CUsbCntl_RegisterWrite(This->m_pUsbCntl, dwAddr, dwData));
}



static zoe_errs_t USB_RegisterReadEx(IZOEHALAPI * This,
							         uint32_t dwAddr,
							         uint32_t * pData,
							         uint32_t numReg
							         )
{
    return (CUsbCntl_RegisterReadEx(This->m_pUsbCntl, dwAddr, pData, numReg));
}



static zoe_errs_t USB_RegisterWriteEx(IZOEHALAPI * This,
								      uint32_t dwAddr,
								      uint32_t * pData,
								      uint32_t numReg
								      )
{
    return (CUsbCntl_RegisterWriteEx(This->m_pUsbCntl, dwAddr, pData, numReg));
}



static zoe_errs_t USB_MemoryRead(IZOEHALAPI * This,
						         zoe_dev_mem_t dwAddr,
						         uint32_t * pData,
                                 zoe_bool_t bCached
						         )
{
    return (CUsbCntl_MemoryRead(This->m_pUsbCntl, dwAddr, pData));
}



static zoe_errs_t USB_MemoryWrite(IZOEHALAPI * This,
							      zoe_dev_mem_t dwAddr,
							      uint32_t dwData,
                                  zoe_bool_t bCached
							      )
{
    return (CUsbCntl_MemoryWrite(This->m_pUsbCntl, dwAddr, dwData));
}



static zoe_errs_t USB_MemoryReadEx(IZOEHALAPI * This,
							       zoe_dev_mem_t dwAddr,
							       uint8_t * pData, 
							       uint32_t ulLength,   // in bytes
                                   zoe_bool_t bCached
							       )
{
    return (CUsbCntl_MemoryReadEx(This->m_pUsbCntl, dwAddr, pData, ulLength));
}



static zoe_errs_t USB_MemoryWriteEx(IZOEHALAPI * This,
							        zoe_dev_mem_t dwAddr,
							        uint8_t * pData,
							        uint32_t ulLength,  // in bytes
                                    zoe_bool_t bCached
							        )
{
    return (CUsbCntl_MemoryWriteEx(This->m_pUsbCntl, dwAddr, pData, ulLength));
}



static zoe_errs_t USB_DMARead(IZOEHALAPI * This,
						      zoe_dev_mem_t ulAddr,
						      uint8_t * pHostAddr,
						      uint32_t ulLength,    // in bytes
                              uint32_t ulXferMode,
                              zoe_bool_t bSwap,
                              zoe_sosal_obj_id_t evt,
                              zoe_void_ptr_t p_private
						      )
{
    return (CUsbCntl_StartDMARead(This->m_pUsbCntl, ulAddr, ulLength, pHostAddr, ulXferMode, (uint32_t)bSwap, evt));
}



static zoe_errs_t USB_DMAWrite(IZOEHALAPI * This,
					           zoe_dev_mem_t ulAddr,
						       uint8_t * pHostAddr,
						       uint32_t ulLength,   // in bytes
                               uint32_t ulXferMode,
                               zoe_bool_t bSwap,
                               zoe_sosal_obj_id_t evt,
                               zoe_void_ptr_t p_private
						       )
{
    return (CUsbCntl_StartDMAWrite(This->m_pUsbCntl, ulAddr, ulLength, pHostAddr, ulXferMode, (uint32_t)bSwap, evt));
}



static zoe_errs_t USB_isr_sw_handler_install(IZOEHALAPI *This,
                                             zoe_sosal_isr_sw_numbers_t from_num,
                                             zoe_sosal_isr_sw_proc_t proc, 
                                             void * ctxt
                                             )
{
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t USB_isr_sw_enable_state_set(IZOEHALAPI *This,
                                              zoe_sosal_isr_sw_numbers_t from_num,
                                              zoe_bool_t * enable_ptr
                                              )
{
	return (ZOE_ERRS_SUCCESS);
}


#if 0

static zoe_errs_t USB_isr_sw_trigger(IZOEHALAPI *This,
                                     zoe_sosal_isr_sw_numbers_t to_num,
                                     zoe_sosal_isr_sw_numbers_t from_num
                                     )
{
    return (zoe_sosal_isr_sw_trigger(to_num));
}



static zoe_errs_t USB_isr_sw_clear(IZOEHALAPI *This,
                                   zoe_sosal_isr_sw_numbers_t from_num
                                   )
{
	return (ZOE_ERRS_SUCCESS);
}

#endif


static uint32_t USB_GetMaxDMASize(IZOEHALAPI * This)
{
    return (64 * 1024);
}



static zoe_bool_t USB_CanSwapData(IZOEHALAPI * This)
{
    return (ZOE_FALSE);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL_BUS ZOEHAL_BUS_PCIe access functions
//

static zoe_errs_t PCIe_RegisterRead(IZOEHALAPI * This,
							        uint32_t dwAddr,
							        uint32_t * pData
							        )
{
    return (CPCIeCntl_RegisterRead(This->m_pPCIeCntl, dwAddr, pData));
}



static zoe_errs_t PCIe_RegisterWrite(IZOEHALAPI * This,
							         uint32_t dwAddr,
							         uint32_t dwData 
							         )
{
    return (CPCIeCntl_RegisterWrite(This->m_pPCIeCntl, dwAddr, dwData));
}



static zoe_errs_t PCIe_RegisterReadEx(IZOEHALAPI * This,
							          uint32_t dwAddr,
							          uint32_t * pData,
							          uint32_t numReg
							          )
{
    return (CPCIeCntl_RegisterReadEx(This->m_pPCIeCntl, dwAddr, pData, numReg));
}



static zoe_errs_t PCIe_RegisterWriteEx(IZOEHALAPI * This,
								       uint32_t dwAddr,
								       uint32_t * pData,
								       uint32_t numReg
								       )
{
    return (CPCIeCntl_RegisterWriteEx(This->m_pPCIeCntl, dwAddr, pData, numReg));
}



static zoe_errs_t PCIe_MemoryRead(IZOEHALAPI * This,
						          zoe_dev_mem_t dwAddr,
						          uint32_t * pData,
                                  zoe_bool_t bCached
						          )
{
    return (CPCIeCntl_MemoryRead(This->m_pPCIeCntl, dwAddr, pData));
}



static zoe_errs_t PCIe_MemoryWrite(IZOEHALAPI * This,
							       zoe_dev_mem_t dwAddr,
							       uint32_t dwData,
                                   zoe_bool_t bCached
							       )
{
    return (CPCIeCntl_MemoryWrite(This->m_pPCIeCntl, dwAddr, dwData));
}



static zoe_errs_t PCIe_MemoryReadEx(IZOEHALAPI * This,
							        zoe_dev_mem_t dwAddr,
							        uint8_t * pData, 
							        uint32_t ulLength,   // in bytes
                                    zoe_bool_t bCached
							        )
{
    return (CPCIeCntl_MemoryReadEx(This->m_pPCIeCntl, dwAddr, pData, ulLength));
}



static zoe_errs_t PCIe_MemoryWriteEx(IZOEHALAPI * This,
							         zoe_dev_mem_t dwAddr,
							         uint8_t * pData,
							         uint32_t ulLength,  // in bytes
                                     zoe_bool_t bCached
							         )
{
    return (CPCIeCntl_MemoryWriteEx(This->m_pPCIeCntl, dwAddr, pData, ulLength));
}



static zoe_errs_t PCIe_DMARead(IZOEHALAPI * This,
						       zoe_dev_mem_t ulAddr,
						       uint8_t * pHostAddr,
						       uint32_t ulLength,    // in bytes
                               uint32_t ulXferMode,
                               zoe_bool_t bSwap,
                               zoe_sosal_obj_id_t evt,
                               zoe_void_ptr_t p_private
						       )
{
    return (CPCIeCntl_StartDMARead(This->m_pPCIeCntl, ulAddr, ulLength, pHostAddr, ulXferMode, (uint32_t)bSwap, evt, p_private));
}



static zoe_errs_t PCIe_DMAWrite(IZOEHALAPI * This,
					            zoe_dev_mem_t ulAddr,
						        uint8_t * pHostAddr,
						        uint32_t ulLength,   // in bytes
                                uint32_t ulXferMode,
                                zoe_bool_t bSwap,
                                zoe_sosal_obj_id_t evt,
                                zoe_void_ptr_t p_private
						        )
{
    return (CPCIeCntl_StartDMAWrite(This->m_pPCIeCntl, ulAddr, ulLength, pHostAddr, ulXferMode, (uint32_t)bSwap, evt, p_private));
}



static zoe_errs_t PCIe_isr_sw_handler_install(IZOEHALAPI *This,
                                              zoe_sosal_isr_sw_numbers_t from_num,
                                              zoe_sosal_isr_sw_proc_t proc, 
                                              void * ctxt
                                              )
{
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t PCIe_isr_sw_enable_state_set(IZOEHALAPI *This,
                                               zoe_sosal_isr_sw_numbers_t from_num,
                                               zoe_bool_t * enable_ptr
                                               )
{
	return (ZOE_ERRS_SUCCESS);
}



#if 0

static zoe_errs_t PCIe_isr_sw_trigger(IZOEHALAPI *This,
                                      zoe_sosal_isr_sw_numbers_t to_num,
                                      zoe_sosal_isr_sw_numbers_t from_num
                                      )
{
    return (zoe_sosal_isr_sw_trigger(to_num));
}

#endif


static zoe_errs_t PCIe_isr_sw_clear(IZOEHALAPI *This,
                                    zoe_sosal_isr_sw_numbers_t from_num
                                    )
{
	return (ZOE_ERRS_SUCCESS);
}



static uint32_t PCIe_GetMaxDMASize(IZOEHALAPI * This)
{
    return (CPCIeCntl_GetMaxDMASize(This->m_pPCIeCntl));
}



static zoe_bool_t PCIe_CanSwapData(IZOEHALAPI * This)
{
    return (ZOE_TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL_BUS ZOEHAL_BUS_HPU access functions
//

static zoe_errs_t HPU_RegisterRead(IZOEHALAPI * This,
							       uint32_t dwAddr,
							       uint32_t * pData
							       )
{
    return (CHPUCntl_RegisterRead(This->m_pHPUCntl, dwAddr, pData));
}



static zoe_errs_t HPU_RegisterWrite(IZOEHALAPI * This,
							        uint32_t dwAddr,
							        uint32_t dwData 
							        )
{
    return (CHPUCntl_RegisterWrite(This->m_pHPUCntl, dwAddr, dwData));
}



static zoe_errs_t HPU_RegisterReadEx(IZOEHALAPI * This,
							         uint32_t dwAddr,
							         uint32_t * pData,
							         uint32_t numReg
							         )
{
    return (CHPUCntl_RegisterReadEx(This->m_pHPUCntl, dwAddr, pData, numReg));
}



static zoe_errs_t HPU_RegisterWriteEx(IZOEHALAPI * This,
								      uint32_t dwAddr,
								      uint32_t * pData,
								      uint32_t numReg
								      )
{
    return (CHPUCntl_RegisterWriteEx(This->m_pHPUCntl, dwAddr, pData, numReg));
}



static zoe_errs_t HPU_MemoryRead(IZOEHALAPI * This,
						         zoe_dev_mem_t dwAddr,
						         uint32_t * pData,
                                 zoe_bool_t bCached
						         )
{
    return (CHPUCntl_MemoryRead(This->m_pHPUCntl, dwAddr, pData));
}



static zoe_errs_t HPU_MemoryWrite(IZOEHALAPI * This,
							      zoe_dev_mem_t dwAddr,
							      uint32_t dwData,
                                  zoe_bool_t bCached
							      )
{
    return (CHPUCntl_MemoryWrite(This->m_pHPUCntl, dwAddr, dwData));
}



static zoe_errs_t HPU_MemoryReadEx(IZOEHALAPI * This,
							       zoe_dev_mem_t dwAddr,
							       uint8_t * pData, 
							       uint32_t ulLength,   // in bytes
                                   zoe_bool_t bCached
							       )
{
    return (CHPUCntl_MemoryReadEx(This->m_pHPUCntl, dwAddr, pData, ulLength));
}



static zoe_errs_t HPU_MemoryWriteEx(IZOEHALAPI * This,
							        zoe_dev_mem_t dwAddr,
							        uint8_t * pData,
							        uint32_t ulLength,  // in bytes
                                    zoe_bool_t bCached
							        )
{
    return (CHPUCntl_MemoryWriteEx(This->m_pHPUCntl, dwAddr, pData, ulLength));
}



static zoe_errs_t HPU_DMARead(IZOEHALAPI * This,
						      zoe_dev_mem_t ulAddr,
						      uint8_t * pHostAddr,
						      uint32_t ulLength,    // in bytes
                              uint32_t ulXferMode,
                              zoe_bool_t bSwap,
                              zoe_sosal_obj_id_t evt,
                              zoe_void_ptr_t p_private
						      )
{
    return (CHPUCntl_StartDMARead(This->m_pHPUCntl, ulAddr, ulLength, pHostAddr, ulXferMode, (uint32_t)bSwap, evt, p_private));
}



static zoe_errs_t HPU_DMAWrite(IZOEHALAPI * This,
					           zoe_dev_mem_t ulAddr,
						       uint8_t * pHostAddr,
						       uint32_t ulLength,   // in bytes
                               uint32_t ulXferMode,
                               zoe_bool_t bSwap,
                               zoe_sosal_obj_id_t evt,
                               zoe_void_ptr_t p_private
						       )
{
    return (CHPUCntl_StartDMAWrite(This->m_pHPUCntl, ulAddr, ulLength, pHostAddr, ulXferMode, (uint32_t)bSwap, evt, p_private));
}



static zoe_errs_t HPU_isr_sw_handler_install(IZOEHALAPI *This,
                                             zoe_sosal_isr_sw_numbers_t from_num,
                                             zoe_sosal_isr_sw_proc_t proc, 
                                             void * ctxt
                                             )
{
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t HPU_isr_sw_enable_state_set(IZOEHALAPI *This,
                                              zoe_sosal_isr_sw_numbers_t from_num,
                                              zoe_bool_t * enable_ptr
                                              )
{
	return (ZOE_ERRS_SUCCESS);
}



#if 0

static zoe_errs_t HPU_isr_sw_trigger(IZOEHALAPI *This,
                                     zoe_sosal_isr_sw_numbers_t to_num,
                                     zoe_sosal_isr_sw_numbers_t from_num
                                     )
{
    return (zoe_sosal_isr_sw_trigger(to_num));
}

#endif


static zoe_errs_t HPU_isr_sw_clear(IZOEHALAPI *This,
                                   zoe_sosal_isr_sw_numbers_t from_num
                                   )
{
	return (ZOE_ERRS_SUCCESS);
}



static uint32_t HPU_GetMaxDMASize(IZOEHALAPI * This)
{
    return (CHPUCntl_GetMaxDMASize(This->m_pHPUCntl));
}



static zoe_bool_t HPU_CanSwapData(IZOEHALAPI * This)
{
    return (ZOE_TRUE);
}



/////////////////////////////////////////////////////////////////////////////
//
//

// ZOEHAL_BUS ZOEHAL_BUS_EMULATION access functions
//



/////////////////////////////////////////////////////////////////////////////
//
//

#if 0
// generic access functions
//
static zoe_errs_t Generic_RegisterReadEx(IZOEHALAPI * This,
								         uint32_t dwAddr,
								         uint32_t * pData,
								         uint32_t numReg
								         )
{
	uint32_t    i;
	zoe_errs_t	    err = ZOE_ERRS_SUCCESS;

	for (i = 0; i < numReg; i++)
	{
		err = ZOEHAL_REG_READ(This, 
							  dwAddr + (i * sizeof(uint32_t)),
							  &pData[i]
							  );
		if (!ZOE_SUCCESS(err))
		{
            break;
		}
	}

	return (err);
}



static zoe_errs_t Generic_RegisterWriteEx(IZOEHALAPI * This,
								          uint32_t dwAddr,
								          uint32_t * pData,
								          uint32_t numReg
								          )
{
	uint32_t    i;
	zoe_errs_t	    err = ZOE_ERRS_SUCCESS;

	for (i = 0; i < numReg; i++)
	{
		err = ZOEHAL_REG_WRITE(This, 
							   dwAddr + (i * sizeof(uint32_t)),
							   pData[i]
							   );
		if (!ZOE_SUCCESS(err))
		{
            break;
		}
	}

	return (err);
}

#endif


static zoe_errs_t Generic_WaitIsr(IZOEHALAPI * This,
                                  zoe_sosal_isr_sw_numbers_t from_num,
                                  uint32_t timeout_ms
                                  )
{
    return (ZOE_ERRS_NOTSUPP);
}


#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

#define REG_INT_HW_NUM_BIT(n, b)                                  (XREG_ARB_BASE_PHY + INT_CTRL_XREG_SLV_BASE + INT_CTRL_REG0_INT0_HPU_0_READ + (0x1000 * n) + (4 * b))

// Interrupt number corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIntNumber[ZOE_SOSAL_CHISEL_ISR_SW_NUM] = 
{
	3,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL
	3,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER
	0,	// ZOE_SOSAL_CHISEL_ISR_SW_SPU
	5,	// ZOE_SOSAL_CHISEL_ISR_SW_DMAPU
	5,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0
	5,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1
	5,	// ZOE_SOSAL_CHISEL_ISR_SW_EDPU
	5,	// ZOE_SOSAL_CHISEL_ISR_SW_EEPU
	7,	// ZOE_SOSAL_CHISEL_ISR_SW_MEPU
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL
	4	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER
};

// Interrupt bit corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIntBit[ZOE_SOSAL_CHISEL_ISR_SW_NUM] = 
{
	29,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL
	29,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER
	31,	// ZOE_SOSAL_CHISEL_ISR_SW_SPU
	8,	// ZOE_SOSAL_CHISEL_ISR_SW_DMAPU
	14,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0
	23,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1
	26,	// ZOE_SOSAL_CHISEL_ISR_SW_EDPU
	29,	// ZOE_SOSAL_CHISEL_ISR_SW_EEPU
	10,	// ZOE_SOSAL_CHISEL_ISR_SW_MEPU
	15,	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL
	15	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER
};


// Interrupt number corresponding to a given SW ISR number
static const uint32_t s_SWISRNumFromIntNumber[ZOE_SOSAL_CHISEL_ISR_SW_NUM] = 
{
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_SPU
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_DMAPU
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_EDPU
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_EEPU
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_MEPU
	4,	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL
	4	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER
};

// Interrupt bit corresponding to a given SW ISR number
static const uint32_t s_SWISRNumFromIntBit[ZOE_SOSAL_CHISEL_ISR_SW_NUM] = 
{
	7,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL
	7,	// ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER
	8,	// ZOE_SOSAL_CHISEL_ISR_SW_SPU
	9,	// ZOE_SOSAL_CHISEL_ISR_SW_DMAPU
	10,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_0
	11,	// ZOE_SOSAL_CHISEL_ISR_SW_AUDPU_1
	12,	// ZOE_SOSAL_CHISEL_ISR_SW_EDPU
	13,	// ZOE_SOSAL_CHISEL_ISR_SW_EEPU
	14,	// ZOE_SOSAL_CHISEL_ISR_SW_MEPU
	15,	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL
	15	// ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER
};



#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

// There is a CPU trigger register per each Interrupt Master (32)
#define REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0               (XREG_ARB_BASE_PHY + INT_CTRL_XREG_SLV_BASE + MED_INT_CTRL_INT_CTRL_LCL_REG + MED_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0)
#define REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK(x)               (XREG_ARB_BASE_PHY + INT_CTRL_XREG_SLV_BASE + MED_INT_CTRL_INT_CTRL_LCL_REG + MED_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK00 + ((x) * 64))
#define REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK_CPU2CPU_CLR(x)   (REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK(x) + 4)


// Interrupt Master number corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIntMaster[ZOE_SOSAL_DAWN_ISR_SW_NUM] =
{
	0,	// ZOE_SOSAL_ISR_SW_HPU_KERNEL
	0,	// ZOE_SOSAL_ISR_SW_HPU_USER
	2,	// ZOE_SOSAL_ISR_SW_SPU
	1,	// ZOE_SOSAL_ISR_SW_FWPU
	3,	// ZOE_SOSAL_ISR_SW_MEPU
	8,	// ZOE_SOSAL_ISR_SW_EXT_KERNEL
	8	// ZOE_SOSAL_ISR_SW_EXT_USER
};

// Interrupt Output Line numbers corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIOL[ZOE_SOSAL_DAWN_ISR_SW_NUM] =
{
	0,	// ZOE_SOSAL_ISR_SW_HPU_KERNEL
	0,	// ZOE_SOSAL_ISR_SW_HPU_USER
	16,	// ZOE_SOSAL_ISR_SW_SPU
	8,	// ZOE_SOSAL_ISR_SW_FWPU
	20,	// ZOE_SOSAL_ISR_SW_MEPU
	30,	// ZOE_SOSAL_ISR_SW_EXT_KERNEL
	30	// ZOE_SOSAL_ISR_SW_EXT_USER
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)


// Interrupt Master number corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIntMaster[ZOE_SOSAL_MX6_ISR_SW_NUM] =
{
	0,	// ZOE_SOSAL_ISR_SW_HPU_KERNEL
	0,	// ZOE_SOSAL_ISR_SW_HPU_USER
	8,	// ZOE_SOSAL_ISR_SW_EXT_KERNEL
	8	// ZOE_SOSAL_ISR_SW_EXT_USER
};

// Interrupt Output Line numbers corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIOL[ZOE_SOSAL_MX6_ISR_SW_NUM] =
{
	0,	// ZOE_SOSAL_ISR_SW_HPU_KERNEL
	0,	// ZOE_SOSAL_ISR_SW_HPU_USER
	30,	// ZOE_SOSAL_ISR_SW_EXT_KERNEL
	30	// ZOE_SOSAL_ISR_SW_EXT_USER
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

// There is a CPU trigger register per each Interrupt Master (32)
#define REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0               (XREG_ARB_BASE_PHY + INT_CTRL_XREG_SLV_BASE + MED_INT_CTRL_INT_CTRL_LCL_REG + MED_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0)
#define REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK(x)               (XREG_ARB_BASE_PHY + INT_CTRL_XREG_SLV_BASE + MED_INT_CTRL_INT_CTRL_LCL_REG + MED_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK00 + ((x) * 64))
#define REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK_CPU2CPU_CLR(x)   (REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK(x) + 4)

// Interrupt Master number corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIntMaster[ZOE_SOSAL_MX8_ISR_SW_NUM] =
{
	0,	// ZOE_SOSAL_ISR_SW_HPU_KERNEL
	0,	// ZOE_SOSAL_ISR_SW_HPU_USER
	1	// ZOE_SOSAL_ISR_SW_FWPU
};

// Interrupt Output Line numbers corresponding to a given SW ISR number
static const uint32_t s_SWISRNumToIOL[ZOE_SOSAL_MX8_ISR_SW_NUM] =
{
	0,	// ZOE_SOSAL_ISR_SW_HPU_KERNEL
	0,	// ZOE_SOSAL_ISR_SW_HPU_USER
	8	// ZOE_SOSAL_ISR_SW_FWPU
};

#else //ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP



static zoe_errs_t Generic_isr_sw_trigger(IZOEHALAPI *This,
                                         zoe_sosal_isr_sw_numbers_t to_num,
                                         zoe_sosal_isr_sw_numbers_t from_num
                                         )
{
    zoe_errs_t  err = ZOE_ERRS_PARMS;

//	return (ZOE_ERRS_SUCCESS);

    if (ZOE_SOSAL_ISR_SW_SELF == to_num) 
    {
        to_num = zoe_sosal_isr_sw_my_isr_num();
    }
    
    if (ZOE_SOSAL_ISR_SW_SELF == from_num) 
    {
        from_num = zoe_sosal_isr_sw_my_isr_num();
    }

    // check if this is a kernel 2 user interrupt
    //
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
    if (((ZOE_SOSAL_CHISEL_ISR_SW_HPU_USER == to_num) &&
        (ZOE_SOSAL_CHISEL_ISR_SW_HPU_KERNEL == from_num)) ||
        ((ZOE_SOSAL_CHISEL_ISR_SW_EXT_USER == to_num) &&
        (ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL == from_num))
        )
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
    if (((ZOE_SOSAL_DAWN_ISR_SW_HPU_USER == to_num) &&
        (ZOE_SOSAL_DAWN_ISR_SW_HPU_KERNEL == from_num)) ||
        ((ZOE_SOSAL_DAWN_ISR_SW_EXT_USER == to_num) &&
        (ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL == from_num))
        )
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
    if (((ZOE_SOSAL_MX6_ISR_SW_HPU_USER == to_num) &&
        (ZOE_SOSAL_MX6_ISR_SW_HPU_KERNEL == from_num)) ||
        ((ZOE_SOSAL_MX6_ISR_SW_EXT_USER == to_num) &&
        (ZOE_SOSAL_MX6_ISR_SW_EXT_KERNEL == from_num))
        )
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
    if (((ZOE_SOSAL_MX8_ISR_SW_HPU_USER == to_num) &&
        (ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL == from_num))
        )
#else //ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP
    {
        if (This->m_b_enable_wait_isrs[from_num])
        {
            zoe_sosal_event_set(This->m_evt_wait_isrs[from_num]);
        }
        return (ZOE_ERRS_SUCCESS);
    }

    // real cpu2cpu interrupt
    //
    if ((to_num >= ZOE_SOSAL_ISR_SW_NUM) || (to_num < 0)) 
    {
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
                       "invalid isr TO number %d\n",
                       to_num
                       );
    }
    else
    {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
	    uint32_t    intNum = s_SWISRNumToIntNumber[to_num];
	    uint32_t    intBit = s_SWISRNumToIntBit[to_num];
        uint32_t    read;

	    ZOEHAL_REG_WRITE(This, REG_INT_HW_NUM_BIT(intNum, intBit), 1);
	    ZOEHAL_REG_READ(This, REG_INT_HW_NUM_BIT(intNum, intBit), &read);
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
	    uint32_t    imNum = s_SWISRNumToIntMaster[ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL];
	    uint32_t    iolNum = s_SWISRNumToIOL[to_num];
        uint32_t    read;

	    ZOEHAL_REG_WRITE(This, REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0 + imNum * 4, iolNum);
	    ZOEHAL_REG_READ(This, REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0 + imNum * 4, &read);
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
	    uint32_t    imNum = s_SWISRNumToIntMaster[ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL];
	    uint32_t    iolNum = s_SWISRNumToIOL[to_num];
        uint32_t    read;

	    ZOEHAL_REG_WRITE(This, REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0 + imNum * 4, iolNum);
	    ZOEHAL_REG_READ(This, REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0 + imNum * 4, &read);

#ifdef DEBUG_SW_INT
        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "%s() 0x%08X <= 0x%08X\n",
                       __FUNCTION__,
                       REG_INT_CTRL_LCL_REG_CPU2CPU_INT_TRIGGER0 + imNum * 4,
                       read
		               );
#endif //DEBUG_SW_INT
#else //ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP
        err = ZOE_ERRS_SUCCESS;
    }
    return (err);
}



static zoe_errs_t Generic_isr_sw_clear(IZOEHALAPI *This,
                                       zoe_sosal_isr_sw_numbers_t from_num
                                       )
{
    zoe_errs_t  err = ZOE_ERRS_PARMS;

    if (ZOE_SOSAL_ISR_SW_SELF == from_num) 
    {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
        from_num = ZOE_SOSAL_CHISEL_ISR_SW_EXT_KERNEL;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
        from_num = ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
        from_num = ZOE_SOSAL_MX6_ISR_SW_EXT_KERNEL;
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
        from_num = ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL;
#else //ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP
    }

    if ((from_num >= ZOE_SOSAL_ISR_SW_NUM) || (from_num < 0)) 
    {
        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
                       "invalid isr FROM number %d\n",
                       from_num
                       );
    }
    else
    {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
	    uint32_t    intNum = s_SWISRNumFromIntNumber[from_num];
	    uint32_t    intBit = s_SWISRNumFromIntBit[from_num];
        uint32_t    read;

	    ZOEHAL_REG_WRITE(This, REG_INT_HW_NUM_BIT(intNum, intBit), 0);
	    ZOEHAL_REG_READ(This, REG_INT_HW_NUM_BIT(intNum, intBit), &read);
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
        uint32_t    imBit = 1 << s_SWISRNumToIntMaster[from_num];
        uint32_t    iolNum = s_SWISRNumToIOL[ZOE_SOSAL_DAWN_ISR_SW_EXT_KERNEL];
        uint32_t    read;

        ZOEHAL_REG_WRITE(This, REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK_CPU2CPU_CLR(iolNum), imBit);
        ZOEHAL_REG_READ(This, REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK_CPU2CPU_CLR(iolNum), &read);
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
        uint32_t    imBit = 1 << s_SWISRNumToIntMaster[from_num];
        uint32_t    iolNum = s_SWISRNumToIOL[ZOE_SOSAL_MX8_ISR_SW_HPU_KERNEL];
        uint32_t    read;

        ZOEHAL_REG_WRITE(This, REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK_CPU2CPU_CLR(iolNum), imBit);
        ZOEHAL_REG_READ(This, REG_INT_CTRL_LCL_REG_INT_CTRL_IOL_BANK_CPU2CPU_CLR(iolNum), &read);
#else //ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP
        err = ZOE_ERRS_SUCCESS;
    }
	return (err);
}



static zoe_errs_t Generic_EnableWaitIsr(IZOEHALAPI * This,
                                        zoe_sosal_isr_sw_numbers_t from_num
                                        )
{
    return (ZOE_ERRS_NOTSUPP);
}



static zoe_errs_t Generic_DisableWaitIsr(IZOEHALAPI * This,
                                         zoe_sosal_isr_sw_numbers_t from_num
                                         )
{
    return (ZOE_ERRS_NOTSUPP);
}



static zoe_errs_t Generic_wait_proxy_event(IZOEHALAPI * This,
                                           uint32_t *p_event,
                                           uint32_t timeout_ms
                                           )
{
    return (ZOE_ERRS_NOTSUPP);
}



static zoe_errs_t Generic_set_proxy(IZOEHALAPI * This)
{
    return (ZOE_ERRS_NOTSUPP);
}



static zoe_errs_t Generic_term_proxy(IZOEHALAPI * This)
{
    return (ZOE_ERRS_NOTSUPP);
}



static void * Generic_mmap(IZOEHALAPI *This,
                           int64_t offset, 
                           int64_t size
                           )
{
    return (ZOE_NULL);
}



static zoe_errs_t Generic_unmap(IZOEHALAPI *This,
                                void *ptr, 
                                int64_t length
                                )
{
    return (ZOE_ERRS_NOTSUPP);
}


/////////////////////////////////////////////////////////////////////////////
//
//

#ifdef ZOEHAL_EMU_HANDLE_ISR

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
static const char * s_ZOEHAL_ThreadName[ZOE_SOSAL_ISR_SW_NUM] =
{
    "ZOEHAL_EmuIRQ_Thread_HPU_Kernel",
    "ZOEHAL_EmuIRQ_Thread_HPU_User",
    "ZOEHAL_EmuIRQ_Thread_SPU",
    "ZOEHAL_EmuIRQ_Thread_DMAPU",
    "ZOEHAL_EmuIRQ_Thread_AUD0",
    "ZOEHAL_EmuIRQ_Thread_AUD1",
    "ZOEHAL_EmuIRQ_Thread_EDPU",
    "ZOEHAL_EmuIRQ_Thread_EEPU",
    "ZOEHAL_EmuIRQ_Thread_MEPU",
    "ZOEHAL_EmuIRQ_Thread_EXT_Kernel",
    "ZOEHAL_EmuIRQ_Thread_EXT_User"
};
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
static const char * s_ZOEHAL_ThreadName[ZOE_SOSAL_ISR_SW_NUM] =
{
    "ZOEHAL_EmuIRQ_Thread_HPU_Kernel",
    "ZOEHAL_EmuIRQ_Thread_HPU_User",
    "ZOEHAL_EmuIRQ_Thread_SPU",
    "ZOEHAL_EmuIRQ_Thread_FWPU",
    "ZOEHAL_EmuIRQ_Thread_MEPU",
    "ZOEHAL_EmuIRQ_Thread_EXT_Kernel",
    "ZOEHAL_EmuIRQ_Thread_EXT_User"
};
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX6)
static const char * s_ZOEHAL_ThreadName[ZOE_SOSAL_ISR_SW_NUM] =
{
    "ZOEHAL_EmuIRQ_Thread_HPU_Kernel",
    "ZOEHAL_EmuIRQ_Thread_HPU_User",
    "ZOEHAL_EmuIRQ_Thread_EXT_Kernel",
    "ZOEHAL_EmuIRQ_Thread_EXT_User"
};
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
static const char * s_ZOEHAL_ThreadName[ZOE_SOSAL_ISR_SW_NUM] =
{
    "ZOEHAL_EmuIRQ_Thread_HPU_Kernel",
    "ZOEHAL_EmuIRQ_Thread_HPU_User",
    "ZOEHAL_EmuIRQ_Thread_FWPU"
};
#endif //ZOE_TARGET_CHIP

#endif //ZOEHAL_EMU_HANDLE_ISR




// ZOE Hardware Abstraction Layer API
//
zoe_errs_t zoehal_init(IZOEHALAPI *This,
					   ZOEHAL_BUS busType,
                       uint32_t instance,
					   CUsbCntl *pUsbCntl,
					   CPCIeCntl *pPCIeCntl,
                       CHPUCntl *pHPUCntl,
                       zoe_dbg_comp_id_t dbgID
					   )
{
    zoe_errs_t  err;
    zoe_bool_t  b_kernel_proxy = ZOE_FALSE;
    uint32_t    i;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   dbgID,
				   "zoehal_init() bus(%d) inst(%d)\n",
				   busType,
                   instance
				   );

    if (s_p_hal)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       dbgID,
				       "zoehal_init() Already Exist(%p)!!!\n",
                       s_p_hal
				       );
        return (ZOE_ERRS_INVALID);
    }

	if (!This ||
        (busType < ZOEHAL_BUS_FIRST) ||
		(busType > ZOEHAL_BUS_LAST)
		)
	{
		return (ZOE_ERRS_PARMS);
	}

    // zero init ourselves
    //
	memset((void *)This,
		   0,
		   sizeof(*This)
		   );

    This->m_bus_type = busType;
    This->m_instance = instance;
    This->m_dbgID = dbgID;
    This->m_pUsbCntl = pUsbCntl;
    This->m_pPCIeCntl = pPCIeCntl;
    This->m_pHPUCntl = pHPUCntl;
    This->m_hDevice = ZOE_NULL_HANDLE;

	// setup function table
	//
    switch (This->m_bus_type)
    {
        case ZOEHAL_BUS_EMULATION_USB:
        case ZOEHAL_BUS_EMULATION_PCIe:
        case ZOEHAL_BUS_EMULATION_HPU:
            // open device
            //
            err = zvdrv_open_device(instance, 
                                    busType, 
                                    &This->m_hDevice
                                    );
            if (ZOE_FAIL(err))
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                               dbgID,
				               "zoehal_init() bus(%d) inst(%d)\n",
				               busType,
                               instance
				               );
                return (err);
            }

		    This->RegisterRead = Emu_RegisterRead;
		    This->RegisterWrite = Emu_RegisterWrite;
		    This->RegisterReadEx = Emu_RegisterReadEx;
		    This->RegisterWriteEx = Emu_RegisterWriteEx;
		    This->MemoryRead = Emu_MemoryRead;
		    This->MemoryWrite = Emu_MemoryWrite;
		    This->MemoryReadEx = Emu_MemoryReadEx;
		    This->MemoryWriteEx = Emu_MemoryWriteEx;
		    This->DMARead = Emu_DMARead;
		    This->DMAWrite = Emu_DMAWrite;
		    This->WaitIsr = Emu_WaitIsr;
		    This->EnableWaitIsr = Emu_EnableWaitIsr;
		    This->DisableWaitIsr = Emu_DisableWaitIsr;
            This->isr_sw_handler_install = Emu_isr_sw_handler_install;
            This->isr_sw_enable_state_set = Emu_isr_sw_enable_state_set;
            This->isr_sw_trigger = Emu_isr_sw_trigger;
            This->isr_sw_clear = Emu_isr_sw_clear;
            This->wait_proxy_event = Emu_wait_proxy_event;
            This->set_proxy = Emu_set_proxy;
            This->term_proxy = Emu_term_proxy;
            This->mmap = Emu_mmap;
            This->unmap = Emu_unmap;
            This->GetMaxDMASize = Emu_GetMaxDMASize;
            This->CanSwapData = Emu_CanSwapData;

            // emu ISR
            //
#ifdef ZOEHAL_EMU_HANDLE_ISR
            {
                c_thread    *pThread;
                uint32_t    maxpriorities;
                uint32_t    i;

                maxpriorities = zoe_sosal_thread_maxpriorities_get();

                for (i = 0; i < ZOE_SOSAL_ISR_SW_NUM; i++)
                {
                    This->m_EmuISRThreadCnxt[i].b_isr_enabled = ZOE_FALSE;
                    This->m_EmuISRThreadCnxt[i].isr_proc = ZOE_NULL;
                    This->m_EmuISRThreadCnxt[i].from_num = i;
                    This->m_EmuISRThreadCnxt[i].isr_cnxt = ZOE_NULL;
                    This->m_EmuISRThreadCnxt[i].p_zoehal = This;

                    // create additional event for our thread
                    //
                    err = zoe_sosal_event_create(ZOE_NULL,
                                                 &This->m_EmuISREvtEnable[i]
                                                 );
                    if (ZOE_FAIL(err))
                    {
                        zoehal_done(This);
                        return (ZOE_ERRS_FAIL);
                    }

                    // c_thread
                    //
                    pThread = c_thread_constructor(&This->m_EmuISRThread[i], 
                                                   (char *)s_ZOEHAL_ThreadName[i],
                                                   (zoe_void_ptr_t)&This->m_EmuISRThreadCnxt[i],
                                                   maxpriorities - 1,
                                                   2048,
                                                   1,
                                                   -1,       // no timeout
                                                   ZOE_TRUE, // remove user space mapping
                                                   dbgID
                                                   );
		            if (!pThread)
		            {
                        zoehal_done(This);
                        return (ZOE_ERRS_FAIL);
		            }

                    This->m_EmuISRThread[i].m_init_events = Emu_InitEvents;
                    This->m_EmuISRThread[i].m_handle_events = Emu_HandleEvents;
                }
            }

#endif //ZOEHAL_EMU_HANDLE_ISR
            break;

        case ZOEHAL_BUS_INTERNAL:
		    This->RegisterRead = Internal_RegisterRead;
		    This->RegisterWrite = Internal_RegisterWrite;
		    This->RegisterReadEx = Internal_RegisterReadEx;
		    This->RegisterWriteEx = Internal_RegisterWriteEx;
		    This->MemoryRead = Internal_MemoryRead;
		    This->MemoryWrite = Internal_MemoryWrite;
		    This->MemoryReadEx = Internal_MemoryReadEx;
		    This->MemoryWriteEx = Internal_MemoryWriteEx;
		    This->DMARead = Internal_DMARead;
		    This->DMAWrite = Internal_DMAWrite;
		    This->WaitIsr = Generic_WaitIsr;
		    This->EnableWaitIsr = Generic_EnableWaitIsr;
		    This->DisableWaitIsr = Generic_DisableWaitIsr;
            This->isr_sw_handler_install = Internal_isr_sw_handler_install;
            This->isr_sw_enable_state_set = Internal_isr_sw_enable_state_set;
            This->isr_sw_trigger = Internal_isr_sw_trigger;
            This->isr_sw_clear = Internal_isr_sw_clear;
            This->wait_proxy_event = Generic_wait_proxy_event;
            This->set_proxy = Generic_set_proxy;
            This->term_proxy = Generic_term_proxy;
            This->mmap = Generic_mmap;
            This->unmap = Generic_unmap;
            This->GetMaxDMASize = Internal_GetMaxDMASize;
            This->CanSwapData = Internal_CanSwapData;
            break;

        case ZOEHAL_BUS_USB:
		    This->RegisterRead = USB_RegisterRead;
		    This->RegisterWrite = USB_RegisterWrite;
		    This->RegisterReadEx = USB_RegisterReadEx;
		    This->RegisterWriteEx = USB_RegisterWriteEx;
		    This->MemoryRead = USB_MemoryRead;
		    This->MemoryWrite = USB_MemoryWrite;
		    This->MemoryReadEx = USB_MemoryReadEx;
		    This->MemoryWriteEx = USB_MemoryWriteEx;
		    This->DMARead = USB_DMARead;
		    This->DMAWrite = USB_DMAWrite;
		    This->WaitIsr = Generic_WaitIsr;
		    This->EnableWaitIsr = Generic_EnableWaitIsr;
		    This->DisableWaitIsr = Generic_DisableWaitIsr;
            This->isr_sw_handler_install = USB_isr_sw_handler_install;
            This->isr_sw_enable_state_set = USB_isr_sw_enable_state_set;
            This->isr_sw_trigger = Generic_isr_sw_trigger;
            This->isr_sw_clear = Generic_isr_sw_clear;
            This->wait_proxy_event = Generic_wait_proxy_event;
            This->set_proxy = Generic_set_proxy;
            This->term_proxy = Generic_term_proxy;
            This->mmap = Generic_mmap;
            This->unmap = Generic_unmap;
            This->GetMaxDMASize = USB_GetMaxDMASize;
            This->CanSwapData = USB_CanSwapData;
            b_kernel_proxy = ZOE_TRUE;
            break;

        case ZOEHAL_BUS_PCIe:
		    This->RegisterRead = PCIe_RegisterRead;
		    This->RegisterWrite = PCIe_RegisterWrite;
		    This->RegisterReadEx = PCIe_RegisterReadEx;
		    This->RegisterWriteEx = PCIe_RegisterWriteEx;
		    This->MemoryRead = PCIe_MemoryRead;
		    This->MemoryWrite = PCIe_MemoryWrite;
		    This->MemoryReadEx = PCIe_MemoryReadEx;
		    This->MemoryWriteEx = PCIe_MemoryWriteEx;
		    This->DMARead = PCIe_DMARead;
		    This->DMAWrite = PCIe_DMAWrite;
		    This->WaitIsr = Generic_WaitIsr;
		    This->EnableWaitIsr = Generic_EnableWaitIsr;
		    This->DisableWaitIsr = Generic_DisableWaitIsr;
            This->isr_sw_handler_install = PCIe_isr_sw_handler_install;
            This->isr_sw_enable_state_set = PCIe_isr_sw_enable_state_set;
            This->isr_sw_trigger = Generic_isr_sw_trigger;
            This->isr_sw_clear = PCIe_isr_sw_clear;
            This->wait_proxy_event = Generic_wait_proxy_event;
            This->set_proxy = Generic_set_proxy;
            This->term_proxy = Generic_term_proxy;
            This->mmap = Generic_mmap;
            This->unmap = Generic_unmap;
            This->GetMaxDMASize = PCIe_GetMaxDMASize;
            This->CanSwapData = PCIe_CanSwapData;
            b_kernel_proxy = ZOE_TRUE;
            break;

        case ZOEHAL_BUS_HPU:
		    This->RegisterRead = HPU_RegisterRead;
		    This->RegisterWrite = HPU_RegisterWrite;
		    This->RegisterReadEx = HPU_RegisterReadEx;
		    This->RegisterWriteEx = HPU_RegisterWriteEx;
		    This->MemoryRead = HPU_MemoryRead;
		    This->MemoryWrite = HPU_MemoryWrite;
		    This->MemoryReadEx = HPU_MemoryReadEx;
		    This->MemoryWriteEx = HPU_MemoryWriteEx;
		    This->DMARead = HPU_DMARead;
		    This->DMAWrite = HPU_DMAWrite;
		    This->WaitIsr = Generic_WaitIsr;
		    This->EnableWaitIsr = Generic_EnableWaitIsr;
		    This->DisableWaitIsr = Generic_DisableWaitIsr;
            This->isr_sw_handler_install = HPU_isr_sw_handler_install;
            This->isr_sw_enable_state_set = HPU_isr_sw_enable_state_set;
            This->isr_sw_trigger = Generic_isr_sw_trigger;
            This->isr_sw_clear = HPU_isr_sw_clear;
            This->wait_proxy_event = Generic_wait_proxy_event;
            This->set_proxy = Generic_set_proxy;
            This->term_proxy = Generic_term_proxy;
            This->mmap = Generic_mmap;
            This->unmap = Generic_unmap;
            This->GetMaxDMASize = HPU_GetMaxDMASize;
            This->CanSwapData = HPU_CanSwapData;
            b_kernel_proxy = ZOE_TRUE;
            break;

        default:
            return (ZOE_ERRS_NOTIMPL);
    }

    if (b_kernel_proxy)
    {
        for (i = 0; i < ZOE_SOSAL_ISR_SW_NUM; i++)
        {
            err = zoe_sosal_event_create(ZOE_NULL, 
                                         &This->m_evt_wait_isrs[i]
                                         );
            if (ZOE_FAIL(err))
            {
                zoehal_done(This);
                return (ZOE_ERRS_FAIL);
            }
        }
    }

    // remember HAL pointer
    s_p_hal = This;
	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t zoehal_done(IZOEHALAPI * This)
{
    uint32_t    i;

    if (s_p_hal != This)
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
				       "zoehal_done() s_p_hal(%p) != This(%p)!!!\n",
                       s_p_hal,
                       This
				       );
    }

    switch (This->m_bus_type)
    {
        case ZOEHAL_BUS_EMULATION_USB:
        case ZOEHAL_BUS_EMULATION_PCIe:
        case ZOEHAL_BUS_EMULATION_HPU:
#ifdef ZOEHAL_EMU_HANDLE_ISR
            {
                for (i = 0; i < ZOE_SOSAL_ISR_SW_NUM; i++)
                {
                    Emu_isr_sw_handler_install(This, 
                                               i, 
                                               ZOE_NULL, 
                                               ZOE_NULL
                                               );

                    // delete events
                    //
                    if (This->m_EmuISREvtEnable[i])
                    {
                        zoe_sosal_event_delete(This->m_EmuISREvtEnable[i]);
                        This->m_EmuISREvtEnable[i] = ZOE_NULL;
                    }

                    // c_thread
                    //
	                c_thread_destructor(&This->m_EmuISRThread[i]);
                }
            }
#endif //ZOEHAL_EMU_HANDLE_ISR
            if (ZOE_NULL_HANDLE != This->m_hDevice)
            {
                zvdrv_close_device(This->m_hDevice);
                This->m_hDevice = ZOE_NULL_HANDLE;
            }
            break;

        case ZOEHAL_BUS_INTERNAL:
            break;

        default:
            break;
    }

    for (i = 0; i < ZOE_SOSAL_ISR_SW_NUM; i++)
    {
        if (This->m_evt_wait_isrs[i])
        {
            zoe_sosal_event_delete(This->m_evt_wait_isrs[i]);
            This->m_evt_wait_isrs[i] = ZOE_NULL;
        }
    }

    // clear the HAL pointer
    s_p_hal = ZOE_NULL;
	return (ZOE_ERRS_SUCCESS);
}



