@echo off
echo ============================================
echo         Claude OS Build System
echo ============================================
echo.

where wsl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: WSL is required to build Claude OS.
    echo.
    echo Install WSL:
    echo   wsl --install
    echo.
    echo Then install Ubuntu from the Microsoft Store
    echo and run this script again.
    pause
    exit /b 1
)

echo Building Claude OS via WSL...
echo.

wsl bash -c "cd '%~dp0' && chmod +x build.sh && ./build.sh"

if exist "%~dp0claudeos.iso" (
    echo.
    echo ============================================
    echo   BUILD SUCCESSFUL: claudeos.iso
    echo ============================================
    echo.
    echo To test with QEMU ^(if installed^):
    echo   qemu-system-i386 -cdrom claudeos.iso -m 128M
    echo.
    echo To write to USB ^(use Rufus or Etcher^):
    echo   Select claudeos.iso and write to your USB drive
    echo.
) else (
    echo.
    echo BUILD FAILED - check errors above
)

pause
