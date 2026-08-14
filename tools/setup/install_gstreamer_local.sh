#!/bin/bash
set -euo pipefail
GST_VER=1.28.4
TMP=/tmp/qgc-gstreamer
sudo installer -pkg "$TMP/gstreamer-1.0-${GST_VER}-universal.pkg" -target /
sudo installer -pkg "$TMP/gstreamer-1.0-devel-${GST_VER}-universal.pkg" -target /
ls /Library/Frameworks/GStreamer.framework
echo "GStreamer ${GST_VER} installed."
