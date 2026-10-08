# Part XVIII - Git and GitHub

Requirements 52 and 53 require development history organized around meaningful
engineering milestones rather than a single final upload or vague messages.

## Repository check - 2026-10-08

- Repository: https://github.com/uninhabitableelbow/bca182-freertos-multisensor
- Active branch: main.
- Before this documentation change, local HEAD and GitHub refs/heads/main
  both resolved to ec059ae0f61a35c8e5f10caf693a61ac31e56b41.
- The working directory was clean at the start of this review.
- The reachable history contains 40 commits, including engineering milestones,
  individual fixes, and earlier project initialization steps.

Remote equality was checked with git ls-remote, not inferred from a prior
push message. These are observations at review time, not a permanent claim
about future branch state.

## Required milestone mapping

The table maps the handout's labels to actual commits. Multiple commits with
the same subject exist for some milestones; this table points to the later
completion or verified-fix commit where applicable. Earlier history remains
available for review.

| Handout milestone | Commit | Actual commit subject / status |
|---|---|---|
| Initialize STM32 PlatformIO project | 622f087 | Initialize STM32 PlatformIO project |
| Configure Wokwi simulation | 271cca9 | Configure initial Wokwi simulation; descriptive wording variation |
| Add initial FreeRTOS tasks | 7395dff | Add initial FreeRTOS tasks |
| Implement DHT22 sensor acquisition | 1e65222 | Implement DHT22 sensor acquisition |
| Add LDR measurement | 5045438 | Add LDR measurement |
| Add sensor data queue | 757d64c | Add sensor data queue |
| Implement OLED display task | a6cb033 | Implement OLED display task |
| Add rotary encoder navigation | cc64e8a | Add rotary encoder navigation |
| Implement alarm task | ae98f3e | Implement alarm task |
| Add PIR motion monitoring | de63347 | Add PIR motion monitoring |
| Add system state machine | a74a1a1 | Add system state machine |
| Add FreeRTOS event group | 591e29b | Add FreeRTOS event group |
| Protect serial output with mutex | 9966061 | Protect serial output with mutex |
| Add alarm unit tests | a0ea775 | Add alarm unit tests |
| Add navigation unit tests | e1cb701 | Add navigation unit tests |
| Add state machine unit tests | ea66941 | Add state machine unit tests |
| Resolve static analysis findings | 38290e7 | Resolve static analysis findings |
| Complete Wokwi verification | 69934dc | Complete Wokwi verification |
| Finalize technical documentation | Not yet due | Pending completion and review of the remaining documentation requirements |

Additional user-approved commits document explicit priorities (1d1da3d),
module organization (f23d216), and fault experiments (ec059ae). Their subjects
describe the engineering work performed.

## Interpretation

The repository contains successive sensor, task, queue, display, input,
alarm, motion, state, synchronization, testing, analysis, and verification
milestones rather than one final bulk commit. No reachable commit subject
is exactly one of the handout's unacceptable examples: update, changes,
working, final, final2, or finalfinal. Repeated initialization and corrective
commits are visible; they are not evidence of separate completed milestones
and are not hidden or rewritten by this review.

Eighteen completed handout milestones have corresponding commits. The
nineteenth, Finalize technical documentation, must wait until the remaining
documentation is actually complete. Part XVIII's current-history review is
complete; this does not declare the remaining lab parts complete.

## Continuing workflow

1. Finish the current part and run its appropriate verification.
2. Record real test observations and limitations.
3. Commit meaningful work with a descriptive subject after user authorization.
4. Push to main after authorization and check the resulting remote state.

The user runs firmware builds and Wokwi simulations. Host unit tests and
static analysis may be run by the agent. Do not mark runtime tests passed
without user observations, and do not finalize technical documentation early.

The user authorized committing and pushing this audit on 2026-10-08:
`Document Git milestone history`. Final technical documentation remains pending.
