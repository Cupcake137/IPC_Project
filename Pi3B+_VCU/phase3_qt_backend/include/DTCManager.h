#ifndef DTCMANAGER_H
#define DTCMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

class DTCManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int activeCode READ activeCode NOTIFY activeCodeChanged)
    Q_PROPERTY(int rawCode READ rawCode NOTIFY rawCodeChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)

public:
    explicit DTCManager(QObject* parent = nullptr);

    int activeCode() const { return m_activeCode; }
    int rawCode() const { return m_rawCode; }
    QStringList history() const { return m_history; }

    Q_INVOKABLE QString messageForCode(int code) const;
    Q_INVOKABLE bool isCritical(int code) const;

    void reportEcuCode(int code);
    void setCommunicationFault(bool active);
    bool clearLatched();

signals:
    void activeCodeChanged(int code);
    void rawCodeChanged(int code);
    void historyChanged();

private:
    void recompute();
    void appendHistory(int code, const QString& action);

    int m_ecuCode = 0;
    int m_rawCode = 0;
    int m_latchedCode = 0;
    int m_activeCode = 0;
    bool m_communicationFault = false;
    QStringList m_history;
};

#endif
