#include <linux/io.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <uapi/drm/imx_drm.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <video/dpu.h>

#include "dpu-prv.h"
#include "dpu-registers.h"
#include "dpu-blit.h"


static inline uint32_t dpu_be_read(struct dpu_bliteng *dpu_be, uint32_t offset)
{
	uint32_t val = 0;

	val = readl(dpu_be->base + offset);
	return val;
}

static inline void dpu_be_write(struct dpu_bliteng *dpu_be, uint32_t value,
	uint32_t offset)
{
	writel(value, dpu_be->base + offset);
}

#if USE_COMMAND_SEQUENCER
int dpu_cs_wait_fifo_space(struct dpu_bliteng *dpu_be)
{
	int ret = 0;

	while ((dpu_be_read(dpu_be, CMDSEQ_STATUS) &
		CMDSEQ_STATUS_FIFOSPACE_MASK) < CMDSEQ_FIFO_SPACE_THRESHOLD) {

		usleep_range(1000, 2000);
	}

	return ret;
}

int dpu_cs_wait_idle(struct dpu_bliteng *dpu_be)
{
	int ret = 0;

	while ((dpu_be_read(dpu_be, CMDSEQ_STATUS) &
		CMDSEQ_STATUS_IDLE_MASK) == 0x0) {

		mdelay(1);
	}

	return ret;
}

int dpu_cs_alloc_command_buffer(struct dpu_bliteng *dpu_be)
{
	dpu_be->buffer_addr_virt =
		dma_alloc_coherent(dpu_be->dpu->dev, COMMAND_BUFFER_SIZE * 4,
			(dma_addr_t *)&(dpu_be->buffer_addr_phy),
			GFP_DMA | GFP_KERNEL);

	memset(dpu_be->buffer_addr_virt, 0, COMMAND_BUFFER_SIZE * 4);

	return 0;
}

int dpu_cs_static_setup(struct dpu_bliteng *dpu_be)
{
	int ret = 0;

	dpu_cs_wait_idle(dpu_be);

	/* LockUnlock and LockUnlockHIF */
	dpu_be_write(dpu_be, CMDSEQ_LOCKUNLOCKHIF_LOCKUNLOCKHIF__UNLOCK_KEY,
		CMDSEQ_LOCKUNLOCKHIF);
	dpu_be_write(dpu_be, CMDSEQ_LOCKUNLOCK_LOCKUNLOCK__UNLOCK_KEY,
		CMDSEQ_LOCKUNLOCK);

	/* Control */
	dpu_be_write(dpu_be, 1 << CMDSEQ_CONTROL_CLEAR_SHIFT,
		CMDSEQ_CONTROL);

	/* BufferAddress and BufferSize */
	dpu_be_write(dpu_be, dpu_be->buffer_addr_phy, CMDSEQ_BUFFERADDRESS);
	dpu_be_write(dpu_be, COMMAND_BUFFER_SIZE / WORD_SIZE,
		CMDSEQ_BUFFERSIZE);

	return ret;
}
#endif

struct dpu_bliteng *dpu_be_get(struct dpu_soc *dpu)
{
	struct dpu_bliteng *dpu_be = dpu->bliteng;

	mutex_lock(&dpu_be->mutex);
	if (dpu_be->inuse) {
		mutex_unlock(&dpu_be->mutex);
		dpu_be = ERR_PTR(-EBUSY);
		goto out;
	}

	dpu_be->inuse = true;
	mutex_unlock(&dpu_be->mutex);

out:
	return dpu_be;
}
EXPORT_SYMBOL(dpu_be_get);

void dpu_be_put(struct dpu_bliteng *dpu_be)
{
	mutex_lock(&dpu_be->mutex);

	dpu_be->inuse = false;

	mutex_unlock(&dpu_be->mutex);
}
EXPORT_SYMBOL(dpu_be_put);

void dpu_be_setup_decode(struct dpu_bliteng *dpu_be,
		struct fetch_unit *fetch)
{
	if (fetch->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, FETCHDECODE9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be,
			FETCHDECODE9_BURSTBUFFERMANAGEMENT, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->burst_buf, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x1400000C, CMDSEQ_HIF);
		dpu_be_write(dpu_be, FETCHDECODE9_BASEADDRESS0, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_address, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_attributes, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->color_bits, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->color_shift, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->layer_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->clip_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->clip_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->const_color, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->layer_property, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->frame_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->frame_resample, CMDSEQ_HIF);
	}
}

void dpu_be_setup_persp(struct dpu_bliteng *dpu_be,
		struct fetch_unit *fetch)
{
	if (fetch->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, FETCHWARP9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x1400000b, CMDSEQ_HIF);
		dpu_be_write(dpu_be,
			FETCHWARP9_BURSTBUFFERMANAGEMENT, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->burst_buf, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_address, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_attributes, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->color_bits, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->color_shift, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->layer_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->clip_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->clip_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->const_color, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->layer_property, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000002, CMDSEQ_HIF);
		dpu_be_write(dpu_be, FETCHWARP9_FRAMEDIMENSIONS, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->frame_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->frame_resample, CMDSEQ_HIF);
	}
}

void dpu_be_setup_eco(struct dpu_bliteng *dpu_be,
		struct fetch_unit *fetch)
{
	if (fetch->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, FETCHECO9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x1400000D, CMDSEQ_HIF);
		dpu_be_write(dpu_be, FETCHECO9_BURSTBUFFERMANAGEMENT,
			CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->burst_buf, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_address, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_attributes, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->buf_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->color_bits, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->color_shift, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->layer_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->clip_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->clip_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->const_color, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->layer_property, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->frame_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, fetch->frame_resample, CMDSEQ_HIF);
	}
}

void dpu_be_setup_store(struct dpu_bliteng *dpu_be,
		struct store_unit *store)
{
	if (store->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, STORE9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, STORE9_BURSTBUFFERMANAGEMENT, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->burst_buf, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000006, CMDSEQ_HIF);
		dpu_be_write(dpu_be, STORE9_BASEADDRESS, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->buf_address, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->buf_attributes, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->buf_dimension, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->frame_offset, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->color_bits, CMDSEQ_HIF);
		dpu_be_write(dpu_be, store->color_shift, CMDSEQ_HIF);
	}
}

void dpu_be_setup_hscaler(struct dpu_bliteng *dpu_be,
		struct hscaler_unit *hscaler)
{
	if (hscaler->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, HSCALER9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, hscaler->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000002, CMDSEQ_HIF);
		dpu_be_write(dpu_be, HSCALER9_SETUP1, CMDSEQ_HIF);
		dpu_be_write(dpu_be, hscaler->setup1, CMDSEQ_HIF);
		dpu_be_write(dpu_be, hscaler->setup2, CMDSEQ_HIF);
	}
}

void dpu_be_setup_vscaler(struct dpu_bliteng *dpu_be,
		struct vscaler_unit *vscaler)
{
	if (vscaler->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, VSCALER9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, vscaler->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000005, CMDSEQ_HIF);
		dpu_be_write(dpu_be, VSCALER9_SETUP1, CMDSEQ_HIF);
		dpu_be_write(dpu_be, vscaler->setup1, CMDSEQ_HIF);
		dpu_be_write(dpu_be, vscaler->setup2, CMDSEQ_HIF);
		dpu_be_write(dpu_be, vscaler->setup3, CMDSEQ_HIF);
		dpu_be_write(dpu_be, vscaler->setup4, CMDSEQ_HIF);
		dpu_be_write(dpu_be, vscaler->setup5, CMDSEQ_HIF);
	}
}

void dpu_be_setup_blitblend(struct dpu_bliteng *dpu_be,
		struct blitblend_unit *bb)
{
	if (bb->in_pipeline) {
		dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
		dpu_be_write(dpu_be, BLITBLEND9_CONTROL, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->control, CMDSEQ_HIF);

		dpu_be_write(dpu_be, 0x14000007, CMDSEQ_HIF);
		dpu_be_write(dpu_be, BLITBLEND9_CONSTANTCOLOR, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->const_color, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->red_func, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->green_func, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->blue_func, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->alpha_func, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->blend_mode1, CMDSEQ_HIF);
		dpu_be_write(dpu_be, bb->blend_mode2, CMDSEQ_HIF);
	}
}

void dpu_be_setup_engcfg(struct dpu_bliteng *dpu_be,
		struct engcfg_unit *engcfg)
{
	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_FETCHWARP9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->fetchpersp9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_FETCHDECODE9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->fetchdecode9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_ROP9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->rop9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_MATRIX9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->matrix9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_HSCALER9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->hscaler9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_VSCALER9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->vscaler9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_BLITBLEND9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->blitblend9_dynamic, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_STORE9_DYNAMIC, CMDSEQ_HIF);
	dpu_be_write(dpu_be, engcfg->store9_dynamic, CMDSEQ_HIF);
}

int dpu_be_blit_cfg(struct dpu_bliteng *dpu_be,
		struct drm_imx_dpu_blit *blit)
{
	dpu_be_setup_decode(dpu_be, &blit->fetch_decode);
	dpu_be_setup_persp(dpu_be, &blit->fetch_persp);
	dpu_be_setup_eco(dpu_be, &blit->fetch_eco);
	dpu_be_setup_store(dpu_be, &blit->store);
	dpu_be_setup_hscaler(dpu_be, &blit->hscaler);
	dpu_be_setup_vscaler(dpu_be, &blit->vscaler);
	dpu_be_setup_blitblend(dpu_be, &blit->blitblend);
	dpu_be_setup_engcfg(dpu_be, &blit->engcfg);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, PIXENGCFG_STORE9_TRIGGER, CMDSEQ_HIF);
	dpu_be_write(dpu_be, 0x00000001, CMDSEQ_HIF);

	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, STORE9_START, CMDSEQ_HIF);
	dpu_be_write(dpu_be, 0x00000001, CMDSEQ_HIF);

	/* Use SYNC, Wait for hardware event */
	dpu_be_write(dpu_be, 0x20000100, CMDSEQ_HIF);

	/* clear shadow load irq */
	dpu_be_write(dpu_be, 0x14000001, CMDSEQ_HIF);
	dpu_be_write(dpu_be, COMCTRL_INTERRUPTCLEAR0, CMDSEQ_HIF);
	dpu_be_write(dpu_be, 0x00000001, CMDSEQ_HIF);

	return 0;
}
EXPORT_SYMBOL(dpu_be_blit_cfg);

int dpu_be_blit(struct dpu_bliteng *dpu_be,
	uint32_t *cmdlist, uint32_t cmdnum)
{
	int i;

	for (i = 0; i < cmdnum; i++) {
		dpu_be_write(dpu_be, cmdlist[i], CMDSEQ_HIF);
	}

	return 0;
}
EXPORT_SYMBOL(dpu_be_blit);

#define STORE9_SEQCOMPLETE_IRQ		2U
#define STORE9_SEQCOMPLETE_IRQ_MASK	1U<<STORE9_SEQCOMPLETE_IRQ
int dpu_be_wait(struct dpu_bliteng *dpu_be)
{
	dpu_be_write(dpu_be, 0x10, PIXENGCFG_STORE9_TRIGGER);

	while ((dpu_be_read(dpu_be, COMCTRL_INTERRUPTSTATUS0) &
		STORE9_SEQCOMPLETE_IRQ_MASK) == 0) {
		usleep_range(1000, 2000);
	}

	dpu_be_write(dpu_be, STORE9_SEQCOMPLETE_IRQ_MASK,
		COMCTRL_INTERRUPTCLEAR0);

	return 0;
}
EXPORT_SYMBOL(dpu_be_wait);

int dpu_be_init_units(struct dpu_bliteng *dpu_be)
{
	uint32_t staticcontrol;
	uint32_t pixengcfg_unit_static, pixengcfg_unit_dynamic;

	staticcontrol =
	1 << FETCHDECODE9_STATICCONTROL_SHDEN_SHIFT |
	0 << FETCHDECODE9_STATICCONTROL_BASEADDRESSAUTOUPDATE_SHIFT |
	FETCHDECODE9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, FETCHDECODE9_STATICCONTROL);

	staticcontrol =
	1 << FETCHWARP9_STATICCONTROL_SHDEN_SHIFT |
	0 << FETCHWARP9_STATICCONTROL_BASEADDRESSAUTOUPDATE_SHIFT |
	FETCHWARP9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, FETCHWARP9_STATICCONTROL);

	staticcontrol =
	1 << FETCHECO9_STATICCONTROL_SHDEN_SHIFT |
	0 << FETCHECO9_STATICCONTROL_BASEADDRESSAUTOUPDATE_SHIFT |
	FETCHECO9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, FETCHECO9_STATICCONTROL);

	staticcontrol =
	1 << HSCALER9_STATICCONTROL_SHDEN_SHIFT |
	HSCALER9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, HSCALER9_STATICCONTROL);

	staticcontrol =
	1 << VSCALER9_STATICCONTROL_SHDEN_SHIFT |
	VSCALER9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, VSCALER9_STATICCONTROL);

	staticcontrol =
	1 << ROP9_STATICCONTROL_SHDEN_SHIFT |
	ROP9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, ROP9_STATICCONTROL);

	staticcontrol =
	1 << MATRIX9_STATICCONTROL_SHDEN_SHIFT |
	MATRIX9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, MATRIX9_STATICCONTROL);

	staticcontrol =
	1 << BLITBLEND9_STATICCONTROL_SHDEN_SHIFT |
	BLITBLEND9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, BLITBLEND9_STATICCONTROL);

	staticcontrol =
	1 << STORE9_STATICCONTROL_SHDEN_SHIFT |
	0 << STORE9_STATICCONTROL_BASEADDRESSAUTOUPDATE_SHIFT |
	STORE9_STATICCONTROL_RESET_VALUE;
	dpu_be_write(dpu_be, staticcontrol, STORE9_STATICCONTROL);

	/* Safety_Pixengcfg Static */
	pixengcfg_unit_static =
	1 << PIXENGCFG_STORE9_STATIC_STORE9_SHDEN_SHIFT |
	0 << PIXENGCFG_STORE9_STATIC_STORE9_POWERDOWN_SHIFT |
	PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE__SINGLE <<
	PIXENGCFG_STORE9_STATIC_STORE9_SYNC_MODE_SHIFT |
	PIXENGCFG_STORE9_STATIC_STORE9_SW_RESET__OPERATION <<
	PIXENGCFG_STORE9_STATIC_STORE9_SW_RESET_SHIFT |
	PIXENGCFG_DIVIDER_RESET <<
	PIXENGCFG_STORE9_STATIC_STORE9_DIV_SHIFT;
	dpu_be_write(dpu_be, pixengcfg_unit_static, PIXENGCFG_STORE9_STATIC);

	/* Safety_Pixengcfg Dynamic */
	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_FETCHDECODE9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_FETCHDECODE9_DYNAMIC);

	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_FETCHWARP9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_FETCHWARP9_DYNAMIC);

	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_ROP9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_ROP9_DYNAMIC);

	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_MATRIX9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_MATRIX9_DYNAMIC);

	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_HSCALER9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_HSCALER9_DYNAMIC);

	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_VSCALER9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_VSCALER9_DYNAMIC);

	pixengcfg_unit_dynamic =
	PIXENGCFG_CLKEN__AUTOMATIC << PIXENGCFG_CLKEN_SHIFT |
	PIXENGCFG_BLITBLEND9_DYNAMIC_RESET_VALUE;
	dpu_be_write(dpu_be, pixengcfg_unit_dynamic,
		PIXENGCFG_BLITBLEND9_DYNAMIC);

	return 0;
}

int dpu_bliteng_init(struct dpu_bliteng *dpu_bliteng)
{
	struct dpu_soc *dpu = dev_get_drvdata(dpu_bliteng->dev->parent);
	struct platform_device *dpu_pdev =
		container_of(dpu->dev, struct platform_device, dev);
	struct resource *res;
	unsigned long dpu_base;
	void __iomem *base;

	res = platform_get_resource(dpu_pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	dpu_base = res->start;

	/* remap with bigger size */
	base = devm_ioremap(dpu->dev, dpu_base, 64*SZ_1K);
	dpu_bliteng->base = base;
	dpu_bliteng->dpu = dpu;

	mutex_init(&dpu_bliteng->mutex);

	/* Init the uints used by blit engine */
	/* Maybe this should be in dpu-common.c */
	dpu_be_init_units(dpu_bliteng);

#if USE_COMMAND_SEQUENCER
	dpu_cs_alloc_command_buffer(dpu_bliteng);
	dpu_cs_static_setup(dpu_bliteng);
#endif

	dpu->bliteng = dpu_bliteng;

	return 0;	
}
EXPORT_SYMBOL_GPL(dpu_bliteng_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Semiconductor");
MODULE_DESCRIPTION("i.MX DPU BLITENG");
MODULE_ALIAS("platform:imx-dpu-bliteng");
