@echo off
setlocal
chcp 65001 >nul
pushd "%~dp0"

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo [ERROR] Visual Studio C++ x64 build environment not found:
    echo         %VCVARS%
    popd
    exit /b 1
)

call "%VCVARS%" >nul
if errorlevel 1 goto :failed

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0export_pdf_icon.ps1"
if errorlevel 1 goto :failed

rc.exe /nologo /fo embedded_pdf.res embedded_pdf.rc
if errorlevel 1 goto :failed

cl.exe /nologo /std:c17 /utf-8 /W4 /O2 /DUNICODE /D_UNICODE launcher.c embedded_pdf.res ^
    /link /SUBSYSTEM:WINDOWS /OUT:..\resume_viewer.stub.exe Shell32.lib User32.lib
if errorlevel 1 goto :failed

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0append_pdf_overlay.ps1"
if errorlevel 1 goto :failed

del /q launcher.obj embedded_pdf.res ..\resume_viewer.stub.exe 2>nul
echo [OK] Created: %CD%\..\resume_viewer.exe
popd
exit /b 0

:failed
echo [ERROR] Build failed.
popd
exit /b 1
