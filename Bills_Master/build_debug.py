#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

def main():
    """
    Navigates to the project root, sets up a debug build directory,
    and runs CMake to build the project.
    """
    # --- Script Configuration ---
    # Record the start time (monotonic clock is best for measuring duration)
    start_time = time.monotonic()
    
    # --- Main Logic ---
    try:
        # Get the directory of the current script and switch to it.
        # This is the Python equivalent of the SCRIPT_DIR logic in the bash script.
        project_dir = Path(__file__).resolve().parent
        os.chdir(project_dir)
        print(f"==> Switched to project directory: {os.getcwd()}")

        build_dir = "build_debug"

        # If the build directory exists, remove it.
        if os.path.isdir(build_dir):
            print("==> Found existing debug build directory. Removing it...")
            shutil.rmtree(build_dir)
            print("==> Old debug build directory removed.")

        # Create the new build directory and enter it.
        print("==> Creating new debug build directory...")
        os.makedirs(build_dir)
        os.chdir(build_dir)
        print(f"==> Entered debug build directory: {os.getcwd()}")

        # Run CMake to configure the project. The `check=True` argument will
        # cause the script to exit if the command fails (like `set -e`).
        print("==> Running CMake to configure the project for DEBUG...")
        subprocess.run(
            ["cmake", "-DCMAKE_BUILD_TYPE=Debug", ".."],
            check=True
        )

        # Compile the project.
        print("==> Compiling the project (Debug Mode)...")
        subprocess.run(
            ["cmake", "--build", "."],
            check=True
        )

    except subprocess.CalledProcessError as e:
        print(f"\n!!! A build step failed with exit code {e.returncode}.")
        sys.exit(e.returncode)
    except Exception as e:
        print(f"\n!!! An unexpected error occurred: {e}")
        sys.exit(1)

    # --- Completion Message ---
    print("")
    print("================================================================")
    print("Debug build complete!")
    print(f"Executables are located in the '{build_dir}/bin' directory.")
    print(f"You can run it from the project root with: ./{build_dir}/bin/bill_master_cli")
    print("================================================================")

    # --- Calculate and print the total duration ---
    end_time = time.monotonic()
    duration = int(end_time - start_time)
    minutes, seconds = divmod(duration, 60)

    print(f"Total build time: {minutes}m {seconds}s")
    print("================================================================")


if __name__ == "__main__":
    main()