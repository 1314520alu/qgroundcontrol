#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace TopotekTq10Protocol
{

static constexpr quint16 kDefaultPort = 9003;
static constexpr quint16 kLocalPort = 9004;
static constexpr const char *kDefaultHost = "192.168.144.108";

/// Build #TP frame: network source P, fixed length field '2', ASCII data, 2-byte hex CRC.
QByteArray buildCommand(char dstAddr, char ctrl, const char *ident, const QByteArray &dataAscii);

QByteArray buildCap();
QByteArray buildRecToggle();
QByteArray buildRecQuery();
QByteArray buildZoom(const char *dataTwoChar);
QByteArray buildPtz(const char *dataTwoChar);

quint8 crc8Sum(const QByteArray &frameWithoutCrc);
bool verifyCrc(const QByteArray &frame);

/// Parse REC inquiry/feedback from ASCII datagram; returns true if recording state updated.
bool parseRecordState(const QByteArray &datagram, quint8 *recordStaOut);

} // namespace TopotekTq10Protocol
