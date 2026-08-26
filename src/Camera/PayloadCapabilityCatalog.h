#pragma once

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

/// Runtime lookup of UniGCS overlay capabilities from PayloadCapabilities.json.
class PayloadCapabilityCatalog
{
public:
    struct Features
    {
        bool gimbal = false;
        bool lens = false;
        bool laser = false;
        bool ai = false;
        bool follow = false;
        bool exposureAuto = false;
        bool photo = false;
        bool video = false;
        bool zoom = false;
        bool focus = false;
        bool mediaLibrary = false;
    };

    struct Entry
    {
        QString id;
        QString vendor;
        QString modelName;
        QString videoSource;
        Features features;
        QStringList overlayPhase1;
    };

    static PayloadCapabilityCatalog& instance();

    bool isLoaded() const { return _loaded; }

    bool load(const QString& resourcePath = QStringLiteral(":/json/PayloadCapabilities.json"));

    const Entry* byVideoSource(const QString& videoSource) const;
    const Entry* byModelName(const QString& modelName) const;

    /// True when feature is listed in overlay_phase1 for this camera.
    static bool overlayHas(const Entry* entry, const QString& feature);

private:
    PayloadCapabilityCatalog() = default;

    bool _loaded = false;
    QHash<QString, Entry> _byVideoSource;
    QHash<QString, Entry> _byModelName;
};
