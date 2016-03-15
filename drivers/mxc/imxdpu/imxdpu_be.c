/*
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
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

#include "imxdpu_private.h"
#include "imxdpu_registers.h"
#include "imxdpu_be.h"

static struct imxdpu_info imxdpu_instance;
static DECLARE_WAIT_QUEUE_HEAD(imxdpu_be_idle_waitqueue);
static DECLARE_WAIT_QUEUE_HEAD(imxdpu_be_load_waitqueue);
static DEFINE_SPINLOCK(imxdpu_lock);
static unsigned int imxdpu_irq_status;
DEFINE_MUTEX(imxdpu_be_mutex);

uint32_t imxdpu_be_read(struct imxdpu_info *imxdpu, uint32_t offset)
{
	uint32_t val = 0;
	val = __raw_readl(imxdpu->base + offset);
	printk("##read reg 0x%08x --> val 0x%08x\n", (uint32_t) offset,
	       (uint32_t) val);
	return val;
}

void imxdpu_be_write(struct imxdpu_info *imxdpu, uint32_t offset,
		     uint32_t value)
{
	__raw_writel(value, imxdpu->base + offset);
	printk("##write reg 0x%08x <-- val 0x%08x\n", (uint32_t) offset,
	       (uint32_t) value);
}

//static irqreturn_t imxdpu_isr(int irq, void *data)
irqreturn_t imxdpu_isr(int irq, void *data)
{
	uint32_t status;
	unsigned long irq_flags;
	struct imxdpu_info *imxdpu = &imxdpu_instance;
	spin_lock_irqsave(&imxdpu_lock, irq_flags);
	status = imxdpu_be_read(imxdpu, IMXDPU_COMCTRL_INTERRUPTSTATUS0);
	printk("%s:%d status:%x\n", __FUNCTION__, __LINE__, status);
	imxdpu_irq_status |= status;
	if (status & 0x1) {
		wake_up(&imxdpu_be_load_waitqueue);
		imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTCLEAR0, 0x1);
	}
	if (status & 0x2) {
		imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTCLEAR0, 0x2);
	}
	if (status & 0x4) {
		wake_up(&imxdpu_be_idle_waitqueue);
		imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTCLEAR0, 0x4);
	}
	spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	return IRQ_HANDLED;
}

void imxdpu_be_setup_decode(struct imxdpu_info *imxdpu,
			    struct fetch_unit *fetch)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (fetch->in_pipeline) {
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
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_persp(struct imxdpu_info *imxdpu, struct fetch_unit *fetch)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (fetch->in_pipeline) {
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
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_store(struct imxdpu_info *imxdpu, struct store_unit *store)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (store->in_pipeline) {
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
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_rop(struct imxdpu_info *imxdpu, struct rop_unit *rop)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (rop->in_pipeline) {
		imxdpu_be_write(imxdpu, IMXDPU_ROP9_CONTROL, rop->control);
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_matrix(struct imxdpu_info *imxdpu,
			    struct matrix_unit *matrix)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (matrix->in_pipeline) {
		imxdpu_be_write(imxdpu, IMXDPU_MATRIX9_CONTROL,
				matrix->control);
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_hscaler(struct imxdpu_info *imxdpu,
			     struct hscaler_unit *hscaler)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (hscaler->in_pipeline) {
		imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_CONTROL,
				hscaler->control);
		imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_SETUP1,
				hscaler->setup1);
		imxdpu_be_write(imxdpu, IMXDPU_HSCALER9_SETUP2,
				hscaler->setup2);
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_vscaler(struct imxdpu_info *imxdpu,
			     struct vscaler_unit *vscaler)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (vscaler->in_pipeline) {
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
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_blitblend(struct imxdpu_info *imxdpu,
			       struct blitblend_unit *bb)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	if (bb->in_pipeline) {
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
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

void imxdpu_be_setup_engcfg(struct imxdpu_info *imxdpu,
			    struct engcfg_unit *engcfg)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
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
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}

int imxdpu_be_blit(struct imxdpu_info *imxdpu, struct be_blit_cfg *cfg)
{
	imxdpu = &imxdpu_instance;
	imxdpu_be_setup_decode(imxdpu, &cfg->fetch_decode);
	imxdpu_be_setup_persp(imxdpu, &cfg->fetch_persp);
	imxdpu_be_setup_store(imxdpu, &cfg->store);
	imxdpu_be_setup_rop(imxdpu, &cfg->rop);
	imxdpu_be_setup_matrix(imxdpu, &cfg->matrix);
	imxdpu_be_setup_hscaler(imxdpu, &cfg->hscaler);
	imxdpu_be_setup_vscaler(imxdpu, &cfg->vscaler);
	imxdpu_be_setup_blitblend(imxdpu, &cfg->blitblend);
	imxdpu_be_setup_engcfg(imxdpu, &cfg->engcfg);
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_TRIGGER, 0x1);
	imxdpu_be_write(imxdpu, IMXDPU_STORE9_START, 0x1);
	return 0;
}

static uint32_t imxdpu_be_check_irq(uint32_t irq_mask)
{
	uint32_t ret = 0;
	unsigned long irq_flags = 0;

	spin_lock_irqsave(&imxdpu_lock, irq_flags);
	ret = imxdpu_irq_status & irq_mask;
	printk("%s:%d imxdpu_irq_status:%x ret:%d\n", __FUNCTION__, __LINE__,
	       imxdpu_irq_status, ret);
	imxdpu_irq_status &= ~(irq_mask);
	spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	return ret;
}

int imxdpu_be_wait_shadow_load(struct imxdpu_info *imxdpu)
{
#if 0
	uint32_t read_data;
	imxdpu = &imxdpu_instance;

	read_data = imxdpu_be_read(imxdpu, IMXDPU_COMCTRL_INTERRUPTSTATUS0);
	while (!(read_data & 0x00000001)) {
		read_data =
		    imxdpu_be_read(imxdpu, IMXDPU_COMCTRL_INTERRUPTSTATUS0);
	}
	imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTCLEAR0, 0x1);
	return 0;
#else
	uint32_t ret = 0;
	unsigned long irq_flags;

	if (!wait_event_timeout
	    (imxdpu_be_load_waitqueue, imxdpu_be_check_irq(0x1), HZ)) {
		spin_lock_irqsave(&imxdpu_lock, irq_flags);
		printk("%s:%d imxdpu_irq_status:%x\n", __FUNCTION__, __LINE__,
		       imxdpu_irq_status);
		imxdpu_irq_status &= ~(0x1);
		ret = -ETIMEDOUT;
		printk("%s:%d timeout\n", __FUNCTION__, __LINE__);
		spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	}

	return ret;
#endif
}

int imxdpu_be_wait_complete(struct imxdpu_info *imxdpu)
{
#if 0
	uint32_t read_data;
	imxdpu = &imxdpu_instance;

	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_TRIGGER, 0x10);

	read_data = imxdpu_be_read(imxdpu, IMXDPU_COMCTRL_INTERRUPTSTATUS0);
	while (!(read_data & 0x00000004)) {
		read_data =
		    imxdpu_be_read(imxdpu, IMXDPU_COMCTRL_INTERRUPTSTATUS0);
	}
	imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTCLEAR0, 0x7);
	return 0;
#else
	uint32_t ret = 0;
	unsigned long irq_flags;

	imxdpu = &imxdpu_instance;
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_TRIGGER, 0x10);
	if (!wait_event_timeout
	    (imxdpu_be_idle_waitqueue, imxdpu_be_check_irq(0x4), HZ)) {
		spin_lock_irqsave(&imxdpu_lock, irq_flags);
		printk("%s:%d imxdpu_irq_status:%x\n", __FUNCTION__, __LINE__,
		       imxdpu_irq_status);
		imxdpu_irq_status &= ~(0x4);
		ret = -ETIMEDOUT;
		printk("%s:%d timeout\n", __FUNCTION__, __LINE__);
		spin_unlock_irqrestore(&imxdpu_lock, irq_flags);
	}
	return ret;
#endif
}

int imxdpu_be_load(struct imxdpu_info *imxdpu, void __user * p)
{
	struct be_blit_cfg blit_cfg;
	if (copy_from_user(&blit_cfg, p, sizeof(struct be_blit_cfg)))
		return -EFAULT;
	mutex_lock(&imxdpu_be_mutex);
	imxdpu_be_blit(NULL, &blit_cfg);
	imxdpu_be_wait_shadow_load(NULL);
	mutex_unlock(&imxdpu_be_mutex);
	return 0;
}

int imxdpu_be_wait(struct imxdpu_info *imxdpu)
{
	mutex_lock(&imxdpu_be_mutex);
	imxdpu_be_wait_complete(NULL);
	mutex_unlock(&imxdpu_be_mutex);
	return 0;
}

int imxdpu_be_init(void)
{
	int ret = 0;
	struct imxdpu_info *imxdpu;
	//imxdpu = kzalloc(sizeof(struct imxdpu_info), GFP_KERNEL);
	imxdpu = &imxdpu_instance;
	imxdpu->base = ioremap(IMXDPU_REGS_BASE_PHY0, IMXDPU_REGS_BASE_SIZE);
	if (!imxdpu->base) {
		pr_err("%s(): can't ioremap IMXDPU physical base \n", __func__);
		return -ENOMEM;
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	imxdpu_be_read(imxdpu, 0x0);

	/*static reset all units */
	/* IMXDPU_PIXENGCFG_STORE9_STATIC */
	imxdpu_be_write(imxdpu, IMXDPU_PIXENGCFG_STORE9_STATIC,
			IMXDPU_PIXENGCFG_STORE9_STATIC_RESET_VALUE);
	///* IMXDPU_FETCHDECODE9_STATICCONTROL  */
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

	/*enable interrput */
	imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTENABLE0, 0x7);
	imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_INTERRUPTCLEAR0, 0x7);
	/*usr interrupt */
	imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTENABLE0, 0x7);
	imxdpu_be_write(imxdpu, IMXDPU_COMCTRL_USERINTERRUPTCLEAR0, 0x7);

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
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	return ret;
}
