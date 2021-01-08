@echo off
setlocal
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set _DEPOT_TOOLS_COMMIT=master
REM set _DEPOT_TOOLS_COMMIT=d9c1c8
set _WEBRTC_VERSION=88
set _BRANCH_NUMBER=4324
set ARGS_RELEASE="use_rtti=true is_debug=false is_official_build=true use_custom_libcxx=false rtc_use_h264=true chrome_pgo_phase=0 ffmpeg_branding=\"Chrome\" is_chrome_branded=false target_cpu=\"x64\""
set ARGS_DEBUG="use_rtti=true is_debug=true use_custom_libcxx=false rtc_use_h264=true ffmpeg_branding=\"Chrome\" target_cpu=\"x64\" enable_iterator_debugging=true"
set IS_DEBUG=0
if "%~1"=="Debug" (set IS_DEBUG=1)
if "%BUILD_TYPE%"=="Debug" (set IS_DEBUG=1)

set "OBSAGENTS_ROOT=%CD%"
cd ..\..\..
set "OBS_ROOT=%CD%"
set "BUILD_ROOT=%OBS_ROOT%\build64"
cd ..
set "DEV_DIR=%CD%"
call :configure_log

if defined GYP_MSVS_VERSION (echo -- WebRTC: GYP_MSVS_VERSION already defined: %GYP_MSVS_VERSION% 1>> %LOG%) else (call :find_visual_studio)
if defined DEPOT_TOOLS_COMMIT (echo -- WebRTC: DEPOT_TOOLS_COMMIT already defined: %DEPOT_TOOLS_COMMIT% 1>> %LOG%) else (set DEPOT_TOOLS_COMMIT=%_DEPOT_TOOLS_COMMIT%)
if defined WEBRTC_VERSION (echo -- WebRTC: WEBRTC_VERSION already defined: %WEBRTC_VERSION% 1>> %LOG%) else (set WEBRTC_VERSION=%_WEBRTC_VERSION%)
if defined WEBRTC_ARGS (echo -- WebRTC: WEBRTC_ARGS already defined: %WEBRTC_ARGS% 1>> %LOG%) else (
  if %IS_DEBUG% EQU 1 (set "WEBRTC_ARGS=%ARGS_DEBUG%") else (set "WEBRTC_ARGS=%ARGS_RELEASE%")
)

if defined WEBRTC_ROOT_DIR (echo -- WebRTC: WEBRTC_ROOT_DIR already defined: %WEBRTC_ROOT_DIR% 1>> %LOG%) else (
  if defined WEBRTC (set "WEBRTC_ROOT_DIR=%WEBRTC%") else (set "WEBRTC_ROOT_DIR=%DEV_DIR%\webrtc")
)
if defined BRANCH_NUMBER (echo -- WebRTC: BRANCH_NUMBER already defined: %BRANCH_NUMBER% 1>> %LOG%) else (set BRANCH_NUMBER=%_BRANCH_NUMBER%)
if %WEBRTC_VERSION% LSS 73 (set WEBRTC_BRANCH=%WEBRTC_VERSION%) else (set WEBRTC_BRANCH=m%WEBRTC_VERSION%)
if %WEBRTC_VERSION% GTR 79 (set WEBRTC_BRANCH=%BRANCH_NUMBER%)

set "wrtcStartTime=%time: =0%"
echo -- WebRTC: build started %DATE% %TIME%
echo -- WebRTC: version: m%WEBRTC_VERSION%
echo -- WebRTC: args: %WEBRTC_ARGS%
echo -- WebRTC: root: %WEBRTC_ROOT_DIR%

call :install_depot_tools

rmdir /q /s "%WEBRTC_ROOT_DIR%" 1>> %LOG% 2>&1
mkdir "%WEBRTC_ROOT_DIR%\lib" 1>> %LOG% 2>&1
cd "%WEBRTC_ROOT_DIR%"

call "%DEPOT_TOOLS%\fetch" --nohooks --force webrtc 1>> %LOG%
cd "%WEBRTC_ROOT_DIR%\src"

call "%DEPOT_TOOLS%\git" checkout -f refs/remotes/branch-heads/%WEBRTC_BRANCH%
call "%DEPOT_TOOLS%\gclient" sync -f -D -R --with_branch_heads --with_tags
call "%DEPOT_TOOLS%\gn" gen out\m%WEBRTC_VERSION% --args=%WEBRTC_ARGS%
call "%DEPOT_TOOLS%\autoninja" -C out\m%WEBRTC_VERSION%

copy /y /b "%WEBRTC_ROOT_DIR%\src\out\m%WEBRTC_VERSION%\obj\webrtc.lib" "%WEBRTC_ROOT_DIR%\lib\webrtc.lib" 1>> %LOG%
if errorlevel 1 (exit /b)
copy /y "%WEBRTC_ROOT_DIR%\src\out\m%WEBRTC_VERSION%\args.gn" "%WEBRTC_ROOT_DIR%\lib\args.gn" 1>> %LOG%
cd ..

call :write_webrtc_version

echo -- WebRTC: finished building
set "wrtcEndTime=%time: =0%"
call :elapsed_time
cd "%OBSAGENTS_ROOT%"
endlocal
exit /b

:::::::: END OF SCRIPT ::::::::

:install_depot_tools
  if not exist "%DEV_DIR%\depot_tools\gclient.py" (
    echo -- WebRTC: Downloading depot tools
    curl -ksLRO https://storage.googleapis.com/chrome-infra/depot_tools.zip
    echo -- WebRTC: Downloading depot tools - done
    echo -- WebRTC: Installing depot_tools: %DEPOT_TOOLS_COMMIT%
    7z x -aoa depot_tools.zip -odepot_tools 1>> %LOG%
    del depot_tools.zip
    cd "%DEV_DIR%\depot_tools"
    call "%DEV_DIR%\depot_tools\gclient" >nul
    if not "%DEPOT_TOOLS_COMMIT%"=="master" (
      call "%DEV_DIR%\depot_tools\git" reset --hard
      call "%DEV_DIR%\depot_tools\git" checkout -b m%WEBRTC_VERSION% %DEPOT_TOOLS_COMMIT%
      set DEPOT_TOOLS_UPDATE=0
      call "%DEV_DIR%\depot_tools\vpython3" update_depot_tools_toggle.py --disable
      call "%DEV_DIR%\depot_tools\git" add .
      call "%DEV_DIR%\depot_tools\git" commit -m "disable auto update" 1>> %LOG%
    )
    echo -- WebRTC: Installing depot_tools - done
  ) else (
    echo -- WebRTC: Depot tools already installed
  )
  set "PATH=%DEV_DIR%\depot_tools;%PATH%"
  set "DEPOT_TOOLS=%DEV_DIR%\depot_tools"
  echo -- WebRTC: DEPOT_TOOLS: %DEPOT_TOOLS%
  goto :eof

:find_visual_studio
  set VS9Pre=C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\MSBuild\Current\Bin\MSBuild.exe
  set VS9E=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe
  set VS9P=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe
  set VS9C=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe
  if defined MSVC_PATH (set "VS=%MSVC_PATH%") else (
    if exist "%VS9Pre%" (set "VS=%VS9Pre%" & set "VS_VER=16 2019") else (
      if exist "%VS9E%" (set "VS=%VS9E%" & set "VS_VER=16 2019") else (
        if exist "%VS9P%" (set "VS=%VS9P%" & set "VS_VER=16 2019") else (
          if exist "%VS9C%" (set "VS=%VS9C%" & set "VS_VER=16 2019") else (
            echo -- WebRTC: Visual Studio 2019 not found
            echo -- WebRTC: Installing Windows SDK
            powershell.exe -file "%OBSAGENTS_ROOT%\scripts\install-winsdk.ps1"
            if errorlevel 1 (echo -- Installing Windows SDK - failed) else (
              echo -- WebRTC: Installing Windows SDK - done
            )
            echo -- WebRTC: Installing Visual Studio 2019
            powershell.exe -file "%OBSAGENTS_ROOT%\scripts\install-vs2019.ps1"
            if errorlevel 1 (
              echo -- WebRTC: Installing Visual Studio 2019 - failed
              exit /b 1
            )
            echo -- WebRTC: Installing Visual Studio 2019 - done
            set "VS=%VS9C%" & set "VS_VER=16 2019"
          )
        )
      )
    )
  )
  set "MSVC_PATH=%VS%"
  if defined MSVC_VERSION (echo -- MSVC_VERSION already defined: %MSVC_VERSION% 1>> %LOG% 2>&1) else (set "MSVC_VERSION=Visual Studio %VS_VER%")
  set "MSVC_VERSION_64=%MSVC_VERSION% Win64"
  set "GYP_MSVS_VERSION=%VS_VER:~3,4%"
  goto :eof

:write_webrtc_version
  echo #ifndef WEBRTC_VERSION_H_> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo #define WEBRTC_VERSION_H_>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo.>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo #ifdef WEBRTC_VERSION>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo #undef WEBRTC_VERSION>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo #endif>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo.>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo #define WEBRTC_VERSION %WEBRTC_VERSION%>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo.>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  echo #endif  // WEBRTC_VERSION_H_>> "%WEBRTC_ROOT_DIR%\webrtc_version.h"
  goto :eof

:configure_log
  set CUR_YYYY=%date:~10,4%
  set CUR_MM=%date:~4,2%
  set CUR_DD=%date:~7,2%
  set CUR_HH=%time:~0,2%
  if %CUR_HH% lss 10 (set CUR_HH=0%time:~1,1%)
  set CUR_NN=%time:~3,2%
  set CUR_SS=%time:~6,2%
  set DATE_TIME=%CUR_YYYY%%CUR_MM%%CUR_DD%-%CUR_HH%%CUR_NN%%CUR_SS%
  set "LOG=%DEV_DIR%\build-webrtc_%DATE_TIME%.log"
  goto :eof

:elapsed_time
  setlocal EnableDelayedExpansion
  :: Get elapsed time:
  set "end=!wrtcEndTime:%time:~8,1%=%%100)*100+1!" & set "start=!wrtcStartTime:%time:~8,1%=%%100)*100+1!"
  set /A "elap=((((10!end:%time:~2,1%=%%100)*60+1!%%100)-((((10!start:%time:~2,1%=%%100)*60+1!%%100)"
  :: Convert elapsed time to HH:MM:SS:CC format:
  set /A "cc=elap%%100+100,elap/=100,ss=elap%%60+100,elap/=60,mm=elap%%60+100,hh=elap/60+100"
  :: Display elapsed time
  echo.
  echo -- Start:    %wrtcStartTime%
  echo -- End:      %wrtcEndTime%
  echo -- Elapsed:  %hh:~1%%time:~2,1%%mm:~1%%time:~2,1%%ss:~1%%time:~8,1%%cc:~1%
  echo.
  endlocal
  goto :eof
