#ifndef VCU_CORE_H
#define VCU_CORE_H

#include <stdint.h>
#include "can_manager.h"

typedef enum {
    MODE_ECO,
    MODE_NORMAL,
    MODE_SPORT,
} DriveMode_t;

typedef enum {
    VEHICLE_OFF,
    VEHICLE_ACC,
    VEHICLE_READY,
    VEHICLE_CHARGING,
    VEHICLE_FAULT,
} VehicleState_t;

typedef enum {
    GEAR_P = 0,
    GEAR_R = 1,
    GEAR_N = 2,
    GEAR_D = 3,
} GearPosition_t;

typedef struct {
    VehicleState_t state;
    DriveMode_t drive_mode;
    uint8_t pedal_input;
    uint8_t battery_soc;
    uint8_t gear_position;
    uint8_t vehicle_speed;
    uint8_t active_dtc_fault;
    uint8_t authorized_pwm;
} VCU_VehicleContext_t;

void vcu_init(VCU_VehicleContext_t* ctx);
void vcu_process_frame(VCU_VehicleContext_t* ctx, const CAN_Frame_t* rxFrame);
uint8_t vcu_calculate_motor_pwm(VCU_VehicleContext_t* ctx);
void vcu_print_status(const VCU_VehicleContext_t* ctx);

#endif
