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

## References

- [Wokwi DHT22 pins and controls](https://docs.wokwi.com/parts/wokwi-dht22).
- [Aosong DHT22 datasheet](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf).
- [Official Blue Pill pin definitions](https://github.com/wokwi/wokwi-boards/blob/main/boards/stm32-bluepill/board.json).
