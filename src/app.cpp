#include "app.h"
#include "stm32f1xx_hal.h"
#include "serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dht22.h"
#include "ldr.h"
#include "rtos_objects.h"
#include "diagnostics.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"

namespace {
constexpr uint16_t task_stack_words = 256; // 1 KiB per task on Cortex-M3.
// Part XII: urgency and latency rationale in docs/part-xii-priorities.md.
// DHT pulse acquisition must outrank task-level polling on physical hardware.
constexpr UBaseType_t sensor_priority = 3;
constexpr UBaseType_t motion_priority = 2;
constexpr UBaseType_t input_priority = 2;
constexpr UBaseType_t alarm_priority = 2;
constexpr UBaseType_t display_priority = 1;
constexpr UBaseType_t logging_priority = 1;
constexpr UBaseType_t diagnostic_priority = 1;
static_assert(sensor_priority < configMAX_PRIORITIES,
              "Sensor priority must fit the configured FreeRTOS range");
}

using namespace app_runtime;

extern "C" void app_main(void) {
    if (serial_init() != HAL_OK ||
        serial_write("BCA182 FreeRTOS Multisensor\r\nSystem starting...\r\n") != HAL_OK) {
        fail_stop();
    }

    create_rtos_objects();


    if (!dht22_init()) {
        serial_write_fault("DHT22 initialization failed\r\n");
        fail_stop();
    }
    if (!ldr_init()) {
        serial_write_fault("LDR ADC initialization failed\r\n");
        fail_stop();
    }

    if (xTaskCreate(motion_task, "MotionTask", 256, nullptr, motion_priority, nullptr) != pdPASS ||
        xTaskCreate(sensor_task, "SensorTask", 384, nullptr, sensor_priority, nullptr) != pdPASS ||
        xTaskCreate(sensor_log_task, "SensorLogTask", 384, nullptr, logging_priority, nullptr) != pdPASS ||
        xTaskCreate(display_task, "DisplayTask", 384, nullptr, display_priority, nullptr) != pdPASS ||
        xTaskCreate(input_task, "InputTask", 256, nullptr, input_priority, nullptr) != pdPASS ||
        xTaskCreate(alarm_task, "AlarmTask", 256, nullptr, alarm_priority, nullptr) != pdPASS ||
        xTaskCreate(task_a, "TaskA", task_stack_words, nullptr, diagnostic_priority, nullptr) != pdPASS ||
        xTaskCreate(task_b, "TaskB", task_stack_words, nullptr, diagnostic_priority, nullptr) != pdPASS) {
        fail_stop();
    }

    vTaskStartScheduler();
    // A successful scheduler start does not return.
    fail_stop();
}
