#pragma once

// Part XVII only. Select with scripts/select_fault.py; 0 is normal firmware.
#define LAB_FAULT_EXPERIMENT 0

#if LAB_FAULT_EXPERIMENT < 0 || LAB_FAULT_EXPERIMENT > 3
#error "LAB_FAULT_EXPERIMENT must be 0, 1, 2, or 3"
#endif
#if LAB_FAULT_EXPERIMENT != 0 && !defined(WOKWI_FREERTOS_PORT)
#error "Lab fault experiments are restricted to the Wokwi build"
#endif
