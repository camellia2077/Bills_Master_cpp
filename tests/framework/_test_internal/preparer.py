# preparer.py

import os
import shutil

from . import app_config as config
from . import constants


class TestPreparer:
    """Prepare the runtime workspace before CLI tests run."""

    _RUNTIME_SIDECAR_EXTS = {".dll", ".exe", ".pdb", ".manifest"}

    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.exe_name = "bill_master_cli.exe"

    def _copy_exe_sidecars(self):
        copied_files = []
        main_exe_lower = self.exe_name.lower()
        for entry in os.scandir(config.WORKSPACE_DIR):
            if not entry.is_file():
                continue
            name_lower = entry.name.lower()
            if name_lower == main_exe_lower:
                continue
            ext = os.path.splitext(name_lower)[1]
            if ext not in self._RUNTIME_SIDECAR_EXTS:
                continue

            dest_path = os.path.join(self.base_dir, entry.name)
            shutil.copy(entry.path, dest_path)
            copied_files.append(entry.name)

        if copied_files:
            copied_files_str = ", ".join(sorted(copied_files))
            print(
                f"  {constants.GREEN}Copied runtime sidecars to tests: "
                f"{len(copied_files)} ({copied_files_str}){constants.RESET}"
            )
        else:
            print(
                f"  {constants.YELLOW}No extra runtime sidecars found; skip copy.{constants.RESET}"
            )

    def prepare_runtime_env(self):
        print(f"{constants.CYAN}--- 1. Preparing Runtime Environment ---{constants.RESET}")

        if not os.path.exists(config.WORKSPACE_DIR):
            print(
                f"  {constants.RED}Error: workspace directory not found: "
                f"'{config.WORKSPACE_DIR}'.{constants.RESET}"
            )
            return False

        source_exe_path = os.path.join(config.WORKSPACE_DIR, self.exe_name)
        dest_exe_path = os.path.join(self.base_dir, self.exe_name)
        if not os.path.exists(source_exe_path):
            print(
                f"  {constants.RED}Error: executable not found: "
                f"'{source_exe_path}'.{constants.RESET}"
            )
            return False

        shutil.copy(source_exe_path, dest_exe_path)
        print(f"  {constants.GREEN}Copied executable: {self.exe_name}{constants.RESET}")

        self._copy_exe_sidecars()

        source_config_dir = os.path.join(config.WORKSPACE_DIR, "config")
        dest_config_dir = os.path.join(self.base_dir, "config")
        if not os.path.exists(source_config_dir):
            print(
                f"  {constants.RED}Error: config directory not found in workspace "
                f"output: '{source_config_dir}'.{constants.RESET}"
            )
            return False

        shutil.copytree(source_config_dir, dest_config_dir)
        print(f"  {constants.GREEN}Copied config directory: config{constants.RESET}")

        print(
            f"  {constants.GREEN}Built-in report formatters need no extra DLL "
            f"copy step.{constants.RESET}"
        )
        return True

    def cleanup_and_setup_dirs(self):
        print(f"{constants.CYAN}--- 0. Cleaning Artifacts ---{constants.RESET}")
        for dir_name in config.DIRS_TO_DELETE:
            dir_path = os.path.join(self.base_dir, dir_name)
            if os.path.exists(dir_path):
                shutil.rmtree(dir_path)
        legacy_plugins_dir = os.path.join(self.base_dir, "plugins")
        if os.path.exists(legacy_plugins_dir):
            shutil.rmtree(legacy_plugins_dir)
        for file_name in config.FILES_TO_DELETE:
            file_path = os.path.join(self.base_dir, file_name)
            if os.path.exists(file_path):
                os.remove(file_path)

        print(f"  {constants.GREEN}Cleanup completed.{constants.RESET}")
        return True
