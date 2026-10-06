# Part III — FreeRTOS foundation

Laboratory steps 17–19 introduce two simple diagnostic tasks before sensors.
Both perform finite work and block between executions.

## Task design

| Task | Responsibility | Priority | Period | Stack | Typical blocked condition |
| --- | --- | --- | --- | --- | --- |
| TaskA | Print `Task A running` | 2 | 100 ms (10 Hz) | 256 words / 1 KiB | `vTaskDelayUntil()` or serial mutex |
| TaskB | Print `Task B running` | 1 | 100 ms (10 Hz) | 256 words / 1 KiB | `vTaskDelayUntil()` or serial mutex |
| Idle | Kernel housekeeping | 0 | When no application task is ready | 128 words / 512 bytes | Normally ready |

Task A has the higher priority to make priority selection observable:
when both tasks are ready at a scheduling point, A runs first. In the Wokwi
build, scheduling points are yields and blocking calls; becoming Ready does
not interrupt the currently running task. These are demonstration priorities;
later sensor and input tasks will receive priorities based on their duties.

Each task stores its own previous wake time. `vTaskDelayUntil()` schedules
the next release relative to that time, limiting drift caused by execution
time. In contrast, `vTaskDelay()` waits relative to when it is called, adding
the work duration to each cycle. If a task overruns its period, an already
expired release does not block; neither diagnostic should overrun its
100-ms period during normal operation.

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

The project compiles STM32CubeF1's unchanged FreeRTOS V10.3.1 kernel using
`scripts/freertos.py`. The `heap_4` allocator has an
8 KiB heap. Allocation and task creation results are checked; kernel
assertions, allocation failures, and stack overflows enter a fail-stop loop.

The default `bluepill_wokwi` environment uses `ports/wokwi`: cooperative
switching with an 8 MHz HSI CPU clock to reduce simulator workload.
The 100 Hz TIM3 tick is derived from that clock. Task context uses PSP while
interrupts use MSP. Each task's 48-byte context frame preserves r4–r11,
the return address, critical-section nesting, PRIMASK, and the mask saved
by the outermost critical-section entry. PRIMASK protects critical sections.
The tick handler advances FreeRTOS by one tick and HAL time by 10 ms;
the cooperative Idle task yields and sleeps between timer events.

The physical `bluepill_f103c8` environment compiles the standard GCC
Cortex-M3 port unchanged: preemptive scheduling, SVC/PendSV switches,
and a shared 1 kHz HAL/FreeRTOS SysTick. Tickless idle is disabled in both
environments. Demonstrations of interrupt preemption require physical hardware.

### Wokwi compatibility

The first simulator run displayed the startup lines but stopped at the
FreeRTOS Cortex-M3 port's priority-width assertion (`port.c:301`). The
assertion compares the NVIC priority-register probe with `__NVIC_PRIO_BITS`,
which is 4 for STM32F103. This failure was captured in the Serial Monitor.

Masking the probe's lower four bits did not resolve the assertion. That
approach has been removed. A prior investigation of this simulated board
also reports missing NVIC priority, BASEPRI, SVC, and PendSV behavior.
The Wokwi port uses Thread-mode calls and PRIMASK instead of those mechanisms.
The kernel, task priorities, blocking delays, and mutex remain native FreeRTOS.
The simulator port does not claim to validate hardware NVIC priority widths
or demonstrate preemption. Its explicit checks cover task-mode switching,
critical-section state, timer configuration, allocation, and stack overflow.

Task output uses the exact diagnostic messages required by the laboratory.

Assertion messages include the expression and source location. Fault output
uses bounded UART register polling without HAL tick timeouts or RTOS locks,
so an assertion can report its cause even with interrupts disabled.

## Simulator verification

1. Build with `pio run` (default: `bluepill_wokwi`).
2. Press F1 in VS Code and choose **Wokwi: Start Simulator**.
3. Confirm the startup lines and repeated
   diagnostic lines:

   ```text
   BCA182 FreeRTOS Multisensor
   System starting...
   Task A running
   Task B running
   Task A running
   Task B running
   ```

4. Observe output for at least 3 simulated seconds; count approximately ten
   lines per simulated second from each task. Serial text indicates execution; it does not directly
   measure the duration spent in Ready or Blocked states.
5. Stop and restart the simulator and confirm the same behavior.

The tasks retain a 100-ms blocked period using `vTaskDelayUntil()`. Output
contains only `Task A running` and `Task B running`, without debug timestamps.
Verify recurrence using simulated time; simulation checks are performed manually.

## Verification record

Checks completed on 2026-10-06:

- `pio run -e bluepill_wokwi -e bluepill_f103c8`: both builds passed.
- Wokwi build: 8,660 bytes RAM, 11,528 bytes flash.
- Hardware build: 8,592 bytes RAM, 11,188 bytes flash.
- Five compiled ARM instruction tests passed under Unicorn: task register
  and stack restoration, nested critical-section state, existing interrupt
  mask preservation, first-task startup, and recurring application output.
- The integrated instruction test uses the real application, FreeRTOS
  kernel, mutex, heap, task selection, delays, and TIM3 HAL update handler.
  Both tasks print at ticks 0, 25, 50, and 75; HAL time reaches 750 ms.
  UART and timer setup are stubbed. Timer update calls are injected in
  Thread mode; this does not verify Wokwi's interrupt delivery, exception
  return, WFI wakeup, or UART simulation.

The Serial Monitor observations establish startup output and the repeated
priority-width assertion for the earlier integration. Wokwi task output
using the cooperative port is pending confirmation. No measured simulator
task frequency is claimed yet.

## References

- BCA182 Laboratory Activity 1, Part III, steps 17–19.
- [FreeRTOS task states](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/02-Task-states).
- Bundled FreeRTOS `task.h`, `semphr.h`, and `portable/GCC/ARM_CM3/port.c`
  in PlatformIO's STM32CubeF1 1.8.7 framework package.
- [Prior STM32/Wokwi compatibility investigation](https://github.com/monxx-ie/BCA182-freetos-multisensor#running-freertos-in-wokwi-simulator-compatibility).
