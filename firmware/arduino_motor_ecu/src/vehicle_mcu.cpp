#include "vehicle_mcu.h"
#include "can_manager.h"

static const uint32_t BUTTON_DEBOUNCE_MS = 35;
static const uint32_t PEDAL_SAMPLE_MS = 10;
static const uint32_t VEHICLE_UPDATE_MS = 100;
static const uint32_t DRIVE_STATUS_MS = 50;
static const uint32_t SLOW_STATUS_MS = 1000;
static const uint32_t MOTOR_OVERLOAD_MS = 3000;

static float simulatedSoC = 100.0f;
static uint8_t currentGear = GEAR_P;
static uint8_t vehicleSpeedKmh = 0;
static uint8_t activeDtcCode = 0x00;
static uint8_t pedalPercent = 0;

static uint8_t driveCounter = 0;
static uint8_t energyCounter = 0;
static uint8_t diagCounter = 0;
static uint8_t appliedPwm = 0;
static uint32_t lastPedalSampleMs = 0;
static uint32_t lastTelemetryMs = 0;
static uint32_t lastSlowTelemetryMs = 0;
static uint32_t lastSimulationMs = 0;
static uint32_t overloadStartMs = 0;
static uint32_t lastMotorCommandMs = 0;
static bool motorCommandSeen = false;
static bool motorCommandTimedOut = false;

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

// L298 motor output

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

static void apply_motor_command(uint8_t authorized_pwm) {
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

void vehicle_receive_motor_command(uint8_t authorized_pwm) {
    const bool wasTimedOut = motorCommandTimedOut;
    motorCommandSeen = true;
    motorCommandTimedOut = false;
    lastMotorCommandMs = millis();
    apply_motor_command(authorized_pwm);

    if (wasTimedOut) {
        Serial.println(F("[ECU] VCU motor command restored."));
    }
}

static void service_motor_command_watchdog() {
    if (!motorCommandSeen || motorCommandTimedOut) {
        return;
    }

    const uint32_t now = millis();
    if (now - lastMotorCommandMs <= MOTOR_COMMAND_TIMEOUT_MS) {
        return;
    }

    motorCommandTimedOut = true;
    apply_motor_command(0);
    Serial.println(F("[ECU] FAIL-SAFE: VCU motor command timeout; motor stopped."));
}

static void send_drive_status(SoftwareSerial& serial_line) {
    CAN_Frame_t frame;
    if (can_pack_drive_status(pedalPercent, vehicleSpeedKmh, currentGear,
                              appliedPwm, driveCounter, &frame)) {
        uart_send_frame(serial_line, frame);
        driveCounter = (uint8_t)((driveCounter + 1) & 0x0F);
    }
}

// Gear-button input

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
    apply_motor_command(0);
    send_drive_status(serial_line);
}

static void debounce_buttons(SoftwareSerial& serial_line) {
    const uint32_t now = millis();
    const bool rawUp = digitalRead(BTN_UP_PIN);
    const bool rawDown = digitalRead(BTN_DOWN_PIN);

    if (rawUp != lastRawUp) {
        lastRawUp = rawUp;
        lastUpChangeMs = now;
    }
    if ((now - lastUpChangeMs) >= BUTTON_DEBOUNCE_MS && rawUp != lastStableUp) {
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
    if ((now - lastDownChangeMs) >= BUTTON_DEBOUNCE_MS && rawDown != lastStableDown) {
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

    if (motorCommandTimedOut) {
        activeDtcCode = 0xE2;
        overloadStartMs = 0;
    } else if (simulatedSoC < 15.0f) {
        activeDtcCode = 0x11;
    } else if (appliedPwm >= 250) {
        if (overloadStartMs == 0) {
            overloadStartMs = millis();
        }
        activeDtcCode = (millis() - overloadStartMs >= MOTOR_OVERLOAD_MS) ? 0x22 : 0x00;
    } else {
        activeDtcCode = 0x00;
        overloadStartMs = 0;
    }
}

// CAN telemetry sent to the Raspberry Pi

static void send_fast_telemetry(SoftwareSerial& serial_line) {
    send_drive_status(serial_line);
}

static void send_slow_telemetry(SoftwareSerial &serial_line) {
    // Serial.print(F("[ECU] Pot raw="));
    // Serial.print(analogRead(POT_PIN));
    // Serial.print(F(" | Pedal="));
    // Serial.print(pedalPercent);
    // Serial.println(F("%"));

    CAN_Frame_t frame;
    const uint8_t soc = (uint8_t)constrain((int)(simulatedSoC + 0.5f), 0, 100);
    if (can_pack_energy_status(soc, energyCounter, &frame)) {
        uart_send_frame(serial_line, frame);
        energyCounter = (uint8_t)((energyCounter + 1) & 0x0F);
    }

    uint8_t faultFlags = activeDtcCode == 0 ? 0 : DIAG_FAULT_PRESENT;
    if (activeDtcCode == 0x11) {
        faultFlags |= DIAG_LOW_SOC;
    } else if (activeDtcCode == 0x22) {
        faultFlags |= DIAG_MOTOR_OVERLOAD;
    } else if (activeDtcCode == 0xE2) {
        faultFlags |= DIAG_COMMAND_TIMEOUT;
    }
    if (can_pack_diag_status(activeDtcCode, faultFlags, diagCounter, &frame)) {
        uart_send_frame(serial_line, frame);
        diagCounter = (uint8_t)((diagCounter + 1) & 0x0F);
    }
}

void vehicle_tick(SoftwareSerial& serial_line) {
    const uint32_t now = millis();

    if (now - lastPedalSampleMs >= PEDAL_SAMPLE_MS) {
        lastPedalSampleMs = now;
        pedalPercent = read_pedal_percent();
    }

    debounce_buttons(serial_line);
    service_motor_command_watchdog();

    if (now - lastSimulationMs >= VEHICLE_UPDATE_MS) {
        lastSimulationMs = now;
        update_simulation();
    }

    if (now - lastTelemetryMs >= DRIVE_STATUS_MS) {
        lastTelemetryMs = now;
        send_fast_telemetry(serial_line);
    }

    if (now - lastSlowTelemetryMs >= SLOW_STATUS_MS) {
        lastSlowTelemetryMs = now;
        send_slow_telemetry(serial_line);
    }
}

uint8_t vehicle_current_gear() {
    return currentGear;
}
