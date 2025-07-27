# main.py (在所有任务完成后统一打印报告)

import os
import sys
import time
import shutil
from typing import Tuple, List, Dict

# --- 从 compile_module 包中导入所有需要的编译器 ---
from compile_module.base_compiler import BaseCompiler
from compile_module.latex_compiler import LaTeXCompiler
from compile_module.typst_compiler import TypstCompiler
from compile_module.md_compiler import MdCompiler
from compile_module.rst_compiler import RstCompiler # <-- 1. 导入新的 RstCompiler

from config import COMPILE_TYPES, COMPILER_CONFIGS, SOURCE_ROOT_DIR, PDF_OUTPUT_ROOT_DIR
from utils import check_compiler_availability, find_source_files


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
    # 为了报告对齐，这里稍微调整一下输出格式
    if file_count > 0:
        average_time = total_time / file_count
        
        print(f"  - 类型 '{c_type.upper()}':")
        print(f"    - 文件总数: {file_count} 个")
        print(f"    - 总计耗时: {total_time:.2f} 秒")
        print(f"    - 平均耗时: {average_time:.2f} 秒/文件")
    else:
        # 这个分支在当前逻辑下不太可能被触发，但保留是好习惯
        print(f"  - 类型 '{c_type.upper()}': 无文件编译。")


def main():
    """主函数，编排整个编译流程。"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    compiler_map = {
        'tex': {'class': LaTeXCompiler, 'check_cmd': ['xelatex', '--version']},
        'typ': {'class': TypstCompiler, 'check_cmd': ['typst', '--version']},
        'md': {'class': MdCompiler, 'check_cmd': ['pandoc', '--version']},
        'rst': {'class': RstCompiler, 'check_cmd': ['pandoc', '--version']} # <-- 添加 rst 编译器
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

    # --- 1. 创建一个字典来存储所有类型的统计结果 ---
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
        
        # --- 2. 将结果存入字典，而不是立即打印 ---
        if file_count > 0:
            all_stats[c_type] = (total_time, file_count)

    # --- 3. 所有编译循环结束后，打印统一的总结报告 ---
    print("\n" + "="*40)
    print("--- 综合编译统计 ---")
    
    if not all_stats:
        print("  未完成任何文件的编译。")
    else:
        for c_type, (total_time, file_count) in all_stats.items():
            print_summary_report(c_type, total_time, file_count)
            
    print("="*40)

    print("\n--- 所有编译任务已完成 ---")


if __name__ == "__main__":
    main()