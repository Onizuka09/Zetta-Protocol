# Zetta Protocol (v0.0.1)
**Zetta** is a lightweight, transport-agnostic packet protocol designed for embedded systems.

It provides a simple framing format on top of any byte-oriented transport such as **UART, SPI, I²C, USB CDC**, or even TCP streams.  
The protocol is implemented in **C for microcontrollers** and mirrored with a **Python API** for PC-side communication and testing.
> For now the library only tested with UART transport protocol and tested on STM32

> This project is currently **v0.0.1** and under active development.

> APIs and internal behavior may change.
---
## Features

- Transport independent (UART / SPI / I2C / etc.)
- Byte-wise RX state machine (safe for DMA & ISR usage)
- Configurable payload size
- Hardware or software CRC support
- Minimal memory footprint
- Python implementation for host-side usage
- Designed for bare-metal and RTOS systems
--- 
## Frame Format

Each packet transmitted on the wire follows this format:
```
+------------+------+-----+----------+-----+-----------+
| START BYTE | TYPE | LEN | PAYLOAD | CRC | STOP BYTE  |
+------------+------+-----+----------+-----+-----------+
|     0xAA   | 1B   | 1B  | 0–25 B  | 1B  | 0xBC       |
+------------+------+-----+----------+-----+-----------+
```
- **START BYTE**: Frame delimiter (default: `0xAA`)
- **TYPE**: Packet type (PUBLISH, SUBSCRIBE, ACK, ...)
- **LEN**: Payload length in bytes
- **PAYLOAD**: User data
- **CRC**: Integrity check (implementation-defined)
- **STOP BYTE**: Frame delimiter (default: `0xBC`)
## Packet Types

```c
typedef enum {
    MSG_ACK       = 0,
    MSG_PUBLISH   = 1,
    MSG_SUBSCRIBE = 2,
} ZettaPacketType_t;
``` 
--- 
## Usage 
### Using C (STM32)
- Transmitting data 
```C
#include "zetta/zetta_protocol.h"

/* Transport bindings */
void uart_send(void* data, uint8_t size)
{
    HAL_UART_Transmit_DMA(&huart2, data, size);
}

uint32_t crc_compute(uint32_t* data, uint32_t size)
{
    __HAL_CRC_DR_RESET(&hcrc);
    return HAL_CRC_Calculate(&hcrc, data, size);
}

/* Zetta instance */
Zetta_t zetta;

int main(void)
{
    ZettaInterface_t iface = {
        .send = uart_send,
        .computeCRC = crc_compute,
    };

    zetta_init(&zetta, iface);

    struct {
        uint8_t id;
        float value;
    } payload = {1, 23.5f};

    zetta_send(&zetta, MSG_PUBLISH, &payload, sizeof(payload));

    while (1)
    {
        // RX handled byte-by-byte in UART ISR
    }
}
```
- UART RX Callback 
```C
uint8_t rx_byte;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (zetta_ParseByte(&zetta, rx_byte) == ZETTA_OK)
    {
        uint8_t buffer[32];
        Zetta_GetPayload(&zetta, buffer);
        // process buffer
    }

    HAL_UART_Receive_DMA(&huart2, &rx_byte, 1);
}
``` 
### Python 
Take a look at the python example 