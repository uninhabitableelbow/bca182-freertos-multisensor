#include "alarm.h"
#include "rtos_objects.h"
#include "diagnostics.h"
#include "task.h"
#include "sensor_data.h"
#include "buzzer.h"

namespace app_runtime {
void alarm_task(void *) {
    if (!buzzer_init()) {
        print_diagnostic("Buzzer initialization failed\r\n");
        fail_stop();
    }
    const char *last_report = nullptr;
    SensorMessage message = {};
    bool have_sample = false;
    TickType_t received_at = 0;
    for (;;) {
        if (xQueueReceive(alarm_queue, &message, pdMS_TO_TICKS(100)) == pdPASS) {
            have_sample = true;
            received_at = xTaskGetTickCount();
        }
        const SystemSnapshot snapshot = read_system();
        const bool active = snapshot.state == SystemState::ACTIVE;
        const bool received = have_sample && message.state_epoch == snapshot.epoch &&
            xTaskGetTickCount() - received_at < pdMS_TO_TICKS(3000);
        const bool valid = active && received && message.dht_status == Dht22Status::ok &&
                           validAlarmTemperature(message.values.temperature);
        const AlarmState state = valid ? evaluateTemperature(message.values.temperature) : AlarmState::NORMAL;
        // Recheck under the same lock as MotionTask's transition publication.
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) != pdTRUE) { fail_stop(); }
        const bool still_active = system_snapshot.state == SystemState::ACTIVE;
        const bool current_epoch = system_snapshot.epoch == snapshot.epoch;
        const bool enabled = alarmBuzzerEnabled(valid, state) && still_active &&
                             current_epoch;
        buzzer_set(enabled);
        const EventBits_t bits = xEventGroupGetBits(system_events);
        if (enabled && !(bits & EVENT_ALARM)) { xEventGroupSetBits(system_events, EVENT_ALARM); }
        else if (!enabled && (bits & EVENT_ALARM)) { xEventGroupClearBits(system_events, EVENT_ALARM); }
        if (xSemaphoreGive(state_mutex) != pdTRUE) { fail_stop(); }
        const char *report = !still_active ? "Alarm: INACTIVE; buzzer OFF\r\n" :
            (!received || !current_epoch ? "Alarm: no fresh sample; buzzer OFF\r\n" :
            (!valid ? "Alarm: invalid temperature; buzzer OFF\r\n" :
            (state == AlarmState::NORMAL ? "Alarm: NORMAL; buzzer OFF\r\n" :
            (state == AlarmState::LOW_TEMPERATURE ? "Alarm: LOW_TEMPERATURE; buzzer ON\r\n" :
                                                 "Alarm: HIGH_TEMPERATURE; buzzer ON\r\n"))));
        if (report != last_report) {
            print_diagnostic(report);
            last_report = report;
        }
    }
}
} // namespace app_runtime
