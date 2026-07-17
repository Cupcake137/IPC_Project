#include "VCUStateMachine.h"

namespace {
constexpr int SimulatedMaxSpeedKmh = 120;
constexpr int ReverseSpeedLimitKmh = 20;
constexpr int ReverseMaxPwm =
    (ReverseSpeedLimitKmh * 255 + SimulatedMaxSpeedKmh - 1) / SimulatedMaxSpeedKmh;
}

VCUStateMachine::VCUStateMachine(VehicleModel* model, DTCManager* dtcManager, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_dtcManager(dtcManager)
{
    if (m_dtcManager != nullptr) {
        connect(m_dtcManager, &DTCManager::activeCodeChanged, this, [this](int code) {
            if (m_model != nullptr) {
                m_model->setActiveDtc(code);
                updateVehicleState();
                updateAuthorizedPwm();
            }
        });
    }
}

void VCUStateMachine::processFrame(IpcProtocol::Frame frame)
{
    if (m_model == nullptr) {
        return;
    }

    bool recognizedFrame = false;
    switch (frame.id) {
    case IpcProtocol::PedalSpeed:
        if (frame.dlc >= 2) {
            m_model->setPedal(frame.data[0]);
            m_model->setSpeed(frame.data[1]);
            recognizedFrame = true;
        }
        break;
    case IpcProtocol::BatterySoc:
        if (frame.dlc >= 1) {
            m_model->setSoc(frame.data[0]);
            recognizedFrame = true;
        }
        break;
    case IpcProtocol::GearState:
        if (frame.dlc >= 1) {
            recognizedFrame = true;
            if (m_model->pedal() <= 2) {
                m_model->setGear(frame.data[0]);
            }
        }
        break;
    case IpcProtocol::DtcStatus:
        if (frame.dlc >= 1 && m_dtcManager != nullptr) {
            m_dtcManager->reportEcuCode(frame.data[0]);
            recognizedFrame = true;
        }
        break;
    default:
        break;
    }

    if (recognizedFrame) {
        m_telemetryReceived = true;
    }
    updateVehicleState();
    updateAuthorizedPwm();
}

void VCUStateMachine::setTransportOpen(bool open)
{
    m_transportOpen = open;
    if (!open) {
        m_communicationHealthy = false;
        if (m_telemetryReceived && m_dtcManager != nullptr) {
            m_dtcManager->setCommunicationFault(true);
        }
    }
    updateVehicleState();
    updateAuthorizedPwm();
}

void VCUStateMachine::setCommunicationHealthy(bool healthy)
{
    m_communicationHealthy = healthy;
    if (m_dtcManager != nullptr) {
        m_dtcManager->setCommunicationFault(!healthy && m_telemetryReceived);
    }
    updateVehicleState();
    updateAuthorizedPwm();
}

void VCUStateMachine::setCharging(bool charging)
{
    if (charging && m_model != nullptr
        && (m_model->gear() != VehicleModel::Park || m_model->pedal() != 0)) {
        return;
    }
    m_charging = charging;
    updateVehicleState();
    updateAuthorizedPwm();
}

bool VCUStateMachine::clearDtc()
{
    if (m_model == nullptr || m_dtcManager == nullptr) {
        return false;
    }
    const bool safeToClear = m_model->gear() == VehicleModel::Park
        && m_model->pedal() == 0
        && m_model->speed() == 0;
    if (!safeToClear || !m_dtcManager->clearLatched()) {
        return false;
    }
    updateVehicleState();
    updateAuthorizedPwm();
    return true;
}

int VCUStateMachine::calculateAuthorizedPwm() const
{
    if (m_model == nullptr || m_model->state() != VehicleModel::Ready || m_model->soc() == 0) {
        return 0;
    }

    if (m_model->gear() != VehicleModel::Drive && m_model->gear() != VehicleModel::Reverse) {
        return 0;
    }

    int pwm = (m_model->pedal() * 255) / 100;
    if (m_model->driveMode() == VehicleModel::Eco) {
        pwm = (pwm * 60) / 100;
    } else if (m_model->driveMode() == VehicleModel::Sport && m_model->pedal() > 0 && m_model->pedal() < 30) {
        pwm = (pwm * 140) / 100;
    }

    if (m_model->gear() == VehicleModel::Reverse && pwm > ReverseMaxPwm) {
        pwm = ReverseMaxPwm;
    }
    if (m_model->activeDtc() != 0 && pwm > 80) {
        pwm = 80;
    }
    return qBound(0, pwm, 255);
}

void VCUStateMachine::updateVehicleState()
{
    if (m_model == nullptr) {
        return;
    }

    if (m_model->soc() == 0 || m_model->activeDtc() != 0) {
        m_model->setState(VehicleModel::Fault);
    } else if (!m_transportOpen) {
        m_model->setState(VehicleModel::Off);
    } else if (m_charging) {
        m_model->setState(VehicleModel::Charging);
    } else if (!m_communicationHealthy || !m_telemetryReceived) {
        m_model->setState(VehicleModel::Acc);
    } else {
        m_model->setState(VehicleModel::Ready);
    }
}

void VCUStateMachine::updateAuthorizedPwm()
{
    if (m_model == nullptr) {
        return;
    }
    const int pwm = calculateAuthorizedPwm();
    m_model->setAuthorizedPwm(pwm);
    emit motorPwmChanged(pwm);
}
