# Part XIII - Modular software design

Requirements 40 and 41 split task responsibilities into separate modules and
keep the entry point focused on initialization and scheduler startup.

| Interface / source | Responsibility |
|---|---|
| app.h / app.cpp | Startup orchestration, named priorities, task creation, scheduler start |
| sensors.h / sensors.cpp | SensorTask acquisition and SensorLogTask queue consumption |
| display.h / display.cpp | DisplayTask; sole OLED owner |
| input.h / input.cpp | InputTask; encoder positions and page selection |
| alarm.h / alarm.cpp | Pure alarm decisions and AlarmTask; sole buzzer owner |
| motion.h / motion.cpp | MotionTask; PIR polling, inactivity transition publication |
| system_state.h / system_state.cpp | Pure activity decision and synchronized snapshot reading |
| rtos_objects.h / rtos_objects.cpp | One definition of shared mutexes, event group, snapshot, and queues; checked creation |
| diagnostics.h / diagnostics.cpp | Checked serial mutex output, Task A/B, and fatal RTOS hooks |

main.cpp initializes HAL and the system clock, then calls app_main. app_main
initializes the serial and sensor drivers, creates RTOS objects and tasks,
and starts the scheduler. Peripheral-specific setup for OLED, encoder, PIR,
and buzzer remains with the owning task, as in the verified implementation.
Application operation is driven by task functions rather than a main loop.

## Justified differences from the example tree

Existing dht22/ldr/oled/encoder/pir/buzzer/serial files remain low-level
drivers. Keeping these separate from task orchestration avoids coupling pulse
timing, ADC, I2C, GPIO, and UART details to unrelated tasks. sensors.cpp includes
logging because that task consumes the sensor FIFO. diagnostics.cpp groups
serial locking and diagnostic/fatal output. app.cpp is a small startup layer
between hardware-oriented main.cpp and scheduler-driven task modules.

Hardware-independent alarm, navigation, activity, and sensor conversion
functions remain in headers so their existing constexpr checks work without
HAL or a scheduler. alarm.h adds a plain task declaration without RTOS types.
The shared snapshot structure and runtime read_system declaration live in
rtos_objects.h, so system_state.h stays usable by host-side logic tests.

Runtime functions and object definitions use namespace app_runtime. Header
declarations refer to one object definition in rtos_objects.cpp; modules do
not create separate queues or mutex copies. Direct state access is still
guarded by state_mutex. MotionTask publishes motion/activity transitions;
AlarmTask publishes the alarm event. Lock ordering, queue ownership, sampling
intervals, stale-sample epoch checks, and priorities are retained.

## Checks performed

- Strict ARM C++11 compilation with warnings treated as errors passed for
  all nine application modules under both native and Wokwi configurations.
- Relocatable linking of those modules passed in both configurations; symbol
  inspection found no unresolved app_runtime references or duplicate definitions.
- Peripheral drivers and FreeRTOS are external to this module-link check;
  it does not substitute for the final PlatformIO firmware build.
- No PlatformIO build or simulation was run by the agent.

## Verification checklist and user approval

1. Build `pio run -e bluepill_wokwi` and start Wokwi. Confirm no compiler or
   linker errors, then Task A/B and sensor samples continue periodically.
2. Trigger PIR to keep ACTIVE. Change temperature, humidity, and light:
   confirm readings update. Turn the encoder quickly both ways: all four
   pages work without freezing.
3. Test 34 C and 25.4 C: alarm and OLED alarm indicator switch on/off after
   new samples. Test a low temperature below 18 C as well.
4. Allow 15 seconds without PIR motion: OLED turns off and alarm silences.
   Wake with PIR at 34 C: readings resume and the alarm returns without Error
   or repeated alarm cycling. Repeat sleep/wake once.

No separate Part XIII commit milestone is listed in the handout. After
receiving the checklist above, the user explicitly authorized a separate
commit and push on 2026-10-08: `Organize application into task modules`.
