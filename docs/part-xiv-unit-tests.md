# Part XIV - Unit testing

Requirements 42-44 call for deterministic hardware-independent tests, at
least 5 alarm / 4 navigation / 4 system-state cases, and passing pio test
results with test sources in the repository.

## Run

From the project root:

```powershell
pio test -e native
```

The explicit environment runs Windows host executables through PlatformIO
and Unity. The default firmware environment remains bluepill_wokwi so
ordinary firmware builds still use the simulator configuration. Plain
`pio test` selects that default environment and has no applicable tests;
zero tests is not a successful unit-test verification. Firmware environments
ignore host suites so this command does not upload to STM32 or start Wokwi.

Native tests compile the actual alarm.h, navigation.h, and system_state.h
logic used by firmware. test_build_src is disabled to exclude HAL startup,
RTOS tasks, and peripheral drivers. The managed MinGW package supplies the
Windows compiler; scripts/native_tests.py selects its compiler path and
links runtime libraries statically so the test executables run without
manually adding compiler DLLs to the user's PATH.

## Coverage and results - 2026-10-08

| Suite | Required | Implemented and passed | Decisions exercised |
|---|---:|---:|---|
| test_alarm | 5 | 11 | Below 18 C, exactly 18 C, normal, exactly 30 C, above 30 C; valid/invalid buzzer gating; NaN and infinities |
| test_navigation | 4 | 13 | Every forward and reverse transition including both wrap directions; full encoder cycles, bounce, invalid transitions, and rapid repeated cycles |
| test_system_state | 4 | 10 | ACTIVE before timeout, transition at timeout, inactive without motion, motion reactivation; motion at boundary, refreshed deadline, startup grace, and 32-bit clock wrap |
| Total | 13 | 34 | All suites passed |

Actual command result: `34 test cases: 34 succeeded`. Strict C++11 compiler
warnings are errors. Tests use separate named Unity cases, not screenshot
evidence or a duplicated Python implementation. Failure expectations are
asserted against production functions; a failed assertion returns a failing
test process status to PlatformIO.

The state helper computes the next activity state from motion and elapsed
time rather than taking a redundant current-state parameter. ACTIVE/timeout
and INACTIVE/no-motion therefore use expired timestamps; INACTIVE/motion
uses the same expired timestamp with motion true. MotionTask supplies the
last-motion time and refreshes it on detection in firmware.

Earlier alarm/system-state/sensor constexpr checks remain supplemental
compiler checks. The old standalone navigation assert executable is now
covered by the named Unity navigation suite, including encoder edge cases.

## Scope

These tests verify pure decisions and encoder decoding. They do not establish
PIR electrical behavior, DHT pulse timing, OLED I2C operation, mutex contention,
task scheduling latency, or simulator performance. Those remain integration
and hardware/simulation checks. No simulation or firmware build was run for
this part, and no application logic or hardware timing was changed.

The handout lists these meaningful commit milestones:

- `Add alarm unit tests`
- `Add navigation unit tests`
- `Add state machine unit tests`

The user authorized committing and pushing on 2026-10-08 after all 34 tests
passed. The native test runner setup accompanies the first unit-test milestone.

PlatformIO configuration references:
[test hierarchy](https://docs.platformio.org/en/stable/advanced/unit-testing/structure/hierarchy.html),
[test_build_src](https://docs.platformio.org/en/latest/projectconf/sections/env/options/test/test_build_src.html),
and [test_ignore](https://docs.platformio.org/en/latest/projectconf/sections/env/options/test/test_ignore.html).
