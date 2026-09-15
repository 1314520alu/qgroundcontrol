#pragma once

#include "UnitTest.h"

class VideoSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _effectiveRtpJitterLatencyMs_data();
    void _effectiveRtpJitterLatencyMs();
    void _rtpJitterLatencyMsBelowMinRejected();
    void _rtpJitterLatencyMsAboveMinAccepted();
};
