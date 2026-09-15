#pragma once

#include "UnitTest.h"

class CameraPayloadCapabilitiesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSimulatedDefaultsFalse();
    void testCatalogPhase1Sets();
    void testTopotekHasGimbalPad();
    void testUnipodPhase1Flags();
    void testA8Phase1Flags();
    void testZr10CatalogPhase1();
    void testUnipodMediaLibraryFollowsReady();
};
