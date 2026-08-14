#include "FlightModeNameTranslatorTest.h"

#include "FlightModeNameTranslator.h"

#include <QtCore/QLocale>

void FlightModeNameTranslatorTest::_testEnglishUnchanged()
{
    const QLocale english(QLocale::English, QLocale::UnitedStates);
    const QString name = QStringLiteral("Altitude Hold");
    QCOMPARE(FlightModeNameTranslator::translate(name, false, english), name);
}

void FlightModeNameTranslatorTest::_testChineseCopterModesFromFirmware()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const bool fixedWing = false;

    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Auto RTL"), fixedWing, chinese),
             QStringLiteral("自动返航"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Auto"), fixedWing, chinese),
             QStringLiteral("自动"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Stabilize"), fixedWing, chinese),
             QStringLiteral("自稳"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Altitude Hold"), fixedWing, chinese),
             QStringLiteral("定高"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Loiter"), fixedWing, chinese),
             QStringLiteral("定点悬停"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Land"), fixedWing, chinese),
             QStringLiteral("降落"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("RTL"), fixedWing, chinese),
             QStringLiteral("返航"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Position Hold"), fixedWing, chinese),
             QStringLiteral("定点"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Avoid ADSB"), fixedWing, chinese),
             QStringLiteral("避让 ADSB"));
}

void FlightModeNameTranslatorTest::_testChineseCopterModeAliases()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const bool fixedWing = false;

    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("AutoRTL"), fixedWing, chinese),
             QStringLiteral("自动返航"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("ALT_HOLD"), fixedWing, chinese),
             QStringLiteral("定高"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("POSHOLD"), fixedWing, chinese),
             QStringLiteral("定点"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Smart RTL"), fixedWing, chinese),
             QStringLiteral("智能返航"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Heli_Autorotate"), fixedWing, chinese),
             QStringLiteral("自转"));
}

void FlightModeNameTranslatorTest::_testChinesePlaneLoiter()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Loiter"), true, chinese),
             QStringLiteral("盘旋"));
}

void FlightModeNameTranslatorTest::_testChineseUnknownLeftInEnglish()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const QString unknown = QStringLiteral("CustomModeXYZ");
    QCOMPARE(FlightModeNameTranslator::translate(unknown, false, chinese), unknown);
}

void FlightModeNameTranslatorTest::_testChineseDuplicateSuffixPreserved()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Loiter (1)"), false, chinese),
             QStringLiteral("定点悬停 (1)"));
}

#include "UnitTest.h"

UT_REGISTER_TEST(FlightModeNameTranslatorTest, TestLabel::Unit)
