#include "VehicleModel.h"
#include <QtGlobal>

void VehicleModel::setPedal(int value)
{
    m_pedal = qBound(0, value, 100);
}
void VehicleModel::setSpeed(int value) { m_speed = qBound(0, value, 255); }
void VehicleModel::setSoc(int value)
{
    m_soc = qBound(0, value, 100);
}
void VehicleModel::setGear(int value) { m_gear = qBound(0, value, 3); }
void VehicleModel::setState(int value) { m_state = qBound(0, value, 4); }
void VehicleModel::setDriveMode(int value)
{
    m_driveMode = qBound(0, value, 2);
}
void VehicleModel::setActiveDtc(int value) { m_activeDtc = qBound(0, value, 255); }
void VehicleModel::setAuthorizedPwm(int value) { m_authorizedPwm = qBound(0, value, 255); }
