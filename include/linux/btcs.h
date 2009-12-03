/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __BTCS_H__
#define __BTCS_H__

extern int __init btcs_init(void);
extern void btcs_poll(void);
extern void __init btcs_reserve_sdram(void);

#endif /* __BTCS_H__ */

