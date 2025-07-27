# compilers.py

import os
import shutil
import subprocess
import tempfile
from abc import ABC, abstractmethod

class BaseCompiler(ABC):
    """编译器的抽象基类，定义了编译单个文件的接口。"""
    
    @abstractmethod
    def compile(self, source_path: str, output_dir: str):
        """编译单个源文件并将其 PDF 输出到指定目录。"""
        pass

class LaTeXCompiler(BaseCompiler):
    """使用 xelatex 编译 .tex 文件。"""
    
    def compile(self, source_path: str, output_dir: str):
        base_name = os.path.splitext(os.path.basename(source_path))[0]
        final_pdf_path = os.path.join(output_dir, f"{base_name}.pdf")
        
        with tempfile.TemporaryDirectory() as temp_dir:
            command = [
                "xelatex",
                "-interaction=nonstopmode",
                "-halt-on-error",
                f"-output-directory={temp_dir}",
                source_path
            ]
            
            print(f"  > 编译: {os.path.basename(source_path)}")
            process = subprocess.run(command, capture_output=True, text=True, errors='ignore')

            if process.returncode == 0:
                generated_pdf = os.path.join(temp_dir, f"{base_name}.pdf")
                if os.path.exists(generated_pdf):
                    shutil.move(generated_pdf, final_pdf_path)
                    print(f"    ✅ 成功 -> {final_pdf_path}")
                else:
                    print(f"    ❌ 失败: 编译成功但未找到 PDF。")
            else:
                print(f"    ❌ 失败: 返回码 {process.returncode}")
                # 可以选择在这里打印 process.stdout 来进行调试

class TypstCompiler(BaseCompiler):
    """使用 typst 编译 .typ 文件。"""

    def compile(self, source_path: str, output_dir: str):
        base_name = os.path.splitext(os.path.basename(source_path))[0]
        final_pdf_path = os.path.join(output_dir, f"{base_name}.pdf")
        
        command = ["typst", "compile", source_path, final_pdf_path]

        print(f"  > 编译: {os.path.basename(source_path)}")
        process = subprocess.run(command, capture_output=True, text=True, errors='ignore')

        if process.returncode == 0:
            print(f"    ✅ 成功 -> {final_pdf_path}")
        else:
            print(f"    ❌ 失败: 返回码 {process.returncode}")
            print(f"      错误信息: {process.stderr.strip()}")