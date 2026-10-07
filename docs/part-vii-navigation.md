# Part VII - Rotary encoder navigation

Requirements 28 and 29: InputTask selects the `DisplayMode` enumeration:
TEMPERATURE, HUMIDITY, LIGHT, MOTION. Clockwise advances in this order;
counterclockwise reverses it. Both directions wrap around.

## Wiring and task ownership

### Wokwi capture

Following repeated user-reported stalls with rapid clicks, the Wokwi build
now leaves EXTI2/3 disabled. The existing simulator HAL time polling calls
`encoder_poll()`; the bounded DHT timing loops also sample the pins. Idle
scheduling and blocking HAL I/O already poll time frequently, so capture is
not limited to InputTask's 10-ms period. A full quadrature decoder counts
completed cycles and rejects invalid transitions. The sampler performs no
RTOS calls, logging, waiting, or display work. InputTask still owns page
selection and publishes it through the queue. The user confirmed the rapid-turn
stall was fixed after testing this version. Exact capture limits and DHT timing
have not been measured.

### Physical hardware capture

KY-040 CLK connects to PA2, DT to PA3, VCC to 3.3 V, and GND to common ground.
SW is intentionally unconnected: this part specifies rotation, not a button action.
The [Wokwi encoder reference](https://docs.wokwi.com/parts/wokwi-ky-040)
defines clockwise as CLK going low before DT.

EXTI2 captures CLK falling and reads DT for direction. The handler immediately
disables its NVIC interrupt; InputTask rearms it only after both pins return
high. EXTI3 stays disabled. This bounds repeated interrupt entry during a pulse
and avoids four interrupts per click. The handler uses no FreeRTOS APIs and
alone writes an unsigned position counter; InputTask reads it with an aligned
atomic 32-bit load. Wraparound is
handled modulo four. InputTask has priority 2, 256 stack words, and blocks for
10 ms between checks. Physical switch bounce still requires hardware testing.

InputTask publishes the latest enum through a one-item queue. DisplayTask
retains its own latest sensor sample and checks both mailboxes every 20 ms,
blocking between checks. It redraws only when a sample or page changes.
The OLED remains exclusively owned by DisplayTask. No navigation state or
sensor structures are shared unsafely between tasks.

Temperature, humidity and normalized light come from the queued sensor data.
Motion displays `Not available` because PIR acquisition is a later part.
All needed label characters and the percent sign are included in the font.

## Verification procedure

The user builds with `pio run -e bluepill_wokwi` and runs Wokwi.
Stop and reopen the simulator to load the added encoder.

1. Confirm the initial Temperature page.
2. Click the encoder's upper arrow four times, one click at a time:
   Humidity -> Light -> Motion -> Temperature.
3. Click its lower arrow four times:
   Motion -> Light -> Humidity -> Temperature.
4. Reverse direction mid-sequence and check one page per click, with no
   spontaneous changes while idle. Try repeated rotations as well.
5. Change temperature, humidity and light; check the corresponding page
   against serial output. Confirm Task A/B and sample numbers continue.
6. Confirm Motion says `Not available`, and shorter labels/values leave no
   old characters behind.

`test/navigation_test.cpp` checks pure navigation and the Wokwi quadrature
decoder. Those decoder tests do not validate physical falling-edge capture
and rearming, which require hardware checks.
This is not Wokwi verification.
The user confirmed the fix and authorized committing and pushing this milestone.

On 2026-10-08 the ARM compiler's C++11 syntax checks passed for app.cpp,
encoder.cpp, oled.cpp, and navigation_test.cpp using the Wokwi configuration.
Diagram JSON and connection component IDs were checked. The navigation test
was syntax-checked, not executed. The full PlatformIO build and simulation
are left to the user as requested.

The OLED now caches rows and sends only changed column spans. Unchanged rows
produce no I2C traffic. The cache is invalidated on initialization or transfer
failure. The final polling implementation and OLED optimization passed ARM
syntax checks. Runtime confirmation came from the user's Wokwi run.
On hardware, the temporary `Encoder:` diagnostic reports interrupt counts at most once per
simulated second when activity occurs. Rapid clicks should produce roughly one
interrupt per click, not an unbounded stream. Turns entirely within the rearm
interval can be missed; physical hardware behavior remains unverified.
The Wokwi polling path does not produce interrupt-count messages.

## Recorded user verification - 2026-10-08

After the polling change, the user reported "ok now its fixed" and requested
commit and push. This records user confirmation that the reported rapid-click
stall was resolved; no automated simulator run or measured latency is claimed.
