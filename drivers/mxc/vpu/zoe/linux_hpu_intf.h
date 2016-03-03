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
// linux_hpu_intf.h
//
// Description: 
//
//  Linux Kernel Mode hpu bus interface internal data structures
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __LINUX_HPU_INTF_H__
#define __LINUX_HPU_INTF_H__

#include "chpuintf.h"
#include "zv_hpu_def.h"
#include "zoe_linux_user_pages.h"



typedef struct _ZV_DMABUF 
{
	struct list_head        ListEntry;

	// for userland buffer
    PZOE_USER_PAGES         p_user_mappings;
	unsigned int            offset;
	struct page			    **pages;
    uint64_t                pg_index;

	// for kernel buffer
	void				    *vmalloc;

	// for dma contig buffer
    dma_addr_t              dma_addr;

	// common
	struct scatterlist	    *sglist;
	volatile int		    sglen;
	unsigned int		    nr_pages;
	int					    direction;

    volatile uint32_t       curElement;
    volatile uint32_t       curOffset;
    uint32_t                numElementXfer;

    // dma buffer mode
    unsigned long           bufMode;

	// user passed in pointer and size
	unsigned long		    size;
	void				    *data;

    // device address
    zoe_dev_mem_t           deviceAddress;

    // swap
	uint32_t			    DataSwap;

    // DMA direction
    uint32_t                DmaDirection;

    // requester complete event
    zoe_sosal_obj_id_t      evtComplete;

} ZV_DMABUF, *PZV_DMABUF;


typedef struct _DMA_DEVICE
{
	struct
	{
		PPED_DMA_DESCRIPTOR	VirtAddress;
		dma_addr_t	        PhysAddress;
    } DmaDescriptor;

    uint32_t			    NumDescriptors;

    uint32_t			    Timeout;

    volatile PZV_DMABUF     pDmaRequest;    // current DMA request
	struct list_head        DmaRequestList; // pending DMA request list

	zoe_bool_t			    bStarted;

    // lock for dma request
	struct semaphore        m_semDmaReq;

} DMA_DEVICE, *PDMA_DEVICE;



typedef struct _HPU_DEVICE_EXTENSION 
{
    CHPUInterface           *pHpuIntf;

    struct platform_device  *plat_dev;
	spinlock_t				irq_lock;
    zoe_bool_t              dma_64bit;
        
	zoe_bool_t			    devconnected;

	int			            irq_nb;
	zoe_bool_t				InterruptEnabled;

	volatile uint32_t	    InterruptStatusDMA;
    volatile uint32_t       InterruptStatusCPU;

    // dpc for irq
	struct work_struct      irq_work_dma;
	struct work_struct      irq_work_cpu;

    DMA_DEVICE		        DmaDevice;

    void __iomem            *regs_base;
    u32                     regs_size;

    // work queue
    struct workqueue_struct *m_workqueue;

#ifdef ZV_SERIALIZE_DMA
    // lock for dma
	struct semaphore        m_semDMA;
#endif //ZV_SERIALIZE_DMA

} HPU_DEVICE_EXTENSION, *PHPU_DEVICE_EXTENSION;


#define vpu_writel(pdx, r, v)  writel((v), pdx->regs_base + (r))
#define vpu_readl(pdx, r)      readl(pdx->regs_base + (r))

#endif //__LINUX_HPU_INTF_H__

