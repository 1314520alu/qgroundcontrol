#pragma once

#include <QtCore/QList>
#include <QtCore/QString>

/// Builds ArduPilot-aligned motor test layout entries (letter, board motor index, angle, spin).
/// Angles: 0° = nose (up), increasing clockwise. motorIndex is 1-based BOARD order for Vehicle::motorTest.
class MotorLayoutBuilder
{
public:
    enum class SpinDir {
        Unknown = 0,
        CW,
        CCW,
    };

    enum class Layer {
        Single = 0,
        Top,
        Bottom,
    };

    struct Entry {
        QString letter;   ///< A, B, C, ...
        int motorIndex = 0; ///< 1-based board motor number for MAV_CMD_DO_MOTOR_TEST
        double angleDeg = 0; ///< 0 = nose, clockwise positive
        SpinDir dir = SpinDir::Unknown;
        Layer layer = Layer::Single;
    };

    struct Result {
        QString topologyName;
        bool spatial = false; ///< false → UI should use numbered grid fallback
        QList<Entry> motors;
    };

    /// @param frameClass APM FRAME_CLASS (-1 if unavailable)
    /// @param frameType APM FRAME_TYPE (-1 if unavailable)
    /// @param motorCount Vehicle::motorCount() (-1 if unknown)
    static Result build(int frameClass, int frameType, int motorCount);

    static constexpr int kMaxMotors = 16;

    // APM FRAME_CLASS / FRAME_TYPE (match APMAirframeComponentController)
    static constexpr int kFrameClassQuad = 1;
    static constexpr int kFrameClassHex = 2;
    static constexpr int kFrameClassOcta = 3;
    static constexpr int kFrameClassOctaQuad = 4;
    static constexpr int kFrameClassY6 = 5;
    static constexpr int kFrameClassDodecaHexa = 12;
    static constexpr int kFrameClassDeca = 14;

    static constexpr int kFrameTypePlus = 0;
    static constexpr int kFrameTypeX = 1;
    static constexpr int kFrameTypeY6B = 10;

private:
    static Result _quadX();
    static Result _quadPlus();
    static Result _hexaX();
    static Result _octoX();
    static Result _octoQuadX();
    static Result _y6b();
    static Result _dodecaHexaX();
    static Result _dodecaHexaPlus();
    static Result _ringFallback(const QString &name, int count, double startAngleDeg, bool plusLayout);
    static Result _unknownFallback(int motorCount);
    static Entry _entry(QChar letter, int motorIndex, double angleDeg, SpinDir dir, Layer layer = Layer::Single);
};
