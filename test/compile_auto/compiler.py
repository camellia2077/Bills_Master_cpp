import os
import subprocess
import sys
import argparse
import time
import shutil
from typing import Callable, List, Optional, Tuple
import concurrent.futures

# ==============================================================================
# 1. 公共逻辑模块 (The Engine)
# ==============================================================================

def compile_single_file(input_path: str, final_pdf_path: str, target_output_dir: str, command_builder: Callable, log_file_type: str) -> dict:
    """
    编译单个文件并返回结果。此函数将在子进程中运行。
    """
    file_name = os.path.basename(input_path)
    command = command_builder(input_path, final_pdf_path, target_output_dir)
    
    try:
        os.makedirs(target_output_dir, exist_ok=True)
    except OSError as e:
        return {
            "success": False,
            "file": file_name,
            "duration": 0,
            "log": f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}"
        }

    try:
        file_start_time = time.perf_counter()
        result = subprocess.run(
            command, capture_output=True, text=True, encoding='utf-8'
        )
        file_duration = time.perf_counter() - file_start_time

        if result.returncode == 0:
            return {
                "success": True,
                "file": file_name,
                "duration": file_duration,
                "log": f"✅ 成功: '{file_name}' 已成功编译。 (耗时: {file_duration:.2f}秒)"
            }
        else:
            error_log = (
                f"❌ 失败: '{file_name}' 编译失败。返回码: {result.returncode} (耗时: {file_duration:.2f}秒)\n"
                f"--- {log_file_type} 编译器错误日志 ---\n"
                f"{result.stdout or '没有标准输出。'}\n"
                f"{result.stderr or '没有标准错误输出。'}\n"
                "-----------------------------"
            )
            return {"success": False, "file": file_name, "duration": file_duration, "log": error_log}

    except FileNotFoundError:
        return {"success": False, "file": file_name, "duration": 0, "log": f"错误：命令 '{command[0]}' 未找到。"}
    except Exception as e:
        return {"success": False, "file": file_name, "duration": 0, "log": f"处理文件 '{file_name}' 时发生未知错误: {e}"}


def process_directory(
    source_dir: str,
    base_output_dir: str,
    file_extension: str,
    log_file_type: str,
    command_builder: Callable[[str, str, str], List[str]],
    max_workers: Optional[int] = None,
    post_process_hook: Optional[Callable[[str], None]] = None
) -> int:
    """通用的文件编译处理函数，使用并行处理并返回成功处理的文件数量。"""
    source_dir = os.path.abspath(source_dir)
    source_folder_name = os.path.basename(source_dir)
    type_specific_output_root = os.path.join(base_output_dir, source_folder_name)

    worker_count = max_workers or os.cpu_count()
    print(f"\n===== 开始处理 {log_file_type} 文件 (使用最多 {worker_count} 个并行任务) =====")
    print(f"源目录: '{source_dir}'")
    print(f"输出到: '{type_specific_output_root}'")

    tasks = []
    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.endswith(file_extension):
                input_path = os.path.join(root, file)
                relative_path = os.path.relpath(root, source_dir)
                target_output_dir = os.path.join(type_specific_output_root, relative_path)
                pdf_filename = os.path.splitext(file)[0] + '.pdf'
                final_pdf_path = os.path.join(target_output_dir, pdf_filename)
                tasks.append((input_path, final_pdf_path, target_output_dir))

    if not tasks:
        print(f"\n在目录 '{source_dir}' 中没有找到任何 {file_extension} 文件。")
        return 0

    success_count = 0
    with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
        future_to_file = {
            executor.submit(compile_single_file, input_p, output_p, target_d, command_builder, log_file_type): os.path.basename(input_p)
            for input_p, output_p, target_d in tasks
        }

        for i, future in enumerate(concurrent.futures.as_completed(future_to_file), 1):
            file_name = future_to_file[future]
            print(f"\n--- [{i}/{len(tasks)}] 文件 '{file_name}' 处理完成 ---")
            try:
                result = future.result()
                print(result["log"])
                if result["success"]:
                    success_count += 1
            except Exception as e:
                print(f"处理 '{file_name}' 时发生严重错误: {e}")

    if post_process_hook:
        post_process_hook(type_specific_output_root)

    return success_count

# ==============================================================================
# 2. 特定编译器的处理模块 (The Configurations)
# ==============================================================================

# --- 将 command_builder 移至顶层 ---

def build_tex_command(input_path, _, target_dir):
    """构建 TeX 编译命令。"""
    return ['xelatex', '-interaction=nonstopmode', f'-output-directory={target_dir}', input_path]

def build_typ_command(input_path, output_path, _):
    """构建 Typst 编译命令。"""
    return ['typst', 'compile', input_path, output_path]

class PandocCommandBuilder:
    """为需要额外配置（如字体）的 Pandoc 命令构建一个可序列化的类。"""
    def __init__(self, source_format: str, font: str):
        self.source_format = source_format
        self.font = font

    def __call__(self, input_path: str, output_path: str, _) -> List[str]:
        return [
            'pandoc',
            f'--from={self.source_format}',
            input_path,
            '-o',
            output_path,
            '--pdf-engine=xelatex',
            '-V', f'mainfont={self.font}',
            '-V', 'lang=zh-CN',
            '-V', 'geometry:margin=1in'
        ]

# --- 修改后的 handle_* 函数 ---

def handle_tex(args):
    start_time = time.perf_counter()
    def cleanup_temp_files(directory: str):
        extensions = ['.aux', '.log', '.out']
        print(f"\n--- 在 '{directory}' 中清理临时文件 ---")
        deleted_count = 0
        for root, _, files in os.walk(directory):
            for file in files:
                if any(file.endswith(ext) for ext in extensions):
                    path = os.path.join(root, file)
                    try:
                        os.remove(path)
                        deleted_count += 1
                        print(f"🗑️ 已删除: {path}")
                    except OSError as e:
                        print(f"❌ 错误：无法删除文件 '{path}': {e}")
        if deleted_count == 0: print("没有找到需要清理的临时文件。")
        print("--- 清理完成 ---")

    file_count = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension='.tex',
        log_file_type='TeX',
        command_builder=build_tex_command,  # 直接传递顶层函数
        max_workers=args.jobs,
        post_process_hook=cleanup_temp_files
    )
    
    end_time = time.perf_counter()
    print(f"===== TeX 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_md(args):
    start_time = time.perf_counter()
    print(f"将使用字体: '{args.font}'")
    
    # 创建一个可序列化的类的实例
    command_builder_instance = PandocCommandBuilder(source_format='gfm', font=args.font)

    file_count = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension='.md',
        log_file_type='Markdown',
        command_builder=command_builder_instance, # 传递类的实例
        max_workers=args.jobs
    )
    
    end_time = time.perf_counter()
    print(f"===== Markdown 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_rst(args):
    start_time = time.perf_counter()
    print(f"将使用字体: '{args.font}'")

    # 创建一个可序列化的类的实例
    command_builder_instance = PandocCommandBuilder(source_format='rst', font=args.font)

    file_count = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension='.rst',
        log_file_type='RST',
        command_builder=command_builder_instance, # 传递类的实例
        max_workers=args.jobs
    )
    
    end_time = time.perf_counter()
    print(f"===== RST 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_typ(args):
    start_time = time.perf_counter()
    file_count = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension='.typ',
        log_file_type='Typst',
        command_builder=build_typ_command, # 直接传递顶层函数
        max_workers=args.jobs
    )
    
    end_time = time.perf_counter()
    print(f"===== Typst 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

# ==============================================================================
# 2.5. 自动模式调度模块 (The Auto-Dispatcher)
# ==============================================================================
def handle_auto(args):
    parent_dir = args.source_dir
    print(f"===== 启动自动编译模式 =====")
    print(f"扫描父目录: '{parent_dir}'")
    
    timing_summary = {}
    
    compiler_map: dict[Tuple[str, ...], Tuple[str, Callable]] = {
        ('latex', 'tex'): ('TeX', handle_tex),
        ('markdown', 'md'): ('Markdown', handle_md),
        ('rst', 'rest'): ('RST', handle_rst),
        ('typst', 'typ'): ('Typst', handle_typ)
    }

    found_dirs_count = 0
    for subdir_name in os.listdir(parent_dir):
        full_subdir_path = os.path.join(parent_dir, subdir_name)
        if os.path.isdir(full_subdir_path):
            sub_dir_lower = subdir_name.lower()
            matched = False
            for keywords, (log_name, handler_func) in compiler_map.items():
                for keyword in keywords:
                    if keyword in sub_dir_lower:
                        found_dirs_count += 1
                        print(f"\n\n>>> 自动检测到 '{subdir_name}' -> 使用 {log_name} 编译器...")
                        mock_args = argparse.Namespace(
                            source_dir=full_subdir_path,
                            font=args.font,
                            output_dir=args.output_dir,
                            jobs=args.jobs
                        )
                        
                        format_start_time = time.perf_counter()
                        file_count = handler_func(mock_args)
                        format_end_time = time.perf_counter()
                        
                        timing_summary[log_name] = (format_end_time - format_start_time, file_count)
                        
                        matched = True
                        break
                if matched:
                    break
    
    if found_dirs_count == 0:
        print(f"\n在 '{parent_dir}' 中没有找到任何可识别的编译子目录。")
        print("请确保子目录名称包含关键字: latex, tex, markdown, md, rst, rest, typst, typ")
    else:
        print("\n\n" + "="*35)
        print("     自动模式编译时间摘要")
        print("="*35)
        for format_name, (duration, count) in timing_summary.items():
            avg_time_str = f"平均: {(duration / count):.2f} 秒/文件" if count > 0 else "无文件编译"
            print(f"- {format_name:<10} | 总耗时: {duration:>7.2f} 秒 | {avg_time_str}")
        print("="*35)

# ==============================================================================
# 3. 主程序入口和命令行解析 (The Dispatcher)
# ==============================================================================
def main():
    program_start_time = time.perf_counter()
    parser = argparse.ArgumentParser(
        description="一个通用的、支持并行的文档编译器。",
        epilog="示例: python compiler.py auto ./my_docs -j 8"
    )
    
    parser.add_argument('--output-dir', type=str, default='output_pdf', help="顶级输出目录 (默认: 'output_pdf')")
    parser.add_argument('--no-clean', action='store_true', help='【可选】启动时不清理旧的输出目录。')
    parser.add_argument(
        '--jobs', '-j',
        type=int,
        default=None,
        help="【可选】并行编译的任务数量 (默认: 使用所有可用的CPU核心)"
    )

    subparsers = parser.add_subparsers(dest='command', required=True, help='编译命令')
    
    parser_auto = subparsers.add_parser('auto', help='自动检测源目录下的子目录并调用相应编译器')
    parser_auto.add_argument('source_dir', type=str, help='包含待编译子目录的父文件夹路径 (例如 ./my_docs)')
    parser_auto.add_argument('--font', type=str, default="Noto Serif SC", help="为 Pandoc 指定 CJK 字体 (默认: Noto Serif SC)")
    parser_auto.set_defaults(func=handle_auto)

    parser_tex = subparsers.add_parser('tex', help='编译指定目录下的所有 .tex 文件')
    parser_tex.add_argument('source_dir', type=str, help='包含 .tex 文件的源文件夹路径')
    parser_tex.set_defaults(func=handle_tex)
    
    parser_md = subparsers.add_parser('md', help='编译指定目录下的所有 .md 文件')
    parser_md.add_argument('source_dir', type=str, help='包含 .md 文件的源文件夹路径')
    parser_md.add_argument('--font', type=str, default="Noto Serif SC", help="指定 CJK 字体 (默认: Noto Serif SC)")
    parser_md.set_defaults(func=handle_md)

    parser_rst = subparsers.add_parser('rst', help='编译指定目录下的所有 .rst 文件')
    parser_rst.add_argument('source_dir', type=str, help='包含 .rst 文件的源文件夹路径')
    parser_rst.add_argument('--font', type=str, default="Noto Serif SC", help="指定 CJK 字体 (默认: Noto Serif SC)")
    parser_rst.set_defaults(func=handle_rst)
    
    parser_typ = subparsers.add_parser('typ', help='编译指定目录下的所有 .typ 文件')
    parser_typ.add_argument('source_dir', type=str, help='包含 .typ 文件的源文件夹路径')
    parser_typ.set_defaults(func=handle_typ)

    args = parser.parse_args()
    
    output_dir_to_process = os.path.join(os.getcwd(), args.output_dir)
    if not args.no_clean:
        if os.path.exists(output_dir_to_process):
            print(f"🧹 默认执行清理，正在删除旧的输出目录: '{output_dir_to_process}'")
            try:
                shutil.rmtree(output_dir_to_process)
                print("✅ 旧目录已成功删除。")
            except OSError as e:
                print(f"致命错误：无法删除输出目录 '{output_dir_to_process}': {e}")
                sys.exit(1)
        else:
            print(f"🧹 输出目录 '{output_dir_to_process}' 不存在，无需清理。")
    else:
        print("🚫 用户选择跳过清理步骤。")

    try:
        os.makedirs(output_dir_to_process, exist_ok=True)
    except OSError as e:
        print(f"致命错误：无法创建顶级输出目录 '{output_dir_to_process}': {e}")
        sys.exit(1)

    if not os.path.isdir(args.source_dir):
        print(f"错误：提供的路径 '{args.source_dir}' 不是一个有效的目录。")
        sys.exit(1)
        
    args.func(args)
    
    program_end_time = time.perf_counter()
    print(f"\n\n🚀 程序总运行时间: {program_end_time - program_start_time:.2f} 秒")

if __name__ == '__main__':
    main()