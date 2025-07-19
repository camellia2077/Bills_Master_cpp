import os
import subprocess
import shutil
import tempfile
from abc import ABC, abstractmethod

# --- 全局变量定义 ---
# 定义要编译的文件类型。可以设置为 ['tex'], ['typ'], 或 ['tex', 'typ']。
# 默认为 ['tex', 'typ']，表示编译 LaTeX 和 Typst 文件。
COMPILE_TYPES = ['typ', 'tex'] # 默认编译Typst和LaTeX

class BaseCompiler(ABC):
    """
    编译器的抽象基类，定义了通用编译流程和方法。
    """
    def __init__(self, script_dir: str, source_subfolder: str, output_subfolder: str, source_extension: str):
        self.script_dir = script_dir
        self.source_subfolder = source_subfolder
        self.output_subfolder = output_subfolder
        self.source_extension = source_extension

        self.source_root = os.path.join(self.script_dir, "exported_files", self.source_subfolder)
        self.pdf_output_root = os.path.join(self.script_dir, "pdf_files", self.output_subfolder)

    @property
    @abstractmethod
    def compiler_command_prefix(self) -> list:
        """返回编译器命令的前缀部分，不包含输入和输出文件。"""
        pass

    @property
    @abstractmethod
    def check_command(self) -> list:
        """返回用于检查编译器是否存在的命令。"""
        pass

    @property
    @abstractmethod
    def needs_temp_dir(self) -> bool:
        """指示编译器是否需要临时目录来处理中间文件。"""
        pass

    @abstractmethod
    def _build_compile_command(self, source_file_full_path: str, temp_compile_dir: str, final_pdf_path: str) -> list:
        """
        根据不同的编译器构建具体的编译命令。
        Args:
            source_file_full_path (str): 源文件的完整路径。
            temp_compile_dir (str): 临时编译目录的路径 (如果 needs_temp_dir 为 True)。
            final_pdf_path (str): 最终 PDF 的输出路径。
        Returns:
            list: 编译命令及其参数。
        """
        pass

    @abstractmethod
    def _handle_successful_compile(self, base_name: str, temp_compile_dir: str, final_pdf_path: str, process_stdout: str, process_stderr: str):
        """
        处理成功编译后的逻辑，例如移动 PDF、检查文件是否存在。
        Args:
            base_name (str): 源文件名 (不含扩展名)。
            temp_compile_dir (str): 临时编译目录的路径。
            final_pdf_path (str): 最终 PDF 的输出路径。
            process_stdout (str): 编译器标准输出。
            process_stderr (str): 编译器标准错误。
        """
        pass

    def _check_source_directory(self) -> bool:
        """检查源目录是否存在。"""
        if not os.path.exists(self.source_root):
            print(f"错误：源目录 '{self.source_root}' 未找到。跳过编译 {self.source_extension} 文件。")
            return False
        return True

    def _create_output_directories(self):
        """创建 PDF 输出目录（如果不存在）。"""
        os.makedirs(self.pdf_output_root, exist_ok=True)

    def _check_compiler_availability(self) -> bool:
        """检查编译器命令是否可用。"""
        compiler_name = self.compiler_command_prefix[0]
        try:
            subprocess.run(self.check_command, capture_output=True, check=True, timeout=10)
            print(f"'{compiler_name}' 命令已找到。")
            return True
        except FileNotFoundError:
            print(f"错误：未找到 '{compiler_name}' 命令。跳过编译 {self.source_extension} 文件。")
            print(f"请确保已安装相应的编译器，并将其可执行文件目录添加到系统 PATH 环境变量中。")
            if compiler_name == "xelatex":
                print("例如，在 Windows 上可以安装 MiKTeX 或 TeX Live。在 Linux 上，安装 texlive-xetex 软件包。")
            elif compiler_name == "typst":
                print("例如，从 Typst 官网下载并安装 CLI 工具 (https://typst.app/docs/getting-started/cli/)。")
            return False
        except subprocess.CalledProcessError as e:
            print(f"警告：找到 '{compiler_name}' 命令，但在检查时返回错误 ({e.returncode})。输出：")
            print(e.stdout.decode(errors='ignore') if e.stdout else "无输出")
            print(e.stderr.decode(errors='ignore') if e.stderr else "无输出")
            print("将继续尝试编译，但可能失败。")
            return True # 允许继续尝试，但用户应注意
        except subprocess.TimeoutExpired:
            print(f"警告：'{compiler_name}' 命令超时。它可能存在但响应缓慢。")
            print("将继续尝试编译，但编译可能缓慢或存在问题。")
            return True # 允许继续尝试

    def _compile_single_file(self, source_file_full_path: str):
        """
        编译单个源文件，并处理其输出。
        """
        base_name = os.path.splitext(os.path.basename(source_file_full_path))[0]

        relative_path_from_source_root = os.path.relpath(os.path.dirname(source_file_full_path), self.source_root)
        target_pdf_dir = os.path.join(self.pdf_output_root, relative_path_from_source_root)
        os.makedirs(target_pdf_dir, exist_ok=True)
        final_pdf_path = os.path.join(target_pdf_dir, base_name + ".pdf")

        temp_compile_dir = None
        try:
            if self.needs_temp_dir:
                temp_compile_dir = tempfile.mkdtemp()
                print(f"\n正在编译：{source_file_full_path}")
                print(f"  临时输出目录：{temp_compile_dir}")
                print(f"  最终 PDF 将保存到：{final_pdf_path}")
            else:
                print(f"\n正在编译：{source_file_full_path}")
                print(f"  最终 PDF 将保存到：{final_pdf_path}")
            
            command = self._build_compile_command(source_file_full_path, temp_compile_dir, final_pdf_path)
            process = subprocess.run(command, capture_output=True, text=True, errors='ignore')

            if process.returncode == 0:
                self._handle_successful_compile(base_name, temp_compile_dir, final_pdf_path, process.stdout, process.stderr)
            else:
                print(f"  错误：{os.path.basename(source_file_full_path)} 编译失败。返回码：{process.returncode}")
                print("--- 编译器标准输出 ---")
                print(process.stdout)
                print("--- 编译器标准错误 ---")
                print(process.stderr)

        except Exception as e:
            print(f"  编译 {source_file_full_path} 时发生意外错误：{e}")
        finally:
            if self.needs_temp_dir and temp_compile_dir and os.path.exists(temp_compile_dir):
                try:
                    shutil.rmtree(temp_compile_dir)
                except OSError as e:
                    print(f"  清理临时目录 {temp_compile_dir} 时出错：{e}")

    def compile_all(self):
        """
        主编译方法：设置环境，遍历并编译所有相关源文件。
        """
        if not self._check_source_directory():
            return
        if not self._check_compiler_availability():
            return

        self._create_output_directories()

        print(f"\n--- 正在编译 {self.source_extension} 文件 ---")
        print(f"正在扫描文件：{self.source_root}")
        print(f"PDF 将保存到：{self.pdf_output_root}")

        compiled_count = 0
        for root, _, files in os.walk(self.source_root):
            for file in files:
                if file.endswith(self.source_extension):
                    compiled_count += 1
                    source_file_full_path = os.path.join(root, file)
                    self._compile_single_file(source_file_full_path)
        
        if compiled_count == 0:
            print(f"  未找到要编译的 {self.source_extension} 文件。")
        print(f"\n--- {self.source_extension} 文件编译过程完成 ---")

class LaTeXCompiler(BaseCompiler):
    def __init__(self, script_dir: str):
        super().__init__(script_dir, "latex_bills", "latex_bills", ".tex")

    @property
    def compiler_command_prefix(self) -> list:
        return ["xelatex", "-interaction=nonstopmode", "-halt-on-error"]

    @property
    def check_command(self) -> list:
        return ["xelatex", "--version"]

    @property
    def needs_temp_dir(self) -> bool:
        return True

    def _build_compile_command(self, source_file_full_path: str, temp_compile_dir: str, final_pdf_path: str) -> list:
        # LaTeX 使用 -output-directory 将所有输出文件放入临时目录
        return self.compiler_command_prefix + [
            f"-output-directory={temp_compile_dir}",
            source_file_full_path
        ]

    def _handle_successful_compile(self, base_name: str, temp_compile_dir: str, final_pdf_path: str, process_stdout: str, process_stderr: str):
        generated_pdf_name = base_name + ".pdf"
        generated_pdf_temp_path = os.path.join(temp_compile_dir, generated_pdf_name)

        if os.path.exists(generated_pdf_temp_path):
            shutil.move(generated_pdf_temp_path, final_pdf_path)
            print(f"  成功：PDF 已移动到 {final_pdf_path}")
        else:
            print(f"  警告：未在临时目录中找到 {base_name}.pdf。请检查以下日志。")
            print("--- 编译器标准输出 ---")
            print(process_stdout)
            print("--- 编译器标准错误 ---")
            print(process_stderr)

class TypstCompiler(BaseCompiler):
    def __init__(self, script_dir: str):
        super().__init__(script_dir, "typst_bills", "typst_bills", ".typ")

    @property
    def compiler_command_prefix(self) -> list:
        return ["typst", "compile"]

    @property
    def check_command(self) -> list:
        return ["typst", "--version"]

    @property
    def needs_temp_dir(self) -> bool:
        return False # Typst 直接输出 PDF，不需要临时目录

    def _build_compile_command(self, source_file_full_path: str, temp_compile_dir: str, final_pdf_path: str) -> list:
        # Typst 直接指定输入文件和输出文件
        return self.compiler_command_prefix + [
            source_file_full_path,
            final_pdf_path
        ]

    def _handle_successful_compile(self, base_name: str, temp_compile_dir: str, final_pdf_path: str, process_stdout: str, process_stderr: str):
        if os.path.exists(final_pdf_path):
            print(f"  成功：PDF 已保存到 {final_pdf_path}")
        else:
            print(f"  警告：未找到 {base_name}.pdf。请检查以下日志。")
            print("--- 编译器标准输出 ---")
            print(process_stdout)
            print("--- 编译器标准错误 ---")
            print(process_stderr)

if __name__ == "__main__":
    current_script_dir = os.path.dirname(os.path.abspath(__file__))
    print(f"将编译的文件类型: {', '.join(COMPILE_TYPES)}")

    compilers = {
        'tex': LaTeXCompiler(current_script_dir),
        'typ': TypstCompiler(current_script_dir)
    }

    for compile_type in COMPILE_TYPES:
        if compile_type in compilers:
            compilers[compile_type].compile_all()
        else:
            print(f"警告：不支持的编译类型 '{compile_type}'。请检查 COMPILE_TYPES 变量。")
    
    print("\n所有选定文件类型的编译任务已完成。")