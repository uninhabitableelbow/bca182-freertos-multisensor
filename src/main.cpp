#include "stm32f1xx_hal.h"

int main(void) {
    // Initialize the low-level Hardware Abstraction Layer
    HAL_Init();
    
    // Infinite loop keeping the processor core alive
    while (1) {
        // Fallback safety spin lock
    }
}

// Map framework startup wrappers cleanly
void app_main(void) {
    main();
}
