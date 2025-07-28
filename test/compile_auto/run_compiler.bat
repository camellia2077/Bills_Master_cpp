@echo off
cd /d %~dp0

REM 设置硬编码路径
set "TARGET_PATH=C:\Computer\my_github\github_cpp\bill_master\Bills_Master_cpp\my_test\exported_files"

REM 执行 Python 脚本并传入参数
python compiler.py auto "%TARGET_PATH%"

pause
