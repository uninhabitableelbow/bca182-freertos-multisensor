# Requirements Traceability Matrix

Part XXI, requirement 60: every major requirement must map to implemented
behavior and verification evidence. The FR identifiers below follow the
handout's functional-requirements table. Additional labels are local audit
labels, not new identifiers from the handout.

## Evidence scope

Unit results are executed host tests of production pure logic. FT results
are user-confirmed Wokwi observations recorded in
[functional verification](part-xvi-functional-verification.md). These are
not independent hardware measurements. Source review shows implementation;
it does not substitute for runtime testing. Screenshot evidence establishes
only the visible circuit or displayed measurement.

## Functional requirements

| Requirement | Implementation | Verification evidence | Status and limits |
|---|---|---|---|
| FR-01 - Periodic temperature measurement | [SensorTask](../src/sensors.cpp), [DHT22 driver](../src/dht22.cpp): two-second sampling and minimum-read interval | FT-01 in [functional record](part-xvi-functional-verification.md); [running screenshot](images/finished-system.png) shows control/OLED at 28.8 C | Implemented; user-confirmed functional PASS. No measured timing bound or physical sensor test. |
| FR-02 - Periodic humidity measurement | [SensorTask](../src/sensors.cpp), [DHT22 driver](../src/dht22.cpp): validated humidity frame and queued sample | FT-02 in [functional record](part-xvi-functional-verification.md) | Implemented; user-confirmed PASS. Screenshot humidity control alone is not an OLED humidity test. |
| FR-03 - Relative ambient-light measurement | [LDR ADC driver](../src/ldr.cpp), [relative conversion](../include/sensor_values.h), SensorTask | FT-03 in [functional record](part-xvi-functional-verification.md); supplemental [conversion checks](../test/sensor_values_compile.cpp) | Implemented; user-confirmed PASS. Relative ADC percentage, not calibrated lux. |
| FR-04 - PIR motion detection | [MotionTask](../src/motion.cpp), [PIR driver](../src/pir.cpp): PA4 sampled every 50 ms; motion flag published | FT-08 in [functional record](part-xvi-functional-verification.md) | Implemented; user-confirmed PASS. Electrical pulse capture not measured on hardware. |
| FR-05 - One selected measurement on OLED | [DisplayTask](../src/display.cpp), [OLED driver](../src/oled.cpp): selected page, latest sample, validity/state handling | FT-01 through FT-05; [finished capture](images/finished-system.png); [display notes](part-vi-display.md) | Implemented; user-confirmed PASS. Capture independently shows Temperature page only. |
| FR-06 - Encoder navigation among four pages | [InputTask](../src/input.cpp), [navigate/decoder](../include/navigation.h), [encoder driver](../src/encoder.cpp) | Executed [13 navigation/encoder cases](../test/test_navigation/test_main.cpp); FT-04/FT-05 | Implemented; unit PASS and user-confirmed functional PASS. Host rapid-cycle test does not measure simulator responsiveness. |
| FR-07 - Temperature alarm outside normal range | [AlarmTask](../src/alarm.cpp), [pure alarm decisions](../include/alarm.h), [PWM driver](../src/buzzer.cpp): normal 18-30 C inclusive | Executed [11 alarm cases](../test/test_alarm/test_main.cpp); FT-06/FT-07 | Implemented; unit PASS and user-confirmed functional PASS. No physical audible-output measurement. |
| FR-08 - ACTIVE and INACTIVE states | [activityState](../include/system_state.h), [MotionTask](../src/motion.cpp), [snapshot reader](../src/system_state.cpp); consumers honor activity | Executed [10 state cases](../test/test_system_state/test_main.cpp); FT-08/FT-09/FT-10 | Implemented; unit PASS and user-confirmed functional PASS. INACTIVE is application behavior, not hardware sleep. |
| FR-09 - Automatic inactivity after no motion | MotionTask refreshes last-motion while PIR is high, then applies 15000-ms timeout through activityState | State tests for before/exact/after timeout and refreshed deadline; FT-09 | Implemented; unit PASS and user-confirmed functional PASS. Timeout is simulated time after motion clears. |
| FR-10 - Automatic motion reactivation | MotionTask sets ACTIVE and increments epoch; sensor/display wait on ACTIVE; alarm accepts current-epoch data | State wake/boundary cases; FT-10; [wake-up design](part-ix-motion-state.md) | Implemented; unit PASS and user-confirmed functional PASS. Wake response duration was not measured. |

## Architecture and FreeRTOS requirements

| Major requirement / handout reference | Implementation | Verification / interpretation | Status and limits |
|---|---|---|---|
| STM32Cube, HAL, native FreeRTOS; no Arduino (6, 10, 13) | [platformio.ini](../platformio.ini), [kernel build script](../scripts/freertos.py), [main](../src/main.cpp) | Source/configuration review; framework and APIs identified in [report](laboratory-report.pdf), sections 1-4 | Implemented; physical runtime verification not claimed. Isolated ESP-IDF wording is resolved in favor of the explicit STM32Cube requirement. |
| At least five meaningful tasks (7) | [app_main task creation](../src/app.cpp): five required operational tasks plus logging and two diagnostics | [actual task table](part-xii-priorities.md); report section 3; user functional checks | Implemented. Separate StateTask is recommended rather than mandatory; MotionTask owns transitions and a separate module reads synchronized state. |
| Explicit priorities and technical justification (9, 38-39) | Named priorities in app.cpp; Sensor 3, Motion/Input/Alarm 2, display/logging/diagnostics 1 | [priority rationale](part-xii-priorities.md); [priority fault observation](part-xvii-fault-experiments.md) | Reviewed. Deliberate deviation protects synchronous DHT timing; no hardware latency bound established. |
| Blocking delays and periodic vTaskDelayUntil (9, 16-18) | Sensor, Motion, Task A/B periodic waits; other tasks use bounded waits/delays | Source review; recurring user output; [no-delay experiment](part-xvii-fault-experiments.md) | Implemented. No-delay experiment establishes repeated output, not quantified CPU load/starvation. |
| At least one queue with a real role (9, Part V) | [RTOS objects](../src/rtos_objects.cpp): 4-item logging FIFO and one-item display/alarm/mode mailboxes | [queue design](part-v-data-communication.md); advancing sensor records and FT-01 through FT-07 | Implemented; deliberate FIFO overflow runtime test has no recorded observation. |
| At least one mutex/shared-resource protection (9, 36-37) | [serial ownership](../src/diagnostics.cpp); state mutex in MotionTask, AlarmTask, snapshot reader | [mutex review](part-xi-mutex.md); user verification; clean-output [mutex-removal experiment](part-xvii-fault-experiments.md) | Implemented. Cooperative clean output is not proof of preemptive contention safety. |
| Event group or task notification (9, Part X) | [event definitions](../include/system_events.h): ACTIVE/MOTION/ALARM; producer/consumer calls in task modules | [event ownership/checks](part-x-events.md); FT-08 through FT-10 | Implemented; persistent ACTIVE wakes both consumers without receipt-side clearing. |
| State machine / inter-task communication (9, Parts IX-X) | MotionTask owns transitions; state mutex, epochs, event bits, and copied sample messages coordinate tasks | FR-08 through FR-10 above; state tests and user sleep/wake results | Implemented; hardware power-management behavior not claimed. |
| Modular source and focused main (40-41) | [module map](part-xiii-modules.md), task files, object module, and startup layer | Strict module compilation/internal linkage in both configurations; moved function-body comparison; user approved Part XIII | Implemented. Separate low-level driver files are a justified extension of the example tree. |

## Quality, evidence, and documentation requirements

| Major requirement / handout reference | Artifact / implementation | Verification evidence | Status and limits |
|---|---|---|---|
| Minimum 13 meaningful unit tests and passing pio test (42-44) | [alarm suite](../test/test_alarm/test_main.cpp), [navigation suite](../test/test_navigation/test_main.cpp), [state suite](../test/test_system_state/test_main.cpp) | [34 executed tests](part-xiv-unit-tests.md): 11 + 13 + 10; pio test -e native | Complete; host decision tests do not cover peripherals or RTOS concurrency. |
| Static analysis and interpreted findings table (45-46) | [analysis configuration](../platformio.ini), [33-row findings table](part-xv-static-analysis.md), corrected OLED lookup | Both firmware configurations: 0 high, 0 medium, 33 reviewed low; strict OLED compilation and 34 tests after correction | Complete for recorded analysis; framework/custom C port not covered by src-focused report. |
| Ten functional tests with actual records (47-48) | [FT-01 through FT-10 table](part-xvi-functional-verification.md) | User reported no failures after receiving all ten test steps | Complete as user-reported verification; separate per-test measurements/timestamps were not supplied. |
| Three faults and restoration (49-51) | [selector](../scripts/select_fault.py), [normal-mode header](../include/lab_fault_config.h), guarded app/diagnostic changes | [fault observations and explanations](part-xvii-fault-experiments.md); all modes compiler-checked; user restored normal mode | Complete; no invented interleaving, starvation measurements, or hardware outcomes. |
| Incremental Git/GitHub history (52-53) | Engineering milestone commits on main | [history mapping](part-xviii-git-history.md); remote equality checked at audit time | Completed milestones present. Final documentation commit waits for overall documentation completion. |
| Public README structure and five captioned figures (54-56) | [README](../README.md), two [actual image files](images/README.md), three Mermaid diagrams | [README checks](part-xix-readme.md): 21 sections, local links, image inspection | Complete. Figures demonstrate specific technical points, not every test independently. |
| Separate academic PDF, eight sections, justified task table (57-59) | [laboratory report](laboratory-report.pdf), [generator](../scripts/build_lab_report.py) | [report review](part-xx-report.md): all nine rendered pages inspected; all eight headings extracted | Complete; follows actual priorities and distinguishes user reports from measured outcomes. |
| Requirements traceability matrix (60) | This document | Source/test/artifact links reviewed; FR-01 through FR-10 and major architecture/quality requirements mapped | Complete for work through Part XXI; later publication/submission requirements remain separate. |

## Maintenance

After a behavior change, update the implementation mapping, repeat the
appropriate test, and record new evidence before changing its status. The
matrix maps current code and recorded evidence; it is not an unconditional
claim that every possible schedule, fault, or physical condition passes.

No firmware build, simulation, or fresh unit/static-analysis run was needed
for this documentation-only part. Evidence above refers to existing recorded
runs. The user authorized committing and pushing Part XXI on 2026-10-08:
`Add requirements traceability matrix`.
