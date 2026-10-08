#include "rtos_objects.h"
#include "diagnostics.h"

namespace app_runtime {
SystemSnapshot read_system() {
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) != pdTRUE) { fail_stop(); }
    SystemSnapshot snapshot = system_snapshot;
    const EventBits_t bits = xEventGroupGetBits(system_events);
    snapshot.state = (bits & EVENT_ACTIVE) ? SystemState::ACTIVE : SystemState::INACTIVE;
    snapshot.motion = (bits & EVENT_MOTION) != 0;
    snapshot.alarm = (bits & EVENT_ALARM) != 0;
    if (xSemaphoreGive(state_mutex) != pdTRUE) { fail_stop(); }
    return snapshot;
}
}
