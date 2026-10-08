#include "diagnostics.h"
#include "rtos_objects.h"
#include "stm32f1xx_hal.h"
#include "serial.h"
#include "task.h"
#include <cstdio>

namespace app_runtime {
constexpr TickType_t diagnostic_period = pdMS_TO_TICKS(1000);
[[noreturn]] void fail_stop() {
    __disable_irq();
    for (;;) { __NOP(); }
}

// Protect USART1, its shared HAL handle, and each complete diagnostic report.
// Optional detail and its newline belong to the same protected report.
void print_diagnostic(const char *message, const char *detail) {
    if (xSemaphoreTake(serial_mutex, portMAX_DELAY) != pdTRUE) {
        fail_stop();
    }
    HAL_StatusTypeDef status = serial_write(message);
    if (status == HAL_OK && detail != nullptr) {
        status = serial_write(detail);
        if (status == HAL_OK) { status = serial_write("\r\n"); }
    }
    const BaseType_t released = xSemaphoreGive(serial_mutex);
    if (status != HAL_OK) {
        __disable_irq();
        serial_write_fault("UART transmission failed\r\n");
        fail_stop();
    }
    if (released != pdTRUE) {
        __disable_irq();
        serial_write_fault("Serial mutex release failed\r\n");
        fail_stop();
    }
}

void task_a(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        print_diagnostic("Task A running\r\n");
        vTaskDelayUntil(&last_wake, diagnostic_period);
    }
}

void task_b(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    EventBits_t last_events = EVENT_MASK + 1;
    for (;;) {
        print_diagnostic("Task B running\r\n");
        // Observe persistent event levels without consuming another task's flags.
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) != pdTRUE) { fail_stop(); }
        const EventBits_t bits = xEventGroupGetBits(system_events) & EVENT_MASK;
        if (xSemaphoreGive(state_mutex) != pdTRUE) { fail_stop(); }
        if (bits != last_events) {
            char line[80];
            std::snprintf(line, sizeof(line), "Events: ACTIVE=%u MOTION=%u ALARM=%u\r\n",
                (bits & EVENT_ACTIVE) != 0 ? 1U : 0U,
                (bits & EVENT_MOTION) != 0 ? 1U : 0U,
                (bits & EVENT_ALARM) != 0 ? 1U : 0U);
            print_diagnostic(line);
            last_events = bits;
        }
        vTaskDelayUntil(&last_wake, diagnostic_period);
    }
}
}

using app_runtime::fail_stop;
extern "C" void vApplicationMallocFailedHook(void) {
    __disable_irq();
    serial_write_fault("FreeRTOS heap allocation failed\r\n");
    fail_stop();
}

#ifdef WOKWI_FREERTOS_PORT
extern "C" void vApplicationIdleHook(void) {
    // Return to the cooperative kernel's Idle loop, which yields each pass.
    // The Wokwi port samples TIM3 at safe yields instead of using a tick IRQ.
    // Stay awake so time advances while application tasks are blocked.
}
#endif

extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char *) {
    __disable_irq();
    serial_write_fault("FreeRTOS task stack overflow\r\n");
    fail_stop();
}

extern "C" void rtos_assert_failed(const char *condition, const char *file,
                                    unsigned int line) {
    __disable_irq();
    serial_write_fault("FreeRTOS assertion failed: ");
    serial_write_fault(condition);
    serial_write_fault("\r\nLocation: ");
    serial_write_fault(file);
    serial_write_fault(":");
    char digits[11] = {};
    unsigned int position = sizeof(digits) - 1;
    do {
        digits[--position] = '0' + line % 10;
        line /= 10;
    } while (line != 0);
    serial_write_fault(&digits[position]);
    serial_write_fault("\r\n");
    fail_stop();
}
