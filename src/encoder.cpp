#include "encoder.h"
#include "navigation.h"
#include "stm32f1xx_hal.h"

namespace {
volatile uint32_t position = 0;
volatile uint32_t interrupt_count = 0;
volatile bool armed = false;
#ifdef WOKWI_FREERTOS_PORT
EncoderDecoder decoder;
bool initialized = false;
#endif
uint8_t pins() {
    const uint32_t value = GPIOA->IDR;
    return ((value & GPIO_PIN_2) ? 2 : 0) | ((value & GPIO_PIN_3) ? 1 : 0);
}
}

void encoder_init() {
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_2;
#ifdef WOKWI_FREERTOS_PORT
    gpio.Mode = GPIO_MODE_INPUT;
#else
    gpio.Mode = GPIO_MODE_IT_FALLING;
#endif
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
    HAL_NVIC_DisableIRQ(EXTI3_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2 | GPIO_PIN_3);
#ifdef WOKWI_FREERTOS_PORT
    // No encoder exceptions in the simulator. Sample at existing safe points.
    CLEAR_BIT(EXTI->IMR, GPIO_PIN_2 | GPIO_PIN_3);
    decoder.reset(pins());
    initialized = true;
#else
    HAL_NVIC_SetPriority(EXTI2_IRQn, 6, 0);
    HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
    armed = true;
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
#endif
}

extern "C" void encoder_poll(void) {
#ifdef WOKWI_FREERTOS_PORT
    if (!initialized) { return; }
    const int step = decoder.update(pins());
    if (step > 0) { ++position; }
    else if (step < 0) { --position; }
#endif
}

uint32_t encoder_position() {
#ifdef WOKWI_FREERTOS_PORT
    encoder_poll();
#else
    // Called once per 10-ms InputTask iteration. Keep the IRQ disabled through
    // the rest of the pulse and contact bounce, then rearm only at idle.
    if (!armed && pins() == 3) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
        HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
        armed = true;
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    }
#endif
    // Aligned 32-bit load is atomic on Cortex-M3; ISR is the only writer.
    return position;
}
uint32_t encoder_interrupt_count() { return interrupt_count; }

extern "C" void EXTI2_IRQHandler() {
    // Mask immediately, even on a spurious entry, to bound interrupt work.
    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
    ++interrupt_count;
    if (armed && __HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2)) {
        // Wokwi KY-040: DT high at CLK falling = clockwise.
        if (GPIOA->IDR & GPIO_PIN_3) { ++position; }
        else { --position; }
    }
    armed = false;
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
    HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
    __DSB();
}
