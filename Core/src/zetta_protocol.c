
#include "zetta_protocol.h"
#include <stdio.h>
#include <string.h>
// pstate Machine States
uint8_t tx_buf[MAX_ZETTA_FRAME_SIZE];
uint16_t buf_tx_size = 0;
uint32_t dbg_crc_val = 0;
static uint32_t zetta_compute_crc(Zetta_t* packet);

void zetta_init(Zetta_t* packet, ZettaInterface_t interface)
{

    // check for error
    memset(packet, 0, sizeof(Zetta_t));
    packet->_internal.pstate = ZETTA_STATE_RX_READY;
    packet->interface = interface;
    packet->_internal.payload_ready = 0;
    packet->_internal.error = ZETTA_OK ;
    packet->interface.rxCpltClbk = zetta_recieve_cplt_clb;
    packet->interface.txCpltClbk = zetta_transmit_cplt_clb;
    packet->interface.OnError = zetta_error_manager;
    return;
}

static uint32_t zetta_compute_crc(Zetta_t* packet)
{
    // type + len + payload size
    uint32_t crc = 0;
    uint32_t data_bytes =
        1 + 1 + packet->_internal.frame.len; // type + len + payload

    uint8_t* data_ptr = &packet->_internal.frame.type;

    crc = packet->interface.computeCRC((uint32_t*)data_ptr, data_bytes);
    // // 1. Reset the CRC DR register to Initial Value (0xFFFFFFFF)
    dbg_crc_val = crc;
    // // 2. Feed bytes one by one to avoid 32-bit alignment/padding issues
    // // Note: STM32G0 HAL handles byte-sized writes to the CRC register
    // // when InputDataFormat is set to CRC_INPUTDATA_FORMAT_BYTES
    // crc_val = HAL_CRC_Calculate(&hcrc, (uint32_t*)data_ptr, data_bytes);

    // for (uint16_t i = 0; i < len; i++)
    // {
    //     crc_val ^= data[i]; // Simple XOR CRC
    // }
    return crc;
}

ZettaError_t zetta_send(Zetta_t* packet, ZettaPacketType_t type, void* pData,
                        uint8_t len)
{
    while (packet->_internal.pstate == ZETTA_STATE_TX_BUSY)
    {
        // __NOP();
    }
    if (len > MAX_PAYLOAD_SIZE)
    {
        packet->interface.OnError(packet, ZETTA_ERROR_PAYLOAD_TOO_LARGE);
        return ZETTA_ERROR_PAYLOAD_TOO_LARGE;
    }

    buf_tx_size = 0;
    memset(tx_buf, 0, 30);
    packet->_internal.pstate = ZETTA_STATE_TX_BUSY;

    tx_buf[buf_tx_size++] = START_BYTE;
    tx_buf[buf_tx_size++] = type;
    tx_buf[buf_tx_size++] = len;
    memcpy(&tx_buf[buf_tx_size], pData, len);
    buf_tx_size += len;
    packet->_internal.frame.type = type ; 
    packet->_internal.frame.len = len ; 
    memcpy(packet->_internal.frame.payload, pData, len);
    packet->_internal.frame.crc = zetta_compute_crc(packet);
    tx_buf[buf_tx_size++] = packet->_internal.frame.crc ; 
    tx_buf[buf_tx_size++] = STOP_BYTE;

    // Hand it to hardware
    packet->interface.send(tx_buf, buf_tx_size);
    // TODO: Create a timout callback that after some time resets the packet
    return ZETTA_OK;
}
// TODO
static ZettaError_t zetta_check_type(uint8_t byte) { return ZETTA_OK; }

ZettaError_t zetta_ParseByte(Zetta_t* packet, uint8_t byte)
{
    while (packet->_internal.pstate == ZETTA_STATE_RX_BUSY)
    {
    }
    switch (packet->_internal.rx_frame_state)
    {
    case STATE_RX_WAIT_START:
        if (byte == START_BYTE)
        {
            packet->_internal.rx_frame_state = STATE_RX_GET_TYPE;
            packet->_internal.frame.start = START_BYTE;
            packet->_internal.index = 0;
        }
        else
        {
            packet->_internal.error = ZETTA_ERROR_INVALID_START;
            packet->interface.OnError(packet, ZETTA_ERROR_INVALID_START);
        }
        break;

    case STATE_RX_GET_TYPE:
        if (zetta_check_type(byte))
        {
            packet->_internal.frame.type = byte;
            packet->_internal.rx_frame_state = STATE_RX_GET_LEN;
        }
        else
        {
            packet->_internal.error = ZETTA_ERROR_TYPE;
            packet->interface.OnError(packet, ZETTA_ERROR_TYPE);
        }
        break;

    case STATE_RX_GET_LEN:
        if (byte <= MAX_PAYLOAD_SIZE)
        {
            packet->_internal.frame.len = byte;
            packet->_internal.rx_frame_state =
                (byte == 0) ? STATE_RX_GET_CRC : STATE_RX_GET_PAYLOAD;
        }
        else
        {
            packet->_internal.error = ZETTA_ERROR_PAYLOAD_TOO_LARGE;
            packet->interface.OnError(packet, ZETTA_ERROR_PAYLOAD_TOO_LARGE);

            return ZETTA_ERROR;
        }
        break;

    case STATE_RX_GET_PAYLOAD:
        packet->_internal.frame.payload[packet->_internal.index++] = byte;
        if (packet->_internal.index >= packet->_internal.frame.len)
        {
            packet->_internal.rx_frame_state = STATE_RX_GET_CRC;
        }
        break;

    case STATE_RX_GET_CRC:
        packet->_internal.frame.crc = byte;
        packet->_internal.rx_frame_state = STATE_RX_GET_STOP;
        break;

    case STATE_RX_GET_STOP:

        if (byte == STOP_BYTE)
        {
            packet->_internal.rx_frame_state = STATE_RX_WAIT_START;
            packet->_internal.frame.stop = STOP_BYTE;
            // Calculate CRC of received data to verify integrity
            uint32_t crc_val = zetta_compute_crc(packet);

            if (crc_val == packet->_internal.frame.crc)
            {
                packet->_internal.payload_ready = 1;
                packet->_internal.rx_frame_state = STATE_RX_WAIT_START ; 
                return ZETTA_OK; // Valid packet found!
            }
            else
            {
                packet->_internal.error = ZETTA_ERROR_CRC_MISMATCH;
                packet->interface.OnError(packet, ZETTA_ERROR_CRC_MISMATCH);
                // packet->_internal.rx_frame_state = STATE_ERROR_CRC;
            }
        }
        else
        {
            packet->_internal.error = ZETTA_ERROR_INVALID_STOP;
            packet->interface.OnError(packet, ZETTA_ERROR_INVALID_STOP);
            // packet->_internal.rx_frame_state = STATE_FRAME_ERROR;
        }
        break;

    default:
        packet->_internal.error = ZETTA_FRAME_ERROR;
        packet->interface.OnError(packet, ZETTA_FRAME_ERROR);
        break;
    }
    return ZETTA_ERROR;
}
ZettaError_t zetta_ProcessBuffer(Zetta_t* packet, uint8_t* pData, uint16_t size)

{
    for (uint16_t i = 0; i < size; i++)
    {
        if (zetta_ParseByte(packet, pData[i]) == ZETTA_OK)
        {
            return ZETTA_OK;
        }
    }
    return ZETTA_ERROR;
}
void Zetta_GetPayload(Zetta_t* hzetta, void* pDest)
{
    if (hzetta->_internal.payload_ready)
        memcpy(pDest, hzetta->_internal.frame.payload,
               hzetta->_internal.frame.len);
}

ZettaPacketType_t Zetta_GetType(Zetta_t* hzetta)
{

    if (hzetta->_internal.payload_ready)
        return hzetta->_internal.frame.type;
    // TODO Fix  this
    return MSG_ACK;
}
void zetta_error_manager(Zetta_t* packet, ZettaError_t error)
{
    packet->_internal.rx_frame_state = STATE_RX_WAIT_START;

    switch (error)
    {
    case ZETTA_ERROR:
        printf("Zetta error\n");
        /* code */
        break;

    default:
        break;
    }
}
void zetta_recieve_cplt_clb(Zetta_t* packet)
{
    packet->_internal.pstate = ZETTA_STATE_RX_READY;
}
void zetta_transmit_cplt_clb(Zetta_t* packet)
{
    packet->_internal.pstate = ZETTA_STATE_TX_READY;
}