#include "VehicleStatusTextTranslator.h"

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include <algorithm>
#include <utility>

QGC_LOGGING_CATEGORY(VehicleStatusTextTranslatorLog, "MAVLink.VehicleStatusTextTranslator")

namespace {

bool containsHtml(const QString &text)
{
    static const QRegularExpression htmlTagRe(QStringLiteral("<\\s*/?\\s*[A-Za-z][^>]*>"));
    return htmlTagRe.match(text).hasMatch();
}

bool isProtocolText(const QString &text)
{
    return text.startsWith(QLatin1String("[cal]"), Qt::CaseInsensitive);
}

const QList<QPair<QString, QString>> &chinesePhrases()
{
    // Longer phrases first so "Throttle armed" wins over "Armed".
    static const QList<QPair<QString, QString>> phrases = []() {
        QList<QPair<QString, QString>> list = {
            { QStringLiteral("3D Accel calibration needed"), QStringLiteral("需要三轴加速度计校准") },
            { QStringLiteral("3D Accel calibration running"), QStringLiteral("正在进行三轴加速度计校准") },
            { QStringLiteral("Accels calibrated requires reboot"), QStringLiteral("加速度计已校准，需要重启飞控") },
            { QStringLiteral("Accels inconsistent"), QStringLiteral("加速度计不一致") },
            { QStringLiteral("Accels not healthy"), QStringLiteral("加速度计状态异常") },
            { QStringLiteral("AHRS: not using configured AHRS type"), QStringLiteral("AHRS 未使用已配置的类型") },
            { QStringLiteral("AHRS: waiting for home"), QStringLiteral("AHRS 等待原点") },
            { QStringLiteral("AHRS: not using GPS"), QStringLiteral("AHRS 未使用 GPS") },
            { QStringLiteral("AHRS not healthy"), QStringLiteral("AHRS 状态异常") },
            { QStringLiteral("AHRS: EKF3 active"), QStringLiteral("AHRS：EKF3 已激活") },
            { QStringLiteral("Airspeed 1 not healthy"), QStringLiteral("空速计 1 状态异常") },
            { QStringLiteral("Auxiliary authorisation refused"), QStringLiteral("外部授权被拒绝") },
            { QStringLiteral("Battery below minimum arming capacity"), QStringLiteral("电池容量低于解锁最低值") },
            { QStringLiteral("Battery below minimum arming voltage"), QStringLiteral("电池电压低于解锁最低值") },
            { QStringLiteral("Battery 1 below minimum arming capacity"), QStringLiteral("电池1容量低于解锁最低值") },
            { QStringLiteral("Battery 1 below minimum arming voltage"), QStringLiteral("电池1电压低于解锁最低值") },
            { QStringLiteral("Battery 2 below minimum arming capacity"), QStringLiteral("电池2容量低于解锁最低值") },
            { QStringLiteral("Battery 2 below minimum arming voltage"), QStringLiteral("电池2电压低于解锁最低值") },
            { QStringLiteral("below minimum arming capacity"), QStringLiteral("容量低于解锁最低值") },
            { QStringLiteral("below minimum arming voltage"), QStringLiteral("电压低于解锁最低值") },
            { QStringLiteral("Battery 1 below minimum"), QStringLiteral("电池1低于最低值") },
            { QStringLiteral("Battery 1 unhealthy"), QStringLiteral("电池1状态异常") },
            { QStringLiteral("Battery 2 unhealthy"), QStringLiteral("电池2状态异常") },
            { QStringLiteral("critical capacity failsafe"), QStringLiteral("容量严重保护已触发") },
            { QStringLiteral("critical voltage failsafe"), QStringLiteral("电压严重保护已触发") },
            { QStringLiteral("low capacity failsafe"), QStringLiteral("容量低保护已触发") },
            { QStringLiteral("low voltage failsafe"), QStringLiteral("电压低保护已触发") },
            { QStringLiteral("capacity failsafe critical >= low"), QStringLiteral("容量保护阈值配置错误") },
            { QStringLiteral("voltage failsafe critical >= low"), QStringLiteral("电压保护阈值配置错误") },
            { QStringLiteral("Battery critical capacity failsafe"), QStringLiteral("电池容量严重保护已触发") },
            { QStringLiteral("Battery critical voltage failsafe"), QStringLiteral("电池电压严重保护已触发") },
            { QStringLiteral("Battery low capacity failsafe"), QStringLiteral("电池容量低保护已触发") },
            { QStringLiteral("Battery low voltage failsafe"), QStringLiteral("电池电压低保护已触发") },
            { QStringLiteral("Battery unhealthy"), QStringLiteral("电池状态异常") },
            { QStringLiteral("Battery failsafe"), QStringLiteral("电量保护已触发") },
            { QStringLiteral("Compass calibrated requires reboot"), QStringLiteral("罗盘已校准，需要重启飞控") },
            { QStringLiteral("Compass calibration running"), QStringLiteral("罗盘正在校准") },
            { QStringLiteral("Compass not calibrated"), QStringLiteral("罗盘未校准") },
            { QStringLiteral("Compass offsets too high"), QStringLiteral("罗盘偏移过大") },
            { QStringLiteral("Compasses inconsistent"), QStringLiteral("罗盘不一致") },
            { QStringLiteral("Hardware safety switch"), QStringLiteral("安全开关未按下") },
            { QStringLiteral("Safety Switch"), QStringLiteral("安全开关未按下") },
            { QStringLiteral("Motors Emergency Stopped"), QStringLiteral("电机紧急停机") },
            { QStringLiteral("Motors already armed"), QStringLiteral("电机已解锁") },
            { QStringLiteral("Need 3D Fix"), QStringLiteral("需要三维定位") },
            { QStringLiteral("Need Alt Estimate"), QStringLiteral("需要高度估算") },
            { QStringLiteral("Need Position Estimate"), QStringLiteral("需要位置估算") },
            { QStringLiteral("waiting for position estimate"), QStringLiteral("等待位置估算") },
            { QStringLiteral("waiting for home"), QStringLiteral("等待原点") },
            { QStringLiteral("Waiting for Nav Checks"), QStringLiteral("等待导航检查") },
            { QStringLiteral("Waiting for Range Finder"), QStringLiteral("等待测距仪") },
            { QStringLiteral("Check Range Finder"), QStringLiteral("请检查测距仪") },
            { QStringLiteral("Check fence"), QStringLiteral("请检查围栏") },
            { QStringLiteral("Check mag field"), QStringLiteral("磁场异常") },
            { QStringLiteral("RC not calibrated"), QStringLiteral("遥控未校准") },
            { QStringLiteral("RC not found"), QStringLiteral("未检测到遥控") },
            { QStringLiteral("RC calibrating"), QStringLiteral("正在校准遥控") },
            { QStringLiteral("Radio failsafe on"), QStringLiteral("遥控失控保护已触发") },
            { QStringLiteral("Radio failsafe off"), QStringLiteral("遥控失控保护已解除") },
            { QStringLiteral("GCS failsafe on"), QStringLiteral("地面站失控保护已触发") },
            { QStringLiteral("EKF failsafe"), QStringLiteral("EKF 失控保护已触发") },
            { QStringLiteral("EKF3 waiting for GPS config data"), QStringLiteral("EKF3 等待 GPS 配置数据") },
            { QStringLiteral("EKF3 wait for GPS config data"), QStringLiteral("EKF3 等待 GPS 配置数据") },
            { QStringLiteral("EKF2 waiting for GPS config data"), QStringLiteral("EKF2 等待 GPS 配置数据") },
            { QStringLiteral("EKF2 wait for GPS config data"), QStringLiteral("EKF2 等待 GPS 配置数据") },
            { QStringLiteral("EK3 waiting for GPS config data"), QStringLiteral("EKF3 等待 GPS 配置数据") },
            { QStringLiteral("EK3 wait for GPS config data"), QStringLiteral("EKF3 等待 GPS 配置数据") },
            { QStringLiteral("waiting for GPS config data"), QStringLiteral("等待 GPS 配置数据") },
            { QStringLiteral("wait for GPS config data"), QStringLiteral("等待 GPS 配置数据") },
            { QStringLiteral("EKF attitude is bad"), QStringLiteral("EKF 姿态估算异常") },
            { QStringLiteral("EKF compass variance"), QStringLiteral("EKF 罗盘方差过大") },
            { QStringLiteral("EKF height variance"), QStringLiteral("EKF 高度方差过大") },
            { QStringLiteral("EKF position variance"), QStringLiteral("EKF 位置方差过大") },
            { QStringLiteral("EKF velocity variance"), QStringLiteral("EKF 速度方差过大") },
            { QStringLiteral("EKF variance"), QStringLiteral("EKF 方差过大") },
            { QStringLiteral("Gyros not calibrated"), QStringLiteral("陀螺仪未校准") },
            { QStringLiteral("Gyros not healthy"), QStringLiteral("陀螺仪状态异常") },
            { QStringLiteral("Gyros inconsistent"), QStringLiteral("陀螺仪不一致") },
            { QStringLiteral("Logging failed"), QStringLiteral("日志记录失败") },
            { QStringLiteral("Logging not started"), QStringLiteral("日志未开始记录") },
            { QStringLiteral("Internal errors"), QStringLiteral("内部错误") },
            { QStringLiteral("vehicle not upright"), QStringLiteral("飞行器未水平") },
            { QStringLiteral("Vehicle too far from EKF origin"), QStringLiteral("飞行器距 EKF 原点过远") },
            { QStringLiteral("vehicle outside fence"), QStringLiteral("飞行器在围栏外") },
            { QStringLiteral("Fence breached"), QStringLiteral("围栏已突破") },
            { QStringLiteral("Fence enabled"), QStringLiteral("围栏已启用") },
            { QStringLiteral("Fence disabled"), QStringLiteral("围栏已关闭") },
            { QStringLiteral("Fence requires position"), QStringLiteral("围栏需要位置估算") },
            { QStringLiteral("Fences enabled, but none selected"), QStringLiteral("围栏已启用但未选择类型") },
            { QStringLiteral("Fences invalid"), QStringLiteral("围栏无效") },
            { QStringLiteral("No SD card"), QStringLiteral("未检测到 SD 卡") },
            { QStringLiteral("System not Initialized"), QStringLiteral("系统尚未初始化") },
            { QStringLiteral("Throttle below failsafe"), QStringLiteral("油门低于失控保护值") },
            { QStringLiteral("Throttle armed"), QStringLiteral("油门已解锁") },
            { QStringLiteral("Throttle disarmed"), QStringLiteral("油门已上锁") },
            { QStringLiteral("Arming checks bypassed"), QStringLiteral("已跳过解锁检查") },
            { QStringLiteral("Arming checks disabled"), QStringLiteral("解锁检查已关闭") },
            { QStringLiteral("Flight mode change failed"), QStringLiteral("飞行模式切换失败") },
            { QStringLiteral("Takeoff complete"), QStringLiteral("起飞完成") },
            { QStringLiteral("Land complete"), QStringLiteral("降落完成") },
            { QStringLiteral("Mission complete"), QStringLiteral("任务完成") },
            { QStringLiteral("Ready to fly"), QStringLiteral("可以起飞") },
            { QStringLiteral("Takeoff detected"), QStringLiteral("已起飞") },
            { QStringLiteral("Landing detected"), QStringLiteral("已着陆") },
            { QStringLiteral("Failsafe mode activated"), QStringLiteral("失控保护已激活") },
            { QStringLiteral("Failsafe mode deactivated"), QStringLiteral("失控保护已解除") },
            { QStringLiteral("Armed by internal command"), QStringLiteral("内部指令解锁") },
            { QStringLiteral("Disarmed by landing"), QStringLiteral("着陆后上锁") },
            { QStringLiteral("Crash: Disarming"), QStringLiteral("撞击保护：正在上锁") },
            { QStringLiteral("Parachute: Released"), QStringLiteral("降落伞：已开伞") },
            { QStringLiteral("Lost GPS"), QStringLiteral("GPS 丢失") },
            { QStringLiteral("GPS Glitch"), QStringLiteral("GPS 跳变") },
            { QStringLiteral("GPS glitching"), QStringLiteral("GPS 正在跳变") },
            { QStringLiteral("Bad GPS Position"), QStringLiteral("GPS 位置异常") },
            { QStringLiteral("High GPS HDOP"), QStringLiteral("GPS 水平精度不足") },
            { QStringLiteral("GPS blending unhealthy"), QStringLiteral("GPS 融合状态异常") },
            { QStringLiteral("GPS and AHRS differ by"), QStringLiteral("GPS 与 AHRS 位置相差") },
            { QStringLiteral("GPS positions differ by"), QStringLiteral("GPS 位置相差") },
            { QStringLiteral("GPS alt error"), QStringLiteral("GPS 高度误差过大") },
            { QStringLiteral("GPS Node"), QStringLiteral("GPS 节点") },
            { QStringLiteral("primary but TYPE 0"), QStringLiteral("主 GPS 类型未配置") },
            { QStringLiteral("yaw not available"), QStringLiteral("航向不可用") },
            { QStringLiteral("still configuring this GPS"), QStringLiteral("GPS 仍在配置") },
            { QStringLiteral("Can't check rally without position"), QStringLiteral("无位置估算，无法检查集结点") },
            { QStringLiteral("Can’t check rally without position"), QStringLiteral("无位置估算，无法检查集结点") },
            { QStringLiteral("No sufficiently close rally point located"), QStringLiteral("附近没有可用集结点") },
            { QStringLiteral("No rally library present"), QStringLiteral("固件未包含集结点功能") },
            { QStringLiteral("No mission library present"), QStringLiteral("固件未包含任务功能") },
            { QStringLiteral("Failed to open mission.stg"), QStringLiteral("无法打开任务文件") },
            { QStringLiteral("Failed to create log directory"), QStringLiteral("无法创建日志目录") },
            { QStringLiteral("Main loop slow"), QStringLiteral("主循环过慢") },
            { QStringLiteral("CrashDump data detected"), QStringLiteral("检测到崩溃转储数据") },
            { QStringLiteral("Param storage failed"), QStringLiteral("参数存储失败") },
            { QStringLiteral("parameter storage full"), QStringLiteral("参数存储已满") },
            { QStringLiteral("Batch sampling requires reboot"), QStringLiteral("批量采样已更改，需要重启飞控") },
            { QStringLiteral("BendyRuler OA requires reboot"), QStringLiteral("避障配置已更改，需要重启飞控") },
            { QStringLiteral("Dijkstra OA requires reboot"), QStringLiteral("避障配置已更改，需要重启飞控") },
            { QStringLiteral("OA requires reboot"), QStringLiteral("避障配置已更改，需要重启飞控") },
            { QStringLiteral("EKF3 Roll/Pitch inconsistent by"), QStringLiteral("EKF3 横滚/俯仰不一致，相差") },
            { QStringLiteral("EKF3 Yaw inconsistent by"), QStringLiteral("EKF3 航向不一致，相差") },
            { QStringLiteral("vel error"), QStringLiteral("速度误差") },
            { QStringLiteral("EK3 sources require"), QStringLiteral("EKF3 源需要") },
            { QStringLiteral("Check EK3_SRC"), QStringLiteral("请检查 EK3_SRC") },
            { QStringLiteral("Battery capacity failsafe critical >= low"), QStringLiteral("电池容量保护阈值配置错误") },
            { QStringLiteral("Battery voltage failsafe critical >= low"), QStringLiteral("电池电压保护阈值配置错误") },
            { QStringLiteral("Servo voltage to low"), QStringLiteral("舵机电压过低") },
            { QStringLiteral("out of range"), QStringLiteral("超出范围") },
            { QStringLiteral("Duplicate Aux Switch Options"), QStringLiteral("辅助开关功能重复") },
            { QStringLiteral("Mode channel and RCx_OPTION conflict"), QStringLiteral("模式通道与辅助开关冲突") },
            { QStringLiteral("Multiple SERIAL ports configured for RC input"), QStringLiteral("多个串口被配置为遥控输入") },
            { QStringLiteral("In OSD menu"), QStringLiteral("正在配置 OSD") },
            { QStringLiteral("temperature cal running"), QStringLiteral("正在进行温度校准") },
            { QStringLiteral("terrain data expired, possible errors"), QStringLiteral("地形数据已过期，可能有误") },
            { QStringLiteral("terrain required but disabled"), QStringLiteral("任务需要地形数据但未启用") },
            { QStringLiteral("RTL_ALT_TYPE is above-terrain but no terrain data"), QStringLiteral("RTL_ALT_TYPE 为相对地形，但无地形数据") },
            { QStringLiteral("RTL_ALT_TYPE is above-terrain but RTL_ALT>RNGFND_MAX"), QStringLiteral("RTL_ALT_TYPE 为相对地形，但 RTL_ALT 大于测距仪量程") },
            { QStringLiteral("Mount: check TYPE"), QStringLiteral("云台：请检查类型配置") },
            { QStringLiteral("Mount: not healthy"), QStringLiteral("云台状态异常") },
            { QStringLiteral("VisOdom: not healthy"), QStringLiteral("视觉里程计状态异常") },
            { QStringLiteral("VisOdom: out of memory"), QStringLiteral("视觉里程计内存不足") },
            { QStringLiteral("Generator: Not healthy"), QStringLiteral("发电机状态异常") },
            { QStringLiteral("Generator: No backend driver"), QStringLiteral("固件未包含所选发电机驱动") },
            { QStringLiteral("ADSB out of memory"), QStringLiteral("ADSB 内存不足") },
            { QStringLiteral("ADSB threat detected"), QStringLiteral("检测到 ADSB 威胁") },
            { QStringLiteral("Proximity"), QStringLiteral("接近感知") },
            { QStringLiteral("(want >"), QStringLiteral("（需要 >") },
            { QStringLiteral("Board ("), QStringLiteral("飞控板 (") },
            { QStringLiteral("not set as instance"), QStringLiteral("未设置为实例") },
            { QStringLiteral("must be > 0"), QStringLiteral("必须大于 0") },
            { QStringLiteral("must be >"), QStringLiteral("必须大于") },
            { QStringLiteral("RTL_ALT_TYPE is above-terrain but no rangefinder"), QStringLiteral("RTL_ALT_TYPE 为相对地形，但无测距仪") },
            { QStringLiteral("RTL_ALT_TYPE is above-terrain but no terrain data"), QStringLiteral("RTL_ALT_TYPE 为相对地形，但无地形数据") },
            { QStringLiteral("see "), QStringLiteral("参见 ") },
            { QStringLiteral("RELAY"), QStringLiteral("继电器") },
            { QStringLiteral("RPM"), QStringLiteral("转速计") },
            { QStringLiteral("_PIN="), QStringLiteral("引脚=") },
            { QStringLiteral("FFT still analyzing"), QStringLiteral("FFT 仍在分析") },
            { QStringLiteral("FFT calibrating noise"), QStringLiteral("FFT 正在校准噪声") },
            { QStringLiteral("FFT self-test failed"), QStringLiteral("FFT 自检失败") },
            { QStringLiteral("FFT config MAXHZ"), QStringLiteral("FFT 配置 MAXHZ") },
            { QStringLiteral("FFT: calibrated"), QStringLiteral("FFT：已校准") },
            { QStringLiteral("FFT: resolution is"), QStringLiteral("FFT：分辨率是") },
            { QStringLiteral("increase length"), QStringLiteral("请增大长度") },
            { QStringLiteral("Chute is released"), QStringLiteral("降落伞已开伞") },
            { QStringLiteral("Chute has no channel"), QStringLiteral("降落伞未配置输出通道") },
            { QStringLiteral("Chute has no relay"), QStringLiteral("降落伞未配置继电器") },
            { QStringLiteral("AP_Relay not available"), QStringLiteral("继电器功能不可用") },
            { QStringLiteral("DroneCAN: Duplicate Node"), QStringLiteral("DroneCAN：节点 ID 重复") },
            { QStringLiteral("DroneCAN: Failed to access storage!"), QStringLiteral("DroneCAN：无法访问存储") },
            { QStringLiteral("DroneCAN: Failed to add Node"), QStringLiteral("DroneCAN：添加节点失败") },
            { QStringLiteral("DroneCAN: Node"), QStringLiteral("DroneCAN：节点") },
            { QStringLiteral("Same Node Id"), QStringLiteral("相同节点 ID") },
            { QStringLiteral("set for multiple GPS"), QStringLiteral("被用于多个 GPS") },
            { QStringLiteral("Same rfnd on different CAN ports"), QStringLiteral("同一测距仪出现在不同 CAN 口") },
            { QStringLiteral("OpenDroneID: ARM_STATUS not available"), QStringLiteral("OpenDroneID：解锁状态不可用") },
            { QStringLiteral("OpenDroneID: operator location must be set"), QStringLiteral("OpenDroneID：需要设置操作员位置") },
            { QStringLiteral("OpenDroneID: SYSTEM not available"), QStringLiteral("OpenDroneID：系统信息不可用") },
            { QStringLiteral("OpenDroneID: UA_TYPE required in BasicID"), QStringLiteral("OpenDroneID：BasicID 需要 UA 类型") },
            { QStringLiteral("FETtec: Invalid motor mask"), QStringLiteral("FETtec：电机掩码无效") },
            { QStringLiteral("FETtec: Invalid pole count"), QStringLiteral("FETtec：极对数无效") },
            { QStringLiteral("FETtec: No uart"), QStringLiteral("FETtec：未配置串口") },
            { QStringLiteral("FETtec: Not initialised"), QStringLiteral("FETtec：未初始化") },
            { QStringLiteral("FETtec:"), QStringLiteral("FETtec：") },
            { QStringLiteral("ESCs are not running"), QStringLiteral("电调未运行") },
            { QStringLiteral("ESCs are not sending telem"), QStringLiteral("电调未发送遥测") },
            { QStringLiteral("Scripting: loaded CRC incorrect"), QStringLiteral("脚本：已加载脚本 CRC 不正确") },
            { QStringLiteral("Scripting: running CRC incorrect"), QStringLiteral("脚本：运行中脚本 CRC 不正确") },
            { QStringLiteral("Scripting:"), QStringLiteral("脚本：") },
            { QStringLiteral("failed to start"), QStringLiteral("启动失败") },
            { QStringLiteral("Bad parameter:"), QStringLiteral("参数错误：") },
            { QStringLiteral("Motors: Check frame class and type"), QStringLiteral("电机：请检查机架类别与类型") },
            { QStringLiteral("Motors: Check MOT_PWM_MIN and MOT_PWM_MAX"), QStringLiteral("电机：请检查 MOT_PWM_MIN 与 MOT_PWM_MAX") },
            { QStringLiteral("Motors: MOT_SPIN_ARM > MOT_SPIN_MIN"), QStringLiteral("电机：MOT_SPIN_ARM 大于 MOT_SPIN_MIN") },
            { QStringLiteral("Motors: MOT_SPIN_MIN too high"), QStringLiteral("电机：MOT_SPIN_MIN 过高") },
            { QStringLiteral("Motors: no SERVOx_FUNCTION set to Motor"), QStringLiteral("电机：未将舵机输出功能设为电机") },
            { QStringLiteral("Check ACRO_BAL_ROLL/PITCH"), QStringLiteral("请检查 ACRO_BAL_ROLL/PITCH") },
            { QStringLiteral("Check ANGLE_MAX"), QStringLiteral("请检查 ANGLE_MAX") },
            { QStringLiteral("Check FS_THR_VALUE"), QStringLiteral("请检查 FS_THR_VALUE") },
            { QStringLiteral("Check PILOT_SPEED_UP"), QStringLiteral("请检查 PILOT_SPEED_UP") },
            { QStringLiteral("Collective below failsafe"), QStringLiteral("总距低于失控保护值") },
            { QStringLiteral("Interlock/E-Stop Conflict"), QStringLiteral("互锁与急停冲突") },
            { QStringLiteral("Invalid MultiCopter FRAME_CLASS"), QStringLiteral("多旋翼 FRAME_CLASS 无效") },
            { QStringLiteral("Inverted flight option not supported"), QStringLiteral("不支持倒飞选项") },
            { QStringLiteral("FS_GCS_ENABLE=2 removed, see FS_OPTIONS"), QStringLiteral("FS_GCS_ENABLE=2 已移除，请查看 FS_OPTIONS") },
            { QStringLiteral("PiccoloCAN: Servo"), QStringLiteral("PiccoloCAN：舵机") },
            { QStringLiteral("not detected"), QStringLiteral("未检测到") },
            { QStringLiteral("No Data"), QStringLiteral("无数据") },
            { QStringLiteral("heater temp low"), QStringLiteral("加热器温度过低") },
            { QStringLiteral("memory low for auxiliary authorisation"), QStringLiteral("外部授权可用内存不足") },
            { QStringLiteral("Too many auxiliary authorisers"), QStringLiteral("外部授权源过多") },
            { QStringLiteral("OSD_TYPE2 not compatible with first OSD"), QStringLiteral("OSD_TYPE2 与主 OSD 不兼容") },
            { QStringLiteral("disabled (ISR flood)"), QStringLiteral("已禁用（中断过频）") },
            { QStringLiteral("Invalid FENCE_ALT_MAX value"), QStringLiteral("FENCE_ALT_MAX 值无效") },
            { QStringLiteral("Invalid FENCE_ALT_MIN value"), QStringLiteral("FENCE_ALT_MIN 值无效") },
            { QStringLiteral("Invalid FENCE_MARGIN value"), QStringLiteral("FENCE_MARGIN 值无效") },
            { QStringLiteral("Invalid FENCE_RADIUS value"), QStringLiteral("FENCE_RADIUS 值无效") },
            { QStringLiteral("FENCE_ALT_MAX < FENCE_ALT_MIN"), QStringLiteral("FENCE_ALT_MAX 小于 FENCE_ALT_MIN") },
            { QStringLiteral("FENCE_MARGIN is less than FENCE_RADIUS"), QStringLiteral("FENCE_MARGIN 小于 FENCE_RADIUS") },
            { QStringLiteral("FENCE_MARGIN too big"), QStringLiteral("FENCE_MARGIN 过大") },
            { QStringLiteral("Margin is less than inclusion circle radius"), QStringLiteral("边距小于圆形围栏半径") },
            { QStringLiteral("BTN_PIN"), QStringLiteral("按钮引脚") },
            { QStringLiteral("RNGFND"), QStringLiteral("测距仪参数") },
            { QStringLiteral("_PIN not set"), QStringLiteral("引脚未设置") },
            { QStringLiteral(" invalid"), QStringLiteral(" 无效") },
            { QStringLiteral("set SERVOz_FUNCTION=-1"), QStringLiteral("请将 SERVOz_FUNCTION 设为 -1") },
            { QStringLiteral("set SERVx_FUNCTION=-1"), QStringLiteral("请将 SERVx_FUNCTION 设为 -1") },
            { QStringLiteral("RCx_MAX is less than RCx_TRIM"), QStringLiteral("遥控通道最大值小于中位") },
            { QStringLiteral("RCx_MIN is greater than RCx_TRIM"), QStringLiteral("遥控通道最小值大于中位") },
            { QStringLiteral("SERVOx_MAX is less than SERVOx_TRIM"), QStringLiteral("舵机最大值小于中位") },
            { QStringLiteral("SERVOx_MIN is greater than SERVOx_TRIM"), QStringLiteral("舵机最小值大于中位") },
            { QStringLiteral("SERVOx_FUNCTION="), QStringLiteral("舵机功能=") },
            { QStringLiteral("on disabled channel"), QStringLiteral("位于已禁用通道") },
            { QStringLiteral("Pin "), QStringLiteral("引脚 ") },
            { QStringLiteral("< loop rate"), QStringLiteral("低于主循环速率") },
            { QStringLiteral("Pitch is not neutral"), QStringLiteral("俯仰摇杆未回中") },
            { QStringLiteral("Roll is not neutral"), QStringLiteral("横滚摇杆未回中") },
            { QStringLiteral("Throttle is not neutral"), QStringLiteral("油门未回中") },
            { QStringLiteral("Pitch (RC"), QStringLiteral("俯仰 (遥控") },
            { QStringLiteral("Roll (RC"), QStringLiteral("横滚 (遥控") },
            { QStringLiteral("Throttle (RC"), QStringLiteral("油门 (遥控") },
            { QStringLiteral("is not neutral"), QStringLiteral("未回中") },
            { QStringLiteral("radio max too low"), QStringLiteral("遥控最大值过低") },
            { QStringLiteral("radio min too high"), QStringLiteral("遥控最小值过高") },
            { QStringLiteral("VTOL Fwd Throttle"), QStringLiteral("垂起前飞油门") },
            { QStringLiteral("iz not zero"), QStringLiteral("不为零") },
            { QStringLiteral("is not zero"), QStringLiteral("不为零") },
            { QStringLiteral("Auto mode not armable"), QStringLiteral("自动模式不可解锁") },
            { QStringLiteral("RTL mode not armable"), QStringLiteral("返航模式不可解锁") },
            { QStringLiteral("Altitude disparity"), QStringLiteral("高度不一致") },
            { QStringLiteral("Baro not healthy"), QStringLiteral("气压计状态异常") },
            { QStringLiteral("Leaning"), QStringLiteral("飞行器倾斜过大") },
            { QStringLiteral("Motor Interlock Enabled"), QStringLiteral("电机互锁已打开") },
            { QStringLiteral("Motor Interlock not configured"), QStringLiteral("电机互锁未配置") },
            { QStringLiteral("Disarm Switch on"), QStringLiteral("上锁开关处于打开位置") },
            { QStringLiteral("Downloading logs"), QStringLiteral("正在下载日志，无法解锁") },
            { QStringLiteral("Mode requires mission"), QStringLiteral("当前模式需要任务航线") },
            { QStringLiteral("Home too far from EKF origin"), QStringLiteral("原点距 EKF 原点过远") },
            { QStringLiteral("Missing mission item: takeoff"), QStringLiteral("任务缺少起飞航点") },
            { QStringLiteral("Missing mission item: land"), QStringLiteral("任务缺少降落航点") },
            { QStringLiteral("Missing mission item: RTL"), QStringLiteral("任务缺少返航航点") },
            { QStringLiteral("Missing mission item: do land start"), QStringLiteral("任务缺少开始降落航点") },
            { QStringLiteral("Missing mission item: vtol land"), QStringLiteral("任务缺少垂起降落航点") },
            { QStringLiteral("Missing mission item: vtol takeoff"), QStringLiteral("任务缺少垂起飞航点") },
            { QStringLiteral("waiting for terrain data"), QStringLiteral("等待地形数据") },
            { QStringLiteral("terrain disabled"), QStringLiteral("地形跟随未启用") },
            { QStringLiteral("Terrain out of memory"), QStringLiteral("地形数据内存不足") },
            { QStringLiteral("Not Connected"), QStringLiteral("未连接") },
            { QStringLiteral("not healthy"), QStringLiteral("状态异常") },
            { QStringLiteral("was not found"), QStringLiteral("未找到") },
            { QStringLiteral("Bad fix"), QStringLiteral("定位质量差") },
            { QStringLiteral("detected as"), QStringLiteral("识别为") },
            { QStringLiteral("probing for"), QStringLiteral("正在探测") },
            { QStringLiteral("fast sampling"), QStringLiteral("高速采样") },
            { QStringLiteral("Reached command waypoint"), QStringLiteral("到达航点") },
            { QStringLiteral("Reached waypoint"), QStringLiteral("到达航点") },
            { QStringLiteral("Preflight Fail"), QStringLiteral("飞前检查失败") },
            { QStringLiteral("Failsafe"), QStringLiteral("失控保护") },
            { QStringLiteral("Disarmed"), QStringLiteral("已上锁") },
            { QStringLiteral("Armed"), QStringLiteral("已解锁") },
            { QStringLiteral("Initialising"), QStringLiteral("正在初始化") },
            { QStringLiteral("Initializing"), QStringLiteral("正在初始化") },
            { QStringLiteral("calibrating"), QStringLiteral("正在校准") },
            { QStringLiteral("calibration needed"), QStringLiteral("需要校准") },
            { QStringLiteral("requires reboot"), QStringLiteral("需要重启飞控") },
            { QStringLiteral("out of memory"), QStringLiteral("内存不足") },
            { QStringLiteral("not calibrated"), QStringLiteral("未校准") },
            { QStringLiteral("inconsistent"), QStringLiteral("不一致") },
            { QStringLiteral("unhealthy"), QStringLiteral("状态异常") },
            { QStringLiteral("Check "), QStringLiteral("请检查 ") },
            { QStringLiteral("Compass"), QStringLiteral("罗盘") },
            { QStringLiteral("Gyros"), QStringLiteral("陀螺仪") },
            { QStringLiteral("Gyro"), QStringLiteral("陀螺仪") },
            { QStringLiteral("Accels"), QStringLiteral("加速度计") },
            { QStringLiteral("Accel"), QStringLiteral("加速度计") },
            { QStringLiteral("Barometer"), QStringLiteral("气压计") },
            { QStringLiteral("Baro"), QStringLiteral("气压计") },
            { QStringLiteral("Rangefinder"), QStringLiteral("测距仪") },
            { QStringLiteral("Range Finder"), QStringLiteral("测距仪") },
            { QStringLiteral("Airspeed"), QStringLiteral("空速计") },
            { QStringLiteral("Parachute"), QStringLiteral("降落伞") },
            { QStringLiteral("Throttle"), QStringLiteral("油门") },
            { QStringLiteral("Battery"), QStringLiteral("电池") },
            { QStringLiteral("Motors"), QStringLiteral("电机") },
            { QStringLiteral("Frame:"), QStringLiteral("机架：") },
            { QStringLiteral("timeout"), QStringLiteral("超时") },
        };
        std::sort(list.begin(), list.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
            return a.first.size() > b.first.size();
        });
        return list;
    }();
    return phrases;
}

QString protectTokens(QString text, QStringList *saved)
{
    static const QRegularExpression tokenRe(
        QStringLiteral("\\b(?:[A-Z][A-Z0-9]*_[A-Z0-9_]+|0x[0-9A-Fa-f]+)\\b"));
    QRegularExpressionMatch match = tokenRe.match(text);
    while (match.hasMatch()) {
        const QString token = QStringLiteral("\uE000%1\uE001").arg(saved->size());
        saved->append(match.captured());
        text.replace(match.capturedStart(), match.capturedLength(), token);
        match = tokenRe.match(text);
    }
    return text;
}

QString restoreTokens(QString text, const QStringList &saved)
{
    for (int i = 0; i < saved.size(); ++i) {
        text.replace(QStringLiteral("\uE000%1\uE001").arg(i), saved.at(i));
    }
    return text;
}

QString replacePhrases(QString text)
{
    for (const auto &phrase : chinesePhrases()) {
        text.replace(phrase.first, phrase.second, Qt::CaseInsensitive);
    }
    return text;
}

struct Prefix {
    QString english;
    QString chinese;
};

const Prefix kPrefixes[] = {
    { QStringLiteral("PreArm:"), QStringLiteral("解锁前检查：") },
    { QStringLiteral("Arm:"), QStringLiteral("解锁：") },
    { QStringLiteral("Preflight Fail:"), QStringLiteral("飞前检查失败：") },
    { QStringLiteral("preflight:"), QStringLiteral("飞前检查：") },
};

QString applyPrefix(QString text)
{
    for (const Prefix &prefix : kPrefixes) {
        if (text.startsWith(prefix.english, Qt::CaseInsensitive)) {
            QString rest = text.mid(prefix.english.size()).trimmed();
            rest = replacePhrases(rest);
            return prefix.chinese + rest;
        }
    }
    return replacePhrases(text);
}

void polishChinesePunctuation(QString &text)
{
    static const QRegularExpression cjkRe(QStringLiteral("[\\x{4e00}-\\x{9fff}]"));
    if (!cjkRe.match(text).hasMatch()) {
        return;
    }
    text.replace(QStringLiteral(": "), QStringLiteral("："));
}

} // namespace

QString VehicleStatusTextTranslator::translate(const QString &text)
{
    const QLocale locale = qgcApp() ? qgcApp()->getCurrentLanguage() : QLocale();
    return translate(text, locale);
}

QString VehicleStatusTextTranslator::translate(const QString &text, const QLocale &locale)
{
    if (text.isEmpty() || !_shouldTranslate(locale) || isProtocolText(text) || containsHtml(text)) {
        return text;
    }

    const QString translated = _translateChinese(text);
    if (translated != text) {
        qCDebug(VehicleStatusTextTranslatorLog) << text << "->" << translated;
    }
    return translated;
}

bool VehicleStatusTextTranslator::_shouldTranslate(const QLocale &locale)
{
    return locale.language() == QLocale::Chinese;
}

QString VehicleStatusTextTranslator::_translateChinese(QString text)
{
    QStringList savedTokens;
    text = protectTokens(std::move(text), &savedTokens);
    text = applyPrefix(std::move(text));
    text = restoreTokens(std::move(text), savedTokens);
    polishChinesePunctuation(text);
    return text;
}
