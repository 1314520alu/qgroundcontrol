#include "ParameterEnumStringTranslator.h"

#include <QtCore/QHash>

#include "FlightModeNameTranslator.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ParameterEnumStringTranslatorLog, "FirmwarePlugin.ParameterEnumStringTranslator")

namespace {

const QHash<QString, QString> &chineseExact()
{
    // Exact English labels from ArduPilot / PX4 parameter metadata.
    // Prefer wording already used in VehicleConfig / failsafe UI translations.
    static const QHash<QString, QString> labels = {
        // Battery / failsafe actions (BATT_FS_LOW_ACT, BATT_FS_CRT_ACT, …)
        {QStringLiteral("Warn only"), QStringLiteral("仅警告")},
        {QStringLiteral("Warning"), QStringLiteral("警告")},
        {QStringLiteral("None"), QStringLiteral("无")},
        {QStringLiteral("Disabled"), QStringLiteral("禁用")},
        {QStringLiteral("Enabled"), QStringLiteral("启用")},
        {QStringLiteral("Land"), QStringLiteral("降落")},
        {QStringLiteral("Land mode"), QStringLiteral("降落模式")},
        {QStringLiteral("RTL"), QStringLiteral("返航")},
        {QStringLiteral("Return"), QStringLiteral("返航")},
        {QStringLiteral("Return mode"), QStringLiteral("返航模式")},
        {QStringLiteral("SmartRTL"), QStringLiteral("智能返航")},
        {QStringLiteral("SmartRTL or RTL"), QStringLiteral("SmartRTL 或返航")},
        {QStringLiteral("SmartRTL or Land"), QStringLiteral("SmartRTL 或降落")},
        {QStringLiteral("Terminate"), QStringLiteral("终止")},
        {QStringLiteral("Brake or Land"), QStringLiteral("刹车或降落")},
        {QStringLiteral("Auto DO_LAND_START/DO_RETURN_PATH_START or RTL"),
         QStringLiteral("自动 DO_LAND_START/DO_RETURN_PATH_START 或返航")},
        {QStringLiteral("Auto DO_LAND_START or RTL"), QStringLiteral("自动 DO_LAND_START 或返航")},
        {QStringLiteral("Return at critical level, land at emergency level"),
         QStringLiteral("临界电量返航，紧急电量降落")},
        {QStringLiteral("Hold"), QStringLiteral("保持")},
        {QStringLiteral("Position Hold mode"), QStringLiteral("定点悬停模式")},

        // RCx_OPTION / CH*_OPT switch options (ArduPilot)
        {QStringLiteral("Do Nothing"), QStringLiteral("无操作")},
        {QStringLiteral("FLIP Mode"), QStringLiteral("翻转模式")},
        {QStringLiteral("Simple Mode"), QStringLiteral("简单模式")},
        {QStringLiteral("Save Trim"), QStringLiteral("保存配平")},
        {QStringLiteral("Save WP"), QStringLiteral("保存航点")},
        {QStringLiteral("Camera Trigger"), QStringLiteral("相机触发")},
        {QStringLiteral("RangeFinder Enable"), QStringLiteral("启用测距仪")},
        {QStringLiteral("Fence Enable"), QStringLiteral("启用电子围栏")},
        {QStringLiteral("Super Simple Mode"), QStringLiteral("超简单模式")},
        {QStringLiteral("Acro Trainer"), QStringLiteral("特技训练")},
        {QStringLiteral("Sprayer Enable"), QStringLiteral("启用喷洒")},
        {QStringLiteral("AUTO Mode"), QStringLiteral("自动模式")},
        {QStringLiteral("AUTOTUNE Mode"), QStringLiteral("自动调参模式")},
        {QStringLiteral("LAND Mode"), QStringLiteral("降落模式")},
        {QStringLiteral("Gripper"), QStringLiteral("抓手")},
        {QStringLiteral("Parachute Enable"), QStringLiteral("启用降落伞")},
        {QStringLiteral("Parachute Release"), QStringLiteral("释放降落伞")},
        {QStringLiteral("Parachute 3pos"), QStringLiteral("降落伞三段开关")},
        {QStringLiteral("Auto Mission Reset"), QStringLiteral("重置自动任务")},
        {QStringLiteral("AttCon Feed Forward"), QStringLiteral("姿态控制前馈")},
        {QStringLiteral("AttCon Accel Limits"), QStringLiteral("姿态控制加速度限制")},
        {QStringLiteral("Retract Mount1"), QStringLiteral("收起云台1")},
        {QStringLiteral("Retract Mount2"), QStringLiteral("收起云台2")},
        {QStringLiteral("Relay1 On/Off"), QStringLiteral("继电器1 开/关")},
        {QStringLiteral("Relay2 On/Off"), QStringLiteral("继电器2 开/关")},
        {QStringLiteral("Relay3 On/Off"), QStringLiteral("继电器3 开/关")},
        {QStringLiteral("Relay4 On/Off"), QStringLiteral("继电器4 开/关")},
        {QStringLiteral("Relay5 On/Off"), QStringLiteral("继电器5 开/关")},
        {QStringLiteral("Relay6 On/Off"), QStringLiteral("继电器6 开/关")},
        {QStringLiteral("Landing Gear"), QStringLiteral("起落架")},
        {QStringLiteral("Lost Copter Sound"), QStringLiteral("寻机提示音")},
        {QStringLiteral("Motor Emergency Stop"), QStringLiteral("电机紧急停转")},
        {QStringLiteral("Motor Interlock"), QStringLiteral("电机联锁")},
        {QStringLiteral("BRAKE Mode"), QStringLiteral("刹车模式")},
        {QStringLiteral("THROW Mode"), QStringLiteral("抛飞模式")},
        {QStringLiteral("ADSB Avoidance Enable"), QStringLiteral("启用 ADSB 避让")},
        {QStringLiteral("PrecLoiter Enable"), QStringLiteral("启用精确悬停")},
        {QStringLiteral("Proximity Avoidance Enable"), QStringLiteral("启用近距避障")},
        {QStringLiteral("ArmDisarm (4.1 and lower)"), QStringLiteral("解锁/上锁（4.1 及更早）")},
        {QStringLiteral("ArmDisarm (4.2 and higher)"), QStringLiteral("解锁/上锁（4.2 及更新）")},
        {QStringLiteral("ArmDisarm with AirMode  (4.2 and higher)"), QStringLiteral("解锁/上锁并开 AirMode（4.2 及更新）")},
        {QStringLiteral("SMARTRTL Mode"), QStringLiteral("智能返航模式")},
        {QStringLiteral("InvertedFlight Enable"), QStringLiteral("启用倒飞")},
        {QStringLiteral("Winch Enable"), QStringLiteral("启用绞车")},
        {QStringLiteral("Winch Control"), QStringLiteral("绞车控制")},
        {QStringLiteral("RC Override Enable"), QStringLiteral("启用遥控覆盖")},
        {QStringLiteral("User Function 1"), QStringLiteral("用户功能1")},
        {QStringLiteral("User Function 2"), QStringLiteral("用户功能2")},
        {QStringLiteral("User Function 3"), QStringLiteral("用户功能3")},
        {QStringLiteral("ACRO Mode"), QStringLiteral("特技模式")},
        {QStringLiteral("GUIDED Mode"), QStringLiteral("引导模式")},
        {QStringLiteral("LOITER Mode"), QStringLiteral("定点悬停模式")},
        {QStringLiteral("FOLLOW Mode"), QStringLiteral("跟随模式")},
        {QStringLiteral("Clear Waypoints"), QStringLiteral("清除航点")},
        {QStringLiteral("ZigZag Mode"), QStringLiteral("之字形模式")},
        {QStringLiteral("ZigZag SaveWP"), QStringLiteral("之字形保存航点")},
        {QStringLiteral("ZigZag Auto"), QStringLiteral("之字形自动")},
        {QStringLiteral("Compass Learn"), QStringLiteral("罗盘学习")},
        {QStringLiteral("GPS Disable"), QStringLiteral("禁用 GPS")},
        {QStringLiteral("GPS Disable Yaw"), QStringLiteral("禁用 GPS 航向")},
        {QStringLiteral("STABILIZE Mode"), QStringLiteral("自稳模式")},
        {QStringLiteral("POSHOLD Mode"), QStringLiteral("定点模式")},
        {QStringLiteral("ALTHOLD Mode"), QStringLiteral("定高模式")},
        {QStringLiteral("FLOWHOLD Mode"), QStringLiteral("光流悬停模式")},
        {QStringLiteral("CIRCLE Mode"), QStringLiteral("绕圈模式")},
        {QStringLiteral("DRIFT Mode"), QStringLiteral("漂移模式")},
        {QStringLiteral("SurfaceTrackingUpDown"), QStringLiteral("地形跟踪升降")},
        {QStringLiteral("STANDBY Mode"), QStringLiteral("待机模式")},
        {QStringLiteral("RunCam Control"), QStringLiteral("RunCam 控制")},
        {QStringLiteral("RunCam OSD Control"), QStringLiteral("RunCam OSD 控制")},
        {QStringLiteral("VisOdom Align"), QStringLiteral("视觉里程计对齐")},
        {QStringLiteral("Disarm"), QStringLiteral("上锁")},
        {QStringLiteral("AirMode"), QStringLiteral("空中模式")},
        {QStringLiteral("Generator"), QStringLiteral("发电机")},
        {QStringLiteral("EKF Source Set"), QStringLiteral("EKF 源切换")},
        {QStringLiteral("VTX Power"), QStringLiteral("图传功率")},
        {QStringLiteral("AUTO RTL"), QStringLiteral("自动返航")},
        {QStringLiteral("KillIMU1"), QStringLiteral("关闭 IMU1")},
        {QStringLiteral("KillIMU2"), QStringLiteral("关闭 IMU2")},
        {QStringLiteral("KillIMU3"), QStringLiteral("关闭 IMU3")},
        {QStringLiteral("Camera Mode Toggle"), QStringLiteral("相机模式切换")},
        {QStringLiteral("EKF lane switch attempt"), QStringLiteral("尝试 EKF 通道切换")},
        {QStringLiteral("EKF yaw reset"), QStringLiteral("EKF 航向重置")},
        {QStringLiteral("EKF Reset"), QStringLiteral("EKF 重置")},
        {QStringLiteral("use Custom Controller"), QStringLiteral("使用自定义控制器")},
        {QStringLiteral("Loweheiser starter"), QStringLiteral("Loweheiser 启动器")},
        {QStringLiteral("SwitchExternalAHRS"), QStringLiteral("切换外部 AHRS")},
        {QStringLiteral("TURTLE Mode"), QStringLiteral("翻转自救模式")},
        {QStringLiteral("SIMPLE heading reset"), QStringLiteral("简单模式航向重置")},
        {QStringLiteral("Optflow Calibration"), QStringLiteral("光流校准")},
        {QStringLiteral("Force IS_Flying"), QStringLiteral("强制飞行中状态")},
        {QStringLiteral("Turbine Start(heli)"), QStringLiteral("涡轮启动（直升机）")},
        {QStringLiteral("FFT Tune"), QStringLiteral("FFT 调参")},
        {QStringLiteral("Mount Yaw Lock"), QStringLiteral("云台航向锁定")},
        {QStringLiteral("Mount Roll/Pitch Lock"), QStringLiteral("云台滚转/俯仰锁定")},
        {QStringLiteral("Mount POI Lock"), QStringLiteral("云台兴趣点锁定")},
        {QStringLiteral("Mount LRF enable"), QStringLiteral("启用云台激光测距")},
        {QStringLiteral("Pause Stream Logging"), QStringLiteral("暂停流式日志")},
        {QStringLiteral("Arm/Emergency Motor Stop"), QStringLiteral("解锁/紧急停转")},
        {QStringLiteral("Camera Record Video"), QStringLiteral("相机录像")},
        {QStringLiteral("Camera Zoom"), QStringLiteral("相机变焦")},
        {QStringLiteral("Camera Manual Focus"), QStringLiteral("相机手动对焦")},
        {QStringLiteral("Camera Auto Focus"), QStringLiteral("相机自动对焦")},
        {QStringLiteral("Camera Image Tracking"), QStringLiteral("相机图像跟踪")},
        {QStringLiteral("Camera Lens"), QStringLiteral("相机镜头")},
        {QStringLiteral("Calibrate Compasses"), QStringLiteral("校准罗盘")},
        {QStringLiteral("Battery MPPT Enable"), QStringLiteral("启用电池 MPPT")},
        {QStringLiteral("FlightMode Pause/Resume"), QStringLiteral("飞行模式暂停/继续")},
        {QStringLiteral("Test autotuned gains after tune is complete"),
         QStringLiteral("调参完成后测试自动调参增益")},
        {QStringLiteral("AHRS AutoTrim"), QStringLiteral("AHRS 自动配平")},
    };
    return labels;
}

} // namespace

QString ParameterEnumStringTranslator::translate(const QString &text)
{
    const QLocale locale = qgcApp() ? qgcApp()->getCurrentLanguage() : QLocale();
    if (!_shouldTranslate(locale)) {
        return text;
    }
    return translate(text, locale, _activeVehicleFixedWing());
}

QString ParameterEnumStringTranslator::translate(const QString &text, const QLocale &locale)
{
    return translate(text, locale, false);
}

QString ParameterEnumStringTranslator::translate(const QString &text, const QLocale &locale, bool fixedWing)
{
    if (text.isEmpty() || !_shouldTranslate(locale)) {
        return text;
    }

    const QString translated = _translateChinese(text, locale, fixedWing);
    if (translated != text) {
        qCDebug(ParameterEnumStringTranslatorLog) << text << "->" << translated;
    }
    return translated;
}

QStringList ParameterEnumStringTranslator::translateList(const QStringList &strings)
{
    const QLocale locale = qgcApp() ? qgcApp()->getCurrentLanguage() : QLocale();
    if (!_shouldTranslate(locale)) {
        return strings;
    }
    return translateList(strings, locale, _activeVehicleFixedWing());
}

QStringList ParameterEnumStringTranslator::translateList(const QStringList &strings, const QLocale &locale)
{
    return translateList(strings, locale, false);
}

QStringList ParameterEnumStringTranslator::translateList(
    const QStringList &strings, const QLocale &locale, bool fixedWing)
{
    if (!_shouldTranslate(locale) || strings.isEmpty()) {
        return strings;
    }

    QStringList translated;
    translated.reserve(strings.size());
    for (const QString &text : strings) {
        translated.append(translate(text, locale, fixedWing));
    }
    return translated;
}

bool ParameterEnumStringTranslator::_shouldTranslate(const QLocale &locale)
{
    return locale.language() == QLocale::Chinese;
}

bool ParameterEnumStringTranslator::_activeVehicleFixedWing()
{
    MultiVehicleManager *mvm = MultiVehicleManager::instance();
    if (!mvm) {
        return false;
    }

    Vehicle *vehicle = mvm->activeVehicle();
    if (!vehicle) {
        vehicle = mvm->offlineEditingVehicle();
    }
    return vehicle && vehicle->fixedWing();
}

QString ParameterEnumStringTranslator::_translateChinese(const QString &text, const QLocale &locale, bool fixedWing)
{
    const auto it = chineseExact().constFind(text);
    if (it != chineseExact().cend()) {
        return it.value();
    }

    // RCx_OPTION often uses "<MODE> Mode" (e.g. LOITER Mode). Reuse flight-mode mapping.
    static const QString modeSuffix = QStringLiteral(" Mode");
    if (text.endsWith(modeSuffix)) {
        const QString base = text.chopped(modeSuffix.size());
        const QString translatedBase = FlightModeNameTranslator::translate(base, fixedWing, locale);
        if (translatedBase != base) {
            return translatedBase + QStringLiteral("模式");
        }
    }

    return FlightModeNameTranslator::translate(text, fixedWing, locale);
}
