#pragma once

#include <QtCore/QByteArray>

namespace UnipodMt11Protocol
{

static constexpr quint16 kDefaultPort = 37260;
static constexpr const char *kDefaultHost = "192.168.144.25";

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

quint16 crc16(const QByteArray &data);
QByteArray buildFrame(quint8 ctrl, quint16 seq, quint8 cmdId, const QByteArray &payload);
bool parseFrame(const QByteArray &datagram, quint8 *ctrlOut, quint16 *seqOut, quint8 *cmdOut, QByteArray *payloadOut);
QByteArray buildPhotoCommand(quint16 seq);
QByteArray buildRecordToggleCommand(quint16 seq);
QByteArray buildSystemInfoRequest(quint16 seq);
bool parseSystemInfoAck(const QByteArray &payload, SystemInfoAck *out);
bool parseFuncFeedback(const QByteArray &payload, quint8 *infoTypeOut);

} // namespace UnipodMt11Protocol
