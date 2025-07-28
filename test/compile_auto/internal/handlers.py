# internal/handlers.py
import os
import time
import argparse
from typing import Callable, Tuple

# 使用相对导入，从同一个包内的其他模块导入
from .core import process_directory
from .compilers import build_tex_command, build_typ_command, PandocCommandBuilder

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
        command_builder=build_tex_command,
        max_workers=args.jobs,
        post_process_hook=cleanup_temp_files
    )
    
    end_time = time.perf_counter()
    print(f"===== TeX 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_md(args):
    start_time = time.perf_counter()
    print(f"将使用字体: '{args.font}'")
    
    command_builder_instance = PandocCommandBuilder(source_format='gfm', font=args.font)

    file_count = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension='.md',
        log_file_type='Markdown',
        command_builder=command_builder_instance,
        max_workers=args.jobs
    )
    
    end_time = time.perf_counter()
    print(f"===== Markdown 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

def handle_rst(args):
    start_time = time.perf_counter()
    print(f"将使用字体: '{args.font}'")

    command_builder_instance = PandocCommandBuilder(source_format='rst', font=args.font)

    file_count = process_directory(
        source_dir=args.source_dir,
        base_output_dir=args.output_dir,
        file_extension='.rst',
        log_file_type='RST',
        command_builder=command_builder_instance,
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
        command_builder=build_typ_command,
        max_workers=args.jobs
    )
    
    end_time = time.perf_counter()
    print(f"===== Typst 文件处理完成 (共 {file_count} 个文件，总耗时: {end_time - start_time:.2f}秒) =====")
    return file_count

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