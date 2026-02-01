#ifndef PACKET_PROTOCOL_H__
#define PACKET_PROTOCOL_H__
#define  ZETTA_VERSION "0.0.1"
#include <stdint.h>
#include <string.h>

#define START_BYTE 0xAA
#define STOP_BYTE 0xBC
#define MAX_PAYLOAD_SIZE 25
#define USE_HARDWARE_CRC 1

typedef struct Zetta_t Zetta_t;
// This represents the raw structure on the wire
#pragma pack(push, 1)
typedef struct
{
    uint8_t start;
    uint8_t type;
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t crc;
    uint8_t stop;
} ZettaFrame_t;
#pragma pack(pop)
#define MAX_ZETTA_FRAME_SIZE (sizeof(ZettaFrame_t))

typedef enum __attribute__((packed))
{
    MSG_ACK = 0,
    MSG_PUBLISH = 1,
    MSG_SUBSCRIBE = 2,
} ZettaPacketType_t;
typedef enum
{
    STATE_RX_WAIT_START,
    STATE_RX_GET_TYPE,
    STATE_RX_GET_LEN,
    STATE_RX_GET_PAYLOAD,
    STATE_RX_GET_CRC,
    STATE_RX_GET_STOP,
} ZettaFrameRxState_t;

typedef enum
{
    ZETTA_ERROR = 0,
    ZETTA_OK,
    ZETTA_ERROR_TYPE,
    ZETTA_FRAME_ERROR,
    ZETTA_ERROR_INVALID_START,
    ZETTA_ERROR_PAYLOAD_TOO_LARGE,
    ZETTA_ERROR_CRC_MISMATCH,
    ZETTA_ERROR_INVALID_STOP,
    ZETTA_ERROR_TIMEOUT,
    ZETTA_ERROR_TX_BUSY,
    ZETTA_ERROR_RX_BUSY,
} ZettaError_t;

typedef enum
{
    ZETTA_STATE_TX_BUSY,
    ZETTA_STATE_TX_READY,
    ZETTA_STATE_RX_READY,
    ZETTA_STATE_RX_BUSY,

} ZettaProtocolState_t;
// packet context
typedef void (*ZettaTransmit)(void* data, uint8_t size);
typedef void (*ZettaRecieve)(void* data, uint8_t size);
typedef uint32_t (*ZettaComputeCRC)(uint32_t* data, uint32_t size);
typedef void (*ZettaTransmitCpltClbk)(Zetta_t* packet);
typedef void (*ZettaReceiveCpltClbk)(Zetta_t* packet);
typedef void (*HandleError)(Zetta_t* hzetta, ZettaError_t error);

typedef struct
{
    ZettaTransmit send;
    ZettaRecieve receive;
    ZettaComputeCRC computeCRC;
    ZettaReceiveCpltClbk rxCpltClbk;
    ZettaTransmitCpltClbk txCpltClbk;
    HandleError OnError;
} ZettaInterface_t;

typedef struct Zetta_t
{
    ZettaInterface_t interface;
    struct
    {
        ZettaProtocolState_t pstate;
        uint8_t index;
        ZettaFrame_t frame;
        uint32_t last_byte_time; // For timeout detection
        ZettaError_t error;
        ZettaFrameRxState_t rx_frame_state;
        uint8_t payload_ready ; 
    } _internal;

} Zetta_t;

// Function to pack data into a buffer
// Returns the actual number of bytes to send over UART
void zetta_init(Zetta_t* packet, ZettaInterface_t interface);
ZettaError_t zetta_ParseByte(Zetta_t* packet, uint8_t byte);
ZettaError_t zetta_ProcessBuffer(Zetta_t* packet, uint8_t* pData,
                                 uint16_t size);
void Zetta_GetPayload(Zetta_t* hzetta, void* pDest);
ZettaPacketType_t Zetta_GetType(Zetta_t* hzetta);
void zetta_recieve_cplt_clb(Zetta_t* packet) __attribute__((weak));
void zetta_transmit_cplt_clb(Zetta_t* packet) __attribute__((weak));
void zetta_error_manager(Zetta_t* packet, ZettaError_t error);
ZettaError_t zetta_send(Zetta_t* packet, ZettaPacketType_t type, void* pData,
                        uint8_t len);
#endif