# tasks.py
import os
from pathlib import Path

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
        ingest_mode = config.INGEST_MODE.lower()
        if ingest_mode == "ingest":
            cmd_args = ["workspace", "ingest", self.bills_path]
            if config.INGEST_WRITE_JSON:
                cmd_args.append("--write-json-cache")
            if not self.executor.run("Ingest", cmd_args, "1_ingest.log"):
                return False
            return True

        if ingest_mode != "stepwise":
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 未知 ingest_mode: '{config.INGEST_MODE}'{constants.RESET}"
            )
            return False

        if not self.executor.run(
            "Validate",
            ["workspace", "validate", self.bills_path],
            "1_validate.log",
        ):
            return False

        if not self.executor.run(
            "Convert",
            ["workspace", "convert", self.bills_path, "--write-json-cache"],
            "2_convert.log",
        ):
            return False

        if not os.path.exists(self.import_path):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 'workspace convert --write-json-cache' 已执行, 但未能创建 import 目录: '{self.import_path}'{constants.RESET}"
            )
            return False

        if not self.executor.run(
            "Import",
            ["workspace", "import-json", self.import_path],
            "3_import.log",
        ):
            return False

        return True


class QueryTasks:
    """查询类：负责执行数据查询任务。"""

    def __init__(self, executor):
        self.executor = executor

    def run(self):
        print(f"{constants.CYAN}--- 3. Running Query Tasks ---{constants.RESET}")
        if not self.executor.run(
            "Query Year",
            ["report", "show", "year", config.TEST_DATES["single_year"]],
            "4_query_year.log",
        ):
            return False
        if not self.executor.run(
            "Query Month",
            ["report", "show", "month", config.TEST_DATES["single_month"]],
            "5_query_month.log",
        ):
            return False
        return True


class ExportTasks:
    """“全部导出”类：遍历所有格式执行 `report export all` 任务。"""

    def __init__(self, executor):
        self.executor = executor

    def run(self):
        print(f"{constants.CYAN}--- 4. Running 'Export All' Tasks ---{constants.RESET}")
        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export All ({fmt.upper()})"
            log_filename = f"6_export_all_{fmt}.log"
            if not self.executor.run(
                step_name,
                ["report", "export", "all", "--format", fmt],
                log_filename,
            ):
                return False
        return True


class DateExportTasks:
    """“日期导出”类：遍历所有格式对 year/month/range 子命令做覆盖。"""

    def __init__(self, executor):
        self.executor = executor

    def run(self):
        print(f"{constants.CYAN}--- 4. Running 'Export Date' Specific Tasks ---{constants.RESET}")

        # ... (rest of the file is unchanged) ...
        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export Year {config.TEST_DATES['single_year']} ({fmt.upper()})"
            log_filename = f"10_export_date_year_{fmt}.log"
            cmd_args = [
                "report",
                "export",
                "year",
                config.TEST_DATES["single_year"],
                "--format",
                fmt,
            ]
            if not self.executor.run(step_name, cmd_args, log_filename):
                return False

        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export Month {config.TEST_DATES['single_month']} ({fmt.upper()})"
            log_filename = f"11_export_date_month_{fmt}.log"
            cmd_args = [
                "report",
                "export",
                "month",
                config.TEST_DATES["single_month"],
                "--format",
                fmt,
            ]
            if not self.executor.run(step_name, cmd_args, log_filename):
                return False

        for fmt in config.EXPORT_FORMATS:
            step_name = f"Export Range ({fmt.upper()})"
            log_filename = f"12_export_date_range_{fmt}.log"
            cmd_args = [
                "report",
                "export",
                "range",
                config.TEST_DATES["range_start"],
                config.TEST_DATES["range_end"],
                "--format",
                fmt,
            ]
            if not self.executor.run(step_name, cmd_args, log_filename):
                return False

        return True


class MetadataTasks:
    """元数据类：负责执行版本、notices 与 config inspect 任务。"""

    def __init__(self, executor):
        self.executor = executor

    def run(self):
        print(f"{constants.CYAN}--- 5. Running Metadata Tasks ---{constants.RESET}")
        tasks = [
            ("Version", ["meta", "version"], "20_version.log"),
            ("Notices Markdown", ["meta", "notices"], "21_notices.log"),
            ("Notices JSON", ["meta", "notices", "--json"], "22_notices_json.log"),
            ("Config Inspect", ["config", "inspect"], "23_config_inspect.log"),
        ]
        for step_name, args, log_filename in tasks:
            if not self.executor.run(step_name, args, log_filename):
                return False
        return True


class RecordTasks:
    """Record 类：负责执行模板生成、period 列举与 dry-run 预览任务。"""

    def __init__(self, executor, bills_path, run_output_root):
        self.executor = executor
        self.bills_path = bills_path
        self.run_output_root = Path(run_output_root)

    def run(self):
        print(f"{constants.CYAN}--- 6. Running Record Tasks ---{constants.RESET}")
        record_output_dir = self.run_output_root / "record_templates"
        record_output_dir.mkdir(parents=True, exist_ok=True)

        if not self.executor.run(
            "Record Template",
            [
                "template",
                "generate",
                "--period",
                config.TEST_DATES["single_month"],
                "--output-dir",
                str(record_output_dir),
            ],
            "24_record_template.log",
        ):
            return False

        generated_file = (
            record_output_dir
            / config.TEST_DATES["single_month"][:4]
            / f"{config.TEST_DATES['single_month']}.txt"
        )
        if not generated_file.exists():
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 记录模板未生成: '{generated_file}'{constants.RESET}"
            )
            return False

        if not self.executor.run(
            "Record Preview",
            ["template", "preview", str(generated_file)],
            "25_record_preview.log",
        ):
            return False

        if not self.executor.run(
            "Record List",
            ["template", "list-periods", str(record_output_dir)],
            "26_record_list.log",
        ):
            return False

        return True
