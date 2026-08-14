#include "ParameterEnumStringTranslatorTest.h"

#include "ParameterEnumStringTranslator.h"

#include <QtCore/QLocale>

void ParameterEnumStringTranslatorTest::_testEnglishUnchanged()
{
    const QLocale english(QLocale::English, QLocale::UnitedStates);
    const QString text = QStringLiteral("Warn only");
    QCOMPARE(ParameterEnumStringTranslator::translate(text, english), text);
}

void ParameterEnumStringTranslatorTest::_testChineseBatteryFailsafeActions()
{
    const QLocale chinese(QStringLiteral("zh_CN"));

    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Warn only"), chinese),
             QStringLiteral("仅警告"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Land"), chinese),
             QStringLiteral("降落"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("RTL"), chinese),
             QStringLiteral("返航"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("SmartRTL or RTL"), chinese),
             QStringLiteral("SmartRTL 或返航"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("SmartRTL or Land"), chinese),
             QStringLiteral("SmartRTL 或降落"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Terminate"), chinese),
             QStringLiteral("终止"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Brake or Land"), chinese),
             QStringLiteral("刹车或降落"));
}

void ParameterEnumStringTranslatorTest::_testChineseFlightModeEnums()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const bool copter = false;
    const bool plane = true;

    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Stabilize"), chinese, copter),
             QStringLiteral("自稳"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("AltHold"), chinese, copter),
             QStringLiteral("定高"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Loiter"), chinese, copter),
             QStringLiteral("定点悬停"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("PosHold"), chinese, copter),
             QStringLiteral("定点"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("SmartRTL"), chinese, copter),
             QStringLiteral("智能返航"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Avoid_ADSB"), chinese, copter),
             QStringLiteral("避让 ADSB"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Loiter"), chinese, plane),
             QStringLiteral("盘旋"));
}

void ParameterEnumStringTranslatorTest::_testChineseRcOptionEnums()
{
    const QLocale chinese(QStringLiteral("zh_CN"));

    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Do Nothing"), chinese),
             QStringLiteral("无操作"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Save Trim"), chinese),
             QStringLiteral("保存配平"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Camera Trigger"), chinese),
             QStringLiteral("相机触发"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("Motor Emergency Stop"), chinese),
             QStringLiteral("电机紧急停转"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("STABILIZE Mode"), chinese),
             QStringLiteral("自稳模式"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("ALTHOLD Mode"), chinese),
             QStringLiteral("定高模式"));
    QCOMPARE(ParameterEnumStringTranslator::translate(QStringLiteral("LOITER Mode"), chinese, false),
             QStringLiteral("定点悬停模式"));
}

void ParameterEnumStringTranslatorTest::_testChineseUnknownLeftInEnglish()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const QString unknown = QStringLiteral("CustomActionXYZ");
    QCOMPARE(ParameterEnumStringTranslator::translate(unknown, chinese), unknown);
}

void ParameterEnumStringTranslatorTest::_testChineseTranslateList()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const QStringList input = {
        QStringLiteral("Warn only"),
        QStringLiteral("CustomActionXYZ"),
        QStringLiteral("Land"),
        QStringLiteral("Stabilize"),
        QStringLiteral("AltHold"),
    };
    const QStringList expected = {
        QStringLiteral("仅警告"),
        QStringLiteral("CustomActionXYZ"),
        QStringLiteral("降落"),
        QStringLiteral("自稳"),
        QStringLiteral("定高"),
    };
    QCOMPARE(ParameterEnumStringTranslator::translateList(input, chinese), expected);
}

#include "UnitTest.h"

UT_REGISTER_TEST(ParameterEnumStringTranslatorTest, TestLabel::Unit)
