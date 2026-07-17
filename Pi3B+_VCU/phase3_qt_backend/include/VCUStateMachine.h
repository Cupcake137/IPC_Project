#ifndef VCUSTATEMACHINE_H
#define VCUSTATEMACHINE_H

#include <QObject>
#include "DTCManager.h"
#include "Protocol.h"
#include "VehicleModel.h"

class VCUStateMachine : public QObject {
    Q_OBJECT

public:
    explicit VCUStateMachine(VehicleModel* model, DTCManager* dtcManager,
                             QObject* parent = nullptr);

    Q_INVOKABLE bool clearDtc();
    Q_INVOKABLE void setCharging(bool charging);

public slots:
    void processFrame(IpcProtocol::Frame frame);
    void setTransportOpen(bool open);
    void setCommunicationHealthy(bool healthy);

signals:
    void motorPwmChanged(int pwm);

private:
    int calculateAuthorizedPwm() const;
    void updateVehicleState();
    void updateAuthorizedPwm();

    VehicleModel* m_model = nullptr;
    DTCManager* m_dtcManager = nullptr;
    bool m_transportOpen = false;
    bool m_communicationHealthy = false;
    bool m_telemetryReceived = false;
    bool m_charging = false;
};

#endif
