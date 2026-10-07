#ifndef VEHICLE_MCU_H
#define VEHICLE_MCU_H

#include <Arduino.h>
#include <SoftwareSerial.h>

static const uint32_t ECU_UART_BAUD = 9600;
static const uint32_t MOTOR_COMMAND_TIMEOUT_MS = 350;

static const uint8_t PI_RX_PIN = 2;
static const uint8_t PI_TX_PIN = 3;
static const uint8_t POT_PIN = A0;
static const uint8_t ENA_PIN = 5;
static const uint8_t IN1_PIN = 7;
static const uint8_t IN2_PIN = 8;
static const uint8_t BTN_UP_PIN = 9;
static const uint8_t BTN_DOWN_PIN = 10;

enum GearPosition : uint8_t {
    GEAR_P = 0,
    GEAR_R = 1,
    GEAR_N = 2,
    GEAR_D = 3,
};

void vehicle_hardware_init();
void vehicle_tick(SoftwareSerial& serial_line);
void vehicle_receive_motor_command(uint8_t authorized_pwm);
uint8_t vehicle_current_gear();

#endif
