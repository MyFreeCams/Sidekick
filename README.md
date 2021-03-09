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

### Install Dependencies (macOS only)
```bash
./build-deps
```

### Build
```bash
./build
```

### Package
macOS only (omit to skip notarization):
```bash
export NOTARIZATION_USERNAME=your_apple_id_email@provider.com
# only needed first time (password will be stored in keychain)
export NOTARIZATION_PASSWORD=secret
```

```bash
./package
```

Package location:
* mac: `obs-studio/xAuto/MFC-Sidekick-Setup-YYYYMMDD.pkg`
* win: `obs-studio\build64\MFC-Sidekick-Setup-YYYYMMDD.exe`

### Deploy
```bash
./deploy
```

### Customization
To speed up package generation in development, append `dev` to the package command:
```bash
./package dev
```

Build type may be set with the env var `BUILD_TYPE` (default: RelWithDebInfo):
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

To build using Xcode:
```bash
export GENERATOR=Xcode
./build
```

Xcode project file: `obs-studio/xAuto/obs-studio.xcodeproj`
