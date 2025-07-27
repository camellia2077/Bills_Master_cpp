@echo off
:: This batch script automatically runs the Python compilation process.

:: Change directory to the location of this batch file.
:: %~dp0 is a special variable that expands to the drive and path of the batch file.
cd /d "%~dp0"

:: Print the current working directory (optional, for debugging).
echo Current directory: %cd%
echo.

:: Execute the main.py script.
echo --- Starting Python compile script ---
py main.py

:: Pause after the script finishes to allow the user to see the output.
echo.
echo --- Script finished. Press any key to exit ---
pause >nul