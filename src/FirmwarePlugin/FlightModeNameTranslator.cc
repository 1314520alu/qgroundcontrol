#include "FlightModeNameTranslator.h"

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(FlightModeNameTranslatorLog, "FirmwarePlugin.FlightModeNameTranslator")

namespace {

const QRegularExpression &duplicateSuffixRe()
{
    static const QRegularExpression re(QStringLiteral(R"( \((\d+)\)$)"));
    return re;
}

const QHash<QString, QString> &chineseNames()
{
    // Keys are lowercase with spaces, underscores, and hyphens removed.
    static const QHash<QString, QString> names = {
        // Copter / shared ArduPilot
        {QStringLiteral("stabilize"), QStringLiteral("自稳")},
        {QStringLiteral("stabilized"), QStringLiteral("自稳")},
        {QStringLiteral("acro"), QStringLiteral("特技")},
        {QStringLiteral("althold"), QStringLiteral("定高")},
        {QStringLiteral("altitudehold"), QStringLiteral("定高")},
        {QStringLiteral("altitude"), QStringLiteral("定高")},
        {QStringLiteral("altctl"), QStringLiteral("定高")},
        {QStringLiteral("auto"), QStringLiteral("自动")},
        {QStringLiteral("guided"), QStringLiteral("引导")},
        {QStringLiteral("rtl"), QStringLiteral("返航")},
        {QStringLiteral("return"), QStringLiteral("返航")},
        {QStringLiteral("circle"), QStringLiteral("绕圈")},
        {QStringLiteral("land"), QStringLiteral("降落")},
        {QStringLiteral("landing"), QStringLiteral("降落")},
        {QStringLiteral("drift"), QStringLiteral("漂移")},
        {QStringLiteral("sport"), QStringLiteral("运动")},
        {QStringLiteral("flip"), QStringLiteral("翻转")},
        {QStringLiteral("autotune"), QStringLiteral("自动调参")},
        {QStringLiteral("poshold"), QStringLiteral("定点")},
        {QStringLiteral("positionhold"), QStringLiteral("定点")},
        {QStringLiteral("position"), QStringLiteral("定点")},
        {QStringLiteral("posctl"), QStringLiteral("定点")},
        {QStringLiteral("posctlposctl"), QStringLiteral("定点")},
        {QStringLiteral("brake"), QStringLiteral("刹车")},
        {QStringLiteral("throw"), QStringLiteral("抛飞")},
        {QStringLiteral("avoidadsb"), QStringLiteral("避让 ADSB")},
        {QStringLiteral("guidednogps"), QStringLiteral("引导（无 GPS）")},
        {QStringLiteral("smartrtl"), QStringLiteral("智能返航")},
        {QStringLiteral("flowhold"), QStringLiteral("光流悬停")},
        {QStringLiteral("follow"), QStringLiteral("跟随")},
        {QStringLiteral("followme"), QStringLiteral("跟随")},
        {QStringLiteral("zigzag"), QStringLiteral("之字形")},
        {QStringLiteral("systemid"), QStringLiteral("系统辨识")},
        {QStringLiteral("autorotate"), QStringLiteral("自转")},
        {QStringLiteral("heliautorotate"), QStringLiteral("自转")},
        {QStringLiteral("autortl"), QStringLiteral("自动返航")},
        {QStringLiteral("turtle"), QStringLiteral("翻转自救")},

        // Plane
        {QStringLiteral("manual"), QStringLiteral("手动")},
        {QStringLiteral("training"), QStringLiteral("训练")},
        {QStringLiteral("fbwa"), QStringLiteral("姿态")},
        {QStringLiteral("flybywirea"), QStringLiteral("姿态")},
        {QStringLiteral("fbwb"), QStringLiteral("定高")},
        {QStringLiteral("flybywireb"), QStringLiteral("定高")},
        {QStringLiteral("cruise"), QStringLiteral("巡航")},
        {QStringLiteral("takeoff"), QStringLiteral("起飞")},
        {QStringLiteral("initializing"), QStringLiteral("初始化")},
        {QStringLiteral("qstabilize"), QStringLiteral("垂起自稳")},
        {QStringLiteral("quadplanestabilize"), QStringLiteral("垂起自稳")},
        {QStringLiteral("qhover"), QStringLiteral("垂起悬停")},
        {QStringLiteral("quadplanehover"), QStringLiteral("垂起悬停")},
        {QStringLiteral("qloiter"), QStringLiteral("垂起定点")},
        {QStringLiteral("quadplaneloiter"), QStringLiteral("垂起定点")},
        {QStringLiteral("qland"), QStringLiteral("垂起降落")},
        {QStringLiteral("quadplaneland"), QStringLiteral("垂起降落")},
        {QStringLiteral("qrtl"), QStringLiteral("垂起返航")},
        {QStringLiteral("quadplanertl"), QStringLiteral("垂起返航")},
        {QStringLiteral("qautotune"), QStringLiteral("垂起自动调参")},
        {QStringLiteral("quadplaneautotune"), QStringLiteral("垂起自动调参")},
        {QStringLiteral("qacro"), QStringLiteral("垂起特技")},
        {QStringLiteral("quadplaneacro"), QStringLiteral("垂起特技")},
        {QStringLiteral("thermal"), QStringLiteral("热升力")},
        {QStringLiteral("loitertoqland"), QStringLiteral("盘旋转垂起降落")},
        {QStringLiteral("autoland"), QStringLiteral("自动降落")},

        // Rover
        {QStringLiteral("learning"), QStringLiteral("学习")},
        {QStringLiteral("steering"), QStringLiteral("转向")},
        {QStringLiteral("hold"), QStringLiteral("保持")},
        {QStringLiteral("simple"), QStringLiteral("简易")},
        {QStringLiteral("dock"), QStringLiteral("停靠")},

        // Sub
        {QStringLiteral("depthhold"), QStringLiteral("定深")},
        {QStringLiteral("surface"), QStringLiteral("上浮")},
        {QStringLiteral("motordetection"), QStringLiteral("电机检测")},
        {QStringLiteral("surftrak"), QStringLiteral("地形跟踪")},

        // PX4 / MAV_STANDARD_MODE
        {QStringLiteral("mission"), QStringLiteral("任务")},
        {QStringLiteral("orbit"), QStringLiteral("绕飞")},
        {QStringLiteral("offboard"), QStringLiteral("外部控制")},
        {QStringLiteral("ready"), QStringLiteral("就绪")},
        {QStringLiteral("rattitude"), QStringLiteral("半自稳")},
        {QStringLiteral("precisionland"), QStringLiteral("精确降落")},
        {QStringLiteral("positionslow"), QStringLiteral("慢速定点")},
        {QStringLiteral("altitudecruise"), QStringLiteral("定高巡航")},
        {QStringLiteral("termination"), QStringLiteral("终止")},
        {QStringLiteral("vtoltakeoff"), QStringLiteral("垂起起飞")},
        {QStringLiteral("guidedcourse"), QStringLiteral("引导航线")},
        {QStringLiteral("saferecovery"), QStringLiteral("安全回收")},
    };
    return names;
}

} // namespace

QString FlightModeNameTranslator::translate(const QString &name, bool fixedWing)
{
    const QLocale locale = qgcApp() ? qgcApp()->getCurrentLanguage() : QLocale();
    return translate(name, fixedWing, locale);
}

QString FlightModeNameTranslator::translate(const QString &name, bool fixedWing, const QLocale &locale)
{
    if (name.isEmpty() || !_shouldTranslate(locale)) {
        return name;
    }

    const QString translated = _translateChinese(name, fixedWing);
    if (translated != name) {
        qCDebug(FlightModeNameTranslatorLog) << name << "->" << translated;
    }
    return translated;
}

bool FlightModeNameTranslator::_shouldTranslate(const QLocale &locale)
{
    return locale.language() == QLocale::Chinese;
}

QString FlightModeNameTranslator::_normalizedKey(const QString &name)
{
    QString key = name.trimmed().toLower();
    key.remove(QLatin1Char(' '));
    key.remove(QLatin1Char('_'));
    key.remove(QLatin1Char('-'));
    return key;
}

QString FlightModeNameTranslator::_translateChinese(const QString &name, bool fixedWing)
{
    QString base = name;
    QString suffix;
    const QRegularExpressionMatch match = duplicateSuffixRe().match(name);
    if (match.hasMatch()) {
        suffix = match.captured(0);
        base.chop(suffix.size());
    }

    const QString key = _normalizedKey(base);
    if (key == QLatin1String("loiter")) {
        // Copter/rover GPS hold vs airplane circling. Plane plugins mark fixedWing.
        return (fixedWing ? QStringLiteral("盘旋") : QStringLiteral("定点悬停")) + suffix;
    }

    const QString translated = chineseNames().value(key);
    if (translated.isEmpty()) {
        return name;
    }
    return translated + suffix;
}
