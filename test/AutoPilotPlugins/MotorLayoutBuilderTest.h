#pragma once

#include "UnitTest.h"

class MotorLayoutBuilderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testQuadX();
    void _testQuadPlus();
    void _testHexaX();
    void _testOctoX();
    void _testOctoQuadX();
    void _testY6B();
    void _testDodecaHexaX();
    void _testUnknownFallback();
};
