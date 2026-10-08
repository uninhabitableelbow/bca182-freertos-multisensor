#include "rtos_objects.h"
#include "diagnostics.h"
#include "serial.h"
#include "sensor_data.h"
#include "navigation.h"

namespace app_runtime {
SemaphoreHandle_t serial_mutex = nullptr;
SemaphoreHandle_t state_mutex = nullptr;
EventGroupHandle_t system_events = nullptr;
SystemSnapshot system_snapshot = {SystemState::ACTIVE, false, false, 0, false};
QueueHandle_t sensor_queue = nullptr;
QueueHandle_t display_queue = nullptr;
QueueHandle_t mode_queue = nullptr;
QueueHandle_t alarm_queue = nullptr;
constexpr UBaseType_t sensor_queue_length = 4;

void create_rtos_objects() {
    serial_mutex = xSemaphoreCreateMutex();
    state_mutex = xSemaphoreCreateMutex();
    if (serial_mutex == nullptr || state_mutex == nullptr) {
        fail_stop();
    }
    system_events = xEventGroupCreate();
    if (system_events == nullptr) {
        serial_write_fault("System event group allocation failed\r\n");
        fail_stop();
    }
    xEventGroupSetBits(system_events, EVENT_ACTIVE);
    sensor_queue = xQueueCreate(sensor_queue_length, sizeof(SensorMessage));
    display_queue = xQueueCreate(1, sizeof(SensorMessage));
    mode_queue = xQueueCreate(1, sizeof(DisplayMode));
    alarm_queue = xQueueCreate(1, sizeof(SensorMessage));
    if (sensor_queue == nullptr || display_queue == nullptr || mode_queue == nullptr || alarm_queue == nullptr) {
        serial_write_fault("Sensor queue allocation failed\r\n");
        fail_stop();
    }
}
}
