# 顶栏 GPS 指示器说明

飞行界面顶部栏的卫星图标（`VehicleGPSIndicator`）用来看机载 GNSS 够不够用。颜色、星数、HDOP 都按**这一路 GPS 自己的遥测**画，和电池指示器同一套红 / 橙 / 黄 / 绿。

实现：`src/Toolbar/GPSIndicator.qml`。锁定类型来自 `GPS_RAW_INT` / `GPS2_RAW` 的 `fix_type`（Fact `gps.lock` / `gps2.lock`）。

## 顶栏显示什么

| 元素 | 含义 |
|------|------|
| 卫星图标 | 颜色 = 当前锁定质量 |
| 上行数字 | 可见卫星数（`count`） |
| 下行数字 | HDOP（没有有效 HDOP 时不画这一行，星数仍显示） |

双 GPS：飞控发出 `GPS2_RAW` 后，顶栏并排两套图标。**不再在第二套旁边标竖着的「2」**，避免看成「2 颗星」。点开抽屉里仍有「Vehicle GPS Status」和「Vehicle GPS 2 Status」。

地面 RTK 基站连上时，第一套图标左侧会多一个竖排「RTK」字，那是**地面站改正链路**，不是飞机 RTK 固定解。飞机是否 RTK 看锁定颜色（见下表）。

## 颜色

图标、星数、HDOP 同色。两套 GPS 各自着色。

| 锁定（`lock` / MAVLink `GPS_FIX_TYPE`） | 颜色 | 飞行含义 |
|------------------------------------------|------|----------|
| 无遥测，或 `0` None | 白 / 灰（普通文字色，半透明） | 还没收到这路 GPS |
| `1` No Fix | 红 | 没有定位，不要飞 |
| `2` 2D Lock | 橙 | 只有 2D，高度不可靠 |
| `3` 3D Lock，且星数 &lt; 6 **或** HDOP &gt; 2.0 | 黄 | 有 3D，但质量差 |
| `3` 3D Lock，星数 ≥ 6 且 HDOP ≤ 2.0（HDOP 未知则只看星数） | 绿 | 普通 3D，可用 |
| `4` 3D DGPS | 绿 | 差分增强 |
| `5` RTK float | 黄绿 | 浮点解，精度一般不如固定解 |
| `6` RTK fixed、`7` Static | 绿 | 固定解 / 静态 |

例子：15 颗星、HDOP 0.9、3D 锁定 → **绿**。第二路若是 No Fix → **红**。

## 点开抽屉

点卫星图标打开详情：星数、锁定类型文案、HDOP / VDOP、航迹向、GPS heading（有 yaw 时）等。有第二路 GPS 时多一组 GPS 2。地面 RTK 连着时还有 RTK 状态和设置。

干扰 / 欺骗 / 认证不走这个图标，走旁边的 **GPS Resilience** 指示器（有对应遥测才出现）。

## 改阈值

弱 3D 判定写在 `GpsCluster.statusColor`：

- 星数阈值：`6`
- HDOP 阈值：`2.0`

颜色用 `QGCPalette`：`colorRed` / `colorOrange` / `colorYellow` / `colorYellowGreen` / `colorGreen`，和电池图标一致。
