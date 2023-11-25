#include "bridge_protocol_server_example.h"
#include "protocol/bridge_protocol.h"

/**@brief Function for writing data to bus.
 *
 * @param[in] data     Data for writing.
 * @param[in] data_len Length of the data in bytes.
 *
 * @retval BRIDGE_CALLBACK_RESULT_SUCCESS  If the data was writing successfully.
 * @retval BRIDGE_CALLBACK_RESULT_IO_ERROR If I/O error occured.
 */
static bridge_callback_result_t bus_write(uint8_t * data, uint16_t data_len)
{
#error Add your implementation
    
    return BRIDGE_CALLBACK_RESULT_SUCCESS;
}

/**@brief Function for reading one byte from bus (wait for new byte for provided timeout).
 *
 * @param[out] byte       Pointer to store received byte.
 * @param[in]  timeout_ms Minimal amount of time to wait for byte reception.
 *                        UINT32_MAX means wait forever.
 *
 * @retval BRIDGE_CALLBACK_RESULT_SUCCESS      If the data was reading successfully.
 * @retval BRIDGE_CALLBACK_RESULT_READ_TIMEOUT No data received during set timeout.
 */
static bridge_callback_result_t bus_read(uint8_t * byte, uint32_t timeout_ms)
{
#error Add your implementation
    
    return BRIDGE_CALLBACK_RESULT_SUCCESS;
}

/**@brief Function awaiting recovery after receiving corrupted message.
 *
 * @retval true  If recovery completed.
 * @retval false I/O error occured.
 */
static bool bridge_recovery_wait(void)
{
    bridge_protocol_result_t result;
    do
    {
        result = bridge_protocol_recover(bus_read, 1000);
    } while (result == BRIDGE_PROTOCOL_RESULT_TIMEOUT);
    
    return (result == BRIDGE_PROTOCOL_RESULT_IO_ERROR) ? false : true;
}

bool bridge_init(void)
{
    if (bridge_recovery_wait() == false)
    {
        //IO ERROR occured, something is very wrong
        return false;
    }
    
    return true;
}

int32_t bridge_process(void)
{
    bridge_protocol_result_t result;
    
    bridge_request_t request;
    result = bridge_protocol_request_read(bus_read, 0, &request);
    
    if (result == BRIDGE_PROTOCOL_RESULT_TIMEOUT)
    {
        return BRIDGE_REQUEST_TYPE_UNDEFINED;
    }
    
    if (result == BRIDGE_PROTOCOL_RESULT_IO_ERROR)
    {
        return -1;
    }
    
    if (result == BRIDGE_PROTOCOL_RESULT_CORRUPTED)
    {
        if (bridge_recovery_wait())
        {
            return BRIDGE_REQUEST_TYPE_UNDEFINED;
        }
        
        return -1;
    }
    
    switch (request.type)
    {
        case BRIDGE_REQUEST_TYPE_MATCH_PROTOCOL_VERSION:
        {
            result = bridge_protocol_match_protocol_version_answer(bus_write);
            if (result == BRIDGE_PROTOCOL_RESULT_IO_ERROR)
                return -1;
            
            //In theory, if the protocol versions don't match, 
            //client will not send commands (except, perhaps, command GET_DEVICE_INFO)
            
            break;
        }
            
        case BRIDGE_REQUEST_TYPE_GET_DEVICE_INFO:
        {
            device_info_t info =
            {
                .firmware_version = 1,
                .hardware_version = 1
            };
            
            result = bridge_protocol_get_device_info_answer(bus_write, &info);
            if (result == BRIDGE_PROTOCOL_RESULT_IO_ERROR)
            {
                return -1;
            }
            
            break;
        }
        
        default:
        {
            //Unknown request type, should never happen, if happened anyway - probably it's best to recover
            if (bridge_recovery_wait())
            {
                return BRIDGE_REQUEST_TYPE_UNDEFINED;
            }
            
            return -1;
        }
    }
    
    return request.type;
}
