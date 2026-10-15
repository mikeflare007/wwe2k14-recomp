@echo off
setlocal EnableDelayedExpansion
title WWE 2K14 Recomp
rem Start script: reads settings.txt next to it, then runs wwe2k14.exe with your game folder.
set "HERE=%~dp0"
set "GAME=%HERE%game"
cd /d "%HERE%"

rem New build delivered: join the download parts, unpack wwe2k14.exe + DLLs, then remove the parts
if exist "%HERE%wwe2k14-build.zip.000" (
  echo Unpacking the new game build...
  copy /b "wwe2k14-build.zip.000"+"wwe2k14-build.zip.001" "wwe2k14-build.zip" >nul || goto :fail
  tar -xf "wwe2k14-build.zip" || goto :fail
  del "wwe2k14-build.zip" "wwe2k14-build.zip.000" "wwe2k14-build.zip.001"
)

rem ---- Settings (settings.txt next to this file) ----
set "RES=4k"
set "FPS=original"
set "FULL=no"
if exist "%HERE%settings.txt" (
  for /f "usebackq eol=# tokens=1,2 delims==" %%a in ("%HERE%settings.txt") do (
    if /i "%%a"=="resolution" set "RES=%%b"
    if /i "%%a"=="fps" set "FPS=%%b"
    if /i "%%a"=="fullscreen" set "FULL=%%b"
    if /i "%%a"=="game_folder" if not "%%b"=="" set "GAME=%%b"
  )
)
set "WWE2K14_RES_SCALE=3"
if /i "%RES%"=="720p" set "WWE2K14_RES_SCALE=1"
if /i "%RES%"=="1440p" set "WWE2K14_RES_SCALE=2"
if /i "%RES%"=="4k" set "WWE2K14_RES_SCALE=3"
if /i "%RES%"=="2160p" set "WWE2K14_RES_SCALE=3"
set "FX="
if /i "%RES%"=="4k-fsr" (
  set "WWE2K14_RES_SCALE=2"
  set "FX=--present_effect=fsr"
)
set "WWE2K14_KEEP_30="
if "%FPS%"=="30" set "WWE2K14_KEEP_30=1"
if /i "%FPS%"=="original" set "WWE2K14_KEEP_30=1"
set "FS=--fullscreen=false"
if /i "%FULL%"=="yes" set "FS=--fullscreen=true"

if not exist "%GAME%\default.xex" (
  echo Can't find default.xex in "%GAME%". Set game_folder= in settings.txt.
  goto :fail
)
mkdir "%HERE%logs" 2>nul
mkdir "%HERE%userdata" 2>nul
for /f %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "STAMP=%%t"
set "LOG=%HERE%logs\wwe2k14-%STAMP%.log"
set REX_DEBUG_UI=false
rem The title update's files are read from UPDATE:\ and the game uses the cache: drive
set "REX_UPDATE_DATA_ROOT=%HERE%update"
set REX_MOUNT_CACHE=true

echo Starting WWE 2K14 at %RES%, fps=%FPS% (change these in settings.txt).
echo The first matches can stutter while graphics shaders are built; it gets smoother as the cache fills.
echo Log file: %LOG%
"%HERE%wwe2k14.exe" "%GAME%" %FS% %FX% --vulkan_allow_present_mode_immediate=false --ignore_offset_for_ranged_allocations=true --log_verbose=false --mnk_mode=true "--log_file=%LOG%" "--user_data_root=%HERE%userdata" --debug_ui=false
echo.
echo The game closed (exit code %errorlevel%). If something went wrong, the log is in the logs folder.
pause
exit /b 0

:fail
echo Something went wrong. Take a screenshot of this window when reporting the problem.
pause
exit /b 1
