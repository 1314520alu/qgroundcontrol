#pragma once

#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

class MotorComponentController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList motors READ motors NOTIFY layoutChanged)
    Q_PROPERTY(QString topologyName READ topologyName NOTIFY layoutChanged)
    Q_PROPERTY(bool spatialLayout READ spatialLayout NOTIFY layoutChanged)
    Q_PROPERTY(QString vehicleName READ vehicleName NOTIFY layoutChanged)
    Q_PROPERTY(int motorCount READ motorCount NOTIFY layoutChanged)
    Q_PROPERTY(int defaultThrottle READ defaultThrottle CONSTANT)
    Q_PROPERTY(int maxThrottle READ maxThrottle CONSTANT)
    Q_PROPERTY(int motorTimeoutSecs READ motorTimeoutSecs CONSTANT)

public:
    explicit MotorComponentController(QObject *parent = nullptr);

    QVariantList motors() const { return _motors; }
    QString topologyName() const { return _topologyName; }
    bool spatialLayout() const { return _spatialLayout; }
    QString vehicleName() const { return _vehicleName; }
    int motorCount() const { return _motorCount; }

    static constexpr int defaultThrottle() { return 10; }
    static constexpr int maxThrottle() { return 30; }
    static constexpr int motorTimeoutSecs() { return 5; }

    Q_INVOKABLE int clampThrottle(int percent) const;
    Q_INVOKABLE void refreshLayout();

signals:
    void layoutChanged();

private slots:
    void _rebuildLayout();

private:
    QVariantList _motors;
    QString _topologyName;
    QString _vehicleName;
    bool _spatialLayout = false;
    int _motorCount = 0;
};
