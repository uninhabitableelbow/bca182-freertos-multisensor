# Part XI - Shared-resource protection

Requirements 36 and 37 require a working mutex and an explanation of the
actual resource, competing tasks, and failure prevented. The serial mutex
was introduced with the diagnostic tasks; this part reviews all current
writers and makes multi-line OLED error reports one protected operation.

## Actual shared resource

The resource is USART1 output together with the `UART_HandleTypeDef` in
`src/serial.cpp`. All tasks share that one HAL handle and serial byte stream.
`print_diagnostic` in `src/app.cpp` takes `serial_mutex` with `portMAX_DELAY`,
transmits the complete report, and gives the mutex. Creation, acquisition,
UART status, and release are checked. Optional OLED error details and the
terminating newline are transmitted before releasing the same lock.

| Competing task | Diagnostic output |
|---|---|
| Task A | One-second Task A message |
| Task B | One-second Task B message and changed event flags |
| SensorLogTask | Sample number, measurements, driver errors, and sampled motion |
| MotionTask | PIR changes and ACTIVE/INACTIVE transitions |
| AlarmTask | Temperature alarm/fault changes |
| DisplayTask | OLED failures and retry messages |
| InputTask | Encoder interrupt counts on physical hardware; Wokwi uses polling |

SensorTask publishes data through queues and does not directly print.
The OLED, buzzer, and sensor drivers do not write the UART independently.

## Failure prevented

Without the mutex, Task A could begin transmission and be preempted by
another task using the same HAL UART handle. The second HAL call can observe
the first call's busy state and fail; an alternative unprotected byte writer
can splice text from different reports into the same stream. Task-level
mutual exclusion prevents overlapping normal transmissions and preserves
complete reports. Previously, individually protected OLED prefix/detail
calls still allowed an unrelated report between them; they now share one
mutex acquisition.

Sensor sample blocks contain several separately protected reports, so intact
Task A/B lines may legitimately appear between temperature and light lines.
That is not corrupted or interleaved text within a protected report.

In the cooperative Wokwi port, current polling UART calls do not yield during
transmission, which can hide concurrency bugs. The mutex still explicitly
protects ownership and is required for the preemptive physical build.
It must not be removed merely because a simulator run looks orderly.

## Blocking, priority inheritance, and lock ordering

A task waiting for the serial mutex blocks rather than spinning. Native
FreeRTOS mutexes provide priority inheritance: if a higher-priority task
waits, the owner can inherit its priority until release. This helps prevent
unrelated ready tasks from indefinitely delaying the owner; it does not
make the UART faster or change the cooperative port into a preemptive one.

Message formatting happens before the serial lock. HAL UART transmission
uses a 100-ms timeout per call. OLED reports can contain three bounded calls
under one acquisition. No serial mutex is held over periodic delays, OLED
I2C operations, queue waits, or sensor reads.

The existing `state_mutex` protects state/epoch snapshots and coherent event
publication. Tasks release that mutex before serial diagnostics; no code
holds both mutexes at once. This avoids a state-to-serial / serial-to-state
lock-order cycle. OLED and buzzer ownership are enforced by their tasks,
rather than multiple tasks sharing those peripherals with a mutex.

## Startup and fatal exceptions

The startup banner is written before tasks run, so no task can compete with
it. Fault hooks and assertion reporting use `serial_write_fault`, a bounded
register-polling path with interrupts disabled and no RTOS lock or heap
allocation. They terminate execution; attempting to take a mutex there can
deadlock if the stopped task already owns it. These are documented fatal
exceptions, not alternative normal task writers.

## Checks performed - 2026-10-08

Strict ARM C++11 syntax checks passed for app.cpp in both firmware
configurations. All normal task UART writes were reviewed for mutex use,
and state-mutex sections were checked for absence of serial I/O and nested
serial locking. No PlatformIO build or Wokwi simulation was run by the agent.

SensorTask now blocks for any remaining DHT22 minimum interval before reading.
This prevents a slightly early periodic deadline from publishing a not-ready
measurement and interrupting the high-temperature alarm after wake-up. State
and epoch are checked again after the wait. Actual sensor failures still reach
the display and alarm fault handling.

## User verification confirmed - 2026-10-08

The user confirmed all part checks and the corrected PIR wake-up behavior at
34 C, then authorized committing and pushing. The checklist used was:

1. Stop Wokwi, build `pio run -e bluepill_wokwi`, then start it yourself.
2. While ACTIVE, rotate the encoder, trigger motion, and alternate DHT22
   temperature between 31 C and 25.4 C. Let several sample cycles run.
3. Check Task A/B, event, motion, alarm, and sensor reports. Every report
   should remain readable, with no spliced text, partial lines, UART failures,
   mutex-release failures, or freezes.
4. Let it sleep and trigger PIR to wake it. Confirm logging, sensor readings,
   display, and alarm behavior resume as before.
5. If an OLED error is produced during later fault testing, its prefix and
   detailed error should remain together. No OLED disconnection is needed
   for the normal concurrency check above.

Visual checks cannot prove all possible schedules. The code ownership and
lock review complement Wokwi observations; preemptive hardware contention
has not been measured or tested in this part.

Milestone: `Protect serial output with mutex`.
