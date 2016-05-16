/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*!
 * Header file containing the public API for the System Controller (SC)
 * Interrupt (IRQ) function.
 *
 * @addtogroup IRQ_SVC (SVC) Interrupt Service
 *
 * Module for the Interrupt (IRQ) service.
 *
 * @{
 */

#ifndef _SC_IRQ_API_H
#define _SC_IRQ_API_H

/* Includes */

#include <soc/imx8/sc/types.h>

/* Defines */

#define SC_IRQ_NUM_GROUP    1           //!< Number of groups

/* Types */

/*!
 * This type is used to declare an interrupt group.
 */
typedef enum sc_irq_group_e
{
    SC_IRQ_GROUP_ALARM      = 0,        //!< Alarm interrupts
} sc_irq_group_t;

/*!
 * This type is used to declare a bit mask of alarm interrupts.
 */
typedef enum sc_irq_alarm_e
{
    SC_IRQ_ALARM_WDOG       = (1 << 0), //!< Watchdog interrupt
    SC_IRQ_ALARM_TEMP       = (1 << 1), //!< Temp alarm interrupt
    SC_IRQ_ALARM_CPU0_TEMP  = (1 << 2), //!< CPU0 temp alarm interrupt
    SC_IRQ_ALARM_CPU1_TEMP  = (1 << 3), //!< CPU1 temp alarm interrupt
    SC_IRQ_ALARM_GPU0_TEMP  = (1 << 4), //!< GPU0 temp alarm interrupt
    SC_IRQ_ALARM_GPU1_TEMP   = (1 << 5), //!< GPU1 temp alarm interrupt
    SC_IRQ_ALARM_PMIC0_TEMP  = (1 << 6), //!< PMIC0 temp alarm interrupt
    SC_IRQ_ALARM_PMIC1_TEMP  = (1 << 7), //!< PMIC1 temp alarm interrupt
    SC_IRQ_ALARM_RTC         = (1 << 8)  //!< RTC interrupt
} sc_irq_alarm_t;

/* Functions */

/*!
 * This function enables/disables interrupts. If pending interrupts
 * are unmasked, an interrupt will be triggered.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    MU channel
 * @param[in]     group       group the interrupts are in
 * @param[in]     mask        mask of interrupts to affect
 * @param[in]     enable      state to change interrupts to
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if group invalid
 */
sc_err_t sc_irq_enable(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_irq_group_t group, uint32_t mask, bool enable);

/*!
 * This function returns the current interrupt status (regardless if
 * masked). Automatically clears pending interrupts.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    MU channel
 * @param[in]     group       groups the interrupts are in
 * @param[in]     status      status of interrupts
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if group invalid
 *
 * The returned \a status may show interrupts pending that are
 * currently masked.
 */
sc_err_t sc_irq_status(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_irq_group_t group, uint32_t *status);

#endif /* _SC_IRQ_API_H */

/**@}*/

