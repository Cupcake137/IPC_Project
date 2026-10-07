#ifndef IPC_CAN_DB_H
#define IPC_CAN_DB_H

#include <Arduino.h>

static const uint16_t CAN_SWC_KEY_EVENT = 0x300;
static const uint16_t CAN_SWC_NETWORK_STATUS = 0x301;

enum KeyEventType : uint8_t {
    KEY_PRESS = 0,
    KEY_RELEASE = 1,
    KEY_LONG_PRESS = 2
};

struct CAN_Frame_t {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
};

bool can_pack_key_event(uint8_t keyCode, uint8_t eventType,
                        uint16_t pressedKeys, uint8_t counter,
                        CAN_Frame_t* frame);
bool can_pack_network_status(bool wifiConnected, bool mqttConnected,
                             bool keypadValid, int8_t rssi,
                             uint32_t uptimeSeconds, uint8_t counter,
                             CAN_Frame_t* frame);
void can_frame_to_text(const CAN_Frame_t& frame, char* output, size_t outputSize);

#endif
