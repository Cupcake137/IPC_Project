#ifndef VEHICLEMODEL_H
#define VEHICLEMODEL_H

class VehicleModel {

public:
    enum Gear { Park = 0, Reverse = 1, Neutral = 2, Drive = 3 };
    enum VehicleState { Off = 0, Acc = 1, Ready = 2, Charging = 3, Fault = 4 };
    enum DriveMode { Eco = 0, Normal = 1, Sport = 2 };

    int pedal() const { return m_pedal; }
    int speed() const { return m_speed; }
    int soc() const { return m_soc; }
    int gear() const { return m_gear; }
    int state() const { return m_state; }
    int driveMode() const { return m_driveMode; }
    int activeDtc() const { return m_activeDtc; }
    int authorizedPwm() const { return m_authorizedPwm; }
    void setPedal(int value);
    void setSpeed(int value);
    void setSoc(int value);
    void setGear(int value);
    void setState(int value);
    void setDriveMode(int value);
    void setActiveDtc(int value);
    void setAuthorizedPwm(int value);
private:
    int m_pedal = 0;
    int m_speed = 0;
    int m_soc = 100;
    int m_gear = Park;
    int m_state = Off;
    int m_driveMode = Normal;
    int m_activeDtc = 0;
    int m_authorizedPwm = 0;
};

#endif
