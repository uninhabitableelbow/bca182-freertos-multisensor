# Part IV - Sensor subsystem

## DHT22 acquisition (step 20)

The DHT22 data pin connects to PA1, with a 4.7k resistor from data to 3.3 V.
VCC connects to `bluepill:3V3.1`, GND to `bluepill:GND.1`; NC is unused.
TIM2 runs freely at 1 MHz for pulse timing; TIM3 remains reserved for the
Wokwi scheduler. The GPIO is open-drain, so releasing it allows the sensor
to drive the line. No Arduino API or external sensor library is used.

The driver bounds every polling loop, checks the 40-bit frame checksum and
valid ranges, and decodes sign-magnitude temperature. Values are stored in
tenths, with integer formatting to avoid floating-point printf overhead.
Two decimal digits in serial output match the handout; this does not imply
0.01-degree sensor resolution. Errors print a reason without publishing
invalid data. Acquisition requires at least 2 seconds between attempts.

## SensorTask and periodic timing (steps 22-23)

SensorTask uses priority 3 and 384 stack words (1.5 KiB). It blocks for 2
seconds initially for sensor stabilization and uses `vTaskDelayUntil()`
with a 2000-ms period (0.5 Hz). Tasks A and B retain their 1-second periods
and priorities 2 and 1. The serial mutex protects each complete output block.

`vTaskDelayUntil()` calculates the next release from the previous scheduled
release. With 6 ms of acquisition/output work, releases still aim for 2.000,
4.000, 6.000 seconds. `vTaskDelay(2000)` after that work would instead aim
for 2.006, 4.012, 6.018 seconds, accumulating drift. Delays block the task;
other tasks and Idle can run. Work that overruns its period is not corrected
magically: a past deadline can cause an immediate next iteration. The DHT22
driver's minimum-interval check prevents too-frequent sensor transactions.

Pulse polling is bounded work while Running, not an RTOS blocked delay.
Interrupts stay enabled. SensorTask outranks the diagnostic tasks, but future
higher-priority work or long interrupt handlers would require revisiting
the timing-sensitive driver. Native hardware uses the normal preemptive
port; Wokwi uses the documented cooperative port.

## LDR acquisition (step 21)

The photoresistor module's AO pin connects to PA0 / ADC1 channel 0; VCC
connects to 3.3 V and GND to common ground. DO is unused. ADC1 takes one
right-aligned 12-bit conversion per SensorTask cycle. Its clock is PCLK2/6
(12 MHz on the physical board); a long sample time suits the resistor divider.
Conversion polling has a 5-ms timeout, and failure prints an error without
presenting a fabricated light value. A DHT22 failure does not skip the LDR read.

For this module, darkness increases the analog voltage. Relative light is:

```text
light_percent = round(100 * (4095 - raw_adc) / 4095)
```

Thus raw 0 maps to 100%, raw 4095 to 0%, and midscale to about 50%.
This is an inverted, normalized ADC scale, not a calibrated lux measurement
or a linear percentage of physical illumination. Serial output includes the
raw value so the conversion can be checked independently.

Physical hardware performs ADC calibration at initialization. The Wokwi
environment uses basic ADC1 conversions without the calibration operation,
consistent with its documented limited ADC support. Hardware behavior and
simulator behavior must be verified separately.

## Manual verification

Stop Wokwi, run `pio run -e bluepill_wokwi`, wait for SUCCESS, then start it
yourself. After about 2 simulated seconds, expect the diagram defaults:

```text
Temperature: 25.40 C
Humidity: 61.20 %
```

Click the DHT22 and change temperature and humidity independently. Check
updated readings on subsequent samples, including a negative temperature.
Disconnect data while stopped, run again, and check for a bounded timeout
message while diagnostic tasks continue. Restore the connection afterward.
Record actual observed values separately; no simulator run has been performed
by the coding agent for this part.

For the LDR, increase the light control and check that raw ADC decreases and
the displayed relative percentage increases. Reduce light and check the
opposite. Confirm values stay within raw 0-4095 and light 0-100%. Observe
several 2-second sensor cycles while the 1-second diagnostic tasks continue.
The initial light percentage depends on the simulated analog divider; no
fixed default percentage is claimed without a run.

## Checks performed

Both target environments compile in `.pio/compile-check`, separately from
the firmware watched by Wokwi. `test/sensor_values_compile.cpp` contains
compile-time checks for positive/negative DHT22 values, checksum corruption,
humidity range, and light conversion endpoints/midscale. These are pure
conversion checks, not a sensor or MCU simulation. The existing compiled
ARM port tests now stub sensor hardware, but were not executed for this part.
Manual sensor timing, wiring, and live values remain to be verified above.

## References

- [Wokwi DHT22 pins and controls](https://docs.wokwi.com/parts/wokwi-dht22).
- [Aosong DHT22 datasheet](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf).
- [Official Blue Pill pin definitions](https://github.com/wokwi/wokwi-boards/blob/main/boards/stm32-bluepill/board.json).
- [Wokwi photoresistor module](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor).
- [Wokwi Blue Pill peripheral support](https://docs.wokwi.com/parts/board-stm32-bluepill).
