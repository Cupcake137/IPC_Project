#ifndef DTCMANAGER_H
#define DTCMANAGER_H

#include <QString>

class DTCManager {
public:
    int activeCode() const { return m_latchedCode; }
    int rawCode() const;

    QString messageForCode(int code) const;
    bool isCritical(int code) const;

    void reportEcuCode(int code);
    void setCommunicationFault(bool active);
    bool clearLatched();

private:
    void latchCurrentFault();

    int m_ecuCode = 0;
    int m_latchedCode = 0;
    bool m_communicationFault = false;
};

#endif
