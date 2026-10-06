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
env.BuildSources(
    env.subst("$BUILD_DIR") + "/FreeRTOS",
    str(kernel),
    src_filter=[
        "+<tasks.c>", "+<list.c>", "+<queue.c>",
        "+<portable/GCC/ARM_CM3/port.c>",
        "+<portable/MemMang/heap_4.c>",
    ],
)
