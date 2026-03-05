# preparer.py

import os
import shutil
# [MODIFICATION] Use relative imports for modules within the same package
from . import app_config as config
from . import constants

class TestPreparer:
    """预处理类：负责所有测试开始前的准备工作。"""
    _RUNTIME_SIDECAR_EXTS = {".dll", ".exe", ".pdb", ".manifest"}

    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.exe_name = "bill_master_cli.exe"
        self.local_plugins_dir = os.path.join(self.base_dir, "plugins")

    def _copy_exe_sidecars(self):
        """复制 exe 运行时附属文件到测试根目录（不含主 exe）。"""
        copied_files = []
        main_exe_lower = self.exe_name.lower()
        for entry in os.scandir(config.BUILD_DIR):
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
                f"  {constants.GREEN}已复制 exe 相关文件到 tests: "
                f"{len(copied_files)} 个 ({copied_files_str}){constants.RESET}"
            )
        else:
            print(
                f"  {constants.YELLOW}未发现额外 exe 相关文件，跳过附属文件复制。{constants.RESET}"
            )

    def prepare_runtime_env(self):
        """准备完整的运行时环境，包括可执行文件、配置文件和所有插件。"""
        # ... (rest of the file is unchanged) ...
        print(f"{constants.CYAN}--- 1. Preparing Runtime Environment ---{constants.RESET}")
        
        if not os.path.exists(config.BUILD_DIR):
            print(f"  {constants.RED}错误: 构建目录未找到 '{config.BUILD_DIR}'.{constants.RESET}")
            return False

        # --- 步骤1: 复制可执行文件 ---
        source_exe_path = os.path.join(config.BUILD_DIR, self.exe_name)
        dest_exe_path = os.path.join(self.base_dir, self.exe_name)
        if not os.path.exists(source_exe_path):
            print(f"  {constants.RED}错误: 源可执行文件未找到 '{source_exe_path}'.{constants.RESET}")
            return False
        shutil.copy(source_exe_path, dest_exe_path)
        print(f"  {constants.GREEN}已复制可执行文件: {self.exe_name}{constants.RESET}")

        # --- 步骤1.1: 复制 exe 相关附属文件到 tests 根目录 ---
        self._copy_exe_sidecars()

        # --- 步骤2: 复制 config 文件夹 ---
        source_config_dir = os.path.join(config.BUILD_DIR, "config")
        dest_config_dir = os.path.join(self.base_dir, "config")
        if not os.path.exists(source_config_dir):
            print(f"  {constants.RED}错误: 'config' 文件夹在构建目录中未找到 '{source_config_dir}'.{constants.RESET}")
            return False
        
        shutil.copytree(source_config_dir, dest_config_dir)
        print(f"  {constants.GREEN}已复制配置文件夹: config{constants.RESET}")

        # --- 步骤3: 复制插件 DLLs ---
        os.makedirs(self.local_plugins_dir, exist_ok=True)
        all_plugins_found = True
        for dll_name in config.PLUGIN_DLLS:
            source_dll_path = os.path.join(config.BUILD_DIR, dll_name)
            dest_dll_path = os.path.join(self.local_plugins_dir, dll_name)
            
            if not os.path.exists(source_dll_path):
                print(f"  {constants.RED}错误: 插件未找到 '{source_dll_path}'.{constants.RESET}")
                all_plugins_found = False
                continue
            
            shutil.copy(source_dll_path, dest_dll_path)
        
        if not all_plugins_found: return False
            
        print(f"  {constants.GREEN}所有插件已复制, 运行时环境已准备就绪。{constants.RESET}")
        return True

    def cleanup_and_setup_dirs(self):
        """清理旧的产物并设置日志目录。"""
        print(f"{constants.CYAN}--- 0. Cleaning Artifacts ---{constants.RESET}")
        for dir_name in config.DIRS_TO_DELETE:
            dir_path = os.path.join(self.base_dir, dir_name)
            if os.path.exists(dir_path): shutil.rmtree(dir_path)
        for file_name in config.FILES_TO_DELETE:
            file_path = os.path.join(self.base_dir, file_name)
            if os.path.exists(file_path): os.remove(file_path)
        
        print(f"  {constants.GREEN}清理完成。{constants.RESET}")
        return True
