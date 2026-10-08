#include "input.h"
#include "rtos_objects.h"
#include "diagnostics.h"
#include "task.h"
#include "encoder.h"
#include "navigation.h"
#include <cstdio>

namespace app_runtime {
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
} // namespace app_runtime
