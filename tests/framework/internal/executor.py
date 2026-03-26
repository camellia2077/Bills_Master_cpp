# executor.py

import os
import re
import subprocess
import time

# [MODIFICATION] Use relative imports
from . import constants


# ... (rest of the file is unchanged) ...
def remove_ansi_codes(text):
    """一个辅助函数，用于从字符串中移除ANSI颜色转义码。"""
    ansi_escape = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")
    return ansi_escape.sub("", text)


class CommandExecutor:
    """一个工具类，用于执行外部命令并记录其输出。"""

    def __init__(self, exe_path, output_dir):
        self.exe_path = exe_path
        self.output_dir = output_dir
        self.records = []

    def _append_record(
        self, step_name, command_args, log_filename, return_code, duration_seconds, status
    ):
        self.records.append(
            {
                "step": step_name,
                "status": status,
                "return_code": return_code,
                "duration_seconds": round(duration_seconds, 3),
                "log_file": log_filename,
                "command_args": list(command_args),
            }
        )

    def _write_log(self, log_path, full_command, return_code, stdout_text, stderr_text):
        with open(log_path, "w", encoding="utf-8") as f:
            f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
            f.write(f"--- Return Code ---\n{return_code}\n\n")
            f.write(f"--- STDOUT ---\n{stdout_text}\n")
            if stderr_text:
                f.write(f"\n--- STDERR ---\n{stderr_text}\n")

    def read_log_text(self, log_filename):
        log_path = os.path.join(self.output_dir, log_filename)
        with open(log_path, "r", encoding="utf-8") as f:
            return f.read()

    def run(self, step_name, command_args, log_filename):
        """执行一个命令，并将其 stdout 和 stderr 保存到日志文件。"""
        full_command = [self.exe_path] + command_args
        log_path = os.path.join(self.output_dir, log_filename)

        print(f"  -> {step_name:<25} | Log: {log_filename}", end="", flush=True)

        try:
            start_time = time.time()
            result = subprocess.run(
                full_command, capture_output=True, text=True, encoding="utf-8", errors="ignore"
            )
            end_time = time.time()
            duration = end_time - start_time

            clean_stdout = remove_ansi_codes(result.stdout)
            clean_stderr = remove_ansi_codes(result.stderr)

            self._write_log(
                log_path, full_command, result.returncode, clean_stdout, clean_stderr
            )

            if result.returncode == 0:
                print(f" ... {constants.GREEN}OK{constants.RESET} ({duration:.2f}s)")
                self._append_record(
                    step_name=step_name,
                    command_args=command_args,
                    log_filename=log_filename,
                    return_code=result.returncode,
                    duration_seconds=duration,
                    status="ok",
                )
                return True
            else:
                print(f" ... {constants.RED}FAILED{constants.RESET} (Code: {result.returncode})")
                self._append_record(
                    step_name=step_name,
                    command_args=command_args,
                    log_filename=log_filename,
                    return_code=result.returncode,
                    duration_seconds=duration,
                    status="failed",
                )
                return False

        except Exception as e:
            print(f" ... {constants.RED}CRASHED{constants.RESET}\n      Error: {e}")
            with open(log_path, "w", encoding="utf-8") as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- CRITICAL ERROR ---\nFailed to execute command.\nError: {e}\n")
            self._append_record(
                step_name=step_name,
                command_args=command_args,
                log_filename=log_filename,
                return_code=-1,
                duration_seconds=0.0,
                status="crashed",
            )
            return False

    def run_expected_failure(self, step_name, command_args, log_filename):
        """执行一个预期失败的命令，并将其输出保存到日志文件。"""
        full_command = [self.exe_path] + command_args
        log_path = os.path.join(self.output_dir, log_filename)

        print(f"  -> {step_name:<25} | Log: {log_filename}", end="", flush=True)

        try:
            start_time = time.time()
            result = subprocess.run(
                full_command, capture_output=True, text=True, encoding="utf-8", errors="ignore"
            )
            end_time = time.time()
            duration = end_time - start_time

            clean_stdout = remove_ansi_codes(result.stdout)
            clean_stderr = remove_ansi_codes(result.stderr)

            self._write_log(
                log_path, full_command, result.returncode, clean_stdout, clean_stderr
            )

            if result.returncode != 0:
                print(f" ... {constants.GREEN}OK{constants.RESET} (expected failure)")
                self._append_record(
                    step_name=step_name,
                    command_args=command_args,
                    log_filename=log_filename,
                    return_code=result.returncode,
                    duration_seconds=duration,
                    status="expected_fail",
                )
                return True

            print(
                f" ... {constants.RED}FAILED{constants.RESET} "
                "(command unexpectedly succeeded)"
            )
            self._append_record(
                step_name=step_name,
                command_args=command_args,
                log_filename=log_filename,
                return_code=result.returncode,
                duration_seconds=duration,
                status="failed",
            )
            return False

        except Exception as e:
            print(f" ... {constants.RED}CRASHED{constants.RESET}\n      Error: {e}")
            with open(log_path, "w", encoding="utf-8") as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- CRITICAL ERROR ---\nFailed to execute command.\nError: {e}\n")
            self._append_record(
                step_name=step_name,
                command_args=command_args,
                log_filename=log_filename,
                return_code=-1,
                duration_seconds=0.0,
                status="crashed",
            )
            return False
