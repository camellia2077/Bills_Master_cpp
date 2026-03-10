import os
import subprocess
import tempfile
import time
from collections.abc import Callable

from ..compilers import build_md_to_typ_command, build_typ_command, get_typst_template_content


def compile_single_file(
    input_path: str,
    final_pdf_path: str,
    target_output_dir: str,
    command_builder: Callable,
    log_file_type: str,
) -> dict:
    file_name = os.path.basename(input_path)
    command = command_builder(input_path, final_pdf_path, target_output_dir)
    try:
        os.makedirs(target_output_dir, exist_ok=True)
    except OSError as e:
        return {
            "success": False,
            "file": file_name,
            "duration": 0,
            "log": f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}",
        }
    try:
        file_start_time = time.perf_counter()
        result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8")
        file_duration = time.perf_counter() - file_start_time
        if result.returncode == 0:
            return {
                "success": True,
                "file": file_name,
                "duration": file_duration,
                "log": f"✅ 成功: '{file_name}'",
            }
        error_log = (
            f"\n{'=' * 20} 错误日志: {file_name} {'=' * 20}\n"
            f"❌ 失败: '{file_name}' (耗时: {file_duration:.2f}s)\n"
            f"--- {log_file_type} 编译器错误日志 ---\n{result.stderr or result.stdout}\n{'=' * 50}"
        )
        return {
            "success": False,
            "file": file_name,
            "duration": file_duration,
            "log": error_log,
        }
    except Exception as e:
        return {
            "success": False,
            "file": file_name,
            "duration": 0,
            "log": f"❌ 处理文件 '{file_name}' 时发生未知错误: {e}",
        }


def compile_md_via_typ(
    input_path: str, final_pdf_path: str, target_output_dir: str, font: str
) -> dict:
    file_name = os.path.basename(input_path)
    typ_filename = os.path.splitext(file_name)[0] + ".typ"
    intermediate_typ_path = os.path.join(target_output_dir, typ_filename)

    try:
        os.makedirs(target_output_dir, exist_ok=True)
    except OSError as e:
        return {
            "success": False,
            "file": file_name,
            "log": f"❌ 错误：无法创建输出子目录 '{target_output_dir}': {e}",
        }

    conversion_duration = 0.0
    temp_template_path = None

    try:
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".typ", encoding="utf-8", delete=False
        ) as temp_template:
            temp_template_path = temp_template.name
            template_content = get_typst_template_content(font)
            temp_template.write(template_content)
            temp_template.close()

        conversion_command = build_md_to_typ_command(
            input_path, intermediate_typ_path, temp_template_path
        )

        conv_start_time = time.perf_counter()
        conv_result = subprocess.run(
            conversion_command, capture_output=True, text=True, encoding="utf-8"
        )
        conversion_duration = time.perf_counter() - conv_start_time

        if conv_result.returncode != 0:
            return {
                "success": False,
                "file": file_name,
                "conversion_time": conversion_duration,
                "log": f"❌ 步骤 1/2 (MD->Typ) 失败: {conv_result.stderr or conv_result.stdout}",
            }

    except OSError as e:
        return {
            "success": False,
            "file": file_name,
            "log": f"❌ 步骤 1/2 (MD->Typ) 创建临时模板失败: {e}",
        }
    finally:
        if temp_template_path and os.path.exists(temp_template_path):
            try:
                os.remove(temp_template_path)
            except OSError:
                pass

    compile_command = build_typ_command(intermediate_typ_path, final_pdf_path, None)
    comp_start_time = time.perf_counter()
    comp_result = subprocess.run(compile_command, capture_output=True, text=True, encoding="utf-8")
    compilation_duration = time.perf_counter() - comp_start_time

    if os.path.exists(intermediate_typ_path):
        try:
            os.remove(intermediate_typ_path)
        except OSError:
            pass

    if comp_result.returncode != 0:
        return {
            "success": False,
            "file": file_name,
            "conversion_time": conversion_duration,
            "compilation_time": compilation_duration,
            "log": f"❌ 步骤 2/2 (Typ->PDF) 失败: {comp_result.stderr or comp_result.stdout}",
        }

    return {
        "success": True,
        "file": file_name,
        "conversion_time": conversion_duration,
        "compilation_time": compilation_duration,
        "total_time": conversion_duration + compilation_duration,
        "log": f"✅ 成功: '{file_name}'",
    }
