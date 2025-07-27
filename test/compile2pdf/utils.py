# utils.py (修改后)

import os
import subprocess

def check_compiler_availability(check_command: list) -> bool:
    """通用函数，检查一个编译器命令是否在系统中可用。"""
    compiler_name = check_command[0]
    try:
        subprocess.run(check_command, capture_output=True, check=True, text=True, timeout=10)
        print(f"✅ '{compiler_name}' 命令已找到。")
        return True
    except FileNotFoundError:
        print(f"❌ 错误：未找到 '{compiler_name}' 命令。请确保已安装并将其添加到系统 PATH。")
        return False
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print(f"⚠️ 警告：'{compiler_name}' 命令存在问题: {e}")
        return True

def find_source_files(directory: str, extension: str) -> list[str]:
    """
    扫描指定目录及其子目录，返回所有具有特定扩展名的文件的完整路径列表。
    该函数假定调用者已经验证了目录的存在。
    """
    found_files = []
    # --- 移除了这里的 if not os.path.isdir(directory) 检查 ---
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(extension):
                found_files.append(os.path.join(root, file))
    return found_files