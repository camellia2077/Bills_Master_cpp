# compile_module/rst_compiler.py

import os
import subprocess
from .base_compiler import BaseCompiler # Use relative import

class RstCompiler(BaseCompiler):
    """
    Uses Pandoc to compile reStructuredText (.rst) files to PDF.
    This compiler is specifically configured to handle Chinese characters
    by using the xelatex engine and a specified CJK font.
    """

    def compile(self, source_path: str, output_dir: str):
        """
        Compiles a single .rst file to PDF using Pandoc.
        
        Args:
            source_path (str): The full path to the source .rst file.
            output_dir (str): The directory where the PDF file should be saved.
        """
        # Ensure the output directory exists
        os.makedirs(output_dir, exist_ok=True)
        
        # 1. Construct the output PDF file path
        base_name = os.path.splitext(os.path.basename(source_path))[0]
        final_pdf_path = os.path.join(output_dir, f"{base_name}.pdf")

        # 2. Define the command list based on your working command
        # This is the core logic for compiling RST with Chinese support.
        command = [
            "pandoc",
            source_path,
            "-o", final_pdf_path,
            "--pdf-engine=xelatex",
            "-V", "mainfont=Noto Serif SC"  # Or any other Chinese font you have installed
        ]

        # This print statement is for debugging and will only be visible if you 
        # modify main.py to show individual file compilation again.
        print(f"  > [Pandoc-RST] Compiling: {os.path.basename(source_path)}")
        
        # 3. Execute the command
        process = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='ignore')

        # 4. Check the result and provide feedback
        if process.returncode == 0:
            print(f"    ✅ Success -> {final_pdf_path}")
        else:
            print(f"    ❌ Failed: Pandoc returned code {process.returncode}")
            # Printing stderr is crucial for debugging font or LaTeX issues
            print(f"      Error (stdout): {process.stdout.strip()}")
            print(f"      Error (stderr): {process.stderr.strip()}")