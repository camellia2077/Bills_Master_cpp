import os
import subprocess
import sys
import shutil  # *** 新增: 导入 shutil 模块用于删除非空目录 ***

# ===================================================================
# 全局配置: 定义需要清理的文件和文件夹
# ===================================================================
FILES_TO_DELETE = [
    "bills.db"
]
DIRS_TO_DELETE = [
    "txt_raw",
    "exported_files"
]
# ===================================================================

def cleanup_previous_artifacts(base_dir):
    """
    Deletes specified files and directories to ensure a clean test environment.
    """
    # 1. 定义常量和颜色（用于美化输出）
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    
    print(f"{YELLOW}--- Cleaning up previous test artifacts ---{RESET}")
    
    # 2. 删除配置中指定的目录
    for dir_name in DIRS_TO_DELETE:
        dir_path = os.path.join(base_dir, dir_name)
        if os.path.exists(dir_path):
            try:
                shutil.rmtree(dir_path)
                print(f"  {GREEN}Successfully removed directory:{RESET} {dir_path}")
            except OSError as e:
                print(f"  {RED}Error removing directory {dir_path}: {e}{RESET}")
                
    # 3. 删除配置中指定的文件
    for file_name in FILES_TO_DELETE:
        file_path = os.path.join(base_dir, file_name)
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                print(f"  {GREEN}Successfully removed file:{RESET} {file_path}")
            except OSError as e:
                print(f"  {RED}Error removing file {file_path}: {e}{RESET}")
    
    print(f"{YELLOW}--- Cleanup complete ---\n{RESET}")

def run_bill_master_test():
    """
    Locates and runs the bill_master_cli.exe with the specified command.
    """
    # 定义常量和颜色
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    RESET = '\033[0m'

    # 获取当前脚本所在的目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 构建目标可执行文件和bills文件夹的路径
    exe_name = "bill_master_cli.exe"
    bills_folder_name = "bills"
    
    exe_path = os.path.join(script_dir, exe_name)
    bills_path = os.path.join(script_dir, bills_folder_name)
    
    print(f"{CYAN}--- Bill Master CLI Test Script ---{RESET}")
    print(f"Script directory: {script_dir}")
    print(f"Executable path:  {exe_path}")
    print(f"Bills folder path:  {bills_path}\n")

    # 检查所需的文件和文件夹是否存在
    if not os.path.exists(exe_path):
        print(f"{RED}Error: Executable '{exe_name}' not found in the script directory.{RESET}")
        print("Please place this script in the same folder as your executable.")
        sys.exit(1)
        
    if not os.path.exists(bills_path):
        print(f"{YELLOW}Warning: '{bills_folder_name}' folder not found. Creating it for you...{RESET}")
        try:
            os.makedirs(bills_path)
            print(f"{GREEN}Successfully created '{bills_folder_name}' folder.{RESET}")
            print(f"{YELLOW}Please add some .txt bill files to the '{bills_folder_name}' folder for a meaningful test.{RESET}")
        except OSError as e:
            print(f"{RED}Error: Could not create '{bills_folder_name}' folder: {e}{RESET}")
            sys.exit(1)

    # *** 新增: 在执行测试前调用清理函数 ***
    cleanup_previous_artifacts(script_dir)

    # 构建要执行的命令
    command_to_run = [
        exe_path,
        "-a",
        "e",
        bills_path
    ]
    
    print(f"{CYAN}Running command: {' '.join(command_to_run)}{RESET}\n")
    print(f"{YELLOW}--- Start of bill_master_cli.exe output ---{RESET}")

    try:
        # 执行命令
        with subprocess.Popen(
            command_to_run, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            text=True, 
            bufsize=1,
            encoding='utf-8',
            errors='ignore'
        ) as process:
            # 实时打印标准输出
            for line in process.stdout:
                print(line, end='')
                
            # 等待进程结束并捕获标准错误
            stdout, stderr = process.communicate()
            
            # 检查是否有错误输出
            if stderr:
                print(f"{RED}\n--- Errors detected ---{RESET}")
                print(f"{RED}{stderr}{RESET}")

            # 获取最终的返回码
            return_code = process.returncode

        print(f"\n{YELLOW}--- End of bill_master_cli.exe output ---{RESET}")

        # 根据返回码判断测试结果
        if return_code == 0:
            print(f"\n{GREEN}✅ Test Completed Successfully (Exit Code: 0).{RESET}")
        else:
            print(f"\n{RED}❌ Test Failed (Exit Code: {return_code}).{RESET}")
            
    except FileNotFoundError:
        print(f"{RED}Fatal Error: The system could not find the file specified: {exe_path}{RESET}")
    except Exception as e:
        print(f"{RED}An unexpected error occurred: {e}{RESET}")

if __name__ == "__main__":
    # 在Windows上启用ANSI颜色代码
    if os.name == 'nt':
        os.system('color')
    run_bill_master_test()