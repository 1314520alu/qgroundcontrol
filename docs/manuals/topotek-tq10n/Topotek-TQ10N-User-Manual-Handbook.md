# Topotek TQ10N — User Manual Handbook

> Aggregated from Topotek official OSS docs, ArduPilot Topotek integration notes, and QGC fork wiring. No standalone TQ10N PDF on topotek.cn; MINI-series connection manual applies.

## Product

| Item | Value |
|------|-------|
| Model | **TQ10N** |
| Vendor | Topotek / 拓扑联创 |
| Zoom | 10× optical |
| Network | Static IP on payload Ethernet |
| Storage | TF card (onboard photo/video) |

## Network defaults

| Service | URI / address |
|---------|----------------|
| Default IP | `192.168.144.108` |
| RTSP main (1080p) | `rtsp://192.168.144.108:554/stream=0` |
| RTSP sub | `rtsp://192.168.144.108:554/stream=1` |
| UDP control | Pod `:9003`, GCS bind `:9004` |

On Skydroid G20 / SIYI MK32/MK15 remotes, the radio Ethernet segment is typically `192.168.144.0/24`. QGC calls `ensureSiyiRadioEthernet()` before opening RTSP (same gate as UniPod MT11 / SIYI A8).

## Video in QGroundControl

1. **Settings → Video → Video Source** → **Topotek TQ10N**
2. RTSP URL is preset; Connection URL field is hidden for preset sources.
3. Fly View shows live RTSP via GStreamer when the link is up.

## Onboard capture (Phase-1)

- **Photo** → TF card still (UDP `CAP`)
- **Video** → TF card H.264 (`REC` toggle); not local GStreamer record
- **Zoom** → hold `+`/`-` (UDP `ZMC` in/out); release sends stop
- **Gimbal** → hold direction on flat D-pad (UDP `PTZ` 01–04); release stop; center tap → home (05)

## Wiring

See [pdf/TOPOTEK-Wiring-Instruction.docx](pdf/TOPOTEK-Wiring-Instruction.docx) and [pdf/TOPOTEK-Connection-Manual-MINI-10P.docx](pdf/TOPOTEK-Connection-Manual-MINI-10P.docx) for power, Ethernet, and 10P terminal pinout.

## Out of scope (this fork)

- SMB media browser `\\192.168.144.108` — not implemented in Phase-1
- UART control path (address `U`) — network `P` only in QGC

## References

- [Topotek-TQ10N-SDK-Handbook.md](Topotek-TQ10N-SDK-Handbook.md)
- [pdf/Topotek-Udp-Uart-Protocol-V1.1.3-EN.pdf](pdf/Topotek-Udp-Uart-Protocol-V1.1.3-EN.pdf)
- [pdf/QGC-VLC-POTPLAYER.docx](pdf/QGC-VLC-POTPLAYER.docx)
