# UniPod MT11 外部 SDK / 接口整理手册

> 来源：锐川官方 PDF  
> - `UniPod-MT11-SDK-V0.1.0.pdf`（外部 SDK 协议 V0.1.0，2025-09-16）  
> - `UniPod-MT11-Web-Server.pdf`（Web Server 接口 V1.0）  
> - `UniPod-MT11-User-Manual-v1.2.pdf`  
>
> 结构化数据：[`db/sdk.db`](db/sdk.db) / [`db/sdk.json`](db/sdk.json)  
> 本文为便于集成的整理稿；字段细节以官方 PDF 为准。

## 1. 连接与媒体

| 通道 | 参数 |
|------|------|
| TTL 串口 | 115200 8N1 |
| UDP | `192.168.144.25:37260` |
| TCP | `192.168.144.25:37260`（需周期性发 `0x00` 心跳） |
| 主码流 RTSP | `rtsp://192.168.144.25:8554/video1` |
| 硬件产品码 | 硬件 ID 前缀 `8A` = MT11 |

默认依赖电台以太网 `192.168.144.x`。QGC 中选择视频源 **UniPod MT11** 时会固定上述 RTSP，并等待网段就绪后再起流。

## 2. 帧格式

全小端。CRC 覆盖 CRC 字段之前的整包；多项式 `X^16+X^12+X^5+1`（表驱动 CRC16-CCITT，初值 0）。

| 字段 | 偏移 | 长度 | 说明 |
|------|------|------|------|
| STX | 0 | 2 | `0x6655`（线上为 `55 66`） |
| CTRL | 2 | 1 | bit0=`need_ack`，bit1=`ack_pack` |
| Data_len | 3 | 2 | DATA 字节数 |
| SEQ | 5 | 2 | 0..65535 |
| CMD_ID | 7 | 1 | 命令号 |
| DATA | 8 | Data_len | 载荷 |
| CRC16 | 末尾 | 2 | 整包校验 |

TCP 心跳示例：

```text
55 66 01 01 00 00 00 00 00 59 8B
```

## 3. 命令总表

| CMD | 名称 | 分类 | 方向 | MT11 备注 |
|-----|------|------|------|-----------|
| 0x00 | TCP 心跳 | link | 请求 | 仅 TCP |
| 0x01 | 固件版本 | system | 请求 | 无变焦固件；相机启动约 30s 内版本为 0 |
| 0x02 | 硬件 ID | system | 请求 | 12 字节字符串，前 2 位产品码 |
| 0x04 | 自动对焦 | camera | 请求 | 光学变焦；双拼时 x 取半宽 |
| 0x05 | 手动变倍 | camera | 请求 | 1 放大 / 0 停 / -1 缩小；ACK 为倍数×10 |
| 0x06 | 手动对焦 | camera | 请求 | 仅光学变焦产品 |
| 0x07 | 云台速度转向 | gimbal | 请求 | yaw/pitch -100..100，松手发 0 |
| 0x08 | 一键回中 | gimbal | 请求 | 1 回中 / 2 居中朝下 / 3 居中 / 4 朝下 |
| 0x0A | 相机系统信息 | system | 请求 | HDR/录像/模式/安装方向/输出/联动变倍 |
| 0x0B | 功能反馈 | system | 推送 | 相机主动推送 |
| 0x0C | 拍照/录像/模式 | camera | 请求 | 无 ACK；部分功能暂不支持 |
| 0x0D | 云台姿态 | gimbal | 请求 | 值/10=度；建议改用 0x25 推送 |
| 0x0E | 设置姿态角 | gimbal | 请求 | Pitch -90..30；角度×10 |
| 0x0F | 绝对变倍 | camera | 请求 | 整数 + 小数（0..9） |
| 0x10 | 读视频拼接 | video | 请求 | 主/副码流镜头组合 |
| 0x11 | 写视频拼接 | video | 请求 | 暂仅支持有限组合（见下） |
| 0x12 | 点测温 | thermal | 请求 | 温度/100 |
| 0x13 | 局部测温 | thermal | 请求 | 矩形；电子变倍下以返回框为准 |
| 0x14 | 全局测温 | thermal | 请求 | 全帧最大/最小 |
| 0x15 | 激光测距 | laser | 请求 | dm；需 0x32 开启；5–1200 m |
| 0x16 | 变倍范围 | camera | 请求 | |
| 0x17 | 激光目标经纬度 | laser | 请求 | degE7 |
| 0x18 | 当前变倍 | camera | 请求 | |
| 0x19 | 云台模式 | gimbal | 请求 | 0 锁定 / 1 跟随 / 2 FPV |
| 0x1A / 0x1B | 伪彩色读/写 | thermal | 请求 | 见色板枚举 |
| 0x20 / 0x21 | 编码参数读/写 | video | 请求 | 录像流编码暂不可改 |
| 0x22 | 注入飞控姿态 | fc_bridge | 请求 | NED；推荐 20–50 Hz |
| 0x23 / 0x24 | RC / 飞控流 | fc_bridge | 请求 | 文档标注暂不使用 |
| 0x25 | 请求云台推流 | gimbal | 请求 | 姿态/激光/磁编/电机电压 |
| 0x26 | 磁编码角度 | gimbal | 请求 | 值/10=度 |
| 0x2A | 电机电压 | gimbal | 请求 | 值/1000=V |
| 0x30 / 0x40 | UTC / 系统时间 | system | 请求 | 暂不使用 |
| 0x31 / 0x32 | 激光状态读/写 | laser | 请求 | |
| 0x37 / 0x38 | IR 增益读/写 | thermal | 请求 | 0 低 / 1 高 |
| 0x3E | 注入 GPS | fc_bridge | 请求 | |
| 0x48 / 0x49 | 格式化 SD / TF 信息 | storage | 请求 | 容量/100=GB |
| 0x4D..0x57 | AI 追踪相关 | ai | 请求/推送 | 0x50 为推送流 |
| 0x60 / 0x61 | EIS 读/写 | video | 请求 | |
| 0x81 / 0x82 | IP 读/写 | network | 请求 | |

完整字段、示例包、枚举见数据库 `commands` / `command_fields` / `examples` / `enums`。

### 3.1 MT11 视频拼接（0x11）当前支持组合

官方注明暂时仅支持：

1. 主变焦 — 副热成像  
2. 主热成像 — 副变焦  
3. 主「变焦拼热成像」— 副热成像  

### 3.2 姿态角（0x0E）

- Yaw：无限  
- Pitch（MT11）：**-90.0 ~ 30.0°**  
- 发送/回读均为实际角度 × 10（一位小数）

### 3.3 0x25 推送频率

| 值 | 频率 |
|----|------|
| 0 | 关闭 |
| 1 | 2 Hz |
| 2 | 4 Hz |
| 3 | 5 Hz |
| 4 | 10 Hz |
| 5 | 20 Hz |
| 6 | 50 Hz |
| 7 | 100 Hz |

激光测距推送频率不可设：`data_freq != 0` 即开启。

### 3.4 热成像伪彩色（0x1A / 0x1B）

| 值 | 名称 |
|----|------|
| 0 | 白热 White_Hot |
| 1 | 保留 |
| 2 | 辉金 Sepia |
| 3 | 铁红 Ironbow |
| 4 | 彩虹 Rainbow |
| 5 | 微光 Night |
| 6 | 极光 Aurora |
| 7 | 红热 Red_Hot |
| 8 | 丛林 Jungle |
| 9 | 医疗 Medical |
| 10 | 黑热 Black_Hot |
| 11 | 金红 Glory_Hot |

### 3.5 AI 追踪要点

1. `0x55` 开启 AI 追踪模式（夜视 / AI 超分开启时可能只能框选跟踪）。  
2. `0x56` 点选或框选目标（点选时 `touch_rx/ry = 0`）。  
3. `0x51` 打开坐标推流后，设备主动发 `0x50`。  
4. `0x50` 坐标原点在识别框中心，像素基准 **1280×720**。  
5. `Target_ID`：0 人 / 1 汽车 / 2 巴士 / 3 卡车 / 255 任意物体。

## 4. 常用十六进制示例

包字节颜色含义（对照官方）：`STX CTRL LEN SEQ CMD DATA CRC`。

| 动作 | 十六进制包 |
|------|------------|
| 请求固件版本 | `55 66 01 00 00 00 00 01 64 C4` |
| 拍照 | `55 66 01 01 00 00 00 0C 00 34 CE` |
| 录像切换 | `55 66 01 01 00 00 00 0C 02 76 EE` |
| 姿态 10 Hz | `55 66 01 02 00 00 00 25 01 04 E2 BF` |
| 开激光 | `55 66 01 01 00 00 00 32 01 8F F8` |
| 关激光 | `55 66 01 01 00 00 00 32 00 AE E8` |
| 设置默认 IP | `55 66 02 0C 00 00 00 82 C0 A8 90 19 FF FF FF 00 C0 A8 90 01 DF A6` |

更多示例：

```bash
sqlite3 db/sdk.db "SELECT cmd_hex, title, hex_packet FROM examples JOIN commands USING(cmd_id);"
```

## 5. Web Server 媒体文件接口（V1.0）

实机基址（厂家确认，**非**文档里的默认 80 根路径）：

```text
http://192.168.144.25:82/cgi-bin/media.cgi
```

示例：`GET .../api/v1/getdirectories?media_type=0`  
列文件时 **必须带目录 `path=`**（如 `2026-08-17`）；空 path 会返回空列表。  
文件下载 URL 形如 `http://192.168.144.25:82/photo/...` 或 `.../mp4/...`（不经 cgi-bin）。

统一响应外壳：

```json
{ "code": 200, "data": {}, "success": true, "message": "" }
```

| Method | Path | 作用 |
|--------|------|------|
| GET | `/api/v1/getdirectories` | 目录列表；`media_type` 0 图 / 1 视频 |
| GET | `/api/v1/getmediacount` | 文件个数；`path` **必填**（空 path 会失败） |
| GET | `/api/v1/getmedialist` | 文件列表；需 `path` + `start`/`count` 分页 |
| POST | `/api/v1/deletemediadirectories` | 删文件夹；`paths[]` |
| POST | `/api/v1/deletemedialist` | 删文件；`paths[]` 可为完整 URL |

错误通常 `400`，例如 `Invalid media type` / `path not exist` / `Invalid parameter`。

## 6. 与本仓库 QGC 的关系

| 项 | 位置 |
|----|------|
| 视频源名 / 默认 RTSP | `src/Settings/VideoSettings.h`（`videoSourceUnipodMT11` / `unipodMT11RtspUrl`） |
| 起流前等待 `192.168.144.x` | `VideoManager` UniPod ethernet gate |
| SIYI 电台以太网辅助 | Android `QGCSiyiEthernetHelper` 等 |
| Phase 1 拍照/录像 UDP | `UnipodMt11Protocol` / `UnipodMt11Client` / `UnipodMt11CameraControl` |
| Phase 2 媒体库 HTTP | `UnipodMt11MediaClient` + `UnipodMt11MediaGallery.qml`（Fly 拍照条旁入口） |
| Fly 视图红圈控件 | `PhotoVideoControl.qml` → `QGCCameraManager`（MT11 源时优先 UniPod 控制） |

### QGC Phase 1（拍照 / 录像）

当视频源为 **UniPod MT11** 且电台以太网 `192.168.144.x` 就绪时：

- Fly 视图红圈 **拍照 / 录像** 经 UDP `192.168.144.25:37260` 发送 **`0x0C`**（拍照 `func_type=0`；录像切换 `func_type=2`），触发载荷 TF 卡 onboard 存储。
- **录像 UI 状态** 来自周期性 **`0x0A`** 查询 ACK 的 `record_sta`（约 1 Hz）；收到 **`0x0B`** 推送时同步更新。
- 不调用本地 GStreamer 录制；非 MT11 视频源时恢复 Simulated / MAVLink 相机行为。
- 示例包见 §4（拍照 `34 CE`，录像 `76 EE`，`0x0A` 请求 `0F 75`）。

### QGC Phase 2（媒体预览 / 下载）

当视频源为 **UniPod MT11** 且电台以太网就绪时，Fly 视图拍照条旁出现 **Media** 按钮：

- 通过 HTTP `http://192.168.144.25:82/cgi-bin/media.cgi/api/v1/...` 列出载荷 TF 卡上的照片/视频（先 getdirectories，再按目录 getmedialist）。
- 照片可大图预览；视频在弹层内播放（播放时暂停直播 RTSP，关闭后恢复）。
- 单文件下载到 QGC 的照片/视频保存目录。

云台 / 测温 / AI 等其它 SDK 命令尚未接入；扩展时以本目录 `db/sdk.db` 为命令权威表。

### 现场 HIL 验收清单（操作员）

在 **思翼 SIYI 遥控器 + MT11 + TF 卡** 上逐项确认：

1. 视频源选 **UniPod MT11** → RTSP 画面正常。
2. 打开 Fly 拍照/录像条 → 点 **拍照** → TF 卡（或 UniGCS 媒体库）出现新文件。
3. 点 **录像** → 按钮显示录制中；再点停止 → TF 出现视频；`record_sta` 与 UI 一致。
4. 拔出 TF → 录像禁用或提示无卡。
5. 切换视频源离开 MT11 → 不再发 UDP；Simulated / MAVLink 行为恢复。

## 7. 维护说明

- 官方更新 SDK PDF 后：替换本目录 PDF，再更新 `db/` 与本手册。  
- 重建数据库脚本未单独入库；如需再生，以本手册 + 官方 PDF 为源，保持表结构与 `db/schema.sql` 一致。  
- CRC 参考实现见官方 SDK PDF 第五节 `CRC16_cal` / `crc16_tab`。
