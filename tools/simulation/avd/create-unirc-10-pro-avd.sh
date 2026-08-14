#!/usr/bin/env bash
# Create UniRC 10 Pro–sized AVD: 1920x1200 landscape, API 33, ~224 dpi.
#
# Usage: ./create-unirc-10-pro-avd.sh [--force]
set -euo pipefail

AVD_NAME="unirc-10-pro"
API_LEVEL=33
FORCE=0

for arg in "$@"; do
    case "$arg" in
        --force) FORCE=1 ;;
        -h|--help)
            sed -n '2,4p' "$0"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Library/Android/sdk}}"
export ANDROID_SDK_ROOT="$SDK"

find_cmdline_bin() {
    local name="$1"
    local candidate
    if [[ -x "$SDK/cmdline-tools/latest/bin/$name" ]]; then
        printf '%s' "$SDK/cmdline-tools/latest/bin/$name"
        return 0
    fi
    # Prefer highest-sorted cmdline-tools version dir
    for candidate in $(ls -1d "$SDK/cmdline-tools"/*/bin/"$name" 2>/dev/null | sort -r); do
        if [[ -x "$candidate" ]]; then
            printf '%s' "$candidate"
            return 0
        fi
    done
    return 1
}

AVDMANAGER="$(find_cmdline_bin avdmanager || true)"
SDKMANAGER="$(find_cmdline_bin sdkmanager || true)"

if [[ -z "$AVDMANAGER" ]]; then
    echo "Error: avdmanager not found under $SDK/cmdline-tools" >&2
    echo "Install Android SDK cmdline-tools and set ANDROID_SDK_ROOT." >&2
    exit 1
fi

ARCH=x86_64
if [[ "$(uname -m)" == "arm64" ]]; then
    ARCH=arm64-v8a
fi
PKG="system-images;android-${API_LEVEL};google_apis;${ARCH}"
SYSIMG_DIR="$SDK/system-images/android-${API_LEVEL}/google_apis/${ARCH}"

if "$AVDMANAGER" list avd 2>/dev/null | grep -q "Name: ${AVD_NAME}"; then
    if [[ "$FORCE" -eq 0 ]]; then
        echo "AVD ${AVD_NAME} already exists (use --force to recreate)."
        exit 0
    fi
    echo "no" | "$AVDMANAGER" delete avd -n "$AVD_NAME" || true
fi

if [[ ! -d "$SYSIMG_DIR" ]]; then
    if [[ -z "$SDKMANAGER" ]]; then
        echo "Error: system image missing ($SYSIMG_DIR) and sdkmanager not found." >&2
        echo "Install: sdkmanager \"$PKG\"" >&2
        exit 1
    fi
    echo "Installing $PKG (may take several minutes)..."
    # Accept licenses non-interactively; avoid unbounded `yes` hang.
    yes | "$SDKMANAGER" --sdk_root="$SDK" --licenses >/tmp/qgc-sdk-licenses.log 2>&1 || true
    if ! "$SDKMANAGER" --sdk_root="$SDK" --install "$PKG" "platforms;android-${API_LEVEL}"; then
        echo "Error: failed to install $PKG. See sdkmanager output above." >&2
        echo "You can install manually, then re-run this script." >&2
        exit 1
    fi
else
    echo "System image already present: $SYSIMG_DIR"
fi

echo "no" | "$AVDMANAGER" create avd \
    -n "$AVD_NAME" \
    -k "$PKG" \
    -d "pixel_c" \
    --force

AVD_DIR="${ANDROID_AVD_HOME:-$HOME/.android/avd}/${AVD_NAME}.avd"
CONFIG="$AVD_DIR/config.ini"
if [[ ! -f "$CONFIG" ]]; then
    echo "Error: expected $CONFIG after create" >&2
    exit 1
fi

{
    echo "hw.lcd.width=1920"
    echo "hw.lcd.height=1200"
    echo "hw.lcd.density=224"
    echo "hw.initialOrientation=landscape"
    echo "hw.keyboard=yes"
} >> "$CONFIG"

echo "Created/updated AVD ${AVD_NAME} (${ARCH}, API ${API_LEVEL}, 1920x1200)."
