#!/usr/bin/env bash
rm -rf "$DSTVOLUME/Library/Application Support/obs-studio"
rm -rf "$HOME/Library/Application Support/obs-studio"
rm -rf "$DSTVOLUME/Library/CoreMediaIO/Plug-Ins/DAL/obs-mac-virtualcam.plugin"
rm -rf "$HOME/Library/Caches/com.obsproject.obs-studio"
rm -f "$HOME/Library/Preferences/com.obsproject.obs-studio.plist"
rm -f "$HOME/Library/Preferences/com.obsproject.obs-studio.helper.renderer.plist"
rm -rf "$HOME/Library/Saved Application State/com.obsproject.obs-studio.savedState"
