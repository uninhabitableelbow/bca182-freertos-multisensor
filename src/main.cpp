#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

static void Error_Handler(void) {
    while (1) {
    }
}

static void MX_USART1_UART_Init(void) {
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

extern "C" int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t *>(ptr), len, HAL_MAX_DELAY);
    return len;
}

int main(void) {
    HAL_Init();
    MX_USART1_UART_Init();

    printf("BCA182 FreeRTOS Multisensor\r\n");
    printf("System starting...\r\n");

    while (1) {
        HAL_Delay(1000);
    }
}