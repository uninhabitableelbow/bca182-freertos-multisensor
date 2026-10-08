"""Select PlatformIO's managed MinGW toolchain for Windows host tests."""
import os
from pathlib import Path

Import("env")

if os.name == "nt":
    package = env.PioPlatform().get_package_dir("toolchain-gccmingw32")
    if not package:
        raise RuntimeError("Install the configured MinGW package before native tests")
    env.PrependENVPath("PATH", str(Path(package) / "bin"))
    # Test executables run outside SCons' compiler PATH. Embed MinGW runtimes.
    env.Append(LINKFLAGS=["-static", "-static-libgcc", "-static-libstdc++"])
