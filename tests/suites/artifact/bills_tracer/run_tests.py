import importlib
import json
import os
import shutil
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from _bootstrap import bootstrap_test_paths

TEST_ROOT, REPO_ROOT = bootstrap_test_paths(__file__)

config: Any = None
constants: Any = None
resolve_artifact_project_root: Any = None
CommandExecutor: Any = None
TestPreparer: Any = None
HelpTasks: Any = None
DateExportTasks: Any = None
ExportTasks: Any = None
ImportTasks: Any = None
MetadataTasks: Any = None
QueryTasks: Any = None
RecordTasks: Any = None
BundleTasks: Any = None


def _bootstrap_imports() -> None:
    global config
    global constants
    global resolve_artifact_project_root
    global CommandExecutor
    global TestPreparer
    global HelpTasks
    global DateExportTasks
    global ExportTasks
    global ImportTasks
    global MetadataTasks
    global QueryTasks
    global RecordTasks
    global BundleTasks

    build_layout = importlib.import_module("tools.toolchain.services.build_layout")
    framework_config = importlib.import_module("framework.internal.app_config")
    framework_constants = importlib.import_module("framework.internal.constants")
    executor_module = importlib.import_module("framework.internal.executor")
    preparer_module = importlib.import_module("framework.internal.preparer")
    tasks_module = importlib.import_module("framework.internal.tasks")

    resolve_artifact_project_root = build_layout.resolve_artifact_project_root
    config = framework_config
    constants = framework_constants
    CommandExecutor = executor_module.CommandExecutor
    TestPreparer = preparer_module.TestPreparer
    HelpTasks = tasks_module.HelpTasks
    DateExportTasks = tasks_module.DateExportTasks
    ExportTasks = tasks_module.ExportTasks
    ImportTasks = tasks_module.ImportTasks
    MetadataTasks = tasks_module.MetadataTasks
    QueryTasks = tasks_module.QueryTasks
    RecordTasks = tasks_module.RecordTasks
    BundleTasks = tasks_module.BundleTasks


_bootstrap_imports()

SUMMARY_FILENAME = "test_summary.json"
FORMAT_OUTPUT_SPECS = {
    "json": ("standard_json", ".json"),
    "md": ("Markdown_bills", ".md"),
    "tex": ("LaTeX_bills", ".tex"),
    "rst": ("reST_bills", ".rst"),
    "typ": ("Typst_bills", ".typ"),
}


def write_summary_file(summary_path: Path, summary_payload: dict) -> None:
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text(
        json.dumps(summary_payload, ensure_ascii=False, separators=(",", ":")),
        encoding="utf-8",
    )


def build_summary(
    ok: bool,
    executor: CommandExecutor | None,
    mode: str,
    note: str = "",
) -> dict:
    records = executor.records if executor is not None else []
    success_count = sum(
        1
        for rec in records
        if rec.get("status") in {"ok", "expected_fail"}
    )
    failed_count = len(records) - success_count
    return {
        "ok": ok,
        "mode": mode,
        "total": len(records),
        "success": success_count,
        "failed": failed_count,
        "note": note,
        "export_formats": list(config.EXPORT_FORMATS),
    }


def resolve_runtime_base_dir() -> Path:
    runtime_base = str(config.RUNTIME_BASE_DIR).strip()
    if runtime_base:
        return Path(runtime_base).resolve()
    return TEST_ROOT


def resolve_run_output_root(project_output_root: Path) -> Path:
    runtime_output = str(getattr(config, "RUNTIME_OUTPUT_DIR", "")).strip()
    if runtime_output:
        return Path(runtime_output).resolve()
    return project_output_root / "latest"


def resolve_summary_path(project_output_root: Path) -> Path:
    runtime_summary = str(getattr(config, "RUNTIME_SUMMARY_PATH", "")).strip()
    if runtime_summary:
        return Path(runtime_summary).resolve()
    return project_output_root / "latest" / SUMMARY_FILENAME


def prepare_run_output_root(run_output_root: Path, summary_path: Path) -> None:
    cleanup_dirs = [
        run_output_root / "cache",
        run_output_root / "exports",
        run_output_root / "logs",
        run_output_root / "record_templates",
        run_output_root / "bundles",
        run_output_root / "txt2josn",
        run_output_root / "exported_files",
    ]
    cleanup_files = [
        run_output_root / "test_python_output.log",
        run_output_root / "run_manifest.json",
        summary_path,
    ]

    for directory in cleanup_dirs:
        if directory.exists():
            shutil.rmtree(directory)
    for file_path in cleanup_files:
        if file_path.exists():
            file_path.unlink()


def archive_project_outputs(run_output_root: Path, runtime_base_dir: Path) -> None:
    mappings = [
        (
            runtime_base_dir / "cache" / "txt2json",
            run_output_root / "cache" / "txt2json",
        ),
        (runtime_base_dir / "exports", run_output_root / "exports"),
    ]
    for source_dir, target_dir in mappings:
        if not source_dir.exists():
            continue
        if target_dir.exists():
            shutil.rmtree(target_dir)
        target_dir.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(source_dir), str(target_dir))


def validate_export_outputs(run_output_root: Path) -> list[str]:
    exported_root = run_output_root / "exports"
    if not exported_root.exists():
        return [f"missing exports directory: {exported_root}"]

    errors: list[str] = []
    for fmt in config.EXPORT_FORMATS:
        normalized = str(fmt).strip().lower()
        folder_name, extension = FORMAT_OUTPUT_SPECS.get(
            normalized,
            (f"{normalized}_bills", f".{normalized}"),
        )
        format_root = exported_root / folder_name
        if not format_root.exists():
            errors.append(f"missing format directory for {normalized}: {format_root}")
            continue

        matching_files = sorted(
            path
            for path in format_root.rglob("*")
            if path.is_file() and path.suffix.lower() == extension
        )
        if not matching_files:
            errors.append(
                f"no exported {normalized} files with extension {extension} under {format_root}"
            )
    return errors


def main():
    if os.name == "nt":
        os.system("color")
    print(f"[TEST_START] {datetime.now().isoformat(timespec='seconds')}")

    runtime_base_dir = resolve_runtime_base_dir()
    os.environ["BILLS_TRACER_RUNTIME_WORKSPACE_DIR"] = str(runtime_base_dir)
    test_root = str(runtime_base_dir)
    project_output_root = resolve_artifact_project_root(REPO_ROOT, config.OUTPUT_PROJECT)
    run_output_root = resolve_run_output_root(project_output_root)
    summary_path = resolve_summary_path(project_output_root)
    prepare_run_output_root(run_output_root, summary_path)
    preparer = TestPreparer(test_root)

    # --- 步骤 1: 清理环境 (根据开关决定是否执行) ---
    if config.RUN_CLEANUP:
        if not preparer.cleanup_and_setup_dirs():
            write_summary_file(
                summary_path,
                build_summary(
                    ok=False,
                    executor=None,
                    mode="setup",
                    note="cleanup_and_setup_dirs failed",
                ),
            )
            sys.exit(1)
    else:
        print(f"{constants.YELLOW}--- Skipped Cleanup Step (as per config) ---{constants.RESET}")

    # --- 步骤 2: 准备运行时环境/复制文件 (根据开关决定是否执行) ---
    if config.RUN_PREPARE_ENV:
        if not preparer.prepare_runtime_env():
            write_summary_file(
                summary_path,
                build_summary(
                    ok=False,
                    executor=None,
                    mode="setup",
                    note="prepare_runtime_env failed",
                ),
            )
            sys.exit(1)
    else:
        print(
            f"{constants.YELLOW}--- Skipped Environment Preparation Step (as per config) ---{constants.RESET}"
        )

    # --- 步骤 3: 执行指令测试 (根据开关决定是否执行) ---
    if config.RUN_TESTS:
        final_result, summary_payload = run_test_sequence(
            runtime_base_dir, preparer, run_output_root
        )
        archive_project_outputs(run_output_root, runtime_base_dir)
        if final_result:
            export_errors = validate_export_outputs(run_output_root)
            summary_payload["validated_export_outputs"] = len(export_errors) == 0
            if export_errors:
                final_result = False
                summary_payload["ok"] = False
                summary_payload["note"] = "; ".join(export_errors)
                print(f"{constants.RED}[FAILED] Export output validation failed:{constants.RESET}")
                for error in export_errors:
                    print(f"  - {error}")
            else:
                print(
                    f"{constants.GREEN}[OK] Export output validation passed for formats: "
                    f"{', '.join(config.EXPORT_FORMATS)}{constants.RESET}"
                )
        write_summary_file(summary_path, summary_payload)
        if not final_result:
            sys.exit(1)
    else:
        print(
            f"{constants.YELLOW}--- Skipped Test Execution Step (as per config) ---{constants.RESET}"
        )
        write_summary_file(
            summary_path,
            build_summary(
                ok=True,
                executor=None,
                mode="skipped",
                note="run_tests is disabled in config",
            ),
        )

    print(f"\n{constants.GREEN}[OK] Script finished.{constants.RESET}")


def run_test_sequence(runtime_base_dir: Path, preparer, run_output_root: Path):
    exe_path = runtime_base_dir / preparer.exe_name
    output_dir = run_output_root / "logs"
    output_dir.mkdir(parents=True, exist_ok=True)

    if not exe_path.exists():
        print(f"\n{constants.RED}错误: 可执行文件 '{preparer.exe_name}' 未找到。{constants.RESET}")
        print(
            f"{constants.YELLOW}请在 config.toml 中设置 'run_prepare_env = true' 并重新运行脚本来复制文件。{constants.RESET}"
        )
        return False, build_summary(
            ok=False,
            executor=None,
            mode="execution",
            note=f"executable not found: {preparer.exe_name}",
        )

    if not os.path.exists(config.BILLS_DIR):
        print(f"\n{constants.RED}错误: 'bills' 文件夹不存在: '{config.BILLS_DIR}'{constants.RESET}")
        print(f"{constants.YELLOW}请检查 config.toml 文件中的 'bills_dir' 变量。{constants.RESET}")
        return False, build_summary(
            ok=False,
            executor=None,
            mode="execution",
            note=f"bills_dir not found: {config.BILLS_DIR}",
        )

    # --- 初始化任务 ---
    executor = CommandExecutor(str(exe_path), str(output_dir))
    help_tasks = HelpTasks(executor)
    importer = ImportTasks(executor, config.BILLS_DIR, config.IMPORT_DIR)
    querier = QueryTasks(executor)
    exporter = ExportTasks(executor)
    date_exporter = DateExportTasks(executor)
    metadata_tasks = MetadataTasks(executor)
    record_tasks = RecordTasks(executor, config.BILLS_DIR, str(run_output_root))
    bundle_tasks = BundleTasks(
        executor,
        config.BILLS_DIR,
        runtime_base_dir,
        str(run_output_root),
    )

    # --- 执行测试序列 ---
    print(f"\n{constants.CYAN}========== Starting Test Sequence =========={constants.RESET}")

    base_tasks_ok = help_tasks.run() and importer.run() and querier.run()
    final_result = False

    if base_tasks_ok:
        if config.RUN_EXPORT_ALL_TASKS:
            final_result = exporter.run()
        else:
            final_result = date_exporter.run()
    if final_result:
        final_result = metadata_tasks.run()
    if final_result:
        final_result = record_tasks.run()
    if final_result:
        final_result = bundle_tasks.run()

    # --- 报告最终结果 ---
    if final_result:
        print(f"\n{constants.GREEN}[OK] All test steps completed successfully!{constants.RESET}")
        print(
            f"{constants.GREEN}   Check the "
            f"'{output_dir}' directory for detailed logs."
            f"{constants.RESET}"
        )
    else:
        print(
            f"\n{constants.RED}[FAILED] A test step failed. Please check the corresponding "
            f"log file in '{output_dir}' for details."
            f"{constants.RESET}"
        )
    summary_mode = "export_all" if config.RUN_EXPORT_ALL_TASKS else "date_export"
    summary_payload = build_summary(
        ok=final_result,
        executor=executor,
        mode=summary_mode,
    )
    return final_result, summary_payload


if __name__ == "__main__":
    main()
