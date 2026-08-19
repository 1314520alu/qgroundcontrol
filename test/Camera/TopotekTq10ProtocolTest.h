#pragma once

#include "UnitTest.h"

class TopotekTq10ProtocolTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testCapFrameMatchesPython();
    void testRecToggleFrameMatchesPython();
    void testRecQueryFrameMatchesPython();
    void testZoomStopFrameMatchesPython();
    void testPtzStopFrameMatchesPython();
    void testPtzHomeFrameMatchesPython();
    void testCrcVerification();
    void testParseRecordState();
};
