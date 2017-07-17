#include <linux/io.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <video/dpu.h>

#include "dpu-prv.h"
#include "imx_drm_subdrv.h"
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
		dpu_be = ERR_PTR(-EBUSY);
		goto out;
	}

	dpu_be->inuse = true;
out:
	mutex_unlock(&dpu_be->mutex);

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

int dpu_be_blit(struct dpu_bliteng *dpu_be,
	uint32_t *cmdlist, uint32_t cmdnum)
{
	int i;
	printk("!!! %s(%d), !!!\n", __func__, __LINE__);

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
	printk("!!! %s(%d), !!!\n", __func__, __LINE__);

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
