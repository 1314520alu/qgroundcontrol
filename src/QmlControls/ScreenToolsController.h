#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

/// \brief This Qml control is used to return screen parameters
///
class ScreenToolsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool isAndroid READ isAndroid CONSTANT)
    Q_PROPERTY(bool isiOS READ isiOS CONSTANT)
    Q_PROPERTY(bool isMobile READ isMobile CONSTANT)
    Q_PROPERTY(bool fakeMobile READ fakeMobile CONSTANT)
    Q_PROPERTY(bool isDebug READ isDebug CONSTANT)
    Q_PROPERTY(bool isMacOS READ isMacOS CONSTANT)
    Q_PROPERTY(bool isLinux READ isLinux CONSTANT)
    Q_PROPERTY(bool isWindows READ isWindows CONSTANT)
    Q_PROPERTY(bool isSerialAvailable READ isSerialAvailable CONSTANT)
    Q_PROPERTY(bool hasTouch READ hasTouch CONSTANT)
    Q_PROPERTY(QString iOSDevice READ iOSDevice CONSTANT)
    Q_PROPERTY(QString fixedFontFamily READ fixedFontFamily CONSTANT)
    Q_PROPERTY(QString normalFontFamily READ normalFontFamily CONSTANT)

public:
    explicit ScreenToolsController(QObject* parent = nullptr);
    ~ScreenToolsController();

    /// Returns current mouse position
    Q_INVOKABLE static int mouseX();
    Q_INVOKABLE static int mouseY();

    // QFontMetrics::descent for default font
    Q_INVOKABLE static double defaultFontDescent(int pointSize);

    /// Android Build.MODEL (empty on non-Android).
    Q_INVOKABLE static QString androidProductModel();
    /// True if the given Android package is installed (always false on non-Android).
    Q_INVOKABLE static bool androidPackageInstalled(const QString& packageName);
    /// Best-effort remote GCS preset name matching LinkConfigurationManager tiles, or empty.
    Q_INVOKABLE static QString detectRemoteControllerPreset();
    /// Recommended UI scale percent for a remote-preset tile name, or 0 if unknown.
    Q_INVOKABLE static int recommendedUiScalePercentForPreset(const QString& presetName);
    /// Detected Android remote's recommended UI scale, or 0 if none.
    Q_INVOKABLE static int recommendedUiScalePercent();

    /// Kick SIYI radio ethernet (eth0 / 192.168.144.x) bring-up on Android remotes. No-op elsewhere.
    Q_INVOKABLE static void ensureSiyiRadioEthernet();
    /// True when a local IPv4 on 192.168.144.x is present.
    Q_INVOKABLE static bool isSiyiRadioEthernetReady();
    /// Local 192.168.144.x address, or empty.
    Q_INVOKABLE static QString siyiRadioEthernetAddress();

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    static bool isMobile() { return true; }

    static bool fakeMobile() { return false; }
#else
    static bool isMobile() { return fakeMobile(); }

    static bool fakeMobile();
#endif

#if defined(Q_OS_ANDROID)
    static bool isAndroid() { return true; }

    static bool isiOS() { return false; }

    static bool isLinux() { return false; }

    static bool isMacOS() { return false; }

    static bool isWindows() { return false; }
#elif defined(Q_OS_IOS)
    static bool isAndroid() { return false; }

    static bool isiOS() { return true; }

    static bool isLinux() { return false; }

    static bool isMacOS() { return false; }

    static bool isWindows() { return false; }
#elif defined(Q_OS_MACOS)
    static bool isAndroid() { return false; }

    static bool isiOS() { return false; }

    static bool isLinux() { return false; }

    static bool isMacOS() { return true; }

    static bool isWindows() { return false; }
#elif defined(Q_OS_LINUX)
    static bool isAndroid() { return false; }

    static bool isiOS() { return false; }

    static bool isLinux() { return true; }

    static bool isMacOS() { return false; }

    static bool isWindows() { return false; }
#elif defined(Q_OS_WIN)
    static bool isAndroid() { return false; }

    static bool isiOS() { return false; }

    static bool isLinux() { return false; }

    static bool isMacOS() { return false; }

    static bool isWindows() { return true; }
#else
    static bool isAndroid() { return false; }

    static bool isiOS() { return false; }

    static bool isLinux() { return false; }

    static bool isMacOS() { return false; }

    static bool isWindows() { return false; }
#endif

#if defined(QGC_NO_SERIAL_LINK)
    static bool isSerialAvailable() { return false; }
#else
    static bool isSerialAvailable() { return true; }
#endif

#ifdef QT_DEBUG
    static bool isDebug() { return true; }
#else
    static bool isDebug() { return false; }
#endif

    static bool hasTouch();
    static QString iOSDevice();
    static QString fixedFontFamily();
    static QString normalFontFamily();
};
