/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __BTCS_DEFS_H__
#define __BTCS_DEFS_H__

#define BTCS_NUM_REGS 10

enum btcs_cmd {
	BTCS_CAN_IRQ,
	BTCS_REG_MODIFIED,
	BTCS_PRINTK,
};

struct btcs_event {
	int cmd;
	void *arg;
};

struct btcs_shm {
	int reg[BTCS_NUM_REGS];
};

struct btcs_cb_interface {
	int magic;
	int size;
	int state;
	int (*printk) (void *, int);
	int (*setup) (void);
	int (*start) (void);
	int (*stop) (void);
	int (*cleanup) (void);
	int (*callback) (struct btcs_event *);
	int (*ossignal) (struct btcs_event *);
	struct btcs_shm *shm;
};

#endif /* __BTCS_DEFS_H__ */

