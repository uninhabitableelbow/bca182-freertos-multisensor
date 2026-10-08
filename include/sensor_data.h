#pragma once

#include "dht22.h"
#include <cstdint>
#include <type_traits>

// Part V measurement contract: Celsius, percent humidity, percent light.
struct SensorData {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
};

// Validity accompanies each sample so failures cannot look like measurements.
struct SensorMessage {
    SensorData values;
    Dht22Status dht_status;
    bool light_valid;
    bool motion_valid;
    uint16_t light_raw;
    uint32_t sequence;
    uint32_t dropped_samples;
    uint32_t state_epoch; // Reject samples acquired before a sleep/wake transition.
};

static_assert(std::is_trivially_copyable<SensorMessage>::value,
              "FreeRTOS queue items must be safe to copy by value");
