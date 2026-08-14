# Source this before building QGC: source ./env-qgc.sh
#
# Desktop:  just configure && just build
# Android:  "$QT_ANDROID_ROOT/bin/qt-cmake" ...  (see comment at bottom)

_qgc_qt_version="6.11.1"
_qgc_qt_root="${HOME}/Qt/${_qgc_qt_version}"

# JDK 21 — required by Android Gradle (.github/build-config.json)
export JAVA_HOME="/opt/homebrew/opt/openjdk@21"
export PATH="${JAVA_HOME}/bin:${PATH}"

# Android SDK / NDK — versions must match .github/build-config.json
export ANDROID_SDK_ROOT="${HOME}/Library/Android/sdk"
export ANDROID_HOME="${ANDROID_SDK_ROOT}"
export ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/27.2.12479018"
export ANDROID_NDK="${ANDROID_NDK_ROOT}"
# Do not export ANDROID_PLATFORM=36. That value is Gradle compile/target SDK
# (build-config.json android.platform). NDK r27c only supports up to API 35;
# leaving it unset matches CI and lets the NDK pick a supported native API.
export QT_ANDROID_ABIS="${QT_ANDROID_ABIS:-arm64-v8a}"

# Host Qt (macOS desktop + Android host tools: moc, rcc, qmlcachegen)
# Do not set QT_ROOT_DIR to the Android kit — that would break desktop `just configure`.
export QT_HOST_PATH="${_qgc_qt_root}/macos"
export Qt6_DIR="${QT_HOST_PATH}"
export PATH="${QT_HOST_PATH}/bin:${ANDROID_SDK_ROOT}/platform-tools:${PATH}"

# Android target Qt (arm64-v8a real devices)
export QT_ANDROID_ROOT="${_qgc_qt_root}/android_arm64_v8a"

# Persist CPM / GStreamer / OpenSSL downloads across clean builds.
# GStreamer Android SDK (~1 GB) lives at $CPM_SOURCE_CACHE/gstreamer-android/
export CPM_SOURCE_CACHE="${CPM_SOURCE_CACHE:-${HOME}/.cache/CPM}"

# Homebrew USTC mirrors (if not already in ~/.zprofile)
export HOMEBREW_API_DOMAIN="${HOMEBREW_API_DOMAIN:-https://mirrors.ustc.edu.cn/homebrew-bottles/api}"
export HOMEBREW_BOTTLE_DOMAIN="${HOMEBREW_BOTTLE_DOMAIN:-https://mirrors.ustc.edu.cn/homebrew-bottles}"

unset _qgc_qt_version _qgc_qt_root

echo "QGC env ready:"
echo "  host Qt:    ${QT_HOST_PATH}"
echo "  android Qt: ${QT_ANDROID_ROOT}"
echo "  ABI:        ${QT_ANDROID_ABIS}"
echo "  NDK:        ${ANDROID_NDK}"
echo "  JAVA:       ${JAVA_HOME}"

# Android Debug configure (after sourcing this file), from the repo root:
#   "$QT_ANDROID_ROOT/bin/qt-cmake" -S . -B build/Android-debug -G Ninja \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DPython3_EXECUTABLE="$PWD/.venv/bin/python" \
#     -DQT_HOST_PATH="$QT_HOST_PATH" \
#     -DQT_ANDROID_ABIS="$QT_ANDROID_ABIS" \
#     -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
#     -DANDROID_NDK="$ANDROID_NDK" \
#     -DQT_ANDROID_SIGN_APK=OFF
#   cmake --build build/Android-debug -j4
# Prefer a modest -j on Android: unlimited --parallel + AUTOMOC can thrash the host.
