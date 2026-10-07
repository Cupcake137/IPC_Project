#include <QtTest>
#include "CanDatabase.h"
#include "DTCManager.h"
#include "Protocol.h"
#include "SteeringInputManager.h"
#include "VCUStateMachine.h"
#include "VehicleModel.h"

namespace {
IpcProtocol::Frame driveFrame(quint8 pedal, quint8 speed,
                              quint8 gear, quint8 counter)
{
    IpcProtocol::Frame frame;
    CanDatabase::DriveStatus status;
    status.pedal = pedal;
    status.speed = speed;
    status.gear = gear;
    status.aliveCounter = counter;
    CanDatabase::packDriveStatus(status, &frame);
    return frame;
}

IpcProtocol::Frame diagFrame(quint8 dtc, quint8 counter)
{
    IpcProtocol::Frame frame;
    CanDatabase::DiagnosticStatus status;
    status.dtc = dtc;
    status.faultFlags = dtc == 0 ? 0 : 1;
    status.aliveCounter = counter;
    CanDatabase::packDiagnosticStatus(status, &frame);
    return frame;
}
}

class BackendTests : public QObject {
    Q_OBJECT

private slots:
    void canDatabaseKnownFrames();
    void protocolRoundTrip();
    void checksumErrorIsRejected();
    void stateMachineAndLatchedDtc();
    void motorCommandTimeoutDtcIsCritical();
    void warningAndCriticalFaultPolicy();
    void rejectedDriveFramesDoNotRefreshTelemetry();
    void steeringInputParsingAndValidation();
    void steeringInputRejectsDuplicateCounter();
};

void BackendTests::canDatabaseKnownFrames()
{
    CanDatabase::MotorCommand motor;
    motor.pwm = 107;
    motor.gear = CanDatabase::Drive;
    motor.torqueEnable = true;
    motor.aliveCounter = 0;

    IpcProtocol::Frame frame;
    QVERIFY(CanDatabase::packMotorCommand(motor, &frame));
    QCOMPARE(CanDatabase::toText(frame), QByteArrayLiteral("080#6B0700B2"));

    CanDatabase::DriveStatus drive;
    drive.pedal = 42;
    drive.speed = 63;
    drive.gear = CanDatabase::Drive;
    drive.appliedPwm = 107;
    drive.aliveCounter = 1;
    QVERIFY(CanDatabase::packDriveStatus(drive, &frame));
    QCOMPARE(CanDatabase::toText(frame), QByteArrayLiteral("100#2A3F036B01DA"));
}

void BackendTests::protocolRoundTrip()
{
    CanDatabase::MotorCommand command;
    command.pwm = 123;
    command.gear = CanDatabase::Drive;
    command.torqueEnable = true;
    command.aliveCounter = 7;
    IpcProtocol::Frame source;
    QVERIFY(CanDatabase::packMotorCommand(command, &source));
    const QByteArray bytes = IpcProtocol::encode(source);

    IpcProtocol::FrameParser parser;
    IpcProtocol::Frame parsed;
    bool accepted = false;
    for (const char byte : bytes) {
        if (parser.feed(static_cast<quint8>(byte), &parsed)) {
            accepted = true;
        }
    }

    QVERIFY(accepted);
    QCOMPARE(parsed.id, static_cast<quint16>(CanDatabase::VcuMotorCommand));
    QCOMPARE(parsed.dlc, static_cast<quint8>(4));
    QCOMPARE(parsed.data[0], static_cast<quint8>(123));
    CanDatabase::MotorCommand decoded;
    QVERIFY(CanDatabase::unpackMotorCommand(parsed, &decoded));
    QCOMPARE(decoded.aliveCounter, static_cast<quint8>(7));
}

void BackendTests::checksumErrorIsRejected()
{
    QByteArray bytes = IpcProtocol::encode(driveFrame(80, 20, CanDatabase::Drive, 0));
    bytes[bytes.size() - 1] = static_cast<char>(bytes.at(bytes.size() - 1) + 1);

    IpcProtocol::FrameParser parser;
    bool accepted = false;
    IpcProtocol::Frame parsed;
    for (const char byte : bytes) {
        if (parser.feed(static_cast<quint8>(byte), &parsed)) {
            accepted = true;
        }
    }

    QVERIFY(!accepted);
    QCOMPARE(parser.checksumErrors(), static_cast<quint64>(1));
}

void BackendTests::stateMachineAndLatchedDtc()
{
    VehicleModel model;
    DTCManager dtcManager;
    VCUStateMachine stateMachine(model, dtcManager);

    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Off));
    stateMachine.setTransportOpen(true);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Acc));
    stateMachine.setCommunicationHealthy(true);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Acc));

    stateMachine.processFrame(driveFrame(0, 0, CanDatabase::Park, 0));
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Ready));

    stateMachine.processFrame(driveFrame(100, 20, CanDatabase::Reverse, 1));
    QCOMPARE(model.authorizedPwm(), 43);

    stateMachine.processFrame(driveFrame(0, 0, CanDatabase::Drive, 2));
    stateMachine.processFrame(driveFrame(50, 20, CanDatabase::Drive, 3));
    QCOMPARE(model.authorizedPwm(), 127);
    stateMachine.setDriveMode(VehicleModel::Eco);
    QCOMPARE(model.authorizedPwm(), 76);
    stateMachine.setDriveMode(VehicleModel::Normal);
    QCOMPARE(model.authorizedPwm(), 127);

    stateMachine.processFrame(diagFrame(0x22, 0));
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Fault));
    QCOMPARE(model.authorizedPwm(), 0);
    stateMachine.processFrame(diagFrame(0x00, 1));
    QVERIFY(!stateMachine.clearDtc());

    stateMachine.processFrame(driveFrame(0, 0, CanDatabase::Park, 4));
    QVERIFY(stateMachine.clearDtc());
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Ready));

    stateMachine.setCommunicationHealthy(false);
    QCOMPARE(model.activeDtc(), 0xE1);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Fault));
    stateMachine.setCommunicationHealthy(true);
    QVERIFY(stateMachine.clearDtc());

    stateMachine.setCharging(true);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Charging));
    QCOMPARE(model.authorizedPwm(), 0);
    stateMachine.setCharging(false);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Ready));
}

void BackendTests::motorCommandTimeoutDtcIsCritical()
{
    DTCManager dtcManager;

    QCOMPARE(dtcManager.messageForCode(0xE2),
             QStringLiteral("U0101 - ECU motor-command timeout"));
    QVERIFY(dtcManager.isCritical(0xE2));

    dtcManager.reportEcuCode(0xE2);
    QCOMPARE(dtcManager.rawCode(), 0xE2);
    QCOMPARE(dtcManager.activeCode(), 0xE2);

    dtcManager.reportEcuCode(0x00);
    QCOMPARE(dtcManager.rawCode(), 0x00);
    QCOMPARE(dtcManager.activeCode(), 0xE2);
    QVERIFY(dtcManager.clearLatched());
    QCOMPARE(dtcManager.activeCode(), 0x00);
}

void BackendTests::warningAndCriticalFaultPolicy()
{
    VehicleModel model;
    DTCManager dtc;
    VCUStateMachine vcu(model, dtc);
    vcu.setTransportOpen(true);
    vcu.setCommunicationHealthy(true);
    vcu.processFrame(driveFrame(100, 20, CanDatabase::Drive, 0));
    vcu.processFrame(diagFrame(0x11, 0));
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Ready));
    QCOMPARE(model.authorizedPwm(), 80);
    QVERIFY(!dtc.isCritical(0x11));
    QVERIFY(dtc.isCritical(0x99));
    vcu.processFrame(driveFrame(100, 20, CanDatabase::Reverse, 1));
    QCOMPARE(model.authorizedPwm(), 43);
    vcu.processFrame(diagFrame(0x22, 1));
    vcu.processFrame(diagFrame(0x11, 2));
    QCOMPARE(model.activeDtc(), 0x22);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Fault));
    QCOMPARE(model.authorizedPwm(), 0);
    vcu.processFrame(diagFrame(0, 3));
    QVERIFY(!vcu.clearDtc());
    vcu.processFrame(driveFrame(0, 0, CanDatabase::Park, 2));
    QVERIFY(vcu.clearDtc());
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Ready));
    vcu.processFrame(diagFrame(0x99, 4));
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Fault));
    QCOMPARE(model.authorizedPwm(), 0);
}

void BackendTests::rejectedDriveFramesDoNotRefreshTelemetry()
{
    VehicleModel model;
    DTCManager dtc;
    VCUStateMachine vcu(model, dtc);
    auto frame = driveFrame(20, 24, CanDatabase::Drive, 0);
    QVERIFY(vcu.processFrame(frame));
    QVERIFY(!vcu.processFrame(frame));
    frame = driveFrame(20, 24, CanDatabase::Drive, 1);
    frame.data[0] ^= 1;
    QVERIFY(!vcu.processFrame(frame));
    QCOMPARE(model.speed(), 24);
}

void BackendTests::steeringInputParsingAndValidation()
{
    CanDatabase::KeyEvent source;
    source.keyCode = 5;
    source.eventType = CanDatabase::KeyPress;
    source.pressedKeys = 1U << 5;
    source.aliveCounter = 4;
    IpcProtocol::Frame frame;
    QVERIFY(CanDatabase::packKeyEvent(source, &frame));

    CanDatabase::KeyEvent parsed;
    QVERIFY(SteeringInputManager::parseKeyEvent(
        CanDatabase::toText(frame), &parsed));
    QCOMPARE(parsed.keyCode, static_cast<quint8>(5));
    QCOMPARE(parsed.eventType, static_cast<quint8>(CanDatabase::KeyPress));
    QCOMPARE(parsed.aliveCounter, static_cast<quint8>(4));
    QVERIFY(!SteeringInputManager::parseKeyEvent(
        QByteArrayLiteral("300#0000"), &parsed));
}

void BackendTests::steeringInputRejectsDuplicateCounter()
{
    SteeringInputManager manager;
    QSignalSpy eventSpy(&manager, &SteeringInputManager::keyEventReceived);

    CanDatabase::KeyEvent event;
    event.keyCode = 3;
    event.eventType = CanDatabase::KeyPress;
    event.pressedKeys = 1U << 3;
    event.aliveCounter = 10;
    IpcProtocol::Frame frame;
    QVERIFY(CanDatabase::packKeyEvent(event, &frame));
    const QByteArray first = CanDatabase::toText(frame);

    QVERIFY(manager.ingestCanPayload(first));
    QCOMPARE(eventSpy.count(), 1);
    QCOMPARE(eventSpy.at(0).at(0).toInt(), static_cast<int>(CanDatabase::KeyPress));
    QCOMPARE(eventSpy.at(0).at(1).toString(), QStringLiteral("A"));

    QVERIFY(!manager.ingestCanPayload(first));
    QCOMPARE(eventSpy.count(), 1);

    event.keyCode = 7;
    event.aliveCounter = 11;
    QVERIFY(CanDatabase::packKeyEvent(event, &frame));
    QVERIFY(manager.ingestCanPayload(CanDatabase::toText(frame)));
    QCOMPARE(eventSpy.count(), 2);
    QCOMPARE(eventSpy.at(1).at(1).toString(), QStringLiteral("B"));
}

QTEST_GUILESS_MAIN(BackendTests)
#include "backend_tests.moc"
