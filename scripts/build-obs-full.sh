#!/usr/bin/env bash

set -e # exit if something fails

readonly _OBS_TAG='26.1.2'
readonly _MAC_DEPLOYMENT_TARGET='10.13'

readonly _BUILD_DIR='xAuto'
readonly _BUILD_TYPE='RelWithDebInfo'
readonly _GENERATOR='Unix Makefiles'
declare -ri _RESET_OBS=1

readonly SIDEKICK_ROOT="$(pwd)"
readonly BUILD_DEPS="${SIDEKICK_ROOT}/scripts/build-deps-full.sh"

readonly _QT_VERSION=$(/usr/bin/sed -En "s/^readonly _QT_VERSION=['\"]?([0-9\.]+)['\"]?/\1/p" < "${BUILD_DEPS}")
readonly _VLC_VERSION=$(/usr/bin/sed -En "s/^readonly _VLC_VERSION=['\"]?([0-9\.]+)['\"]?/\1/p" < "${BUILD_DEPS}")
readonly _CEF_VERSION=$(/usr/bin/sed -En "s/^readonly _CEF_VERSION=['\"]?([0-9]+\.[0-9]+\.[0-9]+\+[a-z0-9]{8}\+chromium-[0-9]+\.[0-9]+\.[0-9]{4}\.[0-9]+)['\"]?/\1/p" < "${BUILD_DEPS}")
declare -ri MACOS_CEF_BUILD_VERSION=$(echo "${_CEF_VERSION}" | /usr/bin/sed -En "s/[0-9]+\.[0-9]+\.[0-9]+\+[a-z0-9]{8}\+chromium-[0-9]+\.[0-9]+\.([0-9]{4})\.[0-9]+/\1/p")

declare -xr MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-${_MAC_DEPLOYMENT_TARGET}}
declare -xr CMAKE_BUILD_TYPE=${BUILD_TYPE:-${_BUILD_TYPE}}
declare -xr BUILD_TYPE=${CMAKE_BUILD_TYPE}
readonly _NUM_CORES=$(sysctl -n hw.ncpu)
declare -xri NUM_CORES=${_NUM_CORES}
declare -xri CMAKE_BUILD_PARALLEL_LEVEL=${NUM_CORES}
declare -xr CMAKE_GENERATOR=${CMAKE_GENERATOR:-${_GENERATOR}}
export GENERATOR=${GENERATOR:-${CMAKE_GENERATOR}}
export NINJA_PATH="${DEV_DIR}/depot_tools/ninja"

declare -xr BUILD_BROWSER=${BROWSER:-ON}
declare -xr BROWSER=${BUILD_BROWSER}
declare -xr ENABLE_SCRIPTING=${SCRIPTING:-ON}
declare -xr DISABLE_PYTHON=${DISABLE_PYTHON:-ON}

export OBS_TAG=${OBS_TAG:-${_OBS_TAG}}
declare -xr QT_VERSION=${QT_VERSION:-${_QT_VERSION}}
declare -xr VLC_VERSION=${VLC_VERSION:-${_VLC_VERSION}}
declare -xr CEF_VERSION=${CEF_VERSION:-${_CEF_VERSION}}
declare -xr CEF_BUILD_VERSION=${CEF_BUILD_VERSION:-${CEF_VERSION}}
readonly LEGACY_BROWSER="$(test "${MACOS_CEF_BUILD_VERSION}" -le 3770 && echo "ON" || echo "OFF")"
declare -ri REFRESH_OBS=${REFRESH_OBS:=${_RESET_OBS}}
declare -ri RESET_OBS=${RESET_OBS:=${REFRESH_OBS}}

readonly HOST_ARCH=$(uname -m)
readonly HOMEBREW_PREFIX=$(test "${HOST_ARCH}" = "arm64" && echo "/opt/homebrew" || echo "/usr/local")
readonly CEF_ARCH=$(test "${HOST_ARCH}" = "arm64" && echo "arm64" || echo "x64")
#declare -xr CMAKE_OSX_ARCHITECTURES=arm64;x86_64
declare -xr CMAKE_OSX_ARCHITECTURES="${HOST_ARCH}"

# readonly SIDEKICK_ROOT="$(pwd)"
cd ../../.. || exit
readonly OBS_ROOT="$(pwd)"
cd .. || exit
readonly DEV_DIR="${DEV_DIR:-$(pwd)}"

readonly BUILD_DIR="${BUILD_DIR:-${_BUILD_DIR}}"
readonly BUILD_ROOT="${OBS_ROOT}/${BUILD_DIR}"

declare -xr OBSDEPS="${OBSDEPS:-${DEV_DIR}/obsdeps}"
declare -xr DepsPath="${OBSDEPS}"
declare -xr X264_INCLUDE_DIR="${X264_INCLUDE_DIR:-${OBSDEPS}/include}"
# declare -xr CURL_INCLUDE_DIR="${CURL_INCLUDE_DIR:-/usr/include}"
declare -xr CURL_INCLUDE_DIR="${CURL_INCLUDE_DIR:-${HOMEBREW_PREFIX}/opt/curl/include}"
declare -xr VLCPath="${DEV_DIR}/vlc-${VLC_VERSION}"
# declare -xr QTDIR="${QTDIR:-${HOMEBREW_PREFIX}/opt/qt}"
declare -xr QTDIR="${QTDIR:-${OBSDEPS}}"
readonly _CEF_DIR="${CEF:-${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}}"
declare -xr CEF_ROOT="${CEF_ROOT:-${_CEF_DIR}}"
declare -xr CEF_ROOT_DIR="${CEF_ROOT_DIR:-${CEF_ROOT}}"
declare -xr BOOST_ROOT="${BOOST_ROOT:-${HOMEBREW_PREFIX}/opt/boost}"
readonly _OPENSSL_DIR="${OPENSSL:-${HOMEBREW_PREFIX}/opt/openssl@1.1}"
declare -xr OPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR:-${_OPENSSL_DIR}}"
readonly _WEBRTC_DIR="${WEBRTC:-${DEV_DIR}/webrtc}"
declare -xr WEBRTC_ROOT_DIR="${WEBRTC_ROOT_DIR:-${_WEBRTC_DIR}}"

readonly PREV_CFLAGS="${CFLAGS}"
readonly PREV_CXXFLAGS="${CXXFLAGS}"
readonly _CFLAGS="-Wno-unused-variable -Wno-unused-parameter \
  -Wno-typedef-redefinition -Wno-enum-conversion -Wno-deprecated \
  -Wno-unused-private-field -Wno-sign-compare -Wno-vla"
readonly _CXXFLAGS="-Wno-pragmas -Wno-deprecated-declarations"

readonly red=$'\e[1;31m'
readonly grn=$'\e[1;32m'
readonly blu=$'\e[1;34m'
readonly mag=$'\e[1;35m'
readonly cyn=$'\e[1;36m'
readonly bold=$'\e[1m'
readonly reset=$'\e[0m'

declare start
declare end
declare -i start_ts
declare -i end_ts
declare -i runtime
declare -i hours
declare -i minutes
declare -i seconds

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

print_env() {
  echo
  echo "${bold}Building           Sidekick${reset}"
  echo "${red}BUILD_TYPE:        ${BUILD_TYPE}${reset}"
  echo "GENERATOR:         ${GENERATOR}"
  echo "OBS_TAG:           ${OBS_TAG}"
  echo
  echo "SIDEKICK_ROOT:     ${SIDEKICK_ROOT}"
  echo "BUILD_ROOT:        ${BUILD_ROOT}"
  echo "OBS_ROOT:          ${OBS_ROOT}"
  echo "DEV_DIR:           ${DEV_DIR}"
  echo
  echo "DepsPath:          ${DepsPath}"
  echo "CURL_INCLUDE_DIR:  ${CURL_INCLUDE_DIR}"
  echo "X264_INCLUDE_DIR:  ${X264_INCLUDE_DIR}"
  echo "VLCPath:           ${VLCPath}"
  echo "QTDIR:             ${QTDIR}"
  echo "CEF_ROOT_DIR:      ${CEF_ROOT_DIR}"
  echo "BOOST_ROOT:        ${BOOST_ROOT}"
  echo "OPENSSL_ROOT_DIR:  ${OPENSSL_ROOT_DIR}"
  echo "WEBRTC_ROOT_DIR:   ${WEBRTC_ROOT_DIR}"
  echo "RESET OBS:         ${RESET_OBS}"
  echo
}

print_start() {
  start_ts=$(date +%s)
  start="$(date '+%Y-%m-%d %H:%M:%S')"
  hr "Build Started:     ${start}"
}

install_ninja() {
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

install_dir_prep() {
  sudo rm -rf "/Library/Application Support/obs-studio/sidekick"
  sudo mkdir -p "/Library/Application Support/obs-studio/sidekick"
  sudo chmod 775 "/Library/Application Support/obs-studio/sidekick"
}

build_dependencies() {
  cd "${SIDEKICK_ROOT}"
  ./build-deps skip_build_tools
}

reset_obs() {
  if [ ${RESET_OBS} -eq 1 ]; then
    echo "-- obs-studio hard reset - initiating"
    cd "${OBS_ROOT}"
    rm -rf ./plugins/enc-amf > /dev/null
    git reset --hard > /dev/null
    git submodule foreach git reset --hard > /dev/null
    git fetch origin > /dev/null
    git checkout ${OBS_TAG} > /dev/null
    git submodule update --init --recursive > /dev/null
    git reset --hard > /dev/null
    echo "-- obs-studio hard reset - done"
  else
    echo "-- obs-studio hard reset - skipping"
  fi
}

apply_patches() {
  cd "${OBS_ROOT}"
  for f in "${SIDEKICK_ROOT}/patches/mac/bash/"*.patch; do
    echo "-- applying patch $f"
    git apply -p1 --ignore-whitespace --whitespace=nowarn "$f"
    if [ $? -eq 0 ]; then
      echo "-- applying patch $f - success"
    else
      git apply -p1 -R --check --ignore-whitespace --whitespace=nowarn "$f"
      if [ $? -eq 0 ]; then
        echo "-- applying patch $f - already applied"
      else
        echo "-- applying patch $f - error"
      fi
    fi
  done
}

edit_cmakelists() {
  # Append add_subdirectory(plugins/MyFreeCams) to ${OBS_ROOT}/CMakeLists.txt
  # echo "-- Editing ${OBS_ROOT}/CMakeLists.txt"
  # if grep -Fxq "add_subdirectory(plugins/MyFreeCams)" "${OBS_ROOT}/CMakeLists.txt"; then
  #   echo "-- Editing ${OBS_ROOT}/CMakeLists.txt - already modified"
  # else
  #   sed -i '' -e '/add_subdirectory(libobs)/a\'$'\n''add_subdirectory(plugins/MyFreeCams)'$'\n''' "${OBS_ROOT}/CMakeLists.txt"
  #   echo "-- Editing ${OBS_ROOT}/CMakeLists.txt - success"
  # fi

  # Append add_subdirectory(MyFreeCams) to ${OBS_ROOT}/plugins/CMakeLists.txt
  echo "-- Editing ${OBS_ROOT}/plugins/CMakeLists.txt"
  if grep -Fxq "add_subdirectory(MyFreeCams)" "${OBS_ROOT}/plugins/CMakeLists.txt"; then
    echo "-- Editing ${OBS_ROOT}/plugins/CMakeLists.txt - already modified"
  else
    echo -e "add_subdirectory(MyFreeCams)\n" >> "${OBS_ROOT}/plugins/CMakeLists.txt"
    echo "-- Editing ${OBS_ROOT}/plugins/CMakeLists.txt - success"
  fi

  # Create CMakeLists.txt in ${OBS_ROOT}/plugins/MyFreeCams
  echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt"
  if [ -f "${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt" ]; then
    echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt - file exists"
  else
    echo -e "add_subdirectory(Sidekick)\n" > "${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt"
    echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt - success"
  fi
}

cmake_generate() {
  if [ -d "${BUILD_ROOT}" ]; then
    rm -rf "${BUILD_DIR}"
    # mv "${BUILD_DIR}" "${BUILD_DIR}-$(date +'%Y%m%d_%H%M%S')"
  fi
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}"
  # export CFLAGS="${CFLAGS} -I${OBSDEPS}/include"
  export LDFLAGS="${LDFLAGS} -L${OBSDEPS}/lib"
  export PKG_CONFIG_PATH="${OBSDEPS}/lib/pkgconfig:${PKG_CONFIG_PATH}"
  export CFLAGS="${CFLAGS} ${_CFLAGS} -I${OBSDEPS}/include"
  export CXXFLAGS="${CXXFLAGS} ${_CXXFLAGS}"
  cmake \
    -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL} \
    -DCMAKE_OSX_ARCHITECTURES="${CMAKE_OSX_ARCHITECTURES}" \
    -DPROJECT_ARCH="${CMAKE_OSX_ARCHITECTURES}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
    -DDepsPath="${OBSDEPS}" \
    -DSWIGDIR="${OBSDEPS}" \
    -DX264_INCLUDE_DIR="${X264_INCLUDE_DIR}" \
    -DCURL_INCLUDE_DIR="${CURL_INCLUDE_DIR}" \
    -DVLCPath="${VLCPath}" \
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
    -DBROWSER_LEGACY="${LEGACY_BROWSER}" \
    -DENABLE_VLC=ON \
    -DWITH_RTMPS=ON \
    -DDISABLE_PYTHON="${DISABLE_PYTHON}" \
    -DENABLE_SCRIPTING="${ENABLE_SCRIPTING}" \
    ..
}

print_summary() {
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
  export CFLAGS="${PREV_CFLAGS}"
  export CXXFLAGS="${PREV_CXXFLAGS}"
}

main() {
  print_env
  install_dir_prep
  print_start

  if [ "${GENERATOR}" = "Ninja" ]; then install_ninja; fi

  # build_dependencies
  reset_obs
  # apply_patches
  edit_cmakelists
  cmake_generate
  cmake --build . --config ${BUILD_TYPE}
  print_summary
}

main "$@"
