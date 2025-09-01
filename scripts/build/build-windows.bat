@echo off
REM Simple NTP Daemon - Windows Build Script
REM Copyright 2024 SimpleDaemons
REM Licensed under Apache 2.0

setlocal enabledelayedexpansion

REM Script configuration
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..
set BUILD_DIR=%PROJECT_ROOT%\build
set BUILD_TYPE=Release
set GENERATOR=Visual Studio 17 2022
set ARCH=x64

echo Building simple-ntpd on Windows...

REM Check if we're on Windows
if not "%OS%"=="Windows_NT" (
    echo ERROR: This script is designed for Windows systems
    exit /b 1
)

REM Check for Visual Studio
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    echo ERROR: Visual Studio not found
    echo Please install Visual Studio 2019 or 2022 with C++ development tools
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: CMake not found
    echo Please install CMake from https://cmake.org/download/
    exit /b 1
)

REM Check CMake version
for /f "tokens=3" %%i in ('cmake --version 2^>nul ^| findstr /r "cmake version"') do (
    set CMAKE_VERSION=%%i
    goto :version_check_done
)
:version_check_done

if not defined CMAKE_VERSION (
    echo ERROR: Could not determine CMake version
    exit /b 1
)

echo Found CMake version: %CMAKE_VERSION%

REM Clean and create build directory
if exist "%BUILD_DIR%" (
    echo Cleaning build directory...
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure build
echo Configuring build...
cmake "%PROJECT_ROOT%" ^
    -G "%GENERATOR%" ^
    -A %ARCH% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_CXX_STANDARD=17 ^
    -DBUILD_TESTS=ON ^
    -DBUILD_EXAMPLES=ON

if %errorLevel% neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM Build project
echo Building project...
cmake --build . --config %BUILD_TYPE% --parallel

if %errorLevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

REM Run tests if available
if exist "test\Debug\*.exe" (
    echo Running tests...
    ctest --output-on-failure --config %BUILD_TYPE%
) else if exist "test\Release\*.exe" (
    echo Running tests...
    ctest --output-on-failure --config %BUILD_TYPE%
) else (
    echo No tests found to run
)

echo Build completed successfully!
echo.
echo Build artifacts are in: %BUILD_DIR%
echo.
echo Next steps:
echo   1. Test the binary: %BUILD_DIR%\%BUILD_TYPE%\simple-ntpd.exe
echo   2. Install: cmake --install . --config %BUILD_TYPE%
echo   3. Create package: cpack -G NSIS -C %BUILD_TYPE%
