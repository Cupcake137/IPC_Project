#include "DTCManager.h"
#include <QDateTime>

DTCManager::DTCManager(QObject* parent)
    : QObject(parent)
{
}

QString DTCManager::messageForCode(int code) const
{
    switch (code) {
    case 0x11:
        return QStringLiteral("P0A80 - Battery pack low voltage");
    case 0x22:
        return QStringLiteral("P1A10 - Motor current overload");
    case 0xE1:
        return QStringLiteral("U0100 - Lost communication with ECU");
    default:
        return code == 0 ? QString() : QStringLiteral("Unknown DTC 0x%1").arg(code, 2, 16, QLatin1Char('0')).toUpper();
    }
}

bool DTCManager::isCritical(int code) const
{
    return code == 0x11 || code == 0x22 || code == 0xE1;
}

void DTCManager::reportEcuCode(int code)
{
    m_ecuCode = qBound(0, code, 255);
    recompute();
}

void DTCManager::setCommunicationFault(bool active)
{
    if (m_communicationFault == active) {
        return;
    }
    m_communicationFault = active;
    recompute();
}

bool DTCManager::clearLatched()
{
    if (m_rawCode != 0 || m_latchedCode == 0) {
        return false;
    }

    const int clearedCode = m_latchedCode;
    m_latchedCode = 0;
    appendHistory(clearedCode, QStringLiteral("CLEARED"));
    recompute();
    return true;
}

void DTCManager::recompute()
{
    const int newRawCode = m_communicationFault ? 0xE1 : m_ecuCode;
    if (newRawCode != m_rawCode) {
        m_rawCode = newRawCode;
        emit rawCodeChanged(m_rawCode);
    }

    if (newRawCode != 0 && newRawCode != m_latchedCode) {
        m_latchedCode = newRawCode;
        appendHistory(newRawCode, QStringLiteral("ACTIVE"));
    }

    const int newActiveCode = m_latchedCode;
    if (newActiveCode != m_activeCode) {
        m_activeCode = newActiveCode;
        emit activeCodeChanged(m_activeCode);
    }
}

void DTCManager::appendHistory(int code, const QString& action)
{
    const QString entry = QStringLiteral("%1 | %2 | 0x%3 | %4")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate), action)
        .arg(code, 2, 16, QLatin1Char('0'))
        .arg(messageForCode(code));
    m_history.prepend(entry);
    while (m_history.size() > 20) {
        m_history.removeLast();
    }
    emit historyChanged();
}
