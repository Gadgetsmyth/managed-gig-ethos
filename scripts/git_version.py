# PlatformIO pre-build script: define FW_VERSION from `git describe` so the firmware
# can report exactly which commit it was built from. Falls back to "unknown" when git
# or the repository is unavailable (e.g. building from a source tarball).
import subprocess

Import("env")

try:
    version = (
        subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=env["PROJECT_DIR"],
            stderr=subprocess.DEVNULL,
        )
        .decode()
        .strip()
    )
except Exception:
    version = "unknown"

env.Append(CPPDEFINES=[("FW_VERSION", env.StringifyMacro(version))])
