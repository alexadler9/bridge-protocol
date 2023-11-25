#ifndef _BRIDGE_PROTOCOL_H_
#define _BRIDGE_PROTOCOL_H_

/**
 * @defgroup bridge_protocol Bridge communication protocol
 *
 * @brief A client-server communication protocol between two devices, one of which 
 * acts as a server that listens for and responds to client requests.
 *
 * When any protocol call returns result CORRUPTED, the side which detected 
 * the event should perform protocol reset and recovery procedure by calling bridge_protocol_recover().
 * This procedure should also be performed immediately after the protocol has started.
 *
 * All request types are listed in bridge_request_type_t. Request always contains a request type field and, depending
 * on request type, may contain additional data. Server gets requests by calling bridge_protocol_request_read().
 *
 * After getting a request, server send appropriate answer by calling bridge_protocol_*_answer(), where * is a request type.
 * If request type is not listed in bridge_request_type_t, server should not answer.
 *
 * Answer always contains type field, answer types are listed in bridge_answer_type_t: 
 * - value SUCCESS should be used when request successfully processed; 
 * - value REQUEST_REJECTED should be used when received request is inappropriate server state or for similar reason;
 * - value WRONG_REQUEST_ARGUMENTS should be used when any data passed in request have incorrect values.
 *
 * Answer should be sent no later than BRIDGE_PROTOCOL_WAIT_ANSWER_TIMEOUT_MS milliseconds after receiving a request.
 * When client or server sends a message, the time between individual bytes in said message should be less than
 * BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS milliseconds.
 *
 * Client sends requests and receives answers by calling bridge_protocol_*(), where * is request type.
 *
 * Structures for data exchange between devices must be defined in the file bridge_data_types.h.
 *
 * @{
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bridge_data_types.h"

#define BRIDGE_PROTOCOL_VERSION                     1
#define BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS    50
#define BRIDGE_PROTOCOL_WAIT_ANSWER_TIMEOUT_MS      5000
#define BRIDGE_PROTOCOL_RECOVER_TIMEOUT_MS          100

/**@brief Bridge request types. */
typedef enum
{
    BRIDGE_REQUEST_TYPE_UNDEFINED,
    BRIDGE_REQUEST_TYPE_MATCH_PROTOCOL_VERSION,                 /**< Match bridge protocol version. It should never change! */
    BRIDGE_REQUEST_TYPE_GET_DEVICE_INFO,                        /**< Get device (aka server) info. It should never change! */
    //Your request types:
    //...
    BRIDGE_REQUEST_TYPE_FORCE_SIZE_32BITS = UINT32_MAX
} bridge_request_type_t;

/**@brief Bridge request structure. Data field is filled according to request type. */
typedef struct
{
    bridge_request_type_t type;                                 /**< Type of request. */
    
    union
    {
        struct
        {
            uint16_t protocol_version;                          /**< Bridge protocol version. */
        } match_protocol_version;
        //Your request data:
        //...
    } data;
} bridge_request_t;

/**@brief Bridge answer types. */
typedef enum
{
    BRIDGE_ANSWER_TYPE_SUCCESS,                                 /**< Request successfully processed. */
    BRIDGE_ANSWER_TYPE_REQUEST_REJECTED,                        /**< Request rejected because of inappropriate server state or for other similar reason. */
    BRIDGE_ANSWER_TYPE_WRONG_REQUEST_ARGUMENTS,                 /**< Request contains wrong (probably out of appropriate range) arguments. */
    BRIDGE_ANSWER_TYPE_FORCE_SIZE_32BITS = UINT32_MAX
} bridge_answer_type_t;

/**@brief Bridge callback results. */
typedef enum
{
    BRIDGE_CALLBACK_RESULT_SUCCESS,                             /**< Callback operation successfully completed. */
    BRIDGE_CALLBACK_RESULT_READ_TIMEOUT,                        /**< Read callback timed out. */
    BRIDGE_CALLBACK_RESULT_IO_ERROR                             /**< I/O error occured in callback. */
} bridge_callback_result_t;

/**@brief Bus write callback.
 *
 * @param[in] data     Pointer to data to write.
 * @param[in] data_len Length of the data in bytes.
 *
 * @retval BRIDGE_CALLBACK_RESULT_SUCCESS  Data succesfully written to bus.
 * @retval BRIDGE_CALLBACK_RESULT_IO_ERROR If I/O error occured.
 */
typedef bridge_callback_result_t (*bridge_write_callback_t)(uint8_t * data, 
                                                            uint16_t data_len);

/**@brief Bus read callback.
 *
 * @param[out] byte       Pointer to store received byte.
 * @param[in]  timeout_ms Minimal amount of time to wait for byte reception.
 *                        UINT32_MAX means wait forever.
 *
 * @retval BRIDGE_CALLBACK_RESULT_SUCCESS      Data successfully read from bus.
 * @retval BRIDGE_CALLBACK_RESULT_READ_TIMEOUT No data received during set timeout.
 * @retval BRIDGE_CALLBACK_RESULT_IO_ERROR     I/O error occured.
 */
typedef bridge_callback_result_t (*bridge_read_callback_t)(uint8_t * byte, 
                                                           uint32_t timeout_ms);

/**@brief Bridge protocol results. */
typedef enum
{
    BRIDGE_PROTOCOL_RESULT_SUCCESS,                             /**< Protocol operation completed successfully. */
    BRIDGE_PROTOCOL_RESULT_TIMEOUT,                             /**< Protocol operation not completed due to reaching certain timeout. */
    BRIDGE_PROTOCOL_RESULT_CORRUPTED,                           /**< Received message is corrupted, protocol recovery required. */
    BRIDGE_PROTOCOL_RESULT_REQUEST_REJECTED,                    /**< Request rejected because of inappropriate server state or for other similar reason. */
    BRIDGE_PROTOCOL_RESULT_WRONG_REQUEST_ARGUMENTS,             /**< Request contains wrong (probably out of appropriate range) arguments. */
    BRIDGE_PROTOCOL_RESULT_IO_ERROR                             /**< I/O error occured during protocol operation. */
} bridge_protocol_result_t;

/**@brief Recover after receiving corrupted message. Blocks until no new data received 
 *        for BRIDGE_PROTOCOL_RECOVER_TIMEOUT_MS timespan or specified timeout reached.
 *
 * @param[in] read       Read callback.
 * @param[in] timeout_ms Minimal amount of time to wait for protocol recovery.
 *                       If less than BRIDGE_PROTOCOL_RECOVER_TIMEOUT_MS then result is always TIMEOUT.
 *
 * @retval BRIDGE_PROTOCOL_RESULT_SUCCESS   Recovery completed.
 * @retval BRIDGE_PROTOCOL_RESULT_TIMEOUT   Recovery not completed over set timeout.
 * @retval BRIDGE_PROTOCOL_RESULT_IO_ERROR  I/O error occured.
 */
bridge_protocol_result_t bridge_protocol_recover(bridge_read_callback_t read, 
                                                 uint32_t timeout_ms);

/**@brief Read request.
 *
 * @param[in]  read                  Read callback.
 * @param[in]  first_byte_timeout_ms Minimal amount of time to wait for the first byte of request.
 *                                   Timeout between bytes is fixed to BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS.
 *                                   UINT32_MAX means wait forever.
 * @param[out] out_request           Pointer to structure to fill.
 *
 * @retval BRIDGE_PROTOCOL_RESULT_SUCCESS   Request successfully received.
 * @retval BRIDGE_PROTOCOL_RESULT_TIMEOUT   No request received during set timeout.
 * @retval BRIDGE_PROTOCOL_RESULT_CORRUPTED Received message is corrupted, protocol recovery required.
 * @retval BRIDGE_PROTOCOL_RESULT_IO_ERROR  I/O error occured.
 */
bridge_protocol_result_t bridge_protocol_request_read(bridge_read_callback_t read, 
                                                      uint32_t first_byte_timeout_ms, 
                                                      bridge_request_t * out_request);

/**@brief Answer to MATCH_PROTOCOL_VERSION request. Version is fixed as BRIDGE_PROTOCOL_VERSION definition.
 *
 * @param[in] write Write callback.
 *
 * @retval BRIDGE_PROTOCOL_RESULT_SUCCESS   Successfully answered.
 * @retval BRIDGE_PROTOCOL_RESULT_IO_ERROR  I/O error occured.
 */
bridge_protocol_result_t bridge_protocol_match_protocol_version_answer(bridge_write_callback_t write);

/**@brief Answer to GET_DEVICE_INFO request.
 *
 * @param[in] write Write callback.
 * @param[in] info  Pointer to device info structure to send.
 *
 * @retval BRIDGE_PROTOCOL_RESULT_SUCCESS   Successfully answered.
 * @retval BRIDGE_PROTOCOL_RESULT_IO_ERROR  I/O error occured.
 */
bridge_protocol_result_t bridge_protocol_get_device_info_answer(bridge_write_callback_t write,
                                                                const device_info_t * info);

/**@brief Match bridge protocol version.
 *
 * @param[in]  read             Read callback.
 * @param[in]  write            Write callback.
 * @param[out] protocol_version Pointer to variable to store protocol version.
 *                              Stored result should be equal to BRIDGE_PROTOCOL_VERSION.
 *
 * @retval BRIDGE_PROTOCOL_RESULT_SUCCESS   Completed successfully.
 * @retval BRIDGE_PROTOCOL_RESULT_TIMEOUT   No answer received.
 * @retval BRIDGE_PROTOCOL_RESULT_CORRUPTED Received message is corrupted, protocol recovery required.
 * @retval BRIDGE_PROTOCOL_RESULT_IO_ERROR  I/O error occured.
 */
bridge_protocol_result_t bridge_protocol_match_protocol_version(bridge_read_callback_t read,
                                                                bridge_write_callback_t write,
                                                                uint16_t * protocol_version);

/**@brief Get device (aka server) info.
 *
 * @param[in]  read  Read callback.
 * @param[in]  write Write callback.
 * @param[out] info  Pointer to structure to store device info.
 *
 * @retval BRIDGE_PROTOCOL_RESULT_SUCCESS   Completed successfully.
 * @retval BRIDGE_PROTOCOL_RESULT_TIMEOUT   No answer received.
 * @retval BRIDGE_PROTOCOL_RESULT_CORRUPTED Received message is corrupted, protocol recovery required.
 * @retval BRIDGE_PROTOCOL_RESULT_IO_ERROR  I/O error occured.
 */
bridge_protocol_result_t bridge_protocol_get_device_info(bridge_read_callback_t read,
                                                         bridge_write_callback_t write,
                                                         device_info_t * info);

#endif

/** @} */