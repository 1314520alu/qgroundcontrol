# Topotek TQ10N — SDK Handbook (Phase-1)

> From **Udp&uart-Protocol V1.1.3** and official `pythonSampleCode/topotekcmdparse.py`.

## Frame format (`#TP`)

| Field | Size | Example (network) |
|-------|------|-------------------|
| Header | 3 | `#TP` |
| Source | 1 | `P` (network), `U` (UART) |
| Dest | 1 | `D` camera, `G` gimbal, `M` zoom motor |
| Length | 1 | ASCII `2` (fixed for `#TP`) |
| Ctrl | 1 | `w` write, `r` read |
| Ident | 3 | e.g. `CAP`, `REC`, `PTZ`, `ZMC` |
| Data | var | ASCII payload |
| CRC | 2 | Sum of all preceding bytes `& 0xFF`, as 2 uppercase hex ASCII |

### CRC (matches Python)

```python
crc_val = sum(command_bytes_before_crc) & 0xFF
crc_ascii = f"{crc_val:02X}"  # appended to frame
```

## Phase-1 commands (source `P`)

| Function | Dest | Ident | Ctrl | Data | Example (ASCII) |
|----------|------|-------|------|------|-----------------|
| Photo | D | CAP | w | `0` | `#TPPD2wCAP008` |
| Record toggle | D | REC | w | `0A` | `#TPPD2wREC0A4F` |
| Record query | D | REC | r | `0` | `#TPPD2rREC009` |
| Zoom in | M | ZMC | w | `02` | `#TPPM2wZMC0259` |
| Zoom out | M | ZMC | w | `01` | `#TPPM2wZMC0158` |
| Zoom stop | M | ZMC | w | `00` | `#TPPM2wZMC0057` |
| PTZ up | G | PTZ | w | `01` | `#TPPG2wPTZ0166` |
| PTZ down | G | PTZ | w | `02` | … |
| PTZ left | G | PTZ | w | `03` | … |
| PTZ right | G | PTZ | w | `04` | … |
| PTZ home | G | PTZ | w | `05` | `#TPPG2wPTZ056A` |
| PTZ stop | G | PTZ | w | `00` | `#TPPG2wPTZ0065` |

### REC data semantics

| Data | Meaning |
|------|---------|
| `0` | Stop (write) / query (read) |
| `1` | Start |
| `A` | State flip (toggle) — use `0A` as 2-char data field for toggle |

### REC inquiry response

Read response data bit `x2`: `0` = stopped, `1` = recording.

Feedback examples (UART, lower case `#tp`): `#tpDUAwREC11…` start, `#tpDUAwREC00…` stop.

## Transport

| Parameter | Value |
|-----------|-------|
| Pod IP | `192.168.144.108` |
| Pod port | `9003` |
| Local bind | `9004` (official demo) |

## QGC implementation map

| Module | Role |
|--------|------|
| `TopotekTq10Protocol` | Frame build + CRC |
| `TopotekTq10Client` | UDP lifecycle, REC poll ~1 Hz |
| `TopotekTq10CameraControl` | `MavlinkCameraControlInterface` + `ptzStart/Stop/Home` |
| `QGCCameraManager._syncTopotekCamera()` | Start/stop with video source |

## Hex golden values (unit tests)

See [db/sdk.json](db/sdk.json) `examples` — verified against Python `build_command()`.
