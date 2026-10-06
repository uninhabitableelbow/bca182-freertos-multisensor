#ifndef DHT22_H
#define DHT22_H
#include <stdint.h>

struct Dht22Reading {
    int16_t temperature_tenths;
    uint16_t humidity_tenths;
};
enum class Dht22Status { ok, not_ready, timeout, checksum, range };

// Single owner: SensorTask. PA1 data, TIM2 free-running at 1 MHz.
extern "C" bool dht22_init(void);
extern "C" Dht22Status dht22_read(Dht22Reading *reading);
const char *dht22_status_text(Dht22Status status);
#endif
