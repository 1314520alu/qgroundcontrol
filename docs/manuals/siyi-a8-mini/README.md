# SIYI A8 Mini 资料包

思翼（SIYI）**A8 mini** 4K AI 迷你变焦云台相机 — 官方用户手册与外部 SDK 协议（用于 QGC / 电台以太网对接开发）。

官网下载页：

- 中文：<https://siyi.biz/zh/product/tri-axis-single-camera-gimbal/a8-mini/download/>
- 英文：<https://siyi.biz/en/product/tri-axis-single-camera-gimbal/a8-mini/download/>

## 官方 PDF

| 文件 | 版本 / 说明 |
| --- | --- |
| [SIYI-A8-Mini-User-Manual-V1.10-CN.pdf](SIYI-A8-Mini-User-Manual-V1.10-CN.pdf) | 用户手册中文 V1.10（2026-07-02） |
| [SIYI-A8-Mini-User-Manual-V1.10-EN.pdf](SIYI-A8-Mini-User-Manual-V1.10-EN.pdf) | 用户手册英文 V1.10 |
| [SIYI-Gimbal-Camera-External-SDK-Protocol-V0.1.1-CN.pdf](SIYI-Gimbal-Camera-External-SDK-Protocol-V0.1.1-CN.pdf) | 云台相机外部 SDK 协议中文 V0.1.1 |
| [SIYI-Gimbal-Camera-External-SDK-Protocol-V0.1.1-EN.pdf](SIYI-Gimbal-Camera-External-SDK-Protocol-V0.1.1-EN.pdf) | 外部 SDK 协议英文 V0.1.1（官网英文页文件名带 Update_Log，正文为完整协议） |

## QGC / 电台以太网对接摘要

默认依赖思翼图数传网段 `192.168.144.x`（与 MK15/MK32 / UniRC 一致）。

| 用途 | 地址 |
| --- | --- |
| 相机默认 IP | `192.168.144.25` |
| SDK UDP | `192.168.144.25:37260` |
| SDK TCP | `192.168.144.25:37260`（需 TCP 心跳 `0x00`） |
| 主码流 RTSP（旧地址，A8 Mini） | `rtsp://192.168.144.25:8554/main.264` |
| 副码流 / 新地址（ZT30+） | `rtsp://…/video1`、`…/video2`（A8 不用） |
| QGC 视频源 | **SIYI A8 Mini**（自动填 `main.264` + 等 `192.168.144.x`） |
| Web 媒体 | `http://192.168.144.25:82/cgi-bin/media.cgi`（协议文档） |
| 硬件 ID | `0x73` = A8 Mini |

常用网段其它地址（手册 §4.6）：空中端 `.11`，地面端 `.12`，手持 Android `.20`。

帧格式：`55 66` 头 + CMD + DATA_LEN + SEQ + CMD_ID + DATA + CRC16（低字节在前）。拍照/录像等见 CMD `0x0C`；姿态 `0x0D`/`0x0E`；转向 `0x07`；回中 `0x08`。

> 说明：本仓库已有 UniPod MT11 对接也走 `192.168.144.25:37260` 同类帧；A8 Mini 为思翼原生协议机型（硬件 ID `0x73`），开发时以本目录 SDK PDF 为准。

UniGCS 浮层能力表（产品功能 vs 本轮按钮）：[../payload-capabilities/](../payload-capabilities/)。

## 建议开发阅读顺序

1. 用户手册 §2 接线 / §3 控制方式 / §4.6 IP 与 RTSP  
2. 外部 SDK 协议：通信接口 + CMD 表 + CRC16  
3. 在 G20 / UniRC 上确认 `192.168.144.x` 就绪后再起 RTSP / UDP
