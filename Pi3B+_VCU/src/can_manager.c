#include <string.h>
#include "can_manager.h"

typedef enum {
    WAIT_SOF_0,
    WAIT_SOF_1,
    READ_ID_H,
    READ_ID_L,
    READ_DLC,
    READ_COUNTER,
    READ_DATA,
    READ_CHECKSUM,
} ParserState_t;

uint8_t uart_frame_checksum(uint16_t id, uint8_t dlc, uint8_t counter, const uint8_t* data) {
    uint16_t sum = 0;
    sum += (uint8_t)(id >> 8);
    sum += (uint8_t)(id & 0xFF);
    sum += dlc;
    sum += counter;

    for (uint8_t i = 0; i < dlc; ++i) {
        sum += data[i];
    }

    return (uint8_t)(sum & 0xFF);
}

void uart_parser_init(UartFrameParser_t* parser) {
    parser->state = WAIT_SOF_0;
    parser->index = 0;
    parser->checksum = 0;
    parser->frame.id = 0;
    parser->frame.dlc = 0;
    parser->frame.counter = 0;
    memset(parser->frame.data, 0, sizeof(parser->frame.data));
}

static void parser_restart(UartFrameParser_t* parser, uint8_t byte_in) {
    uart_parser_init(parser);
    if (byte_in == UART_SOF_0) {
        parser->state = WAIT_SOF_1;
    }
}

bool uart_parser_feed(UartFrameParser_t* parser, uint8_t byte_in, CAN_Frame_t* out_frame) {
    switch ((ParserState_t)parser->state) {
        case WAIT_SOF_0:
            if (byte_in == UART_SOF_0) {
                parser->state = WAIT_SOF_1;
            }
            break;

        case WAIT_SOF_1:
            if (byte_in == UART_SOF_1) {
                parser->state = READ_ID_H;
                parser->checksum = 0;
            } else {
                parser_restart(parser, byte_in);
            }
            break;

        case READ_ID_H:
            parser->frame.id = ((uint16_t)byte_in) << 8;
            parser->checksum += byte_in;
            parser->state = READ_ID_L;
            break;

        case READ_ID_L:
            parser->frame.id |= byte_in;
            parser->checksum += byte_in;
            parser->state = READ_DLC;
            break;

        case READ_DLC:
            if (byte_in > UART_MAX_DLC) {
                parser_restart(parser, byte_in);
                break;
            }
            parser->frame.dlc = byte_in;
            parser->checksum += byte_in;
            parser->index = 0;
            memset(parser->frame.data, 0, sizeof(parser->frame.data));
            parser->state = READ_COUNTER;
            break;

        case READ_COUNTER:
            parser->frame.counter = byte_in;
            parser->checksum += byte_in;
            parser->state = (parser->frame.dlc == 0) ? READ_CHECKSUM : READ_DATA;
            break;

        case READ_DATA:
            parser->frame.data[parser->index++] = byte_in;
            parser->checksum += byte_in;
            if (parser->index >= parser->frame.dlc) {
                parser->state = READ_CHECKSUM;
            }
            break;

        case READ_CHECKSUM:
            if (parser->checksum == byte_in) {
                *out_frame = parser->frame;
                uart_parser_init(parser);
                return true;
            }
            parser_restart(parser, byte_in);
            break;
    }

    return false;
}

int uart_encode_frame(uint8_t* out, int out_len, uint16_t id, uint8_t dlc, const uint8_t* data, uint8_t* tx_counter) {
    if (out == NULL || out_len < 7 || dlc > UART_MAX_DLC || out_len < (int)(7 + dlc)) {
        return -1;
    }

    const uint8_t counter = *tx_counter & 0x0F;
    uint8_t payload[UART_MAX_DLC] = {0};
    if (data != NULL && dlc > 0) {
        memcpy(payload, data, dlc);
    }

    int n = 0;
    out[n++] = UART_SOF_0;
    out[n++] = UART_SOF_1;
    out[n++] = (uint8_t)(id >> 8);
    out[n++] = (uint8_t)(id & 0xFF);
    out[n++] = dlc;
    out[n++] = counter;
    for (uint8_t i = 0; i < dlc; ++i) {
        out[n++] = payload[i];
    }
    out[n++] = uart_frame_checksum(id, dlc, counter, payload);

    *tx_counter = (uint8_t)((counter + 1) & 0x0F);
    return n;
}
