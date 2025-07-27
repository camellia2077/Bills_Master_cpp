# base_compiler.py
from abc import ABC, abstractmethod

class BaseCompiler(ABC):
    """
    编译器的抽象基类。
    定义了所有具体编译器（如 LaTeX, Typst）都必须实现的通用接口。
    """
    
    @abstractmethod
    def compile(self, source_path: str, output_dir: str):
        """
        编译单个源文件并将其 PDF 输出到指定目录。
        
        Args:
            source_path (str): 源文件的完整路径。
            output_dir (str): PDF 文件应被保存的目标目录。
        """
        pass