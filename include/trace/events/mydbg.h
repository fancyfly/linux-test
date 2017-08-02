#undef TRACE_SYSTEM
#define TRACE_SYSTEM mydbg

#if !defined(_TRACE_MYDBG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MYDBG_H

#include <linux/tracepoint.h>
#include <linux/trace_events.h>
#include <qp/qp.h>

#define TPS(x) tracepoint_string(x)

#define MYDBG_LOCK_IRQMODE_NO		0
#define MYDBG_LOCK_IRQMODE_YES		1
#define MYDBG_LOCK_IRQMODE_SAVE		2

struct rcu_node;

TRACE_EVENT(mydbg_lock_rcu_node,

	TP_PROTO(struct rcu_node* rnp, bool lock, int irqmode, raw_spinlock_t *lockval),

	TP_ARGS(rnp, lock, irqmode, lockval),

	TP_STRUCT__entry(
		__field(struct rcu_node*, rnp)
		__field(bool, lock)
		__field(int, irqmode)
		__field(bool, were_irqs_disabled)
		__field(bool, bad)
		__field(u32, lockval)
	),

	TP_fast_assign(
		__entry->rnp = rnp;
		__entry->lock = lock;
		__entry->irqmode = irqmode;
		__entry->were_irqs_disabled = irqs_disabled();
		__entry->lockval = lockval->raw_lock.slock;
	),

	TP_printk("%slock%s rnp=%p irqs_disabled()=%d lockval=0x%08x",
		__entry->lock ? "" : "un",
		((__entry->irqmode == MYDBG_LOCK_IRQMODE_SAVE)
			? (__entry->lock ? "_irqsave" : "_irqrestore")
			: ((__entry->irqmode == MYDBG_LOCK_IRQMODE_YES)
				? "_irq" : "")),
		__entry->rnp,
		!!__entry->were_irqs_disabled,
		__entry->lockval)
);

#endif /* _TRACE_MYDBG_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
