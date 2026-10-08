# Part XII - Task priorities

Requirements 38 and 39: assign explicit priorities and justify scheduling
urgency and acceptable latency. Higher FreeRTOS numbers mean higher priority.
Idle runs at 0; configured application priorities range from 1 to 4.
The named constants in src/app.cpp are used by every task creation call.

| Task | Priority | Timing and scheduling rationale |
|---|---|---|
| SensorTask | 3 | Samples every 2 seconds and blocks between samples. Its bounded DHT22 transaction measures pulses lasting tens of microseconds. Task-level preemption during those pulses can corrupt a frame, so it outranks the polling tasks during acquisition. |
| MotionTask | 2 | Polls every 50 ms. PIR pulses are much longer than this interval; a brief sensor transaction can delay polling without losing the pulse. Keeps the 15-second inactivity deadline and wakes consumers. |
| InputTask | 2 | Checks navigation every 10 ms. Short acquisition delays are acceptable for human input. Wokwi also polls encoder transitions during HAL time reads and DHT pulse loops; physical hardware captures falling CLK edges through EXTI. |
| AlarmTask | 2 | Blocks on its sample queue for up to 100 ms. A queued sample wakes it promptly; sampling every 2 seconds dominates temperature detection latency. The hardware PWM runs independently once enabled. |
| DisplayTask | 1 | Checks updates every 20 ms when active, skips unchanged content, and blocks between checks. Screen refresh can tolerate delays behind acquisition and state/alarm processing. |
| SensorLogTask | 1 | Blocks on the sample FIFO. Serial presentation can tolerate latency; it must not preempt acquisition or input/state processing. The FIFO holds four samples and tracks drops if output falls behind. |
| Task A and Task B | 1 | One-second diagnostics can tolerate delays behind operational work. Both block between prints and share the serial mutex. |

The handout suggests Motion/Input at 3 and Sensor/Alarm at 2, but allows
justified alternatives. This driver reads DHT22 pulses synchronously rather
than using timer input capture or DMA. SensorTask therefore remains at 3;
Motion/Input/Alarm use 2. Logging and Task A now use 1 alongside the display
and Task B. All application tasks block or have bounded peripheral operations.
A high-priority task that spins indefinitely could starve every lower task;
priority alone cannot fix that bug.

These intervals are design targets, not measured worst-case latency bounds.
UART mutex contention, bounded HAL operations, tick rounding, and other ready
tasks add delay. Serial mutex priority inheritance helps its owner finish
when a higher-priority task waits. No mutex is held through sensor sampling
or periodic task delays. Poor priority choices could interrupt DHT pulses,
delay PIR wake-up or navigation behind printing, or starve screen updates.

## Scheduler distinction

Physical firmware uses preemption at 1000 Hz: a higher-priority ready task
can preempt a lower-priority task. Equal-priority tasks time-slice.
Wokwi uses the established cooperative port at 100 Hz. Priorities choose the
next ready task at a blocking/yield point; they cannot interrupt a currently
running task. Do not change the simulator port to claim preemption. Wokwi
checks functional responsiveness; physical scheduling latency needs hardware
measurement and has not been measured here.

## Verification checklist and user approval

Strict ARM C++11 compiler checks cover both firmware configurations. No
PlatformIO build or simulation is run by the agent.

1. Build `pio run -e bluepill_wokwi`, then start Wokwi.
2. Trigger PIR to keep ACTIVE. Turn the encoder quickly in both directions:
   pages should respond and the simulator should continue advancing.
3. Change temperature to 34 C, then 25.4 C. On subsequent sensor samples,
   confirm readings update and the alarm turns on, then off. Check humidity
   and light changes too; no unexpected Error or DHT22 failure should appear.
4. Confirm sample numbers continue advancing and Task A/B messages continue
   about once per simulated second. Their order can change with scheduling.
5. Let PIR clear and wait 15 seconds with no motion: OLED switches off and
   alarm silences. Trigger PIR again: screen and readings resume; repeat at
   34 C and confirm the alarm resumes after a valid sample without cycling.

Part XII is not a separate item in the handout's commit milestone list.
After receiving the checklist above, the user explicitly requested a separate
commit and push on 2026-10-08. Commit: `Assign explicit task priorities`.
