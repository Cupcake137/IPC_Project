#ifndef VCU_CORE_H
#define VCU_CORE_H

#include <stdint.h>
#include "can_manager.h"

typedef enum { MODE_ECO, MODE_NORMAL, MODE_SPORT } DriveMode_t;

typedef struct {
    int is_ready_to_drive;
    DriveMode_t drive_mode;
    uint8_t pedal_input;
    uint8_t battery_soc;
    uint8_t gear_position;
    uint8_t vehicle_speed;    
    uint8_t active_dtc_fault; 
    uint8_t authorized_pwm;
} VCU_VehicleContext_t;

void execute_vcu_powertrain_strategy(VCU_VehicleContext_t* ctx, CAN_Frame_t* rxFrame);

#endif