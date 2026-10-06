#include "app.h"
#include "stm32f1xx_hal.h"
#include "serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

namespace {
SemaphoreHandle_t serial_mutex = nullptr;
constexpr uint16_t task_stack_words = 256; // 1 KiB per task on Cortex-M3.
constexpr TickType_t diagnostic_period = pdMS_TO_TICKS(1000);

[[noreturn]] void fail_stop() {
    __disable_irq();
    for (;;) { __NOP(); }
}

void print_diagnostic(const char *message) {
    if (xSemaphoreTake(serial_mutex, portMAX_DELAY) != pdTRUE) {
        fail_stop();
    }
    const HAL_StatusTypeDef status = serial_write(message);
    const BaseType_t released = xSemaphoreGive(serial_mutex);
    if (status != HAL_OK || released != pdTRUE) {
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
    for (;;) {
        print_diagnostic("Task B running\r\n");
        vTaskDelayUntil(&last_wake, diagnostic_period);
    }
}
} // namespace

extern "C" void vApplicationMallocFailedHook(void) {
    fail_stop();
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char *) {
    fail_stop();
}

/**
 * @brief Application main entry point.
 *        Runs after the HAL and system clock are initialized in main().
 *
 *        Part III starts two periodic tasks after the serial startup message.
 */
extern "C" void app_main(void) {
    if (serial_init() != HAL_OK ||
        serial_write("BCA182 FreeRTOS Multisensor\r\nSystem starting...\r\n") != HAL_OK) {
        fail_stop();
    }

    serial_mutex = xSemaphoreCreateMutex();
    if (serial_mutex == nullptr) {
        fail_stop();
    }

    if (xTaskCreate(task_a, "TaskA", task_stack_words, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(task_b, "TaskB", task_stack_words, nullptr, 1, nullptr) != pdPASS) {
        fail_stop();
    }

    vTaskStartScheduler();
    // A successful scheduler start does not return.
    fail_stop();
}
