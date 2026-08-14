#!/bin/bash
# Launch QGroundControl for UI preview.
#
#   ./run-qgc.sh                    desktop
#   ./run-qgc.sh --fake-mobile      desktop with Android/mobile layout (731x411)
#   ./run-qgc.sh --android          install Android-debug APK to the first USB/device
#   ./run-qgc.sh --android-emulator install APK to the first running emulator
#
# After changing QML/C++:
#   desktop:  just build && ./run-qgc.sh --fake-mobile
#   android:  source ./env-qgc.sh && cmake --build build/Android-debug -j4 && ./run-qgc.sh --android
#   emulator: ... && ./run-qgc.sh --android-emulator

set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
MODE="desktop"
for arg in "$@"; do
    case "$arg" in
        --fake-mobile) MODE="fake-mobile" ;;
        --android) MODE="android" ;;
        --android-emulator) MODE="android-emulator" ;;
        -h|--help)
            sed -n '2,16p' "$0"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg (try --help)" >&2
            exit 1
            ;;
    esac
done

desktop_bin() {
    local app="$ROOT/build/Debug/QGroundControl.app"
    if [[ ! -d "$app" ]]; then
        app="$ROOT/build/QGroundControl.app"
    fi
    local bin="$app/Contents/MacOS/QGroundControl"
    if [[ ! -x "$bin" ]]; then
        echo "QGC app not found. Build first: just build" >&2
        exit 1
    fi
    printf '%s' "$bin"
}

pick_android_serial() {
    local prefer_emulator="$1" # 1 = emulator only, 0 = first device
    if [[ "$prefer_emulator" -eq 1 ]]; then
        adb devices | awk 'NR>1 && $1 ~ /^emulator-/ && $2=="device" {print $1; exit}'
    else
        adb devices | awk 'NR>1 && $2=="device" {print $1; exit}'
    fi
}

install_and_launch_android() {
    local serial="$1"
    local apk="$ROOT/build/Android-debug/android-build/QGroundControl.apk"
    if [[ ! -f "$apk" ]]; then
        echo "Android APK not found. Build first:" >&2
        echo "  source ./env-qgc.sh && cmake --build build/Android-debug -j4" >&2
        exit 1
    fi
    echo "Installing APK to $serial..."
    if ! adb -s "$serial" install -r -d "$apk"; then
        echo "Install failed (likely signature mismatch with store/OEM QGC). Uninstalling org.mavlink.qgroundcontrol and retrying..."
        adb -s "$serial" uninstall org.mavlink.qgroundcontrol || true
        adb -s "$serial" install -r -d "$apk"
    fi
    adb -s "$serial" shell am force-stop org.mavlink.qgroundcontrol
    adb -s "$serial" shell am start -n org.mavlink.qgroundcontrol/org.mavlink.qgroundcontrol.QGCActivity
    echo "QGC launched on $serial"
}

if [[ "$MODE" == "android" || "$MODE" == "android-emulator" ]]; then
    # shellcheck disable=SC1091
    source "$ROOT/env-qgc.sh" >/dev/null
    prefer_emulator=0
    if [[ "$MODE" == "android-emulator" ]]; then
        prefer_emulator=1
    fi
    serial="$(pick_android_serial "$prefer_emulator")"
    if [[ -z "$serial" ]]; then
        if [[ "$prefer_emulator" -eq 1 ]]; then
            echo "No running Android emulator (emulator-* device). Start the AVD first." >&2
        else
            echo "No authorized Android device. Enable USB debugging and tap Allow." >&2
        fi
        adb devices
        exit 1
    fi
    install_and_launch_android "$serial"
    exit 0
fi

quit_desktop_qgc() {
    if ! pgrep -x QGroundControl >/dev/null 2>&1; then
        return 0
    fi
    echo "Closing existing QGroundControl instance..."
    pkill -x QGroundControl 2>/dev/null || true
    local i
    for i in $(seq 1 20); do
        pgrep -x QGroundControl >/dev/null 2>&1 || return 0
        sleep 0.25
    done
    pkill -9 -x QGroundControl 2>/dev/null || true
}

BIN="$(desktop_bin)"
quit_desktop_qgc
if [[ "$MODE" == "fake-mobile" ]]; then
    exec "$BIN" --fake-mobile
fi
exec open -na "$(dirname "$(dirname "$(dirname "$BIN")")")"
