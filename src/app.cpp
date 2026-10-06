#include "app.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Application main entry point.
 *        Runs after the HAL and system clock are initialized in main().
 *
 *        PART I does not add application logic yet: the empty project must
 *        compile before sensors or FreeRTOS tasks are introduced
 *        (laboratory PART I, step 14 - "First Build").
 */
extern "C" void app_main(void) {
    // Part I has no application peripherals. Sleep between interrupts.
    // FreeRTOS tasks start in Part III.
    while (1) {
        __WFI();
    }
}
