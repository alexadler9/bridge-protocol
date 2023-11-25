#ifndef _BRIDGE_PROTOCOL_SERVER_EXAMPLE_H_
#define _BRIDGE_PROTOCOL_SERVER_EXAMPLE_H_

/**
 * @ingroup bridge_protocol_example
 *
 * @defgroup bridge_protocol_server_example Server example
 *
 * @brief Simplified example of server using the bridge communication protocol.
 *
 * It is intended only to demonstrate interaction with the protocol. To make this example work:
 * - add your own implementation of the read/write bus (see functions bus_read() and bus_write());
 * - periodically call the bridge_server_process() in some loop.
 *
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

/**@brief Function initializes the bridge of communication with client.
 *
 * @retval true if successful, otherwise false.
 */
bool bridge_init(void);

/**@brief Function to check for requests from client. It is recommended to call somewhere in the loop.
 *
 * @retval BRIDGE_REQUEST_TYPE_UNDEFINED  If no requests were received.
 * @retval request_type                   If request was received and processed (see @ref bridge_request_type_t).
 * @retval -1                             If I/O error occured.
 */
int32_t bridge_process(void);

#endif

/** @} */
