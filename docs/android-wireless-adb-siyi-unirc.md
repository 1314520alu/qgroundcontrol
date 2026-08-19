# UniRC 无线 ADB 指南

在思翼 UniRC 手持机（如 `Standard-10inch_A2`）上调试、安装本仓库 Android 包。

插 Type-C 时系统会问是否「关闭视频显示，进行文件传输」：选「是」才能 USB 调试，但图传常断；选「否」则 USB 调试通常不可用。日常请用**无线 ADB**，飞测时不要一直插电脑线。

---

## 1. 准备环境

电脑与手持连**同一个 Wi‑Fi**。手持打开：**开发者选项 → USB 调试**。

每个新终端先执行：

```bash
cd ~/Projects/qgroundcontrol
source ./env-qgc.sh
which adb
```

应输出一行路径，例如：

```text
/Users/chenlu210cl/Library/Android/sdk/platform-tools/adb
```

若提示 `command not found: adb`，说明还没 `source ./env-qgc.sh`。也可不用 PATH，直接调用：

```bash
~/Library/Android/sdk/platform-tools/adb devices
```

下文一律假设已 `source ./env-qgc.sh`。

---

## 2. 多设备时必看（`more than one device/emulator`）

USB 和无线常会**同时**出现在列表里，例如：

```text
List of devices attached
d                       device
192.168.0.100:5555      device
```

这时直接执行 `adb shell`、`adb tcpip`、`adb install` 会报错：

```text
error: more than one device/emulator
```

**处理方式（三选一）：**

```bash
# ① 指定无线设备（推荐，日常装包用这个）
adb -s 192.168.0.100:5555 shell "ip -4 addr show wlan0"
adb -s 192.168.0.100:5555 install -r -d build/Android-debug/android-build/QGroundControl.apk

# ② 指定 USB 设备（序列号以 adb devices 为准，常见为 d）
adb -s d shell "ip -4 addr show wlan0"
adb -s d tcpip 5555

# ③ 只留一台：拔掉 USB，或断开无线
adb disconnect 192.168.0.100:5555
```

规则：只要 `adb devices` 里多于一行 `device`，所有命令都加上 `-s <序列号或 IP:端口>`。

---

## 3. 方法 A：adb tcpip（推荐）

需要短暂插一次 USB，之后拔线用 Wi‑Fi。

**手持：** 插 Type-C，弹窗选「是」。

**电脑：**

```bash
adb devices
```

若此时只有 USB 一台：

```bash
adb tcpip 5555
adb shell "ip -4 addr show wlan0"
```

若已有无线在线（两台），对 USB 指定序列号：

```bash
adb -s d tcpip 5555
adb -s d shell "ip -4 addr show wlan0"
```

从输出里找到 IP，例如 `inet 192.168.0.100/24` 则 IP 为 `192.168.0.100`。

**拔掉 USB**（图传一般会恢复），然后：

```bash
adb connect 192.168.0.100:5555
adb devices
```

成功时应看到：

```text
192.168.0.100:5555    device
```

以后断线再连：

```bash
adb connect 192.168.0.100:5555
```

把上面的 IP 换成你实际查到的。手持重启后需重新做本节（从插 USB 开始）。

---

## 4. 方法 B：安卓「无线调试」（配对码）

可不插 USB。需要手持有「无线调试」开关（Android 11+）。

**手持：**

1. 开发者选项里打开「USB 调试」「无线调试」
2. 进入「无线调试」→「使用配对码配对设备」
3. 记下屏幕上的：
   - 配对用：`IP` + `配对端口` + `6 位配对码`
   - 主页面另有调试用：`IP` + `调试端口`（与配对端口不同）

**电脑：**（把下面的文字换成屏幕上的数字）

```bash
adb pair IP:配对端口
```

按提示输入 6 位配对码，成功后再执行：

```bash
adb connect IP:调试端口
adb devices
```

关掉「无线调试」或重启手持后，通常要重新配对。若同时还插着 USB，后续命令请加 `-s IP:调试端口`。

---

## 5. 安装本仓库 QGC

无线已连接后（建议先拔 USB，避免多设备）：

```bash
cd ~/Projects/qgroundcontrol
source ./env-qgc.sh
cmake --build build/Android-debug -j8
./run-qgc.sh --android
```

`run-qgc.sh --android` 会装到 `adb devices` 里**第一个** `device`。若 USB 和无线都在，可能装到 USB 那台；更稳妥是指定无线：

```bash
adb -s 192.168.0.100:5555 install -r -d build/Android-debug/android-build/QGroundControl.apk
adb -s 192.168.0.100:5555 shell am force-stop org.mavlink.qgroundcontrol
adb -s 192.168.0.100:5555 shell am start -n org.mavlink.qgroundcontrol/.QGCActivity
```

---

## 6. 常用命令

```bash
adb devices -l

# 多设备时必须带 -s
adb -s 192.168.0.100:5555 shell "ip -4 addr show wlan0"
adb -s 192.168.0.100:5555 shell "dumpsys wifi | grep -m1 'mWifiInfo SSID'"

adb disconnect 192.168.0.100:5555
adb kill-server
adb start-server
```

---

## 7. 排错

| 现象 | 处理 |
|------|------|
| `command not found: adb` | 先 `source ./env-qgc.sh` |
| `more than one device/emulator` | USB + 无线同时在线；用 `adb -s …`，或拔 USB / `adb disconnect` |
| `adb devices` 为空 | 同 Wi‑Fi？再 `adb connect IP:5555`；方法 A 先 USB 执行 `adb tcpip 5555` |
| `unable to connect` | 核对 IP；关防火墙；路由器关闭 AP/客户端隔离 |
| 插 USB 后无视频 | 正常；装完拔线，或改用无线 ADB |

---

## 8. 图传备忘

- 数传可用 `127.0.0.1:19856`，不依赖 `eth0`
- UniPod 等 RTSP（如 `192.168.144.25`）依赖电台以太网
- 开发优先：无线 ADB + 不插电脑 Type-C
