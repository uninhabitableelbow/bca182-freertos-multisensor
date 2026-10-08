# Part XXV - Final Submission Checklist

Final review: 08 October 2026. This checklist follows requirement 65 of the
laboratory handout. Previously completed runtime checks are retained; this
review does not claim a new firmware build or simulator execution.

## Submission checks

| Category | Requirement | Status | Evidence |
|---|---|---|---|
| Build | pio run succeeds | Complete | Current Wokwi build SUCCESS confirmed before Hackster preparation; Part XXII record |
| FreeRTOS | At least five meaningful tasks | Complete | Sensor, Display, Input, Motion, Alarm, and SensorLog tasks in src/app.cpp |
| FreeRTOS | Explicit priorities | Complete | src/app.cpp; docs/part-xii-priorities.md |
| FreeRTOS | Queue | Complete | Logging FIFO and latest-value mailboxes in src/rtos_objects.cpp |
| FreeRTOS | Mutex | Complete | State and serial mutexes; docs/part-xi-mutex.md |
| FreeRTOS | Event group or task notification | Complete | ACTIVE, MOTION, ALARM events; docs/part-x-events.md |
| FreeRTOS | vTaskDelayUntil used appropriately | Complete | Sensor, Motion, and diagnostic task periodic blocking |
| FreeRTOS | ACTIVE/INACTIVE state machine | Complete | src/motion.cpp, include/system_state.h; state tests and FT-09/10 |
| Simulation | DHT22, LDR, PIR, rotary encoder, OLED, buzzer | Complete | diagram.json and docs/images/wokwi-circuit.png |
| Simulation | Wokwi simulation operational | Complete | Recorded functional verification and subsequent confirmed fixes |
| Testing | At least 13 meaningful unit tests | Complete | 34 cases: 11 alarm, 13 navigation, 10 system state |
| Testing | pio test succeeds | Complete | pio test -e native passed all 34 cases in final recheck |
| Testing | Functional verification table completed | Complete | docs/part-xvi-functional-verification.md: FT-01 through FT-10 PASS |
| Testing | Fault experiments completed | Complete | docs/part-xvii-fault-experiments.md; normal fault mode 0 restored |
| Quality | pio check completed | Complete | Final Wokwi recheck; physical configuration analysis in Part XV |
| Quality | Findings analyzed and significant warnings addressed | Complete | 0 high, 0 medium, 33 reviewed low findings; docs/part-xv-static-analysis.md |
| GitHub | Public repository | Complete | https://github.com/uninhabitableelbow/bca182-freertos-multisensor |
| GitHub | Meaningful commit history | Complete | Milestone commits; docs/part-xviii-git-history.md |
| GitHub | Professional README | Complete | README.md; retained as useful project documentation |
| GitHub | Architecture diagrams | Complete | README system, task communication, and state diagrams |
| GitHub | No unnecessary binaries/build artifacts | Complete | Tracked-file review: no .pio output, firmware binaries, or executables; report PDF is a required submission artifact |
| Report | Laboratory report included | Complete | docs/laboratory-report.pdf: nine pages, rendered and reviewed |
| Report | FreeRTOS task table | Complete | Report section 3: responsibility, priority, communication, blocking |
| Report | Requirements traceability matrix | Complete | docs/requirements-traceability.md |
| Report | Test results and static-analysis discussion | Complete | Report sections 5 and 6; supporting records in docs |
| Report | Limitations documented | Complete | Report, README, and traceability matrix distinguish simulation from physical measurements |
| Portfolio | Hackster.io article | Published link provided | https://www.hackster.io/kevinchristianvillareal/stm32-freertos-room-multisensor-a69b8a; public page contents could not be independently retrieved during this review |
| Portfolio | GitHub link and attribution included | Completed during setup | Source attachment and article references supplied during Hackster setup; final page not independently retrieved |

## Instructor guidance

Sir Castor allowed a choice between the laboratory report and the individual
technical checkoff. I selected the laboratory report, which is completed and
included in the repository. Part XXIV's technical checkoff is therefore not
required for my submission under this instruction. The existing README and
architecture diagrams remain useful project documentation.

## Final submission details

- Confirm the published page displays its GitHub source link and attribution. The project URL is now recorded.
- Instructor collaborator: Paul Rodolf P. Castor is saved, confirmed on 08 October 2026 (requirement 62).
- Selected submission option: laboratory report, completed. No separate technical checkoff is required under the instructor's stated alternative.
- Commit and push this final checklist when authorized.

## Submission package

- GitHub repository: https://github.com/uninhabitableelbow/bca182-freertos-multisensor
- Laboratory report: docs/laboratory-report.pdf
- Traceability matrix: docs/requirements-traceability.md
- Verification and analysis evidence: docs/part-xiv-unit-tests.md through docs/part-xvii-fault-experiments.md
- Circuit and demonstration captures: docs/images/wokwi-circuit.png and docs/images/finished-system.png
- Hackster public project link: https://www.hackster.io/kevinchristianvillareal/stm32-freertos-room-multisensor-a69b8a

Repository baseline reviewed: 892d8f7 (Finalize technical documentation),
matching remote main at the start of this review. No application code changes
were made for this checklist.
