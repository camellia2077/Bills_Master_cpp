#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

def main():
    """
    Navigates to the project root, sets up a release build directory,
    and runs CMake to build the project. Supports a 'clean' argument.
    """
    # 检查命令行是否传入了 'clean' 参数
    should_clean_first = len(sys.argv) > 1 and sys.argv[1].lower() == 'clean'

    start_time = time.monotonic()

    try:
        project_dir = Path(__file__).resolve().parent
        os.chdir(project_dir)
        print(f"==> Switched to project directory: {os.getcwd()}")

        build_dir = "build"
        build_dir_path = Path(build_dir)

        # ✅ --- 新增的清理逻辑 ---
        if should_clean_first and build_dir_path.is_dir():
            print(f"==> 'clean' option provided. Cleaning existing build directory...")
            os.chdir(build_dir)
            # 使用 'ninja clean' 或 'cmake --build . --target clean'
            subprocess.run(["ninja", "clean"], check=True)
            print("==> Build directory cleaned.")
            os.chdir(project_dir) # 清理后返回项目根目录

        # --- 后续逻辑与之前保持一致 ---
        is_first_build = not build_dir_path.is_dir()

        if is_first_build:
            print(f"==> Build directory '{build_dir}' not found. Creating and configuring...")
            os.makedirs(build_dir)
            os.chdir(build_dir)
            
            cmake_command = [
                "cmake", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
                "-DCMAKE_C_COMPILER_LAUNCHER=ccache", ".."
            ]
            subprocess.run(cmake_command, check=True)
        else:
            os.chdir(build_dir)
            print(f"==> Entered existing build directory: {os.getcwd()}")

        print("==> Compiling the project (incremental build)...")
        subprocess.run(["cmake", "--build", "."], check=True)

    except subprocess.CalledProcessError as e:
        print(f"\n!!! A build step failed with exit code {e.returncode}.")
        sys.exit(e.returncode)
    except Exception as e:
        print(f"\n!!! An unexpected error occurred: {e}")
        sys.exit(1)

    # --- 结束信息 ---
    print("\n================================================================")
    print("Build complete!")
    print(f"Executables are located in the '{build_dir}/bin' directory.")
    print("================================================================")

    end_time = time.monotonic()
    duration = int(end_time - start_time)
    minutes, seconds = divmod(duration, 60)
    print(f"Total build time: {minutes}m {seconds}s")
    print("================================================================")


if __name__ == "__main__":
    main()