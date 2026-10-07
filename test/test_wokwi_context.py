"""Exercise the compiled Wokwi port's ARM instructions, not a Python model.

Build bluepill_wokwi first. Requires pyelftools and Unicorn 2.1.4; temporary
verification dependencies can live under the ignored .pio/verification-deps.
"""
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / ".pio/verification-deps"))
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import (
    UC_ARM_REG_CONTROL, UC_ARM_REG_MSP, UC_ARM_REG_PSP,
    UC_ARM_REG_PRIMASK, UC_ARM_REG_PC, UC_ARM_REG_LR, UC_ARM_REG_R0,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
    UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11,
)

SAVED_REGISTERS = [UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
                   UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11]


class ContextSwitchTests(unittest.TestCase):
    def setUp(self):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        self.cpu.mem_map(0x08000000, 0x10000)
        self.cpu.mem_map(0x20000000, 0x10000)
        self.cpu.mem_map(0xE0000000, 0x100000)
        with (ROOT / ".pio/build/bluepill_wokwi/firmware.elf").open("rb") as stream:
            elf = ELFFile(stream)
            self.symbols = {s.name: s["st_value"] for s in elf.get_section_by_name(".symtab").iter_symbols()}
            for segment in elf.iter_segments():
                if segment["p_type"] == "PT_LOAD" and segment["p_filesz"]:
                    self.cpu.mem_write(segment["p_vaddr"], segment.data())
        self.tcb_a, self.tcb_b = 0x20003000, 0x20003100
        self.sp_a, self.sp_b = 0x20006000, 0x20007000
        self.return_a, self.return_b = 0x0800F000, 0x0800F004
        self.cpu.reg_write(UC_ARM_REG_MSP, 0x20008000)
        self.cpu.reg_write(UC_ARM_REG_PSP, self.sp_a)
        self.cpu.reg_write(UC_ARM_REG_CONTROL, 2)

    def write_word(self, address, value):
        self.cpu.mem_write(address, struct.pack("<I", value))

    def read_word(self, address):
        return struct.unpack("<I", self.cpu.mem_read(address, 4))[0]

    def set_state(self, nesting, current_mask, outer_mask):
        self.write_word(self.symbols["critical_nesting"], nesting)
        self.write_word(self.symbols["saved_outer_mask"], outer_mask)
        self.cpu.reg_write(UC_ARM_REG_PRIMASK, current_mask)

    def switch(self, destination, stop_address):
        def intercept(cpu, address, size, user_data):
            if address == (self.symbols["vTaskSwitchContext"] & ~1):
                self.write_word(self.symbols["pxCurrentTCB"], destination)
                cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))
            elif address == stop_address:
                cpu.emu_stop()
        hook = self.cpu.hook_add(UC_HOOK_CODE, intercept)
        self.cpu.emu_start(self.symbols["prvSwitchContext"] | 1, 0, count=300)
        self.cpu.hook_del(hook)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PC), stop_address)

    def round_trip(self, nesting, current_mask, outer_mask):
        registers_a = [0xA0000000 + i for i in range(8)]
        registers_b = [0xB0000000 + i for i in range(8)]
        frame_b = registers_b + [self.return_b | 1, 0, 0, 0]
        self.cpu.mem_write(self.sp_b - 48, struct.pack("<12I", *frame_b))
        self.write_word(self.tcb_b, self.sp_b - 48)
        self.write_word(self.symbols["pxCurrentTCB"], self.tcb_a)
        self.set_state(nesting, current_mask, outer_mask)
        for reg, value in zip(SAVED_REGISTERS, registers_a):
            self.cpu.reg_write(reg, value)
        self.cpu.reg_write(UC_ARM_REG_LR, self.return_a | 1)
        self.switch(self.tcb_b, self.return_b)
        self.assertEqual([self.cpu.reg_read(r) for r in SAVED_REGISTERS], registers_b)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PSP), self.sp_b)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PRIMASK), 0)
        self.assertEqual(self.read_word(self.symbols["critical_nesting"]), 0)
        self.assertEqual(self.read_word(self.tcb_a), self.sp_a - 48)

        self.cpu.reg_write(UC_ARM_REG_LR, self.return_b | 1)
        self.switch(self.tcb_a, self.return_a)
        self.assertEqual([self.cpu.reg_read(r) for r in SAVED_REGISTERS], registers_a)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PSP), self.sp_a)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PSP) % 8, 0)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_MSP), 0x20008000)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PRIMASK), current_mask)
        self.assertEqual(self.read_word(self.symbols["critical_nesting"]), nesting)
        self.assertEqual(self.read_word(self.symbols["saved_outer_mask"]), outer_mask)

    def test_task_registers_and_stack_survive_switch(self):
        self.round_trip(0, 0, 0)

    def test_yield_inside_nested_critical_section_preserves_mask(self):
        self.round_trip(2, 1, 0)

    def test_preexisting_interrupt_mask_is_preserved(self):
        self.round_trip(1, 1, 1)

    def test_first_task_starts_on_psp_without_changing_msp(self):
        frame = [0x4000 + i for i in range(8)] + [self.return_b | 1, 0, 0, 0]
        self.cpu.mem_write(self.sp_b - 48, struct.pack("<12I", *frame))
        self.write_word(self.tcb_b, self.sp_b - 48)
        self.write_word(self.symbols["pxCurrentTCB"], self.tcb_b)
        self.cpu.reg_write(UC_ARM_REG_CONTROL, 0)
        self.cpu.reg_write(UC_ARM_REG_PRIMASK, 1)
        self.cpu.emu_start(self.symbols["prvStartFirstTask"] | 1, self.return_b, count=150)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PC), self.return_b)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_CONTROL), 2)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PSP), self.sp_b)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_MSP), 0x20008000)
        self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PRIMASK), 0)

    def test_hal_time_handles_counter_wrap_without_advancing_kernel(self):
        self.cpu.mem_map(0x40000000, 0x30000)
        self.write_word(self.symbols["tick_timer"], 0x40000400)
        self.write_word(self.symbols["timer_running"], 1)
        self.cpu.mem_write(self.symbols["last_timer_count"], struct.pack("<H", 65530))
        self.write_word(self.symbols["uwTick"], 0xFFFFFFF0)
        self.write_word(self.symbols["xTickCount"], 117)
        self.cpu.reg_write(UC_ARM_REG_PRIMASK, 1)

        # Counter wrap contributes eleven milliseconds; the second sample
        # wraps the uint32_t HAL clock. Neither read may change kernel lists
        # or ticks, and both must preserve the caller's interrupt mask.
        for counter, expected_time in [(5, 0xFFFFFFFB), (12, 2), (12, 2)]:
            self.write_word(0x40000424, counter)  # TIM3 CNT.
            self.cpu.reg_write(UC_ARM_REG_LR, self.return_a | 1)
            self.cpu.emu_start(self.symbols["HAL_GetTick"] | 1,
                               self.return_a, count=300)
            self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PC), self.return_a)
            self.assertEqual(self.cpu.reg_read(UC_ARM_REG_R0), expected_time)
            self.assertEqual(self.read_word(self.symbols["uwTick"]), expected_time)
            self.assertEqual(self.read_word(self.symbols["xTickCount"]), 117)
            self.assertEqual(self.cpu.reg_read(UC_ARM_REG_PRIMASK), 1)

    def test_kernel_tasks_repeat_after_periodic_blocking(self):
        # Exercise the real app, mutex, heap, task creation, tick processing,
        # task selection and ARM port. Only peripheral setup/I/O are stubbed.
        # An Idle call advances TIM3's counter by ten milliseconds. The real
        # port samples that counter at the next yield; no timer ISR or fake
        # task-selection hook is involved in this test.
        self.cpu.mem_map(0x40000000, 0x30000)
        self.write_word(0x40021004, 0)  # Simulation uses undivided 8 MHz HSI.
        self.cpu.reg_write(UC_ARM_REG_CONTROL, 0)
        self.cpu.reg_write(UC_ARM_REG_PRIMASK, 0)
        self.cpu.reg_write(UC_ARM_REG_LR, self.return_a | 1)
        messages = []
        ticks = 0

        def return_from_call(cpu, result=0):
            cpu.reg_write(UC_ARM_REG_R0, result)
            cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))

        def read_string(address):
            value = bytearray()
            while True:
                char = self.cpu.mem_read(address, 1)[0]
                if char == 0:
                    return value.decode()
                value.append(char)
                address += 1

        def intercept(cpu, address, size, user_data):
            nonlocal ticks
            if address == (self.symbols["rtos_assert_failed"] & ~1):
                self.fail("Kernel assertion: " + read_string(cpu.reg_read(UC_ARM_REG_R0)))
            if address == (self.symbols["dht22_init"] & ~1):
                return_from_call(cpu, 1)
            elif address == (self.symbols["ldr_init"] & ~1):
                return_from_call(cpu, 1)
            elif address == (self.symbols["ldr_read"] & ~1):
                return_from_call(cpu, 0)  # No ADC hardware in this port test.
            elif address == (self.symbols["dht22_read"] & ~1):
                return_from_call(cpu, 1)  # Sensor not ready; hardware tested manually.
            elif address == (self.symbols["_Z11serial_initv"] & ~1):
                return_from_call(cpu)
            elif address == (self.symbols["_Z12serial_writePKc"] & ~1):
                message = read_string(cpu.reg_read(UC_ARM_REG_R0))
                messages.append((ticks, message))
                return_from_call(cpu)
                if sum("Task" in m for _, m in messages) >= 8:
                    cpu.emu_stop()
            elif address == (self.symbols["_Z18serial_write_faultPKc"] & ~1):
                return_from_call(cpu)
            elif address == (self.symbols["HAL_RCC_GetPCLK1Freq"] & ~1):
                return_from_call(cpu, 8000000)
            elif address in [(self.symbols[name] & ~1) for name in ["HAL_TIM_Base_Init", "HAL_TIM_Base_Start"]]:
                return_from_call(cpu)
            elif address == (self.symbols["vApplicationIdleHook"] & ~1):
                counter = self.read_word(0x40000424)  # TIM3 CNT.
                self.write_word(0x40000424, (counter + 10) & 0xFFFF)
                ticks += 1
                return_from_call(cpu)

        hook = self.cpu.hook_add(UC_HOOK_CODE, intercept)
        self.cpu.emu_start(self.symbols["app_main"] | 1, self.return_a, count=2000000)
        self.cpu.hook_del(hook)
        diagnostics = [(t, m) for t, m in messages if "Task" in m]
        expected = []
        for tick in [0, 100, 200, 300]:
            for label in ["A", "B"]:
                expected.append(f"Task {label} running\r\n")
        self.assertEqual([m for _, m in diagnostics], expected)
        self.assertEqual([t for t, m in diagnostics if "Task A" in m], [0, 100, 200, 300])
        self.assertEqual([t for t, m in diagnostics if "Task B" in m], [0, 100, 200, 300])
        self.assertEqual(self.read_word(self.symbols["uwTick"]), 3000)


if __name__ == "__main__":
    unittest.main()
