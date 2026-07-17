#include "SerialManager.h"

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
    if (m_port.isOpen()) {
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
    m_seenFrame = false;
    m_haveRxCounter = false;
    m_openElapsed.restart();
    setCommunicationHealthy(false);
    setTransportOpen(true);
    return true;
}

void SerialManager::sendMotorCommand(int pwm)
{
    if (!m_port.isOpen()) {
        return;
    }

    std::array<quint8, IpcProtocol::MaxDlc> data {};
    data[0] = static_cast<quint8>(qBound(0, pwm, 255));
    const QByteArray frame = IpcProtocol::encode(IpcProtocol::MotorCommand, 1, data, m_txCounter);
    m_port.write(frame);
}

void SerialManager::onReadyRead()
{
    const QByteArray bytes = m_port.readAll();
    for (const char byte : bytes) {
        const auto parsed = m_parser.feed(static_cast<quint8>(byte));
        if (parsed.has_value()) {
            const IpcProtocol::Frame frame = parsed.value();
            ++m_receivedFrames;

            if (m_haveRxCounter) {
                const quint8 delta = static_cast<quint8>((frame.counter - m_lastRxCounter) & 0x0F);
                if (delta == 0) {
                    ++m_duplicateFrames;
                    emit statisticsChanged(m_receivedFrames, m_droppedFrames,
                                           m_duplicateFrames, m_parser.checksumErrors());
                    continue;
                }
                if (delta > 1) {
                    m_droppedFrames += delta - 1;
                }
            }

            m_lastRxCounter = frame.counter;
            m_haveRxCounter = true;
            m_seenFrame = true;
            m_lastFrameElapsed.restart();
            setCommunicationHealthy(true);
            emit frameReceived(frame);
        }
    }

    if (m_parser.checksumErrors() != m_lastReportedChecksumErrors) {
        m_lastReportedChecksumErrors = m_parser.checksumErrors();
    }
    emit statisticsChanged(m_receivedFrames, m_droppedFrames,
                           m_duplicateFrames, m_parser.checksumErrors());
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
    emit communicationStateChanged(healthy);
}
