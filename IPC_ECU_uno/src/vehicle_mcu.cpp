#include "vehicle_mcu.h"
#include "can_manager.h"

float simulatedSoC = 100.0f;
uint8_t currentGear = GEAR_P;
uint8_t vehicleSpeedKmh = 0;
uint8_t activeDtcCode = 0x00;
uint8_t pedalPercent = 0;

static uint8_t localTxCounter = 0;
static uint8_t appliedPwm = 0;
static uint32_t lastPedalSampleMs = 0;
static uint32_t lastTelemetryMs = 0;
static uint32_t lastSlowTelemetryMs = 0;
static uint32_t lastSimulationMs = 0;
static uint32_t overloadStartMs = 0;

static bool lastStableUp = HIGH;
static bool lastStableDown = HIGH;
static bool lastRawUp = HIGH;
static bool lastRawDown = HIGH;
static uint32_t lastUpChangeMs = 0;
static uint32_t lastDownChangeMs = 0;

static uint8_t read_pedal_percent() {
    const int raw = analogRead(POT_PIN);
    if (raw < 40) {
        return 0;
    }
    return (uint8_t)constrain(map(raw, 40, 1023, 0, 100), 0, 100);
}

static char gear_to_char(uint8_t gear) {
    switch (gear) {
        case GEAR_R: return 'R';
        case GEAR_N: return 'N';
        case GEAR_D: return 'D';
        default: return 'P';
    }
}

void vehicle_hardware_init() {
    pinMode(ENA_PIN, OUTPUT);
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_DOWN_PIN, INPUT_PULLUP);

    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, 0);
}

void vehicle_apply_motor_command(uint8_t authorized_pwm) {
    appliedPwm = authorized_pwm;

    if (currentGear == GEAR_D && authorized_pwm > 0) {
        digitalWrite(IN1_PIN, HIGH);
        digitalWrite(IN2_PIN, LOW);
        analogWrite(ENA_PIN, authorized_pwm);
    } else if (currentGear == GEAR_R && authorized_pwm > 0) {
        digitalWrite(IN1_PIN, LOW);
        digitalWrite(IN2_PIN, HIGH);
        analogWrite(ENA_PIN, authorized_pwm);
    } else {
        analogWrite(ENA_PIN, 0);
        digitalWrite(IN1_PIN, LOW);
        digitalWrite(IN2_PIN, LOW);
    }
}

static void send_gear(SoftwareSerial& serial_line) {
    uart_send_frame(serial_line, MSG_GEAR_STATE, 1, &currentGear, &localTxCounter);
}

static void handle_shift_button(bool up_button, SoftwareSerial& serial_line) {
    if (pedalPercent > 2) {
        Serial.println(F("[ECU] Shift blocked: release pedal first."));
        return;
    }

    if (up_button && currentGear < GEAR_D) {
        currentGear++;
    } else if (!up_button && currentGear > GEAR_P) {
        currentGear--;
    } else {
        return;
    }

    Serial.print(F("[ECU] Gear -> "));
    Serial.println(gear_to_char(currentGear));
    vehicle_apply_motor_command(appliedPwm);
    send_gear(serial_line);
}

static void debounce_buttons(SoftwareSerial& serial_line) {
    const uint32_t now = millis();
    const bool rawUp = digitalRead(BTN_UP_PIN);
    const bool rawDown = digitalRead(BTN_DOWN_PIN);

    if (rawUp != lastRawUp) {
        lastRawUp = rawUp;
        lastUpChangeMs = now;
    }
    if ((now - lastUpChangeMs) >= 35 && rawUp != lastStableUp) {
        const bool previous = lastStableUp;
        lastStableUp = rawUp;
        if (previous == HIGH && lastStableUp == LOW) {
            handle_shift_button(true, serial_line);
        }
    }

    if (rawDown != lastRawDown) {
        lastRawDown = rawDown;
        lastDownChangeMs = now;
    }
    if ((now - lastDownChangeMs) >= 35 && rawDown != lastStableDown) {
        const bool previous = lastStableDown;
        lastStableDown = rawDown;
        if (previous == HIGH && lastStableDown == LOW) {
            handle_shift_button(false, serial_line);
        }
    }
}

static void update_simulation() {
    const bool tractionGear = (currentGear == GEAR_D || currentGear == GEAR_R);
    vehicleSpeedKmh = tractionGear ? (uint8_t)(((uint16_t)appliedPwm * 120U) / 255U) : 0;

    if (tractionGear && appliedPwm > 0 && simulatedSoC > 0.0f) {
        simulatedSoC -= 0.03f + ((float)appliedPwm / 255.0f) * 0.12f;
    } else if (simulatedSoC > 0.0f) {
        simulatedSoC -= 0.002f;
    }

    if (simulatedSoC < 0.0f) {
        simulatedSoC = 0.0f;
    }

    if (simulatedSoC < 15.0f) {
        activeDtcCode = 0x11;
    } else if (appliedPwm >= 250) {
        if (overloadStartMs == 0) {
            overloadStartMs = millis();
        }
        activeDtcCode = (millis() - overloadStartMs >= 3000) ? 0x22 : 0x00;
    } else {
        activeDtcCode = 0x00;
        overloadStartMs = 0;
    }
}

static void send_fast_telemetry(SoftwareSerial& serial_line) {
    uint8_t payload[2] = {pedalPercent, vehicleSpeedKmh};
    uart_send_frame(serial_line, MSG_PEDAL_SPEED, sizeof(payload), payload, &localTxCounter);
}

static void send_slow_telemetry(SoftwareSerial& serial_line) {
    uint8_t soc = (uint8_t)constrain((int)(simulatedSoC + 0.5f), 0, 100);
    uart_send_frame(serial_line, MSG_BATTERY_SOC, 1, &soc, &localTxCounter);
    uart_send_frame(serial_line, MSG_DTC_STATUS, 1, &activeDtcCode, &localTxCounter);
}

void vehicle_tick(SoftwareSerial& serial_line) {
    const uint32_t now = millis();

    if (now - lastPedalSampleMs >= 10) {
        lastPedalSampleMs = now;
        pedalPercent = read_pedal_percent();
    }

    debounce_buttons(serial_line);

    if (now - lastSimulationMs >= 100) {
        lastSimulationMs = now;
        update_simulation();
    }

    if (now - lastTelemetryMs >= 50) {
        lastTelemetryMs = now;
        send_fast_telemetry(serial_line);
    }

    if (now - lastSlowTelemetryMs >= 1000) {
        lastSlowTelemetryMs = now;
        send_gear(serial_line);
        send_slow_telemetry(serial_line);
    }
}
