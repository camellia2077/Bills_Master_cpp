# main.py (修改后)

import os
import sys  # 1. 引入 sys 模块以使用 exit()
from config import COMPILE_TYPES, COMPILER_CONFIGS, SOURCE_ROOT_DIR, PDF_OUTPUT_ROOT_DIR
from utils import check_compiler_availability, find_source_files
from compilers import LaTeXCompiler, TypstCompiler

def main():
    """主函数，编排整个编译流程。"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    compiler_map = {
        'tex': {'class': LaTeXCompiler, 'check_cmd': ['xelatex', '--version']},
        'typ': {'class': TypstCompiler, 'check_cmd': ['typst', '--version']}
    }

    print("--- 开始批量编译任务 ---")

    for c_type in COMPILE_TYPES:
        if c_type not in compiler_map:
            print(f"\n-!-> 跳过不支持的类型: '{c_type}'")
            continue

        print(f"\n--- 处理类型: {c_type.upper()} ---")

        if not check_compiler_availability(compiler_map[c_type]['check_cmd']):
            continue

        # 2. 检查源目录并查找文件
        type_config = COMPILER_CONFIGS[c_type]
        source_dir = os.path.join(script_dir, SOURCE_ROOT_DIR, type_config['source_subfolder'])
        
        # --- 这是核心修改 ---
        # 在查找文件之前，先检查源目录是否存在。如果不存在，则这是一个致命错误。
        if not os.path.isdir(source_dir):
            print(f"❌ 致命错误：为类型 '{c_type}' 指定的源目录未找到。")
            print(f"  预期路径: {source_dir}")
            print("  请确保 C++ 导出程序已成功运行，或检查 config.py 中的路径配置。")
            print("  程序已终止。")
            sys.exit(1)  # 退出程序，返回一个错误码

        # 目录确认存在后，再查找文件
        source_files = find_source_files(source_dir, type_config['extension'])

        if not source_files:
            # 如果目录存在但没有找到文件，这不是一个错误，只是一个信息。
            print(f"  信息：在目录 '{source_dir}' 中未找到任何 '{type_config['extension']}' 文件。")
            continue
        
        print(f"  找到 {len(source_files)} 个文件进行编译。")

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