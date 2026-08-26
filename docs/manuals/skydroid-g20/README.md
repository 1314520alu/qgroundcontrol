# Skydroid G20 资料包

云卓（Skydroid）G20 地面站遥控器说明书。

## 官方 PDF

| 文件 | 版本 / 说明 |
| --- | --- |
| [Skydroid-G20-User-Manual-V1.0-EN.pdf](Skydroid-G20-User-Manual-V1.0-EN.pdf) | 用户手册英文 V1.0（23 页，约 1.6 MB） |

来源：[Airmobi](https://www.airmobi.com/wp-content/uploads/2025/04/Skydroid-G20-Ground-Control-Station-User-Manual.pdf)（原文件名 `G20说明书v1.0-英文`）。公开渠道未找到独立中文 PDF。

## QGC 数传对接摘要

电台以太网（图传/吊舱）打开后，数传走同一 `192.168.144.x` 网段，**不是**本机 `127.0.0.1` 桥：

| 用途 | 值 |
| ------ | ----- |
| 链路类型 | UDP |
| QGC 监听 | `14550` |
| 发送目标 | `192.168.144.101:14550` |
| 预设名 | `云卓 G20` / `云卓 H30`（`LinkConfigurationManager.qml`） |

旧手册里的 `14551` ↔ `127.0.0.1:14552` 是本机桥路径；OEM 以太网 NetworkAgent 起来后该桥会停，只剩 `144.101:14550`。图传/吊舱仍用同网段 RTSP（如 `192.168.144.108`）。
