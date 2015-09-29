/*==========================================================================*/
/*!
 * @file svc/pm/api.h
 *
 * Header file containing the public API for the System Controller (SC)
 * Power Management (PM) function. This includes functions for power state
 * control, clock control, reset control, and wake-up event control.
 *
 * @addtogroup PM_SVC (SVC) Power Management Service
 *
 * Module for the Power Management (PM) service.
 *
 * @{
 */
/*==========================================================================*/

#ifndef _SC_PM_API_H
#define _SC_PM_API_H

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>

/* Defines */

/*!
 * @name Defines for type widths
 */
/*@{*/
#define SC_PM_POWER_MODE_W      2       //!< Width of sc_pm_power_mode_t
#define SC_PM_CLOCK_MODE_W      2       //!< Width of sc_pm_clock_mode_t
#define SC_PM_RESET_TYPE_W      1       //!< Width of sc_pm_reset_type_t
#define SC_PM_RESET_REASON_W    3       //!< Width of sc_pm_reset_reason_t
/*@}*/

/*!
 * @name Defines for ALL parameters
 */
/*@{*/
#define SC_PM_CLK_ALL   UINT8_MAX       //!< All clocks
/*@}*/

/* Types */

/*!
 * This type is used to declare a power mode. Note resources only use
 * SC_PM_PW_MODE_OFF and SC_PM_PW_MODE_ON. The other modes are used only
 * as system power modes.
 */
typedef enum sc_pm_power_mode_e
{
    SC_PM_PW_MODE_OFF           = 0,    //!< Power off
    SC_PM_PW_MODE_STBY          = 1,    //!< Power in standby
    SC_PM_PW_MODE_LP            = 2,    //!< Power in low-power
    SC_PM_PW_MODE_ON            = 3     //!< Power on
} sc_pm_power_mode_t;

/*!
 * This type is used to declare a clock.
 */
typedef uint8_t sc_pm_clk_t;

/*!
 * This type is used to declare a clock mode.
 */
typedef enum sc_pm_clk_mode_e
{
    SC_PM_CLK_MODE_OFF          = 0,    //!< Clock is disabled
    SC_PM_CLK_MODE_AUTOGATE_SW  = 1,    //!< Clock is in SW autogate mode
    SC_PM_CLK_MODE_AUTOGATE_HW  = 2,    //!< Clock is in HW autogate mode
    SC_PM_CLK_MODE_ON           = 3     //!< Clock is enabled.
} sc_pm_clk_mode_t;

/*!
 * This type is used to declare clock rates.
 */
typedef uint32_t sc_pm_clock_rate_t;

/*!
 * This type is used to declare a desired reset type.
 */
typedef enum sc_pm_reset_type_e
{
    SC_PM_RESET_TYPE_COLD       = 0,    //!< Cold reset
    SC_PM_RESET_TYPE_WARM       = 1     //!< Warm reset
} sc_pm_reset_type_t;

/*!
 * This type is used to declare a reason for a reset.
 */
typedef enum sc_pm_reset_reason_e
{
    SC_PM_RESET_REASON_POR      = 0,    //!< Power on reset
    SC_PM_RESET_REASON_WARM     = 1,    //!< Warm reset
    SC_PM_RESET_REASON_SW       = 2,    //!< Software reset
    SC_PM_RESET_REASON_WDOG     = 3,    //!< Watchdog reset
    SC_PM_RESET_REASON_LOCKUP   = 4,    //!< Lockup reset
    SC_PM_RESET_REASON_TAMPER   = 5,    //!< Tamper reset
    SC_PM_RESET_REASON_TEMP     = 6,    //!< Temp reset
    SC_PM_RESET_REASON_LOW_VOLT = 7,    //!< Low voltage reset
} sc_pm_reset_reason_t;

/* Functions */

/*!
 * @name Power Functions
 * @{
 */

/*!
 * This function sets the power mode of a partition.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pt          handle of partition
 * @param[in]     mode        power mode to apply
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid partition or mode,
 * - SC_ERR_NOACCESS if caller's partition is not the owner or 
 *   parent of \a pt
 *
 * All resources owned by \a pt that are on will have their power
 * mode changed to \a mode.
 *
 * @see sc_pm_set_resource_power_mode().
 */
/* IDL: E8 SET_SYS_POWER_MODE(I8 pt, I4 mode) */
sc_err_t sc_pm_set_sys_power_mode(sc_ipc_t ipc, sc_rm_pt_t pt,
    sc_pm_power_mode_t mode);

/*!
 * This function gets the power mode of a partition.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pt          handle of partition
 * @param[out]    mode        pointer to return power mode
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid partition
 */
/* IDL: E8 GET_SYS_POWER_MODE(I8 pt, O4 mode) */
sc_err_t sc_pm_get_sys_power_mode(sc_ipc_t ipc, sc_rm_pt_t pt,
    sc_pm_power_mode_t *mode);

/*!
 * This function sets the power mode of a resource.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    ID of the resource
 * @param[in]     mode        power mode to apply
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid resource or mode,
 * - SC_ERR_NOACCESS if caller's partition is not the resource owner
 *   or parent of the owner
 *
 * Note only SC_PM_PW_MODE_OFF and SC_PM_PW_MODE_ON are valid. Other modes
 * will return an error. Resources set to SC_PM_PW_MODE_ON will reflect the
 * power mode of the partition and will change as that changes.
 *
 *  @see sc_pm_set_sys_power_mode().
 */
/* IDL: E8 SET_RESOURCE_POWER_MODE(I16 resource, I4 mode) */
sc_err_t sc_pm_set_resource_power_mode(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_pm_power_mode_t mode);

/*!
 * This function gets the power mode of a resource.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    ID of the resource
 * @param[out]    mode        pointer to return power mode
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Note only SC_PM_PW_MODE_OFF and SC_PM_PW_MODE_ON are valid. The value
 * returned does not reflect the power mode of the partition..
 */
/* IDL: E8 GET_RESOURCE_POWER_MODE(I16 resource, O4 mode) */
sc_err_t sc_pm_get_resource_power_mode(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_pm_power_mode_t *mode);

/* @} */

/*!
 * @name Clock Functions
 * @{
 */

/*!
 * This function sets the rate of a resource's clock.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    ID of the resource
 * @param[in]     clk         clock to affect
 * @param[in,out] rate        pointer to clock rate to set,
 *                            return actual rate
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid resource or clock,
 * - SC_ERR_NOACCESS if caller's partition is not the resource owner
 *   or parent of the owner,
 * - SC_ERR_LOCKED if rate locked (usually because shared clock)
 *
 * Refer to the [Clock List](@ref CLOCKS) for valid clock values.
 */
/* IDL: E8 SET_CLOCK_RATE(I16 resource, I4 clk, IO32 rate) */
sc_err_t sc_pm_set_clock_rate(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_pm_clk_t clk, sc_pm_clock_rate_t *rate);

/*!
 * This function gets the rate of a resource's clock.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    ID of the resource
 * @param[in]     clk         clock to affect
 * @param[out]    rate        pointer to return clock rate
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid resource or clock,
 * - SC_ERR_NOACCESS if caller's partition is not the resource owner
 *   or parent of the owner
 *
 * Refer to the [Clock List](@ref CLOCKS) for valid clock values.
 */
/* IDL: E8 GET_CLOCK_RATE(I16 resource, I4 clk, O32 rate) */
sc_err_t sc_pm_get_clock_rate(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_pm_clk_t clk, sc_pm_clock_rate_t *rate);

/*!
 * This function enables/disables a resource's clock.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    ID of the resource
 * @param[in]     clk         clock to affect
 * @param[in]     enable      enable if true; otherwise disabled
 * @param[in]     autog       HW auto clock gating
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid resource or clock,
 * - SC_ERR_NOACCESS if caller's partition is not the resource owner
 *   or parent of the owner
 *
 * Refer to the [Clock List](@ref CLOCKS) for valid clock values.
 */
/* IDL: E8 CLOCK_ENABLE(I16 resource, I4 clk, I1 enable, I1 autog) */
sc_err_t sc_pm_clock_enable(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_pm_clk_t clk, bool enable, bool autog);

/* @} */

/*!
 * @name Reset Functions
 * @{
 */
 
/*!
 * This function is used to boot a partition.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pt          handle of partition to boot
 * @param[in]     boot_cpu    ID of the CPU resource to start
 * @param[in]     boot_addr   64-bit boot address
 * @param[in]     boot_mu     ID of the MU that must be powered
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid partition, resource, or addr,
 * - SC_ERR_NOACCESS if caller's partition is not the parent of the
 *   partition to boot
 */
/* IDL: E8 BOOT(I8 pt, I16 boot_cpu, I64 boot_addr, I16 boot_mu) */
sc_err_t sc_pm_boot(sc_ipc_t ipc, sc_rm_pt_t pt, sc_rsrc_t boot_cpu,
    sc_faddr_t boot_addr, sc_rsrc_t boot_mu);

/*!
 * This function is used to reboot the caller's partition.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     type        reset type
 *
 * If \a type is SC_PM_RESET_TYPE_COLD, then most peripherals owned by
 * the calling partition will be reset if possible. SC state (partitions,
 * power, clocks, etc.) is reset. The boot SW of the booting CPU must be
 * able to handle peripherals that that are not reset.
 *
 * If \a type is SC_PM_RESET_TYPE_WARM, then only the boot CPU is reset.
 * SC state (partitions, power, clocks, etc.) are NOT reset. The boot SW
 * of the booting CPU must be able to handle peripherals and SC state that
 * that are not reset.
 *
 * If this function returns, then the reset did not occur due to an
 * invalid parameter.
 */
/* IDL: RN REBOOT(I1 type) */
void sc_pm_reboot(sc_ipc_t ipc, sc_pm_reset_type_t type);

/*!
 * This function gets a caller's reset reason.
 *
 * @param[in]     ipc         IPC handle
 * @param[out]    reason      pointer to return reset reason
 */
/* IDL: R0 RESET_REASON(O4 reason) */
void sc_pm_reset_reason(sc_ipc_t ipc, sc_pm_reset_reason_t *reason);

/*!
 * This function is used to start/stop a CPU.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    ID of the CPU resource
 * @param[in]     enable      start if true; otherwise stop
 * @param[in]     addr        64-bit boot address
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_ERR_PARM if invalid resource or addr,
 * - SC_ERR_NOACCESS if caller's partition is not the parent of the
 *   resource (CPU) owner
 */
/* IDL: E8 CPU_START(I16 resource, I1 enable, I64 addr) */
sc_err_t sc_pm_cpu_start(sc_ipc_t ipc, sc_rsrc_t resource, bool enable,
    sc_faddr_t addr);

/* @} */

#endif /* _SC_PM_API_H */

/**@}*/

