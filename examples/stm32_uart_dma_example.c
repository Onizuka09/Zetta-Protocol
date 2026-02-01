/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 Moktar SELLAMI.
 * All rights reserved.
 *
 ******************************************************************************
 */
#include "main.h"
#include "crc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

#include <zetta_protocol.h>
#include <string.h>

uint8_t rx_byte = 0;

void uart_stm32_send_dma(void *data, uint8_t size);
void uart_stm32_receive_dma(void *data, uint8_t size);
uint32_t stm32_crc(uint32_t *data, uint32_t size);

void uart_stm32_send_dma(void *data, uint8_t size)
{
    HAL_UART_Transmit_DMA(&huart2, data, size);
}
void uart_stm32_receive_dma(void *data, uint8_t size)
{
    HAL_UART_Receive_DMA(&huart2, data, size);
}
uint32_t stm32_crc(uint32_t *data, uint32_t size)
{
    __HAL_CRC_DR_RESET(&hcrc);
    return HAL_CRC_Calculate(&hcrc, (uint32_t *)data, size);
}

// user data begin
#pragma pack(push, 1)
struct MetricPacket
{
    uint32_t a;
    float b;
    uint8_t str[5];
};
#pragma pack(pop)

struct MetricPacket metbj = {0};

char test1[] = "hello world";
#pragma pack(push, 1)
struct MyStruct
{
    uint8_t test3[4];
    int age;
    float price;
};
#pragma pack(pop)
// end user data

Zetta_t hzettatx;
Zetta_t hzettarx;

// set zetta interface
ZettaInterface_t zetta1_interface = {
    .computeCRC = stm32_crc,
    .send = uart_stm32_send_dma,
    .receive = uart_stm32_receive_dma,
};
// UART Callbacks begin
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
    zetta_transmit_cplt_clb(&hzettatx);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (zetta_ParseByte(&hzettarx, rx_byte) == ZETTA_OK)
        Zetta_GetPayload(&hzettarx, &metbj);

    zetta_recieve_cplt_clb(&hzettarx);
    HAL_UART_Receive_DMA(&huart2, &rx_byte, 1);
}


int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    MX_CRC_Init();

    struct MyStruct test2;
    test2.age = 40;
    memcpy(test2.test3, "moka", 4);

    test2.price = 20.2f;
    // Transmition
    zetta_init(&hzettatx, zetta1_interface);
    zetta_send(&hzettatx, MSG_PUBLISH, &test2, sizeof(test2));
    zetta_send(&hzettatx, MSG_PUBLISH, test1, sizeof(test1));
    // reception
    zetta_init(&hzettarx, zetta1_interface);

    HAL_UART_Receive_DMA(&huart2, &rx_byte, 1);
    while (1)
    {
        HAL_Delay(1000);
    }
}
