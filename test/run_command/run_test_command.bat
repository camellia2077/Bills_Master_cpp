@echo off

title Test Runner for test_commond.py

rem --- 切换到当前BAT文件所在的目录 ---
rem %~dp0 会扩展为当前批处理文件所在的驱动器和路径
cd /d "%~dp0"



python run_tests.py

