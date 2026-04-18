#!/usr/bin/env bash
# Build a NodeTalk AppImage.
#
# Inputs (env):
#   BUILD_DIR  - cmake build directory (default: ./build)
#   APPDIR     - staging dir         (default: $BUILD_DIR/AppDir)
#   QMAKE      - path to qmake6      (auto-detected)
#   VERSION    - x.y.z used in filename (default: dev)
#
# Requires: linuxdeploy + linuxdeploy-plugin-qt on PATH.

set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
APPDIR="${APPDIR:-$BUILD_DIR/AppDir}"
VERSION="${VERSION:-dev}"
QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"

cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# Desktop file + icon (linuxdeploy expects them at the root of AppDir).
cp packaging/linux/nodetalk.desktop "$APPDIR/"
cp resources/icons/nodetalk.svg     "$APPDIR/nodetalk.svg"

export QMAKE
export OUTPUT="NodeTalk-${VERSION}-x86_64.AppImage"

# Restrict Qt SQL drivers to sqlite (avoid libqsqlmimer.so needing libmimerapi.so, etc.).
export EXTRA_QT_PLUGINS="${EXTRA_QT_PLUGINS:-}"
export EXCLUDE_LIBS="${EXCLUDE_LIBS:-}"
QT_PLUGIN_DIR="$(dirname "$QMAKE")/../plugins"
if [[ -d "$QT_PLUGIN_DIR/sqldrivers" ]]; then
    find "$QT_PLUGIN_DIR/sqldrivers" -maxdepth 1 -type f \
        ! -name 'libqsqlite.so' -delete || true
fi

linuxdeploy \
    --appdir "$APPDIR" \
    --plugin qt \
    --output appimage

echo "Built: $OUTPUT"
