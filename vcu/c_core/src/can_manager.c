#include <string.h>
#include "can_manager.h"

#define CRC16_INITIAL 0xFFFFU
#define CRC16_POLYNOMIAL 0x1021U

typedef enum {
    WAIT_SOF_0,
    WAIT_SOF_1,
    READ_VERSION,
    READ_ID_H,
    READ_ID_L,
    READ_DLC,
    READ_DATA,
    READ_CRC_H,
    READ_CRC_L,
} ParserState_t;

static uint16_t crc16_update(uint16_t crc, uint8_t value) {
    crc ^= (uint16_t)value << 8U;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        crc = (crc & 0x8000U) ? (uint16_t)((crc << 1U) ^ CRC16_POLYNOMIAL)
                              : (uint16_t)(crc << 1U);
    }
    return crc;
}

uint16_t uart_transport_crc16(const uint8_t* data, uint8_t length) {
    uint16_t crc = CRC16_INITIAL;
    if (data == NULL && length != 0U) {
        return 0;
    }
    for (uint8_t index = 0; index < length; ++index) {
        crc = crc16_update(crc, data[index]);
    }
    return crc;
}

void uart_parser_init(UartFrameParser_t* parser) {
    if (parser == NULL) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
    parser->state = WAIT_SOF_0;
    parser->calculated_crc = CRC16_INITIAL;
}

static void parser_restart(UartFrameParser_t* parser, uint8_t byte_in) {
    uart_parser_init(parser);
    if (byte_in == UART_CAN_SOF_0) {
        parser->state = WAIT_SOF_1;
    }
}

bool uart_parser_feed(UartFrameParser_t* parser, uint8_t byte_in, CAN_Frame_t* out_frame) {
    if (parser == NULL || out_frame == NULL) {
        return false;
    }

    switch ((ParserState_t)parser->state) {
        case WAIT_SOF_0:
            if (byte_in == UART_CAN_SOF_0) {
                parser->state = WAIT_SOF_1;
            }
            break;
        case WAIT_SOF_1:
            if (byte_in == UART_CAN_SOF_1) {
                parser->state = READ_VERSION;
                parser->calculated_crc = CRC16_INITIAL;
            } else {
                parser_restart(parser, byte_in);
            }
            break;
        case READ_VERSION:
            if (byte_in != UART_CAN_VERSION) {
                parser_restart(parser, byte_in);
                break;
            }
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte_in);
            parser->state = READ_ID_H;
            break;
        case READ_ID_H:
            parser->frame.can_id = (uint32_t)byte_in << 8U;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte_in);
            parser->state = READ_ID_L;
            break;
        case READ_ID_L:
            parser->frame.can_id |= byte_in;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte_in);
            if (parser->frame.can_id > IPC_CAN_STANDARD_ID_MAX) {
                parser_restart(parser, byte_in);
            } else {
                parser->state = READ_DLC;
            }
            break;
        case READ_DLC:
            if (byte_in > IPC_CAN_MAX_DLC) {
                parser_restart(parser, byte_in);
                break;
            }
            parser->frame.can_dlc = byte_in;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte_in);
            parser->index = 0;
            memset(parser->frame.data, 0, sizeof(parser->frame.data));
            parser->state = byte_in == 0U ? READ_CRC_H : READ_DATA;
            break;
        case READ_DATA:
            parser->frame.data[parser->index++] = byte_in;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte_in);
            if (parser->index >= parser->frame.can_dlc) {
                parser->state = READ_CRC_H;
            }
            break;
        case READ_CRC_H:
            parser->received_crc = (uint16_t)byte_in << 8U;
            parser->state = READ_CRC_L;
            break;
        case READ_CRC_L:
            parser->received_crc |= byte_in;
            if (parser->received_crc == parser->calculated_crc) {
                *out_frame = parser->frame;
                uart_parser_init(parser);
                return true;
            }
            parser_restart(parser, byte_in);
            break;
    }
    return false;
}

int uart_encode_frame(uint8_t* out, int out_len, const CAN_Frame_t* frame) {
    if (out == NULL || frame == NULL || frame->can_id > IPC_CAN_STANDARD_ID_MAX
        || frame->can_dlc > IPC_CAN_MAX_DLC
        || out_len < (int)(9U + frame->can_dlc)) {
        return -1;
    }

    int length = 0;
    out[length++] = UART_CAN_SOF_0;
    out[length++] = UART_CAN_SOF_1;
    const int crc_start = length;
    out[length++] = UART_CAN_VERSION;
    out[length++] = (uint8_t)(frame->can_id >> 8U);
    out[length++] = (uint8_t)(frame->can_id & 0xFFU);
    out[length++] = frame->can_dlc;
    for (uint8_t index = 0; index < frame->can_dlc; ++index) {
        out[length++] = frame->data[index];
    }
    const uint16_t crc = uart_transport_crc16(
        &out[crc_start], (uint8_t)(length - crc_start));
    out[length++] = (uint8_t)(crc >> 8U);
    out[length++] = (uint8_t)(crc & 0xFFU);
    return length;
}
