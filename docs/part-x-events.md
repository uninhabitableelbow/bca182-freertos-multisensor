# Part X - FreeRTOS event group

Requirement 35 uses an event group for meaningful system flags. The native
FreeRTOS event-group source is compiled by `scripts/freertos.py`; the group
is created and checked before task startup. Bits are declared in
`include/system_events.h`.

| Bit | Meaning | Producer | Consumers | Set / clear rule |
|---|---|---|---|---|
| EVENT_ACTIVE, bit 0 | Application is active | Startup sets it; MotionTask owns runtime changes | SensorTask and DisplayTask wait on it; InputTask and AlarmTask gate work through snapshots; Task B observes | Set on ACTIVE; cleared on INACTIVE |
| EVENT_MOTION, bit 1 | Latest PIR output is high | MotionTask | DisplayTask Motion page; SensorTask copies motion into samples; Task B observes | Set while PIR is high; cleared when low |
| EVENT_ALARM, bit 2 | AlarmTask has enabled buzzer PWM | AlarmTask | DisplayTask alarm indicator; Task B observes | Set for a valid, fresh, current-epoch out-of-range temperature while active; cleared for normal, invalid, stale, or inactive conditions |

## Level flags and ownership

These are persistent levels, not pulses or a count of motion occurrences.
Consumers never clear them. SensorTask and DisplayTask use
`xEventGroupWaitBits(EVENT_ACTIVE, pdFALSE, pdFALSE, portMAX_DELAY)`:
they block while inactive, and both can wake from the same ACTIVE signal.
Neither consumes the signal before the other can see it. They recheck the
snapshot after waking in case another transition has happened.

MotionTask updates its state/epoch snapshot and its owned event bits under
the existing state mutex. `read_system()` reads the event flags while holding
that mutex, so consumers see consistent ACTIVE/MOTION levels and epoch data.
AlarmTask rechecks state/epoch under the mutex before updating PWM and its bit.
No state mutex is held across serial or OLED I/O. The short PWM register
update remains in AlarmTask's publication section.

AlarmTask still checks the sensor queue/state at up to 100-ms simulated
intervals, so ALARM may clear shortly after ACTIVE clears. It is cleared
when the actual PWM is silenced, not simply when the sleep decision is made.
No event-group API is called from an ISR. Software timers and a timer-service
task remain disabled; task-context event-group APIs do not require them.

The measurement queues still carry full sensor data, the mode queue carries
page selection, and the mutex still protects snapshots and epoch consistency.
The event group gives sleeping tasks a wake signal and exposes meaningful
current flags; it is not just a declared checklist object.

## Observable behavior

Task B retains its one-second diagnostic and additionally reports a changed
event snapshot, for example:

```text
Events: ACTIVE=1 MOTION=0 ALARM=0
Events: ACTIVE=1 MOTION=1 ALARM=0
Events: ACTIVE=1 MOTION=0 ALARM=1
Events: ACTIVE=0 MOTION=0 ALARM=0
```

This observer samples once per second, so it can omit brief intermediate
transitions. It never clears bits. DisplayTask shows `ALARM` on the OLED's
bottom text row while EVENT_ALARM is set, clearing the row when it clears.
The selected measurement remains the only measurement shown.

Inactive DisplayTask sends display-off and then waits for ACTIVE rather than
polling state every 100 ms. If display-off fails, it retries after a blocking
delay. SensorTask likewise waits for ACTIVE instead of polling during sleep.
InputTask continues checking encoder position to discard inactive turns.
PIR sampling, epoch checks, freshness rules, and normal periods are retained.

## Checks performed - 2026-10-08

Strict ARM C++11 syntax checks passed for app.cpp and oled.cpp in both
firmware configurations. The real bundled `event_groups.c` compiled into an
ARM object with the Wokwi configuration and software timers disabled.
The build script passed Python syntax checks. These are not a full PlatformIO
link/build or runtime verification; those steps are left to the user.

## User verification procedure

1. Stop Wokwi, build `pio run -e bluepill_wokwi`, then start Wokwi yourself.
2. At normal temperature and no motion, check
   `Events: ACTIVE=1 MOTION=0 ALARM=0`.
3. Trigger PIR motion. Check MOTION becomes 1, then returns to 0 after the
   two-second high output ends. Check the OLED Motion page agrees.
4. While active, set temperature above 30 C. After a fresh sample, check
   ALARM=1, audible buzzer, and the OLED `ALARM` indicator. Return to 25.4 C
   and check ALARM=0, silent buzzer, and the cleared indicator.
5. Set high temperature again, then let inactivity occur. Check ACTIVE=0,
   ALARM=0, OLED off, no new sensor samples, and continuing Task A/B messages.
6. Trigger PIR to wake it. Check ACTIVE=1, OLED recovery, new sensor samples,
   and alarm recovery only after a fresh sample. Both waiting tasks must wake.
7. Repeat sleep/wake and rapid encoder navigation; confirm no freeze or loss
   of page selection, and no stale alarm indicator after returning to normal.

## Recorded user verification - 2026-10-08

After following the startup, motion, alarm, sleep, wake, and repeated-operation
flow, the user reported "all goods everything works" and requested commit
and push. This records user confirmation of Part X Wokwi behavior, including
event flags and OLED alarm indication. No agent-run simulation, measured
latency, or physical hardware verification is claimed.

Milestone: `Add FreeRTOS event group`.
