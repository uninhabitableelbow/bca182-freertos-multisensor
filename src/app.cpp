#include "app.h"
#include "stm32f1xx_hal.h"
#include "serial.h"

/**
 * @brief Application main entry point.
 *        Runs after the HAL and system clock are initialized in main().
 *
 *        Part II prints the initial Wokwi startup message using STM32 HAL.
 */
extern "C" void app_main(void) {
    if (serial_init() != HAL_OK ||
        serial_write("BCA182 FreeRTOS Multisensor\r\nSystem starting...\r\n") != HAL_OK) {
        // Initialization failure: stop before introducing application tasks.
        __disable_irq();
        while (1) { __NOP(); }
    }

    // FreeRTOS tasks are introduced in Part III.
    while (1) {
        __WFI();
    }
}
