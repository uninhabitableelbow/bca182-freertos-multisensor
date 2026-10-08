# Part VIII - Alarm subsystem

Requirement 30 separates temperature decisions from buzzer hardware control.
`include/alarm.h` defines the specified `AlarmState` enumeration and pure
`evaluateTemperature(float)` function. It includes no HAL or FreeRTOS headers.
The lab's normal range is inclusive: 18 through 30 degrees Celsius.

| Temperature | AlarmState | Buzzer |
|---|---|---|
| Below 18 C | LOW_TEMPERATURE | On |
| 18 to 30 C, including both limits | NORMAL | Off |
| Above 30 C | HIGH_TEMPERATURE | On |

The decision function expects a valid finite measurement. AlarmTask checks
DHT22 status and `validAlarmTemperature` before calling it. Invalid values
are not reported as NORMAL; they produce an explicit invalid-reading message
and silence the buzzer. Returning to valid data resumes the normal decision.

## Communication and ownership

SensorTask publishes each complete SensorMessage to a separate one-item alarm
mailbox with `xQueueOverwrite`. Display and serial queues continue to receive
their own copies. AlarmTask alone initializes and controls the buzzer and
TIM4 channel 3. Its priority is 2 and stack is 256 words. Sensor acquisition
at priority 3 completes first; alarm decisions then run before lower-priority
OLED work. In Wokwi, task switching remains cooperative.

AlarmTask blocks on its queue, with a three-second timeout. Samples normally
arrive every two simulated seconds after startup stabilization. If samples
stop arriving for three seconds, the buzzer is silenced and a missing-sample
message is logged. This is a documented fault policy, not a fourth temperature
alarm state. Serial diagnostics are protected by the existing mutex and
reported only when the alarm/fault classification changes.

The FreeRTOS heap increases from 10 to 12 KiB for the additional task and
mailbox. Allocation and task-creation failures are checked. Runtime stack
margin and the final firmware memory report still need checking.

## Buzzer wiring and tone

The [Wokwi piezo buzzer](https://docs.wokwi.com/parts/wokwi-buzzer) has its
positive pin 2 connected to PB8 and negative pin 1 to common ground.
The diagram selects smooth audio mode and volume 0.1.
`src/buzzer.cpp` uses STM32Cube HAL PWM on TIM4 channel 3 at 2 kHz and
50 percent duty when enabled; zero duty is silent. The timer uses a 1 MHz
counter at both the 8 MHz simulator clock and the 72 MHz hardware clock.
There is no software tone loop or tone interrupt. TIM2 remains assigned
to DHT22 timing and TIM3 to the Wokwi timebase.

ACTIVE/INACTIVE behavior and event signaling belong to later parts and are
not introduced here. The alarm currently evaluates every valid sensor sample
regardless of which OLED page is selected.

## Checks performed - 2026-10-08

ARM C++11 syntax checks passed for app.cpp and buzzer.cpp with warnings
treated as errors in both Wokwi and hardware configurations. All 15
compiler-evaluated assertions in `test/alarm_compile.cpp` passed. They cover
inclusive boundaries, just-outside values, sensor extremes, NaN/infinity
rejection, and buzzer enable decisions. Diagram JSON and endpoints passed
validation. No full PlatformIO build or simulation was run by the agent.

## User verification procedure

Stop Wokwi, build with `pio run -e bluepill_wokwi`, then reopen Wokwi to
load the buzzer. Allow the next sensor sample after each temperature change:

| Set DHT22 to | Expected serial classification | Expected sound |
|---|---|---|
| 25.4 C | NORMAL | Silent |
| 17.9 C | LOW_TEMPERATURE | Tone |
| 18.0 C | NORMAL | Silent |
| 30.0 C | NORMAL | Silent |
| 30.1 C | HIGH_TEMPERATURE | Tone |
| 25.4 C again | NORMAL | Silent |

While the alarm is sounding, turn the encoder in both directions and change
humidity/light. Confirm navigation remains responsive, samples continue,
and humidity/light changes alone do not determine the alarm. Use the OLED
temperature page or serial temperature to establish that the new value was
actually sampled. If audio is blocked, enable sound manually in the browser
or extension; a serial ON message alone does not verify audible output.

Fault-policy checks: temporarily disconnect DHT22 data and confirm an
invalid-temperature diagnostic silences the buzzer; reconnect it and confirm
normal decisions resume. A deliberate stopped-producer test is needed to
verify the three-second missing-sample timeout, separately from DHT errors.
The user did not provide separate fault-injection results or physical hardware
results; those checks have no recorded evidence.

## Recorded user verification - 2026-10-08

After receiving the threshold checklist, return-to-normal check, and encoder
responsiveness check, the user reported "all is verified it works" and
authorized commit and push. This records user confirmation of the Part VIII
Wokwi checks; no agent-run simulation or measured timing is claimed.

Milestone: `Implement alarm task`.
