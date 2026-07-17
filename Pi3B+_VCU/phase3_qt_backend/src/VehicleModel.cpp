#include "VehicleModel.h"
#include <QtGlobal>

VehicleModel::VehicleModel(QObject* parent)
    : QObject(parent)
{
}

bool VehicleModel::setInt(int& target, int value)
{
    if (target == value) {
        return false;
    }
    target = value;
    emit changed();
    return true;
}

bool VehicleModel::setBool(bool& target, bool value)
{
    if (target == value) {
        return false;
    }
    target = value;
    emit changed();
    return true;
}

void VehicleModel::setPedal(int value) { setInt(m_pedal, qBound(0, value, 100)); }
void VehicleModel::setSpeed(int value) { setInt(m_speed, qBound(0, value, 255)); }
void VehicleModel::setSoc(int value) { setInt(m_soc, qBound(0, value, 100)); }
void VehicleModel::setGear(int value) { setInt(m_gear, qBound(0, value, 3)); }
void VehicleModel::setState(int value) { setInt(m_state, qBound(0, value, 4)); }
void VehicleModel::setDriveMode(int value) { setInt(m_driveMode, qBound(0, value, 2)); }
void VehicleModel::setActiveDtc(int value) { setInt(m_activeDtc, qBound(0, value, 255)); }
void VehicleModel::setAuthorizedPwm(int value) { setInt(m_authorizedPwm, qBound(0, value, 255)); }
void VehicleModel::setTransportOpen(bool value) { setBool(m_transportOpen, value); }
void VehicleModel::setCommunicationHealthy(bool value) { setBool(m_communicationHealthy, value); }

void VehicleModel::setCommunicationStatistics(quint64 received, quint64 dropped,
                                              quint64 duplicates, quint64 checksumErrors)
{
    if (m_receivedFrames == received && m_droppedFrames == dropped
        && m_duplicateFrames == duplicates && m_checksumErrors == checksumErrors) {
        return;
    }
    m_receivedFrames = received;
    m_droppedFrames = dropped;
    m_duplicateFrames = duplicates;
    m_checksumErrors = checksumErrors;
    emit changed();
}
