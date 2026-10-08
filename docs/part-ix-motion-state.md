# Part IX - Motion and system state

Requirements 31-34 add a PIR, MotionTask, and ACTIVE/INACTIVE behavior.
MotionTask is the sole state writer and applies the pure `activityState`
decision in `include/system_state.h`. A separate StateTask is recommended,
not mandatory in the handout; combining PIR observation with state ownership
keeps last-motion timing and transitions together.

## PIR and timing

PIR VCC connects to the Blue Pill 5V pin, GND to common ground, and OUT to
PA4, a digital input with a pull-down. There are no PIR interrupts.
The [Wokwi PIR reference](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor)
describes its high output duration and retriggering. The circuit configures
`delayTime` to 2 seconds and `inhibitTime` to 1.2 seconds. Repeated motion
can extend the high output. For physical hardware, check the PIR module's
supply/output specifications before using the circuit.

MotionTask samples every 50 simulated milliseconds with `vTaskDelayUntil`.
Startup is ACTIVE, with a 15-second grace period. Every high PIR reading
refreshes the last-motion time. Exactly 15 seconds after the last high
reading, the state becomes INACTIVE. Unsigned tick subtraction handles
timebase wraparound. A high PIR reading always restores ACTIVE.

Consequently, one motion trigger normally keeps the system awake for about
17 seconds: 2 seconds of high output plus 15 seconds afterward. The timeout
is not measured from the button click. MotionTask logs detected/clear changes
and ACTIVE/INACTIVE transitions.

## State ownership and task behavior

A mutex protects the shared snapshot: state, current PIR level, validity,
and a transition epoch. MotionTask is the only writer; other tasks take a
copied snapshot under the mutex. No task holds this mutex while doing I/O.
Event signaling is left to Part X.

| Behavior | ACTIVE | INACTIVE |
|---|---|---|
| MotionTask | Samples PIR and maintains timeout | Continues sampling; can wake system |
| SensorTask | Reads DHT22/LDR every 2 seconds | Pauses acquisition; checks state every 100 ms |
| DisplayTask | Displays selected measurement | Sends display-off once; checks state every 100 ms, no row updates |
| InputTask | Publishes page changes | Discards encoder turns; preserves selected page |
| AlarmTask | Evaluates fresh valid temperature | Silences buzzer; checks state at least every 100 ms |
| Task A/B | One-second serial diagnostics | Continue as scheduler diagnostics |

MotionTask has priority 2 and a 256-word stack. The existing time-sensitive
SensorTask retains priority 3, preventing PIR polling/serial diagnostics from
interrupting DHT pulse timing. Its bounded acquisition can delay motion
handling briefly. AlarmTask/InputTask remain priority 2 and DisplayTask 1.
The Wokwi port remains cooperative; response limits need runtime measurement.

On reactivation, DisplayTask enables the OLED and redraws the selected page.
Sensor messages carry the transition epoch; display and alarm reject data
from before a sleep/wake transition and wait for a fresh active sample.
Motion on the OLED uses the latest state snapshot rather than waiting for
the two-second sensor cycle. Invalid/stale alarm data keeps the buzzer silent.
SensorTask resets its periodic baseline after sleeping to avoid catch-up reads.

The RTOS heap increases to 14 KiB for MotionTask and the snapshot mutex.
Allocation and task creation remain checked. The full build's memory report
and runtime stack margins still need verification.

## Checks performed - 2026-10-08

ARM C++11 syntax checks passed with warnings treated as errors for app.cpp,
pir.cpp and oled.cpp in both firmware configurations. All 10
compiler-evaluated assertions in `test/system_state_compile.cpp` passed:
startup, before/exact/after timeout, motion reactivation, refresh, and tick
wraparound. Diagram JSON and component endpoints passed validation.
These checks do not verify the runtime behavior below.

## User verification

Stop Wokwi, run `pio run -e bluepill_wokwi`, then reopen Wokwi to load the PIR.
Use simulated time, not wall-clock time, for the checks.

1. Startup: confirm `System: ACTIVE`, OLED readings, encoder navigation, and
   normal alarm behavior. The Motion page initially shows `None`.
2. Without triggering PIR, wait about 15 simulated seconds. Confirm
   `System: INACTIVE`, OLED off, buzzer silent, and sensor sample numbers
   stop advancing after any already-acquired serial message drains.
   Task A/B should continue.
3. Rotate the encoder while inactive. Click PIR, then `Simulate Motion`.
   Confirm `Motion: detected` and `System: ACTIVE`, OLED wakes on the same
   selected page, new sensor samples resume, and navigation works again.
4. Select Motion. Trigger PIR and confirm `Detected`; after its configured
   high duration, confirm `None` and `Motion: clear`.
5. Trigger motion again before the inactivity timeout. Confirm the system
   stays ACTIVE until 15 seconds after the latest high PIR output ends.
6. Set temperature above 30 C and confirm the buzzer sounds while ACTIVE.
   Let the system become INACTIVE and confirm it stops. Trigger PIR and
   confirm it sounds again only after a fresh high-temperature sample.
7. Repeat sleep/wake and rapid navigation. Confirm no freeze, lost display
   recovery, accumulated inactive turns, or unexpected alarm activation.

## Recorded user verification - 2026-10-08

After receiving the sleep/wake, Motion page, inactive encoder, and alarm
checklist, the user reported "all works" and authorized commit and push.
This records user confirmation of Part IX runtime behavior. No agent-run
simulation, measured response latency, or physical hardware test is claimed.

Milestones: `Add PIR motion monitoring` and `Add system state machine`.
