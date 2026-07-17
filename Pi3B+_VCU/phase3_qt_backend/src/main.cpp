#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "DTCManager.h"
#include "SerialManager.h"
#include "SimulationManager.h"
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
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("IPC Automotive Cluster"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("IPC VCU backend with UART and simulation modes"));
    parser.addHelpOption();
    const QCommandLineOption simulateOption(
        QStringList() << QStringLiteral("s") << QStringLiteral("simulate"),
        QStringLiteral("Run without hardware using deterministic vehicle data."));
    parser.addOption(simulateOption);
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
    VCUStateMachine stateMachine(&model, &dtcManager);

    QObject::connect(&serial, &SerialManager::errorOccurred, [](const QString& message) {
        qWarning() << "[Serial]" << message;
    });
    QObject::connect(&model, &VehicleModel::changed, [&]() {
        qInfo().nospace()
            << "[Vehicle] state=" << model.state()
            << " gear=" << gearText(model.gear())
            << " speed=" << model.speed()
            << "km/h pedal=" << model.pedal()
            << "% soc=" << model.soc()
            << "% pwm=" << model.authorizedPwm()
            << " dtc=" << dtcManager.messageForCode(model.activeDtc());
    });

    if (simulationMode) {
        QObject::connect(&simulation, &SimulationManager::frameGenerated,
                         &stateMachine, &VCUStateMachine::processFrame);
        QObject::connect(&stateMachine, &VCUStateMachine::motorPwmChanged,
                         &simulation, &SimulationManager::acceptMotorCommand);
        QObject::connect(&simulation, &SimulationManager::chargingChanged,
                         &stateMachine, &VCUStateMachine::setCharging);
        QObject::connect(&simulation, &SimulationManager::clearDtcRequested,
                         &stateMachine, &VCUStateMachine::clearDtc);
        model.setTransportOpen(true);
        model.setCommunicationHealthy(true);
        stateMachine.setTransportOpen(true);
        stateMachine.setCommunicationHealthy(true);
        simulation.start();
        qInfo() << "IPC Phase 3 backend started in simulation mode";
    } else {
        QObject::connect(&serial, &SerialManager::frameReceived,
                         &stateMachine, &VCUStateMachine::processFrame);
        QObject::connect(&stateMachine, &VCUStateMachine::motorPwmChanged,
                         &serial, &SerialManager::sendMotorCommand);
        QObject::connect(&serial, &SerialManager::transportStateChanged,
                         &stateMachine, &VCUStateMachine::setTransportOpen);
        QObject::connect(&serial, &SerialManager::communicationStateChanged,
                         &stateMachine, &VCUStateMachine::setCommunicationHealthy);
        QObject::connect(&serial, &SerialManager::transportStateChanged,
                         &model, &VehicleModel::setTransportOpen);
        QObject::connect(&serial, &SerialManager::communicationStateChanged,
                         &model, &VehicleModel::setCommunicationHealthy);
        QObject::connect(&serial, &SerialManager::statisticsChanged, &model,
                         &VehicleModel::setCommunicationStatistics);
        serial.open(portName);
        qInfo() << "IPC Phase 3 backend started on" << portName << "@" << IpcProtocol::BaudRate;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vehicleModel"), &model);
    engine.rootContext()->setContextProperty(QStringLiteral("dtcManager"), &dtcManager);
    engine.rootContext()->setContextProperty(QStringLiteral("vcuController"), &stateMachine);
    engine.rootContext()->setContextProperty(QStringLiteral("simulationMode"), simulationMode);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 2;
    }

    return app.exec();
}
