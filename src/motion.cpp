#include "motion.h"
#include "rtos_objects.h"
#include "diagnostics.h"
#include "task.h"
#include "pir.h"

namespace app_runtime {
void motion_task(void *) {
    pir_init();
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t last_motion = static_cast<uint32_t>(last_wake);
    SystemState previous = SystemState::ACTIVE;
    bool previous_motion = false;
    print_diagnostic("System: ACTIVE\r\n");
    for (;;) {
        const bool motion = pir_motion();
        const uint32_t now = static_cast<uint32_t>(xTaskGetTickCount());
        if (motion) { last_motion = now; }
        const SystemState state = activityState(motion, now, last_motion, pdMS_TO_TICKS(15000));
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) != pdTRUE) { fail_stop(); }
        const bool changed = state != system_snapshot.state || motion != system_snapshot.motion;
        if (state != system_snapshot.state) { ++system_snapshot.epoch; }
        system_snapshot.state = state;
        system_snapshot.motion = motion;
        system_snapshot.motion_valid = true;
        if (changed) {
            const EventBits_t desired = (state == SystemState::ACTIVE ? EVENT_ACTIVE : 0) |
                                       (motion ? EVENT_MOTION : 0);
            xEventGroupClearBits(system_events, (EVENT_ACTIVE | EVENT_MOTION) & ~desired);
            if (desired != 0) { xEventGroupSetBits(system_events, desired); }
        }
        if (xSemaphoreGive(state_mutex) != pdTRUE) { fail_stop(); }
        if (motion != previous_motion) {
            print_diagnostic(motion ? "Motion: detected\r\n" : "Motion: clear\r\n");
            previous_motion = motion;
        }
        if (state != previous) {
            print_diagnostic(state == SystemState::ACTIVE ? "System: ACTIVE\r\n" : "System: INACTIVE\r\n");
            previous = state;
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(50));
    }
}
} // namespace app_runtime
