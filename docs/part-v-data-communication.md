# Part V - Data communication

## Requirements 24 and 25

`include/sensor_data.h` defines the handout's `SensorData` fields:
temperature (Celsius), humidity (percent), lightLevel (normalized percent),
and motionDetected (boolean). Motion acquisition belongs to a later part;
`motion_valid` is false, so the placeholder false is not a measured absence
of motion.

SensorTask -> FreeRTOS sensor queue -> SensorLogTask -> serial output

SensorTask retains priority 3, a 384-word stack, and its 2-second
`vTaskDelayUntil()` period after startup stabilization. It gathers measurements
into a local `SensorMessage` containing `SensorData`, validity/error status,
raw ADC value, sequence number, and cumulative dropped-sample count.
It no longer prints sensor readings directly.

The queue holds four complete messages, copied by value. No pointers to task
stack data or unsynchronized global measurement variables are shared.
The queue handle is initialized before the scheduler starts. Allocation and
task creation failures are checked.

SensorLogTask has priority 2 and a 384-word stack. It blocks on
`xQueueReceive(..., portMAX_DELAY)` when the queue is empty, then formats the
received readings using the existing serial mutex. Task A and Task B retain
their one-second diagnostic periods.

Sending uses zero wait time. If the FIFO is full, the new sample is dropped
and a producer-local counter increases; acquisition keeps its schedule.
The next accepted message exposes the cumulative count and sequence gap.
Driver failures are also queued, and the consumer prints the error instead
of displaying invalid zero readings.

A queue delivers each item to one receiver, not every receiver. Later display
and alarm tasks will need explicit fan-out (for example, separate queues)
if both must receive every sample. They are not implemented in Part V.

## User-run verification

1. Stop Wokwi, run `pio run -e bluepill_wokwi`, then start it yourself.
2. After stabilization, expect `Sensor sample #1 (dropped: 0)` and the
   temperature, humidity, and light lines. Sample numbers should increase
   every two simulated seconds; Task A/B messages continue every second.
3. Change temperature, humidity, and light controls. Confirm subsequent
   queued samples show the changes.
4. Check several samples for increasing sequence numbers and `dropped: 0`.

On 2026-10-08, both `bluepill_f103c8` and `bluepill_wokwi` compiled successfully
in `.pio/compile-check`, separately from the active Wokwi firmware.
Runtime queue behavior and overflow experiments remain
unverified until actual execution; compilation alone does not establish them.

Milestone commit: `Add sensor data queue`.
