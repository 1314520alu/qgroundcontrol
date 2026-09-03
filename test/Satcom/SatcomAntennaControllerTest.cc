#include "SatcomAntennaControllerTest.h"

#include "SatcomAntennaController.h"

void SatcomAntennaControllerTest::_parseTypicalFrame()
{
    const auto frame =
        SatcomAntennaController::parseStatusFrame(QByteArrayLiteral("CC , 2 , -45.2 , 1 , 123.4 , 45.6 , FF\r\n"));
    QVERIFY(frame.valid);
    QCOMPARE(frame.antennaState, 2);
    QCOMPARE(frame.signalStrength, -45.2);
    QCOMPARE(frame.networkState, 1);
    QCOMPARE(frame.azimuthDeg, 123.4);
    QCOMPARE(frame.elevationDeg, 45.6);
}

void SatcomAntennaControllerTest::_parseCompactFrame()
{
    const auto frame = SatcomAntennaController::parseStatusFrame(QByteArrayLiteral("CC,0,0,0,0,0,FF"));
    QVERIFY(frame.valid);
    QCOMPARE(frame.antennaState, 0);
    QCOMPARE(frame.networkState, 0);
    QCOMPARE(frame.signalStrength + 1.0, 1.0);
    QCOMPARE(frame.azimuthDeg + 1.0, 1.0);
    QCOMPARE(frame.elevationDeg + 1.0, 1.0);
}

void SatcomAntennaControllerTest::_parseRejectsBadHeaderOrCount()
{
    QVERIFY(!SatcomAntennaController::parseStatusFrame(QByteArrayLiteral("XX , 1 , 1 , 1 , 1 , 1 , FF")).valid);
    QVERIFY(!SatcomAntennaController::parseStatusFrame(QByteArrayLiteral("CC , 1 , 1 , 1 , 1 , FF")).valid);
    QVERIFY(!SatcomAntennaController::parseStatusFrame(QByteArrayLiteral("")).valid);
    QVERIFY(!SatcomAntennaController::parseStatusFrame(QByteArrayLiteral("CC , a , 1 , 1 , 1 , 1 , FF")).valid);
}

void SatcomAntennaControllerTest::_commandBytes()
{
    QCOMPARE(SatcomAntennaController::commandBytes(SatcomAntennaController::Command::Unfold),
             QByteArrayLiteral("ant -open\r\n"));
    QCOMPARE(SatcomAntennaController::commandBytes(SatcomAntennaController::Command::Fold),
             QByteArrayLiteral("ant -fold\r\n"));
    QCOMPARE(SatcomAntennaController::commandBytes(SatcomAntennaController::Command::ElevationUp),
             QByteArrayLiteral("SPU\r\n"));
    QCOMPARE(SatcomAntennaController::commandBytes(SatcomAntennaController::Command::ElevationDown),
             QByteArrayLiteral("SPD\r\n"));
    QCOMPARE(SatcomAntennaController::commandBytes(SatcomAntennaController::Command::AzimuthUp),
             QByteArrayLiteral("SRL\r\n"));
    QCOMPARE(SatcomAntennaController::commandBytes(SatcomAntennaController::Command::AzimuthDown),
             QByteArrayLiteral("SRR\r\n"));
}

UT_REGISTER_TEST(SatcomAntennaControllerTest, TestLabel::Unit)
