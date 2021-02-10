# Sidekick

OBS Plugin for Broadcasting to MyFreeCams using WebRTC

## Build & Package

* Clone obs-studio
  ```bash
  git clone --recursive git@github.com:obsproject/obs-studio.git && cd obs-studio
  ```

* Create a `MyFreeCams` subdirectory inside `obs-studio/plugins`
  ```bash
  cd plugins && mkdir MyFreeCams && cd MyFreeCams
  ```

* Clone `Sidekick` inside `obs-studio/plugins/MyFreeCams`
  ```bash
  git clone git@github.com:MyFreeCams/Sidekick.git && cd Sidekick
  ```

* macOS only: Install dependencies
  ```bash
  ./build-deps
  ```

* Build
  ```bash
  ./build
  ```

* Package
  ```bash
  export NOTARIZATION_USERNAME=your_apple_id_email@provider.com  # omit to skip notarization
  # only needed first time (password will be stored in keychain)
  export NOTARIZATION_PASSWORD=secret                            # omit to skip notarization
  ./package
  ```

Package location:
* mac: `obs-studio/xAuto/MFC-Sidekick-Setup-YYYYMMDD.pkg`
* win: `obs-studio\build64\MFC-Sidekick-Setup-YYYYMMDD.exe`

Build type may be entered as an optional arguement (default: RelWithDebInfo):
```bash
./build Release
```
or by setting the env var `BUILD_TYPE`:
macOS:
```bash
export BUILD_TYPE=Debug
./build
```
win:
```bat
set BUILD_TYPE=Debug
build
```
or in-line (mac only):
```bash
BUILD_TYPE=RelWithDebInfo ./build
```

To build using Xcode:
```bash
export GENERATOR=Xcode
./build
```

Xcode project file: `obs-studio/xAuto/obs-studio.xcodeproj`
