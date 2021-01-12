#!/usr/bin/env bash
set -e # exit if something fails

readonly _QT_VERSION=5.10.1
readonly _CEF_VERSION=75.1.14+gc81164e+chromium-75.0.3770.100
readonly _BOOST_VERSION=1.69.0
readonly _OPENSSL_VERSION=1.1.1

readonly _MACOSX_DEPLOYMENT_TARGET=10.12
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-${_MACOSX_DEPLOYMENT_TARGET}}
readonly _BUILD_TYPE=Release
export CMAKE_BUILD_TYPE=${BUILD_TYPE:-${_BUILD_TYPE}}
export BUILD_TYPE=${CMAKE_BUILD_TYPE}

export QT_VERSION=${QT_VERSION:-${_QT_VERSION}}
export CEF_VERSION=${CEF_VERSION:-${_CEF_VERSION}}
export CEF_BUILD_VERSION=${CEF_BUILD_VERSION:-${CEF_VERSION}}
export BOOST_VERSION=${BOOST_VERSION:-${_BOOST_VERSION}}
export OPENSSL_VERSION=${OPENSSL_VERSION:-${_OPENSSL_VERSION}}

_NUM_CORES="$(sysctl -n hw.logicalcpu)"
export NUM_CORES=${_NUM_CORES}
export CMAKE_BUILD_PARALLEL_LEVEL=${NUM_CORES}

_SIDEKICK_ROOT="$(pwd)"
export SIDEKICK_ROOT=${_SIDEKICK_ROOT}
cd ../../..
_OBS_ROOT="$(pwd)"
export OBS_ROOT=${_OBS_ROOT}
cd ..
readonly _DEV_DIR="$(pwd)"
export DEV_DIR="${DEV_DIR:-${_DEV_DIR}}"
mkdir -p "${DEV_DIR}"

red=$'\e[1;31m'
# grn=$'\e[1;32m'
# blu=$'\e[1;34m'
# mag=$'\e[1;35m'
# cyn=$'\e[1;36m'
bold=$'\e[1m'
reset=$'\e[0m'

exists() {
  command -v "$1" >/dev/null 2>&1
}

hr() {
  echo "────────────────────────────────────────────────────────────────"
  echo "$1"
  [ -n "$2" ] && echo "$2"
  [ -n "$3" ] && printf "$3" "$4" "$5" "$6"
  echo "────────────────────────────────────────────────────────────────"
}

install_or_upgrade() {
  if brew ls --versions "$1" >/dev/null; then
    HOMEBREW_NO_AUTO_UPDATE=1 brew upgrade "$1"
  else
    HOMEBREW_NO_AUTO_UPDATE=1 brew install "$1"
  fi
}

install_build_tools() {
  hr "Installing build tools"
  set +e
  install_or_upgrade libtool
  install_or_upgrade automake
  install_or_upgrade pcre
  install_or_upgrade cmake
  install_or_upgrade freetype
  install_or_upgrade nasm
  install_or_upgrade pkg-config
  set -e
}

main() {
  echo "${red}BUILD_TYPE:        ${BUILD_TYPE}${reset}"
  echo "SIDEKICK_ROOT:    ${SIDEKICK_ROOT}"
  echo "OBS_ROOT:          ${OBS_ROOT}"
  echo "DEV_DIR:           ${DEV_DIR}"
  cd "${DEV_DIR}"

  if [ "$1" = "clean" ]; then
    hr "Uninstalling homebrew"
    set +e
    curl -o uninstall.sh -fsSL https://raw.githubusercontent.com/Homebrew/install/master/uninstall.sh
    /bin/bash uninstall.sh --force
    rm uninstall.sh
    set -e
  fi

  start=$(date '+%Y-%m-%d %H:%M:%S')
  start_ts=$(date +%s)
  hr "Building dependencies in: ${DEV_DIR}" "Started ${start}"

  if ! exists brew; then
    hr "Installing Homebrew"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
  fi

  if [ "$1" != "skip_build_tools" ]; then install_build_tools; fi

  # Core OBS Deps
  install_or_upgrade curl-openssl
  install_or_upgrade mbedtls
  install_or_upgrade openssl@1.1

  # x264
  if [ -f "/usr/local/opt/x264/include/x264.h" ]; then
    hr "x264 already installed"
  else
    hr "Installing x264"
    brew install "$SIDEKICK_ROOT/scripts/homebrew/x264.rb"
  fi

  # FFmpeg
  if [ -f "/usr/local/opt/ffmpeg/include/libavcodec/version.h" ]; then
    hr "FFmpeg already installed"
  else
    hr "Installing FFmpeg"
    brew install "$SIDEKICK_ROOT/scripts/homebrew/ffmpeg.rb"
  fi

  # Qt 5.10.1
  if [ "${QT_VERSION}" = "5.10.1" ]; then
    if [ -f "/usr/local/opt/qt/lib/cmake/Qt5/Qt5Config.cmake" ]; then
      QTV=$(brew ls --versions qt | grep -E -o "5\.[0-9]+\.[0-9]+")
      if [ "${QTV}" = "5.10.1" ]; then
        hr "Qt ${QT_VERSION} already installed"
      else
        hr "${red}Qt ${QTV} is installed.${reset}${bold} Please uninstall 'brew uninstall qt' before running this script${reset}"
        exit 1
      fi
    else
      hr "Installing Qt ${QT_VERSION}"
      brew install "$SIDEKICK_ROOT/scripts/homebrew/qt_5_10/qt.rb"
    fi
  fi

  # Qt 5.14.1
  if [ "${QT_VERSION}" = "5.14.1" ]; then
    if [ -f "/usr/local/opt/qt/lib/cmake/Qt5/Qt5Config.cmake" ]; then
      QTV=$(brew ls --versions qt | grep -E -o "5\.[0-9]+\.[0-9]+")
      if [ "${QTV}" = "5.14.1" ]; then
        hr "Qt ${QT_VERSION} already installed"
      else
        hr "${red}Qt ${QTV} is installed.${reset}${bold} Please uninstall 'brew uninstall qt' before running this script${reset}"
        exit 1
      fi
    else
      hr "Installing Qt ${QT_VERSION}"
      brew install "$SIDEKICK_ROOT/scripts/homebrew/qt_5_14/qt.rb"
    fi
  fi

  # Boost
  if [ -f "/usr/local/opt/boost/include/boost/version.hpp" ]; then
    hr "Boost ${BOOST_VERSION} already installed"
  else
    hr "Installing Boost ${BOOST_VERSION}"
    brew install "$SIDEKICK_ROOT/scripts/homebrew/boost.rb"
  fi

  set -e # exit if something fails

  # Packages app (http://s.sudre.free.fr/Software/Packages/about.html)
  if [ -d "/Applications/Packages.app" ] && exists packagesbuild; then
    hr "Packages app already installed"
  else
    hr "Installing Packages app"
    cd "${DEV_DIR}"
    curl -fRL -O http://s.sudre.free.fr/Software/files/Packages.dmg
    hdiutil attach Packages.dmg
    sudo installer -pkg /Volumes/Packages*/packages/Packages.pkg -target /
    hdiutil detach /Volumes/Packages*
    rm "${DEV_DIR}/Packages.dmg"
  fi

  # CEF
  if [ -f "${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macosx64/cmake/FindCEF.cmake" ]; then
    hr "CEF ${CEF_VERSION} already installed"
  else
    hr "Installing CEF ${CEF_VERSION}"
    cd "${DEV_DIR}"
    rm -rf "cef_binary_${CEF_BUILD_VERSION}_macosx64"
    if [ "${BUILD_TYPE}" = "Debug" ]; then CEF_BUILD_TYPE=Debug; else CEF_BUILD_TYPE=Release; fi
    CEF_VERSION_ENCODED=${CEF_BUILD_VERSION//+/%2B}
    curl -fRL -o "cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2" \
      "http://opensource.spotify.com/cefbuilds/cef_binary_${CEF_VERSION_ENCODED}_macosx64.tar.bz2"
    tar -xf "cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2"
    cd "${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macosx64"
    rm -rf tests
    sed -i '' 's/\"10.9\"/\"10.12\"/' ./cmake/cef_variables.cmake
    mkdir -p build
    cd ./build
    cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} -DCMAKE_BUILD_TYPE=${CEF_BUILD_TYPE} ..
    make -j${NUM_CORES}
    mkdir -p libcef_dll
    rm "${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2"
  fi

  # WebRTC
  if [ -f "${DEV_DIR}/webrtc/lib/libwebrtc.a" ]; then
    hr "WebRTC library found: ${DEV_DIR}/webrtc"
  else
    hr "Building WebRTC"
    cd "${SIDEKICK_ROOT}"
    ./build-webrtc
  fi

  end=$(date '+%Y-%m-%d %H:%M:%S')
  end_ts=$(date +%s)
  runtime=$((end_ts-start_ts))
  hours=$((runtime / 3600))
  minutes=$(( (runtime % 3600) / 60 ))
  seconds=$(( (runtime % 3600) % 60 ))

  hr \
    "Start:    ${start}" \
    "End:      ${end}" \
    "Elapsed:  (hh:mm:ss) %02d:%02d:%02d\n" \
    ${hours} ${minutes} ${seconds}
}

main "$@"
