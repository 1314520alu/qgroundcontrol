#include "MotorLayoutBuilderTest.h"

#include "MotorLayoutBuilder.h"

void MotorLayoutBuilderTest::_testQuadX()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassQuad,
                                             MotorLayoutBuilder::kFrameTypeX,
                                             4);
    QCOMPARE(r.topologyName, QStringLiteral("Quad X"));
    QVERIFY(r.spatial);
    QCOMPARE(r.motors.size(), 4);
    QCOMPARE(r.motors[0].letter, QStringLiteral("A"));
    QCOMPARE(r.motors[0].motorIndex, 1);
    QCOMPARE(r.motors[0].angleDeg, 45.0);
    QCOMPARE(static_cast<int>(r.motors[0].dir), static_cast<int>(MotorLayoutBuilder::SpinDir::CCW));
    QCOMPARE(r.motors[1].letter, QStringLiteral("B"));
    QCOMPARE(r.motors[1].motorIndex, 4);
    QCOMPARE(r.motors[2].letter, QStringLiteral("C"));
    QCOMPARE(r.motors[2].motorIndex, 2);
    QCOMPARE(r.motors[3].letter, QStringLiteral("D"));
    QCOMPARE(r.motors[3].motorIndex, 3);
}

void MotorLayoutBuilderTest::_testQuadPlus()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassQuad,
                                             MotorLayoutBuilder::kFrameTypePlus,
                                             4);
    QCOMPARE(r.topologyName, QStringLiteral("Quad +"));
    QCOMPARE(r.motors[0].letter, QStringLiteral("A"));
    QCOMPARE(r.motors[0].motorIndex, 3);
    QCOMPARE(r.motors[0].angleDeg, 0.0);
    QCOMPARE(r.motors[1].motorIndex, 1);
    QCOMPARE(r.motors[2].motorIndex, 4);
    QCOMPARE(r.motors[3].motorIndex, 2);
}

void MotorLayoutBuilderTest::_testHexaX()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassHex,
                                             MotorLayoutBuilder::kFrameTypeX,
                                             6);
    QCOMPARE(r.motors.size(), 6);
    QCOMPARE(r.motors[0].letter, QStringLiteral("A"));
    QCOMPARE(r.motors[0].motorIndex, 5);
    QCOMPARE(r.motors[1].motorIndex, 1);
    QCOMPARE(r.motors[5].letter, QStringLiteral("F"));
    QCOMPARE(r.motors[5].motorIndex, 3);
}

void MotorLayoutBuilderTest::_testOctoX()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassOcta,
                                             MotorLayoutBuilder::kFrameTypeX,
                                             8);
    QCOMPARE(r.motors.size(), 8);
    QCOMPARE(r.motors[0].letter, QStringLiteral("A"));
    QCOMPARE(r.motors[0].motorIndex, 1);
    QCOMPARE(r.motors[1].motorIndex, 3);
    QCOMPARE(r.motors[7].letter, QStringLiteral("H"));
    QCOMPARE(r.motors[7].motorIndex, 5);
}

void MotorLayoutBuilderTest::_testOctoQuadX()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassOctaQuad,
                                             MotorLayoutBuilder::kFrameTypeX,
                                             8);
    QCOMPARE(r.topologyName, QStringLiteral("OctoQuad X Coaxial"));
    QCOMPARE(r.motors.size(), 8);
    QCOMPARE(r.motors[0].letter, QStringLiteral("A"));
    QCOMPARE(r.motors[0].motorIndex, 1);
    QCOMPARE(static_cast<int>(r.motors[0].layer), static_cast<int>(MotorLayoutBuilder::Layer::Top));
    QCOMPARE(r.motors[1].letter, QStringLiteral("B"));
    QCOMPARE(r.motors[1].motorIndex, 6);
    QCOMPARE(static_cast<int>(r.motors[1].layer), static_cast<int>(MotorLayoutBuilder::Layer::Bottom));
    QCOMPARE(r.motors[0].angleDeg, r.motors[1].angleDeg);
}

void MotorLayoutBuilderTest::_testY6B()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassY6,
                                             MotorLayoutBuilder::kFrameTypeY6B,
                                             6);
    QCOMPARE(r.motors.size(), 6);
    QCOMPARE(r.motors[0].motorIndex, 1);
    QCOMPARE(static_cast<int>(r.motors[0].dir), static_cast<int>(MotorLayoutBuilder::SpinDir::CW));
    QCOMPARE(r.motors[1].motorIndex, 2);
    QCOMPARE(static_cast<int>(r.motors[1].dir), static_cast<int>(MotorLayoutBuilder::SpinDir::CCW));
}

void MotorLayoutBuilderTest::_testDodecaHexaX()
{
    const auto r = MotorLayoutBuilder::build(MotorLayoutBuilder::kFrameClassDodecaHexa,
                                             MotorLayoutBuilder::kFrameTypeX,
                                             12);
    QCOMPARE(r.topologyName, QStringLiteral("DodecaHexa X Coaxial"));
    QVERIFY(r.spatial);
    QCOMPARE(r.motors.size(), 12);
    QCOMPARE(r.motors[0].letter, QStringLiteral("A"));
    QCOMPARE(r.motors[0].motorIndex, 1);
    QCOMPARE(r.motors[0].angleDeg, 30.0);
    QCOMPARE(static_cast<int>(r.motors[0].layer), static_cast<int>(MotorLayoutBuilder::Layer::Top));
    QCOMPARE(static_cast<int>(r.motors[0].dir), static_cast<int>(MotorLayoutBuilder::SpinDir::CCW));
    QCOMPARE(r.motors[1].letter, QStringLiteral("B"));
    QCOMPARE(r.motors[1].motorIndex, 2);
    QCOMPARE(r.motors[1].angleDeg, 30.0);
    QCOMPARE(static_cast<int>(r.motors[1].layer), static_cast<int>(MotorLayoutBuilder::Layer::Bottom));
    QCOMPARE(static_cast<int>(r.motors[1].dir), static_cast<int>(MotorLayoutBuilder::SpinDir::CW));
    QCOMPARE(r.motors[11].letter, QStringLiteral("L"));
    QCOMPARE(r.motors[11].motorIndex, 12);
}

void MotorLayoutBuilderTest::_testUnknownFallback()
{
    const auto r = MotorLayoutBuilder::build(-1, -1, -1);
    QVERIFY(!r.spatial);
    QCOMPARE(r.motors.size(), 8);
    QCOMPARE(r.motors[0].motorIndex, 1);
}

#include "UnitTest.h"

UT_REGISTER_TEST(MotorLayoutBuilderTest, TestLabel::Unit)
