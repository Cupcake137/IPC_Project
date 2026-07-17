#include "SimulationManager.h"
#include "VehicleModel.h"
#include <QtGlobal>

namespace {
constexpr int TickMs = 50;
constexpr int ScenarioDurationMs = 20000;
}

SimulationManager::SimulationManager(QObject* parent)
    : QObject(parent)
{
    m_timer.setInterval(TickMs);
    connect(&m_timer, &QTimer::timeout, this, &SimulationManager::advance);
}

void SimulationManager::start()
{
    m_elapsedMs = 0;
    m_lastSlowTelemetryMs = -1000;
    m_authorizedPwm = 0;
    m_lastGear = -1;
    m_lastCharging = false;
    m_clearRequested = false;
    m_counter = 0;
    advance();
    m_timer.start();
}

void SimulationManager::acceptMotorCommand(int pwm)
{
    m_authorizedPwm = qBound(0, pwm, 255);
}

void SimulationManager::advance()
{
    const int scenarioMs = m_elapsedMs % ScenarioDurationMs;

    int gear = VehicleModel::Park;
    int pedal = 0;
    int dtc = 0;
    bool charging = false;

    if (scenarioMs < TickMs) {
        m_clearRequested = false;
    }

    if (scenarioMs >= 2000 && scenarioMs < 6000) {
        gear = VehicleModel::Reverse;
        pedal = ((scenarioMs - 2000) * 40) / 4000;
    } else if (scenarioMs >= 6000 && scenarioMs < 8000) {
        gear = VehicleModel::Neutral;
    } else if (scenarioMs >= 8000 && scenarioMs < 17000) {
        gear = VehicleModel::Drive;
        if (scenarioMs < 14000) {
            pedal = ((scenarioMs - 8000) * 80) / 6000;
        } else if (scenarioMs < 17000) {
            pedal = 80;
            dtc = 0x22;
        }
    } else if (scenarioMs >= 18000) {
        gear = VehicleModel::Park;
        charging = true;
    }

    const int speed = (gear == VehicleModel::Drive || gear == VehicleModel::Reverse)
        ? (m_authorizedPwm * 120) / 255
        : 0;
    const int soc = qMax(0, 100 - (m_elapsedMs / 30000));

    emitFrame(IpcProtocol::PedalSpeed,
              {static_cast<quint8>(pedal), static_cast<quint8>(speed)});

    if (gear != m_lastGear) {
        m_lastGear = gear;
        emitFrame(IpcProtocol::GearState, {static_cast<quint8>(gear)});
    }

    if (charging != m_lastCharging) {
        m_lastCharging = charging;
        emit chargingChanged(charging);
    }

    if (m_elapsedMs - m_lastSlowTelemetryMs >= 1000) {
        m_lastSlowTelemetryMs = m_elapsedMs;
        emitFrame(IpcProtocol::GearState, {static_cast<quint8>(gear)});
        emitFrame(IpcProtocol::BatterySoc, {static_cast<quint8>(soc)});
        emitFrame(IpcProtocol::DtcStatus, {static_cast<quint8>(dtc)});
    }

    if (scenarioMs >= 17500 && !m_clearRequested) {
        m_clearRequested = true;
        emit clearDtcRequested();
    }

    m_elapsedMs += TickMs;
}

void SimulationManager::emitFrame(quint16 id, std::initializer_list<quint8> payload)
{
    IpcProtocol::Frame frame;
    frame.id = id;
    frame.dlc = static_cast<quint8>(qMin(payload.size(), static_cast<size_t>(IpcProtocol::MaxDlc)));
    frame.counter = m_counter;
    m_counter = static_cast<quint8>((m_counter + 1) & 0x0F);

    int index = 0;
    for (const quint8 byte : payload) {
        if (index >= frame.dlc) {
            break;
        }
        frame.data[static_cast<size_t>(index++)] = byte;
    }
    emit frameGenerated(frame);
}
