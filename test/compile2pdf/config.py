# config.py

# --- 用户配置 ---

# 定义要编译的文件类型。
# 可选: ['tex'], ['typ'], 或 ['tex', 'typ' , "md" ,"rst"]
# --- CHANGE THIS LINE ---
COMPILE_TYPES = ['tex']

# 定义源文件和PDF输出的根目录名称
SOURCE_ROOT_DIR = "C:\\Computer\\my_github\\github_cpp\\bill_master\\Bills_Master_cpp\\my_test\\exported_files"
PDF_OUTPUT_ROOT_DIR = "pdf_files"

# 为每种类型定义具体的子文件夹名称
COMPILER_CONFIGS = {
    "tex": {
        "source_subfolder": "LaTeX_bills",
        "output_subfolder": "LaTeX_bills",
        "extension": ".tex"
    },
    "typ": {
        "source_subfolder": "typst_bills",
        "output_subfolder": "typst_bills",
        "extension": ".typ"
    },
    "md": {
         "source_subfolder": "Markdown_bills",
         "output_subfolder": "Markdown_bills",
         "extension": ".md"
     },

    # --- THIS PART IS ALREADY CORRECT ---
    "rst": {
         "source_subfolder": "reST_bills",
         "output_subfolder": "reST_bills",
         "extension": ".rst"
     }
}