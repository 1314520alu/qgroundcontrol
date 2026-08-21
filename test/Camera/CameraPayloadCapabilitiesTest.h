#pragma once

#include "UnitTest.h"

class CameraPayloadCapabilitiesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSimulatedDefaultsFalse();
    void testTopotekHasGimbalPad();
    void testUnipodMediaLibraryFollowsReady();
};
