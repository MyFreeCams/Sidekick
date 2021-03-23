#!/bin/bash

set -e # exit if something fails

readonly _MACOSX_DEPLOYMENT_TARGET='10.13'
readonly _VLC_VERSION='3.0.8'
readonly _QT_VERSION='5.15.2'
# readonly _CEF_VERSION='75.1.14+gc81164e+chromium-75.0.3770.100'
# readonly _CEF_VERSION='85.0.0+g93b66a0+chromium-85.0.4183.121'
# readonly _CEF_VERSION='85.3.12+g3e94ebf+chromium-85.0.4183.121'
# readonly _CEF_VERSION='88.2.9+g5c8711a+chromium-88.0.4324.182'
readonly _CEF_VERSION='89.0.12+g2b76680+chromium-89.0.4389.90'
# readonly MACOS_CEF_BUILD_VERSION='3770'
# readonly MACOS_CEF_BUILD_VERSION='4183'
# readonly MACOS_CEF_BUILD_VERSION='4324'
readonly MACOS_CEF_BUILD_VERSION='4389'
readonly _BOOST_VERSION='1.69.0'
readonly _OPENSSL_VERSION='1.1.1'

readonly OPUS_VERSION='1.3.1'
# readonly OGG_VERSION='1.3.4'
readonly OGG_VERSION='68ca3841567247ac1f7850801a164f58738d8df9'
readonly VORBIS_VERSION='1.3.6'
readonly VPX_VERSION='1.9.0'
readonly X264_COMMIT='origin/stable'
readonly FFMPEG_VERSION='4.2.3'
export FFMPEG_REVISION='06'
readonly PNG_VERSION='1.6.37'
readonly THEORA_VERSION='1.1.1'
readonly LAME_VERSION='3.100'
readonly MBEDTLS_VERSION='2.24.0'
readonly SRT_VERSION='1.4.1'
readonly SWIG_VERSION='4.0.2'
readonly PCRE_VERSION='8.44'
readonly SPEEXDSP_VERSION='1.2.0'
readonly JANSSON_VERSION='2.13.1'
readonly LUAJIT_VERSION='2.1'
readonly LUAJIT_COMMIT='ec6edc5c39c25e4eb3fca51b753f9995e97215da'
readonly FREETYPE_VERSION='2.10.4'
readonly RNNOISE_COMMIT='90ec41ef659fd82cfec2103e9bb7fc235e9ea66c'
# readonly SPARKLE_VERSION='1.23.0'

export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-${_MACOSX_DEPLOYMENT_TARGET}}"
declare -xr CMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}"
declare -xr BUILD_TYPE="${CMAKE_BUILD_TYPE}"

readonly MACOS_VERSION=$(/usr/bin/sw_vers -productVersion)
readonly MACOS_MAJOR=$(/bin/echo ${MACOS_VERSION} | /usr/bin/cut -d '.' -f 1)
readonly MACOS_MINOR=$(/bin/echo ${MACOS_VERSION} | /usr/bin/cut -d '.' -f 2)
# export CFLAGS="${CFLAGS} -arch=arm64 -arch=x86_64"

declare -xr VLC_VERSION="${VLC_VERSION:-${_VLC_VERSION}}"
declare -xr QT_VERSION="${QT_VERSION:-${_QT_VERSION}}"
declare -xr CEF_VERSION="${CEF_VERSION:-${_CEF_VERSION}}"
declare -xr CEF_BUILD_VERSION="${CEF_BUILD_VERSION:-${CEF_VERSION}}"
declare -xr BOOST_VERSION="${BOOST_VERSION:-${_BOOST_VERSION}}"
declare -xr OPENSSL_VERSION="${OPENSSL_VERSION:-${_OPENSSL_VERSION}}"

declare -i SKIP_BUILD_TOOLS=0
if [ "$1" == "skip_build_tools" ] || [ "$2" == "skip_build_tools" ]; then
  SKIP_BUILD_TOOLS=1
fi

declare -i NUM_CORES
NUM_CORES=$(sysctl -n hw.ncpu)
declare -xri CMAKE_BUILD_PARALLEL_LEVEL=${NUM_CORES}

readonly SIDEKICK_ROOT="$(pwd)"
cd ../../..
readonly OBS_ROOT="$(pwd)"
cd ..
readonly DEV_DIR="${DEV_DIR:-$(pwd)}"
readonly WORK_DIR="${WORK_DIR:-${DEV_DIR}/obsdeps-src}"
readonly OBSDEPS="${OBSDEPS:-${DEV_DIR}/obsdeps}"

readonly HOST_ARCH=$(uname -m)
readonly HOMEBREW_PREFIX=$(test "${HOST_ARCH}" = "arm64" && echo "/opt/homebrew" || echo "/usr/local")
readonly CEF_ARCH=$(test "${HOST_ARCH}" = "arm64" && echo "arm64" || echo "x64")
#declare -xr CMAKE_OSX_ARCHITECTURES=arm64;x86_64
declare -xr CMAKE_OSX_ARCHITECTURES="${HOST_ARCH}"

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
  /bin/echo "────────────────────────────────────────────────────────────────"
  /bin/echo "$1"
  [ -n "$2" ] && /bin/echo "$2"
  [ -n "$3" ] && printf "$3" "$4" "$5" "$6"
  /bin/echo "────────────────────────────────────────────────────────────────"
}

init() {
  /bin/echo "${red}BUILD_TYPE:        ${BUILD_TYPE}${reset}"
  /bin/echo "SIDEKICK_ROOT:     ${SIDEKICK_ROOT}"
  /bin/echo "OBS_ROOT:          ${OBS_ROOT}"
  /bin/echo "DEV_DIR:           ${DEV_DIR}"
  start="$(/bin/date '+%Y-%m-%d %H:%M:%S')"
  start_ts=$(/bin/date +%s)
  hr "Building dependencies in: ${DEV_DIR}" "Started ${start}"
}

create_work_dirs() {
  /bin/mkdir -p "${DEV_DIR}"
  /bin/mkdir -p "${WORK_DIR}"
  /bin/mkdir -p "${OBSDEPS}/bin"
  /bin/mkdir -p "${OBSDEPS}/include"
  /bin/mkdir -p "${OBSDEPS}/lib/pkgconfig"
  /bin/mkdir -p "${OBSDEPS}/share"
}

delete_work_dirs() {
  /bin/rm -rf "${WORK_DIR}"
}

# cleanup() {
#   # /bin/rm "${OBSDEPS}/bin/x264"
#   # /bin/rm "${OBSDEPS}"/lib/*.la
#   # /bin/rm "${OBSDEPS}"/lib/*.a
# }

# check_curl() {
#   if [ "${MACOS_MAJOR}" -lt "11" ] && [ "${MACOS_MINOR}" -lt "15" ]; then
#     if [ ! -d "${HOMEBREW_PREFIX}/opt/curl" ]; then
#         hr "Installing Homebrew curl..."
#         brew install curl
#     fi
#     export CURLCMD="${HOMEBREW_PREFIX}/opt/curl/bin/curl"
#   else
#     export CURLCMD='curl'
#   fi
# }

curl() {
  if [ "${MACOS_MAJOR}" -lt "11" ] && [ "${MACOS_MINOR}" -lt "15" ]; then
    if [ ! -d "${HOMEBREW_PREFIX}/opt/curl" ]; then
        hr "Installing Homebrew curl..."
        brew install curl
    fi
    ${HOMEBREW_PREFIX}/opt/curl/bin/curl "$@"
  else
    /usr/bin/curl "$@"
  fi
}

uninstall_homebrew() {
  hr "Uninstalling homebrew"
  set +e
  curl -o uninstall.sh -fsSL https://raw.githubusercontent.com/Homebrew/install/master/uninstall.sh
  /bin/bash uninstall.sh --force
  /bin/rm uninstall.sh
  set -e
}

install_homebrew() {
  set +e
  if [ "$1" = "clean" ]; then uninstall_homebrew; fi
  if ! exists brew; then
    hr "Installing Homebrew"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
  fi
  set -e
}

install_or_upgrade() {
  set +e
  if brew ls --versions "$1" >/dev/null; then
    HOMEBREW_NO_AUTO_UPDATE=1 brew upgrade "$1"
  else
    HOMEBREW_NO_AUTO_UPDATE=1 brew install "$1"
  fi
  set -e
}

restore_brews() {
  set +e
  if [ -d "${HOMEBREW_PREFIX}/opt/xz" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/liblzma.dylib" ]; then
    brew link xz
  fi
  if [ -d "${HOMEBREW_PREFIX}/opt/zstd" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/libzstd.dylib" ]; then
    brew link zstd
  fi
  if [ -d "${HOMEBREW_PREFIX}/opt/libtiff" ] && [ !  -f "${HOMEBREW_PREFIX}/lib/libtiff.dylib" ]; then
    brew link libtiff
  fi
  if [ -d "${HOMEBREW_PREFIX}/opt/webp" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/libwebp.dylib" ]; then
    brew link webp
  fi
  set -e
}

install_build_tools() {
  if [ "$SKIP_BUILD_TOOLS" -eq 0 ]; then
    hr "Installing build tools"
    install_or_upgrade libtool
    install_or_upgrade automake
    install_or_upgrade pcre
    install_or_upgrade cmake
    # install_or_upgrade freetype
    install_or_upgrade nasm
    install_or_upgrade pkg-config
    install_or_upgrade cmocka
  fi
}

install_core_obs_deps() {
  hr "Installing core OBS dependencies"
  set +e
  if [ -d "${HOMEBREW_PREFIX}/opt/openssl@1.0.2t" ]; then
    brew uninstall openssl@1.0.2t
    brew untap local/openssl
  fi
  if [ -d "${HOMEBREW_PREFIX}/opt/python@2.7.17" ]; then
    brew uninstall python@2.7.17
    brew untap local/python2
  fi
  install_or_upgrade curl-openssl
  install_or_upgrade openssl@1.1
  # install_or_upgrade speexdsp
  install_or_upgrade fdk-aac
  brew tap akeru-inc/tap
  install_or_upgrade akeru-inc/tap/xcnotary
  set -e
}

build_png() {
  if [ -f "${OBSDEPS}/lib/libpng.a" ]; then
    hr "png already installed"
  else
    hr "Building png ${PNG_VERSION} (FFmpeg dependency"
    cd "${WORK_DIR}"
    /bin/rm -rf libpng-${PNG_VERSION}
    curl -fkRL -O "https://downloads.sourceforge.net/project/libpng/libpng16/${PNG_VERSION}/libpng-${PNG_VERSION}.tar.xz"
    /usr/bin/tar -xf libpng-${PNG_VERSION}.tar.xz
    cd ./libpng-${PNG_VERSION}
    /bin/mkdir -p build
    cd ./build
    ../configure --disable-shared --enable-static --prefix="${OBSDEPS}"
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_opus() {
  if [ -f "${OBSDEPS}/lib/libopus.a" ]; then
    hr "opus already installed"
  else
    hr "Building opus ${OPUS_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf opus-${OPUS_VERSION}
    curl -fkRL -O "https://ftp.osuosl.org/pub/xiph/releases/opus/opus-${OPUS_VERSION}.tar.gz"
    /usr/bin/tar -xf opus-${OPUS_VERSION}.tar.gz
    cd ./opus-${OPUS_VERSION}
    /bin/mkdir -p build
    cd ./build
    ../configure --disable-shared --enable-static --prefix="${OBSDEPS}" --disable-doc
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_ogg() {
  if [ -f "${OBSDEPS}/lib/libogg.a" ]; then
    hr "ogg already installed"
  else
    hr "Building ogg ${OGG_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf libogg-${OGG_VERSION}
    curl -fkRL -O "https://gitlab.xiph.org/xiph/ogg/-/archive/${OGG_VERSION}/ogg-${OGG_VERSION}.tar.gz"
    /usr/bin/tar -xf ogg-${OGG_VERSION}.tar.gz
    cd ./ogg-${OGG_VERSION}
    /bin/mkdir -p build
    ./autogen.sh
    cd ./build
    ../configure --disable-shared --enable-static --prefix="${OBSDEPS}"
    /usr/bin/make -j 1  # error if j > 1
    /usr/bin/make install
  fi
}

build_vorbis() {
  if [ -f "${OBSDEPS}/lib/libvorbis.a" ]; then
    hr "vorbis already installed"
  else
    hr "Building vorbis ${VORBIS_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf libvorbis-${VORBIS_VERSION}
    curl -fkRL -O "https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-${VORBIS_VERSION}.tar.gz"
    /usr/bin/tar -xf libvorbis-${VORBIS_VERSION}.tar.gz
    cd ./libvorbis-${VORBIS_VERSION}
    /bin/mkdir -p build
    cd ./build
    ../configure --disable-shared --enable-static --prefix="${OBSDEPS}"
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_vpx() {
  if [ -f "${OBSDEPS}/lib/libvpx.a" ]; then
    hr "vpx already installed"
  else
    hr "Building vpx ${VPX_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf libvpx-v${VPX_VERSION}
    if [ "$(arch)" = "arm64" ]; then
      /usr/bin/git clone "https://chromium.googlesource.com/webm/libvpx" libvpx-v${VPX_VERSION}
    else
      curl -fkRL -O "https://chromium.googlesource.com/webm/libvpx/+archive/v${VPX_VERSION}.tar.gz"
      /bin/mkdir -p ./libvpx-v${VPX_VERSION}
      /usr/bin/tar -xf v${VPX_VERSION}.tar.gz -C ./libvpx-v${VPX_VERSION}
    fi
    cd ./libvpx-v${VPX_VERSION}
    /bin/mkdir -p build
    cd ./build
    if [ "$(arch)" = "arm64" ]; then
      ../configure --disable-shared --target=arm64-darwin20-gcc --prefix="${OBSDEPS}" --libdir="${OBSDEPS}/lib" \
        --enable-pic --enable-vp9-highbitdepth --disable-examples --disable-unit-tests --disable-docs
    else
      if [ $(/bin/echo "${MACOSX_DEPLOYMENT_TARGET}" | /usr/bin/cut -d "." -f 1) -lt 11 ]; then
        VPX_TARGET="$(($(/bin/echo ${MACOSX_DEPLOYMENT_TARGET} | /usr/bin/cut -d "." -f 2)+4))"
      else
        VPX_TARGET="$(($(/bin/echo ${MACOSX_DEPLOYMENT_TARGET} | /usr/bin/cut -d "." -f 1)+9))"
      fi
      ../configure --disable-shared  --target=x86_64-darwin${VPX_TARGET}-gcc --prefix="${OBSDEPS}" --libdir="${OBSDEPS}/lib" \
        --enable-pic --enable-vp9-highbitdepth --disable-examples --disable-unit-tests --disable-docs
    fi
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_x264() {
  if [ -f "${OBSDEPS}/lib/libx264.dylib" ]; then
    hr "x264 already installed"
  else
    hr "Building x264"
    cd "${WORK_DIR}"
    /bin/rm -rf x264
    /bin/mkdir -p x264
    /usr/bin/git clone https://code.videolan.org/videolan/x264.git
    cd ./x264
    /usr/bin/git config advice.detachedHead false
    /usr/bin/git checkout -f ${X264_COMMIT} --
    /bin/mkdir -p build
    cd ./build
    ../configure --enable-static --prefix="${OBSDEPS}" --includedir="${OBSDEPS}/include" \
      --disable-lsmash --disable-swscale --disable-ffms --enable-strip \
      --extra-ldflags="-mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}"
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
    ../configure --enable-shared --prefix="${OBSDEPS}" --libdir="${OBSDEPS}/lib" \
      --disable-lsmash --disable-swscale --disable-ffms --enable-strip \
      --extra-ldflags="-mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}"
    /usr/bin/make -j ${NUM_CORES}
    ln -f -s libx264.*.dylib libx264.dylib
    find . -name \*.dylib -exec /bin/cp -pPR \{\} "${OBSDEPS}/lib/" \;
    /bin/mkdir -p "${OBSDEPS}/include"
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ../* "${OBSDEPS}/include/"
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ./* "${OBSDEPS}/include/"
  fi
}

build_theora() {
  if [ -f "${OBSDEPS}/lib/libtheora.a" ]; then
    hr "libtheora already installed"
  else
    hr "Building libtheora ${THEORA_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf libtheora-${THEORA_VERSION}
    curl -fkRL -O "https://ftp.osuosl.org/pub/xiph/releases/theora/libtheora-${THEORA_VERSION}.tar.bz2"
    /usr/bin/tar -xf libtheora-${THEORA_VERSION}.tar.bz2
    cd ./libtheora-${THEORA_VERSION}
    /bin/mkdir -p build
    cd ./build
    ../configure --disable-shared --enable-static --prefix="${OBSDEPS}" --disable-oggtest --disable-vorbistest --disable-examples
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_lame() {
  if [ -f "${OBSDEPS}/lib/libmp3lame.a" ]; then
    hr "liblame already installed"
  else
    hr "Building liblame ${LAME_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf lame-${LAME_VERSION}
    curl -fkRL -O "https://downloads.sourceforge.net/project/lame/lame/${LAME_VERSION}/lame-${LAME_VERSION}.tar.gz"
    /usr/bin/tar -xf lame-${LAME_VERSION}.tar.gz
    cd ./lame-${LAME_VERSION}
    /usr/bin/sed -i '' '/lame_init_old/d' ./include/libmp3lame.sym
    /bin/mkdir -p build
    cd ./build
    ../configure --disable-shared --prefix="${OBSDEPS}" --enable-nasm --disable-dependency-tracking --disable-debug
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

create_mbedtls_pkgconfig() {
  /bin/cat <<EOF > ${OBSDEPS}/lib/pkgconfig/mbedcrypto.pc
prefix=${OBSDEPS}
libdir=\${prefix}/lib
includedir=\${prefix}/include
 
Name: mbedcrypto
Description: lightweight crypto and SSL/TLS library.
Version: ${MBEDTLS_VERSION}
 
Libs: -L\${libdir} -lmbedcrypto
Cflags: -I\${includedir}
 
EOF

  /bin/cat <<EOF > ${OBSDEPS}/lib/pkgconfig/mbedtls.pc
prefix=${OBSDEPS}
libdir=\${prefix}/lib
includedir=\${prefix}/include
 
Name: mbedtls
Description: lightweight crypto and SSL/TLS library.
Version: ${MBEDTLS_VERSION}
 
Libs: -L\${libdir} -lmbedtls
Cflags: -I\${includedir}
Requires.private: mbedx509
 
EOF

  /bin/cat <<EOF > ${OBSDEPS}/lib/pkgconfig/mbedx509.pc
prefix=${OBSDEPS}
libdir=\${prefix}/lib
includedir=\${prefix}/include
 
Name: mbedx509
Description: The mbedTLS X.509 library
Version: ${MBEDTLS_VERSION}
 
Libs: -L\${libdir} -lmbedx509
Cflags: -I\${includedir}
Requires.private: mbedcrypto
 
EOF
}

build_mbedtls() {
  if [ -f "${OBSDEPS}/lib/libmbedtls.dylib" ]; then
    hr "medtls already installed"
  else
    hr "Building mbedtls ${MBEDTLS_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf mbedtls-mbedtls-${MBEDTLS_VERSION}
    curl -fkRL -O "https://github.com/ARMmbed/mbedtls/archive/mbedtls-${MBEDTLS_VERSION}.tar.gz"
    /usr/bin/tar -xf mbedtls-${MBEDTLS_VERSION}.tar.gz
    cd ./mbedtls-mbedtls-${MBEDTLS_VERSION}
    /usr/bin/sed -i '' 's/\/\/\#define MBEDTLS_THREADING_PTHREAD/\#define MBEDTLS_THREADING_PTHREAD/g' include/mbedtls/config.h
    /usr/bin/sed -i '' 's/\/\/\#define MBEDTLS_THREADING_C/\#define MBEDTLS_THREADING_C/g' include/mbedtls/config.h
    /bin/mkdir -p build
    cd ./build
    cmake -DCMAKE_INSTALL_PREFIX="${OBSDEPS}" -DUSE_SHARED_MBEDTLS_LIBRARY=ON -DENABLE_PROGRAMS=OFF ..
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
    install_name_tool -id "${OBSDEPS}/lib/libmbedtls.${MBEDTLS_VERSION}.dylib" "${OBSDEPS}/lib/libmbedtls.${MBEDTLS_VERSION}.dylib"
    install_name_tool -id "${OBSDEPS}/lib/libmbedcrypto.${MBEDTLS_VERSION}.dylib" "${OBSDEPS}/lib/libmbedcrypto.${MBEDTLS_VERSION}.dylib"
    install_name_tool -id "${OBSDEPS}/lib/libmbedx509.${MBEDTLS_VERSION}.dylib" "${OBSDEPS}/lib/libmbedx509.${MBEDTLS_VERSION}.dylib"
    install_name_tool -change libmbedx509.1.dylib "${OBSDEPS}/lib/libmbedx509.1.dylib" "${OBSDEPS}/lib/libmbedtls.${MBEDTLS_VERSION}.dylib"
    install_name_tool -change libmbedcrypto.5.dylib "${OBSDEPS}/lib/libmbedcrypto.5.dylib" "${OBSDEPS}/lib/libmbedtls.${MBEDTLS_VERSION}.dylib"
    install_name_tool -change libmbedcrypto.5.dylib "${OBSDEPS}/lib/libmbedcrypto.5.dylib" "${OBSDEPS}/lib/libmbedx509.${MBEDTLS_VERSION}.dylib"
    create_mbedtls_pkgconfig
  fi
}

build_srt() {
  if [ -f "${OBSDEPS}/lib/libsrt.a" ]; then
    hr "srt already installed"
  else
    hr "Building srt ${SRT_VERSION} (FFmpeg dependency)"
    cd "${WORK_DIR}"
    /bin/rm -rf srt-${SRT_VERSION}
    curl -fkRL -O "https://github.com/Haivision/srt/archive/v${SRT_VERSION}.tar.gz"
    /usr/bin/tar -xf v${SRT_VERSION}.tar.gz
    cd ./srt-${SRT_VERSION}
    /bin/mkdir -p build
    cd ./build
    cmake -DCMAKE_INSTALL_PREFIX="${OBSDEPS}" -DENABLE_STATIC=ON -DENABLE_SHARED=OFF \
      -DSSL_INCLUDE_DIRS="${OBSDEPS}/include" -DSSL_LIBRARY_DIRS="${OBSDEPS}/lib" \
      -DUSE_ENCLIB="mbedtls" -DENABLE_APPS=OFF -DCMAKE_FIND_FRAMEWORK=LAST ..
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_ffmpeg() {
  if [ -f "${OBSDEPS}/lib/libavcodec.dylib" ]; then
    hr "FFmpeg already installed"
  else
    hr "Building FFmpeg ${FFMPEG_VERSION}"
    if [ -d "${HOMEBREW_PREFIX}/opt/xz" ]; then brew unlink xz; fi
    if [ -d "${HOMEBREW_PREFIX}/opt/sdl2" ]; then brew unlink sdl2; fi
    export LD_LIBRARY_PATH="${OBSDEPS}/lib"
    export PKG_CONFIG_PATH="${OBSDEPS}/lib/pkgconfig"
    export LDFLAGS="-L${LD_LIBRARY_PATH}"
    export CFLAGS="-I${OBSDEPS}/include"
    cd "${WORK_DIR}"
    /bin/rm -rf FFmpeg-n${FFMPEG_VERSION}
    curl -fkRL -O "https://github.com/FFmpeg/FFmpeg/archive/n${FFMPEG_VERSION}.zip"
    unzip ./n${FFMPEG_VERSION}.zip
    cd ./FFmpeg-n${FFMPEG_VERSION}
    /bin/mkdir -p build
    cd ./build
    PKG_CONFIG_PATH="${PKG_CONFIG_PATH}" \
    ../configure --enable-shared --disable-static --prefix="${OBSDEPS}" --shlibdir="${OBSDEPS}/lib" \
      --pkg-config-flags="--static" --host-cflags="${CFLAGS}" --host-ldflags="${LDFLAGS}" \
      --extra-ldflags="-mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" \
      --enable-gpl --enable-version3 --enable-nonfree --enable-pthreads \
      --enable-libx264 --enable-libopus --enable-libvorbis --enable-libvpx \
      --enable-libsrt --enable-libtheora --enable-libmp3lame --enable-videotoolbox \
      --disable-doc --disable-libjack --disable-indev=jack --disable-outdev=sdl \
      --disable-programs
    /usr/bin/make -j ${NUM_CORES}
    find . -name \*.dylib -exec /bin/cp -pPR \{\} "${OBSDEPS}/lib/" \;
    /bin/mkdir -p "${OBSDEPS}/include"
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ../* "${OBSDEPS}/include/"
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ./* "${OBSDEPS}/include/"
    unset LD_LIBRARY_PATH
    unset PKG_CONFIG_PATH
    unset LDFLAGS
    unset CFLAGS
    if [ -d "${HOMEBREW_PREFIX}/opt/xz" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/liblzma.dylib" ]; then
      brew link xz
    fi
    if [ -d "${HOMEBREW_PREFIX}/opt/sdl2" ] && ! [ -f "${HOMEBREW_PREFIX}/lib/libSDL2.dylib" ]; then
      brew link sdl2
    fi
  fi
}

build_swig() {
  if [ -f "${OBSDEPS}/bin/swig" ]; then
    hr "swig already installed"
  else
    hr "Building swig ${SWIG_VERSION}"
    if [ -d "$(brew --cellar)/swig" ]; then brew unlink swig; fi
    cd "${WORK_DIR}"
    /bin/rm -rf swig-${SWIG_VERSION}
    curl -fkRL -O "https://downloads.sourceforge.net/project/swig/swig/swig-${SWIG_VERSION}/swig-${SWIG_VERSION}.tar.gz"
    /usr/bin/tar -xf swig-${SWIG_VERSION}.tar.gz
    cd ./swig-${SWIG_VERSION}
    /bin/mkdir -p build
    cd ./build
    curl -fkRL -O "https://ftp.pcre.org/pub/pcre/pcre-${PCRE_VERSION}.tar.bz2"
    ../Tools/pcre-build.sh
    ../configure --prefix="${OBSDEPS}" --disable-dependency-tracking
    /usr/bin/make -j ${NUM_CORES}
    /bin/cp swig "${OBSDEPS}/bin/"
    /bin/mkdir -p "${OBSDEPS}/share/swig/${SWIG_VERSION}"
    rsync -avh --prune-empty-dirs --include="*.i" --include="*.swg" --include="python" --include="lua" \
      --include="typemaps" --exclude="*" ../Lib/* "${OBSDEPS}/share/swig/${SWIG_VERSION}"
  fi
}

build_speexdsp() {
  if [ -f "${OBSDEPS}/lib/libspeexdsp.dylib" ]; then
    hr "speexdsp already installed"
  else
    hr "Building speexdsp ${SPEEXDSP_VERSION}"
    cd "${WORK_DIR}"
    /bin/rm -rf speexdsp-SpeexDSP-${SPEEXDSP_VERSION}
    curl -fkRL -O "https://github.com/xiph/speexdsp/archive/SpeexDSP-${SPEEXDSP_VERSION}.tar.gz"
    /usr/bin/tar -xf speexDSP-${SPEEXDSP_VERSION}.tar.gz
    cd ./speexdsp-SpeexDSP-${SPEEXDSP_VERSION}
    /usr/bin/sed -i '' "s/CFLAGS='-O3'/CFLAGS='-O3  -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}'/" ./SpeexDSP.spec.in
    ./autogen.sh
    /bin/mkdir -p build
    cd ./build
    ../configure --prefix="${OBSDEPS}" --disable-dependency-tracking
    /usr/bin/make -j ${NUM_CORES}
    find . -name \*.dylib -exec /bin/cp -pPR \{\} "${OBSDEPS}/lib/" \;
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ../include/* "${OBSDEPS}/include/"
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ./include/* "${OBSDEPS}/include/"
  fi
}

build_jansson() {
  if [ -f "${OBSDEPS}/lib/libjansson.dylib" ]; then
    hr "jansson already installed"
  else
    hr "Building jansson ${JANSSON_VERSION}"
    cd "${WORK_DIR}"
    /bin/rm -rf jansson-${JANSSON_VERSION}
    curl -fkRL -O "https://digip.org/jansson/releases/jansson-${JANSSON_VERSION}.tar.gz"
    /usr/bin/tar -xf jansson-${JANSSON_VERSION}.tar.gz
    cd ./jansson-${JANSSON_VERSION}
    /bin/mkdir -p build
    cd ./build
    ../configure --libdir="${OBSDEPS}/lib" --enable-shared --disable-static
    /usr/bin/make -j ${NUM_CORES}
    find . -name \*.dylib -exec /bin/cp -pPR \{\} "${OBSDEPS}/lib/" \;
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ../src/* "${OBSDEPS}/include/"
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --exclude="*" ./src/* "${OBSDEPS}/include/"
  fi
}

build_freetype() {
  if [ -f "${OBSDEPS}/lib/libfreetype.dylib" ]; then
    hr "freetype already installed"
  else
    hr "Building freetype ${FREETYPE_VERSION}"
    cd "${WORK_DIR}"
    /bin/rm -rf freetype-${FREETYPE_VERSION}
    curl -fkRL -O "https://downloads.sourceforge.net/project/freetype/freetype2/${FREETYPE_VERSION}/freetype-${FREETYPE_VERSION}.tar.xz"
    /usr/bin/tar -xf freetype-${FREETYPE_VERSION}.tar.xz
    cd ./freetype-${FREETYPE_VERSION}
    /bin/mkdir -p build
    cd ./build
    ../configure --prefix="${OBSDEPS}" --enable-shared --disable-static --without-harfbuzz --without-brotli
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_rnnoise() {
  if [ -f "${OBSDEPS}/lib/librnnoise.dylib" ]; then
    hr "rnnoise already installed"
  else
    hr "Building rnnoise ${RNNOISE_COMMIT}"
    cd "${WORK_DIR}"
    /bin/rm -rf rnnoise-${RNNOISE_COMMIT}
    /bin/mkdir -p rnnoise-${RNNOISE_COMMIT}
    /usr/bin/git clone https://github.com/xiph/rnnoise.git rnnoise-${RNNOISE_COMMIT}
    cd ./rnnoise-${RNNOISE_COMMIT}
    /usr/bin/git config advice.detachedHead false
    /usr/bin/git checkout -f ${RNNOISE_COMMIT} --
    ./autogen.sh
    /bin/mkdir -p build
    cd ./build
    ../configure --prefix="${OBSDEPS}"
    /usr/bin/make -j ${NUM_CORES}
    /usr/bin/make install
  fi
}

build_luajit() {
  if [ -f "${OBSDEPS}/include/luajit.h" ]; then
    hr "LuaJIT already installed"
  else
    hr "Building LuaJIT ${LUAJIT_VERSION}"
    cd "${WORK_DIR}"
    /bin/rm -rf LuaJIT-${LUAJIT_VERSION}
    # curl -fkRL -O "https://luajit.org/download/LuaJIT-${LUAJIT_VERSION}.tar.gz"
    # /usr/bin/tar -xf LuaJIT-${LUAJIT_VERSION}.tar.gz
    /bin/mkdir -p LuaJIT-${LUAJIT_VERSION}
    /usr/bin/git clone https://github.com/LuaJIT/LuaJIT.git LuaJIT-${LUAJIT_VERSION}
    cd LuaJIT-${LUAJIT_VERSION}
    /usr/bin/git checkout ${LUAJIT_COMMIT}
    /bin/mkdir -p /tmp/luajit/lib
    /bin/mkdir -p /tmp/luajit/include
    # /usr/bin/sed -i '' 's/$(DEFAULT_CC)/clang/g' src/Makefile
    # /usr/bin/sed -i '' 's/-march=i686//g' src/Makefile
    # MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} \
    #   CFLAGS="-fno-stack-check ${CFLAGS}" \
    #   /usr/bin/make PREFIX="${OBSDEPS}"  -j ${NUM_CORES}
    /usr/bin/make PREFIX="${OBSDEPS}"  -j ${NUM_CORES}
    /usr/bin/make PREFIX="${OBSDEPS}" install
    find "${OBSDEPS}/lib" -name libluajit\*.dylib -exec /bin/cp -pPR \{\} /tmp/luajit/lib/ \;
    rsync -avh --prune-empty-dirs --include="*/" --include="*.h" --include="*.hpp" --exclude="*" src/* /tmp/luajit/include/
    /usr/bin/make PREFIX="${OBSDEPS}" uninstall
    /bin/mkdir -p "${OBSDEPS}/include"
    /bin/cp -pPR /tmp/luajit/lib/* "${OBSDEPS}/lib/"
    /bin/cp -pPR /tmp/luajit/include/* "${OBSDEPS}/include/"
    /bin/rm -rf /tmp/luajit
  fi
}

install_qt() {
  set +e
  # Qt 5.10.1
  if [ "${QT_VERSION}" = "5.10.1" ]; then
    if [ -f "${HOMEBREW_PREFIX}/opt/qt/lib/cmake/Qt5/Qt5Config.cmake" ]; then
      local QTV="$(brew ls --versions qt | grep -Eo '\d+\.\d+\.\d+')"
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
    if [ -f "${HOMEBREW_PREFIX}/opt/qt/lib/cmake/Qt5/Qt5Config.cmake" ]; then
      local QTV="$(brew ls --versions qt | grep -Eo '\d+\.\d+\.\d+')"
      if [ "${QTV}" = "5.14.1" ]; then
        hr "Qt ${QT_VERSION} already installed"
      else
        hr "${red}Qt ${QTV} is installed.${reset}${bold} Please run 'brew uninstall qt' before running this script${reset}"
        exit 1
      fi
    else
      hr "Installing Qt ${QT_VERSION}"
      brew install "$SIDEKICK_ROOT/scripts/homebrew/qt_5_14/qt.rb"
    fi
  fi
  # Qt 5.15.2
  if [ "${QT_VERSION%.*}" = "5.15" ]; then
    if [ -f "${HOMEBREW_PREFIX}/opt/qt/lib/cmake/Qt5/Qt5Config.cmake" ]; then
      local -r QTV="$(brew ls --versions qt | grep -Eo '\d+\.\d+\.\d+')"
      if [ "${QTV}" = "${QT_VERSION}" ]; then
        hr "Qt ${QT_VERSION} already installed"
      else
        hr "${red}Qt ${QTV} is installed.${reset}${bold} Please run 'brew uninstall qt' before running this script${reset}"
        exit 1
      fi
    elif [ ! -f "${OBSDEPS}/lib/cmake/Qt5/Qt5ConfigVersion.cmake" ]; then
      hr "Installing Qt ${QT_VERSION}"
      cd "${WORK_DIR}"
      if [ -d "${HOMEBREW_PREFIX}/opt/zstd" ]; then brew unlink zstd; fi
      if [ -d "${HOMEBREW_PREFIX}/opt/libtiff" ]; then brew unlink libtiff; fi
      if [ -d "${HOMEBREW_PREFIX}/opt/webp" ]; then brew unlink webp; fi
      /bin/rm -rf qt-everywhere-src*
      local -r QT_VER_MAJOR="$(/bin/echo "${QT_VERSION}" | /usr/bin/cut -d "." -f -2)"
      curl -fkRLO "https://download.qt.io/official_releases/qt/${QT_VER_MAJOR}/${QT_VERSION}/single/qt-everywhere-src-${QT_VERSION}.tar.xz"
      /usr/bin/tar -xf qt-everywhere-src-${QT_VERSION}.tar.xz
      /bin/rm qt-everywhere-src-${QT_VERSION}.tar.xz
      cd qt-everywhere-src-${QT_VERSION}
      # Patch for https://github.com/obsproject/obs-studio/issues/3799
      /usr/bin/sed -i '' '255d;257,258d' ./qtbase/src/widgets/kernel/qwidgetwindow.cpp
      /bin/mkdir -p build
      cd build
      ../configure --prefix="${OBSDEPS}" -release -opensource -confirm-license -system-zlib \
        -qt-libpng -qt-libjpeg -qt-freetype -qt-pcre -nomake examples -nomake tests -no-rpath -no-glib -pkg-config -dbus-runtime \
        -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcharts -skip qtconnectivity -skip qtdatavis3d \
        -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtlocation \
        -skip qtlottie -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquick3d \
        -skip qtquickcontrols -skip qtquickcontrols2 -skip qtquicktimeline -skip qtremoteobjects \
        -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtspeech \
        -skip qttranslations -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin \
        -skip qtwebsockets -skip qtwebview -skip qtwinextras -skip qtx11extras -skip qtxmlpatterns \
        QMAKE_APPLE_DEVICE_ARCHS="${HOST_ARCH}"
      /usr/bin/make -j ${NUM_CORES}
      /usr/bin/make install
      if [ -d "${HOMEBREW_PREFIX}/opt/zstd" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/libzstd.dylib" ]; then brew link zstd; fi
      if [ -d "${HOMEBREW_PREFIX}/opt/libtiff" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/libtiff.dylib" ]; then brew link libtiff; fi
      if [ -d "${HOMEBREW_PREFIX}/opt/webp" ] && [ ! -f "${HOMEBREW_PREFIX}/lib/libwebp.dylib" ]; then brew link webp; fi
    else
      local -r QTV="$(grep -Eo '\d+\.\d+\.\d+' "${OBSDEPS}/lib/cmake/Qt5/Qt5ConfigVersion.cmake")"
      if [ "${QTV}" = "${QT_VERSION}" ]; then
        hr "Qt ${QT_VERSION} already installed"
      else
        hr "${red}Qt ${QTV} is installed.${reset}${bold} Please uninstall before running this script.${reset}"
        /bin/echo "Install location: ${OBSDEPS}"
        exit 1
      fi
    fi
  fi
}

install_boost() {
  install_or_upgrade boost
  # if [ -f "${HOMEBREW_PREFIX}/opt/boost/include/boost/version.hpp" ]; then
  #   hr "Boost ${BOOST_VERSION} already installed"
  # else
  #   hr "Installing Boost ${BOOST_VERSION}"
  #   set +e
  #   #brew install "${SIDEKICK_ROOT}/scripts/homebrew/boost.rb"
  #   #brew pin boost
  #   brew install boost
  #   set -e
  # fi
}

install_packages_app() {
  if [ -d "/Applications/Packages.app" ] && exists packagesbuild; then
    hr "Packages app already installed"
  else
    hr "Installing Packages app"
    cd "${DEV_DIR}"
    curl -fkRL -O http://s.sudre.free.fr/Software/files/Packages.dmg
    hdiutil attach Packages.dmg
    sudo installer -pkg /Volumes/Packages*/packages/Packages.pkg -target /
    hdiutil detach /Volumes/Packages*
    /bin/rm "${DEV_DIR}/Packages.dmg"
  fi
}

install_vlc() {
  if [ -f "${DEV_DIR}/vlc-${VLC_VERSION}/include/vlc/libvlc.h" ]; then
    hr "VLC ${VLC_VERSION} already installed"
  else
    hr "Installing VLC ${VLC_VERSION}"
    cd "${DEV_DIR}"
    /bin/rm -rf "vlc-${VLC_VERSION}"
    curl -fkRL -O "https://downloads.videolan.org/vlc/${VLC_VERSION}/vlc-${VLC_VERSION}.tar.xz"
    /usr/bin/tar -xf "vlc-${VLC_VERSION}.tar.xz"
    /bin/rm "${DEV_DIR}/vlc-${VLC_VERSION}.tar.xz"
  fi
}

install_cef() {
  if [ -f "${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}/cmake/FindCEF.cmake" ]; then
    hr "CEF ${CEF_VERSION} already installed"
  else
    hr "Installing CEF ${CEF_VERSION} (${CEF_ARCH})"
    cd "${DEV_DIR}"
    /bin/rm -rf "cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}"
    if [ "${BUILD_TYPE}" = "Debug" ]; then CEF_BUILD_TYPE=Debug; else CEF_BUILD_TYPE=Release; fi
    local CEF_VERSION_ENCODED="${CEF_BUILD_VERSION//+/%2B}"
    curl -fkRL -o "cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}.tar.bz2" \
      "https://cef-builds.spotifycdn.com/cef_binary_${CEF_VERSION_ENCODED}_macos${CEF_ARCH}.tar.bz2"
    /usr/bin/tar -xf "cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}.tar.bz2"
    /bin/rm "${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}.tar.bz2"
    cd "${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macos${CEF_ARCH}"
    # /bin/rm -rf tests
    # /usr/bin/sed -i '' 's/\"10.9\"/\"10.11\"/' ./cmake/cef_variables.cmake
    local old_ver='10.10'
    local new_ver="${MACOSX_DEPLOYMENT_TARGET}"
    if [ ${MACOS_CEF_BUILD_VERSION} -le 3770 ]; then
      old_ver='10.9'
      new_ver='10.11'
    elif [ ${MACOS_CEF_BUILD_VERSION} -ge 4324 ]; then
      old_ver='10.11'
    fi
    /usr/bin/sed -i '' 's/"'${old_ver}'"/"'${new_ver}'"/' ./cmake/cef_variables.cmake
    /bin/mkdir -p build && cd ./build
    cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations" \
      -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" \
      -DCMAKE_OSX_ARCHITECTURES="${CMAKE_OSX_ARCHITECTURES}" -DPROJECT_ARCH="${CMAKE_OSX_ARCHITECTURES}" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" -DCMAKE_BUILD_TYPE=${CEF_BUILD_TYPE} ..
    /usr/bin/make -j ${NUM_CORES}
    /bin/mkdir -p libcef_dll
  fi
}

build_webrtc() {
  if [ -f "${DEV_DIR}/webrtc/lib/libwebrtc.a" ]; then
    hr "WebRTC library found: ${DEV_DIR}/webrtc"
  else
    hr "Building WebRTC"
    cd "${SIDEKICK_ROOT}"
    ./build-webrtc "$@"
  fi
}

print_summary() {
  end="$(/bin/date '+%Y-%m-%d %H:%M:%S')"
  end_ts="$(/bin/date +%s)"
  runtime=$(( end_ts - start_ts ))
  hours=$(( runtime / 3600 ))
  minutes=$(( (runtime % 3600) / 60 ))
  seconds=$(( (runtime % 3600) % 60 ))

  if [ "$1" != "skip_build_tools" ]; then
    hr \
      "Start:    ${start}" \
      "End:      ${end}" \
      "Elapsed:  (hh:mm:ss) %02d:%02d:%02d\n" \
      ${hours} ${minutes} ${seconds}
  fi
}

build_ffmpeg_deps() {
  build_png
  build_opus
  build_ogg
  build_vorbis
  build_vpx
  build_x264
  build_theora
  build_lame
  build_mbedtls
  build_srt
}

main() {
  init
  # check_curl
  curl --version > /dev/null
  install_homebrew "$@"
  install_build_tools
  create_work_dirs
  install_core_obs_deps
  build_ffmpeg_deps
  build_ffmpeg
  build_swig
  build_speexdsp
  build_jansson
  build_luajit
  build_freetype
  build_rnnoise
  install_qt
  install_boost
  install_packages_app
  install_vlc
  install_cef
  install_or_upgrade akeru-inc/tap/xcnotary
  restore_brews
  delete_work_dirs
  # cleanup
  build_webrtc "$@"
  print_summary "$@"
}

main "$@"
