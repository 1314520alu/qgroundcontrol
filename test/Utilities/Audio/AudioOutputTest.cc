#include "AudioOutputTest.h"

#include "AudioOutput.h"

#include <QtCore/QLocale>

void AudioOutputTest::_testSpokenReplacements()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10.5m, -10.5m. -10.5 m"));
    QCOMPARE(result,
             QStringLiteral("negative 10 point 5 meters, negative 10 point 5 meters. negative 10 point 5  meters"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10m -10 m"));
    QCOMPARE(result, QStringLiteral("negative 10 meters negative 10  meters"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("foo -10m -10 m bar"));
    QCOMPARE(result, QStringLiteral("foo negative 10 meters negative 10  meters bar"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-foom"));
    QCOMPARE(result, QStringLiteral("-foom"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10 moo"));
    QCOMPARE(result, QStringLiteral("10 moo"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10moo"));
    QCOMPARE(result, QStringLiteral("10moo"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10ms"));
    QCOMPARE(result, QStringLiteral("10ms"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1000ms"));
    QCOMPARE(result, QStringLiteral("1 second"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1001ms"));
    QCOMPARE(result, QStringLiteral("1 second and 1 millisecond"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10.5m"), QLocale(QStringLiteral("zh_CN")));
    QCOMPARE(result, QStringLiteral("负 10 点 5 米"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1001ms"), QLocale(QStringLiteral("zh_CN")));
    QCOMPARE(result, QStringLiteral("1 秒 1 毫秒"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("RTL"), QLocale(QStringLiteral("zh_CN")));
    QCOMPARE(result, QStringLiteral("返航"));
}

#include "UnitTest.h"

UT_REGISTER_TEST(AudioOutputTest, TestLabel::Unit, TestLabel::Utilities)
