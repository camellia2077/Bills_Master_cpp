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
    and runs CMake to build the project.
    """
    # --- Script Configuration ---
    # Record the start time
    start_time = time.monotonic()

    # --- Main Logic ---
    try:
        # Get the directory of the current script and switch to it.
        project_dir = Path(__file__).resolve().parent
        os.chdir(project_dir)
        print(f"==> Switched to project directory: {os.getcwd()}")

        # Define the build directory name
        build_dir = "build"

        # If the build directory exists, remove it
        if os.path.isdir(build_dir):
            print(f"==> Found existing build directory. Removing it...")
            shutil.rmtree(build_dir)
            print(f"==> Old build directory removed.")

        # Create the new build directory and enter it
        print(f"==> Creating new build directory...")
        os.makedirs(build_dir)
        os.chdir(build_dir)
        print(f"==> Entered build directory: {os.getcwd()}")

        # Run CMake to configure the project for Release.
        # The `check=True` argument mimics `set -e`.
        print("==> Running CMake to configure the project...")
        subprocess.run(
            ["cmake", "-DCMAKE_BUILD_TYPE=Release", ".."],
            check=True
        )

        # Compile the project
        print("==> Compiling the project...")
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
    print("================================================")
    print("Build complete!")
    print(f"The executable 'main' is located in the '{build_dir}' directory.") #
    print(f"You can run it from the project root with: ./{build_dir}/main") #

    # --- Calculate and print the total duration ---
    end_time = time.monotonic()
    duration = int(end_time - start_time)
    minutes, seconds = divmod(duration, 60)

    print(f"Total build time: {minutes}m {seconds}s")
    print("================================================")


if __name__ == "__main__":
    main()