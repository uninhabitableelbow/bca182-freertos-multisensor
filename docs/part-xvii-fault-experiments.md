# Part XVII - Deliberate FreeRTOS fault experiments

Requirements 49-51 require observing three temporary faults, explaining their
effects, and restoring the correct implementation. No experiment is marked
complete until actual user observations and final restoration are recorded.

## Selection and restoration

Stop Wokwi before changing modes. Select one mode, build, then start Wokwi
yourself. The selector changes only include/lab_fault_config.h; it does not
build or simulate. The checked-in/default selection is 0, normal firmware.

```powershell
python scripts/select_fault.py 1
pio run -e bluepill_wokwi
```

Replace 1 with 2 or 3 for the other experiments. On each nonzero build, the
serial startup banner identifies the selected fault. Test only one fault
at a time. A compiler guard prevents nonzero modes on physical hardware.

Between experiments and after the last experiment, restore the normal
implementation and rebuild before starting Wokwi again:

```powershell
python scripts/select_fault.py 0
pio run -e bluepill_wokwi
```

## Experiment 1 - Remove blocking

Mode 1 omits Task A's vTaskDelayUntil while retaining its continuous loop and
serial output. Other tasks keep their intended delays and priorities.
Observe repeated Task A output, sensor sample progress, encoder/OLED response,
and simulation-clock progress for a short run. Stop the simulator if it
becomes unresponsive; do not wait for it to recover by itself.

Prediction, not a recorded result: Task A can consume all remaining CPU time
and flood serial output. In the cooperative Wokwi scheduler, a mutex taken
and immediately released without contention does not supply the missing
blocking/yield point. Task A can therefore prevent other ready tasks from
running, including higher-priority tasks that become ready later. Idle cannot
run normally either. On preemptive hardware, higher-priority ready tasks can
still preempt Task A, but lower-priority/idle work risks starvation; time
slicing can help equal-priority tasks. UART transmission also consumes time,
so repeated prints are not a numeric CPU-utilization measurement.

Restore mode 0 and confirm Task A prints about once per simulated second,
sensor samples advance, and encoder/PIR response returns.

## Experiment 2 - Change priority

Mode 2 raises InputTask from priority 2 to 4, above SensorTask at 3. Its
frequent 10-ms polling and blocking interval stay unchanged, so priority is
the only experimental variable. While ACTIVE, turn the encoder quickly and
change temperature/humidity/light. Observe sensor errors, missed or delayed
updates, alarm response, and OLED responsiveness.

Prediction: on preemptive hardware, unnecessarily urgent InputTask can
interrupt timing-sensitive DHT acquisition and delay other ready work. In
cooperative Wokwi it only changes task selection at a yield/block point;
it cannot interrupt a running DHT pulse loop. Since InputTask still blocks
every iteration, little or no visible degradation is a valid observation.
Do not claim starvation or a DHT failure unless observed. This experiment
does not establish hardware worst-case latency or justify the high priority.

Restore mode 0; InputTask returns to priority 2.

## Experiment 3 - Remove mutex

Mode 3 bypasses only serial_mutex acquisition/release in print_diagnostic.
UART error handling remains. The state mutex is unchanged. Keep ACTIVE,
turn the encoder, trigger PIR, and alternate temperature between 34 C and
25.4 C to generate competing reports. Observe intact versus spliced lines,
UART failures, freezes, and whether output differs from the normal run.

Prediction: the native preemptive scheduler permits competing task calls to
the same HAL UART handle. Without ownership protection another writer may
see HAL_BUSY or splice output, depending on the transmission path. Wokwi's
polling UART calls do not normally yield during a report, so output can
remain intact even without the mutex. That result does not prove the mutex
unnecessary. Do not insert artificial yields to fabricate interleaving.

Restore mode 0; checked mutex acquisition/release protects reports again.

## Observation record

| Experiment | Actual observation | Explanation tied to observation | Normal behavior restored? |
|---|---|---|---|
| 1 - No blocking delay | User supplied 22 consecutive Task A running lines in the terminal. Sensor progress, encoder response, and clock progress during the fault were not separately reported. | Consistent with a continuous printing loop after removing the delay. This excerpt alone does not establish starvation, responsiveness loss, or measured CPU usage. | User confirmed "ok its back to normal again" after receiving mode-0 restoration instructions. |
| 2 - InputTask priority 4 | User reported "nothing wrong"; no visible degradation reported. Individual response times were not measured. | Cooperative scheduling changes selection at yield/block points rather than preempting a running task. InputTask still blocks every 10 ms, so unchanged responsiveness is plausible; this does not establish hardware latency. | Final normal restoration confirmed by user. |
| 3 - No serial mutex | User reported "Clean output"; no interleaving reported. | Non-yielding polling UART calls in the cooperative port can conceal contention. Clean output does not justify removing the mutex from the preemptive hardware build. | User confirmed "ok its back to normal" after mode-0 restoration instructions. |

## Implementation checks

All four mode selections passed strict ARM C++11 compiler checks for the
Wokwi app/diagnostics modules. Mode 0 also passed the physical configuration
check. The selector was restored to 0 after these checks. No firmware build
or simulation was run by the agent. User-run observations are recorded above.

## Final restoration check

After mode 0 is selected and built, verify the normal startup has no LAB FAULT
banner; Task A/B recur at one second; sensor readings update; encoder pages
respond; high/normal temperature toggles the alarm; inactivity and PIR wake
work. Confirm the header says LAB_FAULT_EXPERIMENT 0 before committing.
The user confirmed normal operation after the final restoration. The agent
also checked that the source header selects mode 0. All three user-run
experiments and their explanations are recorded; detailed starvation metrics
and response-time measurements were not supplied and are not claimed.
Part XVII is complete. The user authorized committing and pushing on
2026-10-08 with normal mode 0 selected: `Document FreeRTOS fault experiments`.
