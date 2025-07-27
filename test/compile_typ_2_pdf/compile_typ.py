import os
import subprocess
import sys
import argparse

def compile_typ_files(source_dir):
    """
    遍历指定目录下的所有 .typ 文件，使用 typst 编译，
    并在输出目录中保留原始的文件夹层级结构。

    :param source_dir: 包含 .typ 文件的源文件夹路径。
    """
    # 1. 定义基础输出文件夹路径
    base_output_dir = os.path.join(os.getcwd(), 'pdf_typst_bills')

    # 将输入的源目录转换为绝对路径，以确保路径计算的准确性
    source_dir = os.path.abspath(source_dir)

    found_typ_file = False

    # 2. 遍历源文件夹及其所有子文件夹
    print(f"\n开始在 '{source_dir}' 中搜索 .typ 文件...")
    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.endswith('.typ'):
                found_typ_file = True
                typ_file_path = os.path.join(root, file)
                print(f"\n--- 发现 Typst 文件: {typ_file_path} ---")

                # 3. 计算相对路径并创建目标子目录
                relative_path = os.path.relpath(root, source_dir)
                target_output_dir = os.path.join(base_output_dir, relative_path)
                
                try:
                    os.makedirs(target_output_dir, exist_ok=True)
                except OSError as e:
                    print(f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}")
                    continue

                # 4. 准备 Typst 编译命令
                pdf_filename = os.path.splitext(file)[0] + '.pdf'
                final_pdf_path = os.path.join(target_output_dir, pdf_filename)
                
                # Typst 的编译命令非常直接： typst compile <input> <output>
                command = [
                    'typst',
                    'compile',
                    typ_file_path,    # 输入文件
                    final_pdf_path    # 输出文件
                ]

                try:
                    print(f"正在编译 '{file}' -> '{final_pdf_path}'")
                    # 执行命令
                    result = subprocess.run(
                        command,
                        capture_output=True,
                        text=True,
                        encoding='utf-8'
                    )

                    # 5. 检查编译结果
                    if result.returncode == 0:
                        print(f"✅ 成功: '{file}' 已成功编译为 '{final_pdf_path}'")
                    else:
                        print(f"❌ 失败: '{file}' 编译失败。返回码: {result.returncode}")
                        print("--- Typst 错误日志 ---")
                        # Typst 的错误信息通常在 stderr
                        print(result.stdout or "没有标准输出。")
                        print(result.stderr or "没有标准错误输出。")
                        print("----------------------")

                except FileNotFoundError:
                    print("错误：'typst' 命令未找到。")
                    print("请确保您已安装 Typst 并将其添加到了系统的 PATH 环境变量中。")
                    return
                except Exception as e:
                    print(f"处理文件 '{file}' 时发生未知错误: {e}")

    if not found_typ_file:
        print(f"在目录 '{source_dir}' 中没有找到任何 .typ 文件。")


def main():
    """
    主函数，用于解析命令行参数。
    """
    parser = argparse.ArgumentParser(
        description="编译目录下的所有 .typ 文件为 PDF。",
        epilog="示例: python compile_typ.py /path/to/your/typ/files"
    )
    parser.add_argument(
        "source_dir",
        type=str,
        help="包含 .typ 文件的源文件夹路径。"
    )
    args = parser.parse_args()

    if not os.path.isdir(args.source_dir):
        print(f"错误：提供的路径 '{args.source_dir}' 不是一个有效的目录。")
        sys.exit(1)

    compile_typ_files(args.source_dir)

if __name__ == '__main__':
    main()