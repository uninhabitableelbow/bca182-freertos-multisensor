#ifndef LDR_H
#define LDR_H
#include <stdint.h>
// Single owner: SensorTask. PA0 / ADC1 channel 0, right-aligned 12-bit raw data.
extern "C" bool ldr_init(void);
extern "C" bool ldr_read(uint16_t *raw);
#endif
