@echo off
echo.
echo *** Configuring + Generating started %date% %time% ***
set "startTime=%time: =0%"

if "%~1"=="resume" (echo -- Resuming build... && goto :build)
if not "%~1"=="generate" (if not "%~1"=="configure" (if not "%~1"=="config" (setlocal)))

:: Clear environment variables if CLEAR_ENV is set or script called with arg clean/clear
if "%~1"=="clean" (set CLEAR_ENV=1)
if "%~1"=="clear" (set CLEAR_ENV=1)
if defined CLEAR_ENV (echo -- Clearing environment variables && call :clear_env)
if "%~1"=="Release" (set BUILD_TYPE=Release)
if "%~1"=="RelWithDebInfo" (set BUILD_TYPE=RelWithDebInfo)
if "%~1"=="Debug" (set BUILD_TYPE=Debug)
if "%~1"=="preserve" (set NO_RESET=1)

set _OBS_TAG=26.1.2
set _Deps_VERSION=2019
set _X264_VERSION=161.r3015
set _CURL_VERSION=7.68.0
set _VLC_VERSION=3.0.4
set _QT_VERSION=5.15.2
set _CEF_VERSION=75.1.14+gc81164e+chromium-75.0.3770.100
set _BOOST_VERSION=1.69.0
set _OPENSSL_VERSION=1.1.1
set _BROWSER_PANEL=0

set _BUILD_TYPE=RelWithDebInfo
set "_INSTALL_PREFIX=C:\Program Files\obs-studio"
set _QUIET=1

set "OBSAGENTS_ROOT=%CD%"
cd ..\..\..
set "OBS_ROOT=%CD%"
cd ..
set "DEV_DIR=%CD%"

:: Install git, cmake, curl, 7zip, grep, sed using scoop (install scooop if needed)
sed --version >nul 2>nul || powershell.exe -file "%OBSAGENTS_ROOT%\scripts\install-build-reqs.ps1" || echo -- Error running scripts/install-build-reqs.ps1

call :find_visual_studio
call :set_deps_hash
call :set_deps_version
call :get_openssl_version
call :url_encode_cef_version
call :set_deps_location
call :set_deps_filename
call :set_deps_url

cd "%OBS_ROOT%"
if defined NO_RESET (goto :no_reset)

rmdir /q /s additional_install_files >nul 2>nul
rmdir /q /s plugins\enc-amf >nul 2>nul
git reset --hard >nul
git submodule foreach git reset --hard >nul 2>nul
echo -- Checking out obs-studio %OBS_TAG%
echo.
git fetch >nul
git checkout %OBS_TAG% >nul 2>nul
git submodule update --init --recursive >nul 2>nul
git reset --hard >nul 2>nul
cd ..

:no_reset
call :edit_cmakelists

if "%~1"=="skip_download" (echo -- Skipping dependency check && goto :skip_download)
if defined NO_RESET (echo -- Skipping dependency check && goto :skip_download)

call :download_dependencies
if errorlevel 1 (exit /b)

call :install_dependencies
if errorlevel 1 (exit /b)

call :delete_downloads
cd "%OBSAGENTS_ROOT%"

call :build_webrtc
if errorlevel 1 (exit /b)

:skip_download
call :print_variables

cd "%OBS_ROOT%"
if exist "%PUBLIC%\Sidekick" (rmdir /q /s "%PUBLIC%\Sidekick" >nul 2>nul)
if exist build (rmdir /q /s build >nul 2>nul)
if exist build64 (rmdir /q /s build64 >nul 2>nul)
if exist "%BUILD_ROOT%" (rmdir /q /s "%BUILD_ROOT%" >nul 2>nul)

:build
mkdir "%BUILD_ROOT%"
cd "%BUILD_ROOT%"

:: CMake: generate solution for Visual Studio
cmake -G "%MSVC_VERSION%" -A x64 -T host=x64^
 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%^
 -DCMAKE_INSTALL_PREFIX="%InstallPath64%"^
 -DCURL_INCLUDE_DIR="%CURL_INCLUDE_DIR%"^
 -DCURL_LIBRARY="%CURL_LIBRARY%"^
 -DBOOST_ROOT="%BOOST_ROOT%"^
 -DBOOST_LIBRARYDIR="%BOOST_LIBRARYDIR%"^
 -DCEF_ROOT="%CEF_ROOT%"^
 -DCEF_ROOT_DIR="%CEF_ROOT%"^
 -DCEF_RUNTIME_LIBRARY_FLAG=/MT^
 -DBROWSER_PANEL=%BROWSER_PANEL%^
 -DBUILD_BROWSER=true^
 -DBUILD_CAPTIONS=true^
 -DCOMPILE_D3D12_HOOK=true^
 -DCOPIED_DEPENDENCIES=false^
 -DCOPY_DEPENDENCIES=true^
 -DENABLE_VLC=ON^
 -DCMAKE_SYSTEM_VERSION=10.0^
 -DVIRTUALCAM_GUID=%VIRTUALCAM_GUID%^
 ..
if errorlevel 1 (exit /b)

if "%~1"=="configure" (goto :skip_build)
if "%~1"=="generate" (goto :skip_build)
if "%~1"=="config" (goto :skip_build)

:: Build solution using Visual Studio
echo.
echo *** Build started %date% %time% ***
echo.
"%MSVC_PATH%" obs-studio.sln /nologo /nr:false /m /p:Configuration=%build_config%
if errorlevel 1 (exit /b)

echo.
echo *** Build completed %date% %time% ***
set "endTime=%time: =0%"
call :elapsed_time
cd "%OBSAGENTS_ROOT%"
if not "%~1"=="resume" (endlocal)
exit /b

:::::::: END OF SCRIPT ::::::::

:set_deps_hash
  set Deps_SHA1=d9412dd21d22f599a6e88d105cc3260a38cf8d74
  set X264_SHA1=0a1afc492ba4949eb7dbb0af8aabbf36424d6e32
  set CURL_SHA256=5e49e97ea3e3dd707f1f56985113b908b9a95e81c3212290c7fefbe4d79fb093
  set VLC_SHA1=218603f05ab5f30de0cc87b0ccceb57bb1efcfa4
  REM set QT_SHA1=4daec4733e75ad03b45fd351441f9a88d3cdc6d8
  set QT_SHA1=d42b99beef90da3947c0d544f9a1a58d29192aaa
  set CEF_MINIMAL_SHA1=c9dcb8edfa159cc0dc14202315af97d5166fffe8
  set CEF_SHA1=adf10a3d049760ff8a19590019ba157a089128c2
  set BOOST_SHA1=15208c018da1e552da7c07b9b1e5cf855bebe8b0
  set OPENSSL_SHA1=4a481f8c58b50fc0880b90af3f2e9a2b372d807d
  set VIRTUALCAM_GUID=A3FCE0F5-3493-419F-958A-ABA1250EC20B
  goto :eof

:set_deps_filename
  set Deps_FILENAME=dependencies%Deps_VERSION%.zip
  set X264_FILENAME=libx264_0.%X264_VERSION%_msvc16.zip
  set CURL_FILENAME=curl-%CURL_VERSION%-win64-mingw.zip
  set VLC_FILENAME=vlc-%VLC_VERSION%.tar.xz
  set QT_FILENAME=Qt_%QT_VERSION%.7z
  set CEF_FILENAME=cef_binary_%CEF_VERSION%_windows64.tar.bz2
  set BOOST_FILENAME=boost_%BOOST_VERSION:.=_%-msvc-14.1-64.exe
  set BOOST_SOURCE_FILENAME=boost_%BOOST_VERSION:.=_%.7z
  set OPENSSL_FILENAME=Win64OpenSSL-%OPENSSL_VERSION:.=_%%OPENSSL_VERSION_LETTER%.exe
  goto :eof

:set_deps_url
  set Deps_URL=https://cdn-fastly.obsproject.com/downloads/dependencies%Deps_VERSION%.zip
  set X264_URL=https://github.com/ShiftMediaProject/x264/releases/download/0.%X264_VERSION%/libx264_0.%X264_VERSION%_msvc16.zip
  set CURL_URL=https://curl.haxx.se/windows/dl-%CURL_VERSION%/curl-%CURL_VERSION%-win64-mingw.zip
  set VLC_URL=https://download.videolan.org/pub/videolan/vlc/%VLC_VERSION%/vlc-%VLC_VERSION%.tar.xz
  set QT_URL=https://cdn-fastly.obsproject.com/downloads/Qt_%QT_VERSION%.7z
  set CEF_URL=https://cef-builds.spotifycdn.com/cef_binary_%CEF_VERSION_URL_ENCODED%_windows64.tar.bz2
  set BOOST_URL=https://downloads.sourceforge.net/project/boost/boost-binaries/%BOOST_VERSION%/boost_%BOOST_VERSION:.=_%-msvc-14.1-64.exe
  set OPENSSL_URL=https://slproweb.com%OPENSSL_URL_PATH%
  goto :eof

:edit_cmakelists
  :: Append add_subdirectory(MyFreeCams) to %OBS_ROOT%\plugins\CMakeLists.txt
  echo -- Editing %OBS_ROOT%\plugins\CMakeLists.txt
  findstr /i /l "MyFreeCams" "%OBS_ROOT%\plugins\CMakeLists.txt" >nul
  if not errorlevel 1 (echo -- Editing %OBS_ROOT%\plugins\CMakeLists.txt - already modified) else (
    echo add_subdirectory^(MyFreeCams^)>> "%OBS_ROOT%\plugins\CMakeLists.txt"
    echo.>> "%OBS_ROOT%\plugins\CMakeLists.txt"
    echo -- Editing %OBS_ROOT%\plugins\CMakeLists.txt - success
  )
  :: Create CMakeLists.txt in %OBS_ROOT%\plugins\MyFreeCams
  echo -- Creating %OBS_ROOT%\plugins\MyFreeCams\CMakeLists.txt
  if exist "%OBS_ROOT%\plugins\MyFreeCams\CMakeLists.txt" (
    echo -- Creating %OBS_ROOT%\plugins\MyFreeCams\CMakeLists.txt - file exists
  ) else (
    echo add_subdirectory^(Sidekick^)> "%OBS_ROOT%\plugins\MyFreeCams\CMakeLists.txt"
    echo.>> "%OBS_ROOT%\plugins\MyFreeCams\CMakeLists.txt"
    echo -- Creating %OBS_ROOT%\plugins\MyFreeCams\CMakeLists.txt - success
  )
  if %BROWSER_PANEL% EQU 1 (
    :: Prepend return() to %OBS_ROOT%\plugins\obs-browser\CMakeLists.txt
    echo -- Editing %OBS_ROOT%\plugins\obs-broswer\CMakeLists.txt
    findstr /i /l "mfc" "%OBS_ROOT%\plugins\obs-browser\CMakeLists.txt" >nul
    if not errorlevel 1 (echo -- Editing %OBS_ROOT%\plugins\obs-browser\CMakeLists.txt - already modified) else (
      type "%OBS_ROOT%\plugins\obs-browser\CMakeLists.txt" > temp.txt
      echo return^(^) # replacing with mfc browser plugin> "%OBS_ROOT%\plugins\obs-browser\CMakeLists.txt"
      echo.>> "%OBS_ROOT%\plugins\obs-browser\CMakeLists.txt"
      type temp.txt >> "%OBS_ROOT%\plugins\obs-browser\CMakeLists.txt"
      del temp.txt
      echo -- Editing %OBS_ROOT%\plugins\obs-browser\CMakeLists.txt - success
    )
  )
  echo.
  goto :eof

:get_openssl_version
  :: Determine current version of OpenSSL 1.1.1 (older versions not available for security reasons)
  echo curl -s "https://slproweb.com/products/Win32OpenSSL.html" ^| grep -o "<a.href.*>" ^| sed -e "s/<a /\n<a /g"^
    ^| grep Win64OpenSSL-1_1_1.*exe ^| sed -e "s/<a.href=\"//" -e "s/\".*$//" -e "/^$/ d" > find_dl.cmd
  for /F "delims=" %%G in ('find_dl.cmd') do set OPENSSL_URL_PATH=%%G
  del find_dl.cmd
  for /F "delims=" %%G in ('echo %OPENSSL_URL_PATH% ^| grep -o ".[.]" ^| sed -e "s/\.//"') do set OPENSSL_VERSION_LETTER=%%G
  goto :eof

:find_visual_studio
  set "VS9Pre=C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\MSBuild\Current\Bin\MSBuild.exe"
  set "VS9E=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
  set "VS9P=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
  set "VS9C=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
  if defined MSVC_PATH (set "VS=%MSVC_PATH%") else (
    if exist "%VS9Pre%" (set "VS=%VS9Pre%" & set "VS_VER=16 2019") else (
      if exist "%VS9E%" (set "VS=%VS9E%" & set "VS_VER=16 2019") else (
        if exist "%VS9P%" (set "VS=%VS9P%" & set "VS_VER=16 2019") else (
          if exist "%VS9C%" (set "VS=%VS9C%" & set "VS_VER=16 2019") else (
            echo ** Visual Studio 2019 not found **
            echo -- Installing Windows SDK
            powershell.exe -file "%OBSAGENTS_ROOT%\scripts\install-winsdk.ps1"
            if errorlevel 1 (echo -- Installing Windows SDK - failed) else (
              echo -- Installing Windows SDK - done
            )
            echo -- Installing Visual Studio 2019
            powershell.exe -file "%OBSAGENTS_ROOT%\scripts\install-vs2019.ps1"
            if errorlevel 1 (
              echo -- Installing Visual Studio 2019 - failed
              exit /b 1
            )
            echo -- Installing Visual Studio 2019 - done
            set "VS=%VS9C%" & set "VS_VER=16 2019"
          )
        )
      )
    )
  )
  set "MSVC_PATH=%VS%"
  if defined MSVC_VERSION (echo -- MSVC_VERSION already defined: %MSVC_VERSION%) else (set "MSVC_VERSION=Visual Studio %VS_VER%")
  set "MSVC_VERSION_64=%MSVC_VERSION% Win64"
  set "GYP_MSVS_VERSION=%VS_VER:~3,4%"
  if defined BUILD_TYPE (echo -- BUILD_TYPE already defined: %BUILD_TYPE%) else (
    if defined build_config (set BUILD_TYPE=%build_config%) else (set BUILD_TYPE=%_BUILD_TYPE%)
  )
  set build_config=%BUILD_TYPE%
  goto :eof

:set_deps_version
  if defined OBS_TAG (echo -- OBS_TAG already defined: %OBS_TAG%) else (set OBS_TAG=%_OBS_TAG%)
  if defined Deps_VERSION (echo -- Deps_VERSION already defined: %Deps_VERSION%) else (set Deps_VERSION=%_Deps_VERSION%)
  if defined X264_VERSION (echo -- X264_VERSION already defined: %X264_VERSION%) else (set X264_VERSION=%_X264_VERSION%)
  if defined CURL_VERSION (echo -- CURL_VERSION already defined: %CURL_VERSION%) else (set CURL_VERSION=%_CURL_VERSION%)
  if defined VLC_VERSION (echo -- VLC_VERSION already defined: %VLC_VERSION%) else (set VLC_VERSION=%_VLC_VERSION%)
  if defined QT_VERSION (echo -- QT_VERSION already defined: %QT_VERSION%) else (set QT_VERSION=%_QT_VERSION%)
  if defined CEF_VERSION (echo -- CEF_VERSION already defined: %CEF_VERSION%) else (set CEF_VERSION=%_CEF_VERSION%)
  if defined BOOST_VERSION (echo -- BOOST_VERSION already defined: %BOOST_VERSION%) else (set BOOST_VERSION=%_BOOST_VERSION%)
  if defined OPENSSL_VERSION (echo -- OPENSSL_VERSION already defined: %OPENSSL_VERSION%) else (set OPENSSL_VERSION=%_OPENSSL_VERSION%)
  if not defined BROWSER_PANEL (set BROWSER_PANEL=%_BROWSER_PANEL%)
  goto :eof

:set_deps_location
  if defined BUILD_ROOT (echo -- BUILD_ROOT already defined: %BUILD_ROOT%) else (set "BUILD_ROOT=%OBS_ROOT%\build64")
  if defined InstallPath64 (echo -- InstallPath64 already defined: %InstallPath64%) else (
    if defined INSTALL_PATH (set "InstallPath64=%INSTALL_PATH%") else (set "InstallPath64=%_INSTALL_PREFIX%")
  )
  if defined DepsPath64 (echo -- DepsPath64 already defined: %DepsPath64%) else (
    if defined DepsPath (set "DepsPath64=%DepsPath%") else (set "DepsPath64=%DEV_DIR%\dependencies%Deps_VERSION%\win64")
  )
  if defined FFmpegPath (echo -- FFmpegPath already defined: %FFmpegPath%) else (set "FFmpegPath=%DepsPath64%\include")
  REM if defined X264_ROOT (echo -- X264_ROOT already defined: %X264_ROOT%) else (set "X264_ROOT=%DEV_DIR%\libx264-%X264_VERSION%")
  if defined x264Path (echo -- x264Path already defined: %x264Path%) else (set "x264Path=%DepsPath64%\include")
  REM if defined x264Path (echo -- x264Path already defined: %x264Path%) else (set "x264Path=%X264_ROOT%")
  REM if defined CURL_ROOT (echo -- CURL_ROOT already defined: %CURL_ROOT%) else (set "CURL_ROOT=%DEV_DIR%\curl\curl-%CURL_VERSION%-win64-mingw")
  if defined CURL_ROOT (echo -- CURL_ROOT already defined: %CURL_ROOT%) else (set "CURL_ROOT=%DepsPath64%")
  if defined CURL_INCLUDE_DIR (echo -- CURL_INCLUDE_DIR already defined: %CURL_INCLUDE_DIR%) else (set "CURL_INCLUDE_DIR=%CURL_ROOT%\include")
  if defined curlPath (echo -- curlPath already defined: %curlPath%) else (set "curlPath=%CURL_INCLUDE_DIR%")
  if defined CURL_LIBRARY (echo -- CURL_LIBRARY already defined: %CURL_LIBRARY%) else (set "CURL_LIBRARY=%CURL_ROOT%\bin\libcurl.lib")
  if defined VLCPath (echo -- VLCPath already defined: %VLCPath%) else (set "VLCPath=%DEV_DIR%\vlc\vlc-%VLC_VERSION%")
  if defined QTDIR64 (echo -- QTDIR64 already defined: %QTDIR64%) else (
    if defined QTDIR (set "QTDIR64=%QTDIR%") else (set "QTDIR64=%DEV_DIR%\Qt\%QT_VERSION%\msvc2019_64")
  )
  if defined CEF_64 (echo -- CEF_64 already defined: %CEF_64%) else (
    if defined CEF (set "CEF_64=%CEF%") else (set "CEF_64=%DEV_DIR%\CEF_64\cef_binary_%CEF_VERSION%_windows64")
    if defined CEF_ROOT (set "CEF_64=%CEF_ROOT%")
  )
  set "CEF_64=%CEF_64:\=/%"
  set "CEF_ROOT=%CEF_64%"
  if defined BOOST_ROOT (echo -- BOOST_ROOT already defined: %BOOST_ROOT%) else (set "BOOST_ROOT=%DEV_DIR%\boost\boost_%BOOST_VERSION:.=_%")
  if defined BOOST_INCLUDEDIR (echo -- BOOST_INCLUDEDIR already defined: %BOOST_INCLUDEDIR%) else (
    if defined BOOST_INCLUDE_DIR (set "BOOST_INCLUDEDIR=%BOOST_INCLUDE_DIR%") else (set "BOOST_INCLUDEDIR=%BOOST_ROOT%\boost")
  )
  if defined BOOST_LIBRARYDIR (echo -- BOOST_LIBRARYDIR already defined: %BOOST_LIBRARYDIR%) else (
    if defined BOOST_LIBRARY_DIR (set "BOOST_LIBRARYDIR=%BOOST_LIBRARY_DIR%") else (set "BOOST_LIBRARYDIR=%BOOST_ROOT%\lib64-msvc-14.1")
  )
  if defined OPENSSL_ROOT_DIR (echo -- OPENSSL_ROOT_DIR already defined: %OPENSSL_ROOT_DIR%) else (
    if defined OPENSSL (set "OPENSSL_ROOT_DIR=%OPENSSL%") else (set "OPENSSL_ROOT_DIR=%DEV_DIR%\OpenSSL-Win64")
  )
  if defined WEBRTC_ROOT_DIR (echo -- WEBRTC_ROOT_DIR already defined: %WEBRTC_ROOT_DIR%) else (
    if defined WEBRTC (set "WEBRTC_ROOT_DIR=%WEBRTC%") else (set "WEBRTC_ROOT_DIR=%DEV_DIR%\webrtc")
  )
  goto :eof

:url_encode_cef_version
  echo [uri]::EscapeDataString('%CEF_VERSION%')> %DEV_DIR%\temp.ps1
  for /F "delims=" %%G in ('powershell.exe . "%DEV_DIR%\temp.ps1"') do (set CEF_VERSION_URL_ENCODED=%%G)
  del "%DEV_DIR%\temp.ps1"
  goto :eof

:download_dependencies
  if defined VERBOSE (set QUIET=0) else (set QUIET=%_QUIET%)
  if %QUIET% EQU 1 (set "Q=<nul" & set "BS=-bso0 -bsp0")
  if %QUIET% EQU 1 (set "QMSBUILD=/verbosity:quiet")
  if %QUIET% EQU 1 (set "QWEBRTC=quiet")
  if not exist "%DepsPath64%\include\libavcodec\avcodec.h" (
    echo -- OBS dependencies: downloading & curl -ksLRO --url "%Deps_URL%"
    if errorlevel 1 (echo -- OBS dependencies: downloading - failed & exit /b)
    echo -- OBS dependencies: downloading - done
  ) else (echo -- OBS dependencies: already installed)
  REM if not exist "%X264_ROOT%\include\x264.h" (
  REM   echo -- x264: downloading & curl -ksLRO --url "%X264_URL%"
  REM   if errorlevel 1 (echo -- x264: downloading - failed & exit /b)
  REM   echo -- x264: downloading - done
  REM ) else (echo -- x264: already installed)
  REM if not exist "%CURL_LIBRARY%" (
  REM   echo -- Curl: downloading & curl -ksLRO --url "%CURL_URL%"
  REM   if errorlevel 1 (echo -- Curl: downloading - failed & exit /b)
  REM   echo -- Curl: downloading - done
  REM ) else (echo -- Curl: already installed: %CURL_VERSION%)
  if not exist "%VLCPath%\include\vlc\libvlc.h" (
    echo -- VLC: downloading & curl -ksLRO --url "%VLC_URL%"
    if errorlevel 1 (echo -- VLC: downloading - failed & exit /b)
    echo -- VLC: downloading - done
  ) else (echo -- VLC: already installed: %VLC_VERSION%)
  if not exist "%QTDIR64%\lib\cmake\Qt5\Qt5Config.cmake" (
    echo -- Qt: downloading & curl -ksLRO "%QT_URL%"
    if errorlevel 1 (echo -- Qt: downloading - failed & exit /b)
    echo -- Qt: downloading - done
  ) else (echo -- Qt: already installed: %QT_VERSION%)
  if not exist "%CEF_64%\Release\libcef.lib" (
    echo -- CEF: downloading & curl -ksLR -o "%CEF_FILENAME%" --url "%CEF_URL%"
    if errorlevel 1 (echo -- CEF: downloading - failed & exit /b)
    echo -- CEF: downloading - done
  ) else (echo -- CEF: already installed: %CEF_VERSION%)
  if not exist "%BOOST_INCLUDEDIR%\version.hpp" (
    echo -- Boost: downloading & curl -ksLRO --url "%BOOST_URL%"
    if errorlevel 1 (echo -- Boost: downloading - failed & exit /b)
    echo -- Boost: downloading - done
  ) else (echo -- Boost: already installed: %BOOST_VERSION%)
  if not exist "%OPENSSL_ROOT_DIR%\include\openssl\opensslv.h" (
    echo -- OpenSSL: downloading & curl -ksLRO --url "%OPENSSL_URL%"
    if errorlevel 1 (echo -- OpenSSL: downloading - failed & exit /b)
    echo -- OpenSSL: downloading - done
  ) else (echo -- OpenSSL: already installed: %OPENSSL_VERSION%)
  goto :eof

:install_dependencies
  call :check_file %Deps_FILENAME% %Deps_SHA1% && (
    echo -- OBS dependencies: hash verified
    echo -- OBS dependencies: installing
    rmdir /q /s "%DepsPath64%" >nul 2>nul
    7z x %BS% -aoa %Deps_FILENAME% -odependencies%Deps_VERSION%
    if errorlevel 1 (echo -- OBS dependencies: installing - failed & exit /b)
    echo -- OBS dependencies: installing - done
  )
  if errorlevel 1 (echo -- OBS dependencies: installing - failed & exit /b)
  REM call :check_file %X264_FILENAME% %X264_SHA1% && (
  REM   echo -- x264: hash verified
  REM   echo -- x264: installing
  REM   rmdir /q /s "%X264_ROOT%" >nul 2>nul
  REM   7z x %BS% -aoa %X264_FILENAME% -olibx264-%X264_VERSION%
  REM   if errorlevel 1 (echo -- x264: installing - failed & exit /b)
  REM   mkdir %X264_ROOT%\lib64
  REM   mkdir %X264_ROOT%\bin64
  REM   robocopy %X264_ROOT%\lib\x64 %X264_ROOT%\lib64 /mir >nul
  REM   robocopy %X264_ROOT%\bin\x64 %X264_ROOT%\bin64 /mir >nul
  REM   copy /b %X264_ROOT%\bin64\x264.dll %X264_ROOT%\bin64\libx264-%X264_VERSION%.dll
  REM   echo -- x264: installing - done
  REM )
  REM if errorlevel 4 (echo -- x264: installing - failed & exit /b)
  REM call :check_file %CURL_FILENAME% %CURL_SHA256% SHA256 && (
  REM   echo -- Curl: hash verified
  REM   echo -- Curl: installing
  REM   rmdir /q /s "%CURL_ROOT%" >nul 2>nul
  REM   7z x %BS% -aoa %CURL_FILENAME% -ocurl
  REM   if errorlevel 1 (echo -- Curl: installing - failed & exit /b)
  REM   echo -- Curl: installing - done
  REM )
  REM if errorlevel 1 (echo -- Curl: installing - failed & exit /b)
  call :check_file %VLC_FILENAME% %VLC_SHA1% && (
    echo -- VLC: hash verified
    echo -- VLC: installing
    rmdir /q /s "%VLCPath%" >nul 2>nul
    7z x %BS% -aoa -so %VLC_FILENAME% | 7z x %BS% -aoa -si -ttar -ovlc
    if errorlevel 1 (echo -- VLC: installing - failed & exit /b)
    echo -- VLC: installing - done
  )
  if errorlevel 1 (echo -- VLC: installing - failed & exit /b)
  call :check_file %QT_FILENAME% %QT_SHA1% && (
    echo -- Qt: hash verified
    echo -- Qt: installing
    rmdir /q /s "%QTDIR64%" >nul 2>nul
    7z x %BS% -aoa %QT_FILENAME% -oQt
    if errorlevel 1 (echo -- Qt: installing - failed & exit /b)
    echo -- Qt: installing - done
  )
  if errorlevel 1 (echo -- Qt: installing - failed & exit /b)
  call :check_file %CEF_FILENAME% %CEF_SHA1% && (
    echo -- CEF: hash verified
    echo -- CEF: extracting
    rmdir /q /s "%CEF_64%" >nul 2>nul
    7z x %BS% -aoa -so %CEF_FILENAME% | 7z x %BS% -aoa -si -ttar -oCEF_64
    if errorlevel 1 (echo -- CEF: extracting - failed & exit /b)
    echo -- CEF: extracting - done
    cd "%CEF_64%" & if errorlevel 1 (exit /b)
    rmdir /q /s tests >nul 2>nul && mkdir build && cd build
    echo -- CEF: configuring wrapper
    cmake -G "%MSVC_VERSION%" -A x64 -T host=x64 -Wno-dev .. >nul
    if errorlevel 1 (echo -- CEF: configuring wrapper - failed & exit /b)
    echo -- CEF: configuring wrapper - done
    echo -- CEF: building release wrapper
    "%MSVC_PATH%" cef.sln /nologo /nr:false /m /p:Configuration=Release %QMSBUILD%
    if errorlevel 1 (echo -- CEF: building release wrapper - failed & exit /b)
    echo -- CEF: building release wrapper - done
    echo -- CEF: building debug wrapper
    "%MSVC_PATH%" cef.sln /nologo /nr:false /m /p:Configuration=Debug %QMSBUILD%
    if errorlevel 1 (echo -- CEF: building debug wrapper - failed & exit /b)
    echo -- CEF: building debug wrapper - done
  )
  if errorlevel 1 (echo -- CEF: install failed & exit /b)
  cd "%DEV_DIR%"
  call :check_file %BOOST_FILENAME% %BOOST_SHA1% && (
    echo -- Boost: hash verified
    echo -- Boost: installing
    rmdir /q /s "%BOOST_ROOT%" >nul 2>nul
    %BOOST_FILENAME% /DIR="%BOOST_ROOT%" /SILENT /SP
    echo -- Boost: installing - done
  )
  if errorlevel 1 (exit /b)
  call :check_file %OPENSSL_FILENAME% %OPENSSL_SHA1% && (
    echo -- OpenSSL: hash verified
    echo -- OpenSSL: installing
    rmdir /q /s "%OPENSSL_ROOT_DIR%" >nul 2>nul
    %OPENSSL_FILENAME% /DIR=%OPENSSL_ROOT_DIR% /SILENT /SP /SUPPRESSMSGBOXES
    echo -- OpenSSL: installing - done
  )
  if errorlevel 1 (exit /b)
  goto :eof

:delete_downloads
  if exist "%QT_FILENAME%" (del "%QT_FILENAME%")
  if exist "%Deps_FILENAME%" (del "%Deps_FILENAME%")
  if exist "%X264_FILENAME%" (del "%X264_FILENAME%")
  if exist "%CURL_FILENAME%" (del "%CURL_FILENAME%")
  if exist "%VLC_FILENAME%" (del "%VLC_FILENAME%")
  if exist "%CEF_FILENAME%" (del "%CEF_FILENAME%")
  if exist "%BOOST_FILENAME%" (del "%BOOST_FILENAME%")
  if exist "%OPENSSL_FILENAME%" (del "%OPENSSL_FILENAME%")
  goto :eof

:build_webrtc
  if not exist "%WEBRTC_ROOT_DIR%\lib\webrtc.lib" (
    echo -- WebRTC: not found, building from source: this will take a while
    cd %OBSAGENTS_ROOT%
    call build-webrtc.cmd %BUILD_TYPE% %QWEBRTC%
    if errorlevel 1 (echo -- WebRTC: build failed & exit /b 1)
  ) else (echo -- WebRTC: already installed)
  goto :eof

:check_file
  setlocal EnableDelayedExpansion
  if not exist "%~1" (exit /b -1)
  if "%~2" NEQ "" (set "REF_HASH=%~2") else (echo -- %~1: ** no reference hash provided ** && exit /b 3)
  if "%~3" NEQ "" (set "ALGORITHM=%~3") else (set ALGORITHM=SHA1)
  for %%# in (certutil.exe) do (if not exist "%%~f$PATH:#" (echo %~1: ** error: cannot verify hash - no certutil installed ** && exit /b 4))
  set "HASH="
  for /f "skip=1 tokens=* delims=" %%# in ('certutil -hashfile "%~f1" %ALGORITHM%') do (
    if not defined HASH (for %%Z in (%%#) do set "HASH=!HASH1!%%Z")
  )
  if %HASH% EQU %REF_HASH% (exit /b 0) else (
    echo -- %~1: ** hash verification failed ** & exit /b 1
  )
  endlocal
  goto :eof

:elapsed_time
  setlocal EnableDelayedExpansion
  :: Get elapsed time:
  set "end=!endTime:%time:~8,1%=%%100)*100+1!" & set "start=!startTime:%time:~8,1%=%%100)*100+1!"
  set /A "elap=((((10!end:%time:~2,1%=%%100)*60+1!%%100)-((((10!start:%time:~2,1%=%%100)*60+1!%%100)"
  :: Convert elapsed time to HH:MM:SS:CC format:
  set /A "cc=elap%%100+100,elap/=100,ss=elap%%60+100,elap/=60,mm=elap%%60+100,hh=elap/60+100"
  :: Display elapsed time
  echo.
  echo -- BUILD TYPE: %BUILD_TYPE%
  echo.
  echo -- Start:      %startTime%
  echo -- End:        %endTime%
  echo -- Elapsed:    %hh:~1%%time:~2,1%%mm:~1%%time:~2,1%%ss:~1%%time:~8,1%%cc:~1%
  echo.
  endlocal
  goto :eof

:print_variables
  echo.
  echo -- BUILD_TYPE:         %BUILD_TYPE%
  echo -- OBS_TAG:            %OBS_TAG%
  echo.
  echo -- BUILD_ROOT:         %BUILD_ROOT%
  echo -- InstallPath64:      %InstallPath64%
  echo.
  echo -- MSVC_PATH:          %MSVC_PATH%
  echo -- MSVC_VERSION:       %MSVC_VERSION%
  echo.
  echo -- DepsPath64:         %DepsPath64%
  echo -- x264Path:           %x264Path%
  echo -- CURL_ROOT:          %CURL_ROOT%
  echo -- QTDIR64:            %QTDIR64%
  echo -- CEF_64:             %CEF_64%
  echo -- VLCPath:            %VLCPath%
  echo -- OPENSSL_ROOT_DIR:   %OPENSSL_ROOT_DIR%
  echo -- BOOST_ROOT:         %BOOST_ROOT%
  echo -- WEBRTC_ROOT_DIR:    %WEBRTC_ROOT_DIR%
  echo.
  echo -- BROWSER_PANEL:      %BROWSER_PANEL%
  echo.
  goto :eof

:clear_env
  set "build_config=" & set "BUILD_TYPE=" & set "BUILD_ROOT="
  set "MSVC_PATH=" & set "MSVC_VERSION=" & set "InstallPath64="
  set "MFC_CEF_APP_EXE_NAME=" & set "MFC_CEF_API_OBJECT=" & set "MFC_CEF_LOGIN_URL="
  set "MFC_API_SVR=" & set "MFC_FILE_SVR=" & set "MFC_DEFAULT_BROADCAST_URL="
  set "OBS_ROOT=" & set "DepsPath=" & set "DepsPath64=" & set "DepsPath32=" & set "Deps_VERSION="
  set "CURL_ROOT=" & set "CURL_INCLUDE_DIR=" & set "CURL_LIBRARY="
  set "VLCPath=" & set "VLC_VERSION="
  set "QTDIR=" & set "QTDIR64=" & set "QTDIR32=" & set "QT_VERSION="
  set "Qt5Core_DIR=" & set "Qt5Gui_DIR=" & set "Qt5MacExtras_DIR=" & set "Qt5Svg_DIR=" & set "Qt5Widgets_DIR="
  set "CEF=" & set "CEF_64=" & set "CEF_32=" & set "CEF_VERSION=" & set "CEF_VERSION_URL_ENCODED="
  set "BOOST_ROOT=" & set "BOOST_INCLUDEDIR=" & set "BOOST_LIBRARYDIR="
  set "BOOST_INCLUDE_DIR=" & set "BOOST_LIBRARY_DIR=" & set "BOOSTROOT=" & set "BOOST_VERSION="
  set "OPENSSL_ROOT_DIR=" & set "OPENSSL_INCLUDE_DIR=" & set "OPENSSL=" & set "OPENSSL_VERSION="
  set "WEBRTC=" & set "WEBRTC_ROOT_DIR=" & set "WEBRTC_INCLUDE_DIR=" & set "WEBRTC_LIB=" & set "WEBRTC_LIBRARY="
  set "BROWSER_PANEL="
  goto :eof

:skip_build
  echo.
  cd "%OBSAGENTS_ROOT%"
  echo *** Configuring + Generating completed %date% %time% ***
  echo.
  echo ** Run 'build resume' to compile
  set "endTime=%time: =0%"
  call :elapsed_time
  exit /b
  goto :eof

:abort
  endlocal
  cmd /c exit 1
