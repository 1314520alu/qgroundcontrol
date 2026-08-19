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
    void testParseSystemInfoAck();
    void testParseFuncFeedback();
};
