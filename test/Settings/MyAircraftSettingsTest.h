#pragma once

#include "UnitTest.h"

class MyAircraftSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _enableToggle();
};
