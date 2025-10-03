# tasks.py
import os
# [MODIFICATION] Use relative imports
from . import app_config as config
from . import constants

class ImportTasks:
    """Import类负责执行数据校验、修改和导入任务。"""
    def __init__(self, executor, bills_path, import_path):
        self.executor = executor
        self.bills_path = bills_path
        self.import_path = import_path

    def run(self):
        print(f"{constants.CYAN}--- 2. Running Import Tasks ---{constants.RESET}")
        if not self.executor.run("Validate", ["--validate", self.bills_path], "1_validate.log"): return False
        
        if not self.executor.run("Modify", ["--modify", self.bills_path], "2_modify.log"): return False

        if not os.path.exists(self.import_path):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(f"      {constants.RED}错误: '--modify' 命令执行了, 但未能创建 import 目录: '{self.import_path}'{constants.RESET}")
            return False

        if not self.executor.run("Import", ["--import", self.import_path], "3_import.log"): return False
        
        return True

class QueryTasks:
    """查询类：负责执行数据查询任务。"""
    def __init__(self, executor):
        self.executor = executor
    def run(self):
        print(f"{constants.CYAN}--- 3. Running Query Tasks ---{constants.RESET}")
        if not self.executor.run("Query Year", ["--query", "year", config.TEST_DATES['single_year']], "4_query_year.log"): return False
        if not self.executor.run("Query Month", ["--query", "month", config.TEST_DATES['single_month']], "5_query_month.log"): return False
        return True

class ExportTasks:
    """“全部导出”类：遍历所有格式执行 --export all 任务。"""
    def __init__(self, executor):
        self.executor = executor
    def run(self):
        print(f"{constants.CYAN}--- 4. Running 'Export All' Tasks ---{constants.RESET}")
        for fmt in config.EXPORT_FORMATS:
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
        print(f"{constants.CYAN}--- 4. Running 'Export Date' Specific Tasks ---{constants.RESET}")
        
        # ... (rest of the file is unchanged) ...
        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export Year {config.TEST_DATES['single_year']} ({fmt.upper()})"
            log_filename = f"10_export_date_year_{fmt}.log"
            cmd_args = ["-e", "d", config.TEST_DATES["single_year"], "-f", fmt]
            if not self.executor.run(step_name, cmd_args, log_filename): return False

        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export Month {config.TEST_DATES['single_month']} ({fmt.upper()})"
            log_filename = f"11_export_date_month_{fmt}.log"
            cmd_args = ["-e", "d", config.TEST_DATES["single_month"], "-f", fmt]
            if not self.executor.run(step_name, cmd_args, log_filename): return False

        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export Range ({fmt.upper()})"
            log_filename = f"12_export_date_range_{fmt}.log"
            cmd_args = ["-e", "d", config.TEST_DATES["range_start"], config.TEST_DATES["range_end"], "-f", fmt]
            if not self.executor.run(step_name, cmd_args, log_filename): return False
            
        return True