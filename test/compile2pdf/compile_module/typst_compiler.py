# typst_compiler.py
import os
import subprocess
from .base_compiler import BaseCompiler # 使用相对导入

class TypstCompiler(BaseCompiler):
    # ... 其余代码保持不变 ...
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

