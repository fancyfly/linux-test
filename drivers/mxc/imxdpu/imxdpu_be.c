/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 */
/*
All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  o Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  o Redistributions in binary form must reproduce the above copyright notice,
    thislist of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  o Neither the name of Freescale Semiconductor, Inc. nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/imxdpu.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include "imxdpu_private.h"
#include "imxdpu_registers.h"
#include "imxdpu_events.h"
#include "imxdpu_be.h"

#define USE_COMMAND_SEQUENCER   1
#define COMMAND_BUFFER_SIZE     1024
#define MINUS_CMDSEQ_FIFO_SPACE_THRESHOLD   128

#if USE_COMMAND_SEQUENCER
unsigned int buffer_addr_virt;
dma_addr_t buffer_addr_phy;
#endif

static struct imxdpu_info imxdpu_instance[2];
static DECLARE_WAIT_QUEUE_HEAD(imxdpu_0_be_wq_idle);
static DECLARE_WAIT_QUEUE_HEAD(imxdpu_0_be_wq_load);
static DECLARE_WAIT_QUEUE_HEAD(imxdpu_1_be_wq_idle);
static DECLARE_WAIT_QUEUE_HEAD(imxdpu_1_be_wq_load);
static unsigned int imxdpu_irq_status[2];
static DEFINE_SPINLOCK(imxdpu_lock);
DEFINE_MUTEX(imxdpu_be_mutex);

uint32_t imxdpu_be_read(struct imxdpu_info *imxdpu, uint32_t offset)
{
	uint32_t val = 0;
	val = __raw_readl(imxdpu->base + offset);
	IMXDPU_TRACE_REG("##read reg 0x%08x --> val 0x%08x\n", (uint32_t) offset,
	       (uint32_t) val);
	return val;
}

void imxdpu_be_write(struct imxdpu_info *imxdpu, uint32_t offset,
		     uint32_t value)
{
	__raw_writel(value, imxdpu->base + offset);
	IMXDPU_TRACE_REG("##write reg 0x%08x <-- val 0x%08x\n", (uint32_t) offset,
	       (uint32_t) value);
}

void imxdpu_be_irq_handler(int8_t imxdpu_id, int8_t irq)
{
	unsigned long irq_flags;
	IMXDPU_TRACE_IRQ("%s:%d irq:%x\n", __FUNCTION__, __LINE__, irq);

	spin_lock_irqsave(&imxdpu_lock, irq_flags);
	if (irq == IMXDPU_STORE9_SHDLOAD_IRQ) {
		imxdpu_irq_status[imxdpu_id] |= INTSTAT0_BIT(IMXDPU_STORE9_SHDLOAD_IRQ);
		wake_up(imxdpu_id == 0 ? &imxdpu_0_be_wq_load : &imxdpu_1_be_wq_load);
	}
	else if (irq == IMXDPU_STORE9_SEQCOMPLETE_IRQ) {
		imxdpu_irq_status[imxdpu_id] |= INTSTAT0_BIT(IMXDPU_STORE9_SEQCOMPLETE_IRQ);
		wake_up(imxdpu_id == 0 ? &imxdpu_0_be_wq_idle : &imxdpu_1_be_wq_idle);
	}
	spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
}

void imxdpu_be_setup_decode(struct imxdpu_info *imxdpu,
			    struct fetch_unit *fetch)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (fetch->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHDECODE9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHDECODE9_BURSTBUFFERMANAGEMENT);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->burst_buf);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x1400000C);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHDECODE9_BASEADDRESS0);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_address);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_attributes);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->color_bits);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->color_shift);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->layer_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->clip_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->clip_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->const_color);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->layer_property);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->frame_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->frame_resample);
#else
        imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_CONTROL,
				fetch->control);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHDECODE9_BURSTBUFFERMANAGEMENT,
				fetch->burst_buf);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_BASEADDRESS0,
				fetch->buf_address);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHDECODE9_SOURCEBUFFERATTRIBUTES0,
				fetch->buf_attributes);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHDECODE9_SOURCEBUFFERDIMENSION0,
				fetch->buf_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_COLORCOMPONENTBITS0,
				fetch->color_bits);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHDECODE9_COLORCOMPONENTSHIFT0,
				fetch->color_shift);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_LAYEROFFSET0,
				fetch->layer_offset);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_CLIPWINDOWOFFSET0,
				fetch->clip_offset);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHDECODE9_CLIPWINDOWDIMENSIONS0,
				fetch->clip_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_CONSTANTCOLOR0,
				fetch->const_color);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_LAYERPROPERTY0,
				fetch->layer_property);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_FRAMEDIMENSIONS,
				fetch->frame_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_FRAMERESAMPLING,
				fetch->frame_resample);
#endif
    }
}

void imxdpu_be_setup_persp(struct imxdpu_info *imxdpu, struct fetch_unit *fetch)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (fetch->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHPERSP9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x1400000d);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHPERSP9_BURSTBUFFERMANAGEMENT);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->burst_buf);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_address);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_attributes);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->color_bits);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->color_shift);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->layer_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->clip_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->clip_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->const_color);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->layer_property);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->frame_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->frame_resample);
#else
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_CONTROL,
				fetch->control);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHPERSP9_BURSTBUFFERMANAGEMENT,
				fetch->burst_buf);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_BASEADDRESS0,
				fetch->buf_address);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHPERSP9_SOURCEBUFFERATTRIBUTES0,
				fetch->buf_attributes);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHPERSP9_SOURCEBUFFERDIMENSION0,
				fetch->buf_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_COLORCOMPONENTBITS0,
				fetch->color_bits);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_COLORCOMPONENTSHIFT0,
				fetch->color_shift);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_LAYEROFFSET0,
				fetch->layer_offset);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_CLIPWINDOWOFFSET0,
				fetch->clip_offset);
		imxdpu_be_write(imxdpu,
				IMXDPU_FETCHPERSP9_CLIPWINDOWDIMENSIONS0,
				fetch->clip_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_CONSTANTCOLOR0,
				fetch->const_color);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_LAYERPROPERTY0,
				fetch->layer_property);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_FRAMEDIMENSIONS,
				fetch->frame_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_FRAMERESAMPLING,
				fetch->frame_resample);
#endif
    }
}

void imxdpu_be_setup_eco(struct imxdpu_info *imxdpu,struct fetch_unit* fetch)
{
	IMXDPU_TRACE("%s:%d\n",__FUNCTION__,__LINE__);
	if(fetch->in_pipeline){
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHECO9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF,fetch->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x1400000D);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_FETCHECO9_BURSTBUFFERMANAGEMENT);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->burst_buf);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_address);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_attributes);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->buf_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->color_bits);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->color_shift);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->layer_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->clip_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->clip_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->const_color);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->layer_property);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->frame_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, fetch->frame_resample);
#else
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_CONTROL,
				fetch->control);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_BURSTBUFFERMANAGEMENT,
				fetch->burst_buf);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_BASEADDRESS0,
				fetch->buf_address);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_SOURCEBUFFERATTRIBUTES0,
				fetch->buf_attributes);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_SOURCEBUFFERDIMENSION0,
				fetch->buf_dimension);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_COLORCOMPONENTBITS0,
				fetch->color_bits);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_COLORCOMPONENTSHIFT0,
				fetch->color_shift);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_LAYEROFFSET0,
				fetch->layer_offset);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_CLIPWINDOWOFFSET0,
				fetch->clip_offset);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_CLIPWINDOWDIMENSIONS0,
				fetch->clip_dimension);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_CONSTANTCOLOR0,
				fetch->const_color);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_LAYERPROPERTY0,
				fetch->layer_property);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_FRAMEDIMENSIONS,
				fetch->frame_dimension);
		imxdpu_be_write(imxdpu,IMXDPU_FETCHECO9_FRAMERESAMPLING,
				fetch->frame_resample);
#endif
    }
}

void imxdpu_be_setup_store(struct imxdpu_info *imxdpu, struct store_unit *store)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (store->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_STORE9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_STORE9_BURSTBUFFERMANAGEMENT);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->burst_buf);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000006);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_STORE9_BASEADDRESS);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->buf_address);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->buf_attributes);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->buf_dimension);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->frame_offset);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->color_bits);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, store->color_shift);
#else
		imxdpu_be_write(imxdpu, IMXDPU_STORE9_CONTROL, store->control);
		imxdpu_be_write(imxdpu, IMXDPU_STORE9_BURSTBUFFERMANAGEMENT,
				store->burst_buf);
		imxdpu_be_write(imxdpu, IMXDPU_STORE9_BASEADDRESS,
				store->buf_address);
		imxdpu_be_write(imxdpu,
				IMXDPU_STORE9_DESTINATIONBUFFERATTRIBUTES,
				store->buf_attributes);
		imxdpu_be_write(imxdpu,
				IMXDPU_STORE9_DESTINATIONBUFFERDIMENSION,
				store->buf_dimension);
		imxdpu_be_write(imxdpu, IMXDPU_STORE9_FRAMEOFFSET,
				store->frame_offset);
		imxdpu_be_write(imxdpu, IMXDPU_STORE9_COLORCOMPONENTBITS,
				store->color_bits);
		imxdpu_be_write(imxdpu, IMXDPU_STORE9_COLORCOMPONENTSHIFT,
				store->color_shift);
#endif
    }
}

void imxdpu_be_setup_rop(struct imxdpu_info *imxdpu, struct rop_unit *rop)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (rop->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_ROP9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, rop->control);
#else

		imxdpu_be_write(imxdpu, IMXDPU_ROP9_CONTROL, rop->control);
#endif
    }
}

void imxdpu_be_setup_matrix(struct imxdpu_info *imxdpu,
			    struct matrix_unit *matrix)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (matrix->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_MATRIX9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, matrix->control);
#else

		imxdpu_be_write(imxdpu, IMXDPU_MATRIX9_CONTROL,
				matrix->control);
#endif
    }
}

void imxdpu_be_setup_hscaler(struct imxdpu_info *imxdpu,
			     struct hscaler_unit *hscaler)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (hscaler->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_HSCALER9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, hscaler->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000002);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_HSCALER9_SETUP1);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, hscaler->setup1);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, hscaler->setup2);
#else

		imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_CONTROL,
				hscaler->control);
		imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_SETUP1,
				hscaler->setup1);
		imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_SETUP2,
				hscaler->setup2);
#endif
    }
}

void imxdpu_be_setup_vscaler(struct imxdpu_info *imxdpu,
			     struct vscaler_unit *vscaler)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (vscaler->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_VSCALER9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, vscaler->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000005);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_VSCALER9_SETUP1);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, vscaler->setup1);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, vscaler->setup2);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, vscaler->setup3);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, vscaler->setup4);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, vscaler->setup5);
#else
		imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_CONTROL,
				vscaler->control);
		imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_SETUP1,
				vscaler->setup1);
		imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_SETUP2,
				vscaler->setup2);
		imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_SETUP3,
				vscaler->setup3);
		imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_SETUP4,
				vscaler->setup4);
		imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_SETUP5,
				vscaler->setup5);
#endif
    }
}

void imxdpu_be_setup_blitblend(struct imxdpu_info *imxdpu,
			       struct blitblend_unit *bb)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
	if (bb->in_pipeline) {
#if USE_COMMAND_SEQUENCER
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_BLITBLEND9_CONTROL);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->control);

        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000007);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_BLITBLEND9_CONSTANTCOLOR);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->const_color);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->red_func);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->green_func);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->blue_func);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->alpha_func);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->blend_mode1);
        imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, bb->blend_mode2);
#else
		imxdpu_be_write(imxdpu, IMXDPU_BLITBLEND9_CONTROL, bb->control);
		imxdpu_be_write(imxdpu, IMXDPU_BLITBLEND9_CONSTANTCOLOR,
				bb->const_color);
		imxdpu_be_write(imxdpu, IMXDPU_BLITBLEND9_COLORREDBLENDFUNCTION,
				bb->red_func);
		imxdpu_be_write(imxdpu,
				IMXDPU_BLITBLEND9_COLORGREENBLENDFUNCTION,
				bb->green_func);
		imxdpu_be_write(imxdpu,
				IMXDPU_BLITBLEND9_COLORBLUEBLENDFUNCTION,
				bb->blue_func);
		imxdpu_be_write(imxdpu, IMXDPU_BLITBLEND9_ALPHABLENDFUNCTION,
				bb->alpha_func);
		imxdpu_be_write(imxdpu,IMXDPU_BLITBLEND9_BLENDMODE1,
				bb->blend_mode1);
		imxdpu_be_write(imxdpu,IMXDPU_BLITBLEND9_BLENDMODE2,
				bb->blend_mode2);
#endif
    }
}

void imxdpu_be_setup_engcfg(struct imxdpu_info *imxdpu,
			    struct engcfg_unit *engcfg)
{
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);

#if USE_COMMAND_SEQUENCER
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_FETCHPERSP9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->fetchpersp9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_FETCHDECODE9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->fetchdecode9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_ROP9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->rop9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_MATRIX9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->matrix9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_HSCALER9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->hscaler9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_VSCALER9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->vscaler9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->blitblend9_dynamic);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_STORE9_DYNAMIC);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, engcfg->store9_dynamic);
#else
    imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_FETCHPERSP9_DYNAMIC,
			engcfg->fetchpersp9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_FETCHDECODE9_DYNAMIC,
			engcfg->fetchdecode9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_ROP9_DYNAMIC,
			engcfg->rop9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_MATRIX9_DYNAMIC,
			engcfg->matrix9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_HSCALER9_DYNAMIC,
			engcfg->hscaler9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_VSCALER9_DYNAMIC,
			engcfg->vscaler9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC,
			engcfg->blitblend9_dynamic);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_DYNAMIC,
			engcfg->store9_dynamic);
#endif
}

int imxdpu_be_blit(struct imxdpu_info *imxdpu, struct be_blit_cfg *cfg)
{
	imxdpu = &imxdpu_instance[0];
	imxdpu_be_setup_decode(imxdpu, &cfg->fetch_decode);
	imxdpu_be_setup_persp(imxdpu, &cfg->fetch_persp);
	imxdpu_be_setup_eco(imxdpu,&cfg->fetch_eco);
	imxdpu_be_setup_store(imxdpu, &cfg->store);
	imxdpu_be_setup_rop(imxdpu, &cfg->rop);
	imxdpu_be_setup_matrix(imxdpu, &cfg->matrix);
	imxdpu_be_setup_hscaler(imxdpu, &cfg->hscaler);
	imxdpu_be_setup_vscaler(imxdpu, &cfg->vscaler);
	imxdpu_be_setup_blitblend(imxdpu, &cfg->blitblend);
	imxdpu_be_setup_engcfg(imxdpu, &cfg->engcfg);

#if USE_COMMAND_SEQUENCER
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_PIXENGCFG_STORE9_TRIGGER);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x00000001);

    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_STORE9_START);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x00000001);

    /* Use SYNC, Wait for hardware event */
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x20000100);

    imxdpu_be_read(imxdpu,IMXDPU_COMCTRL_INTERRUPTSTATUS0);
    /* clear shadow load irq */
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x14000001);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, IMXDPU_COMCTRL_INTERRUPTCLEAR0);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_HIF, 0x00000001);

    imxdpu_be_read(imxdpu,IMXDPU_COMCTRL_INTERRUPTSTATUS0);

#else
    imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_TRIGGER, 0x1);
	imxdpu_be_write(imxdpu, IMXDPU_STORE9_START, 0x1);
#endif

    return 0;
}

static uint32_t imxdpu_be_check_irq(uint32_t irq_mask)
{
	uint32_t ret = 0;
	unsigned long irq_flags = 0;

	spin_lock_irqsave(&imxdpu_lock, irq_flags);
	ret = imxdpu_irq_status[0] & irq_mask;
	IMXDPU_TRACE_IRQ("%s:%d imxdpu_irq_status:%x ret:%d\n", __FUNCTION__, __LINE__,
	       imxdpu_irq_status[0], ret);
	imxdpu_irq_status[0] &= ~(irq_mask);
	spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	return ret;
}

int imxdpu_be_wait_shadow_load(struct imxdpu_info *imxdpu)
{
	uint32_t ret = 0;
	unsigned long irq_flags;

	if (!wait_event_timeout
	    (imxdpu_0_be_wq_load, imxdpu_be_check_irq(0x1), HZ)) {
		spin_lock_irqsave(&imxdpu_lock, irq_flags);
		IMXDPU_TRACE_IRQ("%s:%d timeout, imxdpu_irq_status:%x\n", __FUNCTION__, __LINE__,
		       imxdpu_irq_status[0]);
		imxdpu_irq_status[0] &= ~(0x1);
		ret = -ETIMEDOUT;
		spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	}

	return ret;
}

int imxdpu_be_wait_complete(struct imxdpu_info *imxdpu)
{
	uint32_t ret = 0;
	unsigned long irq_flags;

	imxdpu = &imxdpu_instance[0];
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_TRIGGER, 0x10);
	if (!wait_event_timeout
	    (imxdpu_0_be_wq_idle, imxdpu_be_check_irq(0x4), HZ)) {
		spin_lock_irqsave(&imxdpu_lock, irq_flags);
		IMXDPU_TRACE_IRQ("%s:%d timeout, imxdpu_irq_status:%x\n", __FUNCTION__, __LINE__,
		       imxdpu_irq_status[0]);
		imxdpu_irq_status[0] &= ~(0x4);
		ret = -ETIMEDOUT;
		spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	}
	return ret;
}

int imxdpu_be_load(struct imxdpu_info *imxdpu, void __user * p)
{
	struct be_blit_cfg blit_cfg;
	if (copy_from_user(&blit_cfg, p, sizeof(struct be_blit_cfg)))
		return -EFAULT;
	mutex_lock(&imxdpu_be_mutex);

#if USE_COMMAND_SEQUENCER
    imxdpu_cs_wait_fifo_space(NULL);
#endif

    imxdpu_be_blit(NULL, &blit_cfg);

#if USE_COMMAND_SEQUENCER
    /* need no shadow load wait by software if use command sequencer */
#else
    imxdpu_be_wait_shadow_load(NULL);
#endif

    mutex_unlock(&imxdpu_be_mutex);
	return 0;
}

int imxdpu_be_wait(struct imxdpu_info *imxdpu)
{
	mutex_lock(&imxdpu_be_mutex);

#if USE_COMMAND_SEQUENCER
    imxdpu_cs_wait_idle(NULL);
#else
    imxdpu_be_wait_complete(NULL);
#endif

    mutex_unlock(&imxdpu_be_mutex);
	return 0;
}

static long imxdpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;
	switch (cmd) {
	case IMXDPU_IOC_BLIT:
		ret = imxdpu_be_load(NULL, argp);
		if (ret)
			return ret;
		break;
	case IMXDPU_IOC_WAIT:
		ret = imxdpu_be_wait(NULL);
		if (ret)
			return ret;
		break;
	default:
		IMXDPU_TRACE(KERN_INFO "imxdpu unknown ioctl: %d\n", cmd);
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations imxdpu_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = imxdpu_ioctl,
};

static struct miscdevice imxdpu_dev;

static int imxdpu_be_register_dev(int8_t imxdpu_id)
{
	imxdpu_dev.minor = MISC_DYNAMIC_MINOR;
	imxdpu_dev.name = "imxdpu";
	imxdpu_dev.fops = &imxdpu_fops;
	imxdpu_dev.parent = NULL;
	return misc_register(&imxdpu_dev);
}

static int imxdpu_be_request_irq(int8_t imxdpu_id)
{
	/* Clear and enable the IRQ */
#if USE_COMMAND_SEQUENCER
    imxdpu_clear_irq(imxdpu_id, IMXDPU_STORE9_SEQCOMPLETE_IRQ);
    imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE9_SEQCOMPLETE_IRQ);
#else
    imxdpu_clear_irq(imxdpu_id, IMXDPU_STORE9_SHDLOAD_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE9_SHDLOAD_IRQ);

	imxdpu_clear_irq(imxdpu_id, IMXDPU_STORE9_SEQCOMPLETE_IRQ);
	imxdpu_enable_irq(imxdpu_id, IMXDPU_STORE9_SEQCOMPLETE_IRQ);
#endif

	return 0;
}

int imxdpu_be_init(int8_t imxdpu_id, void __iomem * imxdpu_base)
{
	int ret = 0;
	struct imxdpu_info *imxdpu;

	/* temporally enable first BLT engine only */
	if (imxdpu_id > 0) return 0;

	imxdpu = &imxdpu_instance[0];
	imxdpu->base = imxdpu_base;
	if (!imxdpu->base) {
		pr_err("%s(): invalid imxdpu_base \n", __func__);
		return -ENOMEM;
	}
	IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);

	/*static reset all units */
	/* IMXDPU_PIXENGCFG_STORE9_STATIC */
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_STATIC,
			IMXDPU_PIXENGCFG_STORE9_STATIC_RESET_VALUE);
	/* IMXDPU_FETCHDECODE9_STATICCONTROL  */
	imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_STATICCONTROL,
			IMXDPU_FETCHDECODE9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_FETCHPERSP9_STATICCONTROL   */
	imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_STATICCONTROL,
			IMXDPU_FETCHPERSP9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_FETCHECO9_STATICCONTROL     */
	imxdpu_be_write(imxdpu, IMXDPU_FETCHECO9_STATICCONTROL,
			IMXDPU_FETCHECO9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_MATRIX9_STATICCONTROL       */
	imxdpu_be_write(imxdpu, IMXDPU_MATRIX9_STATICCONTROL,
			IMXDPU_MATRIX9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_ROP9_STATICCONTROL          */
	imxdpu_be_write(imxdpu, IMXDPU_ROP9_STATICCONTROL,
			IMXDPU_ROP9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_HSCALER9_STATICCONTROL      */
	imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_STATICCONTROL,
			IMXDPU_HSCALER9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_VSCALER9_STATICCONTROL      */
	imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_STATICCONTROL,
			IMXDPU_VSCALER9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_BLITBLEND9_STATICCONTROL    */
	imxdpu_be_write(imxdpu, IMXDPU_BLITBLEND9_STATICCONTROL,
			IMXDPU_BLITBLEND9_STATICCONTROL_RESET_VALUE);
	/* IMXDPU_STORE9_STATICCONTROL        */
	imxdpu_be_write(imxdpu, IMXDPU_STORE9_STATICCONTROL,
			IMXDPU_STORE9_STATICCONTROL_RESET_VALUE);

	/*static setup all units */
	/* IMXDPU_PIXENGCFG_STORE9_STATIC */
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_STATIC,
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SHDEN,
			 1) |
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_POWERDOWN,
			 IMXDPU_FALSE) |
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE,
			 IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE__SINGLE)
			|
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_STORE9_STATIC_STORE9_DIV,
			 IMXDPU_PIXENGCFG_DIVIDER_RESET));
	/* IMXDPU_FETCHDECODE9_STATICCONTROL  */
	imxdpu_be_write(imxdpu, IMXDPU_FETCHDECODE9_STATICCONTROL,
			IMXDPU_SET_FIELD
			(IMXDPU_FETCHDECODE9_STATICCONTROL_SHDEN,
			 1) |
			IMXDPU_SET_FIELD
			(IMXDPU_FETCHDECODE9_STATICCONTROL_BASEADDRESSAUTOUPDATE,
			 0));
	/* IMXDPU_FETCHPERSP9_STATICCONTROL   */
	imxdpu_be_write(imxdpu, IMXDPU_FETCHPERSP9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_FETCHPERSP9_STATICCONTROL_SHDEN,
					 1) |
			IMXDPU_SET_FIELD
			(IMXDPU_FETCHPERSP9_STATICCONTROL_BASEADDRESSAUTOUPDATE,
			 0));
	/* IMXDPU_FETCHECO9_STATICCONTROL     */
	imxdpu_be_write(imxdpu, IMXDPU_FETCHECO9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_FETCHECO9_STATICCONTROL_SHDEN,
					 1) |
			IMXDPU_SET_FIELD
			(IMXDPU_FETCHECO9_STATICCONTROL_BASEADDRESSAUTOUPDATE,
			 0));
	/* IMXDPU_ROP9_STATICCONTROL          */
	imxdpu_be_write(imxdpu, IMXDPU_ROP9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_ROP9_STATICCONTROL_SHDEN, 1));
	/* IMXDPU_MATRIX9_STATICCONTROL       */
	imxdpu_be_write(imxdpu, IMXDPU_MATRIX9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_MATRIX9_STATICCONTROL_SHDEN,
					 1));
	/* IMXDPU_HSCALER9_STATICCONTROL      */
	imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_HSCALER9_STATICCONTROL_SHDEN,
					 1));
	/* IMXDPU_VSCALER9_STATICCONTROL      */
	imxdpu_be_write(imxdpu, IMXDPU_VSCALER9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_VSCALER9_STATICCONTROL_SHDEN,
					 1));
	/* IMXDPU_BLITBLEND9_STATICCONTROL    */
	imxdpu_be_write(imxdpu, IMXDPU_BLITBLEND9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_BLITBLEND9_STATICCONTROL_SHDEN,
					 1));
	/* IMXDPU_STORE9_STATICCONTROL        */
	imxdpu_be_write(imxdpu, IMXDPU_STORE9_STATICCONTROL,
			IMXDPU_SET_FIELD(IMXDPU_STORE9_STATICCONTROL_SHDEN, 1) |
			IMXDPU_SET_FIELD
			(IMXDPU_STORE9_STATICCONTROL_BASEADDRESSAUTOUPDATE, 0));

	/*rop9 matrix9 hscaler9 vscaler9 blitblend9Dynamic setup */
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_ROP9_DYNAMIC,
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_PRIM_SEL,
			 IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_PRIM_SEL__FETCHPERSP9)
			|
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_SEC_SEL,
			 IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_ROP9_DYNAMIC_ROP9_TERT_SEL,
			 IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
			IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					 IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_MATRIX9_DYNAMIC,
			IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
					 IMXDPU_PIXENGCFG_MATRIX9_DYNAMIC_MATRIX9_SRC_SEL__ROP9)
			| IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					   IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_HSCALER9_DYNAMIC,
			IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
					 IMXDPU_PIXENGCFG_HSCALER9_DYNAMIC_HSCALER9_SRC_SEL__MATRIX9)
			| IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					   IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_VSCALER9_DYNAMIC,
			IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_SRC_SEL,
					 IMXDPU_PIXENGCFG_VSCALER9_DYNAMIC_VSCALER9_SRC_SEL__HSCALER9)
			| IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					   IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC,
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_PRIM_SEL,
			 IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_PRIM_SEL__VSCALER9)
			|
			IMXDPU_SET_FIELD
			(IMXDPU_PIXENGCFG_BLITBLEND9_DYNAMIC_BLITBLEND9_SEC_SEL,
			 IMXDPU_PIXENGCFG_SRC_SEL__DISABLE) |
			IMXDPU_SET_FIELD(IMXDPU_PIXENGCFG_CLKEN,
					 IMXDPU_PIXENGCFG_CLKEN__AUTOMATIC));

#if USE_COMMAND_SEQUENCER
    //Command sequencer static setup
    imxdpu_cs_alloc_command_buffer(NULL);
    imxdpu_cs_static_setup(NULL, NULL);
#endif

	imxdpu_be_register_dev(imxdpu_id);
	imxdpu_be_request_irq(imxdpu_id);

	return ret;
}

#if USE_COMMAND_SEQUENCER
int imxdpu_cs_alloc_command_buffer(struct imxdpu_info *imxdpu)
{
    IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
    imxdpu = &imxdpu_instance[0];

    buffer_addr_virt = (unsigned int)dma_alloc_coherent(NULL, COMMAND_BUFFER_SIZE, (dma_addr_t *)&buffer_addr_phy,GFP_DMA | GFP_KERNEL);
    //buffer_addr_phy -= 0x10000000;

    return 0;
}

int imxdpu_cs_static_setup(struct imxdpu_info *imxdpu, struct cmdSeq_unit *cmdSeq)
{
    int ret = 0;

    IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
    imxdpu = &imxdpu_instance[0];

    imxdpu_cs_wait_idle(NULL);

    /* LockUnlock and LockUnlockHIF */
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_LOCKUNLOCKHIF, IMXDPU_CMDSEQ_LOCKUNLOCKHIF_LOCKUNLOCKHIF__UNLOCK_KEY);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_LOCKUNLOCK, IMXDPU_CMDSEQ_LOCKUNLOCK_LOCKUNLOCK__UNLOCK_KEY);

    /* BufferAddress and BufferSize */
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_BUFFERADDRESS, buffer_addr_phy);
    imxdpu_be_write(imxdpu, IMXDPU_CMDSEQ_BUFFERSIZE, (COMMAND_BUFFER_SIZE/32)<<3);

    return ret;
}

int imxdpu_cs_wait_fifo_space(struct imxdpu_info *imxdpu)
{
    int ret = 0;
    IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
    imxdpu = &imxdpu_instance[0];

    while((imxdpu_be_read(imxdpu, IMXDPU_CMDSEQ_STATUS) & IMXDPU_CMDSEQ_STATUS_FIFOSPACE_MASK) < MINUS_CMDSEQ_FIFO_SPACE_THRESHOLD)
    {
        mdelay(10);
    }

    return ret;
}

int imxdpu_cs_wait_idle(struct imxdpu_info *imxdpu)
{
    int ret = 0;
    IMXDPU_TRACE("%s:%d\n", __FUNCTION__, __LINE__);
    imxdpu = &imxdpu_instance[0];

    while((imxdpu_be_read(imxdpu, IMXDPU_CMDSEQ_STATUS) & IMXDPU_CMDSEQ_STATUS_IDLE_MASK) == 0x0)
    {
        mdelay(10);
    }

    return ret;
}
#endif
