# Skydroid G20 资料包

云卓（Skydroid）G20 地面站遥控器说明书。

## 官方 PDF

| 文件 | 版本 / 说明 |
| --- | --- |
| [Skydroid-G20-User-Manual-V1.0-EN.pdf](Skydroid-G20-User-Manual-V1.0-EN.pdf) | 用户手册英文 V1.0（23 页，约 1.6 MB） |

来源：[Airmobi](https://www.airmobi.com/wp-content/uploads/2025/04/Skydroid-G20-Ground-Control-Station-User-Manual.pdf)（原文件名 `G20说明书v1.0-英文`）。公开渠道未找到独立中文 PDF。

## QGC 数传对接摘要

Skydroid 型号有两种数传路径，`LinkConfigurationManager.qml` 会在应用预设时自动选择：

| 机型 | 网络接口 | QGC 监听 | 发送目标 |
| --- | --- | --- | --- |
| **G20 / G16**（AR8030，`ar_net0`） | 本机 UDP 桥 | `14551` | `127.0.0.1:14552` |
| **H30 / H16**（USB 以太网 `eth0` 已起） | 192.168.144.x 直连 | `14550` | `192.168.144.101:14550` |

G20 图传/吊舱仍走 `192.168.144.x` RTSP（如 `192.168.144.25`），与数传桥路径独立。

预设名：`云卓 G20` / `云卓 H30` 等（`LinkConfigurationManager.qml`）。
