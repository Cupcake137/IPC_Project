#ifndef VEHICLEMODEL_H
#define VEHICLEMODEL_H

#include <QObject>

class VehicleModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int pedal READ pedal NOTIFY changed)
    Q_PROPERTY(int speed READ speed NOTIFY changed)
    Q_PROPERTY(int soc READ soc NOTIFY changed)
    Q_PROPERTY(int gear READ gear NOTIFY changed)
    Q_PROPERTY(int state READ state NOTIFY changed)
    Q_PROPERTY(int driveMode READ driveMode WRITE setDriveMode NOTIFY changed)
    Q_PROPERTY(int activeDtc READ activeDtc NOTIFY changed)
    Q_PROPERTY(int authorizedPwm READ authorizedPwm NOTIFY changed)
    Q_PROPERTY(bool transportOpen READ transportOpen NOTIFY changed)
    Q_PROPERTY(bool communicationHealthy READ communicationHealthy NOTIFY changed)
    Q_PROPERTY(quint64 receivedFrames READ receivedFrames NOTIFY changed)
    Q_PROPERTY(quint64 droppedFrames READ droppedFrames NOTIFY changed)
    Q_PROPERTY(quint64 duplicateFrames READ duplicateFrames NOTIFY changed)
    Q_PROPERTY(quint64 checksumErrors READ checksumErrors NOTIFY changed)

public:
    enum Gear { Park = 0, Reverse = 1, Neutral = 2, Drive = 3 };
    enum VehicleState { Off = 0, Acc = 1, Ready = 2, Charging = 3, Fault = 4 };
    enum DriveMode { Eco = 0, Normal = 1, Sport = 2 };
    Q_ENUM(Gear)
    Q_ENUM(VehicleState)
    Q_ENUM(DriveMode)

    explicit VehicleModel(QObject* parent = nullptr);

    int pedal() const { return m_pedal; }
    int speed() const { return m_speed; }
    int soc() const { return m_soc; }
    int gear() const { return m_gear; }
    int state() const { return m_state; }
    int driveMode() const { return m_driveMode; }
    int activeDtc() const { return m_activeDtc; }
    int authorizedPwm() const { return m_authorizedPwm; }
    bool transportOpen() const { return m_transportOpen; }
    bool communicationHealthy() const { return m_communicationHealthy; }
    quint64 receivedFrames() const { return m_receivedFrames; }
    quint64 droppedFrames() const { return m_droppedFrames; }
    quint64 duplicateFrames() const { return m_duplicateFrames; }
    quint64 checksumErrors() const { return m_checksumErrors; }

    void setPedal(int value);
    void setSpeed(int value);
    void setSoc(int value);
    void setGear(int value);
    void setState(int value);
    void setDriveMode(int value);
    void setActiveDtc(int value);
    void setAuthorizedPwm(int value);
    void setTransportOpen(bool value);
    void setCommunicationHealthy(bool value);
    void setCommunicationStatistics(quint64 received, quint64 dropped,
                                    quint64 duplicates, quint64 checksumErrors);

signals:
    void changed();

private:
    bool setInt(int& target, int value);
    bool setBool(bool& target, bool value);

    int m_pedal = 0;
    int m_speed = 0;
    int m_soc = 100;
    int m_gear = Park;
    int m_state = Off;
    int m_driveMode = Normal;
    int m_activeDtc = 0;
    int m_authorizedPwm = 0;
    bool m_transportOpen = false;
    bool m_communicationHealthy = false;
    quint64 m_receivedFrames = 0;
    quint64 m_droppedFrames = 0;
    quint64 m_duplicateFrames = 0;
    quint64 m_checksumErrors = 0;
};

#endif
