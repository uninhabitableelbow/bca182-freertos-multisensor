#include "oled.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
I2C_HandleTypeDef bus = {};
constexpr uint16_t address = 0x3c << 1;
constexpr uint32_t timeout_ms = 30;
char error_text[112] = "none";
const char *stage = "initialization";
// DisplayTask owns both the hardware and this cache.
uint8_t displayed[8][128] = {};
bool page_valid[8] = {};

// Compact 5-column, 7-pixel glyphs for the initial temperature screen.
struct Glyph { char character; uint8_t columns[5]; };
constexpr Glyph font[] = {
    {'%', {35,19,8,100,98}}, {'H', {127,8,8,8,127}},
    {'L', {127,64,64,64,64}}, {'D', {127,65,65,34,28}},
    {'b', {127,72,68,68,56}}, {'c', {56,68,68,68,32}},
    {'d', {56,68,68,72,127}}, {'h', {127,8,4,4,120}},
    {'l', {0,65,127,64,0}}, {'v', {28,32,64,32,28}},
    {'y', {12,80,80,80,60}},
    {' ', {0,0,0,0,0}}, {'-', {8,8,8,8,8}}, {'.', {0,96,96,0,0}},
    {'0', {62,81,73,69,62}}, {'1', {0,66,127,64,0}},
    {'2', {66,97,81,73,70}}, {'3', {33,65,69,75,49}},
    {'4', {24,20,18,127,16}}, {'5', {39,69,69,69,57}},
    {'6', {60,74,73,73,48}}, {'7', {1,113,9,5,3}},
    {'8', {54,73,73,73,54}}, {'9', {6,73,73,41,30}},
    {'C', {62,65,65,65,34}}, {'E', {127,73,73,73,65}},
    {'I', {0,65,127,65,0}}, {'M', {127,2,12,2,127}},
    {'N', {127,4,8,16,127}}, {'O', {62,65,65,65,62}},
    {'R', {127,9,25,41,70}}, {'T', {1,1,127,1,1}},
    {'W', {63,64,56,64,63}},
    {'a', {32,84,84,84,120}}, {'e', {56,84,84,84,24}},
    {'g', {12,82,82,82,62}}, {'i', {0,68,125,64,0}},
    {'m', {124,4,24,4,120}}, {'n', {124,8,4,4,120}},
    {'o', {56,68,68,68,56}}, {'p', {124,20,20,20,8}},
    {'r', {124,8,4,4,8}}, {'t', {4,63,68,64,32}},
    {'u', {60,64,64,32,124}}
};

bool send(uint8_t *bytes, uint16_t size) {
    const HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(&bus, address, bytes, size, timeout_ms);
    if (result == HAL_OK) { return true; }
    std::snprintf(error_text, sizeof(error_text),
        "%s: status=%u error=0x%lX remaining=%u SR1=0x%lX SR2=0x%lX",
        stage, static_cast<unsigned>(result), static_cast<unsigned long>(bus.ErrorCode),
        static_cast<unsigned>(bus.XferCount), static_cast<unsigned long>(bus.Instance->SR1),
        static_cast<unsigned long>(bus.Instance->SR2));
    return false;
}
}

const char *oled_error() { return error_text; }

bool oled_line(unsigned page, const char *text) {
    if (page >= 8 || text == nullptr) { return false; }
    uint8_t pixels[129] = {0x40};
    unsigned column = 0;
    while (*text && column + 6 <= 128) {
        const Glyph *selected = nullptr;
        for (const auto &glyph : font) {
            if (glyph.character == *text) { selected = &glyph; break; }
        }
        if (selected != nullptr) {
            for (unsigned i = 0; i < 5; ++i) { pixels[1 + column + i] = selected->columns[i]; }
        }
        column += 6;
        ++text;
    }
    unsigned first = 0;
    unsigned end = 128;
    if (page_valid[page]) {
        while (first < end && displayed[page][first] == pixels[first + 1]) { ++first; }
        if (first == end) { return true; }
        while (end > first && displayed[page][end - 1] == pixels[end]) { --end; }
    }
    uint8_t position[] = {0x00, static_cast<uint8_t>(0xb0 | page),
        static_cast<uint8_t>(first & 15), static_cast<uint8_t>(0x10 | (first >> 4))};
    // On any partial failure, the next attempt must resend the complete row.
    page_valid[page] = false;
    stage = "row address";
    if (!send(position, sizeof(position))) { return false; }
    // Include newly blank pixels so shorter values still erase old digits.
    stage = "row pixels";
    for (unsigned offset = first; offset < end; offset += 16) {
        const unsigned count = end - offset < 16 ? end - offset : 16;
        uint8_t chunk[17] = {0x40};
        std::memcpy(chunk + 1, pixels + 1 + offset, count);
        if (!send(chunk, static_cast<uint16_t>(count + 1))) { return false; }
    }
    std::memcpy(displayed[page], pixels + 1, 128);
    page_valid[page] = true;
    return true;
}

bool oled_init() {
    std::memset(page_valid, 0, sizeof(page_valid));
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    // Match diagram.json explicitly: I2C1 default pins PB6/PB7, not PB8/PB9.
    __HAL_AFIO_REMAP_I2C1_DISABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    // Reset software state too: an aborted transfer may retain HAL's lock/state.
    bus = {};
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_NOPULL; // External 4.7k pull-ups to 3.3 V.
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    bus.Instance = I2C1;
    bus.Init.ClockSpeed = 100000;
    bus.Init.DutyCycle = I2C_DUTYCYCLE_2;
    bus.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    bus.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    bus.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    bus.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&bus) != HAL_OK) {
        std::snprintf(error_text, sizeof(error_text), "HAL I2C initialization failed");
        return false;
    }
    if (HAL_I2C_IsDeviceReady(&bus, address, 2, timeout_ms) != HAL_OK) {
        std::snprintf(error_text, sizeof(error_text),
            "No OLED ACK at 0x3C on PB6/PB7; check loaded circuit. MAPR=0x%lX GPIOB=0x%lX",
            static_cast<unsigned long>(AFIO->MAPR),
            static_cast<unsigned long>(GPIOB->IDR));
        return false;
    }
    uint8_t setup[] = {
        0x00, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40,
        0x8d, 0x14, 0x20, 0x02, 0xa1, 0xc8, 0xda, 0x12,
        0x81, 0x7f, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0x2e
    };
    stage = "setup commands";
    if (!send(setup, sizeof(setup))) { return false; }
    for (unsigned page = 0; page < 8; ++page) {
        if (!oled_line(page, "")) { return false; }
    }
    uint8_t enable[] = {0x00, 0xaf};
    stage = "display enable";
    return send(enable, sizeof(enable));
}
