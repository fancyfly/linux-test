/*
 * Copyright 2017 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _IMX_XEN_SCMUFRONT_H
#define _IMX_XEN_SCMUFRONT_H

#include "sc/main/rpc.h"

struct xen_scmufront;

extern struct xen_scmufront *g_xen_scmufront;

void xen_scmufront_write(struct xen_scmufront *x, sc_rpc_msg_t *msg);
void xen_scmufront_read(struct xen_scmufront *x, sc_rpc_msg_t *msg);

#endif /* _IMX_XEN_SCMUFRONT_H */
