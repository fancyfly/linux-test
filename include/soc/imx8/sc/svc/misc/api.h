/*==========================================================================*/
/*!
 * @file svc/misc/api.h
 *
 * Header file containing the public API for the System Controller (SC)
 * Miscellaneous (MISC) function.
 *
 * @addtogroup MISC_SVC (SVC) Miscellaneous Service
 *
 * Module for the Miscellaneous (MISC) service.
 *
 * @{
 */
/*==========================================================================*/

#ifndef _SC_MISC_API_H
#define _SC_MISC_API_H

/* Includes */

#include <soc/imx8/sc/types.h>
#include <soc/imx8/sc/svc/rm/api.h>

/* Defines */

/* Types */

/* Functions */

/*!
 * This function sets a misc. control value.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    resource the control is associated with
 * @param[in]     ctrl        control to change
 * @param[in]     val         value to apply to the control
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the control owner or parent
 *   of the owner
 */
/* IDL: E8 SET_CONTROL(I16 resource, I32 ctrl, I32 val) */
sc_err_t sc_misc_set_control(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_ctrl_t ctrl, uint32_t val);

/*!
 * This function gets a misc. control value.
 *
 * @param[in]     ipc         IPC handle
 * @param[in]     resource    resource the control is associated with
 * @param[in]     ctrl        control to get
 * @param[out]    val         pointer to return the control value
 *
 * @return Returns an error code (SC_ERR_NONE = success).
 *
 * Return errors:
 * - SC_PARM if arguments out of range or invalid,
 * - SC_ERR_NOACCESS if caller's partition is not the control owner or parent
 *   of the owner
 */
/* IDL: E8 GET_CONTROL(I16 resource, I32 ctrl, O32 val) */
sc_err_t sc_misc_get_control(sc_ipc_t ipc, sc_rsrc_t resource,
    sc_ctrl_t ctrl, uint32_t *val);

#endif /* _SC_MISC_API_H */

/**@}*/

