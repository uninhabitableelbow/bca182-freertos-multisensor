# bca182-freertos-multisensor

FreeRTOS-based STM32 **room multisensor** built for BCA182 Laboratory
Activity 1 (Real-Time Multisensor Room Monitoring System).

This repository is developed **step by step**, one commit per laboratory
part, so the git history documents the evolution of the system:

| Part  | Milestone                                        |
|-------|--------------------------------------------------|
| I     | STM32Cube foundation, successful build |
| II    | Wokwi configuration and HAL serial startup message |
| III   | Next: two simple FreeRTOS tasks |
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

1. Build with `pio run -e bluepill_f103c8`.
2. Open this repository folder in VS Code with the Wokwi extension installed.
3. Press **F1** and choose **Wokwi: Start Simulator**. If prompted, activate
   your Wokwi extension license.
4. Confirm the Serial Monitor displays:

   ```text
   BCA182 FreeRTOS Multisensor
   System starting...
   ```

The Part II circuit contains only the Blue Pill. USART1 TX (PA9) connects
to the Serial Monitor RX, and USART1 RX (PA10) connects to its TX.
Serial uses 115200 baud, 8 data bits, no parity, and one stop bit.
Sensors and FreeRTOS tasks are introduced in later milestones.

See [Part II verification](docs/part-ii-verification.md) for the checks
and current verification status.

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
