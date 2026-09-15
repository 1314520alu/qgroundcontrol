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
/// Continuous zoom: +1 in / 0 stop / -1 out (CMD 0x05). Hold zoom uses 0x0F.
QByteArray buildZoomCommand(quint16 seq, qint8 zoom);
/// Absolute zoom integer + one decimal digit (CMD 0x0F).
QByteArray buildAbsoluteZoomCommand(quint16 seq, double zoom);
/// Request max zoom range (CMD 0x16).
QByteArray buildZoomRangeRequest(quint16 seq);
/// Request current zoom multiple (CMD 0x18).
QByteArray buildCurrentZoomRequest(quint16 seq);
/// Parse CMD 0x05 ACK: zoom_multiple / 10 (one decimal).
bool parseZoomMultipleAck(const QByteArray& payload, double* zoomOut);
/// Parse CMD 0x18 / 0x16 ACK: zoom_int + zoom_float / 10.
bool parseCurrentZoomAck(const QByteArray& payload, double* zoomOut);
/// Request gimbal attitude (CMD 0x0D). Empty payload.
QByteArray buildGimbalAttitudeRequest(quint16 seq);
/// Parse CMD 0x0D ACK: little-endian int16 at offset 2, / 10.0 = pitch degrees.
/// Needs at least 4 bytes (yaw + pitch). Velocities optional.
bool parseGimbalAttitudeAck(const QByteArray& payload, double* pitchDegOut);

static constexpr double kHoldZoomMin = 1.0;
static constexpr double kHoldZoomMaxDefault = 11.0;
static constexpr double kHoldZoomRatioPerSec = 1.4;
static constexpr double kHoldZoomMaxLeadRatio = 1.25;
static constexpr int kHoldZoomActualFreshMs = 400;
static constexpr int kCurrentZoomPollIntervalMs = 150;
static constexpr int kCurrentZoomHoldPollIntervalMs = 80;

/// Next 0x0F target while a zoom button is held. Log-space rate so FOV change stays even;
/// a fresh `actual` reading caps how far the command may lead the lens.
double nextHoldZoomCommand(double startZoom, double elapsedSec, double actual, int actualAgeMs, int direction,
                           double ratioPerSec, double maxLeadRatio, double minZoom, double maxZoom);
/// CMD 0x05 hold should stop at optical min/max so digital zoom does not sprint.
bool holdZoomShouldStopMotor(int direction, double zoom, double minZoom, double maxZoom);
/// True when the zoom timer should send CMD 0x18 (idle RC/SBUS and overlay hold).
bool shouldRequestCurrentZoomOnPoll(int holdDirection, bool holdMotorStopped);
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
