#include <Arduino.h>
#include <SoftwareSerial.h>
#include "can_manager.h"
#include "vehicle_mcu.h"

SoftwareSerial piSerial(PI_RX_PIN, PI_TX_PIN);

static UartFrameParser rxParser;
static bool motorCounterSeen = false;
static uint8_t lastMotorCounter = 0;

static void process_vcu_frame(const CAN_Frame_t& frame) {
    MotorCommand command;
    if (!can_unpack_motor_command(frame, &command)) {
        return;
    }
    if (motorCounterSeen && command.aliveCounter == lastMotorCounter) {
        return;
    }

    motorCounterSeen = true;
    lastMotorCounter = command.aliveCounter;
    const bool commandMatchesGear = command.gear == vehicle_current_gear();
    const uint8_t safePwm = command.torqueEnable && commandMatchesGear
        ? command.pwm
        : 0;
    vehicle_receive_motor_command(safePwm);
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
