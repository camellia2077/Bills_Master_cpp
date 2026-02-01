from pathlib import Path
import sys

# Relative import doesn't work well when running as a script, so we use absolute imports
# or assume the package is in python path. For this simple structure, we can import from .utils
from .utils import run_command
# We will need to import config, but config.py is in the parent directory.
# To keep it clean, we'll let the caller pass config values or setup sys.path if needed.
# For now, we will assume config is available or passed in.

def ensure_cmake_configured(build_dir: Path, build_type: str, generator: str,
                            source_dir: str, compiler: str):
    """Ensure the build directory exists and is configured with CMake."""
    is_first_build = not build_dir.is_dir()

    if is_first_build:
        print(f"==> Build directory '{build_dir.name}' not found. Creating and configuring...")
        build_dir.mkdir()

        # check source dir
        src_path = Path(source_dir)
        if not src_path.is_dir():
             print(f"!!! Error: The CMake source directory specified in config.py does not exist: {source_dir}")
             sys.exit(1)

        cmake_command = [
            "cmake", "-G", generator, f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache", source_dir
        ]
        if compiler:
            cmake_command.insert(-1, f"-DBILL_COMPILER={compiler}")
        run_command(cmake_command, cwd=build_dir)
    else:
        print(f"==> Entered existing build directory: {build_dir.resolve()}")

def build_target(build_dir: Path, target: str = None):
    """Run cmake --build."""
    cmd = ["cmake", "--build", "."]
    if target:
        cmd.extend(["--target", target])
    run_command(cmd, cwd=build_dir)
