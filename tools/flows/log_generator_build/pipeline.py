import time
from pathlib import Path

from .cmake import build_target, ensure_cmake_configured
from .utils import run_command, setup_environment


def handle_clean(build_dir: Path, generator: str, clean_flag: bool) -> None:
    if clean_flag and build_dir.is_dir():
        print("==> 'clean' option provided. Cleaning existing build directory...")

        if generator == "Ninja":
            clean_cmd = ["ninja", "clean"]
        elif generator == "Unix Makefiles":
            clean_cmd = ["make", "clean"]
        else:
            print(
                f"!!! Warning: Unknown generator '{generator}', "
                "skipping specific clean command."
            )
            return

        run_command(clean_cmd, cwd=build_dir)
        print(f"==> Build directory '{build_dir.name}' cleaned.")


def run_build(
    build_type: str,
    build_dir_name: str,
    config: dict,
    project_dir: Path,
    clean_flag: bool,
) -> None:
    start_time = time.monotonic()

    setup_environment(project_dir)
    build_dir = project_dir / build_dir_name
    source_dir = Path(config["build"]["cmake_source_dir"])
    if not source_dir.is_absolute():
        source_dir = (project_dir / source_dir).resolve()

    handle_clean(build_dir, config["build"]["generator"], clean_flag)

    ensure_cmake_configured(
        build_dir=build_dir,
        build_type=build_type,
        generator=config["build"]["generator"],
        source_dir=str(source_dir),
        compiler=config["build"].get("compiler", ""),
    )

    print(f"==> Compiling the project ({build_type} Mode)...")
    build_target(build_dir)

    duration = int(time.monotonic() - start_time)
    minutes, seconds = divmod(duration, 60)
    print("\n================================================================")
    print(f"{build_type} build complete!")
    print(f"Total build time: {minutes}m {seconds}s")
    print("================================================================")


def run_target_only(
    build_type: str,
    build_dir_name: str,
    target: str,
    config: dict,
    project_dir: Path,
) -> None:
    setup_environment(project_dir)
    build_dir = project_dir / build_dir_name
    source_dir = Path(config["build"]["cmake_source_dir"])
    if not source_dir.is_absolute():
        source_dir = (project_dir / source_dir).resolve()

    ensure_cmake_configured(
        build_dir=build_dir,
        build_type=build_type,
        generator=config["build"]["generator"],
        source_dir=str(source_dir),
        compiler=config["build"].get("compiler", ""),
    )

    print(f"==> Running target: {target}...")
    build_target(build_dir, target=target)
    print(f"\n==> Target '{target}' finished successfully.")
