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

    bool open(const QString& portName);
    void close();
    Q_INVOKABLE void sendMotorCommand(int pwm);

signals:
    void frameReceived(IpcProtocol::Frame frame);
    void errorOccurred(QString message);
    void transportStateChanged(bool open);
    void communicationStateChanged(bool healthy);
    void statisticsChanged(quint64 receivedFrames, quint64 droppedFrames,
                           quint64 duplicateFrames, quint64 checksumErrors);

private slots:
    void onReadyRead();
    void onWatchdog();
    void onReconnect();

private:
    bool tryOpen();
    void setTransportOpen(bool open);
    void setCommunicationHealthy(bool healthy);

    QSerialPort m_port;
    QString m_portName;
    IpcProtocol::FrameParser m_parser;
    QTimer m_watchdogTimer;
    QTimer m_reconnectTimer;
    QElapsedTimer m_openElapsed;
    QElapsedTimer m_lastFrameElapsed;
    quint8 m_txCounter = 0;
    quint8 m_lastRxCounter = 0;
    quint64 m_receivedFrames = 0;
    quint64 m_droppedFrames = 0;
    quint64 m_duplicateFrames = 0;
    quint64 m_lastReportedChecksumErrors = 0;
    bool m_transportOpen = false;
    bool m_communicationHealthy = false;
    bool m_seenFrame = false;
    bool m_haveRxCounter = false;
};

#endif
