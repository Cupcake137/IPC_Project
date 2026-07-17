#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define UART_SOF_0 0xAA
#define UART_SOF_1 0x55
#define UART_MAX_DLC 8

typedef enum {
    MSG_MOTOR_COMMAND = 0x0100,
    MSG_PEDAL_SPEED   = 0x0101,
    MSG_BATTERY_SOC   = 0x0201,
    MSG_GEAR_STATE    = 0x0401,
    MSG_DTC_STATUS    = 0x0501,
} MessageId_t;

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[UART_MAX_DLC];
    uint8_t counter;
} CAN_Frame_t;

typedef struct {
    uint8_t state;
    uint8_t index;
    uint8_t checksum;
    CAN_Frame_t frame;
} UartFrameParser_t;

void uart_parser_init(UartFrameParser_t* parser);
bool uart_parser_feed(UartFrameParser_t* parser, uint8_t byte_in, CAN_Frame_t* out_frame);
uint8_t uart_frame_checksum(uint16_t id, uint8_t dlc, uint8_t counter, const uint8_t* data);
int uart_encode_frame(uint8_t* out, int out_len, uint16_t id, uint8_t dlc, const uint8_t* data, uint8_t* tx_counter);

#endif
