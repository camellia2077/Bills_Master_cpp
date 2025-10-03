# executor.py

import os
import subprocess
import time
import re
# [MODIFICATION] Use relative imports
from . import app_config as config
from . import constants

# ... (rest of the file is unchanged) ...
def remove_ansi_codes(text):
    """一个辅助函数，用于从字符串中移除ANSI颜色转义码。"""
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

class CommandExecutor:
    """一个工具类，用于执行外部命令并记录其输出。"""
    def __init__(self, exe_path, output_dir):
        self.exe_path = exe_path
        self.output_dir = output_dir

    def run(self, step_name, command_args, log_filename):
        """执行一个命令，并将其 stdout 和 stderr 保存到日志文件。"""
        full_command = [self.exe_path] + command_args
        log_path = os.path.join(self.output_dir, log_filename)
        
        print(f"  -> {step_name:<25} | Log: {log_filename}", end='', flush=True)

        try:
            start_time = time.time()
            result = subprocess.run(
                full_command, capture_output=True, text=True,
                encoding='utf-8', errors='ignore'
            )
            end_time = time.time()
            duration = end_time - start_time

            clean_stdout = remove_ansi_codes(result.stdout)
            clean_stderr = remove_ansi_codes(result.stderr)

            with open(log_path, 'w', encoding='utf-8') as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- Return Code ---\n{result.returncode}\n\n")
                f.write(f"--- STDOUT ---\n{clean_stdout}\n")
                if result.stderr:
                    f.write(f"\n--- STDERR ---\n{clean_stderr}\n")

            if result.returncode == 0:
                print(f" ... {constants.GREEN}OK{constants.RESET} ({duration:.2f}s)")
                return True
            else:
                print(f" ... {constants.RED}FAILED{constants.RESET} (Code: {result.returncode})")
                return False

        except Exception as e:
            print(f" ... {constants.RED}CRASHED{constants.RESET}\n      Error: {e}")
            with open(log_path, 'w', encoding='utf-8') as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- CRITICAL ERROR ---\nFailed to execute command.\nError: {e}\n")
            return False