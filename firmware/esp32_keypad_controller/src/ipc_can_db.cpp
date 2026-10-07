#include "ipc_can_db.h"

static uint8_t update_crc(uint8_t crc, uint8_t value)
{
    crc ^= value;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        if ((crc & 0x80) != 0) {
            crc = (uint8_t)((crc << 1) ^ 0x1D);
        } else {
            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static uint8_t calculate_crc(const CAN_Frame_t& frame)
{
    uint8_t crc = 0xFF;
    crc = update_crc(crc, (uint8_t)(frame.id >> 8));
    crc = update_crc(crc, (uint8_t)(frame.id & 0xFF));
    crc = update_crc(crc, frame.dlc);
    for (uint8_t index = 0; index + 1 < frame.dlc; ++index) {
        crc = update_crc(crc, frame.data[index]);
    }
    return (uint8_t)(crc ^ 0xFF);
}

static void begin_frame(CAN_Frame_t* frame, uint16_t id, uint8_t dlc)
{
    memset(frame, 0, sizeof(*frame));
    frame->id = id;
    frame->dlc = dlc;
}

bool can_pack_key_event(uint8_t keyCode, uint8_t eventType,
                        uint16_t pressedKeys, uint8_t counter,
                        CAN_Frame_t* frame)
{
    if (frame == nullptr || keyCode > 15 || eventType > KEY_LONG_PRESS) {
        return false;
    }
    begin_frame(frame, CAN_SWC_KEY_EVENT, 6);
    frame->data[0] = keyCode;
    frame->data[1] = eventType;
    frame->data[2] = (uint8_t)(pressedKeys & 0xFF);
    frame->data[3] = (uint8_t)(pressedKeys >> 8);
    frame->data[4] = counter & 0x0F;
    frame->data[5] = calculate_crc(*frame);
    return true;
}

bool can_pack_network_status(bool wifiConnected, bool mqttConnected,
                             bool keypadValid, int8_t rssi,
                             uint32_t uptimeSeconds, uint8_t counter,
                             CAN_Frame_t* frame)
{
    if (frame == nullptr) {
        return false;
    }
    begin_frame(frame, CAN_SWC_NETWORK_STATUS, 8);
    frame->data[0] = (uint8_t)((wifiConnected ? 0x01 : 0)
        | (mqttConnected ? 0x02 : 0)
        | (keypadValid ? 0x04 : 0));
    frame->data[1] = (uint8_t)rssi;
    frame->data[2] = (uint8_t)uptimeSeconds;
    frame->data[3] = (uint8_t)(uptimeSeconds >> 8);
    frame->data[4] = (uint8_t)(uptimeSeconds >> 16);
    frame->data[5] = (uint8_t)(uptimeSeconds >> 24);
    frame->data[6] = counter & 0x0F;
    frame->data[7] = calculate_crc(*frame);
    return true;
}

void can_frame_to_text(const CAN_Frame_t& frame, char* output, size_t outputSize)
{
    if (output == nullptr || outputSize < 21) {
        return;
    }
    int length = snprintf(output, outputSize, "%03X#", frame.id);
    for (uint8_t index = 0; index < frame.dlc; ++index) {
        length += snprintf(output + length, outputSize - length,
                           "%02X", frame.data[index]);
    }
}
