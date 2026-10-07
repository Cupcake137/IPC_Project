#include "can_manager.h"

enum ParserState : uint8_t {
    WAIT_SOF_0,
    WAIT_SOF_1,
    READ_VERSION,
    READ_ID_H,
    READ_ID_L,
    READ_DLC,
    READ_DATA,
    READ_CRC_H,
    READ_CRC_L,
};

static uint16_t update_crc16(uint16_t crc, uint8_t value) {
    crc ^= (uint16_t)value << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        if ((crc & 0x8000) != 0) {
            crc = (uint16_t)((crc << 1) ^ 0x1021);
        } else {
            crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

void uart_parser_init(UartFrameParser* parser) {
    parser->state = WAIT_SOF_0;
    parser->index = 0;
    parser->calculatedCrc = 0xFFFF;
    parser->receivedCrc = 0;
    parser->frame.id = 0;
    parser->frame.dlc = 0;
    memset(parser->frame.data, 0, sizeof(parser->frame.data));
}

static void parser_restart(UartFrameParser* parser, uint8_t byte_in) {
    uart_parser_init(parser);
    if (byte_in == UART_SOF_0) {
        parser->state = WAIT_SOF_1;
    }
}

bool uart_parser_feed(UartFrameParser* parser, uint8_t byte_in, CAN_Frame_t* out_frame) {
    switch (parser->state) {
        case WAIT_SOF_0:
            if (byte_in == UART_SOF_0) {
                parser->state = WAIT_SOF_1;
            }
            break;

        case WAIT_SOF_1:
            if (byte_in == UART_SOF_1) {
                parser->state = READ_VERSION;
                parser->calculatedCrc = 0xFFFF;
            } else {
                parser_restart(parser, byte_in);
            }
            break;

        case READ_VERSION:
            if (byte_in != UART_VERSION) {
                parser_restart(parser, byte_in);
                break;
            }
            parser->calculatedCrc = update_crc16(parser->calculatedCrc, byte_in);
            parser->state = READ_ID_H;
            break;

        case READ_ID_H:
            parser->frame.id = ((uint16_t)byte_in) << 8;
            parser->calculatedCrc = update_crc16(parser->calculatedCrc, byte_in);
            parser->state = READ_ID_L;
            break;

        case READ_ID_L:
            parser->frame.id |= byte_in;
            parser->calculatedCrc = update_crc16(parser->calculatedCrc, byte_in);
            if (parser->frame.id > 0x7FF) {
                parser_restart(parser, byte_in);
            } else {
                parser->state = READ_DLC;
            }
            break;

        case READ_DLC:
            if (byte_in > CAN_MAX_DLC) {
                parser_restart(parser, byte_in);
                break;
            }
            parser->frame.dlc = byte_in;
            parser->calculatedCrc = update_crc16(parser->calculatedCrc, byte_in);
            parser->index = 0;
            memset(parser->frame.data, 0, sizeof(parser->frame.data));
            parser->state = (parser->frame.dlc == 0) ? READ_CRC_H : READ_DATA;
            break;

        case READ_DATA:
            parser->frame.data[parser->index++] = byte_in;
            parser->calculatedCrc = update_crc16(parser->calculatedCrc, byte_in);
            if (parser->index >= parser->frame.dlc) {
                parser->state = READ_CRC_H;
            }
            break;

        case READ_CRC_H:
            parser->receivedCrc = (uint16_t)byte_in << 8;
            parser->state = READ_CRC_L;
            break;

        case READ_CRC_L:
            parser->receivedCrc |= byte_in;
            if (parser->receivedCrc == parser->calculatedCrc) {
                *out_frame = parser->frame;
                uart_parser_init(parser);
                return true;
            }
            parser_restart(parser, byte_in);
            break;
    }

    return false;
}

bool uart_send_frame(Stream& port, const CAN_Frame_t& frame) {
    if (frame.id > 0x7FF || frame.dlc > CAN_MAX_DLC) {
        return false;
    }

    port.write(UART_SOF_0);
    port.write(UART_SOF_1);
    port.write(UART_VERSION);
    port.write((uint8_t)(frame.id >> 8));
    port.write((uint8_t)(frame.id & 0xFF));
    port.write(frame.dlc);

    uint16_t crc = 0xFFFF;
    crc = update_crc16(crc, UART_VERSION);
    crc = update_crc16(crc, (uint8_t)(frame.id >> 8));
    crc = update_crc16(crc, (uint8_t)(frame.id & 0xFF));
    crc = update_crc16(crc, frame.dlc);
    for (uint8_t index = 0; index < frame.dlc; ++index) {
        port.write(frame.data[index]);
        crc = update_crc16(crc, frame.data[index]);
    }
    port.write((uint8_t)(crc >> 8));
    port.write((uint8_t)(crc & 0xFF));
    return true;
}
