#include "stm32f1xx_hal.h"
#include <string.h>

UART_HandleTypeDef huart1;
void app_main(void);

int main(void) {
    /* CRITICAL FIX: Wokwi runs at 8 MHz HSI by default, but the STM32Cube HAL 
       defaults to 72 MHz. This mismatch causes UART baud rate calculation to fail,
       resulting in no serial output. We must update SystemCoreClock to 8 MHz. */
    SystemCoreClock = 8000000; 
    
    HAL_Init();
    app_main();
    for (;;) { }
}

void app_main(void) {
    /* Enable clocks for GPIOA and USART1 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* PA9 = USART1_TX */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_9;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PA10 = USART1_RX */
    gpio.Pin   = GPIO_PIN_10;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* USART1: 115200 8N1, TX-RX mode */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    /* Recognizable startup banner required by Step 16 */
    const char *line1 = "BCA182 FreeRTOS Multisensor\r\n";
    const char *line2 = "System starting...\r\n";
    
    /* Increased timeout to 1000ms just to be safe */
    HAL_UART_Transmit(&huart1, (uint8_t *)line1, strlen(line1), 1000);
    HAL_UART_Transmit(&huart1, (uint8_t *)line2, strlen(line2), 1000);

    /* Keep the system alive */
    while (1) {
        HAL_Delay(1000);
    }
}