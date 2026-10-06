#include "app.h"
#include "stm32f1xx_hal.h"
#include "serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <cstdio>

namespace {
SemaphoreHandle_t serial_mutex = nullptr;
constexpr uint16_t task_stack_words = 256; // 1 KiB per task on Cortex-M3.
constexpr TickType_t diagnostic_period = pdMS_TO_TICKS(250);

[[noreturn]] void fail_stop() {
    __disable_irq();
    for (;;) { __NOP(); }
}

void print_diagnostic(const char *message) {
    if (xSemaphoreTake(serial_mutex, portMAX_DELAY) != pdTRUE) {
        fail_stop();
    }
    char line[80];
    const unsigned long tick = static_cast<unsigned long>(xTaskGetTickCount());
    const int length = std::snprintf(line, sizeof(line), "%s [tick=%lu, time=%lu ms]\r\n",
        message, tick, static_cast<unsigned long>(HAL_GetTick()));
    if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) {
        fail_stop();
    }
    const HAL_StatusTypeDef status = serial_write(line);
    const BaseType_t released = xSemaphoreGive(serial_mutex);
    if (status != HAL_OK || released != pdTRUE) {
        fail_stop();
    }
}

void task_a(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        print_diagnostic("Task A running");
        vTaskDelayUntil(&last_wake, diagnostic_period);
    }
}

void task_b(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        print_diagnostic("Task B running");
        vTaskDelayUntil(&last_wake, diagnostic_period);
    }
}
} // namespace

extern "C" void vApplicationMallocFailedHook(void) {
    __disable_irq();
    serial_write_fault("FreeRTOS heap allocation failed\r\n");
    fail_stop();
}

#ifdef WOKWI_FREERTOS_PORT
extern "C" void vApplicationIdleHook(void) {
    // The cooperative kernel yields in its Idle loop before this hook.
    // TIM3 wakes the CPU; a task becoming Ready is selected on the next loop.
    __WFI();
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

    // Report priority-register readbacks for simulator diagnosis; restore the
    // word before task creation. The Wokwi port does not depend on this probe.
    volatile uint32_t *priority_word = reinterpret_cast<volatile uint32_t *>(0xe000e400UL);
    volatile uint8_t *priority_byte = reinterpret_cast<volatile uint8_t *>(0xe000e400UL);
    const uint32_t original = *priority_word;
    *priority_byte = 0xff;
    const uint8_t byte_readback = *priority_byte;
    *priority_word = (original & 0xffffff00UL) | 0xff;
    const uint8_t word_readback = static_cast<uint8_t>(*priority_word);
    *priority_word = original;
    const char hex[] = "0123456789ABCDEF";
    char byte_digits[] = {hex[byte_readback >> 4], hex[byte_readback & 0x0f], '\0'};
    char word_digits[] = {hex[word_readback >> 4], hex[word_readback & 0x0f], '\0'};
    serial_write_fault("NVIC probe: byte=0x");
    serial_write_fault(byte_digits);
    serial_write_fault(" word=0x");
    serial_write_fault(word_digits);
    serial_write_fault("\r\n");

#ifdef WOKWI_FREERTOS_PORT
    if (serial_write("FreeRTOS scheduler: Wokwi cooperative, 100 Hz\r\n") != HAL_OK) {
        fail_stop();
    }
#endif

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
