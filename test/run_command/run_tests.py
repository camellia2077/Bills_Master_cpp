# run_tests.py

import os
import sys

# --- [核心修改] ---
# 直接从 _test_internal 包导入模块
from _test_internal import app_config as config
from _test_internal import constants
from _test_internal.preparer import TestPreparer
from _test_internal.executor import CommandExecutor
from _test_internal.tasks import ImportTasks, QueryTasks, ExportTasks, DateExportTasks

def main():
    """主函数，根据配置协调整个测试流程。"""
    if os.name == 'nt': os.system('color')
    
    # 获取当前脚本所在目录，也就是我们的项目根目录
    project_root = os.path.dirname(os.path.abspath(__file__))

    # `project_root` 是所有操作的基础目录
    preparer = TestPreparer(project_root)

    # --- 步骤 1: 清理环境 (根据开关决定是否执行) ---
    if config.RUN_CLEANUP:
        if not preparer.cleanup_and_setup_dirs():
            sys.exit(1)
    else:
        print(f"{constants.YELLOW}--- Skipped Cleanup Step (as per config) ---{constants.RESET}")
    
    # --- 步骤 2: 准备运行时环境/复制文件 (根据开关决定是否执行) ---
    if config.RUN_PREPARE_ENV:
        if not preparer.prepare_runtime_env():
            sys.exit(1)
    else:
        print(f"{constants.YELLOW}--- Skipped Environment Preparation Step (as per config) ---{constants.RESET}")

    # --- 步骤 3: 执行指令测试 (根据开关决定是否执行) ---
    if config.RUN_TESTS:
        run_test_sequence(project_root, preparer)
    else:
        print(f"{constants.YELLOW}--- Skipped Test Execution Step (as per config) ---{constants.RESET}")
    
    print(f"\n{constants.GREEN}✅ Script finished.{constants.RESET}")


def run_test_sequence(project_root, preparer):
    """包含所有测试步骤的函数。"""
    # --- 路径与变量检查 ---
    exe_path = os.path.join(project_root, preparer.exe_name)
    output_dir = os.path.join(project_root, "py_output")
    
    os.makedirs(output_dir, exist_ok=True)
    
    if not os.path.exists(exe_path):
        print(f"\n{constants.RED}错误: 可执行文件 '{preparer.exe_name}' 未找到。{constants.RESET}")
        print(f"{constants.YELLOW}请在 config.toml 中设置 'run_prepare_env = true' 并重新运行脚本来复制文件。{constants.RESET}")
        sys.exit(1)

    if not os.path.exists(config.BILLS_DIR):
        print(f"\n{constants.RED}错误: 'bills' 文件夹不存在: '{config.BILLS_DIR}'{constants.RESET}")
        print(f"{constants.YELLOW}请检查 config.toml 文件中的 'bills_dir' 变量。{constants.RESET}")
        sys.exit(1)
        
    # --- 初始化任务 ---
    executor = CommandExecutor(exe_path, output_dir)
    importer = ImportTasks(executor, config.BILLS_DIR, config.IMPORT_DIR)
    querier = QueryTasks(executor)
    exporter = ExportTasks(executor)
    date_exporter = DateExportTasks(executor)

    # --- 执行测试序列 ---
    print(f"\n{constants.CYAN}========== Starting Test Sequence =========={constants.RESET}")
    
    base_tasks_ok = importer.run() and querier.run()
    final_result = False

    if base_tasks_ok:
        if config.RUN_EXPORT_ALL_TASKS:
            final_result = exporter.run()
        else:
            final_result = date_exporter.run()

    # --- 报告最终结果 ---
    if final_result:
        print(f"\n{constants.GREEN}✅ All test steps completed successfully!{constants.RESET}")
        print(f"{constants.GREEN}   Check the 'py_output' directory for detailed logs.{constants.RESET}")
    else:
        print(f"\n{constants.RED}❌ A test step failed. Please check the corresponding log file in the 'py_output' directory for details.{constants.RESET}")


if __name__ == "__main__":
    main()