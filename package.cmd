@echo off
set "YYYYMMDD=%date:~10,4%%date:~4,2%%date:~7,2%"
set "OBSAGENTS_ROOT=%CD%"
set "ISCC=%OBSAGENTS_ROOT%\installer\isc\ISCC.exe"
cd ..\..\..
set "OBS_ROOT=%CD%"
set "BUILD_ROOT=%OBS_ROOT%\build64"
set "MFC_BROWSER_CHECK=%BUILD_ROOT%\plugins\MyFreeCams\Sidekick\mfc-browser\cmake_install.cmake"
cd "%BUILD_ROOT%\plugins\MyFreeCams\Sidekick\installer"
if exist "%MFC_BROWSER_CHECK%" ("%ISCC%" /Qp Installer-del-obs-browser.iss) else ("%ISCC%" /Qp Installer.iss)
REM signtool.exe sign /v /f "%OBSAGENTS_ROOT%\installer\myfreecams.pfx" /p myfreecams /tr http://timestamp.globalsign.com/?signature=sha2 /td sha256 %BUILD_ROOT%\MFC-Sidekick-Setup-%YYYYMMDD%.exe
echo Installer location: %BUILD_ROOT%\MFC-Sidekick-Setup-%YYYYMMDD%.exe
cd "%OBSAGENTS_ROOT%"
exit /b
