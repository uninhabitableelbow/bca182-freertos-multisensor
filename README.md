# bca182-freertos-multisensor

FreeRTOS-based STM32 **room multisensor** built for BCA182 Laboratory
Activity 1 (Real-Time Multisensor Room Monitoring System).

This repository is developed **step by step**, one commit per laboratory
part, so the git history documents the evolution of the system:

| Part  | Milestone                                        |
|-------|--------------------------------------------------|
| I     | Current: STM32Cube foundation, compile verification |
| II    | Next: Wokwi simulation (Blue Pill + serial message) |
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

Wokwi configuration will be added in Part II. This initial milestone has
no serial output, sensors, or FreeRTOS tasks yet.

## Step-by-step guide

See [development notes](docs/development-guide.md) for Part I instructions
and the planned milestones. The platform is pinned to `ststm32@20.0.0`.

## References and acknowledgments

Requirements: *BCA182 — Laboratory Activity No. 1: Real-Time Multisensor Room
Monitoring System*, Asst. Prof. Paul Rodolf P. Castor, September 2026.
The handout's isolated ESP-IDF mention conflicts with its required STM32Cube
framework and example; this project follows the STM32Cube requirement.
