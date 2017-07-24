#ifndef __IMX_DPU_BLITENG
#define __IMX_DPU_BLITENG

#define COMCTRL_INTERRUPTSTATUS0	((uint32_t)(0x68))
#define COMCTRL_INTERRUPTCLEAR0		((uint32_t)(0x60))
#define PIXENGCFG_STORE9_TRIGGER	((uint32_t)(0x954))
#define CMDSEQ_HIF			((uint32_t)(0x400))

#define USE_COMMAND_SEQUENCER	1
#define COMMAND_BUFFER_SIZE	65536 /* up to 64k bytes */
#define CMDSEQ_FIFO_SPACE_THRESHOLD   192
#define WORD_SIZE   4

struct imx_drm_subdrv;

struct dpu_bliteng {
	struct device		*dev;
	void __iomem *base;
	int32_t id;
	struct mutex mutex;
	bool inuse;
	int32_t irq_store9_shdload;
	int32_t irq_store9_framecomplete;
	int32_t irq_store9_seqcomplete;

#if USE_COMMAND_SEQUENCER
	void *buffer_addr_virt;
	dma_addr_t buffer_addr_phy;
#endif

	struct list_head list;
	struct dpu_soc *dpu;
};

int dpu_bliteng_init(struct dpu_bliteng *dpu_bliteng);

#endif
