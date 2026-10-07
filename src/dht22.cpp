#include "dht22.h"
#include "sensor_values.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"

namespace {
TIM_HandleTypeDef timer = {};
bool initialized = false;
uint32_t previous_start_ms = 0;

void data_pin_mode(uint32_t mode) {
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = mode;
    // Keep the receive line high with the internal and external 4.7k pull-ups.
    gpio.Pull = mode == GPIO_MODE_INPUT ? GPIO_PULLUP : GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}

uint16_t micros16() { return static_cast<uint16_t>(__HAL_TIM_GET_COUNTER(&timer)); }
bool delay_us(uint16_t interval) {
    const uint16_t start = micros16();
    // Iteration cap also bounds failure if the timer clock stops.
    for (uint32_t attempts = 0; attempts < 100000U; ++attempts) {
        if (static_cast<uint16_t>(micros16() - start) >= interval) { return true; }
    }
    return false;
}
bool wait_level(GPIO_PinState level) {
    const uint16_t start = micros16();
    for (uint32_t attempts = 0; attempts < 10000U; ++attempts) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == level) { return true; }
        if (static_cast<uint16_t>(micros16() - start) >= 120U) { return false; }
    }
    return false;
}
}

extern "C" bool dht22_init(void) {
    initialized = false;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    data_pin_mode(GPIO_MODE_INPUT);

    uint32_t clock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) { clock *= 2U; }
    if (clock < 1000000U || clock % 1000000U != 0U) { return false; }
    timer.Instance = TIM2;
    timer.Init.Prescaler = clock / 1000000U - 1U;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.Period = 0xffffU;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&timer) != HAL_OK || HAL_TIM_Base_Start(&timer) != HAL_OK) {
        return false;
    }
    previous_start_ms = HAL_GetTick();
    initialized = true;
    return true;
}

extern "C" Dht22Status dht22_read(Dht22Reading *reading) {
    if (!initialized || reading == nullptr) { return Dht22Status::not_ready; }
    const uint32_t now = HAL_GetTick();
    if (static_cast<uint32_t>(now - previous_start_ms) < 2000U) {
        return Dht22Status::not_ready;
    }
    previous_start_ms = now;
    uint8_t frame[5] = {};
    if (!wait_level(GPIO_PIN_SET)) { return Dht22Status::bus_stuck_low; }
    // Preload low before enabling output for the host's start pulse.
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    data_pin_mode(GPIO_MODE_OUTPUT_PP);
    const bool start_ok = delay_us(1100);
    // Explicitly release the bus. Do not rely on open-drain output readback
    // in the simulator. Stay in input mode for the response and all 40 bits.
    data_pin_mode(GPIO_MODE_INPUT);
    if (!start_ok) { return Dht22Status::timer_error; }
    if (!wait_level(GPIO_PIN_RESET) ||
        !wait_level(GPIO_PIN_SET) || !wait_level(GPIO_PIN_RESET)) {
        return Dht22Status::response_timeout;
    }
    // Bounded, timing-sensitive acquisition. IRQs stay enabled; SensorTask
    // has higher priority than the diagnostic tasks on physical hardware.
    for (unsigned bit = 0; bit < 40; ++bit) {
        if (!wait_level(GPIO_PIN_SET)) { return Dht22Status::data_timeout; }
        const uint16_t rise = micros16();
        if (!wait_level(GPIO_PIN_RESET)) { return Dht22Status::data_timeout; }
        const uint16_t high_us = static_cast<uint16_t>(micros16() - rise);
        frame[bit / 8] = static_cast<uint8_t>((frame[bit / 8] << 1) | (high_us > 50U));
    }
    if (!dht22_checksum_ok(frame)) { return Dht22Status::checksum; }
    if (!dht22_range_ok(frame)) { return Dht22Status::range; }
    // Publish only a complete, validated frame; failures leave output untouched.
    reading->temperature_tenths = dht22_temperature(frame);
    reading->humidity_tenths = dht22_humidity(frame);
    return Dht22Status::ok;
}

const char *dht22_status_text(Dht22Status status) {
    switch (status) {
    case Dht22Status::ok: return "OK";
    case Dht22Status::not_ready: return "not ready";
    case Dht22Status::timeout: return "timeout (check wiring)";
    case Dht22Status::checksum: return "checksum error";
    case Dht22Status::range: return "out-of-range data";
    case Dht22Status::bus_stuck_low: return "data line stuck low (check pull-up/wiring)";
    case Dht22Status::timer_error: return "timing clock not advancing";
    case Dht22Status::response_timeout: return "no sensor response after start pulse";
    case Dht22Status::data_timeout: return "sensor data pulse timed out";
    }
    return "unknown error";
}
