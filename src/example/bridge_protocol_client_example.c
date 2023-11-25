#include "bridge_protocol_client_example.h"
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

bool bridge_params_check(void)
{
    bridge_protocol_result_t result;
    uint16_t protocol_version;
    device_info_t device_info;
    
    result = bridge_protocol_match_protocol_version(bus_read, bus_write, &protocol_version);
    if (result == BRIDGE_PROTOCOL_RESULT_SUCCESS)
    {
        result = bridge_protocol_get_device_info(bus_read, bus_write, &device_info);
    }
    
    if (result == BRIDGE_PROTOCOL_RESULT_CORRUPTED)
    {
        //Trying to recovery the protocol
        bridge_recovery_wait();
        
        return false;
    }
    
    if (result != BRIDGE_PROTOCOL_RESULT_SUCCESS)
    {
        //I/O error occured
        return false;
    }
    
    if ((protocol_version != BRIDGE_PROTOCOL_VERSION) ||
        (device_info.firmware_version != 1))
    {
        //Mismatch of client and server parameters
        return false;
    }
    
    return true;
}
