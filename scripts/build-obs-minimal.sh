#!/usr/bin/env bash

readonly _BUILD_TYPE=RelWithDebInfo
readonly _BUILD_DIR=xAuto
readonly _GENERATOR="Unix Makefiles"
readonly _RESET_OBS=1

readonly _OBS_TAG=26.1.0-rc2
readonly _CEF_VERSION=75.1.14+gc81164e+chromium-75.0.3770.100

readonly _CFLAGS="-Wno-unused-variable -Wno-unused-parameter \
  -Wno-typedef-redefinition -Wno-enum-conversion -Wno-deprecated \
  -Wno-unused-private-field -Wno-sign-compare -Wno-vla"
readonly _CXXFLAGS="${CFLAGS} -Wno-pragmas"
export MACOSX_DEPLOYMENT_TARGET=10.12
readonly __BUILD_TYPE=${1:-${_BUILD_TYPE}}
export CMAKE_BUILD_TYPE=${BUILD_TYPE:-${__BUILD_TYPE}}
export BUILD_TYPE=${CMAKE_BUILD_TYPE}
_NUM_CORES="$(sysctl -n hw.logicalcpu)"
declare -xri NUM_CORES=${_NUM_CORES}
export CMAKE_BUILD_PARALLEL_LEVEL=${NUM_CORES}
export CMAKE_GENERATOR=${CMAKE_GENERATOR:-${_GENERATOR}}
export GENERATOR=${GENERATOR:-${CMAKE_GENERATOR}}
export NINJA_PATH="${DEV_DIR}/depot_tools/ninja"

export BUILD_BROWSER=${BROWSER:-ON}
export OBS_TAG=${OBS_TAG:-${_OBS_TAG}}
export CEF_VERSION=${CEF_VERSION:-${_CEF_VERSION}}
export CEF_BUILD_VERSION=${CEF_BUILD_VERSION:-${CEF_VERSION}}
REFRESH_OBS=${REFRESH_OBS:=${_RESET_OBS}}
export RESET_OBS=${RESET_OBS:=${REFRESH_OBS}}

readonly _OBSAGENTS_ROOT="$(pwd)"
export OBSAGENTS_ROOT=${_OBSAGENTS_ROOT}
cd ../../.. || exit
readonly _OBS_ROOT="$(pwd)"
export OBS_ROOT=${_OBS_ROOT}
cd .. || exit
readonly _DEV_DIR="$(pwd)"
export DEV_DIR="${DEV_DIR:-${_DEV_DIR}}"

export BUILD_DIR="${BUILD_DIR:-${_BUILD_DIR}}"
export BUILD_ROOT="${OBS_ROOT}/${BUILD_DIR}"

export CURL_INCLUDE_DIR="${CURL_INCLUDE_DIR:-/usr/local/opt/curl-openssl/include}"
# export CURL_INCLUDE_DIR="${CURL_INCLUDE_DIR:-/usr/include}"
export QTDIR="${QTDIR:-/usr/local/opt/qt}"
readonly _CEF_DIR="${CEF:-${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macosx64}"
export CEF_ROOT="${CEF_ROOT:-${_CEF_DIR}}"
export CEF_ROOT_DIR="${CEF_ROOT_DIR:-${CEF_ROOT}}"
export BOOST_ROOT="${BOOST_ROOT:-/usr/local/opt/boost}"
readonly _OPENSSL_DIR="${OPENSSL:-/usr/local/opt/openssl@1.1}"
export OPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR:-${_OPENSSL_DIR}}"
readonly _WEBRTC_DIR="${WEBRTC:-${DEV_DIR}/webrtc}"
export WEBRTC_ROOT_DIR="${WEBRTC_ROOT_DIR:-${_WEBRTC_DIR}}"

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

install_ninja() {
  export CFLAGS="${CFLAGS} ${_CFLAGS}"
  export CXXFLAGS=${_CXXFLAGS}
  if ! exists ninja; then
    echo "Ninja not found in PATH"
    if [ -f "${NINJA_PATH}" ]; then
      echo "Ninja found: ${NINJA_PATH}. Adding to PATH"
      _DEPOT_TOOLS=$(dirname "${NINJA_PATH}")
      export DEPOT_TOOLS=${_DEPOT_TOOLS}
      export PATH="${DEPOT_TOOLS}:${PATH}"
    else
      echo "Ninja not found. Installing from brew"
      brew install ninja
    fi
  else
    echo "Ninja found in PATH"
  fi
}

main() {
  if [ "${GENERATOR}" = "Ninja" ]; then
    export CFLAGS=${_CFLAGS}
    export CXXFLAGS=${_CXXFLAGS}
    if ! exists ninja; then
      echo "Ninja not found in PATH"
      if [ -f "${NINJA_PATH}" ]; then
        echo "Ninja found: ${NINJA_PATH}. Adding to PATH"
        _DEPOT_TOOLS=$(dirname "${NINJA_PATH}")
        export DEPOT_TOOLS=${_DEPOT_TOOLS}
        export PATH="${DEPOT_TOOLS}:${PATH}"
      else
        echo "Ninja not found. Installing from brew"
        brew install ninja
      fi
    else
      echo "Ninja found in PATH"
    fi
  fi

  echo
  echo "${bold}Building           Sidekick${reset}"
  echo "${red}BUILD_TYPE:        ${BUILD_TYPE}${reset}"
  echo "GENERATOR:         ${GENERATOR}"
  echo "OBS_TAG:           ${OBS_TAG}"
  echo
  echo "OBSAGENTS_ROOT:    ${OBSAGENTS_ROOT}"
  echo "BUILD_ROOT:        ${BUILD_ROOT}"
  echo "OBS_ROOT:          ${OBS_ROOT}"
  echo "DEV_DIR:           ${DEV_DIR}"
  echo
  echo "CURL_INCLUDE_DIR:  ${CURL_INCLUDE_DIR}"
  echo "QTDIR:             ${QTDIR}"
  echo "CEF_ROOT_DIR:      ${CEF_ROOT_DIR}"
  echo "BOOST_ROOT:        ${BOOST_ROOT}"
  echo "OPENSSL_ROOT_DIR:  ${OPENSSL_ROOT_DIR}"
  echo "WEBRTC_ROOT_DIR:   ${WEBRTC_ROOT_DIR}"
  echo "RESET OBS:         ${RESET_OBS}"
  echo

  set -e # exit if something fails
  sudo rm -rf "/Library/Application Support/obs-studio/sidekick"
  sudo mkdir -p "/Library/Application Support/obs-studio/sidekick"
  sudo chmod 775 "/Library/Application Support/obs-studio/sidekick"

  start_ts=$(date +%s)
  start="$(date '+%Y-%m-%d %H:%M:%S')"
  hr "Build Started:     ${start}"

  if [ "${GENERATOR}" = "Ninja" ]; then install_ninja; fi

  cd "${OBSAGENTS_ROOT}"
  "${OBSAGENTS_ROOT}/scripts/build-deps-minimal.sh" skip_build_tools

  cd "${OBS_ROOT}"
  if [ ${RESET_OBS} -eq 1 ]; then
    echo "-- obs-studio hard reset - initiating"
    rm -rf ./plugins/enc-amf > /dev/null
    git reset --hard > /dev/null
    git submodule foreach git reset --hard > /dev/null
    # if [ "${CURRENT_XCODE}" = true ]; then
    #   hr "Checking out obs-studio pull request #2264"
    #   git fetch origin +refs/pull/2264/merge > /dev/null
    #   OBS_TAG=FETCH_HEAD
    # fi
    git fetch origin > /dev/null
    git checkout ${OBS_TAG} > /dev/null
    git submodule update --init --recursive > /dev/null
    git reset --hard > /dev/null
    echo "-- obs-studio hard reset - done"
  else
    echo "-- obs-studio hard reset - skipping"
  fi

  # Append add_subdirectory(plugins/MyFreeCams) to ${OBS_ROOT}/CMakeLists.txt
  echo "-- Editing ${OBS_ROOT}/CMakeLists.txt"
  if grep -Fxq "add_subdirectory(plugins/MyFreeCams)" "${OBS_ROOT}/CMakeLists.txt"; then
    echo "-- Editing ${OBS_ROOT}/CMakeLists.txt - already modified"
  else
    sed -i '' -e '/add_subdirectory(libobs)/a\'$'\n''add_subdirectory(plugins/MyFreeCams)'$'\n''' "${OBS_ROOT}/CMakeLists.txt"
    echo "-- Editing ${OBS_ROOT}/CMakeLists.txt - success"
  fi

  # Create CMakeLists.txt in ${OBS_ROOT}/plugins/MyFreeCams
  echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt"
  if [ -f "${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt" ]; then
    echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt - file exists"
  else
    echo -e "add_subdirectory(Sidekick)\n" > "${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt"
    echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt - success"
  fi

  cd "${OBS_ROOT}"
  if [ -d "${BUILD_ROOT}" ]; then
    mv "${BUILD_DIR}" "${BUILD_DIR}-$(date +'%Y%m%d_%H%M%S')"
  fi
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}"

  cmake \
    -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL} \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
    -DCURL_INCLUDE_DIR="${CURL_INCLUDE_DIR}" \
    -DQTDIR="${QTDIR}" \
    -DQt5Core_DIR="${QTDIR}/lib/cmake/Qt5Core" \
    -DQt5Gui_DIR="${QTDIR}/lib/cmake/Qt5Gui" \
    -DQt5MacExtras_DIR="${QTDIR}/lib/cmake/Qt5MacExtras" \
    -DQt5Svg_DIR="${QTDIR}/lib/cmake/Qt5Svg" \
    -DQt5Widgets_DIR="${QTDIR}/lib/cmake/Qt5Widgets" \
    -DCEF_ROOT="${CEF_ROOT}" \
    -DCEF_ROOT_DIR="${CEF_ROOT_DIR}" \
    -DBOOST_ROOT="${BOOST_ROOT}" \
    -DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}" \
    -DWEBRTC_ROOT_DIR="${WEBRTC_ROOT_DIR}" \
    -DBUILD_BROWSER="${BUILD_BROWSER}" \
    -DBROWSER_DEPLOY="${BUILD_BROWSER}" \
    -DDISABLE_PYTHON=ON \
    -DENABLE_SCRIPTING=OFF \
    -DBUILD_REDISTRIBUTABLE=ON \
    ..

  cmake --build . --config ${BUILD_TYPE}

  end=$(date '+%Y-%m-%d %H:%M:%S')
  end_ts=$(date +%s)
  runtime=$((end_ts-start_ts))
  hours=$((runtime / 3600))
  minutes=$(( (runtime % 3600) / 60 ))
  seconds=$(( (runtime % 3600) % 60 ))

  echo
  echo   "BUILD_TYPE:      ${BUILD_TYPE}"
  echo
  echo   "Start:           ${start}"
  echo   "End:             ${end}"
  printf "Elapsed:         (hh:mm:ss) %02d:%02d:%02d\n" ${hours} ${minutes} ${seconds}
}

main "$@"
