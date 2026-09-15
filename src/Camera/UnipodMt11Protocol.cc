#include "UnipodMt11Protocol.h"

#include <QtCore/QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace UnipodMt11Protocol {

namespace {

static constexpr quint16 kStx = 0x6655;
static constexpr int kHeaderSize = 8;  // STX(2) + CTRL(1) + LEN(2) + SEQ(2) + CMD(1)
static constexpr int kCrcSize = 2;
static constexpr int kMinFrameSize = kHeaderSize + kCrcSize;
static constexpr int kRecordStaOffset = 3;

}  // namespace

// CRC16-CCITT/XMODEM (poly 0x1021, init 0). Computed, not copied from the SDK PDF table
// (that table had OCR typos and dropped live ZR10 ACKs).
quint16 crc16(const QByteArray& data)
{
    quint16 crc = 0;
    for (const char ch : data) {
        crc ^= static_cast<quint16>(static_cast<quint8>(ch)) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000U) {
                crc = static_cast<quint16>((crc << 1) ^ 0x1021U);
            } else {
                crc = static_cast<quint16>(crc << 1);
            }
        }
    }
    return crc;
}

QByteArray buildFrame(quint8 ctrl, quint16 seq, quint8 cmdId, const QByteArray& payload)
{
    const quint16 dataLen = static_cast<quint16>(payload.size());

    QByteArray frame;
    frame.reserve(kHeaderSize + payload.size() + kCrcSize);

    const quint16 stxLe = qToLittleEndian(kStx);
    frame.append(reinterpret_cast<const char*>(&stxLe), sizeof(stxLe));
    frame.append(static_cast<char>(ctrl));

    const quint16 dataLenLe = qToLittleEndian(dataLen);
    frame.append(reinterpret_cast<const char*>(&dataLenLe), sizeof(dataLenLe));

    const quint16 seqLe = qToLittleEndian(seq);
    frame.append(reinterpret_cast<const char*>(&seqLe), sizeof(seqLe));

    frame.append(static_cast<char>(cmdId));
    frame.append(payload);

    const quint16 crc = crc16(frame);
    const quint16 crcLe = qToLittleEndian(crc);
    frame.append(reinterpret_cast<const char*>(&crcLe), sizeof(crcLe));

    return frame;
}

bool parseFrame(const QByteArray& datagram, quint8* ctrlOut, quint16* seqOut, quint8* cmdOut, QByteArray* payloadOut)
{
    if (!ctrlOut || !seqOut || !cmdOut || !payloadOut) {
        return false;
    }

    if (datagram.size() < kMinFrameSize) {
        return false;
    }

    quint16 stx = 0;
    memcpy(&stx, datagram.constData(), sizeof(stx));
    if (qFromLittleEndian(stx) != kStx) {
        return false;
    }

    const int payloadLen = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(datagram.constData() + 3));
    const int expectedSize = kHeaderSize + payloadLen + kCrcSize;
    if (datagram.size() < expectedSize) {
        return false;
    }

    const QByteArray frame = datagram.left(expectedSize);
    const QByteArray frameWithoutCrc = frame.left(frame.size() - kCrcSize);
    quint16 receivedCrc = 0;
    memcpy(&receivedCrc, frame.constData() + frame.size() - kCrcSize, sizeof(receivedCrc));
    receivedCrc = qFromLittleEndian(receivedCrc);

    if (crc16(frameWithoutCrc) != receivedCrc) {
        return false;
    }

    *ctrlOut = static_cast<quint8>(frame.at(2));
    *seqOut = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(frame.constData() + 5));
    *cmdOut = static_cast<quint8>(frame.at(7));
    *payloadOut = frame.mid(kHeaderSize, payloadLen);
    return true;
}

QByteArray buildPhotoCommand(quint16 seq)
{
    return buildFrame(0x01, seq, 0x0C, QByteArray(1, char(0x00)));
}

QByteArray buildRecordToggleCommand(quint16 seq)
{
    return buildFrame(0x01, seq, 0x0C, QByteArray(1, char(0x02)));
}

QByteArray buildSystemInfoRequest(quint16 seq)
{
    return buildFrame(0x01, seq, 0x0A, QByteArray());
}

QByteArray buildZoomCommand(quint16 seq, qint8 zoom)
{
    return buildFrame(0x01, seq, 0x05, QByteArray(1, static_cast<char>(zoom)));
}

QByteArray buildAbsoluteZoomCommand(quint16 seq, double zoom)
{
    if (!std::isfinite(zoom)) {
        zoom = kHoldZoomMin;
    }
    const double tenths = std::round(std::clamp(zoom, kHoldZoomMin, 255.9) * 10.0);
    const int zoomInt = static_cast<int>(tenths) / 10;
    const int zoomFrac = static_cast<int>(tenths) % 10;
    QByteArray payload(2, char(0));
    payload[0] = static_cast<char>(zoomInt);
    payload[1] = static_cast<char>(zoomFrac);
    return buildFrame(0x01, seq, 0x0F, payload);
}

QByteArray buildZoomRangeRequest(quint16 seq)
{
    return buildFrame(0x01, seq, 0x16, QByteArray());
}

QByteArray buildCurrentZoomRequest(quint16 seq)
{
    return buildFrame(0x01, seq, 0x18, QByteArray());
}

QByteArray buildGimbalAttitudeRequest(quint16 seq)
{
    return buildFrame(0x01, seq, 0x0D, QByteArray());
}

bool parseGimbalAttitudeAck(const QByteArray& payload, double* pitchDegOut)
{
    if (!pitchDegOut || payload.size() < 4) {
        return false;
    }

    qint16 pitchRaw = 0;
    memcpy(&pitchRaw, payload.constData() + 2, sizeof(pitchRaw));
    *pitchDegOut = static_cast<double>(qFromLittleEndian(pitchRaw)) / 10.0;
    return true;
}

double nextHoldZoomCommand(double startZoom, double elapsedSec, double actual, int actualAgeMs, int direction,
                           double ratioPerSec, double maxLeadRatio, double minZoom, double maxZoom)
{
    if (direction == 0 || !(startZoom > 0.0) || !(elapsedSec >= 0.0) || !(ratioPerSec > 1.0)) {
        return startZoom;
    }

    const double factor = std::pow(ratioPerSec, elapsedSec);
    double target = (direction > 0) ? (startZoom * factor) : (startZoom / factor);

    const bool actualFresh =
        std::isfinite(actual) && actual > 0.0 && actualAgeMs >= 0 && actualAgeMs <= kHoldZoomActualFreshMs;
    if (actualFresh && maxLeadRatio > 1.0) {
        if (direction > 0) {
            target = std::min(target, actual * maxLeadRatio);
        } else {
            target = std::max(target, actual / maxLeadRatio);
        }
    }

    if (std::isfinite(minZoom) && std::isfinite(maxZoom) && maxZoom >= minZoom) {
        target = std::clamp(target, minZoom, maxZoom);
    }

    return std::round(target * 10.0) / 10.0;
}

bool holdZoomShouldStopMotor(int direction, double zoom, double minZoom, double maxZoom)
{
    if (direction == 0 || !std::isfinite(zoom)) {
        return false;
    }
    if (direction > 0) {
        return std::isfinite(maxZoom) && zoom >= maxZoom;
    }
    return std::isfinite(minZoom) && zoom <= minZoom;
}

bool shouldRequestCurrentZoomOnPoll(int holdDirection, bool holdMotorStopped)
{
    Q_UNUSED(holdDirection);
    Q_UNUSED(holdMotorStopped);
    // ZR10 0x05 ACK does not stream the multiple while the motor runs. Idle covers
    // RC/SBUS zoom; overlay hold also needs 0x18 so the HUD can keep up.
    return true;
}

bool parseZoomMultipleAck(const QByteArray& payload, double* zoomOut)
{
    if (!zoomOut || payload.size() < 2) {
        return false;
    }

    quint16 multiple = 0;
    memcpy(&multiple, payload.constData(), sizeof(multiple));
    *zoomOut = static_cast<double>(qFromLittleEndian(multiple)) / 10.0;
    return true;
}

bool parseCurrentZoomAck(const QByteArray& payload, double* zoomOut)
{
    if (!zoomOut || payload.size() < 2) {
        return false;
    }

    const auto zoomInt = static_cast<quint8>(payload.at(0));
    const auto zoomFrac = static_cast<quint8>(payload.at(1));
    if (zoomFrac > 9) {
        // Some ZR10 builds ACK 0x18 with the 0x05 uint16 / 10 layout.
        return parseZoomMultipleAck(payload, zoomOut);
    }
    *zoomOut = static_cast<double>(zoomInt) + (static_cast<double>(zoomFrac) / 10.0);
    return true;
}

QByteArray buildFocusCommand(quint16 seq, qint8 focus)
{
    return buildFrame(0x01, seq, 0x06, QByteArray(1, static_cast<char>(focus)));
}

QByteArray buildGimbalSpeedCommand(quint16 seq, qint8 yaw, qint8 pitch)
{
    QByteArray payload(2, char(0));
    payload[0] = static_cast<char>(yaw);
    payload[1] = static_cast<char>(pitch);
    return buildFrame(0x01, seq, 0x07, payload);
}

QByteArray buildCenterCommand(quint16 seq, quint8 mode)
{
    return buildFrame(0x01, seq, 0x08, QByteArray(1, static_cast<char>(mode)));
}

QByteArray buildSetVideoLayoutCommand(quint16 seq, quint8 mainMode, quint8 secondaryMode)
{
    QByteArray payload(2, char(0));
    payload[0] = static_cast<char>(mainMode);
    payload[1] = static_cast<char>(secondaryMode);
    return buildFrame(0x01, seq, 0x11, payload);
}

QByteArray buildSetLaserStateCommand(quint16 seq, bool enabled)
{
    return buildFrame(0x01, seq, 0x32, QByteArray(1, enabled ? char(0x01) : char(0x00)));
}

QByteArray buildLaserDistanceRequest(quint16 seq)
{
    return buildFrame(0x01, seq, 0x15, QByteArray());
}

QByteArray buildSetAiTrackModeCommand(quint16 seq, bool enabled)
{
    return buildFrame(0x01, seq, 0x55, QByteArray(1, enabled ? char(0x01) : char(0x00)));
}

bool parseLaserDistanceAck(const QByteArray& payload, quint16* distanceDmOut)
{
    if (!distanceDmOut || payload.size() < 2) {
        return false;
    }
    quint16 dm = 0;
    memcpy(&dm, payload.constData(), sizeof(dm));
    *distanceDmOut = qFromLittleEndian(dm);
    return true;
}

bool parseSystemInfoAck(const QByteArray& payload, SystemInfoAck* out)
{
    if (!out || payload.size() <= kRecordStaOffset) {
        return false;
    }

    out->recordSta = static_cast<quint8>(payload.at(kRecordStaOffset));
    return true;
}

bool parseFuncFeedback(const QByteArray& payload, quint8* infoTypeOut)
{
    if (!infoTypeOut || payload.isEmpty()) {
        return false;
    }

    *infoTypeOut = static_cast<quint8>(payload.at(0));
    return true;
}

}  // namespace UnipodMt11Protocol
