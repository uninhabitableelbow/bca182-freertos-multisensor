#include "ldr.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_adc_ex.h"

namespace {
ADC_HandleTypeDef adc = {};
bool initialized = false;
}

extern "C" bool ldr_init(void) {
    initialized = false;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    // 72 MHz / 6 = 12 MHz on hardware, within the ADC's 14 MHz limit.
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    adc.Instance = ADC1;
    adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc.Init.ContinuousConvMode = DISABLE;
    adc.Init.DiscontinuousConvMode = DISABLE;
    adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&adc) != HAL_OK) { return false; }
    ADC_ChannelConfTypeDef channel = {};
    channel.Channel = ADC_CHANNEL_0;
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&adc, &channel) != HAL_OK) { return false; }
#ifndef WOKWI_FREERTOS_PORT
    // Wokwi documents basic ADC1 conversion support, not calibration.
    if (HAL_ADCEx_Calibration_Start(&adc) != HAL_OK) { return false; }
#endif
    initialized = true;
    return true;
}

extern "C" bool ldr_read(uint16_t *raw) {
    if (!initialized || raw == nullptr) { return false; }
    if (HAL_ADC_Start(&adc) != HAL_OK) { return false; }
    const HAL_StatusTypeDef status = HAL_ADC_PollForConversion(&adc, 5);
    const uint32_t value = status == HAL_OK ? HAL_ADC_GetValue(&adc) : 0U;
    const HAL_StatusTypeDef stopped = HAL_ADC_Stop(&adc);
    if (status != HAL_OK || stopped != HAL_OK || value > 4095U) { return false; }
    *raw = static_cast<uint16_t>(value);
    return true;
}
