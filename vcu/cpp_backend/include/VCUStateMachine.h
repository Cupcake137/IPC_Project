#ifndef VCUSTATEMACHINE_H
#define VCUSTATEMACHINE_H

#include "DTCManager.h"
#include "Protocol.h"
#include "VehicleModel.h"

class VCUStateMachine {
public:
    VCUStateMachine(VehicleModel& model, DTCManager& dtcManager);

    bool clearDtc();
    void setCharging(bool charging);
    bool processFrame(IpcProtocol::Frame frame);
    void setTransportOpen(bool open);
    void setCommunicationHealthy(bool healthy);
    void setDriveMode(int driveMode);

private:
    struct CounterState {
        quint8 lastValue = 0;
        bool received = false;
    };

    int calculateAuthorizedPwm() const;
    bool acceptCounter(quint8 value, CounterState& state);
    bool processDriveFrame(const IpcProtocol::Frame& frame);
    bool processEnergyFrame(const IpcProtocol::Frame& frame);
    bool processDiagnosticFrame(const IpcProtocol::Frame& frame);
    void copyDtcToVehicle();
    void updateVehicleState();
    void updateAuthorizedPwm();

    VehicleModel& m_model;
    DTCManager& m_dtcManager;
    bool m_transportOpen = false;
    bool m_communicationHealthy = false;
    bool m_telemetryReceived = false;
    bool m_charging = false;
    CounterState m_driveCounter;
    CounterState m_energyCounter;
    CounterState m_diagCounter;
};

#endif
