#ifndef SENSOR_VALUES_H
#define SENSOR_VALUES_H
#include <stdint.h>

constexpr uint16_t dht22_humidity(const uint8_t *frame) {
    return static_cast<uint16_t>((frame[0] << 8) | frame[1]);
}
constexpr int16_t dht22_temperature(const uint8_t *frame) {
    return static_cast<int16_t>(((frame[2] & 0x7f) << 8 | frame[3]) *
                                ((frame[2] & 0x80) ? -1 : 1));
}
constexpr bool dht22_checksum_ok(const uint8_t *frame) {
    return static_cast<uint8_t>(frame[0] + frame[1] + frame[2] + frame[3]) == frame[4];
}
constexpr bool dht22_range_ok(const uint8_t *frame) {
    return dht22_humidity(frame) <= 1000 && dht22_temperature(frame) >= -400 &&
           dht22_temperature(frame) <= 800;
}
// Relative brightness for Wokwi's LDR module: lower ADC voltage means brighter.
// Rounded percentage of the inverted ADC scale, not calibrated lux.
constexpr uint16_t ldr_percent(uint16_t raw) {
    return static_cast<uint16_t>(((4095U - (raw > 4095U ? 4095U : raw)) * 100U + 2047U) / 4095U);
}
#endif
