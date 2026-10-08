#include "buzzer.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"

namespace {
TIM_HandleTypeDef timer = {};
constexpr uint32_t counter_hz = 1000000;
constexpr uint32_t tone_hz = 2000;
constexpr uint32_t period_counts = counter_hz / tone_hz;
}

bool buzzer_init() {
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_AFIO_REMAP_TIM4_DISABLE();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    uint32_t clock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) { clock *= 2; }
    if (clock < counter_hz || clock % counter_hz != 0U) { return false; }
    timer.Instance = TIM4;
    timer.Init.Prescaler = clock / counter_hz - 1;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.Period = period_counts - 1;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&timer) != HAL_OK) { return false; }
    TIM_OC_InitTypeDef output = {};
    output.OCMode = TIM_OCMODE_PWM1;
    output.Pulse = 0; // Silent at startup.
    output.OCPolarity = TIM_OCPOLARITY_HIGH;
    output.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&timer, &output, TIM_CHANNEL_3) != HAL_OK) { return false; }
    gpio.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOB, &gpio);
    return HAL_TIM_PWM_Start(&timer, TIM_CHANNEL_3) == HAL_OK;
}

void buzzer_set(bool enabled) {
    // Hardware produces the waveform; no interrupts or software tone loop.
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_3, enabled ? period_counts / 2 : 0);
}
