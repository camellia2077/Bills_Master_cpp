import os
import subprocess
import sys
import shutil
import time
import re
# 用于测试cpp 的命令行程序
# ===================================================================
# 全局配置
# ===================================================================

# *** 重要: 请将此路径修改为您项目中 bill_master_cli.exe 所在的实际构建目录 ***
# 这是一个占位符路径，您必须根据自己的环境进行更改。
#
# Windows 示例: "C:/Projects/MySolution/build/Debug"
# Linux/macOS 示例: "/home/user/projects/my_project/build"
#
BUILD_DIR = "C:/Computer/my_github/github_cpp/Bills_Master_cpp/Bills_Master/build"  # <--- 修改这里！

# 定义需要清理的文件和文件夹
FILES_TO_DELETE = [
    "bills.db"
]
DIRS_TO_DELETE = [
    "txt_raw", # 经过预处理的文件
    "exported_files", # 导出的格式化文件，md,tex,typ
    "output"  # 新增：清理旧的输出日志文件夹
]
# ===================================================================

# 定义常量和颜色（用于美化输出）
GREEN = '\033[92m'
YELLOW = '\033[93m'
RED = '\033[91m'
CYAN = '\033[96m'
RESET = '\033[0m'

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
        """
        执行一个命令，并将其 stdout 和 stderr 保存到日志文件。

        :param step_name: 测试步骤的名称（用于打印状态）。
        :param command_args: 传递给可执行文件的参数列表。
        :param log_filename: 保存输出的日志文件名。
        :return: 如果命令成功执行（返回码为0），则为 True。
        """
        full_command = [self.exe_path] + command_args
        log_path = os.path.join(self.output_dir, log_filename)
        
        print(f"  -> {step_name:<15} | Log: {log_path}", end='', flush=True)

        try:
            start_time = time.time()
            result = subprocess.run(
                full_command,
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='ignore'
            )
            end_time = time.time()
            duration = end_time - start_time

            # *** 新增: 在写入文件前，清除输出中的颜色代码 ***
            clean_stdout = remove_ansi_codes(result.stdout)
            clean_stderr = remove_ansi_codes(result.stderr)

            with open(log_path, 'w', encoding='utf-8') as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- Return Code ---\n{result.returncode}\n\n")
                f.write(f"--- STDOUT ---\n{clean_stdout}\n")
                if result.stderr:
                    f.write(f"\n--- STDERR ---\n{clean_stderr}\n")

            if result.returncode == 0:
                print(f"  ... {GREEN}OK{RESET} ({duration:.2f}s)")
                return True
            else:
                print(f"  ... {RED}FAILED{RESET} (Code: {result.returncode})")
                return False

        except Exception as e:
            print(f"  ... {RED}CRASHED{RESET}\n      Error: {e}")
            with open(log_path, 'w', encoding='utf-8') as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- CRITICAL ERROR ---\nFailed to execute command.\nError: {e}\n")
            return False

class TestPreparer:
    """预处理类：负责所有测试开始前的准备工作。"""
    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.exe_name = "bill_master_cli.exe"

    def prepare_executable(self):
        print(f"{CYAN}--- 1. Preparing Executable ---{RESET}")
        if "C:/path/to/your/project/build/Debug" in BUILD_DIR:
            print(f"  {RED}错误: 请先在脚本中修改 'BUILD_DIR' 变量。{RESET}")
            return False
            
        source_exe_path = os.path.join(BUILD_DIR, self.exe_name)
        dest_exe_path = os.path.join(self.base_dir, self.exe_name)

        if os.path.exists(dest_exe_path):
            os.remove(dest_exe_path)
        
        if not os.path.exists(source_exe_path):
            print(f"  {RED}错误: 源文件未找到 '{source_exe_path}'.{RESET}")
            return False

        shutil.copy(source_exe_path, dest_exe_path)
        print(f"  {GREEN}可执行文件已准备就绪。{RESET}")
        return True

    def cleanup_and_setup_dirs(self):
        print(f"{CYAN}--- 2. Cleaning Artifacts & Setting up Directories ---{RESET}")
        for dir_name in DIRS_TO_DELETE:
            dir_path = os.path.join(self.base_dir, dir_name)
            if os.path.exists(dir_path):
                shutil.rmtree(dir_path)
                print(f"  {GREEN}已删除旧目录: {dir_name}{RESET}")

        for file_name in FILES_TO_DELETE:
            file_path = os.path.join(self.base_dir, file_name)
            if os.path.exists(file_path):
                os.remove(file_path)
                print(f"  {GREEN}已删除旧文件: {file_name}{RESET}")
        
        os.makedirs(os.path.join(self.base_dir, "output"))
        print(f"  {GREEN}已创建 'output' 日志目录。{RESET}")
        return True

class ImportTasks:
    """Import类：负责执行数据校验、修改和导入任务。"""
    def __init__(self, executor, bills_path):
        self.executor = executor
        self.bills_path = bills_path

    def run(self):
        print(f"{CYAN}--- 3. Running Import Tasks ---{RESET}")
        if not self.executor.run("Validate", ["--validate", self.bills_path], "1_validate.log"): return False
        if not self.executor.run("Modify", ["--modify", self.bills_path], "2_modify.log"): return False
        if not self.executor.run("Import", ["--import", self.bills_path], "3_import.log"): return False
        return True

class QueryTasks:
    """查询类：负责执行数据查询任务。"""
    def __init__(self, executor):
        self.executor = executor

    def run(self):
        print(f"{CYAN}--- 4. Running Query Tasks ---{RESET}")
        if not self.executor.run("Query Year", ["--query", "year", "2024"], "4_query_year.log"): return False
        if not self.executor.run("Query Month", ["--query", "month", "202401"], "5_query_month.log"): return False
        return True

class ExportTasks:
    """导出类：负责执行数据导出任务。"""
    def __init__(self, executor, export_path):
        self.executor = executor
        self.export_path = export_path

    def run(self):
        print(f"{CYAN}--- 5. Running Export Tasks ---{RESET}")
        if not self.executor.run("Export All", ["--export", "all", self.export_path], "6_export_all.log"): return False
        return True

def main():
    """主函数，协调整个测试流程。"""
    if os.name == 'nt':
        os.system('color')
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # --- 1. 预处理 ---
    preparer = TestPreparer(script_dir)
    if not preparer.prepare_executable(): sys.exit(1)
    if not preparer.cleanup_and_setup_dirs(): sys.exit(1)

    # --- 2. 设置路径和执行器 ---
    exe_path = os.path.join(script_dir, "bill_master_cli.exe")
    bills_path = os.path.join(script_dir, "bills")
    export_path = os.path.join(script_dir, "exported_files")
    output_dir = os.path.join(script_dir, "output")
    
    if not os.path.exists(bills_path):
        os.makedirs(bills_path)
        print(f"{YELLOW}警告: 'bills' 文件夹不存在，已为您创建。请添加账单文件以进行测试。{RESET}")

    executor = CommandExecutor(exe_path, output_dir)

    # --- 3. 实例化并运行各个任务模块 ---
    importer = ImportTasks(executor, bills_path)
    querier = QueryTasks(executor)
    exporter = ExportTasks(executor, export_path)

    print(f"\n{CYAN}========== Starting Test Sequence =========={RESET}")
    if importer.run() and querier.run() and exporter.run():
        print(f"\n{GREEN}✅ All test steps completed successfully!{RESET}")
        print(f"{GREEN}   Check the 'output' directory for detailed logs.{RESET}")
    else:
        print(f"\n{RED}❌ A test step failed. Please check the corresponding log file in the 'output' directory for details.{RESET}")

if __name__ == "__main__":
    main()
