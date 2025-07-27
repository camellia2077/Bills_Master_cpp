import os
import subprocess
import sys
import argparse

def compile_md_files(source_dir, font_name):
    """
    遍历指定目录下的所有 .md 文件，使用 pandoc 和 xelatex 引擎进行编译，
    并在输出目录中保留原始的文件夹层级结构。

    :param source_dir: 包含 .md 文件的源文件夹路径。
    :param font_name: 用于PDF正文的字体名称。
    """
    # 1. 定义基础输出文件夹路径
    base_output_dir = os.path.join(os.getcwd(), 'pdf_from_md')

    # 将输入的源目录转换为绝对路径，以确保路径计算的准确性
    source_dir = os.path.abspath(source_dir)

    found_md_file = False

    # 2. 遍历源文件夹及其所有子文件夹
    print(f"\n开始在 '{source_dir}' 中搜索 .md 文件...")
    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.endswith('.md'):
                found_md_file = True
                md_file_path = os.path.join(root, file)
                print(f"\n--- 发现 Markdown 文件: {md_file_path} ---")

                # 3. 计算相对路径并创建目标子目录
                relative_path = os.path.relpath(root, source_dir)
                target_output_dir = os.path.join(base_output_dir, relative_path)
                
                try:
                    os.makedirs(target_output_dir, exist_ok=True)
                except OSError as e:
                    print(f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}")
                    continue

                # 4. 准备 Pandoc 编译命令
                pdf_filename = os.path.splitext(file)[0] + '.pdf'
                final_pdf_path = os.path.join(target_output_dir, pdf_filename)
                
                # 构建 Pandoc 命令
                command = [
                    'pandoc',
                    '--from=gfm',  # <--- 这是关键的修改！明确指定输入格式为GFM。
                    md_file_path,
                    '-o', final_pdf_path,
                    '--pdf-engine=xelatex',
                    f'-V', f'mainfont={font_name}',
                    '-V', 'lang=zh-CN',
                    '-V', 'geometry:margin=1in'
                ]

                try:
                    print(f"正在编译 '{file}' -> '{final_pdf_path}'")
                    print(f"使用字体: '{font_name}'")
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
                        print("--- Pandoc/XeLaTeX 错误日志 ---")
                        # Pandoc 的错误信息通常在 stderr 中
                        print(result.stdout or "没有标准输出。")
                        print(result.stderr or "没有标准错误输出。")
                        print("------------------------------")

                except FileNotFoundError:
                    print("错误：'pandoc' 命令未找到。")
                    print("请确保您已安装 Pandoc 和一个 TeX 发行版 (如 TeX Live)，并将它们添加到了系统的 PATH 环境变量中。")
                    return
                except Exception as e:
                    print(f"处理文件 '{file}' 时发生未知错误: {e}")

    if not found_md_file:
        print(f"在目录 '{source_dir}' 中没有找到任何 .md 文件。")


def main():
    """
    主函数，用于解析命令行参数。
    """
    parser = argparse.ArgumentParser(
        description="编译目录下的所有 .md 文件为 PDF，可指定字体。",
        epilog="示例: python compile_md.py /path/to/your/md/files --font \"Noto Serif SC\""
    )
    parser.add_argument(
        "source_dir",
        type=str,
        help="包含 .md 文件的源文件夹路径。"
    )
    parser.add_argument(
        "--font",
        type=str,
        default="Noto Serif SC",
        help="指定PDF使用的主要字体名称。默认为 'Noto Serif SC'。"
    )
    args = parser.parse_args()

    if not os.path.isdir(args.source_dir):
        print(f"错误：提供的路径 '{args.source_dir}' 不是一个有效的目录。")
        sys.exit(1)

    compile_md_files(args.source_dir, args.font)

if __name__ == '__main__':
    main()