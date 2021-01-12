#!/usr/bin/env bash
set -e

DYLIBBUNDLER="${SIDEKICK_ROOT}/scripts/macdylibbundler/build/dylibbundler"
export BUILD_TYPE=${BUILD_TYPE:-Release}

rm -rf ./OBS.app 2> /dev/null
mkdir -p OBS.app/Contents/MacOS
mkdir -p OBS.app/Contents/Plugins
mkdir -p OBS.app/Contents/Resources

cp -pfR rundir/${BUILD_TYPE}/bin/ ./OBS.app/Contents/MacOS
cp -pfR rundir/${BUILD_TYPE}/data ./OBS.app/Contents/Resources
cp -pfR rundir/${BUILD_TYPE}/obs-plugins/ ./OBS.app/Contents/Plugins

cp -pf "${SIDEKICK_ROOT}/scripts/install/osx/Info.plist" ./OBS.app/Contents/
cp -pf "${SIDEKICK_ROOT}/scripts/install/osx/obs.icns" ./OBS.app/Contents/Resources/

ALL_PLUGINS=( coreaudio-encoder.so decklink-ouput-ui.so frontend-tools.so image-source.so \
  linux-jack.so mac-avcapture.so mac-capture.so mac-decklink.so mac-syphon.so mac-vth264.so \
  MFCBroadcast.so MFCUpdater.so obs-browser.so obs-browser-page obs-ffmpeg.so obs-filters.so \
  obs-libfdk.so obs-outputs.so obs-transitions.so obs-vst.so obs-x264.so rtmp-services.so \
  text-freetype2.so vlc-video.so websocketclient.dylib )

for plugin in ${ALL_PLUGINS[*]}; do
  if [ -f "./OBS.app/Contents/Plugins/${plugin}" ]; then
    PLUGINS+="-x ./OBS.app/Contents/Plugins/${plugin} "
  fi
done

if [ -f ./OBS.app/Contents/MacOS/obs-ffmpeg-mux ]; then PLUGINS+="-x ./OBS.app/Contents/MacOS/obs-ffmpeg-mux "; fi
if [ -f ./OBS.app/Contents/MacOS/libobs-opengl.so ]; then PLUGINS+="-x ./OBS.app/Contents/MacOS/libobs-opengl.so "; fi
if [ -f ./OBS.app/Contents/MacOS/obslua.so ]; then PLUGINS+="-x ./OBS.app/Contents/MacOS/obslua.so "; fi
if [ -f ./OBS.app/Contents/MacOS/_obspython.so ]; then PLUGINS+="-x ./OBS.app/Contents/MacOS/_obspython.so "; fi

${DYLIBBUNDLER} -b -f -q -cd -of -a ./OBS.app $(echo ${PLUGINS})

set +e
rm -f ./OBS.app/Contents/MacOS/libobs.0.dylib 2> /dev/null
rm -f ./OBS.app/Contents/MacOS/libobs-frontend-api.dylib 2> /dev/null
rm -f ./OBS.app/Contents/MacOS/libobsglad.0.dylib 2> /dev/null
rm -f ./OBS.app/Contents/MacOS/libobs-scripting.dylib 2> /dev/null
mv -f ./OBS.app/Contents/MacOS/libobs-opengl.so ./OBS.app/Contents/Frameworks

# echo "Bundling Qt dependencies"
# /usr/local/Cellar/qt/5.10.1/bin/macdeployqt ./OBS.app

# echo "Fixing Qt dependencies"

# # put qt network in here becasuse streamdeck uses it
# cp -pfR /usr/local/opt/qt/lib/QtNetwork.framework ./OBS.app/Contents/Frameworks
# chmod -R +w ./OBS.app/Contents/Frameworks/QtNetwork.framework
# rm -f ./OBS.app/Contents/Frameworks/QtNetwork.framework/QtNetwork.prl
# rm -rf ./OBS.app/Contents/Frameworks/QtNetwork.framework/Headers
# rm -rf ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/Headers
# chmod 644 ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/Resources/Info.plist
# install_name_tool -id \
#   @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork \
#   ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
# install_name_tool -change \
#   /usr/local/Cellar/qt/5.10.1/lib/QtCore.framework/Versions/5/QtCore \
#   @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#   ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork

# # copy bearer plugin (for qt network)
# cp -pfR /usr/local/opt/qt/plugins/bearer ./OBS.app/Contents/Plugins
# install_name_tool -id \
#   @rpath/libqgenericbearer.dylib \
#   ./OBS.app/Contents/Plugins/bearer/libqgenericbearer.dylib
# install_name_tool -change \
#   /usr/local/Cellar/qt/5.10.1/lib/QtCore.framework/Versions/5/QtCore \
#   @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#   ./OBS.app/Contents/Plugins/bearer/libqgenericbearer.dylib
# install_name_tool -change \
#   /usr/local/Cellar/qt/5.10.1/lib/QtNetwork.framework/Versions/5/QtNetwork \
#   @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork \
#   ./OBS.app/Contents/Plugins/bearer/libqgenericbearer.dylib

# # decklink ui qt
# if [ -f ./OBS.app/Contents/Plugins/decklink-ouput-ui.so ]; then
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore \
#     @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#     ./OBS.app/Contents/Plugins/decklink-ouput-ui.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui \
#     @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
#     ./OBS.app/Contents/Plugins/decklink-ouput-ui.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets \
#     @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
#     ./OBS.app/Contents/Plugins/decklink-ouput-ui.so
# fi

# # frontend tools qt
# if [ -f ./OBS.app/Contents/Plugins/frontend-tools.so ]; then
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore \
#     @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#     ./OBS.app/Contents/Plugins/frontend-tools.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui \
#     @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
#     ./OBS.app/Contents/Plugins/frontend-tools.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets \
#     @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
#     ./OBS.app/Contents/Plugins/frontend-tools.so
# fi

# # vst qt
# if [ -f ./OBS.app/Contents/Plugins/obs-vst.so ]; then
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore \
#     @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#     ./OBS.app/Contents/Plugins/obs-vst.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui \
#     @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
#     ./OBS.app/Contents/Plugins/obs-vst.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras \
#     @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras \
#     ./OBS.app/Contents/Plugins/obs-vst.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets \
#     @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
#     ./OBS.app/Contents/Plugins/obs-vst.so
# fi

# # mfcbroadcast qt
# if [ -f ./OBS.app/Contents/Plugins/MFCBroadcast.so ]; then
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore \
#     @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#     ./OBS.app/Contents/Plugins/MFCBroadcast.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui \
#     @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
#     ./OBS.app/Contents/Plugins/MFCBroadcast.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg \
#     @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg \
#     ./OBS.app/Contents/Plugins/MFCBroadcast.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets \
#     @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
#     ./OBS.app/Contents/Plugins/MFCBroadcast.so
# fi

# # mfcupdater qt
# if [ -f ./OBS.app/Contents/Plugins/MFCUpdater.so ]; then
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore \
#     @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
#     ./OBS.app/Contents/Plugins/MFCUpdater.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui \
#     @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
#     ./OBS.app/Contents/Plugins/MFCUpdater.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg \
#     @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg \
#     ./OBS.app/Contents/Plugins/MFCUpdater.so
#   install_name_tool -change \
#     /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets \
#     @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
#     ./OBS.app/Contents/Plugins/MFCUpdater.so
# fi
