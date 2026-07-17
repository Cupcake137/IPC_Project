#include <QtTest>
#include "DTCManager.h"
#include "Protocol.h"
#include "VCUStateMachine.h"
#include "VehicleModel.h"

namespace {
IpcProtocol::Frame makeFrame(quint16 id, std::initializer_list<quint8> payload)
{
    IpcProtocol::Frame frame;
    frame.id = id;
    frame.dlc = static_cast<quint8>(payload.size());
    int index = 0;
    for (const quint8 byte : payload) {
        frame.data[static_cast<size_t>(index++)] = byte;
    }
    return frame;
}
}

class BackendTests : public QObject {
    Q_OBJECT

private slots:
    void protocolRoundTrip();
    void checksumErrorIsRejected();
    void stateMachineAndLatchedDtc();
};

void BackendTests::protocolRoundTrip()
{
    std::array<quint8, IpcProtocol::MaxDlc> payload {};
    payload[0] = 123;
    quint8 counter = 7;
    const QByteArray bytes = IpcProtocol::encode(IpcProtocol::MotorCommand, 1, payload, counter);

    IpcProtocol::FrameParser parser;
    std::optional<IpcProtocol::Frame> parsed;
    for (const char byte : bytes) {
        parsed = parser.feed(static_cast<quint8>(byte));
    }

    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->id, static_cast<quint16>(IpcProtocol::MotorCommand));
    QCOMPARE(parsed->dlc, static_cast<quint8>(1));
    QCOMPARE(parsed->counter, static_cast<quint8>(7));
    QCOMPARE(parsed->data[0], static_cast<quint8>(123));
    QCOMPARE(parser.validFrames(), static_cast<quint64>(1));
}

void BackendTests::checksumErrorIsRejected()
{
    std::array<quint8, IpcProtocol::MaxDlc> payload {};
    payload[0] = 80;
    quint8 counter = 0;
    QByteArray bytes = IpcProtocol::encode(IpcProtocol::PedalSpeed, 1, payload, counter);
    bytes[bytes.size() - 1] = static_cast<char>(bytes.at(bytes.size() - 1) + 1);

    IpcProtocol::FrameParser parser;
    bool accepted = false;
    for (const char byte : bytes) {
        accepted = accepted || parser.feed(static_cast<quint8>(byte)).has_value();
    }

    QVERIFY(!accepted);
    QCOMPARE(parser.checksumErrors(), static_cast<quint64>(1));
}

void BackendTests::stateMachineAndLatchedDtc()
{
    VehicleModel model;
    DTCManager dtcManager;
    VCUStateMachine stateMachine(&model, &dtcManager);

    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Off));
    stateMachine.setTransportOpen(true);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Acc));
    stateMachine.setCommunicationHealthy(true);
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Acc));

    stateMachine.processFrame(makeFrame(IpcProtocol::PedalSpeed, {0, 0}));
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Ready));

    stateMachine.processFrame(makeFrame(
        IpcProtocol::GearState, {static_cast<quint8>(VehicleModel::Reverse)}));
    stateMachine.processFrame(makeFrame(IpcProtocol::PedalSpeed, {100, 20}));
    QCOMPARE(model.authorizedPwm(), 43);

    stateMachine.processFrame(makeFrame(IpcProtocol::PedalSpeed, {0, 0}));
    stateMachine.processFrame(makeFrame(
        IpcProtocol::GearState, {static_cast<quint8>(VehicleModel::Drive)}));
    stateMachine.processFrame(makeFrame(IpcProtocol::PedalSpeed, {50, 20}));
    QCOMPARE(model.authorizedPwm(), 127);

    stateMachine.processFrame(makeFrame(IpcProtocol::DtcStatus, {0x22}));
    QCOMPARE(model.state(), static_cast<int>(VehicleModel::Fault));
    QCOMPARE(model.authorizedPwm(), 0);
    stateMachine.processFrame(makeFrame(IpcProtocol::DtcStatus, {0x00}));
    QVERIFY(!stateMachine.clearDtc());

    stateMachine.processFrame(makeFrame(IpcProtocol::PedalSpeed, {0, 0}));
    stateMachine.processFrame(makeFrame(
        IpcProtocol::GearState, {static_cast<quint8>(VehicleModel::Park)}));
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

QTEST_APPLESS_MAIN(BackendTests)
#include "backend_tests.moc"
