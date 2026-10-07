#include <stdio.h>
#include "vcu_core.h"

#define REVERSE_MAX_PWM 43U

static char gear_to_char(uint8_t gear) {
    switch (gear) {
        case GEAR_R: return 'R';
        case GEAR_N: return 'N';
        case GEAR_D: return 'D';
        default: return 'P';
    }
}

static const char* drive_mode_text(DriveMode_t mode) {
    switch (mode) {
        case MODE_ECO: return "ECO";
        case MODE_SPORT: return "SPORT";
        default: return "NORMAL";
    }
}

static const char* state_text(VehicleState_t state) {
    switch (state) {
        case VEHICLE_OFF: return "OFF";
        case VEHICLE_ACC: return "ACC";
        case VEHICLE_READY: return "READY";
        case VEHICLE_CHARGING: return "CHARGING";
        case VEHICLE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

void vcu_init(VCU_VehicleContext_t* ctx) {
    ctx->state = VEHICLE_READY;
    ctx->drive_mode = MODE_NORMAL;
    ctx->pedal_input = 0;
    ctx->battery_soc = 100;
    ctx->gear_position = GEAR_P;
    ctx->vehicle_speed = 0;
    ctx->active_dtc_fault = 0;
    ctx->authorized_pwm = 0;
    ctx->ecu_applied_pwm = 0;
    ctx->last_drive_counter = 0;
    ctx->last_energy_counter = 0;
    ctx->last_diag_counter = 0;
    ctx->drive_counter_seen = false;
    ctx->energy_counter_seen = false;
    ctx->diag_counter_seen = false;
}

static bool counter_is_fresh(uint8_t counter, uint8_t* previous, bool* seen) {
    counter &= 0x0FU;
    if (*seen && counter == *previous) {
        return false;
    }
    *previous = counter;
    *seen = true;
    return true;
}

bool vcu_process_frame(VCU_VehicleContext_t* ctx, const CAN_Frame_t* rx_frame) {
    if (ctx == NULL || rx_frame == NULL) {
        return false;
    }

    switch (rx_frame->can_id) {
        case CAN_ID_ECU_DRIVE_STATUS: {
            EcuDriveStatus_t status;
            if (!ipc_can_unpack_ecu_drive_status(rx_frame, &status)
                || !counter_is_fresh(status.alive_counter, &ctx->last_drive_counter,
                                     &ctx->drive_counter_seen)) {
                return false;
            }
            ctx->pedal_input = status.pedal_percent;
            ctx->vehicle_speed = status.speed_kmh;
            ctx->ecu_applied_pwm = status.applied_pwm;
            if (status.gear != ctx->gear_position && ctx->pedal_input > 2U) {
                printf("[VCU] Reject gear update while pedal is pressed.\n");
            } else {
                ctx->gear_position = status.gear;
            }
            break;
        }

        case CAN_ID_ECU_ENERGY_STATUS: {
            EcuEnergyStatus_t status;
            if (!ipc_can_unpack_ecu_energy_status(rx_frame, &status)
                || !counter_is_fresh(status.alive_counter, &ctx->last_energy_counter,
                                     &ctx->energy_counter_seen)) {
                return false;
            }
            ctx->battery_soc = status.battery_soc;
            break;
        }

        case CAN_ID_ECU_DIAG_STATUS: {
            EcuDiagStatus_t status;
            if (!ipc_can_unpack_ecu_diag_status(rx_frame, &status)
                || !counter_is_fresh(status.alive_counter, &ctx->last_diag_counter,
                                     &ctx->diag_counter_seen)) {
                return false;
            }
            ctx->active_dtc_fault = status.active_dtc;
            break;
        }

        default:
            return false;
    }

    if ((ctx->active_dtc_fault != 0 && ctx->active_dtc_fault != 0x11)
        || ctx->battery_soc == 0) {
        ctx->state = VEHICLE_FAULT;
    } else if (ctx->state == VEHICLE_FAULT) {
        ctx->state = VEHICLE_READY;
    }
    return true;
}

uint8_t vcu_calculate_motor_pwm(VCU_VehicleContext_t* ctx) {
    uint16_t pwm = 0;

    if (ctx->state != VEHICLE_READY || ctx->battery_soc == 0) {
        ctx->authorized_pwm = 0;
        return 0;
    }

    if (ctx->gear_position != GEAR_D && ctx->gear_position != GEAR_R) {
        ctx->authorized_pwm = 0;
        return 0;
    }

    pwm = ((uint16_t)ctx->pedal_input * 255U) / 100U;

    if (ctx->drive_mode == MODE_ECO) {
        pwm = (pwm * 60U) / 100U;
    } else if (ctx->drive_mode == MODE_SPORT && ctx->pedal_input > 0 && ctx->pedal_input < 30) {
        pwm = (pwm * 140U) / 100U;
    }

    if (ctx->gear_position == GEAR_R && pwm > REVERSE_MAX_PWM) {
        pwm = REVERSE_MAX_PWM;
    }
    if (ctx->active_dtc_fault != 0 && pwm > 80U) {
        pwm = 80U;
    }
    if (pwm > 255U) {
        pwm = 255U;
    }

    ctx->authorized_pwm = (uint8_t)pwm;
    return ctx->authorized_pwm;
}

bool vcu_build_motor_command(const VCU_VehicleContext_t* ctx, uint8_t alive_counter,
                             CAN_Frame_t* frame) {
    if (ctx == NULL || frame == NULL) {
        return false;
    }
    VcuMotorCommand_t command = {
        .authorized_pwm = ctx->authorized_pwm,
        .expected_gear = ctx->gear_position,
        .torque_enable = ctx->state == VEHICLE_READY
            && (ctx->gear_position == GEAR_D || ctx->gear_position == GEAR_R)
            && (ctx->active_dtc_fault == 0 || ctx->active_dtc_fault == 0x11),
        .alive_counter = alive_counter,
    };
    return ipc_can_pack_vcu_motor_command(frame, &command);
}

void vcu_print_status(const VCU_VehicleContext_t* ctx) {
    printf("[VCU] State:%s Mode:%s SoC:%u%% Gear:%c Speed:%u km/h Pedal:%u%% PWM:%u",
           state_text(ctx->state),
           drive_mode_text(ctx->drive_mode),
           ctx->battery_soc,
           gear_to_char(ctx->gear_position),
           ctx->vehicle_speed,
           ctx->pedal_input,
           ctx->authorized_pwm);

    if (ctx->active_dtc_fault == 0x11) {
        printf(" DTC:P0A80 Battery low voltage");
    } else if (ctx->active_dtc_fault == 0x22) {
        printf(" DTC:P1A10 Motor overload");
    } else if (ctx->active_dtc_fault != 0) {
        printf(" DTC:0x%02X", ctx->active_dtc_fault);
    }
    printf("\n");
}
