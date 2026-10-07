// Pure conversion checks evaluated by the compiler; no simulated hardware.
#include "sensor_values.h"
constexpr uint8_t normal[] = {0x02, 0x64, 0x00, 0xfe, 0x64};
constexpr uint8_t negative[] = {0x01, 0xf4, 0x80, 0x36, 0xab};
constexpr uint8_t corrupt[] = {0x02, 0x64, 0x00, 0xfe, 0x65};
constexpr uint8_t excess_humidity[] = {0x03, 0xe9, 0x00, 0x00, 0xec};
static_assert(dht22_checksum_ok(normal), "Valid DHT22 checksum");
static_assert(dht22_humidity(normal) == 612, "61.2 percent humidity");
static_assert(dht22_temperature(normal) == 254, "25.4 C temperature");
static_assert(dht22_range_ok(normal), "Normal values in range");
static_assert(dht22_checksum_ok(negative), "Negative frame checksum");
static_assert(dht22_temperature(negative) == -54, "DHT22 uses sign-magnitude");
static_assert(!dht22_checksum_ok(corrupt), "Corrupted data rejected");
static_assert(!dht22_range_ok(excess_humidity), "Humidity cannot exceed 100 percent");
static_assert(ldr_percent(0) == 100, "Low ADC is bright");
static_assert(ldr_percent(4095) == 0, "Full ADC is dark");
static_assert(ldr_percent(2048) == 50, "Midscale rounds to 50 percent");
static_assert(ldr_percent(65535) == 0, "Out-of-range input cannot underflow");
