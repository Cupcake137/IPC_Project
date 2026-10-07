#include "SimulationManager.h"
#include "CanDatabase.h"
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
    m_lastCharging = false;
    m_clearRequested = false;
    m_driveCounter = 0;
    m_energyCounter = 0;
    m_diagCounter = 0;
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

    CanDatabase::DriveStatus driveStatus;
    driveStatus.pedal = static_cast<quint8>(pedal);
    driveStatus.speed = static_cast<quint8>(speed);
    driveStatus.gear = static_cast<quint8>(gear);
    driveStatus.appliedPwm = static_cast<quint8>(m_authorizedPwm);
    driveStatus.aliveCounter = m_driveCounter;
    IpcProtocol::Frame frame;
    if (CanDatabase::packDriveStatus(driveStatus, &frame)) {
        emit frameGenerated(frame);
        m_driveCounter = static_cast<quint8>((m_driveCounter + 1) & 0x0F);
    }

    if (charging != m_lastCharging) {
        m_lastCharging = charging;
        emit chargingChanged(charging);
    }

    if (m_elapsedMs - m_lastSlowTelemetryMs >= 1000) {
        m_lastSlowTelemetryMs = m_elapsedMs;

        CanDatabase::EnergyStatus energyStatus;
        energyStatus.soc = static_cast<quint8>(soc);
        energyStatus.aliveCounter = m_energyCounter;
        if (CanDatabase::packEnergyStatus(energyStatus, &frame)) {
            emit frameGenerated(frame);
            m_energyCounter = static_cast<quint8>((m_energyCounter + 1) & 0x0F);
        }

        CanDatabase::DiagnosticStatus diagnosticStatus;
        diagnosticStatus.dtc = static_cast<quint8>(dtc);
        diagnosticStatus.faultFlags = dtc == 0 ? 0 : 1;
        diagnosticStatus.aliveCounter = m_diagCounter;
        if (CanDatabase::packDiagnosticStatus(diagnosticStatus, &frame)) {
            emit frameGenerated(frame);
            m_diagCounter = static_cast<quint8>((m_diagCounter + 1) & 0x0F);
        }
    }

    if (scenarioMs >= 17500 && !m_clearRequested) {
        m_clearRequested = true;
        emit clearDtcRequested();
    }

    m_elapsedMs += TickMs;
}
