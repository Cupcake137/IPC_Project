#ifndef CAN_DATABASE_H
#define CAN_DATABASE_H

#include "Protocol.h"

namespace CanDatabase {

enum CanId : quint16 {
    VcuMotorCommand = 0x080,
    EcuDriveStatus = 0x100,
    EcuEnergyStatus = 0x101,
    EcuDiagStatus = 0x102,
    SwcKeyEvent = 0x300,
    SwcNetworkStatus = 0x301
};

enum Gear : quint8 {
    Park = 0,
    Reverse = 1,
    Neutral = 2,
    Drive = 3
};

enum KeyEventType : quint8 {
    KeyPress = 0,
    KeyRelease = 1,
    KeyLongPress = 2
};

struct MotorCommand {
    quint8 pwm = 0;
    quint8 gear = Park;
    bool torqueEnable = false;
    quint8 aliveCounter = 0;
};

struct DriveStatus {
    quint8 pedal = 0;
    quint8 speed = 0;
    quint8 gear = Park;
    quint8 appliedPwm = 0;
    quint8 aliveCounter = 0;
};

struct EnergyStatus {
    quint8 soc = 0;
    quint8 aliveCounter = 0;
};

struct DiagnosticStatus {
    quint8 dtc = 0;
    quint8 faultFlags = 0;
    quint8 aliveCounter = 0;
};

struct KeyEvent {
    quint8 keyCode = 0;
    quint8 eventType = KeyPress;
    quint16 pressedKeys = 0;
    quint8 aliveCounter = 0;
};

struct NetworkStatus {
    bool wifiConnected = false;
    bool mqttConnected = false;
    bool keypadValid = false;
    qint8 wifiRssi = 0;
    quint32 uptimeSeconds = 0;
    quint8 aliveCounter = 0;
};

quint8 calculateCrc(const IpcProtocol::Frame& frame);
bool validate(const IpcProtocol::Frame& frame, quint16 id, quint8 dlc);

bool packMotorCommand(const MotorCommand& value, IpcProtocol::Frame* frame);
bool unpackMotorCommand(const IpcProtocol::Frame& frame, MotorCommand* value);
bool packDriveStatus(const DriveStatus& value, IpcProtocol::Frame* frame);
bool unpackDriveStatus(const IpcProtocol::Frame& frame, DriveStatus* value);
bool packEnergyStatus(const EnergyStatus& value, IpcProtocol::Frame* frame);
bool unpackEnergyStatus(const IpcProtocol::Frame& frame, EnergyStatus* value);
bool packDiagnosticStatus(const DiagnosticStatus& value, IpcProtocol::Frame* frame);
bool unpackDiagnosticStatus(const IpcProtocol::Frame& frame, DiagnosticStatus* value);
bool packKeyEvent(const KeyEvent& value, IpcProtocol::Frame* frame);
bool unpackKeyEvent(const IpcProtocol::Frame& frame, KeyEvent* value);
bool packNetworkStatus(const NetworkStatus& value, IpcProtocol::Frame* frame);
bool unpackNetworkStatus(const IpcProtocol::Frame& frame, NetworkStatus* value);

QByteArray toText(const IpcProtocol::Frame& frame);
bool fromText(const QByteArray& text, IpcProtocol::Frame* frame);

} // namespace CanDatabase

#endif
