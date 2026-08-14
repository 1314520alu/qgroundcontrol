#include "EscStatusFactGroupListModel.h"
#include "MAVLinkLib.h"
#include "QGCMAVLink.h"

namespace {
bool _escTelemetryFirstIndex(uint32_t msgid, uint32_t &firstIndex)
{
    switch (msgid) {
    case MAVLINK_MSG_ID_ESC_TELEMETRY_1_TO_4:
        firstIndex = 0;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_5_TO_8:
        firstIndex = 4;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_9_TO_12:
        firstIndex = 8;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_13_TO_16:
        firstIndex = 12;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_17_TO_20:
        firstIndex = 16;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_21_TO_24:
        firstIndex = 20;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_25_TO_28:
        firstIndex = 24;
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_29_TO_32:
        firstIndex = 28;
        return true;
    default:
        return false;
    }
}

bool _decodeEscTelemetry(const mavlink_message_t &message, mavlink_esc_telemetry_1_to_4_t &telem)
{
    // All ESC_TELEMETRY_*_TO_* payloads share the same layout.
    switch (message.msgid) {
    case MAVLINK_MSG_ID_ESC_TELEMETRY_1_TO_4:
        mavlink_msg_esc_telemetry_1_to_4_decode(&message, &telem);
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_5_TO_8:
        mavlink_msg_esc_telemetry_5_to_8_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_5_to_8_t *>(&telem));
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_9_TO_12:
        mavlink_msg_esc_telemetry_9_to_12_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_9_to_12_t *>(&telem));
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_13_TO_16:
        mavlink_msg_esc_telemetry_13_to_16_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_13_to_16_t *>(&telem));
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_17_TO_20:
        mavlink_msg_esc_telemetry_17_to_20_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_17_to_20_t *>(&telem));
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_21_TO_24:
        mavlink_msg_esc_telemetry_21_to_24_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_21_to_24_t *>(&telem));
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_25_TO_28:
        mavlink_msg_esc_telemetry_25_to_28_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_25_to_28_t *>(&telem));
        return true;
    case MAVLINK_MSG_ID_ESC_TELEMETRY_29_TO_32:
        mavlink_msg_esc_telemetry_29_to_32_decode(
            &message, reinterpret_cast<mavlink_esc_telemetry_29_to_32_t *>(&telem));
        return true;
    default:
        return false;
    }
}
} // namespace

EscStatusFactGroupListModel::EscStatusFactGroupListModel(QObject* parent)
    : FactGroupListModel("escStatus", parent)
{

}

bool EscStatusFactGroupListModel::_shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const
{
    bool shouldHandle = false;
    uint32_t firstIndex = 0;

    ids.clear();

    switch (message.msgid) {
    case MAVLINK_MSG_ID_ESC_INFO:
    {
        mavlink_esc_info_t escInfo{};
        mavlink_msg_esc_info_decode(&message, &escInfo);
        firstIndex = escInfo.index;
        shouldHandle = true;
    }
        break;
    case MAVLINK_MSG_ID_ESC_STATUS:
    {
        mavlink_esc_status_t escStatus{};
        mavlink_msg_esc_status_decode(&message, &escStatus);
        firstIndex = escStatus.index;
        shouldHandle = true;
    }
        break;
    default:
        if (_escTelemetryFirstIndex(message.msgid, firstIndex)) {
            shouldHandle = true;
        }
        break;
    }

    if (shouldHandle) {
        for (uint32_t index = firstIndex; index <= firstIndex + 3; index++) {
            ids.append(index);
        }
    }

    return shouldHandle;
}

FactGroupWithId *EscStatusFactGroupListModel::_createFactGroupWithId(uint32_t id)
{
    return new EscStatusFactGroup(id, this);
}

EscStatusFactGroup::EscStatusFactGroup(uint32_t escIndex, QObject *parent)
    : FactGroupWithId(1000, QStringLiteral(":/json/Vehicle/EscStatusFactGroup.json"), parent)
{
    _addFact(&_rpmFact);
    _addFact(&_currentFact);
    _addFact(&_voltageFact);
    _addFact(&_countFact);
    _addFact(&_connectionTypeFact);
    _addFact(&_infoFact);
    _addFact(&_failureFlagsFact);
    _addFact(&_errorCountFact);
    _addFact(&_temperatureFact);

    _idFact.setRawValue(escIndex);
    _rpmFact.setRawValue(0);
    _currentFact.setRawValue(0);
    _voltageFact.setRawValue(0);
    _countFact.setRawValue(0);
    _connectionTypeFact.setRawValue(0);
    _infoFact.setRawValue(0);
    _failureFlagsFact.setRawValue(0);
    _errorCountFact.setRawValue(0);
    _temperatureFact.setRawValue(0);
}

void EscStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_ESC_INFO:
        _handleEscInfo(vehicle, message);
        break;
    case MAVLINK_MSG_ID_ESC_STATUS:
        _handleEscStatus(vehicle, message);
        break;
    default: {
        uint32_t firstIndex = 0;
        if (_escTelemetryFirstIndex(message.msgid, firstIndex)) {
            _handleEscTelemetry(vehicle, message);
        }
        break;
    }
    }
}

void EscStatusFactGroup::_handleEscInfo(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_esc_info_t escInfo{};
    mavlink_msg_esc_info_decode(&message, &escInfo);

    uint8_t index = _idFact.rawValue().toUInt();

    if (index < escInfo.index || index >= escInfo.index + 4) {
        // Disregard ESC info messages which are not targeted at this ESC index
        return;
    }

    index %= 4; // Convert to 0-based index for the arrays in escInfo
    _countFact.setRawValue(escInfo.count);
    _connectionTypeFact.setRawValue(escInfo.connection_type);
    _infoFact.setRawValue(escInfo.info);
    _failureFlagsFact.setRawValue(escInfo.failure_flags[index]);
    _errorCountFact.setRawValue(escInfo.error_count[index]);
    _temperatureFact.setRawValue(escInfo.temperature[index]);

    _setTelemetryAvailable(true);
}

void EscStatusFactGroup::_handleEscStatus(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_esc_status_t escStatus{};
    mavlink_msg_esc_status_decode(&message, &escStatus);

    uint8_t index = _idFact.rawValue().toUInt();

    if (index < escStatus.index || index >= escStatus.index + 4) {
        // Disregard ESC status messages which are not targeted at this ESC index
        return;
    }

    index %= 4; // Convert to 0-based index for the arrays in escStatus
    _rpmFact.setRawValue(escStatus.rpm[index]);
    _currentFact.setRawValue(escStatus.current[index]);
    _voltageFact.setRawValue(escStatus.voltage[index]);

    _setTelemetryAvailable(true);
}

void EscStatusFactGroup::_handleEscTelemetry(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    uint32_t firstIndex = 0;
    if (!_escTelemetryFirstIndex(message.msgid, firstIndex)) {
        return;
    }

    const uint32_t escId = _idFact.rawValue().toUInt();
    if (escId < firstIndex || escId >= firstIndex + 4) {
        return;
    }

    mavlink_esc_telemetry_1_to_4_t telem{};
    if (!_decodeEscTelemetry(message, telem)) {
        return;
    }

    const uint32_t slot = escId - firstIndex;

    // Mark online slots from ArduPilot packet counters / live values.
    uint8_t onlineMask = 0;
    for (uint32_t j = 0; j < 4; j++) {
        if (telem.count[j] > 0 || telem.rpm[j] > 0 || telem.voltage[j] > 0 || telem.current[j] > 0 || telem.temperature[j] > 0) {
            onlineMask |= static_cast<uint8_t>(1u << j);
        }
    }

    // Keep Fact units aligned with ESC_INFO/ESC_STATUS UI expectations:
    // voltage/current in V/A, temperature in centi-degrees C.
    _rpmFact.setRawValue(telem.rpm[slot]);
    _voltageFact.setRawValue(telem.voltage[slot] / 100.0f);
    _currentFact.setRawValue(telem.current[slot] / 100.0f);
    _temperatureFact.setRawValue(static_cast<float>(telem.temperature[slot]) * 100.0f);
    _infoFact.setRawValue(onlineMask);
    _countFact.setRawValue(static_cast<int>(firstIndex + 4));
    // ArduPilot packet counter is not an error count; leave failureFlags at 0.
    _errorCountFact.setRawValue(0);
    _failureFlagsFact.setRawValue(0);

    _setTelemetryAvailable(true);
}
