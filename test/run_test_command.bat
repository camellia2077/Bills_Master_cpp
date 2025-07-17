@echo off
rem 设置窗口标题
title Test Runner for test_commond.py

rem --- 切换到当前BAT文件所在的目录 ---
rem %~dp0 会扩展为当前批处理文件所在的驱动器和路径
cd /d "%~dp0"

echo =======================================================
echo  Running Python test script: test_commond.py
echo  Current directory: %cd%
echo =======================================================
echo.

rem --- 运行Python脚本 ---
rem 假设 'py' 或 'python' 在系统的环境变量 PATH 中
py test_command.py

rem 检查Python脚本的执行结果
if %errorlevel% neq 0 (
    echo.
    echo =======================================================
    echo  Python script finished with errors (errorlevel: %errorlevel%).
    echo =======================================================
) else (
    echo.
    echo =======================================================
    echo  Python script completed successfully.
    echo =======================================================
)

rem --- 暂停以查看输出 ---
echo.
pause
