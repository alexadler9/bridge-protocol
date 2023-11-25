#ifndef _BRIDGE_PROTOCOL_CLIENT_EXAMPLE_H_
#define _BRIDGE_PROTOCOL_CLIENT_EXAMPLE_H_

/**
 * @ingroup bridge_protocol_example
 *
 * @defgroup bridge_protocol_client_example Client example
 *
 * @brief Simplified example of client using the bridge communication protocol.
 *
 * It is intended only to demonstrate interaction with the protocol. To make this example work:
 * - add your own implementation of the read/write bus (see functions bus_read() and bus_write());
 *
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

/**@brief Function initializes the bridge of communication with server.
 *
 * @retval true if successful, otherwise false.
 */
bool bridge_init(void);

/**@brief Function requests and checks data from the server.
 *
 * @retval true if successful, otherwise false.
 */
bool bridge_params_check(void);

#endif

/** @} */
