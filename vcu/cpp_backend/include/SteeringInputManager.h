#ifndef STEERINGINPUTMANAGER_H
#define STEERINGINPUTMANAGER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include "CanDatabase.h"

#ifdef IPC_HAS_MOSQUITTO
struct mosquitto;
struct mosquitto_message;
#endif

class SteeringInputManager : public QObject {
    Q_OBJECT

public:
    explicit SteeringInputManager(QObject* parent = nullptr);
    ~SteeringInputManager() override;

    void start(const QString& host, quint16 port,
               const QString& username, const QString& password);
    void stop();

    bool ingestCanPayload(const QByteArray& payload);

    static bool parseKeyEvent(const QByteArray& payload,
                              CanDatabase::KeyEvent* outputEvent);
    static QString eventTypeName(int eventType);
    static QString keyName(quint8 keyCode);

signals:
    void brokerConnectedChanged(bool connected);
    void controllerOnlineChanged(bool online);
    void keyEventReceived(int eventType, const QString& key, int aliveCounter);
    void errorOccurred(const QString& message);

private:
    void setBrokerConnected(bool connected);
    void setControllerOnline(bool online);

#ifdef IPC_HAS_MOSQUITTO
    static void mqttConnectCallback(struct mosquitto* client, void* context, int result);
    static void mqttDisconnectCallback(struct mosquitto* client, void* context, int result);
    static void mqttMessageCallback(struct mosquitto* client, void* context,
                                    const struct mosquitto_message* message);

    struct mosquitto* m_client = nullptr;
    bool m_libraryInitialized = false;
    bool m_loopStarted = false;
#endif

    bool m_brokerConnected = false;
    bool m_controllerOnline = false;
    bool m_keyCounterSeen = false;
    quint8 m_lastKeyCounter = 0;
};

#endif
