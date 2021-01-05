#!/usr/bin/env bash
chown -R root:admin "$2"
find "$2" -iname "*.dylib" -exec chmod 755 \{\} \;
