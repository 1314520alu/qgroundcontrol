# Android SITL DodecaHexa + UniRC Emulator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** One-command local workflow that starts ArduCopter SITL as DodecaHexa X (12 coaxial motors), launches a UniRC 10 Pro–sized Android emulator, installs a locally built QGC APK, and connects it to SITL over TCP 5760.

**Architecture:** Extend the existing Docker SITL launcher with `--frame dodeca-hexa` and a mounted parm file; add an AVD create script; extend `run-qgc.sh` for emulator serials; wrap everything in `run-android-sitl.sh` + `just android-sitl`.

**Tech Stack:** bash, Docker, Android SDK (`avdmanager` / `emulator` / `adb`), existing `env-qgc.sh` / Android-debug APK path, `just`.

**Spec:** `docs/superpowers/specs/2026-08-14-android-sitl-dodecahexa-design.md`

## Global Constraints

- SITL frame: `--model dodeca-hexa`, `FRAME_CLASS=12`, `FRAME_TYPE=1` (X).
- Default of `run-arducopter-sitl.sh` alone stays `--model +` (backward compatible); orchestrator always passes `--frame dodeca-hexa`.
- TCP port **5760** only for this path (not UDP mock).
- AVD name **`unirc-10-pro`**: 1920×1200 landscape, ~224 dpi, API **33**, `google_apis`.
- Prefer `adb reverse tcp:5760 tcp:5760`; document `10.0.2.2:5760` fallback.
- Apple Silicon APK ABI `arm64-v8a`; Intel needs `x86_64` — document, do not silently build wrong ABI.
- No production QML/C++ motor UI changes; consume existing `MotorLayoutBuilder` DodecaHexa support.
- Conventional Commits; no unrelated refactors.
- Scripts: `set -euo pipefail`, executable bit, pass `shellcheck` when run via `just lint` / pre-commit if applicable.

## File map

| File | Responsibility |
|------|----------------|
| `tools/simulation/params/copter-dodecahexa-x.parm` | Host-mounted FRAME_CLASS/TYPE overrides |
| `tools/simulation/run-arducopter-sitl.sh` | Parse `--frame` / `--with-latency`; start Docker SITL |
| `tools/simulation/avd/create-unirc-10-pro-avd.sh` | Idempotent AVD create |
| `run-qgc.sh` | `--android-emulator` install/launch |
| `tools/simulation/run-android-sitl.sh` | Orchestrator |
| `justfile` | `android-sitl` recipe |
| `tools/simulation/README.md` | Operator docs |

---

### Task 1: DodecaHexa parm + SITL `--frame` support

**Files:**
- Create: `tools/simulation/params/copter-dodecahexa-x.parm`
- Modify: `tools/simulation/run-arducopter-sitl.sh`

**Interfaces:**
- Consumes: Docker image `ardupilot-sitl-4.5.6`, host parm path
- Produces: `./run-arducopter-sitl.sh [--frame dodeca-hexa] [--with-latency]` listening on host TCP 5760

- [ ] **Step 1: Create parm file**

```text
# DodecaHexa X — QGC MotorLayoutBuilder::_dodecaHexaX / FRAME_CLASS=12 FRAME_TYPE=1
FRAME_CLASS 12
FRAME_TYPE 1
```

Write exactly that to `tools/simulation/params/copter-dodecahexa-x.parm`.

- [ ] **Step 2: Rewrite argument parsing and frame selection in `run-arducopter-sitl.sh`**

Replace the single-arg latency check with a loop. Keep defaults: `FRAME="+"` (model `+`), no latency. Supported frames: `+` (default) and `dodeca-hexa`.

Key behavior when `FRAME=dodeca-hexa`:
- `MODEL=dodeca-hexa`
- Mount repo params dir: `-v "$SCRIPT_DIR/params:/qgc-params:ro"`
- Defaults string:  
  `/ardupilot/Tools/autotest/default_params/copter.parm,/qgc-params/copter-dodecahexa-x.parm`

When `FRAME=+`: keep current behavior (image `copter.parm` only, `--model +`).

Sketch (full script should remain one file, `set -euo pipefail`):

```bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FRAME="+"
WITH_LATENCY=0
for arg in "$@"; do
  case "$arg" in
    --with-latency) WITH_LATENCY=1 ;;
    --frame)
      echo "Error: --frame requires a value (e.g. dodeca-hexa)" >&2
      exit 1
      ;;
    --frame=*)
      FRAME="${arg#--frame=}"
      ;;
    --frame*)
      # allow "--frame dodeca-hexa" via shift-style: not in for-loop;
      # prefer requiring --frame=VALUE OR parse with while/shift
      ;;
    -h|--help)
      # print usage including --frame dodeca-hexa
      exit 0
      ;;
    *)
      echo "Unknown option: $arg" >&2
      exit 1
      ;;
  esac
done
```

Prefer a `while [[ $# -gt 0 ]]; do case ... --frame) FRAME="$2"; shift 2;;` parser so `--frame dodeca-hexa` works.

Validate:

```bash
case "$FRAME" in
  +|dodeca-hexa) ;;
  *) echo "Unsupported --frame: $FRAME (use + or dodeca-hexa)" >&2; exit 1 ;;
esac
```

Build `MODEL`, `DEFAULTS`, and optional `VOLUME_ARGS` from `FRAME`. Then construct `ARDUPILOT_CMD` and both latency / non-latency `docker run` paths using those variables (do not leave hard-coded `--model +` only in one branch).

Print which frame started in the success banner.

- [ ] **Step 3: Smoke-test SITL (Docker required)**

```bash
./tools/simulation/run-arducopter-sitl.sh --frame dodeca-hexa
# Expected: container Up; banner mentions dodeca-hexa; tcp://localhost:5760
docker logs arducopter-sitl 2>&1 | tail -40
# Expected: no immediate crash; model/frame related lines if present
docker stop arducopter-sitl || true
```

Optional desktop check: connect desktop QGC to `tcp://localhost:5760`, confirm vehicle / params show FRAME_CLASS 12 (manual).

- [ ] **Step 4: Commit**

```bash
git add tools/simulation/params/copter-dodecahexa-x.parm tools/simulation/run-arducopter-sitl.sh
git commit -m "$(cat <<'EOF'
feat(simulation): add DodecaHexa X frame to ArduCopter SITL launcher

EOF
)"
```

---

### Task 2: UniRC 10 Pro AVD create script

**Files:**
- Create: `tools/simulation/avd/create-unirc-10-pro-avd.sh`

**Interfaces:**
- Consumes: `ANDROID_SDK_ROOT` (default `$HOME/Library/Android/sdk` from `env-qgc.sh`)
- Produces: AVD named `unirc-10-pro` (idempotent unless `--force`)

- [ ] **Step 1: Implement `create-unirc-10-pro-avd.sh`**

```bash
#!/usr/bin/env bash
# Create UniRC 10 Pro–sized AVD: 1920x1200 landscape, API 33, ~224 dpi.
set -euo pipefail

AVD_NAME="unirc-10-pro"
API_LEVEL=33
FORCE=0
for arg in "$@"; do
  case "$arg" in
    --force) FORCE=1 ;;
    -h|--help) sed -n '2,4p' "$0"; exit 0 ;;
    *) echo "Unknown option: $arg" >&2; exit 1 ;;
  esac
done

SDK="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Library/Android/sdk}}"
export ANDROID_SDK_ROOT="$SDK"
CMDLINE="$SDK/cmdline-tools/latest/bin"
AVDMANAGER="$CMDLINE/avdmanager"
SDKMANAGER="$CMDLINE/sdkmanager"
# Fallbacks if "latest" missing: pick newest cmdline-tools/*
if [[ ! -x "$AVDMANAGER" ]]; then
  echo "Error: avdmanager not found under $CMDLINE" >&2
  echo "Install Android SDK cmdline-tools and set ANDROID_SDK_ROOT." >&2
  exit 1
fi

ARCH=x86_64
if [[ "$(uname -m)" == "arm64" ]]; then
  ARCH=arm64-v8a
fi
PKG="system-images;android-${API_LEVEL};google_apis;${ARCH}"

if "$AVDMANAGER" list avd 2>/dev/null | grep -q "Name: ${AVD_NAME}"; then
  if [[ "$FORCE" -eq 0 ]]; then
    echo "AVD ${AVD_NAME} already exists (use --force to recreate)."
    exit 0
  fi
  echo "no" | "$AVDMANAGER" delete avd -n "$AVD_NAME" || true
fi

yes | "$SDKMANAGER" --sdk_root="$SDK" "$PKG" "platforms;android-${API_LEVEL}" || true

echo "no" | "$AVDMANAGER" create avd \
  -n "$AVD_NAME" \
  -k "$PKG" \
  -d "pixel_c" \
  --force

# Patch config for UniRC 10 Pro panel
AVD_DIR="${ANDROID_AVD_HOME:-$HOME/.android/avd}/${AVD_NAME}.avd"
CONFIG="$AVD_DIR/config.ini"
if [[ ! -f "$CONFIG" ]]; then
  echo "Error: expected $CONFIG after create" >&2
  exit 1
fi

# Ensure / rewrite key hardware properties (append overrides; later keys win for emulator)
{
  echo "hw.lcd.width=1920"
  echo "hw.lcd.height=1200"
  echo "hw.lcd.density=224"
  echo "hw.initialOrientation=landscape"
  echo "hw.keyboard=yes"
} >> "$CONFIG"

echo "Created/updated AVD ${AVD_NAME} (${ARCH}, API ${API_LEVEL}, 1920x1200)."
```

Make executable: `chmod +x tools/simulation/avd/create-unirc-10-pro-avd.sh`.

- [ ] **Step 2: Dry-run validation**

```bash
shellcheck tools/simulation/avd/create-unirc-10-pro-avd.sh
# If SDK present:
# ./tools/simulation/avd/create-unirc-10-pro-avd.sh
# Expected: success or clear missing-SDK error; second run prints "already exists"
```

- [ ] **Step 3: Commit**

```bash
git add tools/simulation/avd/create-unirc-10-pro-avd.sh
git commit -m "$(cat <<'EOF'
feat(simulation): add UniRC 10 Pro Android AVD create script

EOF
)"
```

---

### Task 3: `run-qgc.sh --android-emulator`

**Files:**
- Modify: `run-qgc.sh`

**Interfaces:**
- Consumes: APK at `build/Android-debug/android-build/QGroundControl.apk`, `adb`
- Produces: install + launch on first `emulator-*` device with state `device`

- [ ] **Step 1: Extend mode parsing and install logic**

Update header comments to document `--android-emulator`.

In the arg loop, add `--android-emulator) MODE="android-emulator" ;;`.

Extract shared install/launch into a function used by both USB and emulator modes:

```bash
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
    echo "Install failed (likely signature mismatch). Uninstalling and retrying..." >&2
    adb -s "$serial" uninstall org.mavlink.qgroundcontrol || true
    adb -s "$serial" install -r -d "$apk"
  fi
  adb -s "$serial" shell am force-stop org.mavlink.qgroundcontrol
  adb -s "$serial" shell am start -n org.mavlink.qgroundcontrol/org.mavlink.qgroundcontrol.QGCActivity
  echo "QGC launched on $serial"
}

pick_android_serial() {
  local prefer_emulator="$1" # 1 = emulator only, 0 = first device
  if [[ "$prefer_emulator" -eq 1 ]]; then
    adb devices | awk 'NR>1 && $1 ~ /^emulator-/ && $2=="device" {print $1; exit}'
  else
    adb devices | awk 'NR>1 && $2=="device" {print $1; exit}'
  fi
}
```

For `MODE=android` keep USB/first-device behavior via `pick_android_serial 0`.  
For `MODE=android-emulator` use `pick_android_serial 1`; if empty, print that no emulator is online and exit 1.

Source `env-qgc.sh` for both Android modes so `adb` is on PATH.

- [ ] **Step 2: Manual check (emulator running)**

```bash
./run-qgc.sh --android-emulator
# Expected without emulator: clear error
# Expected with emulator + APK: install + launch
```

- [ ] **Step 3: Commit**

```bash
git add run-qgc.sh
git commit -m "$(cat <<'EOF'
feat(tools): add --android-emulator install path to run-qgc.sh

EOF
)"
```

---

### Task 4: Orchestrator `run-android-sitl.sh` + `just android-sitl`

**Files:**
- Create: `tools/simulation/run-android-sitl.sh`
- Modify: `justfile` (Run & Deploy section)

**Interfaces:**
- Consumes: Task 1–3 scripts
- Produces: end-to-end SITL + emulator + reverse + QGC launch

- [ ] **Step 1: Write orchestrator**

```bash
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
    --build) BUILD_APK=1; shift ;;
    --no-emulator) NO_EMULATOR=1; shift ;;
    --with-latency) WITH_LATENCY=1; shift ;;
    --force-avd) FORCE_AVD=1; shift ;;
    -h|--help) sed -n '2,5p' "$0"; exit 0 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
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
# Wait until boot completed
for _ in $(seq 1 60); do
  boot="$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
  if [[ "$boot" == "1" ]]; then
    break
  fi
  sleep 2
done

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
```

`chmod +x tools/simulation/run-android-sitl.sh`.

- [ ] **Step 2: Add `just` recipe**

In `justfile` under Run & Deploy:

```just
# DodecaHexa SITL + UniRC 10 Pro emulator + QGC APK
# Extra args: just android-sitl --build
android-sitl *ARGS:
    ./tools/simulation/run-android-sitl.sh {{ARGS}}
```

- [ ] **Step 3: shellcheck orchestrator + help path**

```bash
shellcheck tools/simulation/run-android-sitl.sh
./tools/simulation/run-android-sitl.sh --help
# Expected: usage lines, exit 0
just --list | grep android-sitl
```

- [ ] **Step 4: Commit**

```bash
git add tools/simulation/run-android-sitl.sh justfile
git commit -m "$(cat <<'EOF'
feat(simulation): orchestrate Android emulator with DodecaHexa SITL

EOF
)"
```

---

### Task 5: Documentation

**Files:**
- Modify: `tools/simulation/README.md`
- Modify: `tools/README.md` (optional one-line pointer under simulation section if present)

- [ ] **Step 1: Extend `tools/simulation/README.md`**

Add sections after ArduCopter SITL:

1. **DodecaHexa X** — `./run-arducopter-sitl.sh --frame dodeca-hexa`, parm file note, desktop connect.
2. **Android + UniRC 10 Pro** — prerequisites (Docker, Android SDK, Qt Android kit / `env-qgc.sh`), ABI note (arm64 vs x86_64), commands:

```bash
# One-shot (APK must already exist, or pass --build)
just android-sitl
just android-sitl --build
just android-sitl --no-emulator   # SITL only
```

3. Comm link instructions (`127.0.0.1:5760` vs `10.0.2.2:5760`).
4. Success check: Motor Setup shows DodecaHexa X / 12 motors.

Update Quick Start table with `run-android-sitl.sh` row.

- [ ] **Step 2: Commit**

```bash
git add tools/simulation/README.md tools/README.md
git commit -m "$(cat <<'EOF'
docs(simulation): document DodecaHexa Android SITL workflow

EOF
)"
```

---

### Task 6: End-to-end verification

**Files:** none (manual / operator)

- [ ] **Step 1: Prerequisites check**

```bash
docker info >/dev/null
source ./env-qgc.sh
test -d "$ANDROID_SDK_ROOT"
test -f build/Android-debug/android-build/QGroundControl.apk || echo "Need APK or use --build"
```

- [ ] **Step 2: Run full path**

```bash
just android-sitl
# or: just android-sitl --build
```

- [ ] **Step 3: Verify in emulator QGC**

1. Add TCP Comm Link `127.0.0.1:5760`, connect.
2. Confirm vehicle online; parameters `FRAME_CLASS=12`, `FRAME_TYPE=1` (or UI shows DodecaHexa).
3. Vehicle Setup → Motors → coaxial DodecaHexa X diagram with 12 motors (A–L).

- [ ] **Step 4: Verify desktop still works**

```bash
# With SITL still up from orchestrator or:
./tools/simulation/run-arducopter-sitl.sh --frame dodeca-hexa
# Desktop QGC → tcp://localhost:5760
```

No commit required unless verification finds bugs; then fix in a follow-up commit on the relevant task files.

---

## Spec coverage checklist

| Spec ID | Task |
|---------|------|
| R1 FRAME_CLASS/TYPE | Task 1 |
| R2 model dodeca-hexa | Task 1 |
| R3 local APK install | Task 3, 4 |
| R4 UniRC AVD | Task 2, 4 |
| R5 adb reverse / fallback | Task 4, 5 |
| R6 just / orchestrator | Task 4 |
| R7 desktop same SITL | Task 1, 6 |
| Docs | Task 5 |
| Error handling | Tasks 1–4 messages |

## Out of scope (do not implement)

- Auto-writing QGC Comm Link settings inside the APK
- PX4 / CI emulator job
- Motor UI code changes
