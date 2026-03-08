import argparse
import os
from collections.abc import Callable
from typing import Any

from .common import format_time
from .format_handlers import handle_md, handle_rst, handle_tex, handle_typ


def _discover_tasks(
    source_dir: str, compiler_map: dict, types_to_compile: list[str]
) -> list[dict[str, Any]]:
    tasks = []
    types_to_process_lower = [t.lower() for t in types_to_compile]
    print(f"注意：根据配置，将只编译以下类型 -> {types_to_compile}")
    for subdir_name in os.listdir(source_dir):
        full_subdir_path = os.path.join(source_dir, subdir_name)
        if not os.path.isdir(full_subdir_path):
            continue
        base_name_to_match = subdir_name.split("_")[0].lower()
        for keywords, (log_name, handler_func) in compiler_map.items():
            if base_name_to_match in keywords:
                if log_name.lower() in types_to_process_lower:
                    print(f"\n>>> 检测到 '{subdir_name}' -> 将使用 {log_name} 编译器...")
                    tasks.append(
                        {
                            "log_name": log_name,
                            "handler_func": handler_func,
                            "source_path": full_subdir_path,
                        }
                    )
                else:
                    print(
                        f"\n>>> 检测到 '{subdir_name}' -> 类型 '{log_name}' 不在编译列表中，已跳过。"
                    )
                break
    return tasks


def _execute_tasks(
    tasks: list[dict[str, Any]], args: argparse.Namespace
) -> tuple[dict, dict, dict]:
    timing_summary = {}
    compilation_stats = {}
    update_summary: dict[str, int] = {}

    for task in tasks:
        task_args = argparse.Namespace(**vars(args))
        task_args.source_dir = task["source_path"]

        success_count, failure_count, duration, updated_files = task["handler_func"](task_args)

        if duration > 0:
            timing_summary[task["log_name"]] = (duration, success_count + failure_count)
        if (success_count + failure_count) > 0:
            compilation_stats[task["log_name"]] = {
                "success": success_count,
                "failed": failure_count,
            }

        if updated_files:
            update_summary[task["log_name"]] = len(updated_files)

    return timing_summary, compilation_stats, update_summary


def _print_time_summary(timing_summary: dict):
    if not timing_summary:
        return
    print("\n\n" + "=" * 45)
    print("⏱️" + " " * 14 + "编译时间摘要" + " " * 15 + "⏱️")
    print("=" * 45)
    for format_name, (duration, count) in timing_summary.items():
        avg_time_str = f"平均: {(duration / count):.2f} 秒/文件" if count > 0 else ""
        print(f"- {format_name:<10} | 总耗时: {format_time(duration)} | {avg_time_str}")
    print("=" * 45)


def _print_stats_summary(stats: dict):
    if not stats:
        return
    print("\n" + "=" * 45)
    print("📊" + " " * 12 + "最终编译统计报告" + " " * 13 + "📊")
    print("=" * 45)
    print(f"{'语言':<12} | {'✅ 成功':<10} | {'❌ 失败':<10}")
    print("-" * 45)
    for lang, counts in stats.items():
        print(f"{lang:<12} | {counts.get('success', 0):<10} | {counts.get('failed', 0):<10}")
    print("=" * 45)


def _print_update_summary(update_summary: dict):
    if not update_summary:
        return
    print("\n" + "=" * 45)
    print("🔄" + " " * 14 + "更新文件统计" + " " * 15 + "🔄")
    print("=" * 45)
    print(f"{'语言':<12} | {'更新数量':<10}")
    print("-" * 45)
    for lang, count in update_summary.items():
        print(f"{lang:<12} | {count:<10}")
    print("=" * 45)


def handle_auto(args: argparse.Namespace):
    parent_dir = args.source_dir
    print("===== 启动自动编译模式 =====")
    print(f"扫描父目录: '{parent_dir}'")
    compiler_map: dict[tuple[str, ...], tuple[str, Callable]] = {
        ("latex", "tex"): ("TeX", handle_tex),
        ("markdown", "md"): ("Markdown", handle_md),
        ("rst", "rest"): ("RST", handle_rst),
        ("typst", "typ"): ("Typst", handle_typ),
    }
    tasks_to_run = _discover_tasks(parent_dir, compiler_map, args.compile_types)
    if not tasks_to_run:
        print(
            f"\n在 '{parent_dir}' 中没有找到任何需要编译的目录。 (配置类型: {args.compile_types})"
        )
        return

    time_summary, stats_summary, update_summary = _execute_tasks(tasks_to_run, args)

    if time_summary:
        _print_time_summary(time_summary)
    if stats_summary:
        _print_stats_summary(stats_summary)
    if update_summary:
        _print_update_summary(update_summary)
