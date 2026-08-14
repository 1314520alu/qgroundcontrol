# QGC Simulation Tools

Tools for testing QGroundControl without physical hardware.

## Quick Start

| Tool | Use Case | Setup |
| --- | --- | --- |
| `mock_vehicle.py` | UI testing, quick checks | `pip install pymavlink` |
| `run-arducopter-sitl.sh` | Full simulation, mission testing | Docker required |
| `run-android-sitl.sh` / `just android-sitl` | DodecaHexa SITL + UniRC 10 Pro emulator + QGC APK | Docker + Android SDK + Android-debug APK |

## Mock Vehicle (Lightweight)

A minimal MAVLink simulator for UI testing. Does not simulate flight dynamics.

```bash
# Install
pip install pymavlink

# Run (QGC connects to UDP 14550)
./mock_vehicle.py

# Multiple vehicles
./mock_vehicle.py --system-id 1 --port 14550 &
./mock_vehicle.py --system-id 2 --port 14551 &

# TCP mode (SITL-style)
./mock_vehicle.py --tcp --port 5760
```

**Features:**

- Heartbeat, GPS, attitude, battery telemetry
- Arm/disarm commands
- Mode changes
- Basic parameter support

**Options:**

| Option | Default | Description |
| --- | --- | --- |
| `--host` | 127.0.0.1 | Host address |
| `--port` | 14550 | Port number |
| `--tcp` | off | Use TCP instead of UDP |
| `--system-id` | 1 | MAVLink system ID |
| `--rate` | 10 | Telemetry rate (Hz) |
| `--sitl` | off | SITL-compatible mode (TCP:5760) |

**Limitations:** No flight dynamics, no mission execution, limited MAVLink support.

## ArduCopter SITL (Full Simulation)

Full ArduPilot simulation via Docker. Supports missions, geofences, all commands.

```bash
# Run (builds image on first run, ~10-15 min) — default frame +
./run-arducopter-sitl.sh

# DodecaHexa X (6 arms, 12 coaxial motors; FRAME_CLASS=12, FRAME_TYPE=1)
./run-arducopter-sitl.sh --frame dodeca-hexa

# With simulated network latency (Herelink-like)
./run-arducopter-sitl.sh --frame dodeca-hexa --with-latency

# Connect QGC to: tcp://localhost:5760
```

DodecaHexa overrides are in `params/copter-dodecahexa-x.parm` (mounted into the container).

**Docker Commands:**

```bash
docker logs -f arducopter-sitl   # View logs
docker stop arducopter-sitl      # Stop simulation
docker rm arducopter-sitl        # Remove container
```

**Requirements:** Docker

Mock Vehicle auto-connects over UDP 14550. For SITL, add a TCP comm link in QGC
(**Application Settings → Comm Links → Add**, Host `localhost`, Port `5760`).

## Android + UniRC 10 Pro (DodecaHexa)

One-command path for landscape GCS UI against a 12-motor coaxial SITL:

```bash
# Prerequisites: Docker, Android SDK (emulator + API 33 image), Qt Android kit (env-qgc.sh),
# and either an existing Android-debug APK or --build after configure.

just android-sitl
just android-sitl --build          # cmake --build build/Android-debug first
just android-sitl --no-emulator    # SITL only (desktop QGC)
just android-sitl --force-avd      # recreate UniRC AVD
```

Or: `./tools/simulation/run-android-sitl.sh` with the same flags.

**AVD:** `unirc-10-pro` — 1920×1200 landscape, ~224 dpi, Android 13 (API 33), matching SIYI UniRC 10 Pro.

**ABI:** Default `QT_ANDROID_ABIS=arm64-v8a` (see `env-qgc.sh`) matches Apple Silicon emulators. On Intel Macs build with `QT_ANDROID_ABIS=x86_64`.

**Network:** The orchestrator runs `adb reverse tcp:5760 tcp:5760`. In QGC on the emulator, add a TCP Comm Link to **127.0.0.1:5760**. If reverse fails, use **10.0.2.2:5760**.

**Success check:** Connect → Vehicle Setup → Motors → DodecaHexa X coaxial diagram (12 motors A–L).

Related helpers:

```bash
./tools/simulation/avd/create-unirc-10-pro-avd.sh   # create AVD only
./run-qgc.sh --android-emulator                     # install/launch APK on running emulator
```

## Comparison

| Feature | mock_vehicle.py | ArduCopter SITL |
| --- | --- | --- |
| Setup time | Seconds | 10-15 min (first run) |
| Dependencies | pymavlink | Docker |
| Flight dynamics | ❌ | ✓ |
| Mission execution | ❌ | ✓ |
| Geofence support | ❌ | ✓ |
| All MAVLink commands | ❌ | ✓ |
| Resource usage | Low | Medium |
| Best for | UI testing | Integration testing |

## Other Simulators

For more advanced simulation:

- [ArduPilot SITL](https://ardupilot.org/dev/docs/sitl-simulator-software-in-the-loop.html) - Native install
- [PX4 SITL](https://docs.px4.io/main/en/simulation/) - PX4 simulation
- [Gazebo](https://gazebosim.org/) - 3D physics simulation
