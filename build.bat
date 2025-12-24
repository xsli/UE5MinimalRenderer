@echo off
REM Build script for UE5MinimalRenderer on Windows

echo UE5MinimalRenderer Build Script
echo ================================

REM Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Please install CMake 3.20 or later.
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

echo.
echo Generating Visual Studio solution...
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake generation failed
    exit /b 1
)

echo.
echo Building Release configuration...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo ================================
echo Build completed successfully!
echo.
echo Executable location: build\Source\Runtime\Release\UE5MinimalRenderer.exe
echo.
echo To run the demo:
echo   cd build\Source\Runtime\Release
echo   UE5MinimalRenderer.exe
echo.

cd ..
