#include "UnipodMt11ProtocolTest.h"

#include <QtCore/QtNumeric>

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

void UnipodMt11ProtocolTest::testParseFrameAcceptsTrailingPadding()
{
    const QByteArray frame = UnipodMt11Protocol::buildCurrentZoomRequest(0);
    QByteArray padded = frame;
    padded.append(char(0x00));
    padded.append(char(0x00));

    quint8 ctrl = 0;
    quint8 cmd = 0;
    quint16 seq = 0;
    QByteArray payload;
    QVERIFY(UnipodMt11Protocol::parseFrame(padded, &ctrl, &seq, &cmd, &payload));
    QCOMPARE(cmd, quint8(0x18));
    QCOMPARE(payload.size(), 0);
}

void UnipodMt11ProtocolTest::testParseFrameThenRemainder()
{
    const QByteArray first = UnipodMt11Protocol::buildCurrentZoomRequest(1);
    const QByteArray second = UnipodMt11Protocol::buildPhotoCommand(2);
    const QByteArray datagram = first + second;

    quint8 ctrl = 0;
    quint8 cmd = 0;
    quint16 seq = 0;
    QByteArray payload;
    QVERIFY(UnipodMt11Protocol::parseFrame(datagram, &ctrl, &seq, &cmd, &payload));
    QCOMPARE(cmd, quint8(0x18));
    QCOMPARE(seq, quint16(1));

    QVERIFY(UnipodMt11Protocol::parseFrame(datagram.mid(first.size()), &ctrl, &seq, &cmd, &payload));
    QCOMPARE(cmd, quint8(0x0C));
    QCOMPARE(seq, quint16(2));
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

void UnipodMt11ProtocolTest::testZoomFramesMatchHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildZoomCommand(0, 1).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 05 01 8D 64"));
    QCOMPARE(UnipodMt11Protocol::buildZoomCommand(0, 0).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 05 00 AC 74"));
    QCOMPARE(UnipodMt11Protocol::buildZoomCommand(0, -1).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 05 FF 5C 6A"));
}

void UnipodMt11ProtocolTest::testAbsoluteZoomFramesMatchHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildAbsoluteZoomCommand(0, 1.0).toHex(' ').toUpper(),
             QByteArray("55 66 01 02 00 00 00 0F 01 00 61 BE"));
    QCOMPARE(UnipodMt11Protocol::buildAbsoluteZoomCommand(0, 7.5).toHex(' ').toUpper(),
             QByteArray("55 66 01 02 00 00 00 0F 07 05 62 44"));
}

void UnipodMt11ProtocolTest::testZoomRangeRequestMatchesHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildZoomRangeRequest(0).toHex(' ').toUpper(),
             QByteArray("55 66 01 00 00 00 00 16 B2 A6"));
}

void UnipodMt11ProtocolTest::testNextHoldZoomCommand()
{
    using UnipodMt11Protocol::nextHoldZoomCommand;

    QCOMPARE(nextHoldZoomCommand(2.0, 1.0, qQNaN(), -1, 1, 1.4, 1.25, 1.0, 11.0), 2.8);
    QCOMPARE(nextHoldZoomCommand(2.0, 2.0, qQNaN(), -1, 1, 1.4, 1.25, 1.0, 11.0), 3.9);
    QCOMPARE(nextHoldZoomCommand(10.0, 1.0, qQNaN(), -1, -1, 1.4, 1.25, 1.0, 11.0), 7.1);
    QCOMPARE(nextHoldZoomCommand(2.0, 2.0, 2.0, 50, 1, 1.4, 1.25, 1.0, 11.0), 2.5);
    QCOMPARE(nextHoldZoomCommand(2.0, 2.0, 2.0, 1000, 1, 1.4, 1.25, 1.0, 11.0), 3.9);
    QCOMPARE(nextHoldZoomCommand(10.0, 5.0, qQNaN(), -1, 1, 1.4, 1.25, 1.0, 11.0), 11.0);
    QCOMPARE(nextHoldZoomCommand(1.2, 5.0, qQNaN(), -1, -1, 1.4, 1.25, 1.0, 11.0), 1.0);
}

void UnipodMt11ProtocolTest::testHoldZoomShouldStopMotor()
{
    using UnipodMt11Protocol::holdZoomShouldStopMotor;

    QVERIFY(holdZoomShouldStopMotor(1, 11.0, 1.0, 11.0));
    QVERIFY(!holdZoomShouldStopMotor(1, 10.9, 1.0, 11.0));
    QVERIFY(holdZoomShouldStopMotor(-1, 1.0, 1.0, 11.0));
    QVERIFY(!holdZoomShouldStopMotor(-1, 1.1, 1.0, 11.0));
    QVERIFY(!holdZoomShouldStopMotor(0, 11.0, 1.0, 11.0));
    QVERIFY(!holdZoomShouldStopMotor(1, qQNaN(), 1.0, 11.0));
}

void UnipodMt11ProtocolTest::testIdlePollRequestsCurrentZoom()
{
    using UnipodMt11Protocol::shouldRequestCurrentZoomOnPoll;

    // RC / SBUS zoom never starts a QGC hold; idle polls must still request 0x18.
    QVERIFY(shouldRequestCurrentZoomOnPoll(0, false));
    QVERIFY(shouldRequestCurrentZoomOnPoll(0, true));
    // ZR10 0x05 ACK does not stream the multiple while the motor runs — keep 0x18
    // going during overlay/joystick hold so the HUD can update.
    QVERIFY(shouldRequestCurrentZoomOnPoll(1, false));
    QVERIFY(shouldRequestCurrentZoomOnPoll(1, true));
    QVERIFY(shouldRequestCurrentZoomOnPoll(-1, false));
    QVERIFY(shouldRequestCurrentZoomOnPoll(-1, true));
}

void UnipodMt11ProtocolTest::testCurrentZoomRequestMatchesHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildCurrentZoomRequest(0).toHex(' ').toUpper(),
             QByteArray("55 66 01 00 00 00 00 18 7C 47"));
}

void UnipodMt11ProtocolTest::testParseZoomAcks()
{
    QByteArray zoomMultiple(2, char(0));
    zoomMultiple[0] = char(0x2D);  // 45 -> 4.5x
    zoomMultiple[1] = char(0x00);
    double zoom = 0.0;
    QVERIFY(UnipodMt11Protocol::parseZoomMultipleAck(zoomMultiple, &zoom));
    QCOMPARE(zoom, 4.5);

    QByteArray currentZoom;
    currentZoom.append(char(3));
    currentZoom.append(char(2));  // 3.2x
    QVERIFY(UnipodMt11Protocol::parseCurrentZoomAck(currentZoom, &zoom));
    QCOMPARE(zoom, 3.2);

    QByteArray uint16Style(2, char(0));
    uint16Style[0] = char(45);  // 45 + 16/10 is not a legal int+frac decimal
    uint16Style[1] = char(16);  // LE uint16 4141 → 414.1x
    QVERIFY(UnipodMt11Protocol::parseCurrentZoomAck(uint16Style, &zoom));
    QCOMPARE(zoom, 414.1);

    QVERIFY(!UnipodMt11Protocol::parseZoomMultipleAck(QByteArray(1, char(0)), &zoom));
    QVERIFY(!UnipodMt11Protocol::parseCurrentZoomAck(QByteArray(1, char(0)), &zoom));
}

void UnipodMt11ProtocolTest::testParseCapturedSiyiZoomAck()
{
    // ZR10 UDP ACKs captured on UniRC. A hand-copied CRC table dropped these frames.
    const QByteArray zoomAck = QByteArray::fromHex("55660202001b5a18050254a9");
    quint8 ctrl = 0;
    quint8 cmd = 0;
    quint16 seq = 0;
    QByteArray payload;
    QVERIFY(UnipodMt11Protocol::parseFrame(zoomAck, &ctrl, &seq, &cmd, &payload));
    QCOMPARE(cmd, quint8(0x18));
    QCOMPARE(payload.toHex(), QByteArray("0502"));
    double zoom = 0.0;
    QVERIFY(UnipodMt11Protocol::parseCurrentZoomAck(payload, &zoom));
    QCOMPARE(zoom, 5.2);

    const QByteArray sysAck = QByteArray::fromHex("5566020600bb180a0000000201011ab6");
    QVERIFY(UnipodMt11Protocol::parseFrame(sysAck, &ctrl, &seq, &cmd, &payload));
    QCOMPARE(cmd, quint8(0x0A));
}

void UnipodMt11ProtocolTest::testFocusFramesMatchHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildFocusCommand(0, 1).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 06 01 DE 31"));
    QCOMPARE(UnipodMt11Protocol::buildFocusCommand(0, 0).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 06 00 FF 21"));
    QCOMPARE(UnipodMt11Protocol::buildFocusCommand(0, -1).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 06 FF 0F 3F"));
}

void UnipodMt11ProtocolTest::testGimbalFramesMatchHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildGimbalSpeedCommand(0, 20, 0).toHex(' ').toUpper(),
             QByteArray("55 66 01 02 00 00 00 07 14 00 46 EB"));
    QCOMPARE(UnipodMt11Protocol::buildGimbalSpeedCommand(0, 0, 0).toHex(' ').toUpper(),
             QByteArray("55 66 01 02 00 00 00 07 00 00 F1 24"));
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 1).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 01 D1 12"));
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 2).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 02 B2 22"));
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 3).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 03 93 32"));
    QCOMPARE(UnipodMt11Protocol::buildCenterCommand(0, 4).toHex(' ').toUpper(),
             QByteArray("55 66 01 01 00 00 00 08 04 74 42"));
}

void UnipodMt11ProtocolTest::testGimbalAttitudeRequestMatchesHandbook()
{
    QCOMPARE(UnipodMt11Protocol::buildGimbalAttitudeRequest(0).toHex(' ').toUpper(),
             QByteArray("55 66 01 00 00 00 00 0D E8 05"));
}

void UnipodMt11ProtocolTest::testParseGimbalAttitudeAck()
{
    double pitch = 0.0;
    // yaw=0, pitch=-270 tenths (−27.0°), LE int16 0xFED2 at offset 2, roll=0, three velocities=0
    const QByteArray twelve = QByteArray::fromHex("0000d2fe0000000000000000");
    QVERIFY(UnipodMt11Protocol::parseGimbalAttitudeAck(twelve, &pitch));
    QCOMPARE(pitch, -27.0);

    const QByteArray four = QByteArray::fromHex("0000d2fe");
    QVERIFY(UnipodMt11Protocol::parseGimbalAttitudeAck(four, &pitch));
    QCOMPARE(pitch, -27.0);

    const QByteArray six = QByteArray::fromHex("0000d2fe0000");
    QVERIFY(UnipodMt11Protocol::parseGimbalAttitudeAck(six, &pitch));
    QCOMPARE(pitch, -27.0);

    QVERIFY(!UnipodMt11Protocol::parseGimbalAttitudeAck(QByteArray::fromHex("0000f4"), &pitch));
    QVERIFY(!UnipodMt11Protocol::parseGimbalAttitudeAck(QByteArray(), &pitch));
    QVERIFY(!UnipodMt11Protocol::parseGimbalAttitudeAck(twelve, nullptr));
}

UT_REGISTER_TEST(UnipodMt11ProtocolTest, TestLabel::Unit)
