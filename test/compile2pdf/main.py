# main.py

import os
import sys
import time
import shutil
import argparse  # <-- 1. 导入 argparse 模块
from typing import Tuple, List, Dict

# --- 从 compile_module 包中导入所有需要的编译器 ---
from compile_module.base_compiler import BaseCompiler
from compile_module.latex_compiler import LaTeXCompiler
from compile_module.typst_compiler import TypstCompiler
from compile_module.md_compiler import MdCompiler
from compile_module.rst_compiler import RstCompiler

from config import COMPILE_TYPES, COMPILER_CONFIGS, SOURCE_ROOT_DIR, PDF_OUTPUT_ROOT_DIR
from utils import check_compiler_availability, find_source_files

# --- run_compilation_for_type 和 print_summary_report 函数保持不变 ---

def run_compilation_for_type(
    compiler: BaseCompiler, 
    source_files: List[str], 
    source_dir: str, 
    output_dir: str
) -> Tuple[float, int]:
    """为指定类型的文件执行编译和计时。"""
    durations = []
    
    print("  进度: ", end="", flush=True)

    for file_path in source_files:
        relative_path = os.path.relpath(os.path.dirname(file_path), source_dir)
        final_output_dir = os.path.join(output_dir, relative_path)
        os.makedirs(final_output_dir, exist_ok=True)
        
        start_time = time.monotonic()
        compiler.compile(file_path, final_output_dir)
        end_time = time.monotonic()
        
        durations.append(end_time - start_time)
        print(".", end="", flush=True)

    print("\n")

    total_time = sum(durations)
    file_count = len(durations)
    
    return total_time, file_count

def print_summary_report(c_type: str, total_time: float, file_count: int) -> None:
    """打印单个编译任务的统计总结报告。"""
    if file_count > 0:
        average_time = total_time / file_count
        
        print(f"  - 类型 '{c_type.upper()}':")
        print(f"    - 文件总数: {file_count} 个")
        print(f"    - 总计耗时: {total_time:.2f} 秒")
        print(f"    - 平均耗时: {average_time:.2f} 秒/文件")
    else:
        print(f"  - 类型 '{c_type.upper()}': 无文件编译。")

# ======================================================================
# ==                      核心修改点 START                            ==
# ======================================================================

def handle_single_file_compilation(args):
    """处理单个文件的编译任务。"""
    print(f"\n--- 开始单文件编译任务 ---")
    
    file_path = args.file
    c_type = args.type

    if not os.path.exists(file_path):
        print(f"❌ 致命错误：指定的文件不存在: '{file_path}'")
        sys.exit(1)

    compiler_map = {
        'tex': {'class': LaTeXCompiler, 'check_cmd': ['xelatex', '--version']},
        'typ': {'class': TypstCompiler, 'check_cmd': ['typst', '--version']},
        'md': {'class': MdCompiler, 'check_cmd': ['pandoc', '--version']},
        'rst': {'class': RstCompiler, 'check_cmd': ['pandoc', '--version']}
    }

    if c_type not in compiler_map:
        print(f"❌ 致命错误：不支持的编译类型 '{c_type}'。支持的类型: {list(compiler_map.keys())}")
        sys.exit(1)
        
    print(f"  文件: {file_path}")
    print(f"  类型: {c_type.upper()}")

    if not check_compiler_availability(compiler_map[c_type]['check_cmd']):
        sys.exit(1)

    # 对于单文件，我们将PDF输出到脚本所在的 'single_file_output' 目录中
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, "single_file_output")
    os.makedirs(output_dir, exist_ok=True)
    
    compiler = compiler_map[c_type]['class']()
    
    print("\n--- 编译信息 ---")
    compiler.compile(file_path, output_dir)
    print("\n--- 任务完成 ---")


def handle_batch_compilation():
    """处理原有的批量编译任务。"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    compiler_map = {
        'tex': {'class': LaTeXCompiler, 'check_cmd': ['xelatex', '--version']},
        'typ': {'class': TypstCompiler, 'check_cmd': ['typst', '--version']},
        'md': {'class': MdCompiler, 'check_cmd': ['pandoc', '--version']},
        'rst': {'class': RstCompiler, 'check_cmd': ['pandoc', '--version']}
    }
    
    output_root_to_delete = os.path.join(script_dir, PDF_OUTPUT_ROOT_DIR)
    
    if os.path.isdir(output_root_to_delete):
        print(f"--- 检测到旧的输出目录，正在清理: '{output_root_to_delete}' ---")
        try:
            shutil.rmtree(output_root_to_delete)
            print("    ✅ 清理完成。")
        except OSError as e:
            print(f"❌ 致命错误：无法删除目录 '{output_root_to_delete}'。 原因: {e}")
            sys.exit(1)

    print("\n--- 开始批量编译任务 ---")

    all_stats: Dict[str, Tuple[float, int]] = {}

    for c_type in COMPILE_TYPES:
        if c_type not in compiler_map:
            print(f"\n-!-> 跳过不支持的类型: '{c_type}'")
            continue

        print(f"\n--- 处理类型: {c_type.upper()} ---")

        if not check_compiler_availability(compiler_map[c_type]['check_cmd']):
            continue

        type_config = COMPILER_CONFIGS[c_type]
        source_dir = os.path.join(SOURCE_ROOT_DIR, type_config['source_subfolder'])
        
        if not os.path.isdir(source_dir):
            print(f"❌ 致命错误：为类型 '{c_type}' 指定的源目录未找到。")
            print(f"  预期路径: {source_dir}")
            sys.exit(1)

        source_files = find_source_files(source_dir, type_config['extension'])

        if not source_files:
            print(f"  信息：在目录 '{source_dir}' 中未找到任何 '{type_config['extension']}' 文件。")
            continue
        
        print(f"  找到 {len(source_files)} 个文件，开始编译...")

        output_dir = os.path.join(script_dir, PDF_OUTPUT_ROOT_DIR, type_config['output_subfolder'])
        os.makedirs(output_dir, exist_ok=True)
        
        compiler = compiler_map[c_type]['class']()
        
        total_time, file_count = run_compilation_for_type(
            compiler, source_files, source_dir, output_dir
        )
        
        if file_count > 0:
            all_stats[c_type] = (total_time, file_count)

    print("\n" + "="*40)
    print("--- 综合编译统计 ---")
    
    if not all_stats:
        print("  未完成任何文件的编译。")
    else:
        for c_type, (total_time, file_count) in all_stats.items():
            print_summary_report(c_type, total_time, file_count)
            
    print("="*40)
    print("\n--- 所有批量编译任务已完成 ---")

def main():
    """主函数，根据命令行参数选择执行模式。"""
    parser = argparse.ArgumentParser(
        description="一个用于批量或单独编译 tex, typ, md, rst 文件的工具。"
    )
    parser.add_argument(
        '-f', '--file',
        type=str,
        help="指定要单独编译的源文件的路径。"
    )
    parser.add_argument(
        '-t', '--type',
        type=str,
        choices=['tex', 'typ', 'md', 'rst'],
        help="当使用 --file 时，必须指定源文件的类型。"
    )
    
    args = parser.parse_args()

    # 根据参数决定执行哪个流程
    if args.file and args.type:
        handle_single_file_compilation(args)
    elif args.file and not args.type:
        parser.error("--file 参数需要与 --type 参数一起使用。")
    elif not args.file and args.type:
        parser.error("--type 参数需要与 --file 参数一起使用。")
    else:
        # 如果没有提供任何参数，则执行默认的批量编译
        handle_batch_compilation()

if __name__ == "__main__":
    main()

# ======================================================================
# ==                       核心修改点 END                             ==
# ======================================================================