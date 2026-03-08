import argparse
import os
import shutil

from ..compilers import PandocCommandBuilder
from ..core import process_directory, process_directory_md_via_typ


def _run_benchmark(args: argparse.Namespace) -> tuple[int, int, float, list[str]]:
    print("\n" + "=" * 50)
    print("🚀  启动 Markdown 编译基准测试模式  🚀")
    print(f"   比较方法: {', '.join(args.markdown_compilers)}")
    print(f"   循环次数: {args.benchmark_loops} 次")
    print(f"   使用字体: '{args.font}'")
    print("=" * 50)

    benchmark_results: dict[str, list[float]] = {
        compiler: [] for compiler in args.markdown_compilers
    }

    for i in range(args.benchmark_loops):
        print(f"\n--- 第 {i + 1}/{args.benchmark_loops} 轮测试 ---")
        for compiler in args.markdown_compilers:
            target_output_path = os.path.join(args.output_dir, os.path.basename(args.source_dir))
            if os.path.exists(target_output_path):
                shutil.rmtree(target_output_path)

            print(f"  > 正在测试: {compiler}...")
            duration = 0.0
            if compiler == "pandoc":
                builder = PandocCommandBuilder(source_format="gfm", font=args.font)
                _, _, duration, _ = process_directory(
                    args.source_dir,
                    args.output_dir,
                    ".md",
                    "Markdown",
                    builder,
                    args.jobs,
                    quiet=True,
                    incremental=False,
                )
            elif compiler == "typst":
                _, duration, _ = process_directory_md_via_typ(
                    args.source_dir,
                    args.output_dir,
                    font=args.font,
                    max_workers=args.jobs,
                    quiet=True,
                    incremental=False,
                )

            benchmark_results[compiler].append(duration)
            print(f"    本轮耗时: {duration:.4f} 秒")

    _print_benchmark_summary(benchmark_results)
    return 1, 0, sum(sum(v) for v in benchmark_results.values()), []


def _print_benchmark_summary(results: dict[str, list[float]]):
    print("\n" + "=" * 50)
    print("📊  基准测试结果摘要  📊")
    print("=" * 50)
    total_times = {compiler: sum(durations) for compiler, durations in results.items()}
    for compiler, total_time in total_times.items():
        avg_time = total_time / len(results[compiler])
        print(f"方法: {compiler.upper()}")
        print(f"  - 总耗时: {total_time:.4f} 秒")
        print(f"  - 平均耗时: {avg_time:.4f} 秒/轮")
    if len(total_times) > 1:
        best_compiler = min(total_times, key=total_times.get)
        print("-" * 50)
        print(f"🏆 结论: [{best_compiler.upper()}] 性能更优！")
    print("=" * 50)
