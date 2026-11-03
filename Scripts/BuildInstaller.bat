@echo off
setlocal EnableExtensions
pushd "%~dp0.."

set "ISCC="
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not defined ISCC for %%I in (ISCC.exe) do if exist "%%~$PATH:I" set "ISCC=%%~$PATH:I"

if not defined ISCC (
    echo [WorkEncourager] Inno Setup 6 was not found.
    echo [WorkEncourager] Install it from https://jrsoftware.org/isdl.php and run this script again.
    goto :fail
)

if not exist "dist\WorkEncourager.exe" (
    echo [WorkEncourager] dist\WorkEncourager.exe is missing. Run Scripts\BuildWIN.bat first.
    goto :fail
)

echo [WorkEncourager] Building installer...
"%ISCC%" /Qp "installer\WorkEncourager.iss"
if errorlevel 1 (
    echo [WorkEncourager] Installer build failed.
    goto :fail
)

powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\WriteChecksums.ps1"
if errorlevel 1 (
    echo [WorkEncourager] Could not generate SHA-256 checksums.
    goto :fail
)

echo.
echo [WorkEncourager] Installer ready in dist\
echo [WorkEncourager] Checksums: dist\SHA256SUMS.txt
popd
if /I not "%~1"=="--no-pause" pause
exit /b 0

:fail
popd
if /I not "%~1"=="--no-pause" pause
exit /b 1
