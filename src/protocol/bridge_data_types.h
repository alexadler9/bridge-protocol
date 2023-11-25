#ifndef _BRIDGE_DATA_TYPES_H_
#define _BRIDGE_DATA_TYPES_H_

/**
 * @ingroup bridge_protocol
 *
 * @defgroup bridge_data_types Structures for data exchange via bridge communication protocol
 *
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

/**@brief Device info structure.
*/
typedef struct
{
    uint32_t hardware_version;          /**< Device hardware version. */
    uint32_t firmware_version;          /**< Device firmware version. */
} device_info_t;

#endif

/** @} */