# main.py

import os
import sys
from config import COMPILE_TYPES, COMPILER_CONFIGS, SOURCE_ROOT_DIR, PDF_OUTPUT_ROOT_DIR
from utils import check_compiler_availability, find_source_files

# --- 核心修改：从 compile_module 包中导入所有需要的编译器 ---
from compile_module.latex_compiler import LaTeXCompiler
from compile_module.typst_compiler import TypstCompiler
from compile_module.md_compiler import MdCompiler  # <-- 1. 导入新的 MdCompiler

def main():
    """主函数，编排整个编译流程。"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # --- 核心修改：在映射中注册 md 类型 ---
    compiler_map = {
        'tex': {
            'class': LaTeXCompiler, 
            'check_cmd': ['xelatex', '--version']
        },
        'typ': {
            'class': TypstCompiler, 
            'check_cmd': ['typst', '--version']
        },
        'md': {
            'class': MdCompiler, 
            'check_cmd': ['pandoc', '--version']
        }  # <-- 2. 在映射中注册 md 类型和其编译器
    }

    print("--- 开始批量编译任务 ---")

    for c_type in COMPILE_TYPES:
        if c_type not in compiler_map:
            print(f"\n-!-> 跳过不支持的类型: '{c_type}'")
            continue

        print(f"\n--- 处理类型: {c_type.upper()} ---")

        if not check_compiler_availability(compiler_map[c_type]['check_cmd']):
            continue

        type_config = COMPILER_CONFIGS[c_type]
        # 在这里，我们假设 SOURCE_ROOT_DIR 是一个绝对路径
        source_dir = os.path.join(SOURCE_ROOT_DIR, type_config['source_subfolder'])
        
        if not os.path.isdir(source_dir):
            print(f"❌ 致命错误：为类型 '{c_type}' 指定的源目录未找到。")
            print(f"  预期路径: {source_dir}")
            print("  请确保 C++ 导出程序已成功运行，或检查 config.py 中的路径配置。")
            print("  程序已终止。")
            sys.exit(1)

        source_files = find_source_files(source_dir, type_config['extension'])

        if not source_files:
            print(f"  信息：在目录 '{source_dir}' 中未找到任何 '{type_config['extension']}' 文件。")
            continue
        
        print(f"  找到 {len(source_files)} 个文件进行编译。")

        # PDF 输出目录基于脚本所在位置
        output_dir = os.path.join(script_dir, PDF_OUTPUT_ROOT_DIR, type_config['output_subfolder'])
        os.makedirs(output_dir, exist_ok=True)
        
        compiler = compiler_map[c_type]['class']()

        for file_path in source_files:
            relative_path = os.path.relpath(os.path.dirname(file_path), source_dir)
            final_output_dir = os.path.join(output_dir, relative_path)
            os.makedirs(final_output_dir, exist_ok=True)
            
            compiler.compile(file_path, final_output_dir)

    print("\n--- 所有编译任务已完成 ---")

if __name__ == "__main__":
    main()