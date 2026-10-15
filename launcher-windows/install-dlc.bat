@echo off
rem Drag your own WWE 2K14 DLC package files (or a folder of them) onto this file.
rem Needs Python 3 (https://www.python.org, tick "Add python.exe to PATH" when installing).
setlocal
set "HERE=%~dp0"
if "%~1"=="" (
  echo Drag WWE 2K14 DLC package files, or a folder containing them, onto install-dlc.bat.
  pause
  exit /b 1
)
set "PY=py -3"
where py >nul 2>nul || set "PY=python"
%PY% "%HERE%install-dlc.py" %* --userdata "%HERE%userdata"
echo.
echo Start play-wwe2k14.bat; installed DLC shows up in game like it does on the console.
pause
