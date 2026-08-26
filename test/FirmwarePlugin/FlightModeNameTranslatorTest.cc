#include "FlightModeNameTranslatorTest.h"

#include <QtCore/QLocale>

#include "FlightModeNameTranslator.h"

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
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Auto"), fixedWing, chinese), QStringLiteral("自动"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Stabilize"), fixedWing, chinese),
             QStringLiteral("自稳"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Altitude Hold"), fixedWing, chinese),
             QStringLiteral("定高"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Loiter"), fixedWing, chinese),
             QStringLiteral("定点悬停"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Land"), fixedWing, chinese), QStringLiteral("降落"));
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("RTL"), fixedWing, chinese), QStringLiteral("返航"));
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
    QCOMPARE(FlightModeNameTranslator::translate(QStringLiteral("Loiter"), true, chinese), QStringLiteral("盘旋"));
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

void FlightModeNameTranslatorTest::_testNamesMatchEnglishAndChinese()
{
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("Stabilize"), QStringLiteral("自稳")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("Altitude Hold"), QStringLiteral("定高")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("Loiter"), QStringLiteral("定点悬停")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("AutoRTL"), QStringLiteral("自动返航")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("RTL"), QStringLiteral("返航")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("Flip"), QStringLiteral("翻滚")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("Flip"), QStringLiteral("翻转")));
    QVERIFY(FlightModeNameTranslator::namesMatch(QStringLiteral("Position Hold"), QStringLiteral("定点")));
    QVERIFY(!FlightModeNameTranslator::namesMatch(QStringLiteral("Loiter"), QStringLiteral("定高")));
    QVERIFY(!FlightModeNameTranslator::namesMatch(QStringLiteral("Auto"), QStringLiteral("自动返航")));
}

void FlightModeNameTranslatorTest::_testIsHiddenCopterDailyModes()
{
    const QStringList hidden =
        QStringLiteral(
            "Acro,Circle,Drift,Sport,Flip,Brake,Throw,Guided,Guided No GPS,Flow Hold,"
            "ZigZag,Turtle,Autotune,SystemID,AutoRotate,Position Hold,Avoid ADSB,Smart RTL,Follow")
            .split(QLatin1Char(','));

    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("Stabilize"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("自稳"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("Altitude Hold"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("定高"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("Loiter"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("定点悬停"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("Auto"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("自动"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("RTL"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("返航"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("Land"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("降落"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("AutoRTL"), hidden));
    QVERIFY(!FlightModeNameTranslator::isHidden(QStringLiteral("自动返航"), hidden));

    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("Guided"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("引导"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("Position Hold"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("定点"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("Smart RTL"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("智能返航"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("Follow"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("跟随"), hidden));
    QVERIFY(FlightModeNameTranslator::isHidden(QStringLiteral("Avoid ADSB"), hidden));
}

#include "UnitTest.h"

UT_REGISTER_TEST(FlightModeNameTranslatorTest, TestLabel::Unit)
