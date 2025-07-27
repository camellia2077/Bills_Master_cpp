import os
import subprocess
import sys
import argparse
import time
import shutil
from typing import Callable, List, Optional, Tuple

# ==============================================================================
# 1. 公共逻辑模块 (The Engine)
# ==============================================================================

def process_directory(
    source_dir: str,
    base_output_dir: str,
    file_extension: str,
    log_file_type: str,
    command_builder: Callable[[str, str, str], List[str]],
    post_process_hook: Optional[Callable[[str], None]] = None
) -> int: # <--- 1. 修改函数签名，明确返回一个整数
    """通用的文件编译处理函数，返回成功处理的文件数量。"""
    source_dir = os.path.abspath(source_dir)
    found_file_count = 0
    source_folder_name = os.path.basename(source_dir)
    type_specific_output_root = os.path.join(base_output_dir, source_folder_name)

    print(f"\n===== 开始处理 {log_file_type} 文件 =====")
    print(f"源目录: '{source_dir}'")
    print(f"输出到: '{type_specific_output_root}'")

    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.endswith(file_extension):
                found_file_count += 1
                input_path = os.path.join(root, file)
                print(f"\n--- 发现 {log_file_type} 文件: {input_path} ---")

                relative_path = os.path.relpath(root, source_dir)
                target_output_dir = os.path.join(type_specific_output_root, relative_path)
                pdf_filename = os.path.splitext(file)[0] + '.pdf'
                final_pdf_path = os.path.join(target_output_dir, pdf_filename)
                
                try:
                    os.makedirs(target_output_dir, exist_ok=True)
                except OSError as e:
                    print(f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}")
                    continue

                command = command_builder(input_path, final_pdf_path, target_output_dir)

                try:
                    print(f"正在编译 '{file}' -> '{final_pdf_path}'")
                    file_start_time = time.perf_counter()
                    result = subprocess.run(
                        command, capture_output=True, text=True, encoding='utf-8'
                    )
                    file_end_time = time.perf_counter()
                    file_duration = file_end_time - file_start_time

                    if result.returncode == 0:
                        print(f"✅ 成功: '{file}' 已成功编译。 (耗时: {file_duration:.2f}秒)")
                    else:
                        print(f"❌ 失败: '{file}' 编译失败。返回码: {result.returncode} (耗时: {file_duration:.2f}秒)")
                        print(f"--- {log_file_type} 编译器错误日志 ---")
                        print(result.stdout or "没有标准输出。")
                        print(result.stderr or "没有标准错误输出。")
                        print("-----------------------------")

                except FileNotFoundError:
                    print(f"错误：命令 '{command[0]}' 未找到。请确保它已安装并位于系统 PATH 中。")
                    return 0 # 出错时返回0
                except Exception as e:
                    print(f"处理文件 '{file}' 时发生未知错误: {e}")

    if found_file_count == 0:
        print(f"\n在目录 '{source_dir}' 中没有找到任何 {file_extension} 文件。")
    
    if post_process_hook:
        post_process_hook(type_specific_output_root)

    return found_file_count # <--- 2. 返回找到并处理的文件数

# ==============================================================================
# 2. 特定编译器的处理模块 (The Configurations)
# ==============================================================================

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

    def command_builder(input_path, _, target_dir):
        return ['xelatex', '-interaction=nonstopmode', f'-output-directory={target_dir}', input_path]
    
    file_count = process_directory(source_dir=args.source_dir, base_output_dir=args.output_dir, file_extension='.tex', log_file_type='TeX', command_builder=command_builder, post_process_hook=cleanup_temp_files)
    
    end_time = time.perf_counter()
    print(f"===== TeX 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count # <--- 返回文件数

def handle_md(args):
    start_time = time.perf_counter()
    print(f"将使用字体: '{args.font}'")
    def command_builder(input_path, output_path, _):
        return ['pandoc', '--from=gfm', input_path, '-o', output_path, '--pdf-engine=xelatex', f'-V', f'mainfont={args.font}', '-V', 'lang=zh-CN', '-V', 'geometry:margin=1in']
    
    file_count = process_directory(source_dir=args.source_dir, base_output_dir=args.output_dir, file_extension='.md', log_file_type='Markdown', command_builder=command_builder)
    
    end_time = time.perf_counter()
    print(f"===== Markdown 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_rst(args):
    start_time = time.perf_counter()
    print(f"将使用字体: '{args.font}'")
    def command_builder(input_path, output_path, _):
        return ['pandoc', '--from=rst', input_path, '-o', output_path, '--pdf-engine=xelatex', f'-V', f'mainfont={args.font}', '-V', 'lang=zh-CN', '-V', 'geometry:margin=1in']
    
    file_count = process_directory(source_dir=args.source_dir, base_output_dir=args.output_dir, file_extension='.rst', log_file_type='RST', command_builder=command_builder)
    
    end_time = time.perf_counter()
    print(f"===== RST 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_typ(args):
    start_time = time.perf_counter()
    def command_builder(input_path, output_path, _):
        return ['typst', 'compile', input_path, output_path]
    
    file_count = process_directory(source_dir=args.source_dir, base_output_dir=args.output_dir, file_extension='.typ', log_file_type='Typst', command_builder=command_builder)
    
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
                        mock_args = argparse.Namespace(source_dir=full_subdir_path, font=args.font, output_dir=args.output_dir)
                        
                        format_start_time = time.perf_counter()
                        # <--- 3. 收集文件数 --->
                        file_count = handler_func(mock_args)
                        format_end_time = time.perf_counter()
                        
                        # 存储总耗时和文件数
                        timing_summary[log_name] = (format_end_time - format_start_time, file_count)
                        
                        matched = True
                        break
                if matched:
                    break
    
    if found_dirs_count == 0:
        print(f"\n在 '{parent_dir}' 中没有找到任何可识别的编译子目录。")
        print("请确保子目录名称包含关键字: latex, tex, markdown, md, rst, rest, typst, typ")
    else:
        # <--- 4. 在摘要中计算并打印平均耗时 --->
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
    parser = argparse.ArgumentParser(description="一个通用的文档编译器...", epilog="...")
    parser.add_argument('--output-dir', type=str, default='output_pdf', help="顶级输出目录 (默认: 'output_pdf')")
    parser.add_argument('--no-clean', action='store_true', help='【可选】不清理输出目录。')
    subparsers = parser.add_subparsers(dest='command', required=True, help='编译命令')
    
    parser_auto = subparsers.add_parser('auto', help='自动检测并编译')
    parser_auto.add_argument('source_dir', type=str, help='源文件夹路径')
    parser_auto.add_argument('--font', type=str, default="Noto Serif SC", help="指定字体")
    parser_auto.set_defaults(func=handle_auto)

    parser_tex = subparsers.add_parser('tex', help='编译 .tex 文件')
    parser_tex.add_argument('source_dir', type=str, help='源文件夹路径')
    parser_tex.set_defaults(func=handle_tex)
    
    parser_md = subparsers.add_parser('md', help='编译 .md 文件')
    parser_md.add_argument('source_dir', type=str, help='源文件夹路径')
    parser_md.add_argument('--font', type=str, default="Noto Serif SC", help="指定字体")
    parser_md.set_defaults(func=handle_md)

    parser_rst = subparsers.add_parser('rst', help='编译 .rst 文件')
    parser_rst.add_argument('source_dir', type=str, help='源文件夹路径')
    parser_rst.add_argument('--font', type=str, default="Noto Serif SC", help="指定字体")
    parser_rst.set_defaults(func=handle_rst)
    
    parser_typ = subparsers.add_parser('typ', help='编译 .typ 文件')
    parser_typ.add_argument('source_dir', type=str, help='源文件夹路径')
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