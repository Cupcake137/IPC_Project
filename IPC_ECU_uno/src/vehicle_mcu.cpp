#include "vehicle_mcu.h"
#include "can_manager.h"

float simulatedSoC = 100.0;
uint8_t currentGear = 0; 
uint8_t vehicleSpeedKmh = 0;
uint8_t activeDtcCode = 0x00; 

static uint8_t localTxCounter = 0;
static bool lastUpState = HIGH;
static bool lastDownState = HIGH;

void vehicle_hardware_init() {
    pinMode(ENA_PIN, OUTPUT);
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_DOWN_PIN, INPUT_PULLUP);

    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, 0);
}

void update_battery_and_dtc_simulation() {
    uint8_t activePwm = OCR0B; 
    
    // Mathematical speed approximation from active PWM
    if (currentGear == 1 || currentGear == 3) {
        vehicleSpeedKmh = (uint8_t)(((uint16_t)activePwm * 120) / 255);
    } else {
        vehicleSpeedKmh = 0; 
    }

    // Battery Drain logic
    if (activePwm > 0) {
        simulatedSoC -= (0.05 + ((float)activePwm / 255.0) * 0.2); 
    } else {
        simulatedSoC -= 0.005; 
    }
    if (simulatedSoC < 0) simulatedSoC = 0.0;

    // Diagnostic Trouble Code Logic
    if (simulatedSoC < 15.0) {
        activeDtcCode = 0x11; // Low Battery
    } else if (activePwm == 255) {
        static unsigned long overloadTimer = 0;
        if (overloadTimer == 0) overloadTimer = millis();
        if (millis() - overloadTimer > 3000) {
            activeDtcCode = 0x22; // Overload
        }
    } else {
        activeDtcCode = 0x00; 
    }
}

static void send_modular_can(SoftwareSerial* port, uint16_t id, uint8_t dlc, uint8_t* data) {
    CAN_Frame_t frame;
    frame.sof = 0x00; frame.can_id = id; frame.rtr = 0; frame.ide_r0 = 0; frame.dlc = dlc;
    memset(frame.data, 0, 8);
    memcpy(frame.data, data, dlc);
    frame.crc = calculate_CAN_CRC(id, dlc, data);
    frame.ack = 0xFF; frame.eof = 0x7F;
    frame.counter = localTxCounter;

    port->write((uint8_t*)&frame, sizeof(CAN_Frame_t));
    localTxCounter = (localTxCounter + 1) % 16;
}

void check_gear_shift_buttons(SoftwareSerial* serial_line) {
    bool upState = digitalRead(BTN_UP_PIN);
    if (upState == LOW && lastUpState == HIGH) {
        delay(20); 
        if (digitalRead(BTN_UP_PIN) == LOW && currentGear < 3) {
            currentGear++;
            uint8_t payload = currentGear;
            send_modular_can(serial_line, 0x0401, 1, &payload);
        }
    }
    lastUpState = upState;

    bool downState = digitalRead(BTN_DOWN_PIN);
    if (downState == LOW && lastDownState == HIGH) {
        delay(20);
        if (digitalRead(BTN_DOWN_PIN) == LOW && currentGear > 0) {
            currentGear--;
            uint8_t payload = currentGear;
            send_modular_can(serial_line, 0x0401, 1, &payload);
        }
    }
    lastDownState = downState;
}

void broadcast_periodic_telemetry(SoftwareSerial* serial_line) {
    static unsigned long lastPedalTime = 0;
    static unsigned long lastSocTime = 0;
    unsigned long now = millis();

    if (now - lastPedalTime >= 100) {
        lastPedalTime = now;
        uint8_t payload[2];
        payload[0] = map(analogRead(POT_PIN), 0, 1023, 0, 100); 
        payload[1] = vehicleSpeedKmh;                            
        send_modular_can(serial_line, 0x0101, 2, payload);       
    }

    if (now - lastSocTime >= 1000) {
        lastSocTime = now;
        uint8_t socPayload = (uint8_t)simulatedSoC;
        send_modular_can(serial_line, 0x0201, 1, &socPayload);

        uint8_t dtcPayload = activeDtcCode;
        send_modular_can(serial_line, 0x0501, 1, &dtcPayload);
    }
}