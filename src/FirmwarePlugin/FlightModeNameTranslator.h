#pragma once

#include <QtCore/QLocale>
#include <QtCore/QString>

/// \brief Translates firmware flight-mode names for display and mode-set matching.
///
/// Autopilot AVAILABLE_MODES and plugin fallbacks are authored in English. When the
/// QGC UI language is Chinese, known names are replaced with field terminology.
/// Unknown names are left unchanged. Display and setFlightMode use the same string.
class FlightModeNameTranslator
{
public:
    /// Translate using the current QGC UI language.
    static QString translate(const QString &name, bool fixedWing);

    /// Translate using an explicit locale. Used by unit tests.
    static QString translate(const QString &name, bool fixedWing, const QLocale &locale);

private:
    static bool _shouldTranslate(const QLocale &locale);
    static QString _translateChinese(const QString &name, bool fixedWing);
    static QString _normalizedKey(const QString &name);
};
