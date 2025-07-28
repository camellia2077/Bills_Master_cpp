import os
import subprocess
import sys
import argparse

def cleanup_temp_files(directory):
    """
    遍历指定目录及其子目录，删除所有LaTeX临时文件。

    :param directory: 需要清理的根目录。
    """
    # 定义要删除的临时文件的扩展名列表
    extensions_to_delete = ['.aux', '.log', '.out']
    print(f"\n--- 开始清理临时文件 ({', '.join(extensions_to_delete)}) ---")
    
    deleted_count = 0
    # 遍历目录
    for root, _, files in os.walk(directory):
        for file in files:
            # 检查文件扩展名是否在要删除的列表中
            if any(file.endswith(ext) for ext in extensions_to_delete):
                file_path = os.path.join(root, file)
                try:
                    os.remove(file_path)
                    print(f"🗑️ 已删除: {file_path}")
                    deleted_count += 1
                except OSError as e:
                    print(f"❌ 错误：无法删除文件 '{file_path}': {e}")
    
    if deleted_count == 0:
        print("没有找到需要清理的临时文件。")
    
    print("--- 清理完成 ---")


def compile_tex_files(source_dir):
    """
    遍历指定目录下的所有 .tex 文件，使用 xelatex 编译，
    保留原始文件夹层级结构，并在最后清理临时文件。

    :param source_dir: 包含 .tex 文件的源文件夹路径。
    """
    # 1. 定义基础输出文件夹路径（脚本同目录下的 'pdf' 文件夹）
    base_output_dir = os.path.join(os.getcwd(), 'pdf')

    # 将输入的源目录转换为绝对路径，以确保路径计算的准确性
    source_dir = os.path.abspath(source_dir)

    found_tex_file = False

    # 2. 遍历源文件夹及其所有子文件夹
    print(f"\n开始在 '{source_dir}' 中搜索 .tex 文件...")
    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.endswith('.tex'):
                found_tex_file = True
                tex_file_path = os.path.join(root, file)
                print(f"\n--- 发现 TeX 文件: {tex_file_path} ---")

                relative_path = os.path.relpath(root, source_dir)
                target_output_dir = os.path.join(base_output_dir, relative_path)
                
                try:
                    os.makedirs(target_output_dir, exist_ok=True)
                except OSError as e:
                    print(f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}")
                    continue

                command = [
                    'xelatex',
                    '-interaction=nonstopmode',
                    f'-output-directory={target_output_dir}',
                    tex_file_path
                ]

                try:
                    print(f"正在编译 '{file}' -> 输出到 '{target_output_dir}'")
                    result = subprocess.run(
                        command,
                        capture_output=True,
                        text=True,
                        encoding='utf-8'
                    )

                    pdf_filename = os.path.splitext(file)[0] + '.pdf'
                    final_pdf_path = os.path.join(target_output_dir, pdf_filename)
                    if result.returncode == 0 and os.path.exists(final_pdf_path):
                        print(f"✅ 成功: '{file}' 已成功编译为 '{final_pdf_path}'")
                    else:
                        print(f"❌ 失败: '{file}' 编译失败。返回码: {result.returncode}")
                        print("--- XeLaTeX 错误日志 ---")
                        print(result.stdout)
                        print(result.stderr or "没有标准错误输出。")
                        print("------------------------")

                except FileNotFoundError:
                    print("错误：'xelatex' 命令未找到。")
                    print("请确保您已安装 TeX 发行版 (如 TeX Live, MiKTeX) 并将其添加到了系统的 PATH 环境变量中。")
                    return
                except Exception as e:
                    print(f"处理文件 '{file}' 时发生未知错误: {e}")

    # --- 新增的清理步骤 ---
    # 如果找到了并尝试编译了至少一个 .tex 文件，则执行清理
    if found_tex_file:
        cleanup_temp_files(base_output_dir)
    else:
        print(f"在目录 '{source_dir}' 中没有找到任何 .tex 文件，无需清理。")


def main():
    """
    主函数，用于解析命令行参数。
    """
    parser = argparse.ArgumentParser(
        description="编译目录下的所有 .tex 文件并自动清理临时文件 (.aux, .log)。",
        epilog="示例: python compile_and_clean.py /path/to/your/tex/files"
    )
    parser.add_argument(
        "source_dir",
        type=str,
        help="包含 .tex 文件的源文件夹路径。"
    )
    args = parser.parse_args()

    if not os.path.isdir(args.source_dir):
        print(f"错误：提供的路径 '{args.source_dir}' 不是一个有效的目录。")
        sys.exit(1)

    compile_tex_files(args.source_dir)

if __name__ == '__main__':
    main()