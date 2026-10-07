#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <QObject>
#include <QTimer>
#include "Protocol.h"

class SimulationManager : public QObject {
    Q_OBJECT

public:
    explicit SimulationManager(QObject* parent = nullptr);

    void start();

public slots:
    void acceptMotorCommand(int pwm);

signals:
    void frameGenerated(IpcProtocol::Frame frame);
    void chargingChanged(bool charging);
    void clearDtcRequested();

private slots:
    void advance();

private:
    QTimer m_timer;
    int m_elapsedMs = 0;
    int m_lastSlowTelemetryMs = -1000;
    int m_authorizedPwm = 0;
    bool m_lastCharging = false;
    bool m_clearRequested = false;
    quint8 m_driveCounter = 0;
    quint8 m_energyCounter = 0;
    quint8 m_diagCounter = 0;
};

#endif
