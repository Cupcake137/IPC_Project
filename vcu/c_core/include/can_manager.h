#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "ipc_can_db.h"

#define UART_CAN_SOF_0 0xAAU
#define UART_CAN_SOF_1 0x55U
#define UART_CAN_VERSION 0x01U
#define UART_CAN_MAX_ENCODED_SIZE 16U

typedef struct {
    uint8_t state;
    uint8_t index;
    uint16_t calculated_crc;
    uint16_t received_crc;
    CAN_Frame_t frame;
} UartFrameParser_t;

void uart_parser_init(UartFrameParser_t* parser);
bool uart_parser_feed(UartFrameParser_t* parser, uint8_t byte_in, CAN_Frame_t* out_frame);
uint16_t uart_transport_crc16(const uint8_t* data, uint8_t length);
int uart_encode_frame(uint8_t* out, int out_len, const CAN_Frame_t* frame);

#endif
