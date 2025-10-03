import os
import subprocess
import sys
import shutil
import time
import re

# ===================================================================
# 全局配置
# ===================================================================

# *** 重要: 这是您项目中所有产物 (exe, dll) 所在的目录 ***
BUILD_DIR = r"C:\Computer\my_github\github_cpp\bills_master\Bills_Master_cpp\Bills_Master\build\bin"  

# *** 重要: 这是您存放原始 .txt 账单文件的目录 ***
BILLS_DIR = r"C:\Computer\my_github\github_cpp\bills_master\my_test\bills_test"

# *** _MODIFIED_: 新增一个变量，指向预处理后 .json 文件所在的目录 ***
IMPORT_DIR = r"C:\Computer\my_github\github_cpp\bills_master\my_test\txt2josn"

# <--- 定义所有需要复制的插件 DLL 名称 --->
PLUGIN_DLLS = [
    "md_month_formatter.dll", "rst_month_formatter.dll", "tex_month_formatter.dll", "typ_month_formatter.dll",
    "md_year_formatter.dll", "rst_year_formatter.dll", "tex_year_formatter.dll", "typ_year_formatter.dll"
]

# ===================================================================
#  ⭐️ 导出任务模式选择与格式配置 ⭐️
# ===================================================================
# 设置为 True  -> 只运行“导出全部”任务 (Running 'Export All' Tasks)
# 设置为 False -> 只运行“按日期导出”任务 (Running 'Export Date' Specific Tasks)
RUN_EXPORT_ALL_TASKS = True

#  <--- 新增: 统一管理所有需要测试的导出格式 --->
EXPORT_FORMATS = ['md', 'tex', 'typ', 'rst']
# ===================================================================

#  用于专项日期导出测试的日期配置
TEST_DATES = {
    "single_year": "2021",           # 测试导出一整年
    "single_month": "202201",        # 测试导出一个月
    "range_start": "202403",         # 测试日期范围的开始
    "range_end": "202404"            # 测试日期范围的结束
}

# 定义需要清理的文件和目录
FILES_TO_DELETE = ["bills.sqlite3"]
# *** [修改] ***：将 "output" 修改为 "py_output"
DIRS_TO_DELETE = ["txt_raw", "exported_files", "py_output", "build", "plugins", "config"]


# 定义常量和颜色
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
                print(f" ... {GREEN}OK{RESET} ({duration:.2f}s)")
                return True
            else:
                print(f" ... {RED}FAILED{RESET} (Code: {result.returncode})")
                return False

        except Exception as e:
            print(f" ... {RED}CRASHED{RESET}\n      Error: {e}")
            with open(log_path, 'w', encoding='utf-8') as f:
                f.write(f"--- Command ---\n{' '.join(full_command)}\n\n")
                f.write(f"--- CRITICAL ERROR ---\nFailed to execute command.\nError: {e}\n")
            return False

class TestPreparer:
    """预处理类：负责所有测试开始前的准备工作。"""
    def __init__(self, base_dir):
        self.base_dir = base_dir
        self.exe_name = "bill_master_cli.exe"
        self.local_plugins_dir = os.path.join(self.base_dir, "plugins")

    def prepare_runtime_env(self):
        """准备完整的运行时环境，包括可执行文件、配置文件和所有插件。"""
        print(f"{CYAN}--- 1. Preparing Runtime Environment ---{RESET}")
        
        if "C:/path/to/your/project/build/Debug" in BUILD_DIR:
            print(f"  {RED}错误: 请先在脚本中修改 'BUILD_DIR' 变量。{RESET}")
            return False
        if not os.path.exists(BUILD_DIR):
            print(f"  {RED}错误: 构建目录未找到 '{BUILD_DIR}'.{RESET}")
            return False

        # --- 步骤1: 复制可执行文件 ---
        source_exe_path = os.path.join(BUILD_DIR, self.exe_name)
        dest_exe_path = os.path.join(self.base_dir, self.exe_name)
        if not os.path.exists(source_exe_path):
            print(f"  {RED}错误: 源可执行文件未找到 '{source_exe_path}'.{RESET}")
            return False
        shutil.copy(source_exe_path, dest_exe_path)
        print(f"  {GREEN}已复制可执行文件: {self.exe_name}{RESET}")

        # --- 步骤2: 复制 config 文件夹 (新添加的逻辑) ---
        source_config_dir = os.path.join(BUILD_DIR, "config")
        dest_config_dir = os.path.join(self.base_dir, "config")
        if not os.path.exists(source_config_dir):
            print(f"  {RED}错误: 'config' 文件夹在构建目录中未找到 '{source_config_dir}'.{RESET}")
            return False
        
        # 使用 shutil.copytree 递归复制整个文件夹
        shutil.copytree(source_config_dir, dest_config_dir)
        print(f"  {GREEN}已复制配置文件夹: config{RESET}")

        # --- 步骤3: 复制插件 DLLs ---
        os.makedirs(self.local_plugins_dir, exist_ok=True)
        all_plugins_found = True
        for dll_name in PLUGIN_DLLS:
            source_dll_path = os.path.join(BUILD_DIR, dll_name)
            dest_dll_path = os.path.join(self.local_plugins_dir, dll_name)
            
            if not os.path.exists(source_dll_path):
                print(f"  {RED}错误: 插件未找到 '{source_dll_path}'.{RESET}")
                all_plugins_found = False
                continue
            
            shutil.copy(source_dll_path, dest_dll_path)
        
        if not all_plugins_found: return False
            
        print(f"  {GREEN}所有插件已复制, 运行时环境已准备就绪。{RESET}")
        return True

    def cleanup_and_setup_dirs(self):
        """清理旧的产物并设置日志目录。"""
        print(f"{CYAN}--- 0. Cleaning Artifacts & Setting up Directories ---{RESET}")
        for dir_name in DIRS_TO_DELETE:
            dir_path = os.path.join(self.base_dir, dir_name)
            if os.path.exists(dir_path): shutil.rmtree(dir_path)
        for file_name in FILES_TO_DELETE:
            file_path = os.path.join(self.base_dir, file_name)
            if os.path.exists(file_path): os.remove(file_path)
        
        # *** [修改] ***：将 "output" 修改为 "py_output"
        os.makedirs(os.path.join(self.base_dir, "py_output"))
        print(f"  {GREEN}清理完成, 已创建 'py_output' 日志目录。{RESET}")
        return True

class ImportTasks:
    """Import类：负责执行数据校验、修改和导入任务。"""
    # *** _MODIFIED_: 构造函数现在接收两个路径 ***
    def __init__(self, executor, bills_path, import_path):
        self.executor = executor
        self.bills_path = bills_path
        self.import_path = import_path # 保存导入路径

    def run(self):
        print(f"{CYAN}--- 2. Running Import Tasks ---{RESET}")
        if not self.executor.run("Validate", ["--validate", self.bills_path], "1_validate.log"): return False
        if not self.executor.run("Modify", ["--modify", self.bills_path], "2_modify.log"): return False
        # *** _MODIFIED_: 在导入步骤中使用新的 import_path ***
        if not self.executor.run("Import", ["--import", self.import_path], "3_import.log"): return False
        return True

class QueryTasks:
    """查询类：负责执行数据查询任务。"""
    def __init__(self, executor):
        self.executor = executor
    def run(self):
        print(f"{CYAN}--- 3. Running Query Tasks ---{RESET}")
        if not self.executor.run("Query Year", ["--query", "year", TEST_DATES['single_year']], "4_query_year.log"): return False
        if not self.executor.run("Query Month", ["--query", "month", TEST_DATES['single_month']], "5_query_month.log"): return False
        return True

class ExportTasks:
    """“全部导出”类：遍历所有格式执行 --export all 任务。"""
    def __init__(self, executor):
        self.executor = executor
    def run(self):
        print(f"{CYAN}--- 4. Running 'Export All' Tasks ---{RESET}")
        for fmt in EXPORT_FORMATS:
            step_name = f"Export All ({fmt.upper()})"
            log_filename = f"6_export_all_{fmt}.log"
            if not self.executor.run(step_name, ["--export", "all", "--format", fmt], log_filename):
                return False
        return True

class DateExportTasks:
    """“日期导出”类：遍历所有格式对每个日期场景执行 --export date 测试。"""
    def __init__(self, executor):
        self.executor = executor
    def run(self):
        print(f"{CYAN}--- 4. Running 'Export Date' Specific Tasks ---{RESET}")
        
        # 测试场景1: 导出单个年份 (遍历所有格式)
        for fmt in EXPORT_FORMATS:
            step_name = f"Export Year {TEST_DATES['single_year']} ({fmt.upper()})"
            log_filename = f"10_export_date_year_{fmt}.log"
            cmd_args = ["-e", "d", TEST_DATES["single_year"], "-f", fmt]
            if not self.executor.run(step_name, cmd_args, log_filename): return False

        # 测试场景2: 导出单个月份 (遍历所有格式)
        for fmt in EXPORT_FORMATS:
            step_name = f"Export Month {TEST_DATES['single_month']} ({fmt.upper()})"
            log_filename = f"11_export_date_month_{fmt}.log"
            cmd_args = ["-e", "d", TEST_DATES["single_month"], "-f", fmt]
            if not self.executor.run(step_name, cmd_args, log_filename): return False

        # 测试场景3: 导出日期范围 (遍历所有格式)
        for fmt in EXPORT_FORMATS:
            step_name = f"Export Range ({fmt.upper()})"
            log_filename = f"12_export_date_range_{fmt}.log"
            cmd_args = ["-e", "d", TEST_DATES["range_start"], TEST_DATES["range_end"], "-f", fmt]
            if not self.executor.run(step_name, cmd_args, log_filename): return False
            
        return True

def main():
    """主函数，协调整个测试流程。"""
    if os.name == 'nt': os.system('color')
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    preparer = TestPreparer(script_dir)
    if not preparer.cleanup_and_setup_dirs(): sys.exit(1)
    if not preparer.prepare_runtime_env(): sys.exit(1)

    exe_path = os.path.join(script_dir, preparer.exe_name)
    bills_path = BILLS_DIR 
    # 处理后的json文件路径
    import_path = IMPORT_DIR
    # *** [修改] ***：将 "output" 修改为 "py_output"
    output_dir = os.path.join(script_dir, "py_output")
    
    if "C:/path/to/your/bills_folder" in bills_path:
        print(f"\n{RED}错误: 请先在脚本顶部的 'BILLS_DIR' 变量中设置您的账单文件夹路径。{RESET}")
        sys.exit(1)
    if not os.path.exists(bills_path):
        print(f"\n{RED}错误: 'bills' 文件夹不存在: '{bills_path}'{RESET}")
        print(f"{YELLOW}请检查脚本顶部的 'BILLS_DIR' 变量是否指向了正确的路径。{RESET}")
        sys.exit(1)

    executor = CommandExecutor(exe_path, output_dir)
    # *** _MODIFIED_: 将两个路径都传递给 ImportTasks 的实例 ***
    importer = ImportTasks(executor, bills_path, import_path)
    querier = QueryTasks(executor)
    exporter = ExportTasks(executor)
    date_exporter = DateExportTasks(executor)

    print(f"\n{CYAN}========== Starting Test Sequence =========={RESET}")
    
    # 根据全局变量 RUN_EXPORT_ALL_TASKS 决定执行哪个导出流程
    base_tasks_ok = importer.run() and querier.run()
    final_result = False

    if base_tasks_ok:
        if RUN_EXPORT_ALL_TASKS:
            # 执行“全部导出”任务
            final_result = exporter.run()
        else:
            # 执行“按日期导出”任务
            final_result = date_exporter.run()

    if final_result:
        print(f"\n{GREEN}✅ All test steps completed successfully!{RESET}")
        print(f"{GREEN}   Check the 'py_output' directory for detailed logs.{RESET}")
    else:
        print(f"\n{RED}❌ A test step failed. Please check the corresponding log file in the 'py_output' directory for details.{RESET}")

if __name__ == "__main__":
    main()