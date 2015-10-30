/*==========================================================================*/
/*!
 * @file  main/ipc.h
 *
 * Header file for the IPC implementation.
 */
/*==========================================================================*/

#ifndef _SC_IPC_H
#define _SC_IPC_H

/* Includes */

#include <soc/imx8/sc/types.h>

/* Defines */

/* Types */

/* Functions */

/*!
 * This function opens an IPC channel.
 *
 * @param[out]    ipc         return pointer for ipc handle
 * @param[in]     id          id of channel to open
 *
 * @return Returns an error code (SC_ERR_NONE = success, SC_ERR_IPC
 *         otherwise).
 *
 * The \a id parameter is implementation specific. Could be an MU
 * address, pointer to a driver path, channel index, etc.
 */
sc_err_t sc_ipc_open(sc_ipc_t *ipc, uint32_t id);

/*!
 * This function closes an IPC channel.
 *
 * @param[in]     ipc         pointer to id of channel to close
 */
void sc_ipc_close(sc_ipc_t *ipc);

#endif /* _SC_IPC_H */

