#include "sensors.h"
#include "rtos_objects.h"
#include "diagnostics.h"
#include "task.h"
#include "dht22.h"
#include "ldr.h"
#include "sensor_values.h"
#include "sensor_data.h"
#include <cstdio>

namespace app_runtime {
void sensor_task(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t sequence = 0;
    uint32_t dropped_samples = 0;
    // Let the sensor stabilize before its first transaction.
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
    for (;;) {
        SystemSnapshot snapshot = read_system();
        bool slept = false;
        while (snapshot.state == SystemState::INACTIVE) {
            slept = true;
            // Persistent ACTIVE wakes every waiter; nobody clears it on receipt.
            xEventGroupWaitBits(system_events, EVENT_ACTIVE, pdFALSE, pdFALSE, portMAX_DELAY);
            snapshot = read_system();
        }
        if (slept) { last_wake = xTaskGetTickCount(); }
        // Periodic tick deadlines can fall slightly before the driver's actual
        // previous start time. Block until the DHT's full interval has elapsed,
        // rather than publishing "not ready" as a failed sensor measurement.
        uint32_t remaining_ms = dht22_ready_in_ms();
        while (remaining_ms != 0U) {
            const TickType_t wait_ticks = static_cast<TickType_t>(
                (remaining_ms * configTICK_RATE_HZ + 999U) / 1000U);
            vTaskDelay(wait_ticks);
            remaining_ms = dht22_ready_in_ms();
        }
        const SystemSnapshot before_read = read_system();
        if (before_read.state != SystemState::ACTIVE || before_read.epoch != snapshot.epoch) {
            last_wake = xTaskGetTickCount();
            continue;
        }
        snapshot = before_read;
        Dht22Reading reading = {};
        SensorMessage message = {};
        message.state_epoch = snapshot.epoch;
        message.motion_valid = snapshot.motion_valid;
        message.values.motionDetected = snapshot.motion;
        message.sequence = ++sequence;
        message.dropped_samples = dropped_samples;
        message.dht_status = dht22_read(&reading);
        if (message.dht_status == Dht22Status::ok) {
            message.values.temperature = reading.temperature_tenths / 10.0f;
            message.values.humidity = reading.humidity_tenths / 10.0f;
        }
        message.light_valid = ldr_read(&message.light_raw);
        if (message.light_valid) {
            message.values.lightLevel = ldr_percent(message.light_raw);
        }
        const SystemSnapshot after_read = read_system();
        if (after_read.state != SystemState::ACTIVE || after_read.epoch != message.state_epoch) {
            last_wake = xTaskGetTickCount();
            continue;
        }
        // A bounded FIFO preserves sample order. Do not stall acquisition if full.
        if (xQueueSend(sensor_queue, &message, 0) != pdPASS) {
            ++dropped_samples;
        }
        // A separate one-item mailbox gives the display the latest sample.
        if (xQueueOverwrite(display_queue, &message) != pdPASS) { fail_stop(); }
        if (xQueueOverwrite(alarm_queue, &message) != pdPASS) { fail_stop(); }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
    }
}

void sensor_log_task(void *) {
    for (;;) {
        SensorMessage message = {};
        if (xQueueReceive(sensor_queue, &message, portMAX_DELAY) != pdPASS) {
            fail_stop();
        }
        char line[112];
        int length = std::snprintf(line, sizeof(line),
            "Sensor sample #%lu (dropped: %lu)\r\n",
            static_cast<unsigned long>(message.sequence),
            static_cast<unsigned long>(message.dropped_samples));
        if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) { fail_stop(); }
        print_diagnostic(line);
        if (message.dht_status == Dht22Status::ok) {
            // Integer formatting avoids requiring printf's floating-point support.
            const float temperature = message.values.temperature;
            const int tenths = static_cast<int>(temperature * 10.0f +
                                               (temperature < 0 ? -0.5f : 0.5f));
            const unsigned humidity = static_cast<unsigned>(message.values.humidity * 10.0f + 0.5f);
            const int magnitude = tenths < 0 ? -tenths : tenths;
            length = std::snprintf(line, sizeof(line),
                "Temperature: %s%d.%02d C\r\nHumidity: %u.%02u %%\r\n",
                tenths < 0 ? "-" : "", magnitude / 10,
                (magnitude % 10) * 10, humidity / 10, (humidity % 10) * 10);
        } else {
            length = std::snprintf(line, sizeof(line), "DHT22: %s\r\n", dht22_status_text(message.dht_status));
        }
        if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) { fail_stop(); }
        print_diagnostic(line);
        if (message.light_valid) {
            length = std::snprintf(line, sizeof(line), "Light: %u %% (ADC: %u)\r\n",
                static_cast<unsigned>(message.values.lightLevel), static_cast<unsigned>(message.light_raw));
            if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) { fail_stop(); }
            print_diagnostic(line);
        } else {
            print_diagnostic("LDR: ADC read failed\r\n");
        }
        print_diagnostic(!message.motion_valid ? "Motion: unavailable\r\n" :
            (message.values.motionDetected ? "Motion sample: detected\r\n" : "Motion sample: clear\r\n"));
    }
}
} // namespace app_runtime
