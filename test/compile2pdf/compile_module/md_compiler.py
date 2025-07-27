# compilers/pandoc_compiler.py
import os
import subprocess
from .base_compiler import BaseCompiler # 使用相对导入

class MdCompiler(BaseCompiler):
    """
    使用 Pandoc 将 Markdown 文件编译为 PDF 的具体实现。
    这个编译器特别配置用于处理包含中文的 Markdown 文件。
    """

    def compile(self, source_path: str, output_dir: str):
        """
        使用 Pandoc 编译单个 Markdown 文件为 PDF。
        
        Args:
            source_path (str): 源 Markdown 文件的完整路径。
            output_dir (str): PDF 文件应被保存的目标目录。
        """
        # 确保输出目录存在
        os.makedirs(output_dir, exist_ok=True)
        
        # 1. 从源文件路径构建输出文件名
        base_name = os.path.splitext(os.path.basename(source_path))[0]
        final_pdf_path = os.path.join(output_dir, f"{base_name}.pdf")

        # 2. 将命令行指令分解成一个列表
        # 这是最关键的一步，它将您的命令翻译成 subprocess 模块可以理解的格式
        command = [
            "pandoc",
            source_path,                               # 输入文件
            "-f", "markdown+hard_line_breaks",         # 指定输入格式和扩展
            "-t", "latex",                             # 指定中间格式
            "--pdf-engine=xelatex",                    # 指定PDF引擎
            "-V", "mainfont=Noto Serif SC",                   # 设置模板变量 (字体)
            "-o", final_pdf_path                       # 指定输出文件
        ]

        # 3. 执行命令并捕获输出
        print(f"  > [Pandoc] 编译: {os.path.basename(source_path)}")
        
        # 使用 subprocess.run 执行命令
        process = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='ignore')

        # 4. 检查结果并打印反馈
        if process.returncode == 0:
            print(f"    ✅ 成功 -> {final_pdf_path}")
        else:
            print(f"    ❌ 失败: Pandoc 返回码 {process.returncode}")
            # 打印 Pandoc 返回的错误信息，这对于调试至关重要
            print(f"      错误信息 (stdout): {process.stdout.strip()}")
            print(f"      错误信息 (stderr): {process.stderr.strip()}")