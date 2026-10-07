#include <QCoreApplication>
#include <QDebug>

#include "CanDatabase.h"
#include "DTCManager.h"
#include "VCUStateMachine.h"
#include "VehicleModel.h"

namespace {
bool sendDrive(VCUStateMachine& vcu, int pedal, int counter)
{
    CanDatabase::DriveStatus value;
    value.pedal = static_cast<quint8>(pedal);
    value.speed = 20;
    value.gear = VehicleModel::Drive;
    value.aliveCounter = static_cast<quint8>(counter);

    IpcProtocol::Frame frame;
    if (!CanDatabase::packDriveStatus(value, &frame)) {
        return false;
    }
    vcu.processFrame(frame);
    return true;
}

bool sendEnergy(VCUStateMachine& vcu, int counter)
{
    CanDatabase::EnergyStatus value;
    value.soc = 50;
    value.aliveCounter = static_cast<quint8>(counter);

    IpcProtocol::Frame frame;
    if (!CanDatabase::packEnergyStatus(value, &frame)) {
        return false;
    }
    vcu.processFrame(frame);
    return true;
}

bool sendDtc(VCUStateMachine& vcu, int code, int counter)
{
    CanDatabase::DiagnosticStatus value;
    value.dtc = static_cast<quint8>(code);
    value.faultFlags = code == 0 ? 0 : 1;
    value.aliveCounter = static_cast<quint8>(counter);

    IpcProtocol::Frame frame;
    if (!CanDatabase::packDiagnosticStatus(value, &frame)) {
        return false;
    }
    vcu.processFrame(frame);
    return true;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    VehicleModel model;
    DTCManager dtc;
    VCUStateMachine vcu(model, dtc);
    vcu.setTransportOpen(true);
    vcu.setCommunicationHealthy(true);

    if (!sendDrive(vcu, 50, 0) || !sendEnergy(vcu, 0)
        || !sendDtc(vcu, 0x11, 0)) {
        return 1;
    }
    if (dtc.isCritical(0x11) || model.state() != VehicleModel::Ready
        || model.authorizedPwm() <= 0 || model.authorizedPwm() > 80) {
        qCritical() << "Non-critical warning behavior failed";
        return 2;
    }

    if (!sendDtc(vcu, 0x22, 1)) {
        return 3;
    }
    if (!dtc.isCritical(0x22) || model.state() != VehicleModel::Fault
        || model.authorizedPwm() != 0) {
        qCritical() << "Critical fault behavior failed";
        return 4;
    }

    qInfo() << "IPC cluster warning-level tests passed";
    return 0;
}
