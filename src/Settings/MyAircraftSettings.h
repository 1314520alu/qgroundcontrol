#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// User aircraft profiles. Selecting Weitong aircraft enables satcom antenna TCP control.
class MyAircraftSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    MyAircraftSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(weitongAircraftEnabled)
    DEFINE_SETTINGFACT(satcomHost)
    DEFINE_SETTINGFACT(satcomPort)
};
