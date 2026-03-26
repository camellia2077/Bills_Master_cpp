# tasks.py
import json
import os
import shutil
import zipfile
from pathlib import Path

# [MODIFICATION] Use relative imports
from . import app_config as config
from . import constants


class HelpTasks:
    """帮助输出与兼容性类：负责执行 help 分层与已移除入口校验。"""

    def __init__(self, executor):
        self.executor = executor

    def run(self):
        print(
            f"{constants.CYAN}--- Running Help And Compatibility Tasks ---"
            f"{constants.RESET}"
        )
        tasks = [
            ("Top Help (No Args)", [], "00_help_no_args.log"),
            ("Top Help (--help)", ["--help"], "01_help_top_flag.log"),
            ("Workspace Help", ["workspace", "--help"], "02_help_workspace.log"),
            ("Report Help", ["report", "--help"], "03_help_report.log"),
            (
                "Report Export Help",
                ["report", "export", "--help"],
                "04_help_report_export.log",
            ),
            (
                "Report Export Year Help",
                ["report", "export", "year", "--help"],
                "05_help_report_export_year.log",
            ),
            (
                "Template Generate Help",
                ["template", "generate", "--help"],
                "06_help_template_generate.log",
            ),
            ("Config Help", ["config", "--help"], "07_help_config.log"),
            ("Meta Help", ["meta", "--help"], "08_help_meta.log"),
        ]
        for step_name, args, log_filename in tasks:
            if not self.executor.run(step_name, args, log_filename):
                return False

        legacy_tasks = [
            (
                "Removed Help Subcommand",
                ["help"],
                "09_removed_help.log",
                "The 'help' subcommand was removed. Use '--help' or '<command> --help'.",
            ),
            (
                "Removed Version Flag",
                ["--version"],
                "10_removed_version.log",
                "The '--version' flag was removed. Use 'meta version'.",
            ),
            (
                "Removed Notices Flag",
                ["--notices"],
                "11_removed_notices.log",
                "Legacy notices flags were removed. Use 'meta notices [--json]'.",
            ),
            (
                "Removed Notices JSON Flag",
                ["--notices-json"],
                "12_removed_notices_json.log",
                "Legacy notices flags were removed. Use 'meta notices [--json]'.",
            ),
        ]
        for step_name, args, log_filename, expected_text in legacy_tasks:
            if not self.executor.run_expected_failure(step_name, args, log_filename):
                return False
            if not self._assert_log_contains(log_filename, expected_text):
                return False

        return self._assert_help_structure()

    def _assert_help_structure(self):
        top_help_log = "00_help_no_args.log"
        if not self._assert_log_contains(top_help_log, "Bills CLI"):
            return False
        if not self._assert_log_contains(top_help_log, "workspace"):
            return False
        if not self._assert_log_contains(
            top_help_log, "Validate, convert, ingest, and bundle workspace data."
        ):
            return False
        if not self._assert_log_contains(top_help_log, "report"):
            return False
        if not self._assert_log_contains(top_help_log, "template"):
            return False
        if not self._assert_log_not_contains(top_help_log, "workspace validate <path>"):
            return False
        if not self._assert_log_not_contains(
            top_help_log, "report export all-months|all-years|all"
        ):
            return False
        if not self._assert_log_not_contains(top_help_log, "Formats:"):
            return False

        if not self._assert_log_contains("02_help_workspace.log", "validate"):
            return False
        if not self._assert_log_contains("02_help_workspace.log", "import-bundle"):
            return False
        if not self._assert_log_contains("03_help_report.log", "show"):
            return False
        if not self._assert_log_contains("03_help_report.log", "export"):
            return False

        report_export_log = "04_help_report_export.log"
        for token in ("year", "month", "range", "all-months", "all-years", "all"):
            if not self._assert_log_contains(report_export_log, token):
                return False
        if not self._assert_log_not_contains(
            report_export_log, "all-months|all-years|all"
        ):
            return False

        report_export_year_log = "05_help_report_export_year.log"
        if not self._assert_log_contains(report_export_year_log, "--format"):
            return False
        if not self._assert_log_contains(report_export_year_log, "Use 'config"):
            return False
        if not self._assert_log_contains(
            report_export_year_log, "inspect currently enabled formats."
        ):
            return False

        template_generate_log = "06_help_template_generate.log"
        if not self._assert_log_contains(
            template_generate_log, "template generate --period <YYYY-MM>"
        ):
            return False
        if not self._assert_log_contains(
            template_generate_log,
            "template generate --start-period <YYYY-MM> --end-period",
        ):
            return False
        if not self._assert_log_contains(
            template_generate_log,
            "--end-period",
        ):
            return False
        if not self._assert_log_contains(
            template_generate_log,
            "template generate --start-year <YYYY> --end-year <YYYY>",
        ):
            return False

        if not self._assert_log_contains("07_help_config.log", "inspect"):
            return False
        if not self._assert_log_contains("07_help_config.log", "formats"):
            return False
        if not self._assert_log_contains("08_help_meta.log", "version"):
            return False
        if not self._assert_log_contains("08_help_meta.log", "notices"):
            return False

        return True

    def _assert_log_contains(self, log_filename, expected_text):
        log_text = self.executor.read_log_text(log_filename)
        if expected_text in log_text:
            return True
        print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
        print(
            f"      {constants.RED}错误: 日志 '{log_filename}' 未包含期望内容: "
            f"'{expected_text}'{constants.RESET}"
        )
        return False

    def _assert_log_not_contains(self, log_filename, unexpected_text):
        log_text = self.executor.read_log_text(log_filename)
        if unexpected_text not in log_text:
            return True
        print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
        print(
            f"      {constants.RED}错误: 日志 '{log_filename}' 包含了不应出现的内容: "
            f"'{unexpected_text}'{constants.RESET}"
        )
        return False


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


class BundleTasks:
    """Parse bundle 类：负责执行 ZIP 导出/导入与失败回滚任务。"""

    def __init__(self, executor, bills_path, runtime_base_dir, run_output_root):
        self.executor = executor
        self.bills_path = Path(bills_path)
        self.runtime_base_dir = Path(runtime_base_dir)
        self.run_output_root = Path(run_output_root)
        self.runtime_config_dir = self.runtime_base_dir / "config"
        self.bundle_output_dir = self.run_output_root / "bundles"

    def run(self):
        print(f"{constants.CYAN}--- 7. Running Parse Bundle Tasks ---{constants.RESET}")
        self.bundle_output_dir.mkdir(parents=True, exist_ok=True)

        explicit_bundle_path = self.bundle_output_dir / "parse_bundle_explicit.zip"
        default_exports_dir = self.runtime_base_dir / "exports"

        if not self.executor.run(
            "Bundle Export (Explicit)",
            [
                "workspace",
                "export-bundle",
                str(self.bills_path),
                "--output",
                str(explicit_bundle_path),
            ],
            "27_bundle_export_explicit.log",
        ):
            return False
        if not explicit_bundle_path.exists():
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 解析包未生成: '{explicit_bundle_path}'{constants.RESET}"
            )
            return False
        if not self._validate_bundle_structure(explicit_bundle_path):
            return False
        if not self._validate_bundle_extract_only(explicit_bundle_path):
            return False

        if not self.executor.run(
            "Bundle Export (Default)",
            ["workspace", "export-bundle", str(self.bills_path)],
            "28_bundle_export_default.log",
        ):
            return False
        generated_default_bundles = sorted(default_exports_dir.glob("parse_bundle_*.zip"))
        if len(generated_default_bundles) != 1:
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 默认导出 ZIP 数量异常: '{default_exports_dir}'{constants.RESET}"
            )
            return False

        invalid_export_input = self.bundle_output_dir / "bundle_invalid_export_input"
        invalid_bundle_path = self.bundle_output_dir / "parse_bundle_invalid_export.zip"
        if invalid_export_input.exists():
            shutil.rmtree(invalid_export_input)
        invalid_export_input.mkdir(parents=True, exist_ok=True)
        source_sample = self.bills_path / config.TEST_DATES["single_year"] / (
            f"{config.TEST_DATES['single_month']}.txt"
        )
        invalid_export_sample_dir = invalid_export_input / config.TEST_DATES["single_year"]
        invalid_export_sample_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_sample, invalid_export_sample_dir / source_sample.name)
        (invalid_export_sample_dir / "invalid.txt").write_text(
            "date:2025-13\nremark:bad\n\nUnknownParent\nUnknownSub\n-12 broken line\n",
            encoding="utf-8",
        )
        if self.executor.run(
            "Bundle Export (Invalid TXT)",
            [
                "workspace",
                "export-bundle",
                str(invalid_export_input),
                "--output",
                str(invalid_bundle_path),
            ],
            "29_bundle_export_invalid.log",
        ):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 非法 TXT 导出本应失败，但命令返回成功。{constants.RESET}"
            )
            return False
        self._mark_last_expected_failure()
        if invalid_bundle_path.exists():
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 非法 TXT 导出失败后仍生成 ZIP: '{invalid_bundle_path}'{constants.RESET}"
            )
            return False

        target_import_dir = self.bundle_output_dir / "bundle_import_target"
        if target_import_dir.exists():
            shutil.rmtree(target_import_dir)
        conflicting_record_path = (
            target_import_dir
            / config.TEST_DATES["single_year"]
            / f"{config.TEST_DATES['single_month']}.txt"
        )
        conflicting_record_path.parent.mkdir(parents=True, exist_ok=True)
        conflicting_record_path.write_text("corrupted bundle target\n", encoding="utf-8")
        self._corrupt_runtime_config()

        if not self.executor.run(
            "Bundle Import (Success)",
            [
                "workspace",
                "import-bundle",
                str(explicit_bundle_path),
                str(target_import_dir),
            ],
            "30_bundle_import_success.log",
        ):
            return False
        if not self._validate_import_success(explicit_bundle_path, target_import_dir):
            return False

        config_snapshot = self._read_runtime_config_texts()
        invalid_txt_bundle_path = self.bundle_output_dir / "parse_bundle_invalid_txt.zip"
        self._rewrite_bundle(
            source_bundle_path=explicit_bundle_path,
            destination_bundle_path=invalid_txt_bundle_path,
            replacements={
                f"records/{config.TEST_DATES['single_year']}/{config.TEST_DATES['single_month']}.txt": (
                    "date:2025-13\nremark:bad\n\nUnknownParent\nUnknownSub\n-5 invalid\n"
                ),
            },
        )
        atomicity_target_dir = self.bundle_output_dir / "bundle_import_atomicity_target"
        if atomicity_target_dir.exists():
            shutil.rmtree(atomicity_target_dir)
        atomicity_target_dir.mkdir(parents=True, exist_ok=True)
        if self.executor.run(
            "Bundle Import (Invalid TXT)",
            [
                "workspace",
                "import-bundle",
                str(invalid_txt_bundle_path),
                str(atomicity_target_dir),
            ],
            "31_bundle_import_invalid_txt.log",
        ):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 非法 TXT 导入本应失败，但命令返回成功。{constants.RESET}"
            )
            return False
        self._mark_last_expected_failure()
        if not self._assert_runtime_config_unchanged(config_snapshot):
            return False
        if any(path.is_file() for path in atomicity_target_dir.rglob("*.txt")):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 非法 TXT 导入失败后仍写入了目标目录。{constants.RESET}"
            )
            return False

        invalid_format_bundle_path = self.bundle_output_dir / "parse_bundle_invalid_format.zip"
        self._rewrite_bundle(
            source_bundle_path=explicit_bundle_path,
            destination_bundle_path=invalid_format_bundle_path,
            omit_entries={"config/export_formats.toml"},
            extra_entries={"extra.txt": "unexpected"},
        )
        invalid_format_target_dir = self.bundle_output_dir / "bundle_import_invalid_format_target"
        if invalid_format_target_dir.exists():
            shutil.rmtree(invalid_format_target_dir)
        invalid_format_target_dir.mkdir(parents=True, exist_ok=True)
        if self.executor.run(
            "Bundle Import (Invalid Format)",
            [
                "workspace",
                "import-bundle",
                str(invalid_format_bundle_path),
                str(invalid_format_target_dir),
            ],
            "32_bundle_import_invalid_format.log",
        ):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 非法格式导入本应失败，但命令返回成功。{constants.RESET}"
            )
            return False
        self._mark_last_expected_failure()
        if not self._assert_runtime_config_unchanged(config_snapshot):
            return False
        if any(path.is_file() for path in invalid_format_target_dir.rglob("*.txt")):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 非法格式导入失败后仍写入了目标目录。{constants.RESET}"
            )
            return False

        return True

    def _validate_bundle_structure(self, bundle_path: Path) -> bool:
        with zipfile.ZipFile(bundle_path) as archive:
            archive_names = sorted(archive.namelist())

        required_entries = {
            "manifest.json",
            "config/validator_config.toml",
            "config/modifier_config.toml",
            "config/export_formats.toml",
        }
        if not required_entries.issubset(set(archive_names)):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 解析包缺少必需条目: '{bundle_path}'{constants.RESET}"
            )
            return False
        if not any(name.startswith("records/") and name.endswith(".txt") for name in archive_names):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 解析包中未发现 records/*.txt 条目: '{bundle_path}'{constants.RESET}"
            )
            return False
        if any(name.startswith("/") or "\\" in name or ".." in name.split("/") for name in archive_names):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 解析包包含非法路径条目: '{bundle_path}'{constants.RESET}"
            )
            return False
        return True

    def _corrupt_runtime_config(self) -> None:
        for file_name in (
            "validator_config.toml",
            "modifier_config.toml",
            "export_formats.toml",
        ):
            (self.runtime_config_dir / file_name).write_text(
                "# corrupted before bundle import\nnot valid = [\n",
                encoding="utf-8",
            )

    def _read_runtime_config_texts(self) -> dict[str, str]:
        result: dict[str, str] = {}
        for file_name in (
            "validator_config.toml",
            "modifier_config.toml",
            "export_formats.toml",
        ):
            result[file_name] = (self.runtime_config_dir / file_name).read_text(
                encoding="utf-8"
            )
        return result

    def _validate_import_success(self, bundle_path: Path, target_import_dir: Path) -> bool:
        with zipfile.ZipFile(bundle_path) as archive:
            bundled_validator = archive.read("config/validator_config.toml").decode("utf-8")
            bundled_modifier = archive.read("config/modifier_config.toml").decode("utf-8")
            bundled_formats = archive.read("config/export_formats.toml").decode("utf-8")
            bundled_record = archive.read(
                f"records/{config.TEST_DATES['single_year']}/{config.TEST_DATES['single_month']}.txt"
            ).decode("utf-8")

        expected_configs = {
            "validator_config.toml": bundled_validator,
            "modifier_config.toml": bundled_modifier,
            "export_formats.toml": bundled_formats,
        }
        for file_name, expected_text in expected_configs.items():
            actual_text = (self.runtime_config_dir / file_name).read_text(encoding="utf-8")
            if self._normalize_text(actual_text) != self._normalize_text(expected_text):
                print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
                print(
                    f"      {constants.RED}错误: 导入后 config 未按 ZIP 覆盖: '{file_name}'{constants.RESET}"
                )
                return False

        target_record = (
            target_import_dir
            / config.TEST_DATES["single_year"]
            / f"{config.TEST_DATES['single_month']}.txt"
        )
        if not target_record.exists():
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 导入后 TXT 未解压到目标目录: '{target_record}'{constants.RESET}"
            )
            return False
        if self._normalize_text(target_record.read_text(encoding="utf-8")) != self._normalize_text(
            bundled_record
        ):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 导入后同名 TXT 未被覆盖为 ZIP 内容。{constants.RESET}"
            )
            return False
        return True

    def _validate_bundle_extract_only(self, bundle_path: Path) -> bool:
        runtime_config_snapshot = self._read_runtime_config_texts()
        extract_root = self.bundle_output_dir / "bundle_extract_only"
        if extract_root.exists():
            shutil.rmtree(extract_root)
        extract_root.mkdir(parents=True, exist_ok=True)

        with zipfile.ZipFile(bundle_path) as archive:
            archive.extractall(extract_root)

        expected_top_level = {"manifest.json", "config", "records"}
        actual_top_level = {path.name for path in extract_root.iterdir()}
        if actual_top_level != expected_top_level:
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 纯解压后的顶层结构异常: '{extract_root}'{constants.RESET}"
            )
            return False

        config_root = extract_root / "config"
        expected_config_files = {
            "validator_config.toml",
            "modifier_config.toml",
            "export_formats.toml",
        }
        actual_config_files = {path.name for path in config_root.iterdir() if path.is_file()}
        if actual_config_files != expected_config_files:
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 纯解压后的 config 文件集合异常: '{config_root}'{constants.RESET}"
            )
            return False

        record_files = sorted(path for path in (extract_root / "records").rglob("*.txt"))
        if not record_files:
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 纯解压后 records 目录中没有 TXT 文件。{constants.RESET}"
            )
            return False
        if any(path.suffix.lower() != ".txt" for path in (extract_root / "records").rglob("*") if path.is_file()):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 纯解压后的 records 目录中出现了非 TXT 文件。{constants.RESET}"
            )
            return False

        manifest_payload = json.loads((extract_root / "manifest.json").read_text(encoding="utf-8"))
        if manifest_payload.get("record_count") != len(record_files):
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: manifest.json 的 record_count 与纯解压结果不一致。{constants.RESET}"
            )
            return False
        if not self._assert_runtime_config_unchanged(runtime_config_snapshot):
            return False
        return True

    def _assert_runtime_config_unchanged(self, expected_snapshot: dict[str, str]) -> bool:
        current_snapshot = self._read_runtime_config_texts()
        if current_snapshot != expected_snapshot:
            print(f" ... {constants.RED}CRITICAL FAILURE{constants.RESET}")
            print(
                f"      {constants.RED}错误: 失败导入后 runtime config 发生了变化。{constants.RESET}"
            )
            return False
        return True

    def _rewrite_bundle(
        self,
        source_bundle_path: Path,
        destination_bundle_path: Path,
        *,
        replacements: dict[str, str] | None = None,
        omit_entries: set[str] | None = None,
        extra_entries: dict[str, str] | None = None,
    ) -> None:
        replacements = replacements or {}
        omit_entries = omit_entries or set()
        extra_entries = extra_entries or {}

        with zipfile.ZipFile(source_bundle_path) as source_archive:
            bundle_entries = {
                info.filename: source_archive.read(info.filename)
                for info in source_archive.infolist()
                if not info.is_dir()
            }

        for file_name in omit_entries:
            bundle_entries.pop(file_name, None)
        for file_name, replacement_text in replacements.items():
            bundle_entries[file_name] = replacement_text.encode("utf-8")
        for file_name, replacement_text in extra_entries.items():
            bundle_entries[file_name] = replacement_text.encode("utf-8")

        if destination_bundle_path.exists():
            destination_bundle_path.unlink()
        with zipfile.ZipFile(destination_bundle_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for file_name in sorted(bundle_entries):
                archive.writestr(file_name, bundle_entries[file_name])

    def _normalize_text(self, text: str) -> str:
        return text.replace("\r\n", "\n").replace("\r", "\n")

    def _mark_last_expected_failure(self) -> None:
        if not self.executor.records:
            return
        self.executor.records[-1]["status"] = "expected_fail"
