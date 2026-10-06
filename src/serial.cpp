#include "serial.h"
#include <cstring>

namespace {
UART_HandleTypeDef uart = {};
constexpr uint32_t transmit_timeout_ms = 100;
}

// HAL_UART_Init calls this override to configure the peripheral pins/clocks.
extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *handle) {
    if (handle->Instance != USART1) {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

HAL_StatusTypeDef serial_init(void) {
    uart.Instance = USART1;
    uart.Init.BaudRate = 115200;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    return HAL_UART_Init(&uart);
}

HAL_StatusTypeDef serial_write(const char *message) {
    if (message == nullptr) {
        return HAL_ERROR;
    }
    const size_t length = std::strlen(message);
    if (length > UINT16_MAX) {
        return HAL_ERROR;
    }
    if (length == 0) {
        return HAL_OK;
    }
    // The polling HAL API takes a mutable pointer but only reads TX data.
    return HAL_UART_Transmit(&uart,
        reinterpret_cast<uint8_t *>(const_cast<char *>(message)),
        static_cast<uint16_t>(length), transmit_timeout_ms);
}

void serial_write_fault(const char *message) {
    if (message == nullptr || uart.Instance != USART1) {
        return;
    }
    while (*message != '\0') {
        uint32_t attempts = 100000;
        while ((USART1->SR & USART_SR_TXE) == 0 && attempts > 0) {
            --attempts;
        }
        if (attempts == 0) {
            return;
        }
        USART1->DR = static_cast<uint8_t>(*message++);
    }
}
