#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>

static const uint8_t UART_SOF_0 = 0xAA;
static const uint8_t UART_SOF_1 = 0x55;
static const uint8_t UART_MAX_DLC = 8;

enum MessageId : uint16_t {
    MSG_MOTOR_COMMAND = 0x0100,
    MSG_PEDAL_SPEED   = 0x0101,
    MSG_BATTERY_SOC   = 0x0201,
    MSG_GEAR_STATE    = 0x0401,
    MSG_DTC_STATUS    = 0x0501,
};

struct CAN_Frame_t {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[UART_MAX_DLC];
    uint8_t counter;
};

struct UartFrameParser {
    uint8_t state;
    uint8_t index;
    uint8_t checksum;
    CAN_Frame_t frame;
};

void uart_parser_init(UartFrameParser* parser);
bool uart_parser_feed(UartFrameParser* parser, uint8_t byte_in, CAN_Frame_t* out_frame);
uint8_t uart_frame_checksum(uint16_t id, uint8_t dlc, uint8_t counter, const uint8_t* data);
bool uart_send_frame(Stream& port, uint16_t id, uint8_t dlc, const uint8_t* data, uint8_t* tx_counter);

#endif
