@echo off
setlocal EnableExtensions
pushd "%~dp0.."

call "%~dp0GenerateWIN.bat" --no-pause
if errorlevel 1 goto :fail

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [WorkEncourager] vswhere.exe was not found. Install Visual Studio 2022 with the C++ desktop workload.
    goto :fail
)

set "VSPATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo [WorkEncourager] Visual Studio 2022 with the MSVC x64 toolchain was not found.
    goto :fail
)

call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [WorkEncourager] Failed to initialize the MSVC environment.
    goto :fail
)

echo [WorkEncourager] Building Dist x64...
msbuild "WorkEncourager.sln" /m /p:Configuration=Dist /p:Platform=x64 /v:minimal /nologo
if errorlevel 1 (
    echo [WorkEncourager] Build failed.
    goto :fail
)

if not exist "bin\windows-x86_64\Dist\WorkEncourager.exe" (
    echo [WorkEncourager] Expected executable was not produced.
    goto :fail
)

if not exist "dist" mkdir "dist"
copy /Y "bin\windows-x86_64\Dist\WorkEncourager.exe" "dist\WorkEncourager.exe" >nul
if errorlevel 1 (
    echo [WorkEncourager] Could not copy the executable to dist.
    goto :fail
)

powershell -NoProfile -Command "$hash = (Get-FileHash -LiteralPath 'dist\WorkEncourager.exe' -Algorithm SHA256).Hash.ToLower(); Set-Content -LiteralPath 'dist\SHA256SUMS.txt' -Value ($hash + '  WorkEncourager.exe') -Encoding ASCII"
if errorlevel 1 (
    echo [WorkEncourager] Could not generate the SHA-256 checksum.
    goto :fail
)

echo.
echo [WorkEncourager] Done.
echo [WorkEncourager] Executable: dist\WorkEncourager.exe
echo [WorkEncourager] Checksum:   dist\SHA256SUMS.txt
popd
if /I not "%~1"=="--no-pause" pause
exit /b 0

:fail
popd
if /I not "%~1"=="--no-pause" pause
exit /b 1
