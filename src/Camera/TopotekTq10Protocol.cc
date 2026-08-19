#include "TopotekTq10Protocol.h"

namespace TopotekTq10Protocol
{

static QByteArray _appendCrc(QByteArray frame)
{
    const quint8 crc = crc8Sum(frame);
    frame.append(QByteArray::number(crc, 16).rightJustified(2, '0').toUpper());
    return frame;
}

quint8 crc8Sum(const QByteArray &frameWithoutCrc)
{
    quint32 sum = 0;
    for (const char byte : frameWithoutCrc) {
        sum += static_cast<quint8>(byte);
    }
    return static_cast<quint8>(sum & 0xFF);
}

bool verifyCrc(const QByteArray &frame)
{
    if (frame.size() < 4) {
        return false;
    }

    const QByteArray body = frame.left(frame.size() - 2);
    const QByteArray crcAscii = frame.right(2);
    bool ok = false;
    const quint8 expected = static_cast<quint8>(crcAscii.toUInt(&ok, 16));
    return ok && (crc8Sum(body) == expected);
}

QByteArray buildCommand(char dstAddr, char ctrl, const char *ident, const QByteArray &dataAscii)
{
    QByteArray frame;
    frame.reserve(16 + dataAscii.size());
    frame.append("#TP");
    frame.append('P');
    frame.append(dstAddr);
    frame.append('2');
    frame.append(ctrl);
    frame.append(ident, 3);
    frame.append(dataAscii);
    return _appendCrc(frame);
}

QByteArray buildCap()
{
    return buildCommand('D', 'w', "CAP", QByteArray("0"));
}

QByteArray buildRecToggle()
{
    return buildCommand('D', 'w', "REC", QByteArray("0A"));
}

QByteArray buildRecQuery()
{
    return buildCommand('D', 'r', "REC", QByteArray("0"));
}

QByteArray buildZoom(const char *dataTwoChar)
{
    return buildCommand('M', 'w', "ZMC", QByteArray(dataTwoChar, 2));
}

QByteArray buildPtz(const char *dataTwoChar)
{
    return buildCommand('G', 'w', "PTZ", QByteArray(dataTwoChar, 2));
}

bool parseRecordState(const QByteArray &datagram, quint8 *recordStaOut)
{
    if (!recordStaOut || datagram.isEmpty()) {
        return false;
    }

    const QString text = QString::fromLatin1(datagram);
    const int recIdx = text.indexOf(QStringLiteral("REC"));
    if (recIdx < 0 || (recIdx + 6) > text.size()) {
        return false;
    }

    const int dataStart = recIdx + 3;
    if (text.size() <= dataStart) {
        return false;
    }

    QChar stateChar = text.at(dataStart);
    if ((text.size() > dataStart + 1) && text.at(dataStart + 1).isDigit()) {
        stateChar = text.at(dataStart + 1);
    }

    if (stateChar == QLatin1Char('0')) {
        *recordStaOut = 0;
        return true;
    }
    if (stateChar == QLatin1Char('1')) {
        *recordStaOut = 1;
        return true;
    }

    return false;
}

} // namespace TopotekTq10Protocol
