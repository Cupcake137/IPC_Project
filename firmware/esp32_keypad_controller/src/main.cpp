#include <Arduino.h>
#include <WiFi.h>
#include <MQTT.h>
#include "ipc_can_db.h"
#include "secrets.h"

constexpr uint16_t MQTT_PORT = 1883;

const char *STATUS_TOPIC = "ipc/swc/status";
const char *CAN_TOPIC = "ipc/can/swc";

// ---------- Keypad configuration ----------

constexpr uint8_t ROWS = 4;
constexpr uint8_t COLS = 4;

const uint8_t drivePins[ROWS] = {16, 4, 2, 15};
const uint8_t sensePins[COLS] = {19, 18, 5, 17};

const char keyMap[ROWS][COLS] = {
    {'1', '4', '7', '*'},
    {'2', '5', '8', '0'},
    {'3', '6', '9', '#'},
    {'A', 'B', 'C', 'D'}
};

constexpr uint32_t DEBOUNCE_MS = 35;
constexpr uint32_t LONG_PRESS_MS = 800;
constexpr uint32_t HEARTBEAT_MS = 2000;
constexpr uint32_t WIFI_RETRY_MS = 5000;
constexpr uint32_t MQTT_RETRY_MS = 3000;

// ---------- Runtime state ----------

WiFiClient networkClient;
MQTTClient mqttClient(256);

uint8_t keyCounter = 0;
uint8_t networkCounter = 0;
uint16_t pressedKeys = 0;

uint32_t lastHeartbeatMs = 0;
uint32_t lastWiFiAttemptMs = 0;
uint32_t lastMqttAttemptMs = 0;

char candidateKey = '\0';
char stableKey = '\0';

uint32_t candidateSinceMs = 0;
uint32_t keyPressedAtMs = 0;

bool longPressPublished = false;

// ---------- Keypad ----------

char scanKeypad()
{
    for (uint8_t drive = 0; drive < ROWS; ++drive) {
        digitalWrite(drivePins[drive], LOW);
        delayMicroseconds(30);

        for (uint8_t sense = 0; sense < COLS; ++sense) {
            if (digitalRead(sensePins[sense]) == LOW) {
                const char key = keyMap[drive][sense];

                digitalWrite(drivePins[drive], HIGH);
                return key;
            }
        }

        digitalWrite(drivePins[drive], HIGH);
    }

    return '\0';
}

uint8_t keyToCode(char key)
{
    const char keys[] = "123A456B789C*0#D";
    for (uint8_t index = 0; index < 16; ++index) {
        if (keys[index] == key) {
            return index;
        }
    }
    return 0;
}

void publishCanFrame(const CAN_Frame_t& frame)
{
    char payload[24] = {0};
    can_frame_to_text(frame, payload, sizeof(payload));
    Serial.println(payload);

    if (mqttClient.connected()) {
        mqttClient.publish(CAN_TOPIC, payload, false, 1);
    }
}

void publishKeyEvent(char key, uint8_t eventType)
{
    const uint8_t keyCode = keyToCode(key);
    const uint16_t keyBit = (uint16_t)1 << keyCode;
    if (eventType == KEY_PRESS) {
        pressedKeys |= keyBit;
    } else if (eventType == KEY_RELEASE) {
        pressedKeys &= (uint16_t)~keyBit;
    }

    CAN_Frame_t frame;
    if (can_pack_key_event(keyCode, eventType, pressedKeys,
                           keyCounter, &frame)) {
        publishCanFrame(frame);
        keyCounter = (uint8_t)((keyCounter + 1) & 0x0F);
    }
}

void serviceKeypad()
{
    const uint32_t now = millis();
    const char scannedKey = scanKeypad();

    if (scannedKey != candidateKey) {
        candidateKey = scannedKey;
        candidateSinceMs = now;
    }

    if (candidateKey != stableKey &&
        now - candidateSinceMs >= DEBOUNCE_MS) {
        const char previousKey = stableKey;
        stableKey = candidateKey;

        if (previousKey != '\0') {
            publishKeyEvent(previousKey, KEY_RELEASE);
        }

        if (stableKey != '\0') {
            keyPressedAtMs = now;
            longPressPublished = false;

            publishKeyEvent(stableKey, KEY_PRESS);
        }
    }

    if (stableKey != '\0' &&
        !longPressPublished &&
        now - keyPressedAtMs >= LONG_PRESS_MS) {
        longPressPublished = true;

        publishKeyEvent(stableKey, KEY_LONG_PRESS);
    }
}

// ---------- Wi-Fi and MQTT ----------

String createClientId()
{
    const uint64_t chipId = ESP.getEfuseMac();

    return "ipc-swc-" + String(
        static_cast<uint32_t>(chipId & 0xFFFFFFFF),
        HEX
    );
}

void beginWiFiConnection()
{
    lastWiFiAttemptMs = millis();

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(IPC_WIFI_SSID);

    WiFi.disconnect();
    WiFi.begin(IPC_WIFI_SSID, IPC_WIFI_PASSWORD);
}

void serviceWiFi()
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (millis() - lastWiFiAttemptMs >= WIFI_RETRY_MS) {
        beginWiFiConnection();
    }
}

void connectMqtt()
{
    if (WiFi.status() != WL_CONNECTED ||
        mqttClient.connected()) {
        return;
    }

    if (millis() - lastMqttAttemptMs < MQTT_RETRY_MS) {
        return;
    }

    lastMqttAttemptMs = millis();

    const String clientId = createClientId();

    Serial.print("Connecting MQTT as ");
    Serial.println(clientId);

    if (mqttClient.connect(
            clientId.c_str(),
            IPC_MQTT_USERNAME,
            IPC_MQTT_PASSWORD)) {
        Serial.println("MQTT connected");

        mqttClient.publish(
            STATUS_TOPIC,
            "online",
            true,
            1
        );
    } else {
        Serial.println("MQTT connection failed");
    }
}

void publishNetworkStatus()
{
    if (!mqttClient.connected() ||
        millis() - lastHeartbeatMs < HEARTBEAT_MS) {
        return;
    }

    lastHeartbeatMs = millis();
    CAN_Frame_t frame;
    if (can_pack_network_status(true, true, true, (int8_t)WiFi.RSSI(),
                                millis() / 1000, networkCounter, &frame)) {
        publishCanFrame(frame);
        networkCounter = (uint8_t)((networkCounter + 1) & 0x0F);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    for (uint8_t drive = 0; drive < ROWS; ++drive) {
        digitalWrite(drivePins[drive], HIGH);
        pinMode(drivePins[drive], OUTPUT);
    }

    for (uint8_t sense = 0; sense < COLS; ++sense) {
        pinMode(sensePins[sense], INPUT_PULLUP);
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    mqttClient.begin(
        IPC_MQTT_HOST,
        MQTT_PORT,
        networkClient
    );

    mqttClient.setWill(
        STATUS_TOPIC,
        "offline",
        true,
        1
    );

    Serial.println();
    Serial.println("============================");
    Serial.println(" IPC Steering Controller");
    Serial.println("============================");

    beginWiFiConnection();
}

void loop()
{
    // Keep scanning while offline, but never replay stale key events.
    serviceKeypad();

    serviceWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        connectMqtt();

        if (mqttClient.connected()) {
            mqttClient.loop();
            publishNetworkStatus();
        }
    }

    delay(2);
}
