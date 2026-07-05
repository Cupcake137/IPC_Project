#include <Arduino.h>
#include <SoftwareSerial.h>
#include "can_manager.h"
#include "vehicle_mcu.h"

SoftwareSerial piSerial(2, 3); // RX = Pin 2, TX = Pin 3

uint8_t rxBuffer[sizeof(CAN_Frame_t)];
int rxByteCount = 0;

void setup() {
    Serial.begin(9600);
    piSerial.begin(9600);
    vehicle_hardware_init();
    Serial.println("--- MCU FIRMWARE RUNNING ---");
}

void loop() {
    static unsigned long lastSimulationTick = 0;
    unsigned long now = millis();
    
    if (now - lastSimulationTick >= 1000) {
        lastSimulationTick = now;
        update_battery_and_dtc_simulation();
    }

    check_gear_shift_buttons(&piSerial);
    broadcast_periodic_telemetry(&piSerial);

    while (piSerial.available() > 0) {
        uint8_t byteIn = piSerial.read();
        if (rxByteCount < (int)sizeof(CAN_Frame_t)) {
            rxBuffer[rxByteCount++] = byteIn;
        }

        if (rxByteCount == sizeof(CAN_Frame_t)) {
            CAN_Frame_t *frame = (CAN_Frame_t*)rxBuffer;
            uint16_t localCrc = calculate_CAN_CRC(frame->can_id, frame->dlc, frame->data);

            if (localCrc == frame->crc && frame->can_id == 0x0100) {
                uint8_t authorizedPwm = frame->data[0];

                if (currentGear == 1) { 
                    digitalWrite(IN1_PIN, LOW);
                    digitalWrite(IN2_PIN, HIGH);
                } else {                
                    digitalWrite(IN1_PIN, HIGH);
                    digitalWrite(IN2_PIN, LOW);
                }

                analogWrite(ENA_PIN, authorizedPwm);
                rxByteCount = 0; 
            } 
            else {
                memmove(rxBuffer, rxBuffer + 1, sizeof(CAN_Frame_t) - 1);
                rxByteCount--;
            }
        }
    }
}