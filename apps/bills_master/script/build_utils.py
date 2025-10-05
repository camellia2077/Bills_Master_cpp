#!/usr/bin/env python3

import os
import subprocess
import sys
import time
from pathlib import Path
from typing import List

# 从配置文件导入构建生成器和源码目录路径
from config import BUILD_GENERATOR, CMAKELIST_SOURCE_DIR

# 检查源码目录是否存在，如果不存在则报错退出
source_dir_path = Path(CMAKELIST_SOURCE_DIR)
if not source_dir_path.is_dir():
    print(f"!!! Error: The CMake source directory specified in config.py does not exist.")
    print(f"!!! Please check the following path: {CMAKELIST_SOURCE_DIR}")
    sys.exit(1)

def run_command(command: List[str], cwd: Path):
    """在指定目录下运行一个子进程命令，并进行错误处理。"""
    print(f"==> Running command: {' '.join(command)}")
    try:
        subprocess.run(command, check=True, cwd=cwd)
    except FileNotFoundError:
        print(f"!!! Error: Command '{command[0]}' not found. Is it installed and in your PATH?")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"\n!!! A build step failed with exit code {e.returncode}.")
        sys.exit(e.returncode)

def setup_environment():
    """切换到项目根目录并返回其路径。"""
    project_dir = Path(__file__).resolve().parent.parent
    os.chdir(project_dir)
    print(f"==> Switched to project directory: {os.getcwd()}")
    return project_dir

def handle_clean_command(build_dir: Path, project_dir: Path):
    """如果用户提供了 'clean' 参数，则清理构建目录。"""
    should_clean = len(sys.argv) > 1 and sys.argv[1].lower() == 'clean'
    if should_clean and build_dir.is_dir():
        print(f"==> 'clean' option provided. Cleaning existing build directory...")

        if BUILD_GENERATOR == "Ninja":
            clean_command = ["ninja", "clean"]
        elif BUILD_GENERATOR == "Unix Makefiles":
            clean_command = ["make", "clean"]
        else:
            print(f"!!! Error: Unknown BUILD_GENERATOR '{BUILD_GENERATOR}' specified in config.py for cleaning.")
            sys.exit(1)

        run_command(clean_command, cwd=build_dir)
        print(f"==> Build directory '{build_dir.name}' cleaned.")
    return should_clean

def configure_and_build(build_dir: Path, build_type: str):
    """配置CMake项目（如果需要）并执行构建。"""
    is_first_build = not build_dir.is_dir()

    if is_first_build:
        print(f"==> Build directory '{build_dir.name}' not found. Creating and configuring...")
        build_dir.mkdir()

        cmake_command = [
            "cmake", "-G", BUILD_GENERATOR, f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache", CMAKELIST_SOURCE_DIR
        ]
        run_command(cmake_command, cwd=build_dir)
    else:
        print(f"==> Entered existing build directory: {build_dir.resolve()}")

    print(f"==> Compiling the project ({build_type} Mode, incremental build)...")
    run_command(["cmake", "--build", "."], cwd=build_dir)


def print_summary(start_time: float, build_dir_name: str, build_type: str):
    """打印构建完成后的总结信息。"""
    end_time = time.monotonic()
    duration = int(end_time - start_time)
    minutes, seconds = divmod(duration, 60)

    print("\n================================================================")
    print(f"{build_type} build complete!")
    print(f"Executables are located in the '{build_dir_name}/bin' directory.")
    print(f"Total build time: {minutes}m {seconds}s")
    print("================================================================")

def run_build_pipeline(build_type: str, build_dir_name: str):
    """
    执行完整的构建流程：环境设置、清理、配置和编译。
    """
    start_time = time.monotonic()

    try:
        project_dir = setup_environment()
        build_dir_path = project_dir / build_dir_name

        handle_clean_command(build_dir_path, project_dir)

        configure_and_build(build_dir_path, build_type)

        print_summary(start_time, build_dir_name, build_type)

    except Exception as e:
        print(f"\n!!! An unexpected error occurred: {e}")
        sys.exit(1)