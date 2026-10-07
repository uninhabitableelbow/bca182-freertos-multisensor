# Part VI - Display subsystem

Requirements 26 and 27 are implemented by DisplayTask. It alone initializes
and writes the OLED through `oled.cpp`; other tasks never call the driver.
The initial screen reads:

```text
ROOM MONITOR
Temperature
25.4 C
```

The value comes from the DHT22, not a fixed example. Until the first sample
the screen says `Waiting...`; invalid DHT22 samples display `Error` rather
than leaving a stale temperature visible. Navigation belongs to Part VII.

## Wiring and driver

| SSD1306 pin | Blue Pill connection |
|---|---|
| VCC | 3.3 V |
| GND | Common ground |
| SCL | PB6, I2C1 clock |
| SDA | PB7, I2C1 data |

Both bus lines have 4.7k pull-ups to 3.3 V in `diagram.json`.
The [Wokwi SSD1306](https://docs.wokwi.com/parts/board-ssd1306) is 128x64
with address 0x3c. The driver uses STM32Cube HAL I2C at 100 kHz, bounded
30-ms transmit timeouts, and the controller's page addressing mode.
Commands follow the [SSD1306 datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf).
It uses a small glyph set for the initial screen and a single 129-byte row
buffer, clearing each updated row to remove old digits. Row data is sent in
16-byte chunks. Initialization explicitly selects PB6/PB7, checks for an
acknowledgement at 0x3c, and resets HAL software state before retries.
Failures report the operation and I2C diagnostics.

## Ownership and scheduling

SensorTask copies each measurement to two independent queues: the existing
four-sample serial FIFO and a one-item display mailbox. `xQueueOverwrite`
retains the newest display sample when the display is busy; it never steals
samples from the logger. DisplayTask blocks on `xQueueReceive` between
updates, normally every two simulated seconds. It has priority 1 and a
384-word stack; acquisition remains priority 3. The heap increases from
8 to 10 KiB to accommodate the additional task and queue.

Only the value row is updated after initialization, limiting I2C traffic.
I2C failure is reported over serial, followed by a two-second blocking delay
before reinitialization. Sensor acquisition and serial logging continue.
Stack margin and physical bus behavior still need runtime verification.

## User verification

1. Stop Wokwi, run `pio run -e bluepill_wokwi`, then start Wokwi yourself.
2. Check the three text lines above, with the actual sensor temperature.
3. Change DHT22 temperature and confirm the OLED and serial readings agree
   on a subsequent sample. Check a negative value and a shorter value to
   verify signs and clearing of old digits.
4. Confirm Task A/B and sensor sample messages continue without OLED errors.

Wokwi has not been run by the agent for this part. Hardware operation and
disconnect/reconnect recovery also remain unverified.

On 2026-10-08 both firmware environments compiled successfully in
`.pio/compile-check`. Diagram JSON and OLED call ownership were checked.
The Wokwi build uses 11,060 bytes of static RAM (including the RTOS heap)
and 20,060 bytes of flash. These checks do not establish runtime behavior.

Milestone: `Implement OLED display task`.

## Recorded user verification - 2026-10-08

The user's Wokwi screenshot shows `ROOM MONITOR`, `Temperature`, and
`25.4 C`, matching serial output of `25.40 C`. Sensor sample numbers advance
with zero dropped samples while Task A/B continue. The user also confirmed
that changing DHT22 temperature updates the OLED to match the terminal.
Negative-value rendering and disconnect recovery have not been confirmed.
The updated Wokwi firmware compiled successfully; its reported sizes are
11,176 bytes RAM and 21,072 bytes flash.
