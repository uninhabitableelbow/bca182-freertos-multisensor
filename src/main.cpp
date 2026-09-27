#include "stm32f1xx_hal.h"
#include <string.h>

UART_HandleTypeDef huart1;

void SystemClock_Config(void);
void MX_USART1_UART_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();

    // The mandatory assignment boot message
    const char *msg = "BCA182 FreeRTOS Multisensor \n System starting...\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    while (1) {
        // Stay here until we add FreeRTOS tasks in Part 3
    }
}

void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void SystemClock_Config(void) {
    // Basic internal clock configuration fallback
}

void app_main(void) {
    main();
}
