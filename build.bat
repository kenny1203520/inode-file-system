@echo off
echo ==========================================
echo       Building MiniFS Project
echo ==========================================



echo [1/3] Compiling Main Test (minifs.exe)...
g++ -std=c++14 -I./include src/main.cpp src/FileSystem.cpp src/InodeManager.cpp src/DiskEmulator.cpp src/Bitmap.cpp -o minifs.exe
if errorlevel 1 (
    echo Error compiling minifs.exe
    pause
    exit /b 1
)

echo [2/3] Compiling Interactive Shell (shell.exe)...
g++ -std=c++14 -I./include src/Shell.cpp src/FileSystem.cpp src/InodeManager.cpp src/DiskEmulator.cpp src/Bitmap.cpp -o shell.exe
if errorlevel 1 (
    echo Error compiling shell.exe
    pause
    exit /b 1
)

echo [3/3] Compiling GUI File Manager (gui.exe)...
g++ -std=c++14 -I./include src/GuiMain.cpp src/FileSystem.cpp src/InodeManager.cpp src/DiskEmulator.cpp src/Bitmap.cpp -o gui.exe -mwindows -luser32 -lgdi32 -lcomctl32
if errorlevel 1 (
    echo Error compiling gui.exe
    pause
    exit /b 1
)

echo.
echo ==========================================
echo        Build Complete!
echo ==========================================
echo Run the following files:
echo   1. minifs.exe  - Automated Test Script
echo   2. shell.exe   - Interactive Command Line
echo   3. gui.exe     - Graphical File Manager
echo.
pause
