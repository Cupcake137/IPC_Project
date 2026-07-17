#include <Arduino.h>
#include <SoftwareSerial.h>
#include "can_manager.h"
#include "vehicle_mcu.h"

SoftwareSerial piSerial(PI_RX_PIN, PI_TX_PIN);

static UartFrameParser rxParser;

static void process_vcu_frame(const CAN_Frame_t& frame) {
    if (frame.id == MSG_MOTOR_COMMAND && frame.dlc >= 1) {
        vehicle_apply_motor_command(frame.data[0]);
    }
}

void setup() {
    Serial.begin(115200);
    piSerial.begin(ECU_UART_BAUD);
    uart_parser_init(&rxParser);
    vehicle_hardware_init();

    Serial.println(F("[ECU] IPC ECU Uno firmware started."));
}

void loop() {
    vehicle_tick(piSerial);

    while (piSerial.available() > 0) {
        CAN_Frame_t frame;
        if (uart_parser_feed(&rxParser, (uint8_t)piSerial.read(), &frame)) {
            process_vcu_frame(frame);
        }
    }
}
