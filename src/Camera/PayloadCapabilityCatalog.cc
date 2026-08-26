#include "PayloadCapabilityCatalog.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PayloadCapabilityCatalogLog, "Camera.PayloadCapabilityCatalog")

namespace {

bool featureTruthy(const QJsonValue& value)
{
    if (value.isBool()) {
        return value.toBool();
    }
    if (value.isDouble()) {
        return value.toInt() != 0;
    }
    if (value.isString()) {
        const QString s = value.toString().trimmed().toLower();
        return (s == QLatin1String("1") || s == QLatin1String("true") || s == QLatin1String("yes"));
    }
    return false;
}

PayloadCapabilityCatalog::Features parseFeatures(const QJsonObject& obj)
{
    PayloadCapabilityCatalog::Features f;
    f.gimbal = featureTruthy(obj.value(QStringLiteral("gimbal")));
    f.lens = featureTruthy(obj.value(QStringLiteral("lens")));
    f.laser = featureTruthy(obj.value(QStringLiteral("laser")));
    // "optional" stays false until a future detector enables it.
    f.ai = featureTruthy(obj.value(QStringLiteral("ai")));
    f.follow = featureTruthy(obj.value(QStringLiteral("follow")));
    f.exposureAuto = featureTruthy(obj.value(QStringLiteral("exposure_auto")));
    f.photo = featureTruthy(obj.value(QStringLiteral("photo")));
    f.video = featureTruthy(obj.value(QStringLiteral("video")));
    f.zoom = featureTruthy(obj.value(QStringLiteral("zoom")));
    f.focus = featureTruthy(obj.value(QStringLiteral("focus")));
    f.mediaLibrary = featureTruthy(obj.value(QStringLiteral("media_library")));
    return f;
}

}  // namespace

PayloadCapabilityCatalog& PayloadCapabilityCatalog::instance()
{
    static PayloadCapabilityCatalog catalog;
    if (!catalog._loaded) {
        (void) catalog.load();
    }
    return catalog;
}

bool PayloadCapabilityCatalog::load(const QString& resourcePath)
{
    _byVideoSource.clear();
    _byModelName.clear();
    _loaded = false;

    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(PayloadCapabilityCatalogLog) << "Failed to open" << resourcePath << file.errorString();
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qCWarning(PayloadCapabilityCatalogLog) << "Invalid PayloadCapabilities JSON root";
        return false;
    }

    const QJsonArray cameras = doc.object().value(QStringLiteral("cameras")).toArray();
    for (const QJsonValue& value : cameras) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        Entry entry;
        entry.id = obj.value(QStringLiteral("id")).toString();
        entry.vendor = obj.value(QStringLiteral("vendor")).toString();
        entry.modelName = obj.value(QStringLiteral("model_name")).toString();
        entry.videoSource = obj.value(QStringLiteral("video_source")).toString();
        entry.features = parseFeatures(obj.value(QStringLiteral("features")).toObject());

        const QJsonArray phase1 = obj.value(QStringLiteral("overlay_phase1")).toArray();
        for (const QJsonValue& feature : phase1) {
            entry.overlayPhase1.append(feature.toString());
        }

        if (entry.videoSource.isEmpty() || entry.modelName.isEmpty()) {
            qCWarning(PayloadCapabilityCatalogLog) << "Skipping camera entry missing video_source/model_name";
            continue;
        }

        _byVideoSource.insert(entry.videoSource, entry);
        _byModelName.insert(entry.modelName, entry);
    }

    _loaded = !_byVideoSource.isEmpty();
    qCDebug(PayloadCapabilityCatalogLog) << "Loaded" << _byVideoSource.size() << "payload cameras from" << resourcePath;
    return _loaded;
}

const PayloadCapabilityCatalog::Entry* PayloadCapabilityCatalog::byVideoSource(const QString& videoSource) const
{
    const auto it = _byVideoSource.constFind(videoSource);
    return (it == _byVideoSource.cend()) ? nullptr : &(*it);
}

const PayloadCapabilityCatalog::Entry* PayloadCapabilityCatalog::byModelName(const QString& modelName) const
{
    const auto it = _byModelName.constFind(modelName);
    return (it == _byModelName.cend()) ? nullptr : &(*it);
}

bool PayloadCapabilityCatalog::overlayHas(const Entry* entry, const QString& feature)
{
    return entry && entry->overlayPhase1.contains(feature);
}
