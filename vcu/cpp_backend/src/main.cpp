#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "CanDatabase.h"
#include "DTCManager.h"
#include "SerialManager.h"
#include "SimulationManager.h"
#include "SteeringInputManager.h"
#include "VCUStateMachine.h"
#include "VehicleModel.h"

static QString gearText(int gear)
{
    switch (gear) {
    case VehicleModel::Reverse: return QStringLiteral("R");
    case VehicleModel::Neutral: return QStringLiteral("N");
    case VehicleModel::Drive: return QStringLiteral("D");
    default: return QStringLiteral("P");
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("IPC VCU Backend"));
    QCoreApplication::setApplicationVersion(QStringLiteral("4.0.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("IPCProject"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("IPC VCU backend with UART and simulation modes"));
    parser.addHelpOption();
    const QCommandLineOption simulateOption(
        QStringList() << QStringLiteral("s") << QStringLiteral("simulate"),
        QStringLiteral("Run without hardware using deterministic vehicle data."));
    const QCommandLineOption smokeTestOption(
        QStringList() << QStringLiteral("smoke-test"),
        QStringLiteral("Exit successfully after the backend runs for 1.5 seconds."));
    const QCommandLineOption noMqttOption(
        QStringList() << QStringLiteral("no-mqtt"),
        QStringLiteral("Disable the steering-controller MQTT input."));
    parser.addOption(simulateOption);
    parser.addOption(smokeTestOption);
    parser.addOption(noMqttOption);
    parser.addPositionalArgument(
        QStringLiteral("serial-port"),
        QStringLiteral("UART device used in hardware mode."),
        QStringLiteral("[/dev/ttyUSB0]"));
    parser.process(app);

    const bool simulationMode = parser.isSet(simulateOption);
    const QStringList positionalArguments = parser.positionalArguments();
    const QString portName = !positionalArguments.isEmpty()
        ? positionalArguments.first()
        : QStringLiteral("/dev/ttyUSB0");

    VehicleModel model;
    DTCManager dtcManager;
    SerialManager serial;
    SimulationManager simulation;
    SteeringInputManager steeringInput;
    VCUStateMachine stateMachine(model, dtcManager);

    QObject::connect(&steeringInput, &SteeringInputManager::errorOccurred,
                     [](const QString& message) {
        qWarning() << "[SWC]" << message;
    });
    QObject::connect(&steeringInput, &SteeringInputManager::brokerConnectedChanged,
                     [](bool connected) {
        qInfo() << "[SWC] broker"
                << (connected ? "connected" : "disconnected");
    });
    QObject::connect(&steeringInput, &SteeringInputManager::controllerOnlineChanged,
                     [](bool online) {
        qInfo() << "[SWC] controller"
                << (online ? "online" : "offline");
    });
    QObject::connect(&steeringInput, &SteeringInputManager::keyEventReceived,
                     [&model, &stateMachine](int eventType,
                                             const QString& key, int aliveCounter) {
        qInfo().nospace()
            << "[SWC] counter=" << aliveCounter
            << " key=" << key
            << " event=" << SteeringInputManager::eventTypeName(eventType);
        if (eventType == CanDatabase::KeyPress
            && key == QStringLiteral("D")) {
            if (model.activeDtc() == 0) {
                qInfo() << "[VCU] no active DTC to clear";
            } else {
                qInfo() << "[VCU] safe DTC clear"
                        << (stateMachine.clearDtc() ? "accepted" : "deferred");
            }
        }
    });

    if (!parser.isSet(noMqttOption)) {
        const QString mqttHost = qEnvironmentVariable("IPC_MQTT_HOST", "127.0.0.1");
        bool validPort = false;
        const int mqttPortValue = qEnvironmentVariableIntValue("IPC_MQTT_PORT", &validPort);
        const quint16 mqttPort = validPort && mqttPortValue > 0 && mqttPortValue <= 65535
            ? static_cast<quint16>(mqttPortValue) : 1883;
        steeringInput.start(
            mqttHost,
            mqttPort,
            qEnvironmentVariable("IPC_MQTT_USER", "ipc_qt"),
            qEnvironmentVariable("IPC_MQTT_PASSWORD"));
    }

    QObject::connect(&serial, &SerialManager::errorOccurred, [](const QString& message) {
        qWarning() << "[Serial]" << message;
    });
    QTimer telemetryLogTimer;
    telemetryLogTimer.setInterval(1000);
    QObject::connect(&telemetryLogTimer, &QTimer::timeout, [&]() {
        qInfo().nospace()
            << "[Vehicle] state=" << model.state()
            << " gear=" << gearText(model.gear())
            << " speed=" << model.speed()
            << "km/h pedal=" << model.pedal()
            << "% soc=" << model.soc()
            << "% pwm=" << model.authorizedPwm()
            << " dtc=" << dtcManager.messageForCode(model.activeDtc());
    });
    telemetryLogTimer.start();

    if (simulationMode) {
        QObject::connect(&simulation, &SimulationManager::frameGenerated,
                         &app, [&stateMachine, &simulation, &model](IpcProtocol::Frame frame) {
            stateMachine.processFrame(frame);
            simulation.acceptMotorCommand(model.authorizedPwm());
        });
        QObject::connect(&simulation, &SimulationManager::chargingChanged,
                         &app, [&stateMachine](bool charging) {
            stateMachine.setCharging(charging);
        });
        QObject::connect(&simulation, &SimulationManager::clearDtcRequested,
                         &app, [&stateMachine]() {
            stateMachine.clearDtc();
        });
        stateMachine.setTransportOpen(true);
        stateMachine.setCommunicationHealthy(true);
        simulation.start();
        qInfo() << "IPC VCU backend started in simulation mode";
    } else {
        QObject::connect(&serial, &SerialManager::frameReceived,
                         &app, [&stateMachine, &serial, &model](IpcProtocol::Frame frame) {
            stateMachine.processFrame(frame);
            serial.sendMotorCommand(model.authorizedPwm(), model.gear());
        });
        QObject::connect(&serial, &SerialManager::transportStateChanged,
                         &app, [&stateMachine, &serial, &model](bool open) {
            stateMachine.setTransportOpen(open);
            serial.sendMotorCommand(model.authorizedPwm(), model.gear());
        });
        QObject::connect(&serial, &SerialManager::communicationStateChanged,
                         &app, [&stateMachine, &serial, &model](bool healthy) {
            stateMachine.setCommunicationHealthy(healthy);
            serial.sendMotorCommand(model.authorizedPwm(), model.gear());
        });
        serial.open(portName);
        qInfo() << "IPC VCU backend started on" << portName << "@" << IpcProtocol::BaudRate;
    }

    if (parser.isSet(smokeTestOption)) {
        QTimer::singleShot(1500, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
