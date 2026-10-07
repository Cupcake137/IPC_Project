#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>
#include "ipc_can_db.h"

static const uint8_t UART_SOF_0 = 0xAA;
static const uint8_t UART_SOF_1 = 0x55;
static const uint8_t UART_VERSION = 0x01;

struct UartFrameParser {
    uint8_t state;
    uint8_t index;
    uint16_t calculatedCrc;
    uint16_t receivedCrc;
    CAN_Frame_t frame;
};

void uart_parser_init(UartFrameParser* parser);
bool uart_parser_feed(UartFrameParser* parser, uint8_t byte_in, CAN_Frame_t* out_frame);
bool uart_send_frame(Stream& port, const CAN_Frame_t& frame);

#endif
