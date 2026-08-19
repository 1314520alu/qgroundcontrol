#include "TopotekTq10ProtocolTest.h"

#include "TopotekTq10Protocol.h"

void TopotekTq10ProtocolTest::testCapFrameMatchesPython()
{
    const QByteArray frame = TopotekTq10Protocol::buildCap();
    QCOMPARE(frame, QByteArray("#TPPD2wCAP008"));
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("23 54 50 50 44 32 77 43 41 50 30 30 38"));
}

void TopotekTq10ProtocolTest::testRecToggleFrameMatchesPython()
{
    const QByteArray frame = TopotekTq10Protocol::buildRecToggle();
    QCOMPARE(frame, QByteArray("#TPPD2wREC0A4F"));
}

void TopotekTq10ProtocolTest::testRecQueryFrameMatchesPython()
{
    const QByteArray frame = TopotekTq10Protocol::buildRecQuery();
    QCOMPARE(frame, QByteArray("#TPPD2rREC009"));
}

void TopotekTq10ProtocolTest::testZoomStopFrameMatchesPython()
{
    const QByteArray frame = TopotekTq10Protocol::buildZoom("00");
    QCOMPARE(frame, QByteArray("#TPPM2wZMC0057"));
}

void TopotekTq10ProtocolTest::testPtzStopFrameMatchesPython()
{
    const QByteArray frame = TopotekTq10Protocol::buildPtz("00");
    QCOMPARE(frame, QByteArray("#TPPG2wPTZ0065"));
}

void TopotekTq10ProtocolTest::testPtzHomeFrameMatchesPython()
{
    const QByteArray frame = TopotekTq10Protocol::buildPtz("05");
    QCOMPARE(frame, QByteArray("#TPPG2wPTZ056A"));
}

void TopotekTq10ProtocolTest::testCrcVerification()
{
    const QByteArray frame = TopotekTq10Protocol::buildCap();
    QVERIFY(TopotekTq10Protocol::verifyCrc(frame));
    QVERIFY(!TopotekTq10Protocol::verifyCrc(frame.left(frame.size() - 1) + QByteArray("FF")));
}

void TopotekTq10ProtocolTest::testParseRecordState()
{
    quint8 recordSta = 0xFF;
    QVERIFY(TopotekTq10Protocol::parseRecordState(QByteArray("#tpDPrREC00AB"), &recordSta));
    QCOMPARE(recordSta, quint8(0));

    QVERIFY(TopotekTq10Protocol::parseRecordState(QByteArray("#tpDUAwREC11AB"), &recordSta));
    QCOMPARE(recordSta, quint8(1));

    QVERIFY(!TopotekTq10Protocol::parseRecordState(QByteArray("#TPPG2wPTZ0065"), &recordSta));
}

UT_REGISTER_TEST(TopotekTq10ProtocolTest, TestLabel::Unit)
