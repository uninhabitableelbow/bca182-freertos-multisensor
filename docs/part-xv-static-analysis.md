# Part XV - Static code analysis

Requirements 45 and 46 require running `pio check` and interpreting findings
in a file/line, cause, and resolution table.

## Commands and results - 2026-10-08

```powershell
pio check
pio check --fail-on-defect high --fail-on-defect medium
pio check -e bluepill_f103c8 --fail-on-defect high --fail-on-defect medium
```

The initial default analysis passed with 0 high, 0 medium, and 34 low findings.
After the OLED lookup change, both configurations passed with 0 high,
0 medium, and 33 low findings each. The remaining categories are identical:
24 unusedFunction, 8 cstyleCast, and 1 constParameterPointer. Reviewed low
findings remain visible; this is not a report of zero findings.

Cppcheck 2.11 analyzes project sources with framework include paths available.
Configuration explicitly selects C++11 and the unix32 data model (32-bit
pointers, int, and long, appropriate for STM32). No new suppressions were
added. The native host-test environment is not firmware analysis. Final
diagnostics and individual resolutions are in the table below; raw JSON
outputs remain in ignored .pio/compile-check/part-xv.

## Finding interpretation

The installed PlatformIO Cppcheck adapter invokes the analyzer per source
file. Its unusedFunction analysis cannot see the other translation units'
callers. All 24 listed functions have production callers, including task
entries registered through xTaskCreate. Removing them would break firmware.

C-style cast findings expand CMSIS peripheral-address macros or FreeRTOS
xSemaphoreGive. These are external framework definitions, not handwritten
application casts. Their types were checked against installed headers.

HAL_UART_MspInit must match the HAL callback declaration with a mutable
UART_HandleTypeDef pointer, even though this override only reads the handle.
Changing to const would conflict with stm32f1xx_hal_uart.h:727.

## Validation and limits

The changed OLED source compiled with ARM C++11, -Wall -Wextra -Werror for
both configurations. pio test -e native passed all 34 cases after the change.
No PlatformIO firmware build or Wokwi simulation was run by the agent.

Static analysis and pure tests do not prove electrical timing, concurrency,
or runtime peripheral behavior. Framework sources and the custom C port
are not covered by this src-focused report. New findings must be reviewed.

Part XV is complete. The user authorized committing and pushing on
2026-10-08 under the handout milestone: `Resolve static analysis findings`.

References: [PlatformIO Cppcheck](https://docs.platformio.org/en/stable/advanced/static-code-analysis/tools/cppcheck.html)
and [check_flags](https://docs.platformio.org/en/stable/projectconf/sections/env/options/check/check_flags.html).

## Findings table

| Finding | File/line | Cause | Resolution |
|---|---|---|---|
| useStlAlgorithm (resolved) | src/oled.cpp:74 (initial) | Manual first-match font search | Use std::find_if, preserving first-match and unknown-character blank output; absent from final runs |
| cstyleCast | src/alarm.cpp:40 | FreeRTOS xSemaphoreGive macro cast | Retain kernel QueueHandle_t cast; handle type checked |
| unusedFunction | src/alarm.cpp:9 | Cross-module caller not visible in per-file check | Retain alarm_task; caller src/app.cpp:56 via xTaskCreate |
| cstyleCast | src/buzzer.cpp:28 | CMSIS register-address macro cast | Retain vendor TIM4/TIM2/I2C1 mapping |
| unusedFunction | src/buzzer.cpp:12 | Cross-module caller not visible in per-file check | Retain buzzer_init; caller src/alarm.cpp:10 |
| unusedFunction | src/buzzer.cpp:46 | Cross-module caller not visible in per-file check | Retain buzzer_set; caller src/alarm.cpp:36 |
| cstyleCast | src/dht22.cpp:56 | CMSIS register-address macro cast | Retain vendor TIM4/TIM2/I2C1 mapping |
| unusedFunction | src/dht22.cpp:113 | Cross-module caller not visible in per-file check | Retain dht22_status_text; caller src/sensors.cpp:101 |
| cstyleCast | src/diagnostics.cpp:26 | FreeRTOS xSemaphoreGive macro cast | Retain kernel QueueHandle_t cast; handle type checked |
| cstyleCast | src/diagnostics.cpp:55 | FreeRTOS xSemaphoreGive macro cast | Retain kernel QueueHandle_t cast; handle type checked |
| unusedFunction | src/diagnostics.cpp:39 | Cross-module caller not visible in per-file check | Retain task_a; caller src/app.cpp:57 via xTaskCreate |
| unusedFunction | src/diagnostics.cpp:47 | Cross-module caller not visible in per-file check | Retain task_b; caller src/app.cpp:58 via xTaskCreate |
| unusedFunction | src/display.cpp:11 | Cross-module caller not visible in per-file check | Retain display_task; caller src/app.cpp:54 via xTaskCreate |
| unusedFunction | src/encoder.cpp:19 | Cross-module caller not visible in per-file check | Retain encoder_init; caller src/input.cpp:11 |
| unusedFunction | src/encoder.cpp:59 | Cross-module caller not visible in per-file check | Retain encoder_position; caller src/input.cpp:18 |
| unusedFunction | src/encoder.cpp:75 | Cross-module caller not visible in per-file check | Retain encoder_interrupt_count; caller src/input.cpp:29 |
| unusedFunction | src/input.cpp:10 | Cross-module caller not visible in per-file check | Retain input_task; caller src/app.cpp:55 via xTaskCreate |
| cstyleCast | src/motion.cpp:32 | FreeRTOS xSemaphoreGive macro cast | Retain kernel QueueHandle_t cast; handle type checked |
| unusedFunction | src/motion.cpp:8 | Cross-module caller not visible in per-file check | Retain motion_task; caller src/app.cpp:51 via xTaskCreate |
| cstyleCast | src/oled.cpp:125 | CMSIS register-address macro cast | Retain vendor TIM4/TIM2/I2C1 mapping |
| unusedFunction | src/oled.cpp:61 | Cross-module caller not visible in per-file check | Retain oled_error; caller src/display.cpp:45,94 |
| unusedFunction | src/oled.cpp:63 | Cross-module caller not visible in per-file check | Retain oled_set_enabled; caller src/display.cpp:31,53 |
| unusedFunction | src/oled.cpp:108 | Cross-module caller not visible in per-file check | Retain oled_init; caller src/display.cpp:42 |
| unusedFunction | src/pir.cpp:4 | Cross-module caller not visible in per-file check | Retain pir_init; caller src/motion.cpp:9 |
| unusedFunction | src/pir.cpp:13 | Cross-module caller not visible in per-file check | Retain pir_motion; caller src/motion.cpp:16 |
| unusedFunction | src/rtos_objects.cpp:18 | Cross-module caller not visible in per-file check | Retain create_rtos_objects; caller src/app.cpp:39 |
| unusedFunction | src/sensors.cpp:12 | Cross-module caller not visible in per-file check | Retain sensor_task; caller src/app.cpp:52 via xTaskCreate |
| unusedFunction | src/sensors.cpp:76 | Cross-module caller not visible in per-file check | Retain sensor_log_task; caller src/app.cpp:53 via xTaskCreate |
| constParameterPointer | src/serial.cpp:10 | Callback pointer could be const locally | Retain mutable pointer to match HAL callback declaration |
| unusedFunction | src/serial.cpp:31 | Cross-module caller not visible in per-file check | Retain serial_init; caller src/app.cpp:34 |
| unusedFunction | src/serial.cpp:43 | Cross-module caller not visible in per-file check | Retain serial_write; caller src/app.cpp:35; diagnostics.cpp:21-24 |
| unusedFunction | src/serial.cpp:60 | Cross-module caller not visible in per-file check | Retain serial_write_fault; caller src/diagnostics.cpp:73,87,94-106 |
| cstyleCast | src/system_state.cpp:12 | FreeRTOS xSemaphoreGive macro cast | Retain kernel QueueHandle_t cast; handle type checked |
| unusedFunction | src/system_state.cpp:5 | Cross-module caller not visible in per-file check | Retain read_system; caller src/sensors.cpp:19; display.cpp:23; alarm.cpp:23 |
