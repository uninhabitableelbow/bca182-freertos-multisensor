# bca182-freertos-multisensor

FreeRTOS-based STM32 **room multisensor** built for BCA182 Laboratory
Activity 1 (Real-Time Multisensor Room Monitoring System).

This repository is developed **step by step**, one commit per laboratory
part, so the git history documents the evolution of the system:

| Part  | Milestone                                        |
|-------|--------------------------------------------------|
| I     | Project initialization (STM32Cube PlatformIO project) |
| II    | Wokwi simulation (Blue Pill + serial message)     |
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

Run from the PlatformIO / Wokwi VS Code extension or the Wokwi web app.