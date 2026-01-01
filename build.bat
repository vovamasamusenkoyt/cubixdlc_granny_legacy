@echo off
echo Building dx11_hook DLL...

if not exist "build" mkdir build
cd build

if exist "C:\Program Files\CMake\bin\cmake.exe" (
    set CMAKE_EXE="C:\Program Files\CMake\bin\cmake.exe"
) else (
    set CMAKE_EXE=cmake
)

echo Configuring project...
%CMAKE_EXE% ..
if errorlevel 1 (
    echo Error: CMake configuration failed.
    cd ..
    pause
    exit /b 1
)

echo Building project...
%CMAKE_EXE% --build . --config Release
if errorlevel 1 (
    echo Error: Build failed.
    cd ..
    pause
    exit /b 1
)

cd ..
echo.
echo Build completed successfully!
echo DLL location: build\bin\Release\dx11_hook.dll
echo.
pause

