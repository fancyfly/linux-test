/*==========================================================================*/
/*!
 * @file svc/pad/api.h
 *
 * Header file containing the public API for the System Controller (SC)
 * Pad Control (PAD) function.
 *
 * @addtogroup PAD_SVC (SVC) Pad Service
 *
 * Module for the Pad Control (PAD) service.
 *
 * @{
 */
/*==========================================================================*/

#ifndef _SC_PAD_API_H
#define _SC_PAD_API_H

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/ipc.h>
#include <soc/imx8/sc/svc/rm/api.h>

/* Defines */

/* Types */

/*!
 * This type is used to declare a pad config. It determines how the
 * output data is driven, pull-up is controlled, and input signal is
 * connected. Normal and OD are typical and only connect the input
 * when the output is not driven.  The IN options are less common and
 * force an input connection even when driving the output.
 */
typedef enum sc_pad_config_e
{
    SC_PAD_CONFIG_NORMAL    = 0,    //!< Normal
    SC_PAD_CONFIG_OD        = 1,    //!< Open Drain
    SC_PAD_CONFIG_OD_IN     = 2,    //!< Open Drain and input
    SC_PAD_CONFIG_OUT_IN    = 3     //!< Output and input
} sc_pad_config_t;

/*!
 * This type is used to declare a pad low-power isolation config.
 * ISO_LATE is the most common setting. ISO_EARLY is only used when
 * an output pin is directly determined by another input pin. The
 * other two are only used when SW wants to directly contol isolation.
 */
typedef enum sc_pad_iso_e
{
    SC_PAD_ISO_OFF          = 0,    //!< ISO latch is transparent
    SC_PAD_ISO_EARLY        = 1,    //!< Follow EARLY_ISO
    SC_PAD_ISO_LATE         = 2,    //!< Follow LATE_ISO
    SC_PAD_ISO_ON           = 3     //!< ISO latched data is held
} sc_pad_iso_t;

/*!
 * This type is used to declare a drive strength. Note it is specific
 * to 28LPP.
 */
typedef enum sc_pad_28lpp_dse_e
{
    SC_PAD_28LPP_DSE_x1     = 0,    //!< Drive strength x1
    SC_PAD_28LPP_DSE_x4     = 1,    //!< Drive strength x4
    SC_PAD_28LPP_DSE_x2     = 2,    //!< Drive strength x2
    SC_PAD_28LPP_DSE_x6     = 3     //!< Drive strength x6
} sc_pad_28lpp_dse_t;

/*!
 * This type is used to declare a pull select. Note it is specific
 * to 28LPP.
 */
typedef enum sc_pad_28lpp_ps_e
{
    SC_PAD_28LPP_PS_PD      = 0,    //!< Pull down
    SC_PAD_28LPP_PS_PU_5K   = 1,    //!< 5K pull up
    SC_PAD_28LPP_PS_PU_47K  = 2,    //!< 47K pull up
    SC_PAD_28LPP_PS_PU_100K = 3     //!< 100K pull up
} sc_pad_28lpp_ps_t;

/*!
 * This type is used to declare a wakeup mode of a pin.
 */
typedef enum sc_pad_wakeup_e
{
    SC_PAD_WAKEUP_OFF       = 0,    //!< Off
    SC_PAD_WAKEUP_CLEAR     = 1,    //!< Clears pending flag
    SC_PAD_WAKEUP_LOW_LVL   = 4,    //!< Low level
    SC_PAD_WAKEUP_FALL_EDGE = 5,    //!< Falling edge
    SC_PAD_WAKEUP_RISE_EDGE = 6,    //!< Rising edge
    SC_PAD_WAKEUP_HIGH_LVL  = 7     //!< High-level
} sc_pad_wakeup_t;

/* Functions */

/*!
 * This function configures the mux settings for a pin. This includes
 * the signal mux, pad config, and low-power isolation mode.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to configure
 * @param[in]     mux         mux setting
 * @param[in]     config      pad config
 * @param[in]     iso         low-power isolation mode
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 SET_MUX(I16 pin, I8 mux, I2 config, I2 iso) */
sc_err_t sc_pad_set_mux(sc_ipc_t ipc, sc_pin_t pin,
    uint8_t mux, sc_pad_config_t config, sc_pad_iso_t iso);

/*!
 * This function configures the general purpose pad control. This
 * is technology dependent and includes things like drive strength,
 * slew rate, pull up/down, etc. Refer to the SoC Reference Manual
 * for bit field details.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to configure
 * @param[in]     ctrl        control value to set
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 SET_GP(I16 pin, I32 ctrl) */
sc_err_t sc_pad_set_gp(sc_ipc_t ipc, sc_pin_t pin, uint32_t ctrl);

/*!
 * This function configures the pad control specific to 28LPP.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to configure
 * @param[in]     dse         drive strength
 * @param[in]     sre         slew rate
 * @param[in]     hys         hysteresis
 * @param[in]     pe          pull enable
 * @param[in]     ps          pull select
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 SET_GP_28LPP(I16 pin, I2 dse, IB sre, IB hys, IB pe, I2 ps) */
sc_err_t sc_pad_set_gp_28lpp(sc_ipc_t ipc, sc_pin_t pin, 
    sc_pad_28lpp_dse_t dse, bool sre, bool hys, bool pe,
    sc_pad_28lpp_ps_t ps);

/*!
 * This function configures the wakeup mode of the pin.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to configure
 * @param[in]     wakeup      wakeup to set
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 SET_WAKEUP(I16 pin, I3 wakeup) */
sc_err_t sc_pad_set_wakeup(sc_ipc_t ipc, sc_pin_t pin,
    sc_pad_wakeup_t wakeup);

/*!
 * This function configures a pad.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to configure
 * @param[in]     mux         mux setting
 * @param[in]     config      pad config
 * @param[in]     iso         low-power isolation mode
 * @param[in]     ctrl        control value
 * @param[in]     wakeup      wakeup to set
 *
 * @see sc_pad_set_mux().
 * @see sc_pad_set_gp().
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 */
/* IDL: E8 SET_ALL(I16 pin, I8 mux, I2 config, I2 iso, I32 ctrl, I3 wakeup) */
sc_err_t sc_pad_set_all(sc_ipc_t ipc, sc_pin_t pin, uint8_t mux, 
    sc_pad_config_t config, sc_pad_iso_t iso, uint32_t ctrl,
    sc_pad_wakeup_t wakeup);

/*!
 * This function gets the mux settings for a pin. This includes
 * the signal mux, pad config, and low-power isolation mode.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to query
 * @param[out]    mux         pointer to return mux setting
 * @param[out]    config      pointer to return pad config
 * @param[out]    iso         pointer to return low-power isolation mode
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 GET_MUX(I16 pin, O8 mux, O2 config, O2 iso) */
sc_err_t sc_pad_get_mux(sc_ipc_t ipc, sc_pin_t pin,
    uint8_t *mux, sc_pad_config_t *config, sc_pad_iso_t *iso);

/*!
 * This function gets the general purpose pad control. This
 * is technology dependent and includes things like drive strength,
 * slew rate, pull up/down, etc. Refer to the SoC Reference Manual
 * for bit field details.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to query
 * @param[out]    ctrl        pointer to return control value
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 GET_GP(I16 pin, O32 ctrl) */
sc_err_t sc_pad_get_gp(sc_ipc_t ipc, sc_pin_t pin, uint32_t *ctrl);

/*!
 * This function gets the pad control specific to 28LPP.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to query
 * @param[out]    dse         pointer to return drive strength
 * @param[out]    sre         pointer to return slew rate
 * @param[out]    hys         pointer to return hysteresis
 * @param[out]    pe          pointer to return pull enable
 * @param[out]    ps          pointer to return pull select
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 GET_GP_28LPP(I16 pin, O2 dse, OB sre, OB hys, OB pe, O2 ps) */
sc_err_t sc_pad_get_gp_28lpp(sc_ipc_t ipc, sc_pin_t pin, 
    sc_pad_28lpp_dse_t *dse, bool *sre, bool *hys, bool *pe,
    sc_pad_28lpp_ps_t *ps);

/*!
 * This function gets the wakeup mode of a pin.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to query
 * @param[out]    wakeup      pointer to return wakeup
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 */
/* IDL: E8 GET_WAKEUP(I16 pin, O3 wakeup) */
sc_err_t sc_pad_get_wakeup(sc_ipc_t ipc, sc_pin_t pin,
    sc_pad_wakeup_t *wakeup);

/*!
 * This function gets a pad's config.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     pin         pin to query
 * @param[out]    mux         pointer to return mux setting
 * @param[out]    config      pointer to return pad config
 * @param[out]    iso         pointer to return low-power isolation mode
 * @param[out]    ctrl        pointer to return control value
 * @param[out]    wakeup      pointer to return wakeup to set
 *
 * @see sc_pad_set_mux().
 * @see sc_pad_set_gp().
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the pin owner
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 */
/* IDL: E8 GET_ALL(I16 pin, O8 mux, O2 config, O2 iso, O32 ctrl, O3 wakeup) */
sc_err_t sc_pad_get_all(sc_ipc_t ipc, sc_pin_t pin, uint8_t *mux, 
    sc_pad_config_t *config, sc_pad_iso_t *iso, uint32_t *ctrl,
    sc_pad_wakeup_t *wakeup);

#endif /* _SC_PAD_API_H */

/**@}*/

