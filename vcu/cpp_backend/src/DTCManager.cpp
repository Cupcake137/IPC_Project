#include "DTCManager.h"
#include <QtGlobal>

QString DTCManager::messageForCode(int code) const
{
    switch (code) {
    case 0x11:
        return QStringLiteral("P0A80 - Battery pack low voltage");
    case 0x22:
        return QStringLiteral("P1A10 - Motor current overload");
    case 0xE1:
        return QStringLiteral("U0100 - Lost communication with ECU");
    case 0xE2:
        return QStringLiteral("U0101 - ECU motor-command timeout");
    default:
        return code == 0 ? QString() : QStringLiteral("Unknown DTC 0x%1").arg(code, 2, 16, QLatin1Char('0')).toUpper();
    }
}

bool DTCManager::isCritical(int code) const
{
    return code != 0 && code != 0x11;
}

int DTCManager::rawCode() const
{
    return m_communicationFault ? 0xE1 : m_ecuCode;
}

void DTCManager::reportEcuCode(int code)
{
    m_ecuCode = qBound(0, code, 255);
    latchCurrentFault();
}

void DTCManager::setCommunicationFault(bool active)
{
    if (m_communicationFault == active) {
        return;
    }
    m_communicationFault = active;
    latchCurrentFault();
}

bool DTCManager::clearLatched()
{
    if (rawCode() != 0 || m_latchedCode == 0) {
        return false;
    }

    m_latchedCode = 0;
    return true;
}

void DTCManager::latchCurrentFault()
{
    const int fault = rawCode();
    // A warning must not replace a critical fault waiting for a safe clear.
    if (fault != 0 && (!isCritical(m_latchedCode) || isCritical(fault))) {
        m_latchedCode = fault;
    }
}
