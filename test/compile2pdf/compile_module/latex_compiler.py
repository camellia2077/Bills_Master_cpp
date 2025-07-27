# latex_compiler.py
import os
import shutil
import subprocess
import tempfile
from .base_compiler import BaseCompiler # 使用相对导入

class LaTeXCompiler(BaseCompiler):
    # ... 其余代码保持不变 ...
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
                    print(f"    ❌ 失败: 编译成功但未找到 PDF。可能是源文件有问题。")
            else:
                print(f"    ❌ 失败: 返回码 {process.returncode}")