# Step-by-step development

This rebuild uses the `step-by-step` branch. Work on other branches remains
available. Commit each milestone after its relevant checks pass.

## Part I — Project initialization

1. Verify `git --version` and `pio --version`.
2. Configure `bluepill_f103c8`, `ststm32@20.0.0`, and `framework = stm32cube`.
3. Initialize HAL and configure the 8 MHz HSE / PLL for a 72 MHz CPU clock.
4. Supply `SysTick_Handler()` so HAL's millisecond counter advances.
5. Enter `app_main()`. The initial application sleeps awaiting interrupts.
6. Run `pio run -e bluepill_f103c8`, then commit and push the foundation.

`main.cpp` owns MCU startup; `app.cpp` owns application behavior.
The declaration in `app.h` is compatible with both C and C++.
FreeRTOS tick integration will be introduced in Part III.

## Upcoming milestones

- Part II: Blue Pill Wokwi circuit and recognizable serial startup message.
- Part III: two blocking FreeRTOS tasks and scheduling observations.
- Sensor acquisition, OLED ownership, encoder navigation, and alarm output.
- PIR motion and the ACTIVE/INACTIVE state machine.
- Queue communication, shared-resource mutex, event signaling, and priorities.
- Hardware-independent logic, unit tests, static analysis, functional tests,
  and deliberate fault experiments.
- Public README, academic report, and requirements traceability.
- Portfolio publication and technical defense preparation.

Runtime observations and screenshots must come from actual executions.

## Requirements retained for later parts

- Temperature, humidity, relative light, and motion measurements.
- Encoder selection of one OLED measurement at a time.
- Alarm outside the inclusive normal range of 18–30 degrees Celsius.
- Motion reactivation and a documented inactivity timeout.
- At least five meaningful tasks with justified priorities.
- Queue, mutex, event group or notification, and `vTaskDelayUntil()`.
- At least 13 meaningful unit tests, static analysis, and fault experiments.

## Part I verification

Environment checked on 2026-10-06: Git 2.54.0.windows.1 and PlatformIO
Core 6.2.0. The original scaffold compiled before the foundation was revised.
The revised foundation passed `pio run -e bluepill_f103c8`: 44 bytes RAM
and 1,992 bytes flash. Build dependencies: STM32 platform 20.0.0,
STM32CubeF1 1.8.7, and GCC ARM 7.2.1.
Simulation and serial output are pending Part II; a build alone does not
verify runtime behavior.
