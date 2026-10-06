"""Compile the STM32Cube FreeRTOS kernel with the selected target port."""
from pathlib import Path

Import("env")

framework = env.PioPlatform().get_package_dir("framework-stm32cubef1")
kernel = Path(framework) / "Middlewares/Third_Party/FreeRTOS/Source"
if not (kernel / "include/FreeRTOS.h").is_file():
    raise RuntimeError("The STM32CubeF1 package is missing its FreeRTOS kernel")

simulation_port = env.GetProjectOption("custom_freertos_port", "native") == "wokwi"
port_dir = Path(env.subst("$PROJECT_DIR")) / "ports/wokwi" if simulation_port else kernel / "portable/GCC/ARM_CM3"
env.Append(CPPPATH=[
    str(Path(env.subst("$PROJECT_DIR")) / "include"),
    str(kernel / "include"),
    str(port_dir),
])

port_filter = "+<portable/GCC/ARM_CM3/port.c>"
if simulation_port:
    env.Append(CPPDEFINES=["WOKWI_FREERTOS_PORT"])
    env.BuildSources(env.subst("$BUILD_DIR") + "/FreeRTOSPort", str(port_dir), src_filter=["+<port.c>"])
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
