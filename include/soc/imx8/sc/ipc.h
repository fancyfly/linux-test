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

#define SC_NUM_IPC          5

#define SC_IPC_AP_CH0       0
#define SC_IPC_AP_CH1       1
#define SC_IPC_AP_CH2       2
#define SC_IPC_AP_CH3       3
#define SC_IPC_AP_CH4       4

/* Types */

typedef uint32_t sc_ipc_t;

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
 * @param[in]     ipc         id of channel to close
 */
void sc_ipc_close(sc_ipc_t ipc);

/*!
 * This function reads a message from an IPC channel.
 *
 * @param[in]     ipc         id of channel read from
 * @param[out]    msg         message buffer to read
 *
 * This function will block if no message is available to be read.
 */
void sc_ipc_read(sc_ipc_t ipc, void *data);

/*!
 * This function writes a message to an IPC channel.
 *
 * @param[in]     ipc         id of channel to write to
 * @param[in]     msg         message buffer to write
 *
 * This function will block if the outgoing buffer is full.
 */
void sc_ipc_write(sc_ipc_t ipc, void *data);

/*!
 * This function initializes the MU connection to SCU.
 *
 * @return  Returns an error code.
 */
int imx8dv_mu_init(void);

#endif /* _SC_IPC_H */

