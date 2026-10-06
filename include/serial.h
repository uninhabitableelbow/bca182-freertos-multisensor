#ifndef SERIAL_H
#define SERIAL_H

#include "stm32f1xx_hal.h"

// USART1: PA9 TX, PA10 RX, 115200 baud, 8 data bits, no parity, 1 stop bit.
HAL_StatusTypeDef serial_init(void);
HAL_StatusTypeDef serial_write(const char *message);

#endif
