# Part XX - Academic laboratory report

The separate academic report is stored at laboratory-report.pdf as required
by requirement 57. Requirements 58-59 are covered by all eight sections:
problem/requirements, system design, FreeRTOS architecture, implementation,
verification/testing, static analysis, engineering discussion, and conclusion.

The task table reflects the actual priorities, periods/events, IPC, and
typical blocked conditions rather than copying suggested values. It explains
why SensorTask is priority 3 and Motion/Input are 2. Test results are actual
host outcomes or explicitly identified user reports; unmeasured CPU usage,
latency, memory headroom, and physical behavior are not invented.

Both user screenshots appear with evidence-specific captions. Source notes
and findings remain linked in the report references. No simulator or firmware
build was run by the agent for this report.

## Generation and review

With reportlab installed, run python scripts/build_lab_report.py to regenerate
the PDF. The report was rendered into nine page images with PyMuPDF and every
page visually inspected: no clipped text, overlapping tables, blank pages,
or broken pagination was observed. Text extraction confirmed all eight
required headings and nine pages. Page previews remain in ignored
.pio/report-preview, not in the submission files.

Part XX is complete. The user authorized committing and pushing the report
and supporting files on 2026-10-08: `Add academic laboratory report`.
