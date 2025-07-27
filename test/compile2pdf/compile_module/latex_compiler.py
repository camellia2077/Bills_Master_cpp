# latex_compiler.py
import os
import shutil
import subprocess
import tempfile
from .base_compiler import BaseCompiler

class LaTeXCompiler(BaseCompiler):
    def compile(self, source_path: str, output_dir: str):
        base_name = os.path.splitext(os.path.basename(source_path))[0]
        final_pdf_path = os.path.join(output_dir, f"{base_name}.pdf")

        # ======================================================================
        # ==                    终极修复：沙箱化编译                         ==
        # ======================================================================

        # 1. 为每一次编译都创建一个全新的、空的临时目录作为“沙箱”
        with tempfile.TemporaryDirectory() as temp_dir:
            
            # 2. 将需要编译的源文件复制到这个干净的沙箱中
            # 这是为了确保编译环境里只有这一个 .tex 文件
            base_name_with_ext = os.path.basename(source_path)
            temp_source_path = os.path.join(temp_dir, base_name_with_ext)
            shutil.copy2(source_path, temp_source_path)
            
            # 3. 定义编译命令。现在我们不再需要 -output-directory，
            #    因为所有的输出（.pdf, .aux, .log）都会自然地生成在当前工作目录（沙箱）中。
            command = [
                "xelatex",
                "-interaction=nonstopmode",
                "-halt-on-error",
                base_name_with_ext  # 只需文件名，因为我们就在它的目录里
            ]

            # 4. 将工作目录(cwd)设置为这个临时的沙箱目录来执行命令
            process = subprocess.run(
                command,
                capture_output=True,
                text=True,
                errors='ignore',
                cwd=temp_dir  # <-- 让 xelatex 在这个一次性的沙箱里工作
            )
            
            # 5. 编译完成后，从沙箱中找到生成的 PDF
            generated_pdf = os.path.join(temp_dir, f"{base_name}.pdf")
            log_file_path = os.path.join(temp_dir, f"{base_name}.log")

            # ======================================================================
            # ==                         修复结束                              ==
            # ======================================================================

            if process.returncode == 0:
                if os.path.exists(generated_pdf):
                    # 将 PDF 移动到最终的目标位置
                    shutil.move(generated_pdf, final_pdf_path)
                    print(f"    ✅ 成功 -> {final_pdf_path}")
                else:
                    print(f"    ❌ 失败: 编译成功但未找到 PDF。")
                    if os.path.exists(log_file_path):
                        self._print_latex_errors(log_file_path)
            else:
                print(f"    ❌ 失败: 返回码 {process.returncode}")
                if os.path.exists(log_file_path):
                    self._print_latex_errors(log_file_path)
                    
        # `with` 语句块结束时，沙箱目录以及其中的所有辅助文件（.aux, .log等）
        # 都会被自动、彻底地删除，确保下一次编译不受任何影响。

    def _print_latex_errors(self, log_path: str):
        """尝试从 LaTeX 日志文件中提取并打印错误信息。"""
        try:
            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
                print("      --- 日志尾部信息 ---")
                for line in lines[-30:]:
                    print(f"        {line.strip()}")
                print("      --------------------")
        except Exception as e:
            print(f"      无法读取日志文件: {e}")