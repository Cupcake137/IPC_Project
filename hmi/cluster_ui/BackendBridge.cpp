#include "BackendBridge.h"

#include <QDebug>
#include <QMap>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QtGlobal>

#include "CanDatabase.h"

BackendBridge::BackendBridge(QObject* parent)
    : QObject(parent)
    , m_stateMachine(m_model, m_dtcManager)
{
    connect(&m_steeringInput, &SteeringInputManager::keyEventReceived,
            this, &BackendBridge::handleKey);
    connect(&m_steeringInput, &SteeringInputManager::controllerOnlineChanged,
            this, &BackendBridge::setKeypadOnline);
    connect(&m_steeringInput, &SteeringInputManager::errorOccurred,
            [](const QString& message) { qWarning() << "[SWC]" << message; });
    connect(&m_serial, &SerialManager::errorOccurred,
            [](const QString& message) { qWarning() << "[Serial]" << message; });

    QTimer* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(50);
    connect(refreshTimer, &QTimer::timeout,
            this, &BackendBridge::updateCalculatedValues);
    m_tripClock.start();
    refreshTimer->start();
    QTimer* saveTimer = new QTimer(this);
    saveTimer->setInterval(5000);
    connect(saveTimer, &QTimer::timeout, this, &BackendBridge::saveDistance);
    saveTimer->start();
}

BackendBridge::~BackendBridge()
{
    saveDistance();
}

void BackendBridge::saveDistance()
{
    if (!m_distancePath.isEmpty() && !m_distance.save(m_distancePath)) {
        qWarning() << "[HMI] Could not save distance metrics";
    }
}

void BackendBridge::processVehicleFrame(IpcProtocol::Frame frame)
{
    const qint64 nowMs = m_tripClock.elapsed();
    m_distance.update(nowMs, m_distanceConnected);
    if (m_stateMachine.processFrame(frame) && frame.id == CanDatabase::EcuDriveStatus) {
        m_distance.recordSpeed(m_model.speed(), nowMs);
    }
}

int BackendBridge::motorOutput() const
{
    return m_model.authorizedPwm() * 100 / 255;
}

QString BackendBridge::gear() const
{
    switch (m_model.gear()) {
    case VehicleModel::Reverse: return QStringLiteral("R");
    case VehicleModel::Neutral: return QStringLiteral("N");
    case VehicleModel::Drive: return QStringLiteral("D");
    default: return QStringLiteral("P");
    }
}

QString BackendBridge::driveModeName() const
{
    switch (m_model.driveMode()) {
    case VehicleModel::Eco: return QStringLiteral("ECO");
    case VehicleModel::Sport: return QStringLiteral("SPORT");
    default: return QStringLiteral("NORMAL");
    }
}

bool BackendBridge::ready() const
{
    return m_model.state() == VehicleModel::Ready;
}

int BackendBridge::warningLevel() const
{
    const int code = m_model.activeDtc();
    if (code != 0) {
        return m_dtcManager.isCritical(code) ? 2 : 1;
    }
    return m_model.soc() <= 20 ? 1 : 0;
}

QString BackendBridge::warningText() const
{
    if (m_model.activeDtc() != 0) {
        return m_dtcManager.messageForCode(m_model.activeDtc());
    }
    return m_model.soc() <= 20
        ? QStringLiteral("Low battery - charge soon") : QString();
}

void BackendBridge::start(bool simulationMode, const QString& serialPort,
                          bool mqttEnabled)
{
    if (mqttEnabled) {
        startMqtt();
    }

    if (simulationMode) {
        m_distanceConnected = true;
        startSimulation();
    } else {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (QDir().mkpath(directory)) {
            m_distancePath = directory + QStringLiteral("/distance.ini");
            m_distance.load(m_distancePath);
        } else {
            qWarning() << "[HMI] Distance directory is unavailable";
        }
        startHardware(serialPort);
    }
}

void BackendBridge::startMqtt()
{
    bool validPort = false;
    const int portValue = qEnvironmentVariableIntValue("IPC_MQTT_PORT", &validPort);

    m_steeringInput.start(
        qEnvironmentVariable("IPC_MQTT_HOST", "127.0.0.1"),
        validPort ? static_cast<quint16>(portValue) : 1883,
        qEnvironmentVariable("IPC_MQTT_USER", "ipc_qt"),
        qEnvironmentVariable("IPC_MQTT_PASSWORD"));
}

void BackendBridge::startSimulation()
{
    connect(&m_simulation, &SimulationManager::frameGenerated,
            this, [this](IpcProtocol::Frame frame) {
        processVehicleFrame(frame);
        m_simulation.acceptMotorCommand(m_model.authorizedPwm());
    });
    connect(&m_simulation, &SimulationManager::chargingChanged,
            this, [this](bool charging) {
        m_stateMachine.setCharging(charging);
    });
    connect(&m_simulation, &SimulationManager::clearDtcRequested,
            this, [this]() { m_stateMachine.clearDtc(); });

    m_stateMachine.setTransportOpen(true);
    m_stateMachine.setCommunicationHealthy(true);
    m_simulation.start();
}

void BackendBridge::startHardware(const QString& serialPort)
{
    connect(&m_serial, &SerialManager::frameReceived,
            this, [this](IpcProtocol::Frame frame) {
        processVehicleFrame(frame);
        m_serial.sendMotorCommand(m_model.authorizedPwm(), m_model.gear());
    });
    connect(&m_serial, &SerialManager::transportStateChanged,
            this, [this](bool open) {
        if (!open) m_distanceConnected = false;
        m_stateMachine.setTransportOpen(open);
        m_serial.sendMotorCommand(m_model.authorizedPwm(), m_model.gear());
    });
    connect(&m_serial, &SerialManager::communicationStateChanged,
            this, [this](bool healthy) {
        m_distanceConnected = healthy;
        m_stateMachine.setCommunicationHealthy(healthy);
        m_serial.sendMotorCommand(m_model.authorizedPwm(), m_model.gear());
    });

    m_serial.open(serialPort);
}

void BackendBridge::handleKey(int eventType, const QString& key, int aliveCounter)
{
    Q_UNUSED(aliveCounter)
    if (eventType != CanDatabase::KeyPress) {
        return;
    }

    if (key == QStringLiteral("1")) {
        cycleLights();
    } else if (key == QStringLiteral("3")) {
        toggleHighBeam();
    } else if (key == QStringLiteral("7")) {
        toggleFogLight();
    } else if (key == QStringLiteral("9")) {
        setDriveMode((driveMode() + 1) % 3);
    } else if (key == QStringLiteral("*")) {
        toggleLeftSignal();
    } else if (key == QStringLiteral("0")) {
        toggleHazard();
    } else if (key == QStringLiteral("#")) {
        toggleRightSignal();
    } else if (key == QStringLiteral("D")) {
        acknowledgeWarning();
    }

    const QMap<QString, QString> actions = {
        {QStringLiteral("2"), QStringLiteral("NAV_UP")},
        {QStringLiteral("4"), QStringLiteral("NAV_LEFT")},
        {QStringLiteral("5"), QStringLiteral("SELECT")},
        {QStringLiteral("6"), QStringLiteral("NAV_RIGHT")},
        {QStringLiteral("8"), QStringLiteral("NAV_DOWN")},
        {QStringLiteral("A"), QStringLiteral("OPEN_MENU")},
        {QStringLiteral("B"), QStringLiteral("BACK")},
        {QStringLiteral("C"), QStringLiteral("HOME")},
        {QStringLiteral("D"), QStringLiteral("ACKNOWLEDGE")}
    };
    if (actions.contains(key)) {
        const QString action = actions.value(key);
        qInfo() << "[HMI] key=" << key << "action=" << action;
        emit uiAction(action);
    }
}

void BackendBridge::cycleLights()
{
    m_lightStage = (m_lightStage + 1) % 3;
    if (m_lightStage == 0) {
        m_highBeamOn = false;
    }
    qInfo() << "[HMI] lights stage=" << m_lightStage;
    emit controlsChanged();
}

void BackendBridge::toggleHighBeam()
{
    m_highBeamOn = !m_highBeamOn;
    qInfo() << "[HMI] high beam=" << m_highBeamOn;
    emit controlsChanged();
}

void BackendBridge::toggleFogLight()
{
    m_fogLightOn = !m_fogLightOn;
    qInfo() << "[HMI] fog light=" << m_fogLightOn;
    emit controlsChanged();
}

void BackendBridge::toggleLeftSignal()
{
    m_leftSignalOn = !m_leftSignalOn;
    m_rightSignalOn = false;
    m_hazardOn = false;
    qInfo() << "[HMI] left signal=" << m_leftSignalOn;
    emit controlsChanged();
}

void BackendBridge::toggleRightSignal()
{
    m_rightSignalOn = !m_rightSignalOn;
    m_leftSignalOn = false;
    m_hazardOn = false;
    qInfo() << "[HMI] right signal=" << m_rightSignalOn;
    emit controlsChanged();
}

void BackendBridge::toggleHazard()
{
    m_hazardOn = !m_hazardOn;
    m_leftSignalOn = false;
    m_rightSignalOn = false;
    qInfo() << "[HMI] hazard=" << m_hazardOn;
    emit controlsChanged();
}

void BackendBridge::setDriveMode(int mode)
{
    m_stateMachine.setDriveMode(qBound(0, mode, 2));
    qInfo() << "[HMI] drive mode=" << driveModeName();
    emit dataChanged();
}

void BackendBridge::setRegenLevel(int level)
{
    const int newLevel = qBound(0, level, 3);
    if (newLevel == m_regenLevel) {
        return;
    }
    m_regenLevel = newLevel;
    emit dataChanged();
}

void BackendBridge::changeBrightness(int amount)
{
    const int newBrightness = qBound(30, m_brightness + amount, 100);
    if (newBrightness == m_brightness) {
        return;
    }
    m_brightness = newBrightness;
    emit brightnessChanged();
}

void BackendBridge::resetTrip()
{
    if (!m_distanceConnected || !m_distance.speedFresh(m_tripClock.elapsed())
        || m_model.gear() != VehicleModel::Park
        || m_model.pedal() != 0 || m_model.speed() != 0) {
        qInfo() << "[HMI] Trip reset requires Park, zero pedal and zero speed";
        return;
    }
    m_distance.resetTrip();
    saveDistance();
    emit dataChanged();
}

void BackendBridge::acknowledgeWarning()
{
    m_stateMachine.clearDtc();
    emit dataChanged();
}

void BackendBridge::updateCalculatedValues()
{
    const qint64 nowMs = m_tripClock.elapsed();
    m_distance.update(nowMs, m_distanceConnected);

    int fullRangeKm = 385;
    if (m_model.driveMode() == VehicleModel::Eco) {
        fullRangeKm = 410;
    } else if (m_model.driveMode() == VehicleModel::Sport) {
        fullRangeKm = 340;
    }

    const int loadPenalty = motorOutput() / 5;
    m_dteKm = (m_model.soc() * fullRangeKm * (100 - loadPenalty)) / 10000;
    emit dataChanged();
}

void BackendBridge::setKeypadOnline(bool online)
{
    if (m_keypadOnline == online) {
        return;
    }
    m_keypadOnline = online;
    emit keypadOnlineChanged();
}
