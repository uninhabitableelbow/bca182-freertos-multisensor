# Part II — Initial Wokwi simulation

Laboratory step 16 requires the Blue Pill alone and a recognizable serial
startup message before sensors or FreeRTOS tasks are introduced.

## Configuration

- `diagram.json`: one `board-stm32-bluepill` component.
- PA9 / USART1 TX -> `$serialMonitor:RX`.
- PA10 / USART1 RX -> `$serialMonitor:TX`.
- `wokwi.toml`: PlatformIO's Blue Pill firmware binary and ELF paths.
- `serial.cpp`: STM32 HAL GPIO setup, USART1 initialization, and bounded
  polling transmission at 115200 baud, 8N1.
- `app_main()`: initializes serial, sends the two startup lines once, then
  sleeps between interrupts.

## Verification procedure

1. Run `pio run -e bluepill_f103c8`.
2. In VS Code, press F1 and select **Wokwi: Start Simulator**.
3. Confirm there is only a Blue Pill in the circuit.
4. Check for both lines, with no garbled characters:

   ```text
   BCA182 FreeRTOS Multisensor
   System starting...
   ```

5. Stop and start the simulator; verify both lines appear again.

## Verification record

Simulator observation is pending. The Wokwi VS Code extension is installed;
no Wokwi CLI or CLI token is available in the execution environment.
Checks completed on 2026-10-06:

- `pio run -e bluepill_f103c8`: passed, 116 bytes RAM and 3,380 bytes flash.
- `diagram.json` and `wokwi.toml`: parsed successfully.
- Configured firmware binary and ELF: both exist after the build.
- `git diff --check`: passed.

Serial output and restart behavior still require the simulator observation
described above. These configuration checks do not establish runtime success.

## References

- [Wokwi Blue Pill reference](https://docs.wokwi.com/parts/board-stm32-bluepill)
- [Wokwi configuration and firmware paths](https://docs.wokwi.com/vscode/project-config)
- [Wokwi Serial Monitor connections](https://docs.wokwi.com/guides/serial-monitor)
