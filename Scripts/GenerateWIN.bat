@echo off
setlocal EnableExtensions
pushd "%~dp0.."

echo [WorkEncourager] Generating Visual Studio 2022 solution...
"%~dp0Premake\premake5.exe" --file=Build.lua vs2022
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    echo [WorkEncourager] Premake failed with exit code %RESULT%.
) else (
    echo [WorkEncourager] Generated WorkEncourager.sln
    echo [WorkEncourager] Open the solution or run Scripts\BuildWIN.bat.
)

popd
if /I not "%~1"=="--no-pause" pause
exit /b %RESULT%
