#!/usr/bin/env bash
# ArduCopter SITL Test Environment
# Runs ArduCopter in Docker for testing QGC without hardware
#
# Usage: ./run-arducopter-sitl.sh [--frame +|dodeca-hexa] [--with-latency]
#
# Options:
#   --frame +|dodeca-hexa  Physics/frame model (default: +)
#   --with-latency         Simulate 100ms round-trip latency (Herelink-like)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COPTER_VERSION="Copter-4.5.6"
IMAGE_NAME="ardupilot-sitl-4.5.6"
CONTAINER_NAME="arducopter-sitl"

FRAME="+"
WITH_LATENCY=0

usage() {
    sed -n '2,10p' "$0"
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
DEFAULTS="/ardupilot/Tools/autotest/default_params/copter.parm"
VOLUME_ARGS=()
if [[ "$FRAME" == "dodeca-hexa" ]]; then
    DEFAULTS="${DEFAULTS},/qgc-params/copter-dodecahexa-x.parm"
    VOLUME_ARGS+=(-v "${SCRIPT_DIR}/params:/qgc-params:ro")
fi

if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

if ! docker image inspect "$IMAGE_NAME" &> /dev/null; then
    echo "Building ArduCopter $COPTER_VERSION SITL image..."
    echo "This may take 10-15 minutes on first run..."
    docker build --tag "$IMAGE_NAME" \
        --build-arg COPTER_TAG="$COPTER_VERSION" \
        https://github.com/radarku/ardupilot-sitl-docker.git
fi

docker rm -f "$CONTAINER_NAME" 2>/dev/null || true

DOCKER_CMD=(docker run -d --name "$CONTAINER_NAME" -p 5760:5760 "${VOLUME_ARGS[@]}")
ARDUPILOT_ARGS=(
    -S
    --model "$MODEL"
    --speedup 1
    --defaults "$DEFAULTS"
    --home 42.3898,-71.1476,14.0,270.0
    --serial0 tcp:0:5760:wait
)
ARDUPILOT_BIN="/ardupilot/build/sitl/bin/arducopter"
ARDUPILOT_CMD="$ARDUPILOT_BIN ${ARDUPILOT_ARGS[*]}"

if [[ "$WITH_LATENCY" -eq 1 ]]; then
    echo ""
    echo "Starting SITL frame=$FRAME with simulated latency (100ms round-trip)..."
    echo "This simulates Herelink-like network conditions."
    echo ""

    "${DOCKER_CMD[@]}" --cap-add=NET_ADMIN --entrypoint /bin/bash "$IMAGE_NAME" \
        -c "tc qdisc add dev eth0 root netem delay 50ms 10ms 2>/dev/null || true; exec $ARDUPILOT_CMD"
else
    echo ""
    echo "Starting SITL frame=$FRAME (no latency simulation)..."
    echo "Use --with-latency to simulate Herelink network conditions."
    echo ""

    "${DOCKER_CMD[@]}" --entrypoint "$ARDUPILOT_BIN" "$IMAGE_NAME" "${ARDUPILOT_ARGS[@]}"
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
    echo ""
    echo "Test procedure:"
    echo "  1. Open QGC and connect to localhost:5760"
    echo "  2. Go to Plan view"
    echo "  3. Create a mission with waypoints (no geofence)"
    echo "  4. Click Upload"
    echo "  5. Verify no 'Geofence Transfer failed' error appears"
    echo ""
    echo "Commands:"
    echo "  View logs:  docker logs -f $CONTAINER_NAME"
    echo "  Stop SITL:  docker stop $CONTAINER_NAME"
    echo "  Remove:     docker rm $CONTAINER_NAME"
    echo ""
else
    echo "Error: SITL container failed to start"
    echo "Logs:"
    docker logs "$CONTAINER_NAME" 2>&1 | tail -20
    exit 1
fi
