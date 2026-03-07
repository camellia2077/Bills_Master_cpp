import os
from pathlib import Path


def _env_list(name: str, default: list[str]) -> list[str]:
    raw = os.environ.get(name, "").strip()
    if not raw:
        return list(default)
    return [item.strip() for item in raw.split(",") if item.strip()]


def _env_bool(name: str, default: bool) -> bool:
    raw = os.environ.get(name, "").strip().lower()
    if not raw:
        return default
    return raw in {"1", "true", "yes", "on"}


_SCRIPT_DIR = Path(__file__).resolve().parent
_REPO_ROOT = _SCRIPT_DIR.parents[2]


# 1. 源文件夹的父目录（可用环境变量覆盖）
SOURCE_DIRECTORY = os.environ.get(
    "BILLS_COMPILE2PDF_SOURCE",
    str(
        _REPO_ROOT
        / "tests"
        / "output"
        / "artifact"
        / "bills_tracer"
        / "latest"
        / "exported_files"
    ),
)

# 2. 统一的输出目录（可用环境变量覆盖）
OUTPUT_DIRECTORY = os.environ.get(
    "BILLS_COMPILE2PDF_OUTPUT",
    str(_REPO_ROOT / "tests" / "output" / "artifact" / "reporting" / "compile2pdf"),
)

# 3. 指定要编译的文档类型（可用环境变量覆盖，逗号分隔）
# 可选值: 'TeX', 'Markdown', 'RST', 'Typst'
COMPILE_TYPES = _env_list("BILLS_COMPILE2PDF_TYPES", ["TeX"])


# === 增量编译设置 ===
INCREMENTAL_COMPILE = _env_bool("BILLS_COMPILE2PDF_INCREMENTAL", True)
# ========================


# === 清理设置 ===
CLEAN_OUTPUT_DEFAULT = _env_bool("BILLS_COMPILE2PDF_CLEAN", False)
# ========================


# --- Markdown 编译器配置 ---
MARKDOWN_COMPILERS = _env_list("BILLS_COMPILE2PDF_MD_COMPILERS", ["pandoc"])
BENCHMARK_LOOPS = int(os.environ.get("BILLS_COMPILE2PDF_BENCHMARK_LOOPS", "3"))
