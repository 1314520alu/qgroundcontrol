#include "UnipodMt11ProtocolTest.h"

#include "UnipodMt11Protocol.h"

void UnipodMt11ProtocolTest::testPhotoFrameMatchesHandbook()
{
    const QByteArray frame = UnipodMt11Protocol::buildPhotoCommand(0);
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("55 66 01 01 00 00 00 0C 00 34 CE"));
}

void UnipodMt11ProtocolTest::testRecordFrameMatchesHandbook()
{
    const QByteArray frame = UnipodMt11Protocol::buildRecordToggleCommand(0);
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("55 66 01 01 00 00 00 0C 02 76 EE"));
}

void UnipodMt11ProtocolTest::testSystemInfoRequestMatchesHandbook()
{
    const QByteArray frame = UnipodMt11Protocol::buildSystemInfoRequest(0);
    QCOMPARE(frame.toHex(' ').toUpper(), QByteArray("55 66 01 00 00 00 00 0A 0F 75"));
}

void UnipodMt11ProtocolTest::testParseRoundTrip()
{
    const QByteArray frame = UnipodMt11Protocol::buildPhotoCommand(7);
    quint8 ctrl = 0;
    quint8 cmd = 0;
    quint16 seq = 0;
    QByteArray payload;
    QVERIFY(UnipodMt11Protocol::parseFrame(frame, &ctrl, &seq, &cmd, &payload));
    QCOMPARE(ctrl, quint8(0x01));
    QCOMPARE(cmd, quint8(0x0C));
    QCOMPARE(seq, quint16(7));
    QCOMPARE(payload, QByteArray(1, char(0x00)));
}

void UnipodMt11ProtocolTest::testParseSystemInfoAck()
{
    QByteArray payload(4, char(0x00));
    payload[3] = char(0x02);

    UnipodMt11Protocol::SystemInfoAck ack;
    QVERIFY(UnipodMt11Protocol::parseSystemInfoAck(payload, &ack));
    QCOMPARE(ack.recordSta, quint8(0x02));

    const QByteArray shortPayload(3, char(0x00));
    UnipodMt11Protocol::SystemInfoAck shortAck;
    QVERIFY(!UnipodMt11Protocol::parseSystemInfoAck(shortPayload, &shortAck));
}

void UnipodMt11ProtocolTest::testParseFuncFeedback()
{
    quint8 infoType = 0xFF;
    QVERIFY(UnipodMt11Protocol::parseFuncFeedback(QByteArray(1, char(5)), &infoType));
    QCOMPARE(infoType, quint8(UnipodMt11Protocol::FuncFeedback::RecordStart));

    QVERIFY(UnipodMt11Protocol::parseFuncFeedback(QByteArray(1, char(6)), &infoType));
    QCOMPARE(infoType, quint8(UnipodMt11Protocol::FuncFeedback::RecordEnd));

    QVERIFY(!UnipodMt11Protocol::parseFuncFeedback(QByteArray(), &infoType));
}

UT_REGISTER_TEST(UnipodMt11ProtocolTest, TestLabel::Unit)
