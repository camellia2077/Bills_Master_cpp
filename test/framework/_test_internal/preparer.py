# preparer.py

import os
import shutil
# [MODIFICATION] Use relative imports for modules within the same package
from . import app_config as config
from . import constants

class TestPreparer:
    """预处理类：负责所有测试开始前的准备工作。"""
    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.exe_name = "bill_master_cli.exe"
        self.local_plugins_dir = os.path.join(self.base_dir, "plugins")

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