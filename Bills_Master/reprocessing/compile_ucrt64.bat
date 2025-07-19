@echo off
REM ============================================================================
REM =  Batch script to compile the Reprocessor Python module in an MSYS2 UCRT64  =
REM =  environment. (With architecture fix)                                   =
REM ============================================================================
REM
REM  ** IMPORTANT **
REM  Please ensure you are running this script from an MSYS2 UCRT64 terminal.
REM

REM --- Step 1: Change directory to the script's location ---
cd /d "%~dp0"
echo.
echo [INFO] Current working directory changed to: %cd%

REM --- Step 2: Clean up the old build directory ---
echo [INFO] Checking for an old 'build' directory...
if exist build (
    echo [ACTION] 'build' directory found. Deleting...
    rd /s /q build
    echo [SUCCESS] Old 'build' directory has been removed.
) else (
    echo [INFO] No old 'build' directory found. No cleanup needed.
)

REM --- Step 3: Create and enter a new build directory ---
echo [ACTION] Creating a new 'build' directory...
mkdir build
cd build
echo [SUCCESS] Entered 'build' directory.

REM --- Step 4: Run CMake to configure the project ---
REM ** THE FIX IS HERE **
REM We now use `cygpath -w` to convert the MSYS2 path (e.g., /ucrt64/bin/python)
REM into a native Windows path (e.g., C:\msys64\ucrt64\bin\python.exe)
REM before passing it to CMake. This resolves the execution issue.
echo.
echo [ACTION] Running CMake to configure the project...
for /f "delims=" %%p in ('bash -c "cygpath -w $(which python)"') do (
    echo [INFO] Forcing CMake to use Python executable: %%p
    cmake .. -G "MinGW Makefiles" -DPython3_EXECUTABLE="%%p"
)


REM Check if CMake succeeded. %errorlevel% is 0 on success.
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration FAILED! Please check the error messages above.
    exit /b %errorlevel%
)
echo [SUCCESS] CMake configured successfully!

REM --- Step 5: Execute the build ---
echo.
echo [ACTION] Building the project with CMake --build...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build FAILED! Please check the error messages above.
    exit /b %errorlevel%
)

REM --- Finished ---
echo.
echo ============================================================================
echo [SUCCESS] Build completed successfully!
echo.
echo The compiled Python module can be found in the current directory (%cd%).
echo ============================================================================
echo.
