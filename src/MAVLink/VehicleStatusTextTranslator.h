#pragma once

#include <QtCore/QLocale>
#include <QtCore/QString>

/// \brief Translates firmware STATUSTEXT for display and speech.
///
/// Autopilot STATUSTEXT is authored in English. When the QGC UI language is Chinese,
/// known operator-facing phrases are replaced with field terminology (not literal
/// machine translation). Unknown fragments and protocol strings such as `[cal]` are
/// left unchanged. Calibration/parsers should keep using the original English text.
class VehicleStatusTextTranslator
{
public:
    /// Translate using the current QGC UI language.
    static QString translate(const QString &text);

    /// Translate using an explicit locale. Used by unit tests.
    static QString translate(const QString &text, const QLocale &locale);

private:
    static bool _shouldTranslate(const QLocale &locale);
    static QString _translateChinese(QString text);
};
