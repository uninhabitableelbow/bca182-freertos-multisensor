# Part XVI - Functional verification in Wokwi

Requirements 47 and 48 specify FT-01 through FT-10 and require an actual
observation before any test is marked PASS. This record is for the current
firmware after Parts XIII-XV, not an inferred pass from earlier checks.

## Preparation

1. Stop Wokwi and build `pio run -e bluepill_wokwi` yourself.
2. Start Wokwi. Confirm startup output, recurring Task A/B messages, and
   sensor samples. Trigger PIR as needed to keep ACTIVE during FT-01-08.
3. Navigate to Temperature and begin at 25.4 C, humidity 61.2 percent.
4. Wait for new samples after each sensor change (nominally every two
   simulated seconds). Use Wokwi's clock, not wall-clock time, when waiting.
5. Record what is actually displayed, heard, or printed. If a result differs
   from expected, record it as FAIL with the observed behavior. Leave a test
   PENDING if it has not been performed.

## Test flow and verification record

The input column gives planned stimuli. If different inputs are used,
update that column to the actual inputs along with the observation.

| Test ID | Input / stimulus | Expected | Actual observation | Result |
|---|---|---|---|---|
| FT-01 | On Temperature page, change DHT22 temperature from 25.4 C to 28 C | OLED changes to 28.0 C; serial sample reflects 28 C | User confirmed displayed temperature updates (user report, 2026-10-08) | PASS |
| FT-02 | On Humidity page, change humidity from 61.2 to 75 percent | OLED changes to 75.0 percent; serial sample reflects 75 percent | User confirmed displayed humidity updates (user report, 2026-10-08) | PASS |
| FT-03 | On Light page, change illumination from 500 lux to 0.8 lux, then 10000 lux | Relative light percentage decreases in darkness and increases in bright light; serial ADC changes | User confirmed light value changes from dark to bright (user report, 2026-10-08) | PASS |
| FT-04 | From Temperature, rotate clockwise four clicks | Humidity, Light, Motion, Temperature in order | User confirmed clockwise navigation and wraparound work (user report, 2026-10-08) | PASS |
| FT-05 | From Temperature, rotate counterclockwise four clicks | Motion, Light, Humidity, Temperature in order | User confirmed counterclockwise navigation and wraparound work (user report, 2026-10-08) | PASS |
| FT-06 | While ACTIVE, set temperature to 34 C and wait for a new sample | Audible buzzer tone, OLED ALARM indicator, serial HIGH_TEMPERATURE / buzzer ON | User confirmed high temperature activates buzzer and OLED alarm (user report, 2026-10-08) | PASS |
| FT-07 | Return temperature to 25.4 C and wait for a new sample | Buzzer silent, OLED ALARM indicator clears, serial NORMAL / buzzer OFF | User confirmed normal temperature stops buzzer and clears alarm (user report, 2026-10-08) | PASS |
| FT-08 | Select Motion page and trigger PIR while ACTIVE | Motion page shows Detected while PIR is high; system remains ACTIVE; serial Motion: detected | User confirmed PIR detection and continued ACTIVE operation (user report, 2026-10-08) | PASS |
| FT-09 | Stop triggering PIR, let Motion: clear occur, then wait 15 simulated seconds | Serial System: INACTIVE; OLED switches off; buzzer silent; PIR monitoring remains available | User confirmed inactivity switches system to INACTIVE and OLED off (user report, 2026-10-08) | PASS |
| FT-10 | Trigger PIR while INACTIVE | Serial System: ACTIVE; OLED wakes, sensor samples resume, encoder pages work | User confirmed PIR wakes system and readings resume (user report, 2026-10-08) | PASS |

FT-01-03 require choosing the matching OLED page. Light is an inverted ADC
percentage, not lux displayed as a percentage; record the actual percentages
and ADC values rather than expecting the illumination control's lux number
on the screen. Unknown/unavailable measurements must not be counted as
successful updates.

FT-04/05 exercise both wrap directions. Record the actual page sequence,
and report any missed clicks or simulator freeze. The encoder button is
not used for page navigation.

For FT-06, enable simulator audio if needed. A serial ON line alone does not
establish that the buzzer sounds. Confirm the OLED indicator too. Keep PIR
active while testing alarms so inactivity does not deliberately silence them.

For FT-08, an already ACTIVE system need not print another System: ACTIVE
line; Motion: detected plus a lit OLED demonstrates continued activity.
The Motion page can show None again once the PIR pulse ends.

For FT-09, the configured PIR pulse lasts about two seconds. MotionTask
refreshes last_motion while the signal remains high, so the 15-second wait
starts when that signal clears. A single trigger can therefore take about
17 simulated seconds to lead to INACTIVE. This is application inactivity;
diagnostic Task A/B messages may continue while the OLED is off.

For FT-10, the chosen page is preserved. A sensor page may briefly show
Waiting until a valid sample arrives. If the temperature remains outside
18-30 C, an alarm can resume after a valid sample; set 25.4 C for this basic
wake test. Repeated Error or alarm cycling is a failure, not normal wake-up.

## Completion

After receiving the FT-01 through FT-10 flow, the user reported
"no failurs all working great" and authorized commit/push on 2026-10-08.
The PASS results below record that user confirmation of the listed expected
behaviors. No separate measured values, screenshots, or timestamps were
supplied. Planned stimulus values are retained as the checklist inputs;
they are not independently transcribed measurements. The agent did not run
the firmware build or simulation. Milestone: `Complete Wokwi verification`.
