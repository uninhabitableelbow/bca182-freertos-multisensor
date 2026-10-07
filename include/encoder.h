#pragma once
#include <cstdint>
void encoder_init();
uint32_t encoder_position();
uint32_t encoder_interrupt_count();

#ifdef __cplusplus
extern "C" {
#endif
void encoder_poll(void);
#ifdef __cplusplus
}
#endif
