"""Select one Part XVII experiment, or restore normal firmware with mode 0."""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("mode", type=int, choices=range(4),
                    help="0 normal; 1 no Task A delay; 2 Input priority 4; 3 no serial mutex")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
header = root / "include" / "lab_fault_config.h"
content = header.read_text(encoding="utf-8")
content, count = re.subn(r"^#define LAB_FAULT_EXPERIMENT [0-3]$",
                        f"#define LAB_FAULT_EXPERIMENT {args.mode}", content,
                        flags=re.MULTILINE)
if count != 1:
    raise SystemExit("Expected exactly one fault-mode definition; file was not changed")
header.write_text(content, encoding="utf-8")
print(f"Selected mode {args.mode}. Build bluepill_wokwi before starting Wokwi.")
