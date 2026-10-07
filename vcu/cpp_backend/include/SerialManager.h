#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QElapsedTimer>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include "Protocol.h"

class SerialManager : public QObject {
    Q_OBJECT

public:
    explicit SerialManager(QObject* parent = nullptr);
    ~SerialManager() override;

    bool open(const QString& portName);
    void close();
    void sendMotorCommand(int pwm, int gear);

signals:
    void frameReceived(IpcProtocol::Frame frame);
    void errorOccurred(QString message);
    void transportStateChanged(bool open);
    void communicationStateChanged(bool healthy);

private slots:
    void onReadyRead();
    void onWatchdog();
    void onReconnect();

private:
    bool tryOpen();
    void writeMotorCommand(int pwm, int gear);
    void setTransportOpen(bool open);
    void setCommunicationHealthy(bool healthy);

    QSerialPort m_port;
    QString m_portName;
    IpcProtocol::FrameParser m_parser;
    QTimer m_watchdogTimer;
    QTimer m_reconnectTimer;
    QTimer m_commandHeartbeatTimer;
    QElapsedTimer m_openElapsed;
    QElapsedTimer m_lastFrameElapsed;
    quint8 m_motorCounter = 0;
    quint64 m_reportedChecksumErrors = 0;
    int m_lastMotorCommand = 0;
    int m_lastGear = 0;
    bool m_transportOpen = false;
    bool m_communicationHealthy = false;
    bool m_seenFrame = false;
    quint8 m_lastDriveCounter = 0;
};

#endif
