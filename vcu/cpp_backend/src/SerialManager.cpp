#include "SerialManager.h"
#include "CanDatabase.h"

SerialManager::SerialManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
            m_port.close();
            setCommunicationHealthy(false);
            setTransportOpen(false);
        }
        if (error != QSerialPort::NoError && error != QSerialPort::NotOpenError) {
            emit errorOccurred(m_port.errorString());
        }
    });

    m_watchdogTimer.setInterval(100);
    connect(&m_watchdogTimer, &QTimer::timeout, this, &SerialManager::onWatchdog);
    m_watchdogTimer.start();

    m_reconnectTimer.setInterval(1000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &SerialManager::onReconnect);

    m_commandHeartbeatTimer.setInterval(100);
    connect(&m_commandHeartbeatTimer, &QTimer::timeout, this, [this]() {
        writeMotorCommand(m_lastMotorCommand, m_lastGear);
    });
    m_commandHeartbeatTimer.start();
}

SerialManager::~SerialManager()
{
    close();
}

bool SerialManager::open(const QString& portName)
{
    close();
    m_portName = portName;
    m_reconnectTimer.start();
    return tryOpen();
}

void SerialManager::close()
{
    m_reconnectTimer.stop();
    m_lastMotorCommand = 0;
    if (m_port.isOpen()) {
        writeMotorCommand(0, m_lastGear);
        m_port.waitForBytesWritten(100);
        m_port.close();
    }
    setCommunicationHealthy(false);
    setTransportOpen(false);
}

bool SerialManager::tryOpen()
{
    if (m_portName.isEmpty()) {
        return false;
    }
    if (m_port.isOpen()) {
        return true;
    }

    m_port.setPortName(m_portName);
    m_port.setBaudRate(IpcProtocol::BaudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        emit errorOccurred(m_port.errorString());
        setTransportOpen(false);
        return false;
    }

    m_parser = IpcProtocol::FrameParser {};
    m_reportedChecksumErrors = 0;
    m_seenFrame = false;
    m_lastMotorCommand = 0;
    m_lastGear = 0;
    m_openElapsed.restart();
    setCommunicationHealthy(false);
    setTransportOpen(true);
    writeMotorCommand(0, m_lastGear);
    return true;
}

void SerialManager::sendMotorCommand(int pwm, int gear)
{
    m_lastMotorCommand = m_communicationHealthy ? qBound(0, pwm, 255) : 0;
    m_lastGear = qBound(0, gear, 3);
    writeMotorCommand(m_lastMotorCommand, m_lastGear);
}

void SerialManager::writeMotorCommand(int pwm, int gear)
{
    if (!m_port.isOpen()) {
        return;
    }

    CanDatabase::MotorCommand command;
    command.pwm = static_cast<quint8>(qBound(0, pwm, 255));
    command.gear = static_cast<quint8>(qBound(0, gear, 3));
    command.torqueEnable = command.pwm > 0;
    command.aliveCounter = m_motorCounter;

    IpcProtocol::Frame frame;
    if (CanDatabase::packMotorCommand(command, &frame)) {
        m_port.write(IpcProtocol::encode(frame));
        m_motorCounter = static_cast<quint8>((m_motorCounter + 1) & 0x0F);
    }
}

void SerialManager::onReadyRead()
{
    const QByteArray bytes = m_port.readAll();
    for (const char byte : bytes) {
        IpcProtocol::Frame frame;
        if (m_parser.feed(static_cast<quint8>(byte), &frame)) {
            // Only new, valid Drive telemetry can keep motor control alive.
            CanDatabase::DriveStatus drive;
            const bool freshDrive = CanDatabase::unpackDriveStatus(frame, &drive)
                && (!m_seenFrame || drive.aliveCounter != m_lastDriveCounter);
            if (freshDrive) {
                m_seenFrame = true;
                m_lastDriveCounter = drive.aliveCounter;
                m_lastFrameElapsed.restart();
            }
            emit frameReceived(frame);
            if (freshDrive) setCommunicationHealthy(true);
        }
    }

    const quint64 checksumErrors = m_parser.checksumErrors();
    if (checksumErrors > m_reportedChecksumErrors) {
        m_reportedChecksumErrors = checksumErrors;
        emit errorOccurred(QStringLiteral("invalid UART frame rejected by CRC16"));
    }
}

void SerialManager::onWatchdog()
{
    if (!m_port.isOpen()) {
        return;
    }

    if (m_seenFrame && m_lastFrameElapsed.isValid() && m_lastFrameElapsed.elapsed() > 500) {
        setCommunicationHealthy(false);
    } else if (!m_seenFrame && m_openElapsed.isValid() && m_openElapsed.elapsed() > 1000) {
        setCommunicationHealthy(false);
    }
}

void SerialManager::onReconnect()
{
    if (!m_port.isOpen()) {
        tryOpen();
    }
}

void SerialManager::setTransportOpen(bool open)
{
    if (m_transportOpen == open) {
        return;
    }
    m_transportOpen = open;
    emit transportStateChanged(open);
}

void SerialManager::setCommunicationHealthy(bool healthy)
{
    if (m_communicationHealthy == healthy) {
        return;
    }
    m_communicationHealthy = healthy;
    if (!healthy) {
        m_lastMotorCommand = 0;
        writeMotorCommand(0, m_lastGear);
    }
    emit communicationStateChanged(healthy);
}
