# UniPod MT11 资料包

锐川（Reebot）迷你四光 AI 吊舱 **UniPod MT11** 的官方 PDF、整理手册与可查询数据库。

## 快速入口

| 整理手册 | 数据库 |
|----------|--------|
| [用户说明书整理](UniPod-MT11-User-Manual-Handbook.md) | [db/manual.db](db/manual.db) / [manual.json](db/manual.json) |
| [外部 SDK 整理](UniPod-MT11-SDK-Handbook.md) | [db/sdk.db](db/sdk.db) / [sdk.json](db/sdk.json) |

## 官方 PDF

| 文件 | 版本 / 日期 |
|------|-------------|
| [UniPod-MT11-User-Manual-V2.0-CN.pdf](UniPod-MT11-User-Manual-V2.0-CN.pdf) | 用户手册中文 V2.0（2026-06-04） |
| [UniPod-MT11-User-Manual-V2.0-EN.pdf](UniPod-MT11-User-Manual-V2.0-EN.pdf) | 用户手册英文 V2.0（2026-06-04） |
| [UniPod-MT11-User-Manual-v1.2.pdf](UniPod-MT11-User-Manual-v1.2.pdf) | 英文 v1.2 归档 |
| [UniPod-MT11-FAQ.pdf](UniPod-MT11-FAQ.pdf) | 常见问题（2026-04-07） |
| [UniPod-MT11-Firmware-Update-Log-2026-07-27-CN.pdf](UniPod-MT11-Firmware-Update-Log-2026-07-27-CN.pdf) | 固件更新记录中文 |
| [UniPod-MT11-Firmware-Update-Log-2026-07-27-EN.pdf](UniPod-MT11-Firmware-Update-Log-2026-07-27-EN.pdf) | 固件更新记录英文 |
| [UniPod-MT11-SDK-V0.1.0.pdf](UniPod-MT11-SDK-V0.1.0.pdf) | 外部 SDK 协议（已整理；官网另有 V0.2.3） |
| [UniPod-MT11-Web-Server.pdf](UniPod-MT11-Web-Server.pdf) | Web Server 媒体接口 V1.0 |

## 官方下载页

- 中文：https://www.reebot.com/index.php?asd=491&id=downloads2  
- 英文：https://reebot.com/en/index.php?asd=481&id=downloads2  

## QGC 对接摘要

| 用途 | 地址 |
|------|------|
| 主码流 RTSP | `rtsp://192.168.144.25:8554/video1` |
| 副码流 RTSP | `rtsp://192.168.144.25:8554/video2` |
| 副 IP 出图 | `rtsp://192.168.144.80:8554/video1` |
| SDK UDP/TCP | `192.168.144.25:37260` |

视频源选项：`UniPod MT11`（`VideoSettings`）。

## 查询示例

```bash
# 说明书规格 / FAQ
sqlite3 docs/manuals/unipod-mt11/db/manual.db \
  "SELECT category, name, value FROM specs WHERE category='整体';"
sqlite3 docs/manuals/unipod-mt11/db/manual.db \
  "SELECT q_no, question FROM faq LIMIT 10;"

# SDK 命令
sqlite3 docs/manuals/unipod-mt11/db/sdk.db \
  "SELECT cmd_hex, name, category FROM commands ORDER BY cmd_id;"
```
