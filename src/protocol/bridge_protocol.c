#include "bridge_protocol.h"

//Message format:
//(uint16_t) payload size | (enum) request or answer type | (array) payload | (uint16_t) checksum of everything prior

#define sizeofmember(type, member) sizeof(((type *)0)->member)

typedef struct
{
    bridge_answer_type_t type;                            //Type of answer. 
                                                          //Data field is filled according to request type.
    union
    {
        struct
        {
            uint16_t protocol_version;                    //Bridge protocol version
        } match_protocol_version;
        
        struct
        {
            device_info_t info;                           //Device info
        } get_device_info;
    } data;
} bridge_answer_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static uint16_t checksum_init(void)
{
    return 0xFFFF;
}

/** Name  : CRC-16 CCITT
 *  Poly  : 0x1021    x^16 + x^12 + x^5 + 1
 *  Init  : 0xFFFF
 *  Revert: false
 *  XorOut: 0x0000
 *  Check : 0x29B1 ("123456789")
 *  MaxLen: 4095 bytes
 */
static uint16_t checksum_append(uint16_t current_crc, const void * data, uint16_t data_len)
{
    static const uint16_t crc16_table[256] =
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
        0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
        0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
        0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
        0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
        0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
        0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
        0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
        0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
        0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
        0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
        0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
        0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
        0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
        0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
        0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
        0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
        0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
        0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
        0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
        0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
        0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
        0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    };
    
    uint16_t result = current_crc;
    for (uint32_t i = 0; i < data_len; i++)
    {
        result = (result << 8) ^ crc16_table[(result >> 8) ^ ((uint8_t*)data)[i]];
    }
    
    return result;
}

static bridge_protocol_result_t callback_to_protocol_result(bridge_callback_result_t callback_result, 
                                                            bool corrupted_if_timeout)
{
    switch (callback_result)
    {
        case BRIDGE_CALLBACK_RESULT_READ_TIMEOUT:
        {
            return (corrupted_if_timeout) ? BRIDGE_PROTOCOL_RESULT_CORRUPTED : BRIDGE_PROTOCOL_RESULT_TIMEOUT;
        }
        
        case BRIDGE_CALLBACK_RESULT_IO_ERROR:
        {
            return BRIDGE_PROTOCOL_RESULT_IO_ERROR;
        }
        
        default:
        {
            return BRIDGE_PROTOCOL_RESULT_SUCCESS;
        }
    }
}

static uint16_t request_payload_size_get(bridge_request_type_t request_type)
{
    switch (request_type)
    {
        case BRIDGE_REQUEST_TYPE_MATCH_PROTOCOL_VERSION:
        {
            return sizeofmember(bridge_request_t, data.match_protocol_version);
        }
        
        default:
        {
            return 0;
        }
    }
}

static uint16_t answer_payload_size_get(bridge_request_type_t request_type, 
                                        bridge_answer_type_t answer_type)
{
    if (answer_type != BRIDGE_ANSWER_TYPE_SUCCESS)
    {
        return 0;
    }
    
    switch (request_type)
    {
        case BRIDGE_REQUEST_TYPE_MATCH_PROTOCOL_VERSION:
        {
            return sizeofmember(bridge_answer_t, data.match_protocol_version);
        }
        
        case BRIDGE_REQUEST_TYPE_GET_DEVICE_INFO:
        {
            return sizeofmember(bridge_answer_t, data.get_device_info);
        }
        
        default:
        {
            return 0;
        }
    }
}

static bridge_callback_result_t multiple_bytes_read(bridge_read_callback_t read, 
                                                    uint32_t first_byte_timeout_ms,
                                                    void * out_data, 
                                                    uint32_t bytes_count, 
                                                    bool * out_timeout_is_on_first_byte)
{
    for (uint32_t i = 0; i < bytes_count; i++)
    {
        bridge_callback_result_t read_result = read(&((uint8_t*)out_data)[i], (i == 0) ?
                                                    first_byte_timeout_ms : 
                                                    BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS);
        
        if (read_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
        {
            if ((read_result == BRIDGE_CALLBACK_RESULT_READ_TIMEOUT) && 
                (out_timeout_is_on_first_byte != NULL))
            {
                *out_timeout_is_on_first_byte = (i == 0);
            }
            
            return read_result;
        }
    }

    return BRIDGE_CALLBACK_RESULT_SUCCESS;
}

static bridge_protocol_result_t answer_read(bridge_read_callback_t read, 
                                            bridge_answer_t * out_answer, 
                                            bridge_request_type_t request_type)
{
    bridge_callback_result_t callback_result;
    
    uint16_t payload_size;
    bool timeout_is_on_first_byte;
    callback_result = multiple_bytes_read(read, 
                                          BRIDGE_PROTOCOL_WAIT_ANSWER_TIMEOUT_MS, 
                                          &payload_size, 
                                          sizeof(payload_size), 
                                          &timeout_is_on_first_byte);
    
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, !timeout_is_on_first_byte);
    }
    
    callback_result = multiple_bytes_read(read, 
                                          BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS, 
                                          &out_answer->type, 
                                          sizeof(out_answer->type), 
                                          NULL);
    
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, true);
    }
    
    if (answer_payload_size_get(request_type, out_answer->type) != payload_size)
    {
        return BRIDGE_PROTOCOL_RESULT_CORRUPTED;
    }
    
    if (payload_size > 0)
    {
        callback_result = multiple_bytes_read(read, 
                                              BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS, 
                                              &out_answer->data, 
                                              payload_size,
                                              NULL);
        
        if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
        {
            return callback_to_protocol_result(callback_result, true);
        }
    }
    
    uint16_t checksum;
    callback_result = multiple_bytes_read(read, 
                                          BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS, 
                                          &checksum, 
                                          sizeof(checksum), 
                                          NULL);
    
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, true);
    }
    
    uint16_t checksum_calculated = checksum_init();
    checksum_calculated = checksum_append(checksum_calculated, &payload_size, sizeof(payload_size));
    checksum_calculated = checksum_append(checksum_calculated, &out_answer->type, sizeof(out_answer->type));
    checksum_calculated = checksum_append(checksum_calculated, &out_answer->data, payload_size);
    
    if (checksum != checksum_calculated)
    {
        return BRIDGE_PROTOCOL_RESULT_CORRUPTED;
    }
    
    if (out_answer->type == BRIDGE_ANSWER_TYPE_REQUEST_REJECTED)
    {
        return BRIDGE_PROTOCOL_RESULT_REQUEST_REJECTED;
    }
    
    if (out_answer->type == BRIDGE_ANSWER_TYPE_WRONG_REQUEST_ARGUMENTS)
    {
        return BRIDGE_PROTOCOL_RESULT_WRONG_REQUEST_ARGUMENTS;
    }
    
    return BRIDGE_PROTOCOL_RESULT_SUCCESS;
}

static bridge_protocol_result_t request_write(bridge_write_callback_t write, 
                                              const bridge_request_t * request)
{
    bridge_callback_result_t callback_result;
    
    uint16_t payload_size = request_payload_size_get(request->type);
    
    callback_result = write((uint8_t*)&payload_size, sizeof(payload_size));
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, false);
    }
    
    callback_result = write((uint8_t*)&request->type, sizeof(request->type));
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, false);
    }
    
    if (payload_size > 0)
    {
        callback_result = write((uint8_t*)&request->data, payload_size);
        if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
        {
            return callback_to_protocol_result(callback_result, false);
        }
    }
    
    uint16_t checksum = checksum_init();
    checksum = checksum_append(checksum, &payload_size, sizeof(payload_size));
    checksum = checksum_append(checksum, &request->type, sizeof(request->type));
    checksum = checksum_append(checksum, &request->data, payload_size);
    
    callback_result = write((uint8_t*)&checksum, sizeof(checksum));
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, false);
    }
    
    return BRIDGE_PROTOCOL_RESULT_SUCCESS;
}

static bridge_protocol_result_t request_make(bridge_read_callback_t read, 
                                             bridge_write_callback_t write,
                                             const bridge_request_t * request, 
                                             bridge_answer_t * out_answer)
{
    bridge_protocol_result_t protocol_result;
    
    protocol_result = request_write(write, request);
    if (protocol_result != BRIDGE_PROTOCOL_RESULT_SUCCESS)
    {
        return protocol_result;
    }
    
    return answer_read(read, out_answer, request->type);
}

static bridge_protocol_result_t answer_write(bridge_write_callback_t write, 
                                             const bridge_answer_t * answer, 
                                             bridge_request_type_t request_type)
{
    bridge_callback_result_t callback_result;
    
    uint16_t payload_size = answer_payload_size_get(request_type, answer->type);
    
    callback_result = write((uint8_t*)&payload_size, sizeof(payload_size));
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, false);
    }
    
    callback_result = write((uint8_t*)&answer->type, sizeof(answer->type));
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, false);
    }
    
    if (payload_size > 0)
    {
        callback_result = write((uint8_t*)&answer->data, payload_size);
        if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
        {
            return callback_to_protocol_result(callback_result, false);
        }
    }
    
    uint16_t checksum = checksum_init();
    checksum = checksum_append(checksum, &payload_size, sizeof(payload_size));
    checksum = checksum_append(checksum, &answer->type, sizeof(answer->type));
    checksum = checksum_append(checksum, &answer->data, payload_size);
    
    callback_result = write((uint8_t*)&checksum, sizeof(checksum));
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, false);
    }
    
    return BRIDGE_PROTOCOL_RESULT_SUCCESS;
}

bridge_protocol_result_t bridge_protocol_recover(bridge_read_callback_t read, 
                                                 uint32_t timeout_ms)
{
    bridge_callback_result_t read_result;
    
    if (timeout_ms < BRIDGE_PROTOCOL_RECOVER_TIMEOUT_MS)
    {
        return BRIDGE_PROTOCOL_RESULT_TIMEOUT;
    }
    
    uint32_t waited_ms = 0;    
    do
    {
        uint8_t dummy;
        read_result = read(&dummy, BRIDGE_PROTOCOL_RECOVER_TIMEOUT_MS);
        
        waited_ms += BRIDGE_PROTOCOL_RECOVER_TIMEOUT_MS;
        
        if ((read_result == BRIDGE_CALLBACK_RESULT_SUCCESS) && (waited_ms >= timeout_ms))
        {
            return BRIDGE_PROTOCOL_RESULT_TIMEOUT;
        }
    } while (read_result == BRIDGE_CALLBACK_RESULT_SUCCESS);
    
    if (read_result == BRIDGE_CALLBACK_RESULT_READ_TIMEOUT)
    {
        return BRIDGE_PROTOCOL_RESULT_SUCCESS;
    }
    
    return callback_to_protocol_result(read_result, false);
}

bridge_protocol_result_t bridge_protocol_request_read(bridge_read_callback_t read, 
                                                      uint32_t first_byte_timeout_ms, 
                                                      bridge_request_t * out_request)
{
    bridge_callback_result_t callback_result;
    
    uint16_t payload_size;
    bool timeout_is_on_first_byte;
    callback_result = multiple_bytes_read(read, 
                                          first_byte_timeout_ms, 
                                          &payload_size, 
                                          sizeof(payload_size), 
                                          &timeout_is_on_first_byte);
    
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, !timeout_is_on_first_byte);
    }
    
    callback_result = multiple_bytes_read(read, 
                                          BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS, 
                                          &out_request->type, 
                                          sizeof(out_request->type), 
                                          NULL);
    
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, true);
    }
    
    if (request_payload_size_get(out_request->type) != payload_size)
    {
        return BRIDGE_PROTOCOL_RESULT_CORRUPTED;
    }
    
    if (payload_size > 0)
    {
        callback_result = multiple_bytes_read(read, 
                                              BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS, 
                                              &out_request->data, 
                                              payload_size, 
                                              NULL);
        
        if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
        {
            return callback_to_protocol_result(callback_result, true);
        }
    }
    
    uint16_t checksum;
    callback_result = multiple_bytes_read(read, 
                                          BRIDGE_PROTOCOL_BETWEEN_BYTES_TIMEOUT_MS, 
                                          &checksum, 
                                          sizeof(checksum), 
                                          NULL);
    
    if (callback_result != BRIDGE_CALLBACK_RESULT_SUCCESS)
    {
        return callback_to_protocol_result(callback_result, true);
    }
    
    uint16_t checksum_calculated = checksum_init();
    checksum_calculated = checksum_append(checksum_calculated, &payload_size, sizeof(payload_size));
    checksum_calculated = checksum_append(checksum_calculated, &out_request->type, sizeof(out_request->type));
    checksum_calculated = checksum_append(checksum_calculated, &out_request->data, payload_size);
    
    if (checksum != checksum_calculated)
    {
        return BRIDGE_PROTOCOL_RESULT_CORRUPTED;
    }
    
    return BRIDGE_PROTOCOL_RESULT_SUCCESS;
}

bridge_protocol_result_t bridge_protocol_match_protocol_version_answer(bridge_write_callback_t write)
{
    bridge_answer_t answer;
    answer.type = BRIDGE_ANSWER_TYPE_SUCCESS;
    answer.data.match_protocol_version.protocol_version = BRIDGE_PROTOCOL_VERSION;
    
    return answer_write(write, &answer, BRIDGE_REQUEST_TYPE_MATCH_PROTOCOL_VERSION);
}

bridge_protocol_result_t bridge_protocol_get_device_info_answer(bridge_write_callback_t write,
                                                                const device_info_t * info)
{
    bridge_answer_t answer;
    answer.type = BRIDGE_ANSWER_TYPE_SUCCESS;
    answer.data.get_device_info.info = *info;
    
    return answer_write(write, &answer, BRIDGE_REQUEST_TYPE_GET_DEVICE_INFO);
}

bridge_protocol_result_t bridge_protocol_match_protocol_version(bridge_read_callback_t read, 
                                                                bridge_write_callback_t write,
                                                                uint16_t * protocol_version)
{
    bridge_protocol_result_t protocol_result;
    
    bridge_request_t request;
    request.type = BRIDGE_REQUEST_TYPE_MATCH_PROTOCOL_VERSION;
    request.data.match_protocol_version.protocol_version = BRIDGE_PROTOCOL_VERSION;
    
    bridge_answer_t answer;
    protocol_result = request_make(read, write, &request, &answer);
    if (protocol_result == BRIDGE_PROTOCOL_RESULT_SUCCESS)
    {
        *protocol_version = answer.data.match_protocol_version.protocol_version;
    }
    
    return protocol_result;
}

bridge_protocol_result_t bridge_protocol_get_device_info(bridge_read_callback_t read,
                                                         bridge_write_callback_t write,
                                                         device_info_t * info)
{
    bridge_protocol_result_t protocol_result;
    
    bridge_request_t request;
    request.type = BRIDGE_REQUEST_TYPE_GET_DEVICE_INFO;
    
    bridge_answer_t answer;
    protocol_result = request_make(read, write, &request, &answer);
    if (protocol_result == BRIDGE_PROTOCOL_RESULT_SUCCESS)
    {
        *info = answer.data.get_device_info.info;
    }
    
    return protocol_result;
}
