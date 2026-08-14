#include "VehicleStatusTextTranslatorTest.h"

#include "VehicleStatusTextTranslator.h"

#include <QtCore/QLocale>

void VehicleStatusTextTranslatorTest::_testEnglishUnchanged()
{
    const QLocale english(QLocale::English, QLocale::UnitedStates);
    const QString text = QStringLiteral("PreArm: Hardware safety switch");
    QCOMPARE(VehicleStatusTextTranslator::translate(text, english), text);
}

void VehicleStatusTextTranslatorTest::_testChinesePreArm()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("PreArm: Hardware safety switch"), chinese),
             QStringLiteral("解锁前检查：安全开关未按下"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("PreArm: Compass not calibrated"), chinese),
             QStringLiteral("解锁前检查：罗盘未校准"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("PreArm: Battery 1 below minimum arming voltage"), chinese),
             QStringLiteral("解锁前检查：电池1电压低于解锁最低值"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("PreArm: Battery 1 below minimum arming capacity"), chinese),
             QStringLiteral("解锁前检查：电池1容量低于解锁最低值"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Arm: Motors already armed"), chinese),
             QStringLiteral("解锁：电机已解锁"));
}

void VehicleStatusTextTranslatorTest::_testChineseParameterPreserved()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("PreArm: Check FS_THR_VALUE"), chinese),
             QStringLiteral("解锁前检查：请检查 FS_THR_VALUE"));
}

void VehicleStatusTextTranslatorTest::_testChineseProtocolUnchanged()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const QString cal = QStringLiteral("[cal] calibration started: 2 gyro");
    QCOMPARE(VehicleStatusTextTranslator::translate(cal, chinese), cal);
}

void VehicleStatusTextTranslatorTest::_testChineseFlightEvents()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Throttle armed"), chinese),
             QStringLiteral("油门已解锁"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Throttle disarmed"), chinese),
             QStringLiteral("油门已上锁"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Radio failsafe on"), chinese),
             QStringLiteral("遥控失控保护已触发"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("GPS 1: not healthy"), chinese),
             QStringLiteral("GPS 1：状态异常"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Ready to fly"), chinese),
             QStringLiteral("可以起飞"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Reached waypoint 3"), chinese),
             QStringLiteral("到达航点 3"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("EKF3 waiting for GPS config data"), chinese),
             QStringLiteral("EKF3 等待 GPS 配置数据"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("EK3 wait for GPS config data"), chinese),
             QStringLiteral("EKF3 等待 GPS 配置数据"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("GPS and AHRS differ by 12m"), chinese),
             QStringLiteral("GPS 与 AHRS 位置相差 12m"));
    QCOMPARE(VehicleStatusTextTranslator::translate(QStringLiteral("Main loop slow (350Hz < 400Hz)"), chinese),
             QStringLiteral("主循环过慢 (350Hz < 400Hz)"));
}

void VehicleStatusTextTranslatorTest::_testChineseUnknownLeftInEnglish()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const QString unknown = QStringLiteral("SomeUniqueFirmwareStringXYZ");
    QCOMPARE(VehicleStatusTextTranslator::translate(unknown, chinese), unknown);
}

void VehicleStatusTextTranslatorTest::_testHtmlUnchanged()
{
    const QLocale chinese(QStringLiteral("zh_CN"));
    const QString html = QStringLiteral("PreArm: <a href=\"param://ARMING_CHECK\">ARMING_CHECK</a>");
    QCOMPARE(VehicleStatusTextTranslator::translate(html, chinese), html);
}

#include "UnitTest.h"

UT_REGISTER_TEST(VehicleStatusTextTranslatorTest, TestLabel::Unit)
