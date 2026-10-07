#include "app.h"
#include "stm32f1xx_hal.h"
#include "serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "dht22.h"
#include "ldr.h"
#include "sensor_values.h"
#include <cstdio>

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
    if (status != HAL_OK) {
        serial_write_fault("UART transmission failed\r\n");
        fail_stop();
    }
    if (released != pdTRUE) {
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
    for (;;) {
        print_diagnostic("Task B running\r\n");
        vTaskDelayUntil(&last_wake, diagnostic_period);
    }
}

void sensor_task(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    // Let the sensor stabilize before its first transaction.
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
    for (;;) {
        Dht22Reading reading = {};
        const Dht22Status status = dht22_read(&reading);
        char line[112];
        int length;
        if (status == Dht22Status::ok) {
            const int magnitude = reading.temperature_tenths < 0 ?
                -reading.temperature_tenths : reading.temperature_tenths;
            length = std::snprintf(line, sizeof(line),
                "Temperature: %s%d.%02d C\r\nHumidity: %u.%02u %%\r\n",
                reading.temperature_tenths < 0 ? "-" : "", magnitude / 10,
                (magnitude % 10) * 10, static_cast<unsigned>(reading.humidity_tenths / 10),
                static_cast<unsigned>((reading.humidity_tenths % 10) * 10));
        } else {
            length = std::snprintf(line, sizeof(line), "DHT22: %s\r\n", dht22_status_text(status));
        }
        if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) { fail_stop(); }
        print_diagnostic(line);
        uint16_t light_raw = 0;
        if (ldr_read(&light_raw)) {
            length = std::snprintf(line, sizeof(line), "Light: %u %% (ADC: %u)\r\n",
                static_cast<unsigned>(ldr_percent(light_raw)), static_cast<unsigned>(light_raw));
            if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) { fail_stop(); }
            print_diagnostic(line);
        } else {
            print_diagnostic("LDR: ADC read failed\r\n");
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
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

/**
 * @brief Application main entry point.
 *        Runs after the HAL and system clock are initialized in main().
 *
 *        Part IV adds SensorTask to the Part III diagnostic tasks.
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

    if (!dht22_init()) {
        serial_write_fault("DHT22 initialization failed\r\n");
        fail_stop();
    }
    if (!ldr_init()) {
        serial_write_fault("LDR ADC initialization failed\r\n");
        fail_stop();
    }

    if (xTaskCreate(sensor_task, "SensorTask", 384, nullptr, 3, nullptr) != pdPASS ||
        xTaskCreate(task_a, "TaskA", task_stack_words, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(task_b, "TaskB", task_stack_words, nullptr, 1, nullptr) != pdPASS) {
        fail_stop();
    }

    vTaskStartScheduler();
    // A successful scheduler start does not return.
    fail_stop();
}
