#pragma once

#include <QtCore/QByteArray>

namespace UnipodMt11Protocol {

static constexpr quint16 kDefaultPort = 37260;
static constexpr const char* kDefaultHost = "192.168.144.25";

enum class FuncFeedback : quint8
{
    PhotoOk = 0,
    PhotoFailNoCard = 1,
    PhotoFail = 2,
    HdrOn = 3,
    HdrOff = 4,
    RecordStart = 5,
    RecordEnd = 6,
};

struct SystemInfoAck
{
    quint8 recordSta = 0;
};

quint16 crc16(const QByteArray& data);
QByteArray buildFrame(quint8 ctrl, quint16 seq, quint8 cmdId, const QByteArray& payload);
bool parseFrame(const QByteArray& datagram, quint8* ctrlOut, quint16* seqOut, quint8* cmdOut, QByteArray* payloadOut);
QByteArray buildPhotoCommand(quint16 seq);
QByteArray buildRecordToggleCommand(quint16 seq);
QByteArray buildSystemInfoRequest(quint16 seq);
/// Continuous zoom: +1 in, -1 out, 0 stop (CMD 0x05).
QByteArray buildZoomCommand(quint16 seq, qint8 zoom);
/// Manual focus: +1 far, -1 near, 0 stop (CMD 0x06).
QByteArray buildFocusCommand(quint16 seq, qint8 focus);
/// Gimbal speed yaw/pitch in -100..100 (CMD 0x07).
QByteArray buildGimbalSpeedCommand(quint16 seq, qint8 yaw, qint8 pitch);
/// Center modes: 1=center, 2=center+down, 3=level, 4=down (CMD 0x08).
QByteArray buildCenterCommand(quint16 seq, quint8 mode = 1);
/// Set main/secondary stream layout (CMD 0x11).
QByteArray buildSetVideoLayoutCommand(quint16 seq, quint8 mainMode, quint8 secondaryMode);
/// Laser on/off (CMD 0x32).
QByteArray buildSetLaserStateCommand(quint16 seq, bool enabled);
/// Request one laser ranging sample (CMD 0x15).
QByteArray buildLaserDistanceRequest(quint16 seq);
/// AI track mode on/off (CMD 0x55).
QByteArray buildSetAiTrackModeCommand(quint16 seq, bool enabled);
/// Parse CMD 0x15 ack distance in decimeters; returns false if invalid.
bool parseLaserDistanceAck(const QByteArray& payload, quint16* distanceDmOut);
bool parseSystemInfoAck(const QByteArray& payload, SystemInfoAck* out);
bool parseFuncFeedback(const QByteArray& payload, quint8* infoTypeOut);

}  // namespace UnipodMt11Protocol
