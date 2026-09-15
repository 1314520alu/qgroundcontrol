#pragma once

#include "UnitTest.h"

class Fact;

class AppSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _preferredFirmwareClassEnumFiltered();
    void _offlineEditingFirmwareClassEnumFiltered();
    void _aircraftModelDefaultAndRoundTrip();
    void _updateManifestUrlDefaultEmpty();
    void _vehicleSetupVisibleComponentsDefault();
    void _vehicleSetupVisibleComponentsToggleAndReset();
    void _vehicleSetupResolveComponentId();
    void _vehicleSetupVisibleIndicesFiltersAndOrders();

private:
    void _verifyFirmwareClassEnumFiltered(Fact* fact);
};
