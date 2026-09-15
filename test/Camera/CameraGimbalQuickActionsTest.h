#pragma once

#include "UnitTest.h"

class CameraGimbalQuickActionsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSimulatedDefaultsFalse();
    void testTopotekOnlyRecenter();
    void testUnipodHasAllFour();
    void testA8HasAllFour();
};
