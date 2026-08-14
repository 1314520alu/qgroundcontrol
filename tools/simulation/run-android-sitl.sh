#!/usr/bin/env bash
# Start DodecaHexa SITL + UniRC 10 Pro emulator + install QGC APK.
#
# Usage: ./run-android-sitl.sh [--build] [--no-emulator] [--with-latency] [--force-avd]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SIM="$ROOT/tools/simulation"
BUILD_APK=0
NO_EMULATOR=0
WITH_LATENCY=0
FORCE_AVD=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --) shift ;;
        --build) BUILD_APK=1; shift ;;
        --no-emulator) NO_EMULATOR=1; shift ;;
        --with-latency) WITH_LATENCY=1; shift ;;
        --force-avd) FORCE_AVD=1; shift ;;
        -h|--help)
            sed -n '2,4p' "$0"
            exit 0
            ;;
        *)
            echo "Unknown option: $1 (try --help)" >&2
            exit 1
            ;;
    esac
done

SITL_ARGS=(--frame dodeca-hexa)
if [[ "$WITH_LATENCY" -eq 1 ]]; then
    SITL_ARGS+=(--with-latency)
fi
"$SIM/run-arducopter-sitl.sh" "${SITL_ARGS[@]}"

if [[ "$NO_EMULATOR" -eq 1 ]]; then
    echo "SITL only. Connect desktop QGC to tcp://localhost:5760"
    exit 0
fi

# shellcheck disable=SC1091
source "$ROOT/env-qgc.sh" >/dev/null

AVD_ARGS=()
if [[ "$FORCE_AVD" -eq 1 ]]; then
    AVD_ARGS+=(--force)
fi
"$SIM/avd/create-unirc-10-pro-avd.sh" "${AVD_ARGS[@]}"

SDK="${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}"
EMULATOR_BIN="$SDK/emulator/emulator"
if [[ ! -x "$EMULATOR_BIN" ]]; then
    echo "Error: emulator not found at $EMULATOR_BIN" >&2
    exit 1
fi

if ! adb devices | awk 'NR>1 && $1 ~ /^emulator-/ && $2=="device" {found=1} END{exit !found}'; then
    echo "Starting emulator unirc-10-pro..."
    "$EMULATOR_BIN" -avd unirc-10-pro -gpu auto -no-snapshot-save >/tmp/qgc-unirc-emulator.log 2>&1 &
    echo "Emulator log: /tmp/qgc-unirc-emulator.log"
fi

echo "Waiting for emulator adb device..."
adb wait-for-device

boot_ok=0
for _ in $(seq 1 60); do
    boot="$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
    if [[ "$boot" == "1" ]]; then
        boot_ok=1
        break
    fi
    sleep 2
done
if [[ "$boot_ok" -ne 1 ]]; then
    echo "Warning: sys.boot_completed not set after wait; continuing anyway" >&2
fi

SERIAL="$(adb devices | awk 'NR>1 && $1 ~ /^emulator-/ && $2=="device" {print $1; exit}')"
if [[ -z "$SERIAL" ]]; then
    echo "Error: emulator did not come online" >&2
    exit 1
fi

if ! adb -s "$SERIAL" reverse tcp:5760 tcp:5760; then
    echo "Warning: adb reverse failed; use Comm Link host 10.0.2.2 port 5760" >&2
else
    echo "adb reverse tcp:5760 tcp:5760 OK (QGC should use 127.0.0.1:5760)"
fi

if [[ "$BUILD_APK" -eq 1 ]]; then
    if [[ ! -d "$ROOT/build/Android-debug" ]]; then
        echo "Error: build/Android-debug missing. Configure Android build first (see env-qgc.sh)." >&2
        exit 1
    fi
    echo "Building Android-debug APK..."
    cmake --build "$ROOT/build/Android-debug" -j"${JOBS:-4}"
fi

"$ROOT/run-qgc.sh" --android-emulator

echo ""
echo "============================================"
echo "Android SITL ready (DodecaHexa X)"
echo "============================================"
echo "In QGC: Application Settings → Comm Links → Add"
echo "  Type TCP, Host 127.0.0.1, Port 5760 (or 10.0.2.2 if reverse failed)"
echo "Then Vehicle Setup → Motors → expect DodecaHexa X coaxial diagram"
echo ""
