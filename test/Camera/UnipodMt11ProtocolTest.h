#pragma once

#include "UnitTest.h"

class UnipodMt11ProtocolTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testPhotoFrameMatchesHandbook();
    void testRecordFrameMatchesHandbook();
    void testSystemInfoRequestMatchesHandbook();
    void testParseRoundTrip();
    void testParseFrameAcceptsTrailingPadding();
    void testParseFrameThenRemainder();
    void testParseSystemInfoAck();
    void testParseFuncFeedback();
    void testZoomFramesMatchHandbook();
    void testAbsoluteZoomFramesMatchHandbook();
    void testZoomRangeRequestMatchesHandbook();
    void testCurrentZoomRequestMatchesHandbook();
    void testParseZoomAcks();
    void testParseCapturedSiyiZoomAck();
    void testNextHoldZoomCommand();
    void testHoldZoomShouldStopMotor();
    void testIdlePollRequestsCurrentZoom();
    void testFocusFramesMatchHandbook();
    void testGimbalFramesMatchHandbook();
    void testGimbalAttitudeRequestMatchesHandbook();
    void testParseGimbalAttitudeAck();
};
