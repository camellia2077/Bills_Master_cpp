import os
import sys
import json
import shutil
from datetime import datetime
from pathlib import Path

TEST_SUITE_DIR = Path(__file__).resolve().parent
TEST_ROOT = TEST_SUITE_DIR.parents[2]
if str(TEST_ROOT) not in sys.path:
    sys.path.insert(0, str(TEST_ROOT))

from framework._test_internal import app_config as config
from framework._test_internal import constants
from framework._test_internal.executor import CommandExecutor
from framework._test_internal.preparer import TestPreparer
from framework._test_internal.tasks import DateExportTasks, ExportTasks, ImportTasks, QueryTasks

SUMMARY_FILENAME = "test_summary.json"


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
    success_count = sum(1 for rec in records if rec.get("status") == "ok")
    failed_count = len(records) - success_count
    return {
        "ok": ok,
        "mode": mode,
        "total": len(records),
        "success": success_count,
        "failed": failed_count,
        "note": note,
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
    return project_output_root


def resolve_summary_path(project_output_root: Path) -> Path:
    runtime_summary = str(getattr(config, "RUNTIME_SUMMARY_PATH", "")).strip()
    if runtime_summary:
        return Path(runtime_summary).resolve()
    return project_output_root / SUMMARY_FILENAME


def archive_project_outputs(run_output_root: Path, runtime_base_dir: Path) -> None:
    shared_output_root = runtime_base_dir / "output"
    mappings = [
        (shared_output_root / "txt2josn", run_output_root / "txt2josn"),
        (shared_output_root / "exported_files", run_output_root / "exported_files"),
    ]
    for source_dir, target_dir in mappings:
        if not source_dir.exists():
            continue
        if target_dir.exists():
            shutil.rmtree(target_dir)
        target_dir.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(source_dir), str(target_dir))


def main():
    if os.name == "nt":
        os.system("color")
    print(f"[TEST_START] {datetime.now().isoformat(timespec='seconds')}")

    runtime_base_dir = resolve_runtime_base_dir()
    test_root = str(runtime_base_dir)
    project_output_root = TEST_ROOT / "output" / config.OUTPUT_PROJECT
    run_output_root = resolve_run_output_root(project_output_root)
    summary_path = resolve_summary_path(project_output_root)
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
        print(f"{constants.YELLOW}--- Skipped Environment Preparation Step (as per config) ---{constants.RESET}")

    # --- 步骤 3: 执行指令测试 (根据开关决定是否执行) ---
    if config.RUN_TESTS:
        final_result, summary_payload = run_test_sequence(
            runtime_base_dir, preparer, run_output_root
        )
        archive_project_outputs(run_output_root, runtime_base_dir)
        write_summary_file(summary_path, summary_payload)
        if not final_result:
            sys.exit(1)
    else:
        print(f"{constants.YELLOW}--- Skipped Test Execution Step (as per config) ---{constants.RESET}")
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
        print(f"{constants.YELLOW}请在 config.toml 中设置 'run_prepare_env = true' 并重新运行脚本来复制文件。{constants.RESET}")
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
    importer = ImportTasks(executor, config.BILLS_DIR, config.IMPORT_DIR)
    querier = QueryTasks(executor)
    exporter = ExportTasks(executor)
    date_exporter = DateExportTasks(executor)

    # --- 执行测试序列 ---
    print(f"\n{constants.CYAN}========== Starting Test Sequence =========={constants.RESET}")
    
    base_tasks_ok = importer.run() and querier.run()
    final_result = False

    if base_tasks_ok:
        if config.RUN_EXPORT_ALL_TASKS:
            final_result = exporter.run()
        else:
            final_result = date_exporter.run()

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
