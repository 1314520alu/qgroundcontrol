#include "ScreenToolsController.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtGui/QCursor>
#include <QtGui/QFontDatabase>
#include <QtGui/QFontMetrics>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputDevice>
#include <QtGui/QScreen>

#include <QtCore/QtMath>

#if defined(Q_OS_ANDROID)
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

#if defined(Q_OS_IOS)
#include <sys/utsname.h>
#endif

QGC_LOGGING_CATEGORY(ScreenToolsControllerLog, "QMLControls.ScreenToolsController")

ScreenToolsController::ScreenToolsController(QObject *parent)
    : QObject(parent)
{
    // qCDebug(ScreenToolsControllerLog) << Q_FUNC_INFO << this;
}

ScreenToolsController::~ScreenToolsController()
{
    // qCDebug(ScreenToolsControllerLog) << Q_FUNC_INFO << this;
}

int ScreenToolsController::mouseX()
{
    return QCursor::pos().x();
}

int ScreenToolsController::mouseY()
{
    return QCursor::pos().y();
}

bool ScreenToolsController::hasTouch()
{
    for (const auto &inputDevice: QInputDevice::devices()) {
        if (inputDevice->type() == QInputDevice::DeviceType::TouchScreen) {
            return true;
        }
    }
    return false;
}

QString ScreenToolsController::iOSDevice()
{
#if defined(Q_OS_IOS)
    struct utsname systemInfo;
    uname(&systemInfo);
    return QString(systemInfo.machine);
#else
    return QString();
#endif
}

QString ScreenToolsController::fixedFontFamily()
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QString ScreenToolsController::normalFontFamily()
{
    //-- See App.SettinsGroup.json for index
    const int langID = SettingsManager::instance()->appSettings()->qLocaleLanguage()->rawValue().toInt();
    if (langID == QLocale::Korean) {
        return QStringLiteral("NanumGothic");
    }

    return QStringLiteral("Open Sans");
}

double ScreenToolsController::defaultFontDescent(int pointSize)
{
    return QFontMetrics(QFont(normalFontFamily(), pointSize)).descent();
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
bool ScreenToolsController::fakeMobile()
{
    return qgcApp()->fakeMobile();
}
#endif

QString ScreenToolsController::androidProductModel()
{
#if defined(Q_OS_ANDROID)
    const QJniObject model = QJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;");
    return model.isValid() ? model.toString() : QString();
#else
    return QString();
#endif
}

bool ScreenToolsController::androidPackageInstalled(const QString &packageName)
{
#if defined(Q_OS_ANDROID)
    if (packageName.isEmpty()) {
        return false;
    }

    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) {
        return false;
    }

    const QJniObject packageManager = context.callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
    if (!packageManager.isValid()) {
        return false;
    }

    const QJniObject jPackageName = QJniObject::fromString(packageName);
    const QJniObject packageInfo = packageManager.callObjectMethod(
        "getPackageInfo",
        "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;",
        jPackageName.object<jstring>(),
        jint(0));

    QJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    return packageInfo.isValid();
#else
    Q_UNUSED(packageName);
    return false;
#endif
}

QString ScreenToolsController::detectRemoteControllerPreset()
{
#if !defined(Q_OS_ANDROID)
    return QString();
#else
    const QString model = androidProductModel().toLower();

    const bool hasSiyi = androidPackageInstalled(QStringLiteral("com.siyi.udpservice"))
                         || androidPackageInstalled(QStringLiteral("biz.siyi.remotecontrol"));
    // G20 ships Device Tool / rcservice / rc_daemon; older images may use rcsdk/server/fly/fpv.
    const bool hasSkydroid = androidPackageInstalled(QStringLiteral("com.skydroid.rcsdk"))
                             || androidPackageInstalled(QStringLiteral("com.skydroid.server"))
                             || androidPackageInstalled(QStringLiteral("com.skydroid.skydroidfly"))
                             || androidPackageInstalled(QStringLiteral("com.skydroid.fpv"))
                             || androidPackageInstalled(QStringLiteral("com.skydroid.rcservice"))
                             || androidPackageInstalled(QStringLiteral("com.skydroid.devicetool"))
                             || androidPackageInstalled(QStringLiteral("com.skydroid.rc_daemon"))
                             || model.contains(QStringLiteral("skydroid"));

    const auto modelContains = [&model](const QStringList &keys) {
        for (const QString &key : keys) {
            if (model.contains(key)) {
                return true;
            }
        }
        return false;
    };

    // Prefer explicit model / marketing strings.
    if (modelContains({QStringLiteral("unirc 10"), QStringLiteral("unirc10"), QStringLiteral("unirc_10"),
                       QStringLiteral("10inch"), QStringLiteral("10-inch"), QStringLiteral("10.1")})) {
        return QStringLiteral("UniRC 10 Pro");
    }
    if (modelContains({QStringLiteral("unirc 7"), QStringLiteral("unirc7"), QStringLiteral("unirc_7")})) {
        return QStringLiteral("UniRC 7");
    }
    if (modelContains({QStringLiteral("mk32")})) {
        return QStringLiteral("MK32");
    }
    if (modelContains({QStringLiteral("mk15")})) {
        return QStringLiteral("MK15");
    }
    if (modelContains({QStringLiteral("g20")})) {
        return QStringLiteral("云卓 G20");
    }
    if (modelContains({QStringLiteral("g16")})) {
        return QStringLiteral("云卓 G16");
    }
    if (modelContains({QStringLiteral("h30")})) {
        return QStringLiteral("云卓 H30");
    }
    if (modelContains({QStringLiteral("h16")})) {
        return QStringLiteral("云卓 H16");
    }

    qreal diagonalInches = 0;
    if (QScreen *const screen = QGuiApplication::primaryScreen()) {
        const QSizeF mm = screen->physicalSize();
        diagonalInches = qSqrt(mm.width() * mm.width() + mm.height() * mm.height()) / 25.4;
    }

    if (hasSkydroid) {
        // Build.MODEL is often a board string (e.g. "Bengal for arm64") with no G20/H16 token.
        // G/H series all use the same on-device UDP bridge (14551 ↔ 127.0.0.1:14552).
        if (diagonalInches >= 9.0) {
            qCDebug(ScreenToolsControllerLog) << "Skydroid ~10\" class; model:" << model << "diag:" << diagonalInches;
            return QStringLiteral("云卓 H30");
        }
        qCDebug(ScreenToolsControllerLog) << "Skydroid 7\"-class G-series; model:" << model << "diag:" << diagonalInches;
        return QStringLiteral("云卓 G20");
    }

    if (hasSiyi) {
        if (diagonalInches >= 9.0) {
            return QStringLiteral("UniRC 10 Pro");
        }
        // 7" class SIYI without MK token → UniRC 7 (same on-device UDP as UniRC 10 Pro).
        return QStringLiteral("UniRC 7");
    }

    qCDebug(ScreenToolsControllerLog) << "No remote controller detected; model:" << model;
    return QString();
#endif
}

void ScreenToolsController::ensureSiyiRadioEthernet()
{
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(
        "org/mavlink/qgroundcontrol/QGCSiyiEthernetHelper",
        "ensureRadioEthernet",
        "()V");
    QJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        qCWarning(ScreenToolsControllerLog) << "ensureSiyiRadioEthernet JNI exception";
    }
#endif
}

bool ScreenToolsController::isSiyiRadioEthernetReady()
{
#if defined(Q_OS_ANDROID)
    const jboolean ready = QJniObject::callStaticMethod<jboolean>(
        "org/mavlink/qgroundcontrol/QGCSiyiEthernetHelper",
        "isRadioEthernetReady",
        "()Z");
    QJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return ready;
#else
    return false;
#endif
}

QString ScreenToolsController::siyiRadioEthernetAddress()
{
#if defined(Q_OS_ANDROID)
    const QJniObject address = QJniObject::callStaticObjectMethod(
        "org/mavlink/qgroundcontrol/QGCSiyiEthernetHelper",
        "radioEthernetAddress",
        "()Ljava/lang/String;");
    QJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return QString();
    }
    return address.isValid() ? address.toString() : QString();
#else
    return QString();
#endif
}
