#pragma once

#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QStringList>

/// \brief Translates firmware parameter enum/bitmask labels for display.
///
/// Autopilot parameter metadata (e.g. BATT_FS_LOW_ACT, FLTMODE1) is authored in
/// English. When the QGC UI language is Chinese, known labels are replaced with
/// field terminology. Unknown labels are left unchanged. Flight-mode names fall
/// back to FlightModeNameTranslator.
class ParameterEnumStringTranslator
{
public:
    /// Translate using the current QGC UI language and active-vehicle type.
    static QString translate(const QString &text);

    /// Translate using an explicit locale. Used by unit tests.
    static QString translate(const QString &text, const QLocale &locale);

    /// Translate using an explicit locale and vehicle type. Used by unit tests.
    static QString translate(const QString &text, const QLocale &locale, bool fixedWing);

    /// Translate each entry; unknown strings are left unchanged.
    static QStringList translateList(const QStringList &strings);
    static QStringList translateList(const QStringList &strings, const QLocale &locale);
    static QStringList translateList(const QStringList &strings, const QLocale &locale, bool fixedWing);

private:
    static bool _shouldTranslate(const QLocale &locale);
    static bool _activeVehicleFixedWing();
    static QString _translateChinese(const QString &text, const QLocale &locale, bool fixedWing);
};
