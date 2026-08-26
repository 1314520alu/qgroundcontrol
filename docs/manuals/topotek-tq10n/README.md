# Topotek TQ10N 资料包

拓扑联创（Topotek）**TQ10N** 10× 光学变焦网络吊舱的官方 PDF、Python 示例、整理手册与可查询数据库。

## 快速入口

| 整理手册 | 数据库 |
| --- | --- |
| [用户说明书整理](Topotek-TQ10N-User-Manual-Handbook.md) | [db/manual.db](db/manual.db) / [manual.json](db/manual.json) |
| [外部 SDK 整理](Topotek-TQ10N-SDK-Handbook.md) | [db/sdk.db](db/sdk.db) / [sdk.json](db/sdk.json) |

## 官方文档（已下载）

| 文件 | 说明 |
| ------ | ------ |
| [pdf/Topotek-Udp-Uart-Protocol-V1.1.3-EN.pdf](pdf/Topotek-Udp-Uart-Protocol-V1.1.3-EN.pdf) | UDP/UART 协议 V1.1.3 |
| [pdf/QGC-VLC-POTPLAYER.docx](pdf/QGC-VLC-POTPLAYER.docx) | QGC/VLC 拉流说明 |
| [pdf/TOPOTEK-Wiring-Instruction.docx](pdf/TOPOTEK-Wiring-Instruction.docx) | 接线说明 |
| [pdf/TOPOTEK-Connection-Manual-MINI-10P.docx](pdf/TOPOTEK-Connection-Manual-MINI-10P.docx) | MINI 系列（10P）连接手册 |

Python 官方示例：[pythonSampleCode/](pythonSampleCode/)

## QGC 对接摘要

| 用途 | 地址 |
| --- | --- |
| 主码流 RTSP | `rtsp://192.168.144.108:554/stream=0` |
| 副码流 RTSP | `rtsp://192.168.144.108:554/stream=1` |
| UDP 控制 | `192.168.144.108:9003`（本地绑定 `:9004`） |

视频源选项：**Topotek TQ10N**（`VideoSettings`）。与 UniPod MT11 平行栈，互不混用。

Phase-1 控制：拍照、录像切换、按住 ± 变焦、PTZ 速度十字盘 + 回中。**不含** SMB 相册。

UniGCS 浮层能力表（产品功能 vs 本轮按钮）：[../payload-capabilities/](../payload-capabilities/)。

## 设计文档

- [docs/superpowers/specs/2026-08-19-topotek-tq10n-design.md](../../superpowers/specs/2026-08-19-topotek-tq10n-design.md)

## 重建数据库

```bash
python3 docs/manuals/topotek-tq10n/db/build_db.py
```

## 查询示例

```bash
sqlite3 docs/manuals/topotek-tq10n/db/manual.db \
  "SELECT category, name, value FROM specs;"
sqlite3 docs/manuals/topotek-tq10n/db/sdk.db \
  "SELECT cmd_hex, name_en, description FROM commands;"
```
