#include "pir.h"
#include "stm32f1xx_hal.h"

void pir_init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &gpio);
}

bool pir_motion() {
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET;
}
