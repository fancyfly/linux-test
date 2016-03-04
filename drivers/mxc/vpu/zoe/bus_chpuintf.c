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
// chpuintf.c
//
// Description: 
//
//  linux hpu bus interface
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <asm/io.h>
#include <linux/pm_runtime.h>

#include "chpuintf.h"
#include "linux_hpu_intf.h"
#include "zoe_hal.h"
#include "zoe_objids.h"


//#define DEBUG_ISR_DMA
//#define DEBUG_ISR_CPU
//#define DEBUG_DMA
//#define DEBUG_PIO
#define PIO_USE_KMAP
//#define ZVHPU_NO_64BIT_ADDR
#define _SYNC_ON_TRANSFER
#define _IO_LOCK_ON_TRANSFER

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

static uint32_t cpu_int_bitmask[MAX_CPU] = 
{
    HPU_INT_MASK,
    FPU_INT_MASK,
    SPU_INT_MASK,
    MEPU_INT_MASK,
    AUD0_INT_MASK,
    AUD1_INT_MASK,
    AUD2_INT_MASK,
    PCIE_INT_MASK
};


static uint32_t s_busintf_cb_cmds[MAX_CPU] = 
{
    ZV_BUSINTF_INT_CPU_FROM_HPU,
    ZV_BUSINTF_INT_CPU_FROM_VID,
    ZV_BUSINTF_INT_CPU_FROM_SPU,
    ZV_BUSINTF_INT_CPU_FROM_MEPU,
    ZV_BUSINTF_INT_CPU_FROM_AUD0,
    ZV_BUSINTF_INT_CPU_FROM_AUD1,
    ZV_BUSINTF_INT_CPU_FROM_AUD2,
    ZV_BUSINTF_INT_CPU_FROM_EXT
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

static uint32_t cpu_int_bitmask[MAX_CPU] = 
{
    HPU_INT_MASK,
    FPU_INT_MASK
};


static uint32_t s_busintf_cb_cmds[MAX_CPU] = 
{
    ZV_BUSINTF_INT_CPU_FROM_HPU,
    ZV_BUSINTF_INT_CPU_FROM_VID
};

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

static uint32_t cpu_int_out_offset[MAX_CPU] = 
{
    REG4_INT7_OFFSET,
    REG4_INT8_OFFSET,
    REG4_INT9_OFFSET,
    REG4_INT10_OFFSET,
    REG4_INT11_OFFSET,
    REG4_INT12_OFFSET,
    REG4_INT13_OFFSET,
    REG4_INT14_OFFSET,
    REG4_INT15_OFFSET
};


static uint32_t s_busintf_cb_cmds[MAX_CPU] = 
{
    ZV_BUSINTF_INT_CPU_FROM_HPU,
    ZV_BUSINTF_INT_CPU_FROM_SPU,
    ZV_BUSINTF_INT_CPU_FROM_DMAPU,
    ZV_BUSINTF_INT_CPU_FROM_AUD0,
    ZV_BUSINTF_INT_CPU_FROM_AUD1,
    ZV_BUSINTF_INT_CPU_FROM_EDPU,
    ZV_BUSINTF_INT_CPU_FROM_EEPU,
    ZV_BUSINTF_INT_CPU_FROM_MEPU,
    ZV_BUSINTF_INT_CPU_FROM_EXT
};

#else // ZOE_TARGET_CHIP == ??
#error Unsupported CHIP
#endif // ZOE_TARGET_CHIP



/////////////////////////////////////////////////////////////////////////////
//
//


static zoe_errs_t CHPUInterface_HwDmaInit(CHPUInterface *This)
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    // enable scb clock
    vpu_writel(pdx, VPU_CLK_RST_XREG_SLV_BASE + CLK_RST_CLK_ENA_SET_OFFSET, CLKRST_CLK_ENABLE_INT_CTL);
    // bring scb out of reset
    vpu_writel(pdx, VPU_CLK_RST_XREG_SLV_BASE + CLK_RST_SOFT_RST_EXT_OFFSET, CLKRST_SOFT_RST_EXT_INT_CTL_RST);

    // enable misc clock
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE +  SCB_CTRL_MISC_CLK_ENA_SET_OFFSET, SCB_CTRL_MISC_CLK);
    // bring misc out of reset
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_MISC_RST_SET_OFFSET, SCB_CTRL_MISC_RST);

    // enable mdma clock
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_MDMA_CLK_ENA_SET_OFFSET, SCB_CTRL_MDMA_CLK);
    // bring mdma out of reset
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_MDMA_RST_SET_OFFSET, SCB_CTRL_MDMA_RST);

    // enable gdma clock
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_GDMA_CLK_ENA_SET_OFFSET, SCB_CTRL_GDMA_CLK);
    // bring gdma out of reset
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_GDMA_RST_SET_OFFSET, SCB_CTRL_GDMA_RST);

    return (ZOE_ERRS_SUCCESS);
}



/////////////////////////////////////////////////////////////////////////////
//
//

static PZV_DMABUF zvdmabuf_alloc(PZOE_USER_PAGES p_user_mappings)
{
    PZV_DMABUF  pDmaBuf = kmalloc(sizeof(ZV_DMABUF), GFP_KERNEL);

    if (pDmaBuf)
    {
	    memset(pDmaBuf,
		       0,
		       sizeof(ZV_DMABUF)
		       );
        pDmaBuf->p_user_mappings = p_user_mappings;
        INIT_LIST_HEAD(&pDmaBuf->ListEntry);
    }

    return (pDmaBuf);
}



static void zvdmabuf_free(PZV_DMABUF pDmaBuf)
{
    if (pDmaBuf)
    {
	    if (!pDmaBuf->p_user_mappings &&
            pDmaBuf->pages
            ) 
	    {
		    int i;
		    for (i = 0; i < pDmaBuf->nr_pages; i++)
		    {
			    page_cache_release(pDmaBuf->pages[i]);
		    }
		    kfree(pDmaBuf->pages);
		    pDmaBuf->pages = ZOE_NULL;
	    }

        kfree(pDmaBuf);
    }
}



static zoe_errs_t zvdmabuf_init_user(PZV_DMABUF pdma, 
                                     zoe_dbg_comp_id_t dbgID
						             )
{
	uint64_t	first, last;

	first = ((uint64_t)pdma->data & PAGE_MASK) >> PAGE_SHIFT;
	last  = (((uint64_t)pdma->data + pdma->size - 1) & PAGE_MASK) >> PAGE_SHIFT;

	pdma->offset = (unsigned int)((uint64_t)pdma->data & ~PAGE_MASK);
	pdma->nr_pages = (unsigned int)(last - first + 1);
    pdma->pg_index = first - pdma->p_user_mappings->first;

    if (pdma->p_user_mappings)
    {
        if ((first < pdma->p_user_mappings->first) ||
            (last > pdma->p_user_mappings->last)
            )
        {
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
						   "%s() page (%p:%p:%d)out of range(%p:%p:%d) \n", 
						   __FUNCTION__,
                           (void *)first,
                           (void *)last,
                           pdma->size,
                           (void *)pdma->p_user_mappings->first,
						   (void *)pdma->p_user_mappings->last,
                           pdma->p_user_mappings->size
						   );
		    return (ZOE_ERRS_INVALID);
        }
        if (pdma->p_user_mappings->pages)
        {
            pdma->pages = &pdma->p_user_mappings->pages[pdma->pg_index];
        }
    }
    else
    {
	    long    ret = 0;

	    pdma->pages = kmalloc(pdma->nr_pages * sizeof(struct page*),
						      GFP_KERNEL
						      );
	    if (ZOE_NULL == pdma->pages)
	    {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
					       "%s() Unable to allocate page array\n", 
					       __FUNCTION__
					       );
		    return (ZOE_ERRS_NOMEMORY);
	    }

	    zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                       dbgID,
				       "%s() init user [%p + 0x%lx => %d pages]\n", 
				       __FUNCTION__,
				       pdma->data,
				       pdma->size,
				       pdma->nr_pages
				       );

	    down_read(&current->mm->mmap_sem);
	    ret = get_user_pages(current,
						     current->mm,
						     (uintptr_t)((uint64_t)pdma->data & PAGE_MASK), 
						     pdma->nr_pages,
						     pdma->direction == DMA_FROM_DEVICE, 
						     1, /* force */
						     pdma->pages, 
						     NULL
						     );
	    up_read(&current->mm->mmap_sem);
	    if (ret != pdma->nr_pages) 
	    {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
					       "%s() get_user_pages: err=%d nr_pages[%d] data[%p] size(%d)\n", 
					       __FUNCTION__,
					       ret,
					       pdma->nr_pages,
					       pdma->data,
					       pdma->size
					       );
		    pdma->nr_pages = (ret >= 0) ? ret : 0;
		    return (ZOE_ERRS_INVALID);
	    }
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zvdmabuf_init_kernel(PZV_DMABUF pdma, 
                                       zoe_dbg_comp_id_t dbgID
						               )
{
	uint64_t	first,last;

	first = ((uint64_t)pdma->data & PAGE_MASK) >> PAGE_SHIFT;
	last  = (((uint64_t)pdma->data + pdma->size - 1) & PAGE_MASK) >> PAGE_SHIFT;
	pdma->nr_pages = (unsigned int)(last - first + 1);

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   dbgID,
				   "%s() init kernel [%p + 0x%lx => %d pages]\n", 
				   __FUNCTION__,
				   pdma->data,
				   pdma->size,
				   pdma->nr_pages
				   );
	pdma->vmalloc = pdma->data;
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zvdmabuf_init_contig(PZV_DMABUF pdma, 
                                       zoe_dbg_comp_id_t dbgID
						               )
{
	pdma->dma_addr = (dma_addr_t)pdma->data;
    pdma->nr_pages = 1;
	return (ZOE_ERRS_SUCCESS);
}



static struct scatterlist* zvdmabuf_vmalloc_to_sg(unsigned char *virt,
										          unsigned long size,
										          int nr_pages,
                                                  zoe_dbg_comp_id_t dbgID
										          )
{
	struct scatterlist	*sglist;
	struct page			*pg;
	int					i;

	sglist = kcalloc(nr_pages, 
					 sizeof(struct scatterlist), 
					 GFP_KERNEL
					 );
	if (ZOE_NULL == sglist)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       dbgID,
					   "%s() Unable to allocate scatterlist\n", 
					   __FUNCTION__
					   );
		return (ZOE_NULL);
	}

	memset(sglist, 
		   0, 
		   nr_pages * sizeof(struct scatterlist)
		   );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	sg_init_table(sglist, 
				  nr_pages
				  );
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)

	// first page
	if (nr_pages >= 1)
	{
		pg = vmalloc_to_page((void *)((uint64_t)virt & PAGE_MASK));
		if (ZOE_NULL == pg)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
						   "%s() vmalloc_to_page faile on data(0x%x)\n", 
						   __FUNCTION__,
						   (unsigned long)virt & PAGE_MASK
						   );
			goto err;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		sg_assign_page(&sglist[0], pg);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
		sglist[0].page = pg;
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		sglist[0].offset = (unsigned long)((uint64_t)virt & ~PAGE_MASK);
		sglist[0].length = ZOE_MIN(PAGE_SIZE - sglist[0].offset, size);
		virt += sglist[0].length;
		size -= sglist[0].length;
	}

	// the rest of the pages
	for (i = 1; i < nr_pages; i++) 
	{
		pg = vmalloc_to_page(virt);
		if (NULL == pg)
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
						   "%s() vmalloc_to_page faile on data(%p)\n", 
						   __FUNCTION__,
						   virt
						   );
			goto err;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		sg_assign_page(&sglist[i], pg);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
		sglist[i].page = pg;
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		sglist[i].offset = 0;
		sglist[i].length = ZOE_MIN(PAGE_SIZE, size);
		virt += sglist[i].length;
		size -= sglist[i].length;
	}
	return (sglist);

err:
	kfree(sglist);
	return (ZOE_NULL);
}



static struct scatterlist* zvdmabuf_pages_to_sg(struct page **pages, 
										        int nr_pages, 
										        unsigned long size,
										        int offset,
                                                zoe_dbg_comp_id_t dbgID
										        )
{
	struct scatterlist	*sglist;
	int					i = 0;

	if (ZOE_NULL == pages[0])
	{
		return (ZOE_NULL);
	}
	sglist = kcalloc(nr_pages, 
					 sizeof(struct scatterlist), 
					 GFP_KERNEL
					 );
	if (ZOE_NULL == sglist)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       dbgID,
					   "%s() Unable to allocate scatterlist\n", 
					   __FUNCTION__
					   );
		return (ZOE_NULL);
	}

	memset(sglist, 
		   0, 
		   nr_pages * sizeof(struct scatterlist)
		   );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	sg_init_table(sglist, 
				  nr_pages
				  );
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	sg_assign_page(&sglist[0], pages[0]);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	sglist[0].page = pages[0];
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	sglist[0].offset = offset;
	sglist[0].length = ZOE_MIN(PAGE_SIZE - offset, size);
	size -= sglist[0].length;

	for (i = 1; i < nr_pages; i++) 
	{
		if (ZOE_NULL == pages[i])
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
						   "%s() no page(%d)\n", 
						   __FUNCTION__,
						   i
						   );
			goto nopage;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		sg_assign_page(&sglist[i], pages[i]);
#else //LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
		sglist[i].page = pages[i];
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
		sglist[i].offset = 0;
		sglist[i].length = ZOE_MIN(PAGE_SIZE, size);
		size -= sglist[i].length;
	}

	return (sglist);

nopage:
	kfree(sglist);
	return (ZOE_NULL);
}



static struct scatterlist* zvdmabuf_sgtable_to_sg(struct sg_table *sg_tbl, 
										          int nr_pages, 
										          unsigned long size,
										          int offset,
                                                  uint64_t pg_index,
                                                  zoe_dbg_comp_id_t dbgID
										          )
{
	struct scatterlist	*sglist;
	int					i = 0;

	sglist = kcalloc(nr_pages, 
					 sizeof(struct scatterlist), 
					 GFP_KERNEL
					 );
	if (ZOE_NULL == sglist)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       dbgID,
					   "%s() Unable to allocate scatterlist\n", 
					   __FUNCTION__
					   );
		return (ZOE_NULL);
	}

	memcpy(sglist, 
		   &sg_tbl->sgl[pg_index], 
		   nr_pages * sizeof(struct scatterlist)
		   );

	sglist[0].offset = offset;
	sglist[0].length = ZOE_MIN(PAGE_SIZE - offset, size);
	size -= sglist[0].length;

	for (i = 1; i < nr_pages; i++) 
	{
		sglist[i].offset = 0;
		sglist[i].length = ZOE_MIN(PAGE_SIZE, size);
		size -= sglist[i].length;
	}

	return (sglist);
}



static struct scatterlist* zvdmabuf_contig_to_sg(dma_addr_t addr,
										         unsigned long size,
                                                 zoe_dbg_comp_id_t dbgID
									             )
{
	struct scatterlist	*sglist;

	sglist = kmalloc(sizeof(struct scatterlist), 
					 GFP_KERNEL
					 );
	if (ZOE_NULL == sglist)
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       dbgID,
					   "%s() Unable to allocate scatterlist\n", 
					   __FUNCTION__
					   );
		return (ZOE_NULL);
	}

	memset(sglist, 
		   0, 
		   sizeof(struct scatterlist)
		   );
	sg_dma_address(&sglist[0]) = addr;
	sglist[0].offset = 0;
	sglist[0].length = size;
	return (sglist);
}



static zoe_errs_t zvdma_sync_cpu(struct device *dev, 
					             PZV_DMABUF pdma
					             )
{
	switch (pdma->bufMode) 
	{
		case DMA_BUFFER_MODE_KERNEL:
		case DMA_BUFFER_MODE_USERPTR:
		    dma_sync_sg_for_cpu(dev,
								pdma->sglist,
								pdma->nr_pages,
								pdma->direction
								);
			break;

		case DMA_BUFFER_MODE_CONTIG:
		    dma_sync_single_for_cpu(dev,
								    pdma->dma_addr,
								    pdma->size,
								    pdma->direction
								    );
			break;

		default:
			break;
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zvdma_sync_device(struct device *dev, 
					                PZV_DMABUF pdma
					                )
{
	switch (pdma->bufMode) 
	{
		case DMA_BUFFER_MODE_KERNEL:
		case DMA_BUFFER_MODE_USERPTR:
		    dma_sync_sg_for_device(dev,
								   pdma->sglist,
								   pdma->nr_pages,
								   pdma->direction
								   );
			break;

		case DMA_BUFFER_MODE_CONTIG:
		    dma_sync_single_for_device(dev,
								       pdma->dma_addr,
								       pdma->size,
								       pdma->direction
								       );
			break;

		default:
			break;
    }
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zvdma_map(struct device *dev, 
				            PZV_DMABUF pdma,
                            zoe_dbg_comp_id_t dbgID
				            )
{
	switch (pdma->bufMode) 
	{
		case DMA_BUFFER_MODE_KERNEL:
		    pdma->sglist = zvdmabuf_vmalloc_to_sg(pdma->vmalloc,
											      pdma->size,
											      pdma->nr_pages,
                                                  dbgID
											      );
			break;

		case DMA_BUFFER_MODE_USERPTR:
	        if (pdma->pages) 
	        {
		        pdma->sglist = zvdmabuf_pages_to_sg(pdma->pages, 
											        pdma->nr_pages,
											        pdma->size,
											        pdma->offset,
                                                    dbgID
											        );
	        }
            else if (pdma->p_user_mappings->sg)
            {
		        pdma->sglist = zvdmabuf_sgtable_to_sg(pdma->p_user_mappings->sg, 
											          pdma->nr_pages,
											          pdma->size,
											          pdma->offset,
                                                      pdma->pg_index,
                                                      dbgID
											          );
            }
			break;

		case DMA_BUFFER_MODE_CONTIG:
		    pdma->sglist = zvdmabuf_contig_to_sg(pdma->dma_addr,
											     pdma->size,
                                                 dbgID
											     );
			break;

		default:
			break;
    }

	if (ZOE_NULL == pdma->sglist) 
	{
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       dbgID,
					   "%s() scatterlist is NULL\n", 
					   __FUNCTION__
					   );
		return (ZOE_ERRS_NOMEMORY);
	}

	if ((DMA_BUFFER_MODE_KERNEL == pdma->bufMode) ||
        (DMA_BUFFER_MODE_USERPTR == pdma->bufMode)
        )
	{
		pdma->sglen = dma_map_sg(dev,
								 pdma->sglist,
								 pdma->nr_pages,
								 pdma->direction
								 );
		if (pdma->sglen < pdma->nr_pages) 
		{
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           dbgID,
						   "%s() dma_map_sg failed mapped(%d) requested(%d)\n", 
						   __FUNCTION__,
						   pdma->sglen,
						   pdma->nr_pages
						   );
			kfree(pdma->sglist);
			pdma->sglist = ZOE_NULL;
			pdma->sglen = 0;
			return (ZOE_ERRS_FAIL);
		}
	}
    else
    {
        pdma->sglen = 1;
    }

#ifndef _SYNC_ON_TRANSFER
    zvdma_sync_device(dev, 
                      pdma
                      );
#endif //!_SYNC_ON_TRANSFER

	return (ZOE_ERRS_SUCCESS);
}




static zoe_errs_t zvdma_unmap(struct device *dev, 
					          PZV_DMABUF pdma
					          )
{
	if ((DMA_BUFFER_MODE_KERNEL == pdma->bufMode) ||
        (DMA_BUFFER_MODE_USERPTR == pdma->bufMode)
        )
	{
		if (pdma->sglen)
		{
			dma_unmap_sg(dev,
						 pdma->sglist,
						 pdma->nr_pages,
						 pdma->direction
						 );
		}
	}

	pdma->sglen = 0;

	if (pdma->sglist)
	{
		kfree(pdma->sglist);
		pdma->sglist = ZOE_NULL;
	}
	return (ZOE_ERRS_SUCCESS);
}



static zoe_errs_t zvdma_iolock(CHPUInterface *This,
				               PZV_DMABUF pdma
				               )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
	zoe_errs_t              err;

	switch (pdma->bufMode) 
	{
		case DMA_BUFFER_MODE_KERNEL:
			err = zvdmabuf_init_kernel(pdma, 
                                       This->m_dbgID
									   );
			break;

		case DMA_BUFFER_MODE_CONTIG:
			err = zvdmabuf_init_contig(pdma, 
                                       This->m_dbgID
									   );
			break;

		case DMA_BUFFER_MODE_USERPTR:
			err = zvdmabuf_init_user(pdma, 
                                     This->m_dbgID
									 );
			break;

		default:
			err = ZOE_ERRS_PARMS;
			break;
	}

	if (ZOE_SUCCESS(err))
	{
		err = zvdma_map(&pdx->plat_dev->dev, 
						pdma,
                        This->m_dbgID
						);
	}

	return (err);
}




static zoe_errs_t zvdmabuf_start_transfer(CHPUInterface *This,
							              PZV_DMABUF pDmaBuf,
                                          zoe_bool_t bCritical
							              )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
    PDMA_DEVICE             pDmaDevice = &pdx->DmaDevice;
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN) || (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)
	PPED_DMA_DESCRIPTOR     pDmaDescr, pDmaDescrSaved;
	PPED_DMA32_DESCRIPTOR   pDma32Descr, pDma32DescrSaved;
    uint32_t		        DescAddress;
#endif //ZOE_TARGET_CHIP_DAWN || ZOE_TARGET_CHIP_MX8
	int					    i;
    uint32_t                FirstBuffer, LastBuffer, XferBuffer;
    uint32_t		        DeviceAddress;
    uint64_t                HostAddress;
    uint32_t                Length;
    zoe_errs_t              err = ZOE_ERRS_SUCCESS;
    uint32_t                DmaControl = 0;
   	uint32_t                max_num_descr = ZV_DMA_MAX_NUM_DESCR;

    if (bCritical)
    {
        down(&pDmaDevice->m_semDmaReq);
    }

    if (pDmaDevice->pDmaRequest)
    {
        if (pDmaBuf)
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                           This->m_dbgID, 
					       "zvdmabuf_start_transfer() DMA is still busy. request:0x%08X\n", 
					       pDmaDevice->pDmaRequest
					       );
            list_add_tail(&pDmaBuf->ListEntry,
                          &pDmaDevice->DmaRequestList
				          );
            err = ZOE_ERRS_PENDING;
            goto e_DmaStartTransfer;
        }
    }
    else
    {
        if (list_empty(&pDmaDevice->DmaRequestList))
        {
            pDmaDevice->pDmaRequest = pDmaBuf;
        }
        else
        {
            pDmaDevice->pDmaRequest = list_entry(pDmaDevice->DmaRequestList.next, 
                                                 ZV_DMABUF, 
                                                 ListEntry
                                                 );
            list_del(pDmaDevice->DmaRequestList.next);
            if (pDmaBuf)
            {
                list_add_tail(&pDmaBuf->ListEntry,
                              &pDmaDevice->DmaRequestList
				              );
            }
        }
    }

    if (pDmaDevice->pDmaRequest)
    {
#ifdef _IO_LOCK_ON_TRANSFER
        if (0 == pDmaDevice->pDmaRequest->nr_pages)
        {
		    // lock host buffer
		    err = zvdma_iolock(This, 
						       pDmaDevice->pDmaRequest
						       );
		    if (!ZOE_SUCCESS(err))
		    {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID, 
						       "%s() zvdma_iolock failed = %d\n",
						       __FUNCTION__,
						       err
						       );
                if (!pDmaBuf)
                {
                    zvdmabuf_free(pDmaDevice->pDmaRequest);
                }

                pDmaDevice->pDmaRequest = ZOE_NULL;
                goto e_DmaStartTransfer;
		    }
        }
#endif //_IO_LOCK_ON_TRANSFER
#ifdef _SYNC_ON_TRANSFER
        if (0 == pDmaDevice->pDmaRequest->curElement)
        {
            zvdma_sync_device(&pdx->plat_dev->dev, 
                              pDmaDevice->pDmaRequest
                              );
        }
#endif //_SYNC_ON_TRANSFER

        FirstBuffer = pDmaDevice->pDmaRequest->curElement;

#if defined(MDMA_USE_1_DESC)
        // get the number of contigious pages
        for (i = FirstBuffer; i < (pDmaDevice->pDmaRequest->sglen - 1); i++)
        {
            if ((sg_dma_address(&pDmaDevice->pDmaRequest->sglist[i]) + pDmaDevice->pDmaRequest->sglist[i].length) == 
                 sg_dma_address(&pDmaDevice->pDmaRequest->sglist[i + 1])
                 )
            {
                max_num_descr++;
            }
            else
            {
                break;
            }
        }
#endif //MDMA_USE_1_DESC

        XferBuffer = ZOE_MIN((pDmaDevice->pDmaRequest->sglen - FirstBuffer), max_num_descr);
        LastBuffer = FirstBuffer + XferBuffer - 1;
        pDmaDevice->pDmaRequest->numElementXfer = XferBuffer;

        DeviceAddress = (uint32_t)pDmaDevice->pDmaRequest->deviceAddress + pDmaDevice->pDmaRequest->curOffset;

   	    // make sure interrupt is enabled
        //
        CHPUInterface_EnableInterrupts(This);

        // descriptor memory
        pDmaDescrSaved = pDmaDescr = pDmaDevice->DmaDescriptor.VirtAddress;
        pDma32DescrSaved = pDma32Descr = (PPED_DMA32_DESCRIPTOR)pDmaDevice->DmaDescriptor.VirtAddress;
        DescAddress = (uint32_t)pDmaDevice->DmaDescriptor.PhysAddress;

        // build hardware DMA link list
        //
#ifdef MDMA_USE_1_DESC
        for (Length = 0, i = FirstBuffer; i <= LastBuffer; i++)
        {
            Length += pDmaDevice->pDmaRequest->sglist[i].length;
        }
        //Length = pDmaDevice->pDmaRequest->sglist[FirstBuffer].length;
        HostAddress = sg_dma_address(&pDmaDevice->pDmaRequest->sglist[FirstBuffer]);

        DmaControl = DMA_CH_CTRL_DIRECTION(ZV_DMA_DIR_DDR_2_DDR) |
                         DMA_CH_CTRL_INC_ADDR(1) |
                         DMA_CH_CTRL_BURST_SIZE(ZV_DMA_BURST_8_BYTES) |
                         DMA_CH_CTRL_XREG_RD_MODE(0) |
                         DMA_CH_CTRL_STREAM_MODE(0) |
                         DMA_CH_CTRL_ENDIAN_SWAP(pDmaDevice->pDmaRequest->DataSwap ? ZV_DMA_SWAP_32BIT : ZV_DMA_SWAP_NONE) |
                         DMA_CH_CTRL_BLOCKING_DESCR(0) |
                         DMA_CH_CTRL_EN_64BIT_ADDR(pdx->dma_64bit ? 1 : 0) |
                         DMA_CH_CTRL_SEC_ID(0) |
                         DMA_CH_CTRL_DESCR_INT_EN(1) |
                         DMA_CH_CTRL_LAST_DESCR(1)
                         ;

        if (pdx->dma_64bit)
        {
            if ((ZVDMA_DIR_READ == pDmaDevice->pDmaRequest->DmaDirection)
            {
                pDmaDescr->ddr_addr = DeviceAddress;
                pDmaDescr->io_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
                pDmaDescr->upper_io_addr = (uint32_t)((HostAddress >> 32) & 0xFFFFFFFF);
            }
            else
            {
                pDmaDescr->io_addr = DeviceAddress;
                pDmaDescr->ddr_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
                pDmaDescr->upper_io_addr = 0;
            }
            pDmaDescr->control = DmaControl;
            pDmaDescr->xfer_size = Length;

#ifdef DEBUG_DMA
		    printk("%s desc_list(0x%08X) ddr_addr(0x%08X) io_addr(0x%08X:%08X) control(0x%08X) Length(%d)\n",
			       (ZVDMA_DIR_WRITE == pDmaDevice->pDmaRequest->DmaDirection) ? "Write" : "Read",
                   DescAddress,
			       pDmaDescr->ddr_addr,
			       pDmaDescr->upper_io_addr,
			       pDmaDescr->io_addr,
                   pDmaDescr->control,
			       pDmaDescr->xfer_size
			       );
#endif //DEBUG_DMA
        }
        else
        {
            if ((ZVDMA_DIR_READ == pDmaDevice->pDmaRequest->DmaDirection)
            {
                pDma32Descr->ddr_addr = DeviceAddress;
                pDma32Descr->io_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
            }
            else
            {
                pDma32Descr->io_addr = DeviceAddress;
                pDma32Descr->ddr_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
            }
            pDma32Descr->control = DmaControl;
            pDma32Descr->xfer_size = Length;

#ifdef DEBUG_DMA
		    printk("%s desc_list(0x%08X) ddr_addr(0x%08X) io_addr(%08X) control(0x%08X) Length(%d)\n",
			       (ZVDMA_DIR_WRITE == pDmaDevice->pDmaRequest->DmaDirection) ? "Write" : "Read",
                   DescAddress,
			       pDma32Descr->ddr_addr,
			       pDma32Descr->io_addr,
                   pDma32Descr->control,
			       pDma32Descr->xfer_size
			       );
#endif //DEBUG_DMA
        }

#else //!MDMA_USE_1_DESC
        for (i = FirstBuffer; i <= LastBuffer; i++)
        {
            Length = pDmaDevice->pDmaRequest->sglist[i].length;
            HostAddress = sg_dma_address(&pDmaDevice->pDmaRequest->sglist[i]);

            DmaControl = DMA_CH_CTRL_DIRECTION(ZV_DMA_DIR_DDR_2_DDR) |
                         DMA_CH_CTRL_INC_ADDR(1) |
                         DMA_CH_CTRL_BURST_SIZE(ZV_DMA_BURST_8_BYTES) |
                         DMA_CH_CTRL_XREG_RD_MODE(0) |
                         DMA_CH_CTRL_STREAM_MODE(0) |
                         DMA_CH_CTRL_ENDIAN_SWAP(pDmaDevice->pDmaRequest->DataSwap ? ZV_DMA_SWAP_32BIT : ZV_DMA_SWAP_NONE) |
                         DMA_CH_CTRL_BLOCKING_DESCR(0) |
                         DMA_CH_CTRL_EN_64BIT_ADDR(pdx->dma_64bit ? 1 : 0) |
                         DMA_CH_CTRL_SEC_ID(0) |
                         DMA_CH_CTRL_DESCR_INT_EN((i != LastBuffer) ? 0 : 1) |
                         DMA_CH_CTRL_LAST_DESCR((i != LastBuffer) ? 0 : 1)
                         ;

            if (pdx->dma_64bit)
            {
                if (ZVDMA_DIR_READ == pDmaDevice->pDmaRequest->DmaDirection)
                {
                    pDmaDescr->ddr_addr = DeviceAddress;
                    pDmaDescr->io_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
                    pDmaDescr->upper_io_addr = (uint32_t)((HostAddress >> 32) & 0xFFFFFFFF);
                }
                else
                {
                    pDmaDescr->io_addr = DeviceAddress;
                    pDmaDescr->ddr_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
                    pDmaDescr->upper_io_addr = 0;
                }
                pDmaDescr->control = DmaControl;
                pDmaDescr->xfer_size = Length;
#ifdef DEBUG_DMA
		        printk("%s[%u] desc_list(0x%08X) ddr_addr(0x%08X) io_addr(0x%08X:%08X) control(0x%08X) Length(%d) last(%d)\n",
				       (ZVDMA_DIR_WRITE == pDmaDevice->pDmaRequest->DmaDirection) ? "Write" : "Read",
				       i, 
                       DescAddress,
				       pDmaDescr->ddr_addr,
				       pDmaDescr->upper_io_addr,
				       pDmaDescr->io_addr,
                       pDmaDescr->control,
				       pDmaDescr->xfer_size, 
                       (LastBuffer == i)
				       );
#endif //DEBUG_DMA
                pDmaDescr++;
            }
            else
            {
                if (ZVDMA_DIR_READ == pDmaDevice->pDmaRequest->DmaDirection)
                {
                    pDma32Descr->ddr_addr = DeviceAddress;
                    pDma32Descr->io_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
                }
                else
                {
                    pDma32Descr->io_addr = DeviceAddress;
                    pDma32Descr->ddr_addr = (uint32_t)(HostAddress & 0xFFFFFFFF);
                }
                pDma32Descr->control = DmaControl;
                pDma32Descr->xfer_size = Length;

#ifdef DEBUG_DMA
		        printk("%s[%u] desc_list(0x%08X) ddr_addr(0x%08X) io_addr(%08X) control(0x%08X) Length(%d) last(%d)\n",
				       (ZVDMA_DIR_WRITE == pDmaDevice->pDmaRequest->DmaDirection) ? "Write" : "Read",
				       i, 
                       DescAddress,
				       pDma32Descr->ddr_addr,
				       pDma32Descr->io_addr,
                       pDma32Descr->control,
				       pDma32Descr->xfer_size, 
                       (LastBuffer == i)
				       );
#endif //DEBUG_DMA
                pDma32Descr++;
            }
            DeviceAddress += Length;
        }
#endif //MDMA_USE_1_DESC

        {
            uint32_t    list_offset = MDMA_LIST_HPU_OFFSET;

            // setup MDMA
            //
            DmaControl = MDMA_LIST_CTRL2_GOBYINT_EN(0) |
                         MDMA_LIST_CTRL2_INT_EN(0) |
                         MDMA_LIST_CTRL2_PRI(1) |
                         MDMA_LIST_CTRL2_OUT_INT_LINE(12) |     // use MDMA int 12 for HPU
                         MDMA_LIST_CTRL2_EN_64BIT(pdx->dma_64bit ? 1 : 0)
                         ;

#ifdef DEBUG_DMA
	        printk("DMA Xfer(%d): element(%d) total(%d) dev_addr(0x%x) host_addr(0x%x:%x) xfer(%d)\n", pDmaDevice->pDmaRequest->DmaDirection, FirstBuffer, pDmaDevice->pDmaRequest->sglen, DeviceAddress, (uint32_t)(HostAddress >> 32), (uint32_t)(HostAddress & 0xFFFFFFFF), XferBuffer);
#endif //DEBUG_DMA

	        vpu_writel(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_BASE_PTR_OFFSET, DescAddress);
#ifdef DEBUG_DMA
            printk("0x%08X =< 0x%08X\n", VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_BASE_PTR_OFFSET, vpu_readl(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_BASE_PTR_OFFSET));
#endif //DEBUG_DMA
	        vpu_writel(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_CTRL2_OFFSET, DmaControl);
#ifdef DEBUG_DMA
            printk("0x%08X =< 0x%08X\n", VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_CTRL2_OFFSET, vpu_readl(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_CTRL2_OFFSET));
#endif //DEBUG_DMA
	        vpu_writel(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_WRITE_PTR_OFFSET, DescAddress + (ZV_DMA_MAX_NUM_DESCR * sizeof(PED_DMA_DESCRIPTOR)));
#ifdef DEBUG_DMA
            printk("0x%08X =< 0x%08X\n", VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_WRITE_PTR_OFFSET, vpu_readl(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_WRITE_PTR_OFFSET));
#endif //DEBUG_DMA
            smp_wmb();
	        vpu_writel(pdx, VPU_MDMA_LIST_OFFSET + list_offset + MDMA_LIST_REG_LS_CTRL1_OFFSET, MDMA_LIST_CTRL1_GO(1));

	        pDmaDevice->bStarted = ZOE_TRUE;
        }
    }

e_DmaStartTransfer:
    if (bCritical)
    {
        up(&pDmaDevice->m_semDmaReq);
    }
	return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//

static void zvhpu_irq_work_dma_handler(struct work_struct *work)
{
    PHPU_DEVICE_EXTENSION   pdx = container_of(work, HPU_DEVICE_EXTENSION, irq_work_dma);
    CHPUInterface           *This = pdx->pHpuIntf;
    PDMA_DEVICE             pDmaDevice = &pdx->DmaDevice;
	PZV_DMABUF			    pDmaBuf;
    uint32_t                size = 0;
    zoe_sosal_obj_id_t      evt = ZOE_NULL;
    zoe_bool_t              bCritical = ZOE_FALSE;
    int                     dir;
    uint32_t                i;

#ifdef DEBUG_ISR_DMA
	printk("dmar_handler\n");
#endif //DEBUG_ISR_DMA

    down(&pDmaDevice->m_semDmaReq);

	pDmaBuf = pDmaDevice->pDmaRequest;

    if (pDmaBuf)
    {
        for (i = 0; i < pDmaBuf->numElementXfer; i++)
        {
            pDmaBuf->curOffset += pDmaBuf->sglist[pDmaBuf->curElement++].length;
        } 
        if (pDmaBuf->curElement >= pDmaBuf->sglen)
        {
            // clear the current DMA
            pDmaDevice->pDmaRequest = ZOE_NULL;

	        zvdma_sync_cpu(&pdx->plat_dev->dev, 
				           pDmaBuf
				           );
            up(&pDmaDevice->m_semDmaReq);
            bCritical = ZOE_TRUE;

            // remember size and complete event
            size = pDmaBuf->size;
            evt = pDmaBuf->evtComplete;
            dir = pDmaBuf->DmaDirection;

	        zvdma_unmap(&pdx->plat_dev->dev, 
					    pDmaBuf
					    );
	        zvdmabuf_free(pDmaBuf);

#ifdef DEBUG_ISR_DMA
	        printk("#### DMA %s done\n", (ZVDMA_DIR_WRITE == dir) ? "Write" : "Read");
#endif //DEBUG_ISR_DMA

            // call back dma requester
            //
	        This->m_pBusCallbackFunc(This->m_pBusCallbackContext, 
                                     (ZVDMA_DIR_WRITE == dir) ? ZV_BUSINTF_DMA_DONE_WRITE: ZV_BUSINTF_DMA_DONE_READ,
				                     (zoe_void_ptr_t)evt,
                                     ZOE_ERRS_SUCCESS,
                                     size
				                     );
        }
    }

    // start next DMA
    //
    zvdmabuf_start_transfer(This, 
                            ZOE_NULL, 
                            bCritical
                            );
    if (!bCritical)
    {
        up(&pDmaDevice->m_semDmaReq);
    }
}



static void zvhpu_irq_work_cpu_handler(struct work_struct *work)
{
    PHPU_DEVICE_EXTENSION   pdx = container_of(work, HPU_DEVICE_EXTENSION, irq_work_cpu);
    CHPUInterface           *This = pdx->pHpuIntf;
    int                     cpu;

#ifdef DEBUG_ISR_CPU
    printk("cpu_handler(0x%08X)\n", pdx->InterruptStatusCPU);
#endif //DEBUG_ISR_CPU

    while (pdx->InterruptStatusCPU)
    {
        for (cpu = 0; cpu < MAX_CPU; cpu++)
        {
            if (pdx->InterruptStatusCPU & cpu_int_bitmask[cpu])
            {
                pdx->InterruptStatusCPU &= ~cpu_int_bitmask[cpu];
                This->m_pBusCallbackFunc(This->m_pBusCallbackContext, 
                                         s_busintf_cb_cmds[cpu],
						                 ZOE_NULL,
                                         0,
                                         0
                                         );
            }
        }
    }
}


// VPU ISR
//
irqreturn_t zvhpu_irqhandler(int irq, 
							 void *pDevInfo
							 )
{
    CHPUInterface           *This = (CHPUInterface *)pDevInfo;
    PHPU_DEVICE_EXTENSION	pdx = This ? (PHPU_DEVICE_EXTENSION)This->m_pPrivateData : ZOE_NULL;
	irqreturn_t		        irqRet = IRQ_NONE;

    if (pdx)
    {
        // read interrupt status
        //
        // check MDMA
        //
		pdx->InterruptStatusDMA = (vpu_readl(pdx, VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_LIST_DONE_INT_STS_SET_OFFSET) & DMA_HPU_LIST_MASK);
        if (pdx->InterruptStatusDMA)
        {
            // clear the MDMA list done interrupt(source)
		    vpu_writel(pdx, VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_LIST_DONE_INT_STS_CLR_OFFSET, pdx->InterruptStatusDMA);
        }

#ifdef DEBUG_ISR_DMA
		printk("I DMA(0x%x)\n", pdx->InterruptStatusDMA);
#endif //DEBUG_ISR_DMA

        // check CPU
        //
        pdx->InterruptStatusCPU = vpu_readl(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_CPU2HPU_STS_SET_OFFSET);

        // clear the interrupt
		vpu_writel(pdx, INT_CTRL_CPU2CPU_STS_CLR_OFFSET, pdx->InterruptStatusCPU);

        pdx->InterruptStatusCPU &= CPU_ALL_INT_MASK; 
#ifdef DEBUG_ISR_CPU
        printk("I CPU(0x%x)\n", pdx->InterruptStatusCPU);
#endif //DEBUG_ISR_CPU

		// handle cpu interrupt
		//
        if (pdx->InterruptStatusCPU)
        {
	        queue_work(pdx->m_workqueue, &pdx->irq_work_cpu);
			irqRet = IRQ_HANDLED;
		}

		// DMA
        if (pdx->InterruptStatusDMA)
		{
            pdx->InterruptStatusDMA &= ~DMA_HPU_LIST_MASK;
	        queue_work(pdx->m_workqueue, &pdx->irq_work_dma);
			irqRet = IRQ_HANDLED;
		}
    }
    else
    {
		printk("I pdx == NULL?\n");
    }
	return (irqRet);
}



/////////////////////////////////////////////////////////////////////////////
//
//
zoe_errs_t CHPUInterface_ReleaseResources(CHPUInterface *This)
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
    PDMA_DEVICE             pDmaDevice = &pdx->DmaDevice;
	PZV_DMABUF			    pDmaBuf;
    zoe_sosal_obj_id_t      evt = ZOE_NULL;
    uint32_t                cmd;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
				   "CHPUInterface_ReleaseResources()\n"
				   );

	// free resources
	if (pDmaDevice->DmaDescriptor.VirtAddress) 
	{
		dma_free_coherent(0, //&pdx->plat_dev->dev, 
						  ZV_DMA_MAX_NUM_DESCR * sizeof(PED_DMA_DESCRIPTOR), 
						  pDmaDevice->DmaDescriptor.VirtAddress, 
						  pDmaDevice->DmaDescriptor.PhysAddress
						  );
		pDmaDevice->DmaDescriptor.VirtAddress = ZOE_NULL;
	}

    down(&pDmaDevice->m_semDmaReq);

    pDmaBuf = pDmaDevice->pDmaRequest;
    if (pDmaBuf)
    {
        cmd = (ZVDMA_DIR_READ == pDmaBuf->DmaDirection) ? ZV_BUSINTF_DMA_DONE_READ : ZV_BUSINTF_DMA_DONE_WRITE;
        pDmaDevice->pDmaRequest = ZOE_NULL;
        evt = pDmaBuf->evtComplete;
	    zvdma_unmap(&pdx->plat_dev->dev, 
				    pDmaBuf
				    );
	    zvdmabuf_free(pDmaBuf);
	    This->m_pBusCallbackFunc(This->m_pBusCallbackContext, 
                                 cmd,
			                     (zoe_void_ptr_t)evt,
                                 ZOE_ERRS_CANCELLED,
                                 0
			                     );
    }

    while (!list_empty(&pDmaDevice->DmaRequestList))
    {
        pDmaBuf = list_entry(pDmaDevice->DmaRequestList.next, 
                             ZV_DMABUF, 
                             ListEntry
                             );
        list_del(pDmaDevice->DmaRequestList.next);
        cmd = (ZVDMA_DIR_READ == pDmaBuf->DmaDirection) ? ZV_BUSINTF_DMA_DONE_READ : ZV_BUSINTF_DMA_DONE_WRITE;
        evt = pDmaBuf->evtComplete;
	    zvdma_unmap(&pdx->plat_dev->dev, 
				    pDmaBuf
				    );
	    zvdmabuf_free(pDmaBuf);
	    This->m_pBusCallbackFunc(This->m_pBusCallbackContext, 
                                 cmd,
			                     (zoe_void_ptr_t)evt,
                                 ZOE_ERRS_CANCELLED,
                                 0
			                     );
    }

    up(&pDmaDevice->m_semDmaReq);

    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DmaInit(CHPUInterface *This)
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
    PDMA_DEVICE             pDmaDevice = &pdx->DmaDevice;
    zoe_errs_t		        err = ZOE_ERRS_SUCCESS;

    pdx->dma_64bit = ZOE_FALSE;

    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                   This->m_dbgID,
                   "CHPUInterface_DmaInit() pdx(0x%x) 64 bit dma(%d)\n",
                   pdx,
                   pdx->dma_64bit
                   );

	// allocate DMA descriptor
	pDmaDevice->DmaDescriptor.VirtAddress = dma_alloc_coherent(&pdx->plat_dev->dev, 
											                   2 * ZV_DMA_MAX_NUM_DESCR * sizeof(PED_DMA_DESCRIPTOR), 
											                   &pDmaDevice->DmaDescriptor.PhysAddress,
											                   GFP_KERNEL | GFP_DMA32
											                   );
	zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                   This->m_dbgID,
			       "%s() descriptor addr = %x physical addr = %x\n", 
			       __FUNCTION__,
				   pDmaDevice->DmaDescriptor.VirtAddress, 
				   pDmaDevice->DmaDescriptor.PhysAddress
			       );

    // current DMA
    pDmaDevice->pDmaRequest = ZOE_NULL;
    // pending DMA
    INIT_LIST_HEAD(&pDmaDevice->DmaRequestList);
    // clear DMA start flag
    pDmaDevice->bStarted = ZOE_FALSE;
    // Initialize lock for current dma request
	sema_init(&pDmaDevice->m_semDmaReq, 1);

    return (err);
}



/////////////////////////////////////////////////////////////////////////////
//
//
zoe_errs_t CHPUInterface_StartDevice(CHPUInterface *This,
                                     zoe_void_ptr_t pBusData,
                                     uint32_t nBusDataSize
                                     )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
	struct resource         *res;
    zoe_errs_t              err = ZOE_ERRS_SUCCESS;
    long                    ret;

    // get the register base
    res = platform_get_resource(pdx->plat_dev, IORESOURCE_MEM, 0);
    if (res)
    {
        pdx->regs_base = devm_ioremap_resource(&pdx->plat_dev->dev, res);
        pdx->regs_size = (u32)((res->end - res->start) + 1);
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID,
			           "%s() reg base: %p size(%x)\n", 
			           __FUNCTION__,
			           pdx->regs_base,
                       pdx->regs_size
			           );
    }
    else
    {
        pdx->regs_base = (void __iomem *)0;
        pdx->regs_size = 0;
    }

    // get the interrupt
    res = platform_get_resource(pdx->plat_dev, IORESOURCE_IRQ, 0);
    if (res) 
    {
        pdx->irq_nb = res->start;
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID,
				       "%s() IRQ no = %x\n", 
				       pdx->irq_nb
				       );
	}
    else
    {
        pdx->irq_nb = -1;
    }

	spin_lock_init(&pdx->irq_lock);
    if (-1 != pdx->irq_nb)
    {
        ret = devm_request_irq(&pdx->plat_dev->dev,
                               pdx->irq_nb, 
					           zvhpu_irqhandler,
                               0, 
					           "VPU ISR", 
					           (void *)This
					           );
	    if (ret) 
	    {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                           This->m_dbgID,
					       "%s() Unable to install irq, Err = %x\n", 
					       __FUNCTION__,
					       ret
					       );
	    }
        else
        {
            // create work queue
	        pdx->m_workqueue = create_workqueue("zvdev");

	        INIT_WORK(&pdx->irq_work_dma, zvhpu_irq_work_dma_handler);
	        INIT_WORK(&pdx->irq_work_cpu, zvhpu_irq_work_cpu_handler);
        }
    }

#ifdef ZV_SERIALIZE_DMA
    // Initialize lock for DMA access
	sema_init(&pdx->m_semDMA, 1);
#endif //ZV_SERIALIZE_DMA
    // Initialize lock for PIO access
	sema_init(&pdx->m_semPIO, 1);

    pm_runtime_enable(&pdx->plat_dev->dev);
    pm_runtime_get_sync(&pdx->plat_dev->dev);

    // init DMA hardware
    err = CHPUInterface_HwDmaInit(This);

    // enable interrupt
    if (ZOE_SUCCESS(err))
    {
        err = CHPUInterface_EnableInterrupts(This);
    }

	pdx->devconnected = ZOE_TRUE;
	return (err);
}



zoe_errs_t CHPUInterface_StopDevice(CHPUInterface *This)
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
	unsigned long           irqflags;


	zoe_dbg_printf(ZOE_DBG_LVL_WARNING, 
                   This->m_dbgID, 
			       "CHPUInterface_StopDevice()\n"
				   );
 
    // stop DMA
    spin_lock_irqsave(&pdx->irq_lock, irqflags);

    spin_unlock_irqrestore(&pdx->irq_lock, irqflags);
    
    msleep(100);

	// disable device interrupt
    CHPUInterface_DisableInterrupt(This);

	// release interrupt handler
    if (-1 != pdx->irq_nb)
    {
	    devm_free_irq(&pdx->plat_dev->dev,
                      pdx->irq_nb, 
			          This
			          );
        // flush work queue
        if (pdx->m_workqueue)
        {
            flush_workqueue(pdx->m_workqueue);
            destroy_workqueue(pdx->m_workqueue);
            pdx->m_workqueue = ZOE_NULL;
        }
    }

	pdx->devconnected = ZOE_FALSE;

	// unmap io region
    if (pdx->regs_base)
    {
        devm_iounmap(&pdx->plat_dev->dev, 
                     pdx->regs_base
                     );
    }

    return (ZOE_ERRS_SUCCESS);
}



void CHPUInterface_InitializeDeviceState(CHPUInterface *This)
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
                   "CHPUInterface_InitializeDeviceState()\n"
				   );
	pdx->devconnected = ZOE_FALSE;
}



zoe_errs_t CHPUInterface_ReadReg(CHPUInterface *This,
                                 uint32_t dwAddr,
                                 uint32_t * pData,
                                 uint32_t numReg
                                 )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
    uint8_t *               pBuf = (uint8_t *)pData;
    uint32_t                i;

    // xreg address fixup
    if (0x2C000000 == (dwAddr & 0xFC000000))
    {
        // 64MB
        dwAddr -= 0x2C000000;
    }
    else if (0x55000000 == (dwAddr & 0xFF000000))
    {
        // 16MB
        dwAddr -= 0x55000000;
    }
    else if (0x10000000 == (dwAddr & 0xF0000000))
    {
        // dawn
        dwAddr -= 0x10000000;
    }
    else if (dwAddr >= pdx->regs_size)
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID,
				       "%s() invalid address(%x)\n", 
				       __FUNCTION__,
				       dwAddr
				       );
        return (ZOE_ERRS_INVALID);
    }

    if (pdx->regs_base)
    {
        for (i = 0; i < numReg; i++, dwAddr += 4, pBuf += 4)
        {
            *(uint32_t *)pBuf = vpu_readl(pdx, dwAddr);
        }
    }
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_WriteReg(CHPUInterface *This,
                                  uint32_t dwAddr,
                                  uint32_t * pData,
                                  uint32_t numReg
                                  )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
    uint8_t *               pBuf = (uint8_t *)pData;
    uint32_t                i;

    // xreg address fixup
    if (0x2C000000 == (dwAddr & 0xFC000000))
    {
        // 64MB
        dwAddr -= 0x2C000000;
    }
    else if (0x55000000 == (dwAddr & 0xFF000000))
    {
        // 16MB
        dwAddr -= 0x55000000;
    }
    else if (0x10000000 == (dwAddr & 0xF0000000))
    {
        // dawn
        dwAddr -= 0x10000000;
    }
    else if (dwAddr >= pdx->regs_size)
    {

		zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID,
				       "%s() invalid address(%x)\n", 
				       __FUNCTION__,
				       dwAddr
				       );
        return (ZOE_ERRS_INVALID);
    }

    if (pdx->regs_base)
    {
        for (i = 0; i < numReg; i++, dwAddr += 4, pBuf += 4)
        {
            vpu_writel(pdx, dwAddr, *(uint32_t *)pBuf);
        }
    }
    return (ZOE_ERRS_SUCCESS);
}


#ifdef PIO_USE_KMAP
extern void __flush_dcache_area_(void *addr, size_t len);
extern void __invalidate_dcache_area_(void *addr, size_t len);
#endif //PIO_USE_KMAP

zoe_errs_t CHPUInterface_ReadMem(CHPUInterface *This,
                                 zoe_dev_mem_t dwAddr,
                                 uint8_t * pBuffer,  // Address of memory where read data is to be copied to.
                                 uint32_t numBytes     // Number of bytes to be read.
                                 )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    down(&pdx->m_semPIO);

    if (pdx->regs_base)
    {
#ifdef PIO_USE_KMAP
        struct page     *page;
        void            *dev_mem = 0;
        unsigned long   first, last, nr_pages, offset, i;
        uint32_t        remain = numBytes;
        uint32_t        copy = 0;

        first = ((unsigned long)dwAddr & PAGE_MASK) >> PAGE_SHIFT;
        last = (((unsigned long)dwAddr + numBytes - 1) & PAGE_MASK) >> PAGE_SHIFT;
        nr_pages = last - first + 1;
        offset = (unsigned long)dwAddr & ~PAGE_MASK;

#ifdef DEBUG_PIO
		printk("%s ddr_addr(%p) nr_pages(%d) offset(%d) size(%d)\n",
               __FUNCTION__,
               (void *)dwAddr,
               nr_pages,
               offset,
		       numBytes
		       );
#endif //DEBUG_PIO

        for (i = 0; i < nr_pages; i++)
        {
            page = pfn_to_page(first + i);
            if (page)
            {
                dev_mem = kmap(page);
                if (dev_mem)
                {
                    copy =  ZOE_MIN(PAGE_SIZE - offset, remain);
#ifdef DEBUG_PIO
		            printk("%s page(%d:%p) io_addr(%p) => buf(%p) size(%d) [\n",
                           __FUNCTION__,
                           i,
                           page,
		                   (void *)((unsigned long)dev_mem + offset),
                           pBuffer,
                           copy
		                   );
#endif //DEBUG_PIO
                    __invalidate_dcache_area_(dev_mem + offset, copy);
                    memcpy(pBuffer, (void *)((unsigned long)dev_mem + offset), copy);
                    pBuffer += copy;
                    remain -= copy;
                    offset = 0;
                    kunmap(page);
#ifdef DEBUG_PIO
		            printk("%s page(%d:%p) ]\n",
                           __FUNCTION__,
                           i,
                           page
		                   );
#endif //DEBUG_PIO
                }
                else
                {
		            printk("%s kmap() page(%d:%p) failed!!!\n",
                           __FUNCTION__,
                           i,
                           page
		                   );
                    break;
                }
            }
            else
            {
		        printk("%s pfn_to_page(%d: pfn:0x%x) failed!!!\n",
                       __FUNCTION__,
                       i,
                       (uint32_t)(first + i)
		               );
                break;
            }
        }
#else //!PIO_USE_KMAP
        void    *dev_mem = phys_to_virt((unsigned long)dwAddr);
	    flush_cache_all();
        memcpy(pBuffer, dev_mem, numBytes);
#ifdef DEBUG_PIO
		printk("%s ddr_addr(%p) size(%d) io_addr(%p)\n",
               __FUNCTION__,
               (void *)dwAddr,
		       numBytes,
		       dev_mem
		       );
#endif //DEBUG_PIO
#endif //PIO_USE_KMAP
    }
    up(&pdx->m_semPIO);
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_WriteMem(CHPUInterface *This,
                                  zoe_dev_mem_t dwAddr,
                                  uint8_t * pBuffer, // Address of memory where write data is to be copied from.
                                  uint32_t numBytes    // Number of bytes to be written.
                                  )
{
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    down(&pdx->m_semPIO);

    if (pdx->regs_base)
    {
#ifdef PIO_USE_KMAP
        struct page     *page;
        void            *dev_mem = 0;
        unsigned long   first, last, nr_pages, offset, i;
        uint32_t        remain = numBytes;
        uint32_t        copy = 0;

        first = ((unsigned long)dwAddr & PAGE_MASK) >> PAGE_SHIFT;
        last = (((unsigned long)dwAddr + numBytes - 1) & PAGE_MASK) >> PAGE_SHIFT;
        nr_pages = last - first + 1;
        offset = (unsigned long)dwAddr & ~PAGE_MASK;

#ifdef DEBUG_PIO
		printk("%s ddr_addr(%p) size(%d)\n",
               __FUNCTION__,
               (void *)dwAddr,
		       numBytes
		       );
#endif //DEBUG_PIO

        for (i = 0; i < nr_pages; i++)
        {
            page = pfn_to_page(first + i);
            if (page)
            {
                dev_mem = kmap(page);
                if (dev_mem)
                {
                    copy = ZOE_MIN(PAGE_SIZE - offset, remain);
#ifdef DEBUG_PIO
		            printk("%s page(%d:%p) io_addr(%p) <= buf(%p) size(%d) [\n",
                           __FUNCTION__,
                           i,
                           page,
		                   (void *)((unsigned long)dev_mem + offset),
                           pBuffer,
                           copy
		                   );
#endif //DEBUG_PIO
                    memcpy((void *)((unsigned long)dev_mem + offset), pBuffer, copy);
                    __flush_dcache_area_(dev_mem + offset, copy);
                    pBuffer += copy;
                    remain -= copy;
                    offset = 0;
                    kunmap(page);
#ifdef DEBUG_PIO
		            printk("%s page(%d:%p) ]\n",
                           __FUNCTION__,
                           i,
                           page
		                   );
#endif //DEBUG_PIO
                }
                else
                {
		            printk("%s kmap() page(%d:%p) failed!!!\n",
                           __FUNCTION__,
                           i,
                           page
		                   );
                    break;
                }
            }
            else
            {
		        printk("%s pfn_to_page(%d: pfn:0x%x) failed!!!\n",
                       __FUNCTION__,
                       i,
                       (uint32_t)(first + i)
		               );
                break;
            }
        }
#else //!PIO_USE_KMAP
        void    *dev_mem = phys_to_virt((unsigned long)dwAddr);
        memcpy(dev_mem, pBuffer,numBytes);
	    flush_cache_all();
#ifdef DEBUG_PIO
		printk("%s ddr_addr(%p) size(%d) io_addr(%p)\n",
               __FUNCTION__,
               (void *)dwAddr,
		       numBytes,
		       dev_mem
		       );
#endif //DEBUG_DMA
#endif //PIO_USE_KMAP
    }
    up(&pdx->m_semPIO);
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DMARead(CHPUInterface *This,
                                 zoe_dev_mem_t ulAddr,  // in byte
                                 uint32_t ulLength, // in byte
                                 uint8_t * pHostAddr,
                                 uint32_t ulXferMode,
                                 uint32_t ulSwap,
                                 zoe_sosal_obj_id_t evt,
                                 zoe_void_ptr_t p_private
                                 )
{
	PZV_DMABUF              pDmaBuf = ZOE_NULL;
	zoe_errs_t	            err = ZOE_ERRS_SUCCESS;
#ifdef ZV_SERIALIZE_DMA
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    down(&pdx->m_semDMA);
#endif //ZV_SERIALIZE_DMA

	if (ulLength > 0) 
	{
        // allocate DMA request
        pDmaBuf = zvdmabuf_alloc((PZOE_USER_PAGES)p_private); 

        if (!pDmaBuf)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID, 
						   "%s() zvdmabuf_alloc failed\n",
						   __FUNCTION__
						   );
			return (ZOE_ERRS_NOMEMORY);
        }

        // setup dam parameters
        pDmaBuf->bufMode = ulXferMode & DMA_BUFFER_MODE_MASK;
	    pDmaBuf->direction = DMA_FROM_DEVICE;
	    pDmaBuf->data = (void *)pHostAddr;
	    pDmaBuf->size = ulLength;
        pDmaBuf->deviceAddress = ulAddr;
        pDmaBuf->DataSwap = ulSwap;
        pDmaBuf->DmaDirection = ZVDMA_DIR_READ;
        pDmaBuf->evtComplete = evt;

#ifdef _IO_LOCK_ON_TRANSFER
        if ((DMA_BUFFER_MODE_USERPTR == pDmaBuf->bufMode) &&
            !p_private
            )
        {
#endif //_IO_LOCK_ON_TRANSFER
		    err = zvdma_iolock(This, 
						       pDmaBuf
						       );
		    if (!ZOE_SUCCESS(err))
		    {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID, 
						       "%s() zvdma_iolock failed = %d\n",
						       __FUNCTION__,
						       err
						       );
			    goto DMA_READ_ERROR;
		    }
#ifdef _IO_LOCK_ON_TRANSFER
        }
#endif //!_IO_LOCK_ON_TRANSFER

        // start the DMA transfer
        err = zvdmabuf_start_transfer(This, 
                                      pDmaBuf, 
                                      ZOE_TRUE
                                      );
		if (!ZOE_SUCCESS(err))
		{
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID, 
						   "%s() zvdmabuf_start_transfer failed = %d\n",
						   __FUNCTION__,
						   err
						   );
			goto DMA_READ_ERROR;
		}
#ifdef ZV_SERIALIZE_DMA
        else
        {
            if (evt)
            {
                err = zoe_sosal_event_wait(evt, 
                                           2500000  // 2 second
                                           );
	            if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
		                           "!!!!%s zoe_sosal_event_wait failed(%d) addr(%p) len(%d)!!!!\n",
						           __FUNCTION__,
                                   err,
                                   pHostAddr,
                                   ulLength
		                           );
	                queue_work(pdx->m_workqueue, &pdx->irq_work_dma);
                    err = ZOE_ERRS_SUCCESS;
                }
                else
                {
                    zoe_sosal_event_set(evt);
                }
            }
        }
#endif //ZV_SERIALIZE_DMA
	}
    else
    {
        err = ZOE_ERRS_INVALID;
    }

#ifdef ZV_SERIALIZE_DMA
    up(&pdx->m_semDMA);
#endif //ZV_SERIALIZE_DMA
	return (err);

DMA_READ_ERROR:
    zvdmabuf_free(pDmaBuf);
#ifdef ZV_SERIALIZE_DMA
    up(&pdx->m_semDMA);
#endif //ZV_SERIALIZE_DMA
	return (err);
}



zoe_errs_t CHPUInterface_DMAWrite(CHPUInterface *This,
                                  zoe_dev_mem_t ulAddr, // in byte
                                  uint32_t ulLength,// in byte
                                  uint8_t * pHostAddr,
                                  uint32_t ulXferMode,
                                  uint32_t ulSwap,
                                  zoe_sosal_obj_id_t evt,
                                  zoe_void_ptr_t p_private
                                  )
{
	PZV_DMABUF              pDmaBuf = ZOE_NULL;
	zoe_errs_t	            err = ZOE_ERRS_SUCCESS;
#ifdef ZV_SERIALIZE_DMA
    PHPU_DEVICE_EXTENSION   pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    down(&pdx->m_semDMA);
#endif //ZV_SERIALIZE_DMA

	if (ulLength > 0) 
	{
        // allocate DMA request
        pDmaBuf = zvdmabuf_alloc((PZOE_USER_PAGES)p_private); 

        if (!pDmaBuf)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID, 
						   "%s() zvdmabuf_alloc failed\n",
						   __FUNCTION__
						   );
			return (ZOE_ERRS_NOMEMORY);
        }

        // setup dam parameters
        pDmaBuf->bufMode = ulXferMode & DMA_BUFFER_MODE_MASK;
	    pDmaBuf->direction = DMA_TO_DEVICE;
	    pDmaBuf->data = (void *)pHostAddr;
	    pDmaBuf->size = ulLength;
        pDmaBuf->deviceAddress = ulAddr;
        pDmaBuf->DataSwap = ulSwap;
        pDmaBuf->DmaDirection = ZVDMA_DIR_WRITE;
        pDmaBuf->evtComplete = evt;

#ifdef _IO_LOCK_ON_TRANSFER
        if ((DMA_BUFFER_MODE_USERPTR == pDmaBuf->bufMode) &&
            !p_private
            )
        {
#endif //_IO_LOCK_ON_TRANSFER
		    // lock host buffer
		    err = zvdma_iolock(This, 
						       pDmaBuf
						       );
		    if (!ZOE_SUCCESS(err))
		    {
                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID, 
						       "%s() zvdma_iolock failed = %d\n",
						       __FUNCTION__,
						       err
						       );
			    goto DMA_WRITE_ERROR;
		    }
#ifdef _IO_LOCK_ON_TRANSFER
        }
#endif //!_IO_LOCK_ON_TRANSFER

        // start the DMA transfer
        err = zvdmabuf_start_transfer(This, 
                                      pDmaBuf, 
                                      ZOE_TRUE
                                      );
		if (!ZOE_SUCCESS(err))
		{
            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID, 
						   "%s() zvdmabuf_start_transfer failed = %d\n",
						   __FUNCTION__,
						   err
						   );
			goto DMA_WRITE_ERROR;
		}
#ifdef ZV_SERIALIZE_DMA
        else
        {
            if (evt)
            {
                err = zoe_sosal_event_wait(evt, 
                                           2500000  // 2 second
                                           );
	            if (ZOE_FAIL(err))
                {
	                zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
		                           "!!!!%s zoe_sosal_event_wait failed(%d) addr(%p) len(%d)!!!!\n",
						           __FUNCTION__,
                                   err,
                                   pHostAddr,
                                   ulLength
		                           );
	                queue_work(pdx->m_workqueue, &pdx->irq_work_dma);
                    err = ZOE_ERRS_SUCCESS;
                }
                else
                {
                    zoe_sosal_event_set(evt);
                }
            }
        }
#endif //ZV_SERIALIZE_DMA
	}
    else
    {
        err = ZOE_ERRS_INVALID;
    }

#ifdef ZV_SERIALIZE_DMA
    up(&pdx->m_semDMA);
#endif //ZV_SERIALIZE_DMA
	return (err);

DMA_WRITE_ERROR:
	zvdmabuf_free(pDmaBuf);
#ifdef ZV_SERIALIZE_DMA
    up(&pdx->m_semDMA);
#endif //ZV_SERIALIZE_DMA
	return (err);
}



zoe_errs_t CHPUInterface_EnableInterrupts(CHPUInterface *This)
{
	PHPU_DEVICE_EXTENSION	pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    zoe_dbg_printf(ZOE_DBG_LVL_TRACE, 
                   This->m_dbgID,
                   "CHPUInterface_EnableInterrupts() pdx(0x%x)\n",
                   pdx
                   );

    if (pdx->InterruptEnabled)
    {
    	return (ZOE_ERRS_SUCCESS);
    }

    // enable int_ctrl clock, scb in the VPU
    vpu_writel(pdx, VPU_CLK_RST_XREG_SLV_BASE + CLK_RST_CLK_ENA_SET_OFFSET, CLKRST_CLK_ENABLE_INT_CTL);
    // enable int_ctrl, scb in Cafe
    vpu_writel(pdx, VPU_CLK_RST_XREG_SLV_BASE + CLK_RST_SOFT_RST_EXT_OFFSET, CLKRST_SOFT_RST_EXT_INT_CTL_RST);

    // enable int_ctrl clock in scb ctrl
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_INT_CTRL_CLK_ENA_SET_OFFSET, SCB_CTRL_INT_CTRL_CLK);
    // bring int_ctrl in scb ctrl out of reset
    vpu_writel(pdx, VPU_SCB_CTRL_XREG_SLV_BASE + SCB_CTRL_INT_CTRL_RST_SET_OFFSET, SCB_CTRL_INT_CTRL_RST);

    // enable dma interrupt
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_ISR_ISR_INT_EN_SET_OFFSET, DMA_HPU_INT_MASK);
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_ISR_CTRL_STS12_OFFSET, 0);
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_IOL_IBER_HPU_SET_OFFSET, INT_CTRL_IOL_ISR_BANK6_MASK);

    // enable cpu interrupt
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_ISR_ISR_INT_EN_HPU_SET_OFFSET, HPU_ISR_INT_MASK);
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_ISR_CTRL_STS0_HPU_OFFSET, 0);
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_CPU2HPU_ENA_SET_OFFSET, CPU_ALL_INT_MASK);
    vpu_writel(pdx, VPU_INTERRUPT_CTRL_BASE_ADDRESS + INT_CTRL_CPU2HPU_IBER_SET_OFFSET, INT_CTRL_IOL_IBER_MASK);

    // enable mdma interrupt
    vpu_writel(pdx, VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_EN_SET_OFFSET, DMA_HPU_INT_MASK);
    printk("MDMA_INT_EN 0x%08X <= 0x%08X\n", VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_EN_SET_OFFSET, vpu_readl(pdx, VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_EN_SET_OFFSET));
    vpu_writel(pdx, VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_LIST_DONE_INT_EN_SET_OFFSET, DMA_HPU_LIST_MASK);
    printk("LIST_INT_EN 0x%08X <= 0x%08X\n", VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_LIST_DONE_INT_EN_SET_OFFSET, vpu_readl(pdx, VPU_MDMA_INT_STAT_OFFSET + MDMA_INT_LIST_DONE_INT_EN_SET_OFFSET));

    pdx->InterruptEnabled = ZOE_TRUE;

	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_DisableInterrupt(CHPUInterface *This)
{
	PHPU_DEVICE_EXTENSION	pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

    pdx->InterruptEnabled = ZOE_FALSE;
	return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t CHPUInterface_RegisterISR(CHPUInterface *This)
{
	PHPU_DEVICE_EXTENSION	pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;
    long                    ret;

    ret = devm_request_irq(&pdx->plat_dev->dev,
                           pdx->irq_nb, 
				           zvhpu_irqhandler,
                           0, 
				           "VPU ISR", 
				           (void *)This
				           );
	if (ret) 
	{
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR, 
                       This->m_dbgID,
				       "%s() Unable to install irq, Err = %x\n", 
				       __FUNCTION__,
				       ret
				       );
        return (ZOE_ERRS_FAIL);
	}
    else
    {
        return (ZOE_ERRS_SUCCESS);
    }
}



zoe_errs_t CHPUInterface_UnregisterISR(CHPUInterface *This)
{
	PHPU_DEVICE_EXTENSION	pdx = (PHPU_DEVICE_EXTENSION)This->m_pPrivateData;

	// release interrupt handler
    if (-1 != pdx->irq_nb)
    {
	    devm_free_irq(&pdx->plat_dev->dev,
                      pdx->irq_nb, 
			          This
			          );
    }
    return (ZOE_ERRS_SUCCESS);
}



uint32_t CHPUInterface_GetMaxDMASize(CHPUInterface *This)
{
	return (ZV_DMA_MAX_LEN);
}




CHPUInterface * CHPUInterface_Constructor(CHPUInterface *pHPUInterface,
                                          c_object *pParent,
                                          uint32_t dwAttributes,
                                          zoe_void_ptr_t pDO,
                                          zoe_void_ptr_t pDOLayered,
								          ZV_BUSINTF_CALLBACK pBusCallbackFunc,
								          zoe_void_ptr_t pBusCallbackContext,
                                          zoe_dbg_comp_id_t dbgID
                                          )
{
    PHPU_DEVICE_EXTENSION   pdx;
    zoe_errs_t              err;

	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   dbgID,
                   "CHPUInterface_Constructor() PDO = %p FDO = %p\n",
                   pDO,
                   pDOLayered
                   );

    if (pHPUInterface)
    {
        // zero init
        //
        memset(pHPUInterface, 
               0, 
               sizeof(CHPUInterface)
               );

        // c_object
        //
        c_object_constructor(&pHPUInterface->m_Object, 
                             pParent, 
                             OBJECT_ZOE_HPU_INTF,
                             dwAttributes
                             );
        pHPUInterface->m_pBusCallbackFunc = pBusCallbackFunc;
        pHPUInterface->m_pBusCallbackContext = pBusCallbackContext;
        pHPUInterface->m_dbgID = dbgID;

        // initialize OS specific data
        //
	    pHPUInterface->m_pPrivateData = kmalloc(sizeof(HPU_DEVICE_EXTENSION), 
										        GFP_KERNEL
										        );

        if (ZOE_NULL == pHPUInterface->m_pPrivateData)
        {
            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           dbgID,
                           "CHPUInterface_Constructor() Unable to allocate private data\n"
                           );
            CHPUInterface_Destructor(pHPUInterface);
            return (ZOE_NULL);
        }

        pdx = (PHPU_DEVICE_EXTENSION)pHPUInterface->m_pPrivateData;

        memset((void *)pdx,
               0,
               sizeof(HPU_DEVICE_EXTENSION)
               );

        // Save the HPU pHPUInterface object
        pdx->pHpuIntf = pHPUInterface;

        // Save the physical device object for future reference
        pdx->plat_dev = (struct platform_device *)pDOLayered;

        // Initialize DMA buffers
        err = CHPUInterface_DmaInit(pHPUInterface);
        if (!ZOE_SUCCESS(err))
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           dbgID,
					       "CHPUInterface_Constructor CHPUInterface_DmaInit failed!=%d\n", 
					       err
					       );
            CHPUInterface_Destructor(pHPUInterface);
            return (ZOE_NULL);
        }

        // Initialize the device state information managed by the driver.
        CHPUInterface_InitializeDeviceState(pHPUInterface);
    }

    return (pHPUInterface);
}



void CHPUInterface_Destructor(CHPUInterface *This)
{
	zoe_dbg_printf(ZOE_DBG_LVL_TRACE,
                   This->m_dbgID,
                   "CHPUInterface_Destructor()\n"
                   );

	// Release resources.
    CHPUInterface_ReleaseResources(This);

    if (This->m_pPrivateData)
    {
        // free private data(device extension)
        kfree(This->m_pPrivateData);
        This->m_pPrivateData = ZOE_NULL;
	}
}

