/*==========================================================================*/
/*!
 * @file svc/wdog/api.h
 *
 * Header file containing the public API for the System Controller (SC)
 * Watchdog (WDOG) function.
 *
 * @addtogroup WDOG_SVC (SVC) Watchdog Service
 *
 * Module for the Watchdog (WDOG) service. Every resource partition has
 * a watchdog it can use.
 *
 * @{
 */
/*==========================================================================*/

#ifndef _SC_WDOG_API_H
#define _SC_WDOG_API_H

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>

/* Defines */

/* Types */

/*!
 * This type is used to store watchdog time values in milliseconds.
 */
typedef uint32_t sc_wdog_time_t;

/* Functions */

/*!
 * This function sets the watchdog timeout in milliseconds. If not
 * set then the timeout defaults to the max. Once locked this value
 * cannot be changed.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     timeout     timeout period for the watchdog
 *
 * @return Returns an error code (SC_ERR_NONE = success, SC_ERR_LOCKED
 *         = locked).
 */
/* IDL: E8 SET_TIMEOUT(I32 timeout) */
sc_err_t sc_wdog_set_timeout(sc_ipc_t ipc, sc_wdog_time_t timeout);

/*!
 * This function starts the watchdog.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     lock        boolean indicating the lock status
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * If \a lock is set then the watchdog cannot be stopped or the timeout
 * period changed.
 */
/* IDL: E8 START(I1 lock) */
sc_err_t sc_wdog_start(sc_ipc_t ipc, bool lock);

/*!
 * This function stops the watchdog if it is not locked. 
 *
 * @param[in]     ipc         IPC handle
 *
 * @return Returns an error code (SC_ERR_NONE = success, SC_ERR_LOCKED
 *         = locked).
 */
/* IDL: E8 STOP() */
sc_err_t sc_wdog_stop(sc_ipc_t ipc);

/*!
 * This function pings (services, kicks) the watchdog resetting the time
 * before expiration back to the timeout.
 *
 * @param[in]     ipc         IPC handle
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 */
/* IDL: E8 PING() */
sc_err_t sc_wdog_ping(sc_ipc_t ipc);

/*!
 * This function gets the status of the watchdog. All arguments are
 * in milliseconds.
 *
 * @param[in]     ipc         IPC handle
 * @param[out]    timeout         pointer to return the timeout
 * @param[out]    max_timeout     pointer to return the max timeout
 * @param[out]    remaining_time  pointer to return the time remaining until
 *                                trigger
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 */
/* IDL: E8 GET_STATUS(O32 timeout, O32 max_timeout, O32 remaining_time) */
sc_err_t sc_wdog_get_status(sc_ipc_t ipc, sc_wdog_time_t *timeout,
    sc_wdog_time_t *max_timeout, sc_wdog_time_t *remaining_time);

#endif /* _SC_WDOG_API_H */

/**@}*/

