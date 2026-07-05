#include "can_manager.h"

uint16_t calculate_CAN_CRC(uint16_t id, uint8_t dlc, uint8_t* data) {
    uint32_t crc = 0;
    crc ^= id; crc ^= dlc;
    for(int i = 0; i < dlc; i++) {
        crc = (crc << 8) ^ data[i];
    }
    return (uint16_t)(crc & 0x7FFF);
}