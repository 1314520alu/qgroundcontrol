#!/usr/bin/env bash
# ArduCopter SITL Test Environment
# Prefers Docker; falls back to native ~/ardupilot SITL when Docker is unavailable.
#
# Usage: ./run-arducopter-sitl.sh [--frame +|dodeca-hexa] [--with-latency]
#
# Options:
#   --frame +|dodeca-hexa  Physics/frame model (default: +)
#   --with-latency         Simulate 100ms RTT (Docker only)
# Env:
#   ARDUPILOT_HOME         Native ArduPilot tree (default: $HOME/ardupilot)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COPTER_VERSION="Copter-4.5.6"
IMAGE_NAME="ardupilot-sitl-4.5.6"
CONTAINER_NAME="arducopter-sitl"
NATIVE_PID_FILE="/tmp/qgc-arducopter-sitl.pid"
NATIVE_LOG="/tmp/qgc-arducopter-sitl.log"
ARDUPILOT_HOME="${ARDUPILOT_HOME:-$HOME/ardupilot}"

FRAME="+"
WITH_LATENCY=0

usage() {
    sed -n '2,12p' "$0"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --with-latency)
            WITH_LATENCY=1
            shift
            ;;
        --frame)
            if [[ $# -lt 2 ]]; then
                echo "Error: --frame requires a value (e.g. dodeca-hexa)" >&2
                exit 1
            fi
            FRAME="$2"
            shift 2
            ;;
        --frame=*)
            FRAME="${1#--frame=}"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1 (try --help)" >&2
            exit 1
            ;;
    esac
done

case "$FRAME" in
    +|dodeca-hexa) ;;
    *)
        echo "Unsupported --frame: $FRAME (use + or dodeca-hexa)" >&2
        exit 1
        ;;
esac

MODEL="$FRAME"

docker_available() {
    # Finder / desktop shortcuts often lack Docker Desktop's CLI path.
    if [[ -x "/Applications/Docker.app/Contents/Resources/bin/docker" ]]; then
        export PATH="/Applications/Docker.app/Contents/Resources/bin:$PATH"
    fi
    command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1
}

stop_native_sitl() {
    if [[ -f "$NATIVE_PID_FILE" ]]; then
        local old
        old="$(cat "$NATIVE_PID_FILE" 2>/dev/null || true)"
        if [[ -n "${old:-}" ]] && kill -0 "$old" 2>/dev/null; then
            kill "$old" 2>/dev/null || true
            sleep 1
            kill -9 "$old" 2>/dev/null || true
        fi
        rm -f "$NATIVE_PID_FILE"
    fi
    # Also clear any leftover listener on 5760 from prior runs
    if command -v lsof >/dev/null 2>&1; then
        local pids
        pids="$(lsof -tiTCP:5760 -sTCP:LISTEN 2>/dev/null || true)"
        if [[ -n "${pids:-}" ]]; then
            # shellcheck disable=SC2086
            kill $pids 2>/dev/null || true
            sleep 0.5
            # shellcheck disable=SC2086
            kill -9 $pids 2>/dev/null || true
        fi
    fi
}

start_native_sitl() {
    local bin="$ARDUPILOT_HOME/build/sitl/bin/arducopter"
    local base_parm="$ARDUPILOT_HOME/Tools/autotest/default_params/copter.parm"
    local defaults="$base_parm"

    if [[ ! -x "$bin" ]]; then
        echo "Error: native SITL binary not found: $bin" >&2
        echo "Build it once:" >&2
        echo "  cd \"$ARDUPILOT_HOME\" && ./waf configure --board sitl && ./waf copter" >&2
        echo "Or install/start Docker Desktop and re-run (Docker path)." >&2
        exit 1
    fi
    if [[ ! -f "$base_parm" ]]; then
        echo "Error: missing $base_parm" >&2
        exit 1
    fi

    if [[ "$FRAME" == "dodeca-hexa" ]]; then
        defaults="${defaults},${SCRIPT_DIR}/params/copter-dodecahexa-x.parm"
    fi

    if [[ "$WITH_LATENCY" -eq 1 ]]; then
        echo "Warning: --with-latency is Docker-only; ignored for native SITL." >&2
    fi

    stop_native_sitl

    echo ""
    echo "Starting native SITL frame=$FRAME (no Docker)..."
    echo "Binary: $bin"
    echo "Log:    $NATIVE_LOG"
    echo ""

    nohup "$bin" -S \
        --model "$MODEL" \
        --speedup 1 \
        --defaults "$defaults" \
        --home 42.3898,-71.1476,14.0,270.0 \
        --serial0 tcp:0:5760:wait \
        >"$NATIVE_LOG" 2>&1 &
    echo $! >"$NATIVE_PID_FILE"
    sleep 2

    if ! kill -0 "$(cat "$NATIVE_PID_FILE")" 2>/dev/null; then
        echo "Error: native SITL failed to start. Last log lines:" >&2
        tail -30 "$NATIVE_LOG" >&2 || true
        exit 1
    fi

    echo "============================================"
    echo "ArduCopter native SITL is running!"
    echo "Frame: $FRAME (model $MODEL)"
    echo "PID:   $(cat "$NATIVE_PID_FILE")"
    echo "============================================"
    echo ""
    echo "Connect QGC to: tcp://localhost:5760"
    echo "Stop: kill \$(cat $NATIVE_PID_FILE)"
    echo ""
}

start_docker_sitl() {
    local defaults="/ardupilot/Tools/autotest/default_params/copter.parm"
    local volume_args=()
    if [[ "$FRAME" == "dodeca-hexa" ]]; then
        defaults="${defaults},/qgc-params/copter-dodecahexa-x.parm"
        volume_args+=(-v "${SCRIPT_DIR}/params:/qgc-params:ro")
    fi

    if ! docker image inspect "$IMAGE_NAME" &> /dev/null; then
        echo "Building ArduCopter $COPTER_VERSION SITL image..."
        echo "This may take 10-15 minutes on first run..."
        # Apple Silicon: base image is amd64-only — build/run under qemu.
        docker build --platform linux/amd64 --tag "$IMAGE_NAME" \
            --build-arg COPTER_TAG="$COPTER_VERSION" \
            https://github.com/radarku/ardupilot-sitl-docker.git
    fi

    docker rm -f "$CONTAINER_NAME" 2>/dev/null || true
    stop_native_sitl

    local docker_cmd=(docker run -d --platform linux/amd64 --name "$CONTAINER_NAME" -p 5760:5760 "${volume_args[@]}")
    local ardupilot_args=(
        -S
        --model "$MODEL"
        --speedup 1
        --defaults "$defaults"
        --home 42.3898,-71.1476,14.0,270.0
        --serial0 tcp:0:5760:wait
    )
    local ardupilot_bin="/ardupilot/build/sitl/bin/arducopter"
    local ardupilot_cmd="$ardupilot_bin ${ardupilot_args[*]}"

    if [[ "$WITH_LATENCY" -eq 1 ]]; then
        echo ""
        echo "Starting Docker SITL frame=$FRAME with simulated latency (100ms round-trip)..."
        echo ""
        "${docker_cmd[@]}" --cap-add=NET_ADMIN --entrypoint /bin/bash "$IMAGE_NAME" \
            -c "tc qdisc add dev eth0 root netem delay 50ms 10ms 2>/dev/null || true; exec $ardupilot_cmd"
    else
        echo ""
        echo "Starting Docker SITL frame=$FRAME (no latency simulation)..."
        echo ""
        "${docker_cmd[@]}" --entrypoint "$ardupilot_bin" "$IMAGE_NAME" "${ardupilot_args[@]}"
    fi

    sleep 3

    if docker ps --filter name="$CONTAINER_NAME" --format "{{.Status}}" | grep -q "Up"; then
        echo ""
        echo "============================================"
        echo "ArduCopter $COPTER_VERSION SITL is running!"
        echo "Frame: $FRAME (model $MODEL)"
        echo "============================================"
        echo ""
        echo "Connect QGC to: tcp://localhost:5760"
        echo "  View logs:  docker logs -f $CONTAINER_NAME"
        echo "  Stop SITL:  docker stop $CONTAINER_NAME"
        echo ""
    else
        echo "Error: SITL container failed to start"
        echo "Logs:"
        docker logs "$CONTAINER_NAME" 2>&1 | tail -20
        exit 1
    fi
}

if docker_available; then
    start_docker_sitl
else
    echo "Docker not available — using native SITL at $ARDUPILOT_HOME"
    start_native_sitl
fi
