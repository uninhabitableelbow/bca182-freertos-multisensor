# Part III — FreeRTOS foundation

Laboratory steps 17–19 introduce two simple diagnostic tasks before sensors.
Both perform finite work and block between executions.

## Task design

| Task | Responsibility | Priority | Period | Stack | Typical blocked condition |
| --- | --- | --- | --- | --- | --- |
| TaskA | Print `Task A running` | 2 | 1,000 ms (~1 Hz) | 256 words / 1 KiB | `vTaskDelayUntil()` or serial mutex |
| TaskB | Print `Task B running` | 1 | 1,000 ms (~1 Hz) | 256 words / 1 KiB | `vTaskDelayUntil()` or serial mutex |
| Idle | Kernel housekeeping | 0 | When no application task is ready | 128 words / 512 bytes | Normally ready |

Task A has the higher priority to make priority selection observable:
when both tasks are ready, A runs first. These are demonstration priorities;
later sensor and input tasks will receive priorities based on their duties.

Each task stores its own previous wake time. `vTaskDelayUntil()` schedules
the next release relative to that time, limiting drift caused by execution
time. In contrast, `vTaskDelay()` waits relative to when it is called, adding
the work duration to each cycle. If a task overruns its period, an already
expired release does not block; neither diagnostic should overrun its
1-second period during normal operation.

## Task states in this implementation

- **Running:** the CPU is executing a task, including its serial output.
- **Ready:** the task can execute but is waiting for the CPU. B can be ready
  while higher-priority A executes.
- **Blocked:** the task awaits its next periodic release or the serial mutex.
  A blocked task consumes no CPU; another ready task or Idle can run.
- **Suspended:** removed from scheduling until explicitly resumed. Neither
  diagnostic task is explicitly suspended.
- **Deleted:** removed by task deletion. Neither diagnostic task is deleted.

Serial polling performs bounded work while the task is running; it is not
an RTOS blocked wait. The task then enters its periodic blocked wait.
The mutex prevents overlapping UART transmissions and supports priority
inheritance if A must wait while B owns it.

## Kernel integration

The project compiles STM32CubeF1's FreeRTOS V10.3.1 kernel and its GCC
Cortex-M3 port using `scripts/freertos.py`. The `heap_4` allocator has an
8 KiB heap. Allocation and task creation results are checked; kernel
assertions, allocation failures, and stack overflows enter a fail-stop loop.

The tick rate is 1,000 Hz, matching HAL's 1 ms time base. SysTick always
increments the HAL counter and calls the kernel tick handler after the
scheduler starts. The port supplies SVC and PendSV for context switching.
Tickless idle is disabled, so HAL's millisecond counter remains consistent.

## Simulator verification

1. Build with `pio run -e bluepill_f103c8`.
2. Press F1 in VS Code and choose **Wokwi: Start Simulator**.
3. Confirm the two startup lines followed by repeated diagnostic lines:

   ```text
   BCA182 FreeRTOS Multisensor
   System starting...
   Task A running
   Task B running
   Task A running
   Task B running
   ```

4. Observe output for at least 10 seconds; count approximately one line per
   second from each task. Serial text indicates execution; it does not directly
   measure the duration spent in Ready or Blocked states.
5. Stop and restart the simulator and confirm the same behavior.

## Verification record

Checks completed on 2026-10-06:

- `pio run -e bluepill_f103c8`: passed; 8,592 bytes RAM (including the
  reserved 8 KiB kernel heap) and 8,320 bytes flash.
- ELF symbols: SVC, PendSV, SysTick, the kernel tick handler, scheduler,
  periodic delay, and failure hooks are linked as application/kernel code.
- Firmware vector table: SVC, PendSV, and SysTick entries match the linked
  handler addresses, including their Cortex-M Thumb bit.
- `git diff --check`: passed.

Simulator output is pending observation in VS Code; Part II's simulator
confirmation is also pending.
The listed frequencies and ordering describe the configured design, not
recorded runtime measurements.

## References

- BCA182 Laboratory Activity 1, Part III, steps 17–19.
- [FreeRTOS task states](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/02-Task-states).
- Bundled FreeRTOS `task.h`, `semphr.h`, and `portable/GCC/ARM_CM3/port.c`
  in PlatformIO's STM32CubeF1 1.8.7 framework package.
