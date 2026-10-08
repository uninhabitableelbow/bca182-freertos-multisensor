#pragma once
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "system_events.h"
#include "system_state.h"

namespace app_runtime {
struct SystemSnapshot {
    SystemState state;
    bool motion;
    bool motion_valid;
    uint32_t epoch;
    bool alarm;
};

extern SemaphoreHandle_t serial_mutex;
extern SemaphoreHandle_t state_mutex;
extern EventGroupHandle_t system_events;
extern SystemSnapshot system_snapshot;
extern QueueHandle_t sensor_queue;
extern QueueHandle_t display_queue;
extern QueueHandle_t mode_queue;
extern QueueHandle_t alarm_queue;
SystemSnapshot read_system();
void create_rtos_objects();
}
