"""Compile only the STM32Cube FreeRTOS kernel and GCC Cortex-M3 port."""
from pathlib import Path

Import("env")

framework = env.PioPlatform().get_package_dir("framework-stm32cubef1")
kernel = Path(framework) / "Middlewares/Third_Party/FreeRTOS/Source"
if not (kernel / "include/FreeRTOS.h").is_file():
    raise RuntimeError("The STM32CubeF1 package is missing its FreeRTOS kernel")

env.Append(CPPPATH=[
    str(Path(env.subst("$PROJECT_DIR")) / "include"),
    str(kernel / "include"),
    str(kernel / "portable/GCC/ARM_CM3"),
])

port_filter = "+<portable/GCC/ARM_CM3/port.c>"
if env.GetProjectOption("custom_wokwi_nvic_workaround", "no") == "yes":
    # Keep the installed framework untouched. Generate a local port copy
    # that ignores the NVIC low bits absent on the STM32F103 (4 MSBs only).
    # The original priority-width and priority-group assertions remain active.
    port_source = (kernel / "portable/GCC/ARM_CM3/port.c").read_text(encoding="utf-8")
    anchor = "ucMaxPriorityValue = *pucFirstUserPriorityRegister;"
    if port_source.count(anchor) != 1:
        raise RuntimeError("FreeRTOS port changed: review the Wokwi NVIC adaptation")
    port_source = port_source.replace(anchor, anchor + "\n" +
        "\t\t/* Wokwi: exclude priority bits absent on this STM32 device. */\n" +
        "\t\tucMaxPriorityValue &= ( uint8_t ) ( 0xffU << ( 8U - configPRIO_BITS ) );")
    generated = Path(env.subst("$BUILD_DIR")) / "generated/freertos_port"
    generated.mkdir(parents=True, exist_ok=True)
    target = generated / "port.c"
    if not target.is_file() or target.read_text(encoding="utf-8") != port_source:
        target.write_text(port_source, encoding="utf-8")
    env.BuildSources(env.subst("$BUILD_DIR") + "/FreeRTOSPort", str(generated))
    port_filter = "-<portable/GCC/ARM_CM3/port.c>"

env.BuildSources(
    env.subst("$BUILD_DIR") + "/FreeRTOS",
    str(kernel),
    src_filter=[
        "+<tasks.c>", "+<list.c>", "+<queue.c>",
        port_filter,
        "+<portable/MemMang/heap_4.c>",
    ],
)
