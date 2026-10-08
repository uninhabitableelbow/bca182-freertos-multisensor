#include "app.h"
#include "stm32f1xx_hal.h"
#include "serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "dht22.h"
#include "ldr.h"
#include "sensor_values.h"
#include "sensor_data.h"
#include "oled.h"
#include "encoder.h"
#include "navigation.h"
#include "alarm.h"
#include "buzzer.h"
#include "pir.h"
#include "system_state.h"
#include <cstdio>

namespace {
SemaphoreHandle_t serial_mutex = nullptr;
SemaphoreHandle_t state_mutex = nullptr;
struct SystemSnapshot {
    SystemState state;
    bool motion;
    bool motion_valid;
    uint32_t epoch;
};
SystemSnapshot system_snapshot = {SystemState::ACTIVE, false, false, 0};
QueueHandle_t sensor_queue = nullptr;
QueueHandle_t display_queue = nullptr;
QueueHandle_t mode_queue = nullptr;
QueueHandle_t alarm_queue = nullptr;
constexpr UBaseType_t sensor_queue_length = 4;
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

SystemSnapshot read_system() {
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) != pdTRUE) { fail_stop(); }
    const SystemSnapshot snapshot = system_snapshot;
    if (xSemaphoreGive(state_mutex) != pdTRUE) { fail_stop(); }
    return snapshot;
}

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
        if (state != system_snapshot.state) { ++system_snapshot.epoch; }
        system_snapshot.state = state;
        system_snapshot.motion = motion;
        system_snapshot.motion_valid = true;
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
            vTaskDelay(pdMS_TO_TICKS(100));
            snapshot = read_system();
        }
        if (slept) { last_wake = xTaskGetTickCount(); }
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
        buzzer_set(alarmBuzzerEnabled(valid, state));
        const char *report = !active ? "Alarm: INACTIVE; buzzer OFF\r\n" :
            (!received ? "Alarm: no fresh sample; buzzer OFF\r\n" :
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

void input_task(void *) {
    encoder_init();
    uint32_t previous = 0;
    uint32_t last_irq_count = 0;
    TickType_t last_report = xTaskGetTickCount();
    DisplayMode mode = DisplayMode::TEMPERATURE;
    bool was_active = true;
    for (;;) {
        const uint32_t position = encoder_position();
        const bool active = read_system().state == SystemState::ACTIVE;
        if (!active || !was_active) {
            // Discard turns made while sleeping; preserve the selected page.
            previous = position;
            was_active = active;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        const TickType_t now = xTaskGetTickCount();
        if (now - last_report >= pdMS_TO_TICKS(1000)) {
            const uint32_t count = encoder_interrupt_count();
            if (count != last_irq_count) {
                char line[96];
                std::snprintf(line, sizeof(line), "Encoder: %lu IRQs in %lu ms, position=%lu\r\n",
                    static_cast<unsigned long>(count - last_irq_count),
                    static_cast<unsigned long>((now - last_report) * portTICK_PERIOD_MS),
                    static_cast<unsigned long>(position));
                print_diagnostic(line);
            }
            last_irq_count = count;
            last_report = now;
        }
        if (position != previous) {
            // Unsigned subtraction preserves wraparound; four steps return home.
            const unsigned steps = (position - previous) & 3U;
            for (unsigned i = 0; i < steps; ++i) { mode = navigate(mode, true); }
            previous = position;
            if (xQueueOverwrite(mode_queue, &mode) != pdPASS) { fail_stop(); }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void display_task(void *) {
    vTaskDelay(pdMS_TO_TICKS(100)); // OLED power-on settling time.
    bool ready = false;
    bool enabled = false;
    SensorMessage message = {};
    bool have_sample = false;
    DisplayMode mode = DisplayMode::TEMPERATURE;
    uint32_t seen_epoch = 0;
    bool previous_motion = false;
    for (;;) {
        bool dirty = false;
        const SystemSnapshot snapshot = read_system();
        if (snapshot.epoch != seen_epoch) {
            have_sample = false;
            seen_epoch = snapshot.epoch;
            dirty = true;
        }
        if (snapshot.state == SystemState::INACTIVE) {
            if (ready && enabled) {
                if (oled_set_enabled(false)) { enabled = false; }
                else { print_diagnostic("OLED: sleep command failed\r\n"); }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (!ready) {
            ready = oled_init() && oled_line(0, "ROOM MONITOR") &&
                    oled_line(2, "Temperature") && oled_line(4, "Waiting...");
            if (!ready) {
                print_diagnostic("OLED: initialization failed; retrying\r\n");
                print_diagnostic(oled_error());
                print_diagnostic("\r\n");
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
        if (!ready) {
            print_diagnostic("OLED: write failed; retrying\r\n");
            print_diagnostic(oled_error());
            print_diagnostic("\r\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
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
 *        SensorTask publishes samples; SensorLogTask consumes the queue.
 */
extern "C" void app_main(void) {
    if (serial_init() != HAL_OK ||
        serial_write("BCA182 FreeRTOS Multisensor\r\nSystem starting...\r\n") != HAL_OK) {
        fail_stop();
    }

    serial_mutex = xSemaphoreCreateMutex();
    state_mutex = xSemaphoreCreateMutex();
    if (serial_mutex == nullptr || state_mutex == nullptr) {
        fail_stop();
    }
    sensor_queue = xQueueCreate(sensor_queue_length, sizeof(SensorMessage));
    display_queue = xQueueCreate(1, sizeof(SensorMessage));
    mode_queue = xQueueCreate(1, sizeof(DisplayMode));
    alarm_queue = xQueueCreate(1, sizeof(SensorMessage));
    if (sensor_queue == nullptr || display_queue == nullptr || mode_queue == nullptr || alarm_queue == nullptr) {
        serial_write_fault("Sensor queue allocation failed\r\n");
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

    if (xTaskCreate(motion_task, "MotionTask", 256, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(sensor_task, "SensorTask", 384, nullptr, 3, nullptr) != pdPASS ||
        xTaskCreate(sensor_log_task, "SensorLogTask", 384, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(display_task, "DisplayTask", 384, nullptr, 1, nullptr) != pdPASS ||
        xTaskCreate(input_task, "InputTask", 256, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(alarm_task, "AlarmTask", 256, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(task_a, "TaskA", task_stack_words, nullptr, 2, nullptr) != pdPASS ||
        xTaskCreate(task_b, "TaskB", task_stack_words, nullptr, 1, nullptr) != pdPASS) {
        fail_stop();
    }

    vTaskStartScheduler();
    // A successful scheduler start does not return.
    fail_stop();
}
