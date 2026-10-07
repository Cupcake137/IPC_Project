#include "VCUStateMachine.h"
#include "CanDatabase.h"

namespace {
constexpr int SimulatedMaxSpeedKmh = 120;
constexpr int ReverseSpeedLimitKmh = 20;
constexpr int ReverseMaxPwm =
    (ReverseSpeedLimitKmh * 255 + SimulatedMaxSpeedKmh - 1) / SimulatedMaxSpeedKmh;
}

VCUStateMachine::VCUStateMachine(VehicleModel& model, DTCManager& dtcManager)
    : m_model(model)
    , m_dtcManager(dtcManager)
{
}

bool VCUStateMachine::processFrame(IpcProtocol::Frame frame)
{
    bool accepted = false;
    switch (frame.id) {
    case CanDatabase::EcuDriveStatus:
        accepted = processDriveFrame(frame);
        break;
    case CanDatabase::EcuEnergyStatus:
        accepted = processEnergyFrame(frame);
        break;
    case CanDatabase::EcuDiagStatus:
        accepted = processDiagnosticFrame(frame);
        break;
    default:
        return false;
    }

    if (!accepted) {
        return false;
    }
    m_telemetryReceived = true;
    updateVehicleState();
    updateAuthorizedPwm();
    return true;
}

bool VCUStateMachine::processDriveFrame(const IpcProtocol::Frame& frame)
{
    CanDatabase::DriveStatus status;
    if (!CanDatabase::unpackDriveStatus(frame, &status)
        || !acceptCounter(status.aliveCounter, m_driveCounter)) {
        return false;
    }

    m_model.setPedal(status.pedal);
    m_model.setSpeed(status.speed);
    m_model.setGear(status.gear);
    return true;
}

bool VCUStateMachine::processEnergyFrame(const IpcProtocol::Frame& frame)
{
    CanDatabase::EnergyStatus status;
    if (!CanDatabase::unpackEnergyStatus(frame, &status)
        || !acceptCounter(status.aliveCounter, m_energyCounter)) {
        return false;
    }

    m_model.setSoc(status.soc);
    return true;
}

bool VCUStateMachine::processDiagnosticFrame(const IpcProtocol::Frame& frame)
{
    CanDatabase::DiagnosticStatus status;
    if (!CanDatabase::unpackDiagnosticStatus(frame, &status)
        || !acceptCounter(status.aliveCounter, m_diagCounter)) {
        return false;
    }

    m_dtcManager.reportEcuCode(status.dtc);
    copyDtcToVehicle();
    return true;
}

bool VCUStateMachine::acceptCounter(quint8 value, CounterState& state)
{
    if (state.received && value == state.lastValue) {
        return false;
    }
    state.received = true;
    state.lastValue = value;
    return true;
}

void VCUStateMachine::setTransportOpen(bool open)
{
    m_transportOpen = open;
    if (!open) {
        m_communicationHealthy = false;
        if (m_telemetryReceived) {
            m_dtcManager.setCommunicationFault(true);
            copyDtcToVehicle();
        }
    }
    updateVehicleState();
    updateAuthorizedPwm();
}

void VCUStateMachine::setCommunicationHealthy(bool healthy)
{
    m_communicationHealthy = healthy;
    m_dtcManager.setCommunicationFault(!healthy && m_telemetryReceived);
    copyDtcToVehicle();
    updateVehicleState();
    updateAuthorizedPwm();
}

void VCUStateMachine::setCharging(bool charging)
{
    if (charging
        && (m_model.gear() != VehicleModel::Park || m_model.pedal() != 0)) {
        return;
    }
    m_charging = charging;
    updateVehicleState();
    updateAuthorizedPwm();
}

bool VCUStateMachine::clearDtc()
{
    const bool safeToClear = m_model.gear() == VehicleModel::Park
        && m_model.pedal() == 0
        && m_model.speed() == 0;
    if (!safeToClear || !m_dtcManager.clearLatched()) {
        return false;
    }
    copyDtcToVehicle();
    updateVehicleState();
    updateAuthorizedPwm();
    return true;
}

void VCUStateMachine::setDriveMode(int driveMode)
{
    m_model.setDriveMode(driveMode);
    updateAuthorizedPwm();
}

int VCUStateMachine::calculateAuthorizedPwm() const
{
    if (m_model.state() != VehicleModel::Ready || m_model.soc() == 0) {
        return 0;
    }

    if (m_model.gear() != VehicleModel::Drive && m_model.gear() != VehicleModel::Reverse) {
        return 0;
    }

    int pwm = (m_model.pedal() * 255) / 100;
    if (m_model.driveMode() == VehicleModel::Eco) {
        pwm = (pwm * 60) / 100;
    } else if (m_model.driveMode() == VehicleModel::Sport && m_model.pedal() > 0 && m_model.pedal() < 30) {
        pwm = (pwm * 140) / 100;
    }

    if (m_model.gear() == VehicleModel::Reverse && pwm > ReverseMaxPwm) {
        pwm = ReverseMaxPwm;
    }
    if (m_model.activeDtc() != 0 && pwm > 80) {
        pwm = 80;
    }
    return qBound(0, pwm, 255);
}

void VCUStateMachine::copyDtcToVehicle()
{
    m_model.setActiveDtc(m_dtcManager.activeCode());
}

void VCUStateMachine::updateVehicleState()
{
    if (m_model.soc() == 0 || m_dtcManager.isCritical(m_model.activeDtc())) {
        m_model.setState(VehicleModel::Fault);
    } else if (!m_transportOpen) {
        m_model.setState(VehicleModel::Off);
    } else if (m_charging) {
        m_model.setState(VehicleModel::Charging);
    } else if (!m_communicationHealthy || !m_telemetryReceived) {
        m_model.setState(VehicleModel::Acc);
    } else {
        m_model.setState(VehicleModel::Ready);
    }
}

void VCUStateMachine::updateAuthorizedPwm()
{
    const int pwm = calculateAuthorizedPwm();
    m_model.setAuthorizedPwm(pwm);
}
