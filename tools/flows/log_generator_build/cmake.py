import shutil
import sys
from pathlib import Path

from .utils import run_command


def ensure_cmake_configured(
    build_dir: Path,
    build_type: str,
    generator: str,
    source_dir: str,
    compiler: str,
) -> None:
    src_path = Path(source_dir).resolve()
    if not src_path.is_dir():
        print(
            "!!! Error: The CMake source directory specified in config "
            f"does not exist: {source_dir}"
        )
        sys.exit(1)

    cache_file = build_dir / "CMakeCache.txt"
    reconfigure = not cache_file.is_file()

    if cache_file.is_file():
        cache_text = cache_file.read_text(encoding="utf-8", errors="ignore")
        home_prefix = "CMAKE_HOME_DIRECTORY:INTERNAL="
        home_line = ""
        for line in cache_text.splitlines():
            if line.startswith(home_prefix):
                home_line = line[len(home_prefix):].strip()
                break
        if home_line:
            cached_source = Path(home_line).resolve()
            if cached_source != src_path:
                print(
                    "==> Existing CMake cache points to another source directory. "
                    "Cleaning build directory for reconfigure..."
                )
                shutil.rmtree(build_dir)
                reconfigure = True

    if reconfigure:
        print(
            f"==> Configuring build directory '{build_dir.name}'..."
        )
        build_dir.mkdir(parents=True, exist_ok=True)
        cmake_command = [
            "cmake",
            "-G",
            generator,
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
            str(src_path),
        ]
        if compiler:
            cmake_command.insert(-1, f"-DBILL_COMPILER={compiler}")
        run_command(cmake_command, cwd=build_dir)
    else:
        print(f"==> Entered existing build directory: {build_dir.resolve()}")


def build_target(build_dir: Path, target: str | None = None) -> None:
    command = ["cmake", "--build", "."]
    if target:
        command.extend(["--target", target])
    run_command(command, cwd=build_dir)
