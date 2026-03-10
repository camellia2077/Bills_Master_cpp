@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
python "%SCRIPT_DIR%run.py" --lang py --dir-over-files --dir-max-depth 2 %*

