#include "SteeringInputManager.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QtGlobal>

#ifdef IPC_HAS_MOSQUITTO
#include <mosquitto.h>
#endif

namespace {
constexpr auto CanTopic = "ipc/can/swc";
constexpr auto StatusTopic = "ipc/swc/status";
}

SteeringInputManager::SteeringInputManager(QObject* parent)
    : QObject(parent)
{
}

SteeringInputManager::~SteeringInputManager()
{
    stop();
}

bool SteeringInputManager::parseKeyEvent(const QByteArray& payload,
                                         CanDatabase::KeyEvent* outputEvent)
{
    IpcProtocol::Frame frame;
    return CanDatabase::fromText(payload, &frame)
        && CanDatabase::unpackKeyEvent(frame, outputEvent);
}

QString SteeringInputManager::eventTypeName(int eventType)
{
    switch (eventType) {
    case CanDatabase::KeyRelease: return QStringLiteral("RELEASE");
    case CanDatabase::KeyLongPress: return QStringLiteral("LONG_PRESS");
    default: return QStringLiteral("PRESS");
    }
}

QString SteeringInputManager::keyName(quint8 keyCode)
{
    const QString keys = QStringLiteral("123A456B789C*0#D");
    if (keyCode >= keys.size()) {
        return QString();
    }
    return QString(keys.at(keyCode));
}

bool SteeringInputManager::ingestCanPayload(const QByteArray& payload)
{
    IpcProtocol::Frame frame;
    if (!CanDatabase::fromText(payload, &frame)) {
        emit errorOccurred(QStringLiteral("invalid MQTT CAN text"));
        return false;
    }

    if (frame.id == CanDatabase::SwcNetworkStatus) {
        CanDatabase::NetworkStatus status;
        if (!CanDatabase::unpackNetworkStatus(frame, &status)) {
            emit errorOccurred(QStringLiteral("invalid SWC network frame"));
            return false;
        }
        setControllerOnline(status.wifiConnected
                            && status.mqttConnected
                            && status.keypadValid);
        return true;
    }

    CanDatabase::KeyEvent event;
    if (!CanDatabase::unpackKeyEvent(frame, &event)) {
        emit errorOccurred(QStringLiteral("invalid SWC key frame"));
        return false;
    }
    if (m_keyCounterSeen && event.aliveCounter == m_lastKeyCounter) {
        emit errorOccurred(QStringLiteral("duplicate SWC key frame"));
        return false;
    }

    m_keyCounterSeen = true;
    m_lastKeyCounter = event.aliveCounter;
    emit keyEventReceived(event.eventType, keyName(event.keyCode),
                          event.aliveCounter);
    return true;
}

void SteeringInputManager::setBrokerConnected(bool connected)
{
    if (m_brokerConnected == connected) {
        return;
    }
    m_brokerConnected = connected;
    emit brokerConnectedChanged(connected);
    if (!connected) {
        setControllerOnline(false);
    }
}

void SteeringInputManager::setControllerOnline(bool online)
{
    if (m_controllerOnline == online) {
        return;
    }
    m_controllerOnline = online;
    emit controllerOnlineChanged(online);
}

void SteeringInputManager::start(const QString& host, quint16 port,
                                 const QString& username, const QString& password)
{
#ifdef IPC_HAS_MOSQUITTO
    stop();
    if (host.isEmpty() || port == 0 || username.isEmpty() || password.isEmpty()) {
        emit errorOccurred(QStringLiteral("incomplete MQTT configuration"));
        return;
    }

    if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS) {
        emit errorOccurred(QStringLiteral("could not initialize libmosquitto"));
        return;
    }
    m_libraryInitialized = true;

    const QByteArray clientId = QStringLiteral("ipc-vcu-%1")
        .arg(QCoreApplication::applicationPid()).toUtf8();
    m_client = mosquitto_new(clientId.constData(), true, this);
    if (!m_client) {
        emit errorOccurred(QStringLiteral("could not create MQTT client"));
        stop();
        return;
    }

    mosquitto_connect_callback_set(m_client, &SteeringInputManager::mqttConnectCallback);
    mosquitto_disconnect_callback_set(m_client, &SteeringInputManager::mqttDisconnectCallback);
    mosquitto_message_callback_set(m_client, &SteeringInputManager::mqttMessageCallback);
    mosquitto_reconnect_delay_set(m_client, 1, 10, true);

    QByteArray userBytes = username.toUtf8();
    QByteArray passwordBytes = password.toUtf8();
    int result = mosquitto_username_pw_set(
        m_client, userBytes.constData(), passwordBytes.constData());
    if (result == MOSQ_ERR_SUCCESS) {
        result = mosquitto_loop_start(m_client);
        m_loopStarted = result == MOSQ_ERR_SUCCESS;
    }
    if (result == MOSQ_ERR_SUCCESS) {
        const QByteArray hostBytes = host.toUtf8();
        result = mosquitto_connect_async(m_client, hostBytes.constData(), port, 10);
    }
    if (result != MOSQ_ERR_SUCCESS) {
        emit errorOccurred(QStringLiteral("MQTT startup failed: %1")
                           .arg(QString::fromUtf8(mosquitto_strerror(result))));
        stop();
    }
#else
    Q_UNUSED(host)
    Q_UNUSED(port)
    Q_UNUSED(username)
    Q_UNUSED(password)
    emit errorOccurred(QStringLiteral("backend was built without libmosquitto"));
#endif
}

void SteeringInputManager::stop()
{
#ifdef IPC_HAS_MOSQUITTO
    if (m_client) {
        mosquitto_disconnect(m_client);
        if (m_loopStarted) {
            mosquitto_loop_stop(m_client, true);
            m_loopStarted = false;
        }
        mosquitto_destroy(m_client);
        m_client = nullptr;
    }
    if (m_libraryInitialized) {
        mosquitto_lib_cleanup();
        m_libraryInitialized = false;
    }
#endif
    setBrokerConnected(false);
}

#ifdef IPC_HAS_MOSQUITTO
void SteeringInputManager::mqttConnectCallback(
    struct mosquitto* client, void* context, int result)
{
    auto* self = static_cast<SteeringInputManager*>(context);
    if (result == 0) {
        mosquitto_subscribe(client, nullptr, CanTopic, 1);
        mosquitto_subscribe(client, nullptr, StatusTopic, 1);
    }
    QMetaObject::invokeMethod(self, [self, result]() {
        self->setBrokerConnected(result == 0);
        if (result != 0) {
            emit self->errorOccurred(QStringLiteral("MQTT connection rejected: %1")
                .arg(QString::fromUtf8(mosquitto_connack_string(result))));
        }
    }, Qt::QueuedConnection);
}

void SteeringInputManager::mqttDisconnectCallback(
    struct mosquitto*, void* context, int result)
{
    auto* self = static_cast<SteeringInputManager*>(context);
    QMetaObject::invokeMethod(self, [self, result]() {
        self->setBrokerConnected(false);
        if (result != 0) {
            emit self->errorOccurred(QStringLiteral("MQTT connection lost"));
        }
    }, Qt::QueuedConnection);
}

void SteeringInputManager::mqttMessageCallback(
    struct mosquitto*, void* context, const struct mosquitto_message* message)
{
    if (!message || !message->topic || !message->payload || message->payloadlen < 0) {
        return;
    }
    auto* self = static_cast<SteeringInputManager*>(context);
    const QByteArray topic(message->topic);
    const QByteArray payload(static_cast<const char*>(message->payload), message->payloadlen);
    QMetaObject::invokeMethod(self, [self, topic, payload]() {
        if (topic == StatusTopic) {
            self->setControllerOnline(payload.trimmed() == QByteArrayLiteral("online"));
        } else if (topic == CanTopic) {
            self->ingestCanPayload(payload);
        }
    }, Qt::QueuedConnection);
}
#endif
