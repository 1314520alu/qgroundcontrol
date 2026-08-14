#include "MotorLayoutBuilder.h"

MotorLayoutBuilder::Entry MotorLayoutBuilder::_entry(QChar letter, int motorIndex, double angleDeg, SpinDir dir, Layer layer)
{
    Entry e;
    e.letter = QString(letter);
    e.motorIndex = motorIndex;
    e.angleDeg = angleDeg;
    e.dir = dir;
    e.layer = layer;
    return e;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_quadX()
{
    // https://ardupilot.org/copter/_images/m_01_01_quad_x.svg
    Result r;
    r.topologyName = QStringLiteral("Quad X");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 1, 45, SpinDir::CCW),
        _entry(u'B', 4, 135, SpinDir::CW),
        _entry(u'C', 2, 225, SpinDir::CCW),
        _entry(u'D', 3, 315, SpinDir::CW),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_quadPlus()
{
    // https://ardupilot.org/copter/_images/m_01_00_quad_plus.svg
    Result r;
    r.topologyName = QStringLiteral("Quad +");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 3, 0, SpinDir::CW),
        _entry(u'B', 1, 90, SpinDir::CCW),
        _entry(u'C', 4, 180, SpinDir::CW),
        _entry(u'D', 2, 270, SpinDir::CCW),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_hexaX()
{
    // https://ardupilot.org/copter/_images/m_02_01_hexa_x.svg
    Result r;
    r.topologyName = QStringLiteral("Hexa X");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 5, 30, SpinDir::CCW),
        _entry(u'B', 1, 90, SpinDir::CW),
        _entry(u'C', 4, 150, SpinDir::CCW),
        _entry(u'D', 6, 210, SpinDir::CW),
        _entry(u'E', 2, 270, SpinDir::CCW),
        _entry(u'F', 3, 330, SpinDir::CW),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_octoX()
{
    // https://ardupilot.org/copter/_images/m_03_01_octo_x.svg
    Result r;
    r.topologyName = QStringLiteral("Octo X");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 1, 22.5, SpinDir::CW),
        _entry(u'B', 3, 67.5, SpinDir::CCW),
        _entry(u'C', 8, 112.5, SpinDir::CW),
        _entry(u'D', 4, 157.5, SpinDir::CCW),
        _entry(u'E', 2, 202.5, SpinDir::CW),
        _entry(u'F', 6, 247.5, SpinDir::CCW),
        _entry(u'G', 7, 292.5, SpinDir::CW),
        _entry(u'H', 5, 337.5, SpinDir::CCW),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_octoQuadX()
{
    // https://ardupilot.org/copter/_images/m_04_01_octo_quad_x.svg
    // Test order: top then bottom at each corner, clockwise from front-right.
    Result r;
    r.topologyName = QStringLiteral("OctoQuad X Coaxial");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 1, 45, SpinDir::CCW, Layer::Top),
        _entry(u'B', 6, 45, SpinDir::CW, Layer::Bottom),
        _entry(u'C', 4, 135, SpinDir::CW, Layer::Top),
        _entry(u'D', 7, 135, SpinDir::CCW, Layer::Bottom),
        _entry(u'E', 3, 225, SpinDir::CCW, Layer::Top),
        _entry(u'F', 8, 225, SpinDir::CW, Layer::Bottom),
        _entry(u'G', 2, 315, SpinDir::CW, Layer::Top),
        _entry(u'H', 5, 315, SpinDir::CCW, Layer::Bottom),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_y6b()
{
    // https://ardupilot.org/copter/_images/m_05_10_y6_b.svg
    Result r;
    r.topologyName = QStringLiteral("Y6B Coaxial");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 1, 60, SpinDir::CW, Layer::Top),
        _entry(u'B', 2, 60, SpinDir::CCW, Layer::Bottom),
        _entry(u'C', 3, 180, SpinDir::CW, Layer::Top),
        _entry(u'D', 4, 180, SpinDir::CCW, Layer::Bottom),
        _entry(u'E', 5, 300, SpinDir::CW, Layer::Top),
        _entry(u'F', 6, 300, SpinDir::CCW, Layer::Bottom),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_dodecaHexaX()
{
    // AP_MotorsMatrix::setup_dodecahexa_matrix MOTOR_FRAME_TYPE_X
    // https://ardupilot.org/copter/_images/m_12_01_dodecahexa_x.svg
    // Angles: 0 = nose, clockwise+. Test order A→L: clockwise from FR, top then bottom.
    Result r;
    r.topologyName = QStringLiteral("DodecaHexa X Coaxial");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 1, 30, SpinDir::CCW, Layer::Top),
        _entry(u'B', 2, 30, SpinDir::CW, Layer::Bottom),
        _entry(u'C', 3, 90, SpinDir::CW, Layer::Top),
        _entry(u'D', 4, 90, SpinDir::CCW, Layer::Bottom),
        _entry(u'E', 5, 150, SpinDir::CCW, Layer::Top),
        _entry(u'F', 6, 150, SpinDir::CW, Layer::Bottom),
        _entry(u'G', 7, 210, SpinDir::CW, Layer::Top),
        _entry(u'H', 8, 210, SpinDir::CCW, Layer::Bottom),
        _entry(u'I', 9, 270, SpinDir::CCW, Layer::Top),
        _entry(u'J', 10, 270, SpinDir::CW, Layer::Bottom),
        _entry(u'K', 11, 330, SpinDir::CW, Layer::Top),
        _entry(u'L', 12, 330, SpinDir::CCW, Layer::Bottom),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_dodecaHexaPlus()
{
    // AP_MotorsMatrix::setup_dodecahexa_matrix MOTOR_FRAME_TYPE_PLUS
    Result r;
    r.topologyName = QStringLiteral("DodecaHexa + Coaxial");
    r.spatial = true;
    r.motors = {
        _entry(u'A', 1, 0, SpinDir::CCW, Layer::Top),
        _entry(u'B', 2, 0, SpinDir::CW, Layer::Bottom),
        _entry(u'C', 3, 60, SpinDir::CW, Layer::Top),
        _entry(u'D', 4, 60, SpinDir::CCW, Layer::Bottom),
        _entry(u'E', 5, 120, SpinDir::CCW, Layer::Top),
        _entry(u'F', 6, 120, SpinDir::CW, Layer::Bottom),
        _entry(u'G', 7, 180, SpinDir::CW, Layer::Top),
        _entry(u'H', 8, 180, SpinDir::CCW, Layer::Bottom),
        _entry(u'I', 9, 240, SpinDir::CCW, Layer::Top),
        _entry(u'J', 10, 240, SpinDir::CW, Layer::Bottom),
        _entry(u'K', 11, 300, SpinDir::CW, Layer::Top),
        _entry(u'L', 12, 300, SpinDir::CCW, Layer::Bottom),
    };
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_ringFallback(const QString &name, int count, double startAngleDeg, bool /*plusLayout*/)
{
    Result r;
    r.topologyName = name;
    r.spatial = true;
    const int n = qBound(1, count, kMaxMotors);
    const double step = 360.0 / n;
    double angle = startAngleDeg;
    for (int i = 0; i < n; ++i) {
        r.motors.append(_entry(QChar(u'A' + i), i + 1, angle, SpinDir::Unknown));
        angle += step;
        if (angle >= 360.0) {
            angle -= 360.0;
        }
    }
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::_unknownFallback(int motorCount)
{
    Result r;
    r.topologyName = QStringLiteral("Unknown");
    r.spatial = false;
    const int n = motorCount < 0 ? 8 : qBound(1, motorCount, kMaxMotors);
    for (int i = 0; i < n; ++i) {
        r.motors.append(_entry(QChar(u'A' + i), i + 1, 0, SpinDir::Unknown));
    }
    return r;
}

MotorLayoutBuilder::Result MotorLayoutBuilder::build(int frameClass, int frameType, int motorCount)
{
    if (frameClass == kFrameClassQuad) {
        if (frameType == kFrameTypePlus) {
            return _quadPlus();
        }
        // Default X (and other X-like types)
        return _quadX();
    }
    if (frameClass == kFrameClassHex) {
        return _hexaX();
    }
    if (frameClass == kFrameClassOcta) {
        return _octoX();
    }
    if (frameClass == kFrameClassOctaQuad) {
        return _octoQuadX();
    }
    if (frameClass == kFrameClassY6) {
        return _y6b();
    }
    if (frameClass == kFrameClassDeca && motorCount > 0) {
        return _ringFallback(QStringLiteral("Deca"), motorCount, 18.0, false);
    }
    if (frameClass == kFrameClassDodecaHexa) {
        if (frameType == kFrameTypePlus) {
            return _dodecaHexaPlus();
        }
        return _dodecaHexaX();
    }

    if (motorCount == 4) {
        return _quadX();
    }
    if (motorCount == 6) {
        return _hexaX();
    }
    if (motorCount == 8) {
        return _octoX();
    }
    if (motorCount > 0) {
        // X-style start: first motor right of nose
        const double start = 360.0 / (2.0 * motorCount);
        return _ringFallback(QStringLiteral("Motors"), motorCount, start, false);
    }

    return _unknownFallback(motorCount);
}
