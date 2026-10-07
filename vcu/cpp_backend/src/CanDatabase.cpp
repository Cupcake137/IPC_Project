#include "CanDatabase.h"

namespace CanDatabase {

static quint8 updateCrc(quint8 crc, quint8 value)
{
    crc ^= value;
    for (quint8 bit = 0; bit < 8; ++bit) {
        if ((crc & 0x80) != 0) {
            crc = static_cast<quint8>((crc << 1) ^ 0x1D);
        } else {
            crc = static_cast<quint8>(crc << 1);
        }
    }
    return crc;
}

static bool beginFrame(IpcProtocol::Frame* frame, quint16 id, quint8 dlc)
{
    if (frame == nullptr || dlc == 0 || dlc > IpcProtocol::MaxDlc) {
        return false;
    }
    *frame = IpcProtocol::Frame();
    frame->id = id;
    frame->dlc = dlc;
    return true;
}

static void finishFrame(IpcProtocol::Frame* frame)
{
    frame->data[frame->dlc - 1] = calculateCrc(*frame);
}

quint8 calculateCrc(const IpcProtocol::Frame& frame)
{
    if (frame.dlc == 0 || frame.dlc > IpcProtocol::MaxDlc) {
        return 0;
    }

    quint8 crc = 0xFF;
    crc = updateCrc(crc, static_cast<quint8>(frame.id >> 8));
    crc = updateCrc(crc, static_cast<quint8>(frame.id & 0xFF));
    crc = updateCrc(crc, frame.dlc);
    for (quint8 index = 0; index + 1 < frame.dlc; ++index) {
        crc = updateCrc(crc, frame.data[index]);
    }
    return static_cast<quint8>(crc ^ 0xFF);
}

bool validate(const IpcProtocol::Frame& frame, quint16 id, quint8 dlc)
{
    return dlc > 0
        && dlc <= IpcProtocol::MaxDlc
        && frame.id == id
        && frame.dlc == dlc
        && frame.data[dlc - 1] == calculateCrc(frame);
}

bool packMotorCommand(const MotorCommand& value, IpcProtocol::Frame* frame)
{
    if (value.gear > Drive || !beginFrame(frame, VcuMotorCommand, 4)) {
        return false;
    }
    frame->data[0] = value.pwm;
    frame->data[1] = static_cast<quint8>(
        value.gear | (value.torqueEnable ? 0x04 : 0x00));
    frame->data[2] = value.aliveCounter & 0x0F;
    finishFrame(frame);
    return true;
}

bool unpackMotorCommand(const IpcProtocol::Frame& frame, MotorCommand* value)
{
    if (value == nullptr || !validate(frame, VcuMotorCommand, 4)) {
        return false;
    }
    value->pwm = frame.data[0];
    value->gear = frame.data[1] & 0x03;
    value->torqueEnable = (frame.data[1] & 0x04) != 0;
    value->aliveCounter = frame.data[2] & 0x0F;
    return value->gear <= Drive;
}

bool packDriveStatus(const DriveStatus& value, IpcProtocol::Frame* frame)
{
    if (value.pedal > 100 || value.gear > Drive
        || !beginFrame(frame, EcuDriveStatus, 6)) {
        return false;
    }
    frame->data[0] = value.pedal;
    frame->data[1] = value.speed;
    frame->data[2] = value.gear;
    frame->data[3] = value.appliedPwm;
    frame->data[4] = value.aliveCounter & 0x0F;
    finishFrame(frame);
    return true;
}

bool unpackDriveStatus(const IpcProtocol::Frame& frame, DriveStatus* value)
{
    if (value == nullptr || !validate(frame, EcuDriveStatus, 6)
        || frame.data[0] > 100 || frame.data[2] > Drive) {
        return false;
    }
    value->pedal = frame.data[0];
    value->speed = frame.data[1];
    value->gear = frame.data[2];
    value->appliedPwm = frame.data[3];
    value->aliveCounter = frame.data[4] & 0x0F;
    return true;
}

bool packEnergyStatus(const EnergyStatus& value, IpcProtocol::Frame* frame)
{
    if (value.soc > 100 || !beginFrame(frame, EcuEnergyStatus, 3)) {
        return false;
    }
    frame->data[0] = value.soc;
    frame->data[1] = value.aliveCounter & 0x0F;
    finishFrame(frame);
    return true;
}

bool unpackEnergyStatus(const IpcProtocol::Frame& frame, EnergyStatus* value)
{
    if (value == nullptr || !validate(frame, EcuEnergyStatus, 3)
        || frame.data[0] > 100) {
        return false;
    }
    value->soc = frame.data[0];
    value->aliveCounter = frame.data[1] & 0x0F;
    return true;
}

bool packDiagnosticStatus(const DiagnosticStatus& value, IpcProtocol::Frame* frame)
{
    if (!beginFrame(frame, EcuDiagStatus, 4)) {
        return false;
    }
    frame->data[0] = value.dtc;
    frame->data[1] = value.faultFlags & 0x0F;
    frame->data[2] = value.aliveCounter & 0x0F;
    finishFrame(frame);
    return true;
}

bool unpackDiagnosticStatus(const IpcProtocol::Frame& frame, DiagnosticStatus* value)
{
    if (value == nullptr || !validate(frame, EcuDiagStatus, 4)) {
        return false;
    }
    value->dtc = frame.data[0];
    value->faultFlags = frame.data[1] & 0x0F;
    value->aliveCounter = frame.data[2] & 0x0F;
    return true;
}

bool packKeyEvent(const KeyEvent& value, IpcProtocol::Frame* frame)
{
    if (value.keyCode > 15 || value.eventType > KeyLongPress
        || !beginFrame(frame, SwcKeyEvent, 6)) {
        return false;
    }
    frame->data[0] = value.keyCode;
    frame->data[1] = value.eventType;
    frame->data[2] = static_cast<quint8>(value.pressedKeys & 0xFF);
    frame->data[3] = static_cast<quint8>(value.pressedKeys >> 8);
    frame->data[4] = value.aliveCounter & 0x0F;
    finishFrame(frame);
    return true;
}

bool unpackKeyEvent(const IpcProtocol::Frame& frame, KeyEvent* value)
{
    if (value == nullptr || !validate(frame, SwcKeyEvent, 6)
        || frame.data[0] > 15 || frame.data[1] > KeyLongPress) {
        return false;
    }
    value->keyCode = frame.data[0];
    value->eventType = frame.data[1];
    value->pressedKeys = static_cast<quint16>(
        frame.data[2] | (static_cast<quint16>(frame.data[3]) << 8));
    value->aliveCounter = frame.data[4] & 0x0F;
    return true;
}

bool packNetworkStatus(const NetworkStatus& value, IpcProtocol::Frame* frame)
{
    if (!beginFrame(frame, SwcNetworkStatus, 8)) {
        return false;
    }
    frame->data[0] = static_cast<quint8>(
        (value.wifiConnected ? 0x01 : 0)
        | (value.mqttConnected ? 0x02 : 0)
        | (value.keypadValid ? 0x04 : 0));
    frame->data[1] = static_cast<quint8>(value.wifiRssi);
    frame->data[2] = static_cast<quint8>(value.uptimeSeconds);
    frame->data[3] = static_cast<quint8>(value.uptimeSeconds >> 8);
    frame->data[4] = static_cast<quint8>(value.uptimeSeconds >> 16);
    frame->data[5] = static_cast<quint8>(value.uptimeSeconds >> 24);
    frame->data[6] = value.aliveCounter & 0x0F;
    finishFrame(frame);
    return true;
}

bool unpackNetworkStatus(const IpcProtocol::Frame& frame, NetworkStatus* value)
{
    if (value == nullptr || !validate(frame, SwcNetworkStatus, 8)) {
        return false;
    }
    value->wifiConnected = (frame.data[0] & 0x01) != 0;
    value->mqttConnected = (frame.data[0] & 0x02) != 0;
    value->keypadValid = (frame.data[0] & 0x04) != 0;
    value->wifiRssi = static_cast<qint8>(frame.data[1]);
    value->uptimeSeconds = frame.data[2]
        | (static_cast<quint32>(frame.data[3]) << 8)
        | (static_cast<quint32>(frame.data[4]) << 16)
        | (static_cast<quint32>(frame.data[5]) << 24);
    value->aliveCounter = frame.data[6] & 0x0F;
    return true;
}

QByteArray toText(const IpcProtocol::Frame& frame)
{
    QByteArray text = QByteArray::number(frame.id, 16).rightJustified(3, '0').toUpper();
    text.append('#');
    for (quint8 index = 0; index < frame.dlc; ++index) {
        text.append(QByteArray::number(frame.data[index], 16).rightJustified(2, '0').toUpper());
    }
    return text;
}

bool fromText(const QByteArray& input, IpcProtocol::Frame* frame)
{
    if (frame == nullptr) {
        return false;
    }

    const QByteArray text = input.trimmed();
    const int separator = text.indexOf('#');
    if (separator <= 0) {
        return false;
    }

    bool idOk = false;
    const uint id = text.left(separator).toUInt(&idOk, 16);
    const QByteArray dataText = text.mid(separator + 1);
    if (!idOk || id > 0x7FF || dataText.size() % 2 != 0
        || dataText.size() > IpcProtocol::MaxDlc * 2) {
        return false;
    }

    IpcProtocol::Frame parsed;
    parsed.id = static_cast<quint16>(id);
    parsed.dlc = static_cast<quint8>(dataText.size() / 2);
    for (quint8 index = 0; index < parsed.dlc; ++index) {
        bool byteOk = false;
        parsed.data[index] = dataText.mid(index * 2, 2).toUInt(&byteOk, 16);
        if (!byteOk) {
            return false;
        }
    }
    *frame = parsed;
    return true;
}

} // namespace CanDatabase
