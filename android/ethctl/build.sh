#!/usr/bin/env bash
# Build the targetSdk=22 ethernet-switch helper APK.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
SDK="${ANDROID_SDK_ROOT:-${HOME}/Library/Android/sdk}"
BT="${SDK}/build-tools/36.0.0"
if [[ ! -d "$BT" ]]; then
    BT="$(find "${SDK}/build-tools" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sort | tail -1)"
fi
ANDROID_JAR="${SDK}/platforms/android-28/android.jar"
if [[ ! -f "$ANDROID_JAR" ]]; then
    ANDROID_JAR="$(find "${SDK}/platforms" -path '*/android.jar' 2>/dev/null | sort | tail -1)"
fi
KS="${HOME}/.android/debug.keystore"
OUT="${ROOT}/build"
SRC="${ROOT}/src"

rm -rf "$OUT"
mkdir -p "$OUT/classes" "$OUT/gen"

"$BT/aapt" package -f -m \
    -J "$OUT/gen" \
    -M "${ROOT}/AndroidManifest.xml" \
    -I "$ANDROID_JAR" \
    -F "$OUT/unsigned.apk"

mapfile -t JAVA_FILES < <(find "$SRC" -name '*.java')
javac -source 8 -target 8 -encoding UTF-8 \
    -bootclasspath "$ANDROID_JAR" \
    -classpath "$ANDROID_JAR" \
    -d "$OUT/classes" \
    "${JAVA_FILES[@]}"

mapfile -t CLASS_FILES < <(find "$OUT/classes" -name '*.class')
"$BT/d8" --min-api 21 --output "$OUT" \
    "${CLASS_FILES[@]}"

cd "$OUT"
"$BT/aapt" add unsigned.apk classes.dex >/dev/null
"$BT/zipalign" -f -p 4 unsigned.apk aligned.apk

if [[ ! -f "$KS" ]]; then
    keytool -genkeypair -v -keystore "$KS" -storepass android -keypass android \
        -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 \
        -dname "CN=Android Debug,O=Android,C=US"
fi

"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android --key-pass pass:android \
    --out "${ROOT}/qgc-ethctl.apk" aligned.apk

echo "Built ${ROOT}/qgc-ethctl.apk"
