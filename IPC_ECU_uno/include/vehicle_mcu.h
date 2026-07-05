#ifndef VEHICLE_MCU_H
#define VEHICLE_MCU_H

#include <Arduino.h>
#include <SoftwareSerial.h>

const int POT_PIN = A0;      
const int ENA_PIN = 5;       
const int IN1_PIN = 7;       
const int IN2_PIN = 8;       
const int BTN_UP_PIN = 9;    
const int BTN_DOWN_PIN = 10; 

extern float simulatedSoC;
extern uint8_t currentGear;
extern uint8_t vehicleSpeedKmh;
extern uint8_t activeDtcCode;

void vehicle_hardware_init();
void update_battery_and_dtc_simulation();
void check_gear_shift_buttons(SoftwareSerial* serial_line);
void broadcast_periodic_telemetry(SoftwareSerial* serial_line);

#endif