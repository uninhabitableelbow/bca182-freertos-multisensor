#ifndef DHT22_H
#define DHT22_H
#include <stdint.h>

struct Dht22Reading {
    int16_t temperature_tenths;
    uint16_t humidity_tenths;
};
enum class Dht22Status {
    ok, not_ready, timeout, checksum, range,
    bus_stuck_low, timer_error, response_timeout, data_timeout
};

// Single owner: SensorTask. PA1 data, TIM2 free-running at 1 MHz.
extern "C" bool dht22_init(void);
// Remaining stabilization/minimum transaction interval; SensorTask may block.
extern "C" uint32_t dht22_ready_in_ms(void);
extern "C" Dht22Status dht22_read(Dht22Reading *reading);
const char *dht22_status_text(Dht22Status status);
#endif
