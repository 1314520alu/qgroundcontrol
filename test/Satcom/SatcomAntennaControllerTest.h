#pragma once

#include "UnitTest.h"

class SatcomAntennaControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseTypicalFrame();
    void _parseCompactFrame();
    void _parseRejectsBadHeaderOrCount();
    void _commandBytes();
};
