#!/usr/bin/env bash
set -e

export BUILD_DIR=xAuto

_SIDEKICK_ROOT="$(pwd)"
export SIDEKICK_ROOT=${_SIDEKICK_ROOT}
cd ../../.. || exit
_OBS_ROOT="$(pwd)"
export OBS_ROOT=${_OBS_ROOT}
export BUILD_ROOT="${OBS_ROOT}/${BUILD_DIR}"

exists() {
  command -v "$1" >/dev/null 2>&1
}

hr() {
  echo "────────────────────────────────────────────────────────────────"
  echo "$1"
  echo "────────────────────────────────────────────────────────────────"
}

if ! exists dmgbuild; then
  if ! exists python3; then brew install python3; fi
  hr "Installing dmgbuild"
  python3 -m pip install dmgbuild
fi

if [ ! -f "${BUILD_ROOT}/OBS.app" ]; then
  cd "${SIDEKICK_ROOT}" || exit
  ./package || exit
fi

cd "${BUILD_ROOT}" || exit
hr "Building dmg for OBS.app"
dmgbuild -s "${SIDEKICK_ROOT}/scripts/install/obs-dmg/settings.json" "OBS" obs.dmg

hr "OBS installer created: ${BUILD_ROOT}/obs.dmg"
