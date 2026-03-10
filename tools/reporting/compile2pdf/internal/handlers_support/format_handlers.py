import argparse
import os

from ..compilers import PandocCommandBuilder, build_tex_command, build_typ_command
from ..core import process_directory, process_directory_md_via_typ
from .benchmark import _run_benchmark


def handle_tex(args) -> tuple[int, int, float, list[str]]:
    def cleanup_temp_files(directory: str):
        extensions_to_clean = [".aux", ".log", ".out"]
        print(f"\n--- 在 '{directory}' 中清理 TeX 临时文件 ---")
        deleted_count = 0
        for root, _, files in os.walk(directory):
            for file in files:
                if any(file.endswith(ext) for ext in extensions_to_clean):
                    path = os.path.join(root, file)
                    try:
                        os.remove(path)
                        deleted_count += 1
                    except OSError as e:
                        print(f"❌ 错误：无法删除文件 '{path}': {e}")
        if deleted_count > 0:
            print(f"--- 清理完成，共删除 {deleted_count} 个文件 ---")

    success_count, failure_count, duration, updated_files = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension=".tex",
        log_file_type="TeX",
        command_builder=build_tex_command,
        max_workers=args.jobs,
        post_process_hook=cleanup_temp_files,
        incremental=args.incremental,
    )

    if (success_count + failure_count) > 0:
        print(f"===== TeX 处理完成 (成功: {success_count}, 失败: {failure_count}) =====")

    return success_count, failure_count, duration, updated_files


def handle_rst(args) -> tuple[int, int, float, list[str]]:
    print(f"将使用字体: '{args.font}'")
    builder = PandocCommandBuilder(source_format="rst", font=args.font)
    success_count, failure_count, duration, updated_files = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension=".rst",
        log_file_type="RST",
        command_builder=builder,
        max_workers=args.jobs,
        incremental=args.incremental,
    )
    if (success_count + failure_count) > 0:
        print(f"===== RST 处理完成 (成功: {success_count}, 失败: {failure_count}) =====")
    return success_count, failure_count, duration, updated_files


def handle_typ(args) -> tuple[int, int, float, list[str]]:
    success_count, failure_count, duration, updated_files = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension=".typ",
        log_file_type="Typst",
        command_builder=build_typ_command,
        max_workers=args.jobs,
        incremental=args.incremental,
    )
    if (success_count + failure_count) > 0:
        print(f"===== Typst 处理完成 (成功: {success_count}, 失败: {failure_count}) =====")
    return success_count, failure_count, duration, updated_files


def handle_md(args: argparse.Namespace) -> tuple[int, int, float, list[str]]:
    compilers = getattr(args, "markdown_compilers", ["pandoc"])

    if len(compilers) > 1 and "markdown" in [t.lower() for t in args.compile_types]:
        return _run_benchmark(args)

    compiler = compilers[0] if compilers else "pandoc"
    success_count = 0
    failure_count = 0
    duration = 0.0
    updated_files: list[str] = []
    print(f"===== 开始处理 Markdown (使用 {compiler} 方式) =====")
    print(f"将使用字体: '{args.font}'")

    if compiler == "typst":
        results, duration, updated_files = process_directory_md_via_typ(
            source_dir=args.source_dir,
            base_output_dir=args.output_dir,
            font=args.font,
            max_workers=args.jobs,
            incremental=args.incremental,
        )
        total_files = len(results)
        success_count = sum(1 for r in results if r.get("success") and not r.get("skipped"))
        skipped_count = sum(1 for r in results if r.get("skipped"))
        failure_count = total_files - success_count - skipped_count

        if total_files > 0:
            print("\n--- Markdown (Typst 路径) 详细统计 ---")
            print(f"成功: {success_count}, 失败: {failure_count}, 跳过: {skipped_count}")

    else:
        builder = PandocCommandBuilder(source_format="gfm", font=args.font)
        success_count, failure_count, duration, updated_files = process_directory(
            source_dir=args.source_dir,
            base_output_dir=args.output_dir,
            file_extension=".md",
            log_file_type="Markdown",
            command_builder=builder,
            max_workers=args.jobs,
            incremental=args.incremental,
        )

    if (success_count + failure_count) > 0:
        print(
            f"===== Markdown ({compiler}) 处理完成 (成功: {success_count}, 失败: {failure_count}) ====="
        )
    return success_count, failure_count, duration, updated_files
