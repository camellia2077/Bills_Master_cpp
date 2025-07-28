# main.py
import os
import sys
import argparse
import time
import shutil

# 从 internal 包中导入命令处理函数
from internal.handlers import handle_auto, handle_tex, handle_md, handle_rst, handle_typ

def main():
    program_start_time = time.perf_counter()
    parser = argparse.ArgumentParser(
        description="一个通用的、支持并行的文档编译器。",
        epilog="示例: python main.py auto ./my_docs -j 8"
    )
    
    # 通用参数
    parser.add_argument('--output-dir', type=str, default='output_pdf', help="顶级输出目录 (默认: 'output_pdf')")
    parser.add_argument('--no-clean', action='store_true', help='【可选】启动时不清理旧的输出目录。')
    parser.add_argument(
        '--jobs', '-j',
        type=int,
        default=None,
        help="【可选】并行编译的任务数量 (默认: 使用所有可用的CPU核心)"
    )

    subparsers = parser.add_subparsers(dest='command', required=True, help='编译命令')
    
    # 'auto' 命令
    parser_auto = subparsers.add_parser('auto', help='自动检测源目录下的子目录并调用相应编译器')
    parser_auto.add_argument('source_dir', type=str, help='包含待编译子目录的父文件夹路径 (例如 ./my_docs)')
    parser_auto.add_argument('--font', type=str, default="Noto Serif SC", help="为 Pandoc 指定 CJK 字体 (默认: Noto Serif SC)")
    parser_auto.set_defaults(func=handle_auto)

    # 'tex' 命令
    parser_tex = subparsers.add_parser('tex', help='编译指定目录下的所有 .tex 文件')
    parser_tex.add_argument('source_dir', type=str, help='包含 .tex 文件的源文件夹路径')
    parser_tex.set_defaults(func=handle_tex)
    
    # 'md' 命令
    parser_md = subparsers.add_parser('md', help='编译指定目录下的所有 .md 文件')
    parser_md.add_argument('source_dir', type=str, help='包含 .md 文件的源文件夹路径')
    parser_md.add_argument('--font', type=str, default="Noto Serif SC", help="指定 CJK 字体 (默认: Noto Serif SC)")
    parser_md.set_defaults(func=handle_md)

    # 'rst' 命令
    parser_rst = subparsers.add_parser('rst', help='编译指定目录下的所有 .rst 文件')
    parser_rst.add_argument('source_dir', type=str, help='包含 .rst 文件的源文件夹路径')
    parser_rst.add_argument('--font', type=str, default="Noto Serif SC", help="指定 CJK 字体 (默认: Noto Serif SC)")
    parser_rst.set_defaults(func=handle_rst)
    
    # 'typ' 命令
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