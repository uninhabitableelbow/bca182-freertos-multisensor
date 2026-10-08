# bca182-freertos-multisensor

FreeRTOS-based STM32 **room multisensor** built for BCA182 Laboratory
Activity 1 (Real-Time Multisensor Room Monitoring System).

This repository is developed **step by step**, commits at the listed engineering
milestones, so the git history documents the evolution of the system:

| Part  | Milestone                                        |
|-------|--------------------------------------------------|
| I     | STM32Cube foundation, successful build |
| II    | Wokwi configuration and HAL serial startup message |
| III   | Two periodic FreeRTOS diagnostic tasks with blocking delays |
| IV    | DHT22 and LDR acquisition in a 2-second SensorTask |
| V     | SensorData messages sent through a FreeRTOS queue to SensorLogTask |
| VI    | DisplayTask owns the OLED and shows queued temperature readings |
| VII   | Rotary encoder navigation with Wokwi polling; user confirmed the rapid-turn fix |
| VIII  | Pure temperature alarm logic and PWM buzzer task; user verified |
| ...   | (to be completed as the lab progresses)           |

## Repository layout

```
include/   Public headers
lib/       Third-party / local libraries
src/       Application source (+ main entry point)
test/      Unit tests
docs/      Project documentation
```

## Build

```
pio run
```

## Simulation (Wokwi)

1. Build with `pio run` (default environment: `bluepill_wokwi`).
2. Open this repository folder in VS Code with the Wokwi extension installed.
3. Press **F1** and choose **Wokwi: Start Simulator**. If prompted, activate
   your Wokwi extension license.
4. Confirm the Serial Monitor displays:

   ```text
   BCA182 FreeRTOS Multisensor
   System starting...
   Task A running
   Task B running
   ```

The circuit includes a DHT22 on PA1 with a 4.7k pull-up and an LDR module
on PA0 (ADC1 channel 0). Both sensors use 3.3 V and common ground. USART1 TX (PA9) connects
to the Serial Monitor RX, and USART1 RX (PA10) connects to its TX.
Serial uses 115200 baud, 8 data bits, no parity, and one stop bit.
The distinct task messages repeat every 1000 firmware milliseconds (1 Hz each). These are simulated-time intervals;
slow simulation can make them take longer in wall-clock time. Task A has priority 2,
Task B priority 1; both use `vTaskDelayUntil()` between executions.
A mutex protects their shared USART1 output. SensorTask (priority 3) reads temperature, humidity, and relative light every 2 seconds after startup stabilization.
See [Part IV sensor notes](docs/part-iv-sensors.md) for wiring and manual checks.
SensorTask now sends complete samples through a four-item queue to SensorLogTask,
which blocks until data arrives and prints it. See [Part V communication notes](docs/part-v-data-communication.md)
for message validity, queue behavior, and verification steps.
DisplayTask receives a separate latest-sample queue and owns the SSD1306 OLED
on PB6/PB7. See [Part VI display notes](docs/part-vi-display.md) for wiring
and the user-run OLED checks.
See [Part VII navigation](docs/part-vii-navigation.md) for the encoder and
the clockwise/counterclockwise verification checklist.
AlarmTask receives its own sensor mailbox and controls the buzzer on PB8.
The normal temperature range is 18-30 C inclusive. See
[Part VIII alarm notes](docs/part-viii-alarm.md) for thresholds and verification.

See [Part II verification](docs/part-ii-verification.md) for the checks
and current verification status.

## FreeRTOS foundation

The build uses STM32CubeF1's bundled FreeRTOS V10.3.1 kernel and `heap_4`
allocator. [The build script](scripts/freertos.py) compiles the required
sources directly from the pinned PlatformIO framework package.
Native APIs are used throughout; no CMSIS-RTOS or Arduino wrapper is used.

- `bluepill_wokwi` uses a simulator port with cooperative Thread-mode
  switching at an 8 MHz HSI CPU clock to reduce simulator workload,
  and a 100 Hz kernel tick derived from a polled TIM3 counter. Higher-priority ready tasks run
  at the next yield or blocking call. This build cannot demonstrate interrupt
  preemption. Each application task must perform bounded work and block.
- `bluepill_f103c8` uses the unmodified GCC Cortex-M3 port with preemption
  and a 1 kHz SysTick. Build it for physical hardware with
  `pio run -e bluepill_f103c8`.

The simulator reads TIM3 at 1-ms resolution for HAL time. At safe yields,
it advances FreeRTOS by one tick per 10 elapsed ms without a timer interrupt. See [Part III scheduling notes](docs/part-iii-scheduling.md)
for task priorities, periods, states, and the simulator verification procedure.

## Port verification

`test/test_wokwi_context.py` executes the compiled ARM instructions under
Unicorn. Its tests cover task-register/stack preservation, nested
critical sections, interrupt-mask restoration, first-task startup, and
recurring Task A/B output using the real FreeRTOS kernel. Peripheral I/O and
timer counter progression are modeled; this is separate from Wokwi runtime verification.

```sh
python -m pip install --target .pio/verification-deps unicorn==2.1.4 pyelftools
pio run -e bluepill_wokwi
python -m unittest discover -s test -p test_wokwi_context.py -v
```

## Step-by-step guide

See [development notes](docs/development-guide.md) for milestone instructions
and the planned milestones. The platform is pinned to `ststm32@20.0.0`.

## References and acknowledgments

Requirements: *BCA182 — Laboratory Activity No. 1: Real-Time Multisensor Room
Monitoring System*, Asst. Prof. Paul Rodolf P. Castor, September 2026.
The handout's isolated ESP-IDF mention conflicts with its required STM32Cube
framework and example; this project follows the STM32Cube requirement.

- [Wokwi Blue Pill reference](https://docs.wokwi.com/parts/board-stm32-bluepill)
- [Wokwi project configuration](https://docs.wokwi.com/vscode/project-config)
- [Wokwi Serial Monitor wiring](https://docs.wokwi.com/guides/serial-monitor)
- [Prior investigation of STM32/Wokwi FreeRTOS compatibility](https://github.com/monxx-ie/BCA182-freetos-multisensor#running-freertos-in-wokwi-simulator-compatibility)
