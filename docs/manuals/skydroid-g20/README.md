# Skydroid G20 资料包

云卓（Skydroid）G20 地面站遥控器说明书。

## 官方 PDF

| 文件 | 版本 / 说明 |
|------|-------------|
| [Skydroid-G20-User-Manual-V1.0-EN.pdf](Skydroid-G20-User-Manual-V1.0-EN.pdf) | 用户手册英文 V1.0（23 页，约 1.6 MB） |

来源：[Airmobi](https://www.airmobi.com/wp-content/uploads/2025/04/Skydroid-G20-Ground-Control-Station-User-Manual.pdf)（原文件名 `G20说明书v1.0-英文`）。公开渠道未找到独立中文 PDF。

## QGC 数传对接摘要

| 用途 | 值 |
|------|-----|
| 链路类型 | UDP |
| QGC 监听 | `14551` |
| 发送目标 | `127.0.0.1:14552` |
| 预设名 | `云卓 G20`（`LinkConfigurationManager.qml`） |

图传/吊舱走电台以太网 `192.168.144.x`（RTSP），与上述 MAVLink 数传 UDP 桥分开。
