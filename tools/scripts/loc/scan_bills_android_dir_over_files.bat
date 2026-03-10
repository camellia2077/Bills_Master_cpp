@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

echo [1/2] Scan Kotlin directories in apps/bills_android
python "%SCRIPT_DIR%run.py" --lang kt apps/bills_android/src/main/java apps/bills_android/src/test/java apps/bills_android/src/androidTest/java --dir-over-files 8 --dir-max-depth 3
if errorlevel 1 exit /b %errorlevel%

echo.
echo [2/2] Scan native C++ directories in apps/bills_android
python "%SCRIPT_DIR%run.py" --lang cpp apps/bills_android/src/main/cpp --dir-over-files 4 --dir-max-depth 2
