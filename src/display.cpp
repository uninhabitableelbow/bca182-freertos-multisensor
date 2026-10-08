#include "display.h"
#include "rtos_objects.h"
#include "diagnostics.h"
#include "task.h"
#include "oled.h"
#include "sensor_data.h"
#include "navigation.h"
#include <cstdio>

namespace app_runtime {
void display_task(void *) {
    vTaskDelay(pdMS_TO_TICKS(100)); // OLED power-on settling time.
    bool ready = false;
    bool enabled = false;
    SensorMessage message = {};
    bool have_sample = false;
    DisplayMode mode = DisplayMode::TEMPERATURE;
    uint32_t seen_epoch = 0;
    bool previous_motion = false;
    bool previous_alarm = false;
    for (;;) {
        bool dirty = false;
        const SystemSnapshot snapshot = read_system();
        if (snapshot.epoch != seen_epoch) {
            have_sample = false;
            seen_epoch = snapshot.epoch;
            dirty = true;
        }
        if (snapshot.state == SystemState::INACTIVE) {
            if (enabled) {
                if (oled_set_enabled(false)) { enabled = false; }
                else { print_diagnostic("OLED: sleep command failed\r\n"); }
            }
            if (!enabled) {
                xEventGroupWaitBits(system_events, EVENT_ACTIVE, pdFALSE, pdFALSE, portMAX_DELAY);
            } else {
                vTaskDelay(pdMS_TO_TICKS(100)); // Retry a failed display-off command.
            }
            continue;
        }
        if (!ready) {
            ready = oled_init() && oled_line(0, "ROOM MONITOR") &&
                    oled_line(2, "Temperature") && oled_line(4, "Waiting...");
            if (!ready) {
                print_diagnostic("OLED: initialization failed; retrying\r\n", oled_error());
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }
            enabled = true;
            dirty = true;
        }
        if (!enabled) {
            ready = oled_set_enabled(true);
            if (!ready) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }
            enabled = true;
            dirty = true;
        }
        if (xQueueReceive(display_queue, &message, 0) == pdPASS) {
            have_sample = message.state_epoch == snapshot.epoch;
            dirty = true;
        }
        if (xQueueReceive(mode_queue, &mode, 0) == pdPASS) { dirty = true; }
        if (snapshot.motion != previous_motion) { dirty = true; previous_motion = snapshot.motion; }
        if (snapshot.alarm != previous_alarm) { dirty = true; previous_alarm = snapshot.alarm; }
        if (!dirty) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        const char *labels[] = {"Temperature", "Humidity", "Light", "Motion"};
        ready = oled_line(2, labels[static_cast<unsigned>(mode)]);
        char value[24];
        if (mode == DisplayMode::MOTION) {
            std::snprintf(value, sizeof(value), "%s", !snapshot.motion_valid ? "Not available" :
                (snapshot.motion ? "Detected" : "None"));
        } else if (!have_sample) {
            std::snprintf(value, sizeof(value), "Waiting...");
        } else if (mode == DisplayMode::LIGHT) {
            if (message.light_valid) { std::snprintf(value, sizeof(value), "%d %%", message.values.lightLevel); }
            else { std::snprintf(value, sizeof(value), "Error"); }
        } else if (message.dht_status != Dht22Status::ok) {
            std::snprintf(value, sizeof(value), "Error");
        } else if (mode == DisplayMode::HUMIDITY) {
            const unsigned tenths = static_cast<unsigned>(message.values.humidity * 10.0f + 0.5f);
            std::snprintf(value, sizeof(value), "%u.%u %%", tenths / 10, tenths % 10);
        } else {
            const float temperature = message.values.temperature;
            const int tenths = static_cast<int>(temperature * 10.0f +
                                               (temperature < 0 ? -0.5f : 0.5f));
            const int magnitude = tenths < 0 ? -tenths : tenths;
            const int length = std::snprintf(value, sizeof(value), "%s%d.%d C",
                tenths < 0 ? "-" : "", magnitude / 10, magnitude % 10);
            if (length < 0 || static_cast<size_t>(length) >= sizeof(value)) { fail_stop(); }
        }
        ready = ready && oled_line(4, value);
        ready = ready && oled_line(6, snapshot.alarm ? "ALARM" : "");
        if (!ready) {
            print_diagnostic("OLED: write failed; retrying\r\n", oled_error());
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
} // namespace app_runtime
