import concurrent.futures
import os
import time
from collections.abc import Callable

from tqdm import tqdm  # type: ignore

from ..task_utils import discover_tasks, filter_incremental_tasks
from .single_file import compile_md_via_typ, compile_single_file


def process_directory(
    source_dir: str,
    base_output_dir: str,
    file_extension: str,
    log_file_type: str,
    command_builder: Callable[[str, str, str], list[str]],
    max_workers: int | None = None,
    post_process_hook: Callable[[str], None] | None = None,
    quiet: bool = False,
    incremental: bool = True,
) -> tuple[int, int, float, list[str]]:
    dir_start_time = time.perf_counter()
    worker_count = max_workers or os.cpu_count()
    source_folder_name = os.path.basename(os.path.abspath(source_dir))
    type_specific_output_root = os.path.join(base_output_dir, source_folder_name)

    if not quiet:
        print(f"\n===== 开始处理 {log_file_type} (最多 {worker_count} 个并行任务) =====")
        print(f"源: '{os.path.abspath(source_dir)}' -> 输出: '{type_specific_output_root}'")

    tasks_to_run = discover_tasks(source_dir, base_output_dir, file_extension)
    if not tasks_to_run:
        if not quiet:
            print(f"\n在 '{source_dir}' 中没有找到 {file_extension} 文件。")
        return 0, 0, 0.0, []

    updated_files: list[str] = []
    if incremental:
        tasks_to_run, _, updated_files = filter_incremental_tasks(
            tasks_to_run, type_specific_output_root, quiet
        )

    if not tasks_to_run:
        if not quiet:
            print("\n所有文件都已是最新版本，无需编译。")
        return 0, 0, 0.0, []

    success_count = 0
    failure_count = 0
    with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
        future_to_file = {
            executor.submit(compile_single_file, *task, command_builder, log_file_type): task[0]
            for task in tasks_to_run
        }
        progress_bar = tqdm(
            concurrent.futures.as_completed(future_to_file),
            total=len(tasks_to_run),
            desc=f"编译 {log_file_type}",
            unit="file",
            disable=quiet,
        )
        for future in progress_bar:
            try:
                result = future.result()
                if result["success"]:
                    success_count += 1
                    if not quiet:
                        progress_bar.set_postfix_str(f"{result['log']} ({result['duration']:.2f}s)")
                else:
                    failure_count += 1
                    tqdm.write(result["log"])
            except Exception as e:
                failure_count += 1
                tqdm.write(f"❌ 处理时发生严重错误: {e}")

    if post_process_hook:
        post_process_hook(type_specific_output_root)
    dir_duration = time.perf_counter() - dir_start_time
    return success_count, failure_count, dir_duration, updated_files


def process_directory_md_via_typ(
    source_dir: str,
    base_output_dir: str,
    font: str,
    max_workers: int | None = None,
    quiet: bool = False,
    incremental: bool = True,
) -> tuple[list[dict], float, list[str]]:
    dir_start_time = time.perf_counter()
    worker_count = max_workers or os.cpu_count()
    source_folder_name = os.path.basename(os.path.abspath(source_dir))
    type_specific_output_root = os.path.join(base_output_dir, source_folder_name)

    if not quiet:
        print(f"\n===== 开始处理 MD->Typ->PDF (最多 {worker_count} 个并行任务) =====")
        print(f"源: '{os.path.abspath(source_dir)}' -> 输出: '{type_specific_output_root}'")

    try:
        os.makedirs(type_specific_output_root, exist_ok=True)
    except OSError as e:
        if not quiet:
            print(f"❌ 致命错误: 无法创建根输出目录 '{type_specific_output_root}': {e}")
        return [], time.perf_counter() - dir_start_time, []

    initial_tasks = discover_tasks(source_dir, base_output_dir, ".md")
    if not initial_tasks:
        if not quiet:
            print(f"\n在 '{source_dir}' 中没有找到 .md 文件。")
        return [], 0.0, []

    tasks_with_font = [task + (font,) for task in initial_tasks]

    tasks_to_run = tasks_with_font
    skipped_count = 0
    updated_files: list[str] = []
    if incremental:
        tasks_to_run, skipped_count, updated_files = filter_incremental_tasks(
            tasks_to_run, type_specific_output_root, quiet
        )

    results: list[dict] = [{"success": True, "skipped": True}] * skipped_count

    if not tasks_to_run:
        if not quiet:
            print("\n所有文件都已是最新版本，无需编译。")
        return results, time.perf_counter() - dir_start_time, []

    with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
        future_to_file = {
            executor.submit(compile_md_via_typ, *task): task[0] for task in tasks_to_run
        }
        progress_bar = tqdm(
            concurrent.futures.as_completed(future_to_file),
            total=len(tasks_to_run),
            desc="编译 MD->Typ->PDF",
            unit="file",
            disable=quiet,
        )
        for future in progress_bar:
            try:
                result = future.result()
                results.append(result)
                if result.get("success") and not result.get("skipped") and not quiet:
                    progress_bar.set_postfix_str(
                        f"{result['log']} (总耗时: {result.get('total_time', 0):.2f}s)"
                    )
                elif not result.get("success"):
                    tqdm.write(result["log"])
            except Exception as e:
                tqdm.write(f"❌ 处理时发生严重错误: {e}")

    dir_duration = time.perf_counter() - dir_start_time
    return results, dir_duration, updated_files
