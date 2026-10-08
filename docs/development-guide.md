# Step-by-step development

Develop on the `main` branch. Commit each laboratory milestone after its
required checks pass, following the sequence in the activity handout.

## Part I — Project initialization

1. Verify `git --version` and `pio --version`.
2. Configure `bluepill_f103c8`, `ststm32@20.0.0`, and `framework = stm32cube`.
3. Initialize HAL and configure the 8 MHz HSE / PLL for a 72 MHz CPU clock.
4. Supply `SysTick_Handler()` so HAL's millisecond counter advances.
5. Enter `app_main()`. The initial application sleeps awaiting interrupts.
6. Run `pio run -e bluepill_f103c8`. Do not add sensors or FreeRTOS tasks
   until this build succeeds.
7. Save and push the project initialization milestone:

   ```sh
   git add .
   git commit -m "Initialize STM32 PlatformIO project"
   git push
   ```

`main.cpp` owns MCU startup; `app.cpp` owns application behavior.
The declaration in `app.h` is compatible with both C and C++.
FreeRTOS tick integration will be introduced in Part III.

## Part II — Initial Wokwi simulation

Build the firmware and follow [the Part II verification procedure](part-ii-verification.md).
The circuit starts with the Blue Pill only and displays the required
startup message. Use the commit message `Configure initial Wokwi simulation`.

## Part III — FreeRTOS foundation

Create two tasks that print distinct messages and block between executions.
See [scheduling notes](part-iii-scheduling.md) for the implemented priorities,
periods, task states, and verification procedure.

## Part IV - Sensor subsystem

See [sensor notes](part-iv-sensors.md) for DHT22 and LDR wiring, the 2-second
SensorTask, periodic timing, and the manual verification procedure. The
implementation milestones are `Implement DHT22 sensor acquisition` and
`Add LDR measurement`. Wokwi verification remains a user-run check.

## Part V - Data communication

Part V implements the `SensorData` contract and a queue from SensorTask to
SensorLogTask. See [data communication notes](part-v-data-communication.md).
The milestone is `Add sensor data queue`; runtime verification is user-run.

## Part VI - Display subsystem

Part VI adds exclusive OLED ownership in DisplayTask and a separate display
mailbox. See [display notes](part-vi-display.md). The milestone is
`Implement OLED display task`; Wokwi verification remains user-run.

## Part VII - Rotary encoder navigation

The user confirmed the rapid-turn fix and authorized the milestone
`Add rotary encoder navigation`. See [navigation notes](part-vii-navigation.md).

## Part VIII - Alarm subsystem

Part VIII implements pure temperature decisions and a
separate AlarmTask controlling the PWM buzzer. See the
[alarm verification checklist](part-viii-alarm.md). The milestone is
`Implement alarm task`. The user confirmed verification and authorized
committing and pushing on 2026-10-08.

## Upcoming milestones

- PIR motion and the ACTIVE/INACTIVE state machine.
- Consumer fan-out, shared-resource mutex, event signaling, and priorities.
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
Core 6.2.0. The foundation passed `pio run -e bluepill_f103c8`: 44 bytes RAM
and 1,992 bytes flash. Build dependencies: STM32 platform 20.0.0,
STM32CubeF1 1.8.7, and GCC ARM 7.2.1.
Simulation and serial output are pending Part II; a build alone does not
verify runtime behavior.
