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
}

void vcu_process_frame(VCU_VehicleContext_t* ctx, const CAN_Frame_t* rxFrame) {
    switch (rxFrame->id) {
        case MSG_PEDAL_SPEED:
            if (rxFrame->dlc >= 2) {
                ctx->pedal_input = rxFrame->data[0];
                ctx->vehicle_speed = rxFrame->data[1];
            }
            break;

        case MSG_BATTERY_SOC:
            if (rxFrame->dlc >= 1) {
                ctx->battery_soc = rxFrame->data[0];
            }
            break;

        case MSG_GEAR_STATE:
            if (rxFrame->dlc >= 1 && rxFrame->data[0] <= GEAR_D) {
                if (ctx->pedal_input > 2) {
                    printf("[VCU] Reject gear update while pedal is pressed.\n");
                } else {
                    ctx->gear_position = rxFrame->data[0];
                }
            }
            break;

        case MSG_DTC_STATUS:
            if (rxFrame->dlc >= 1) {
                ctx->active_dtc_fault = rxFrame->data[0];
            }
            break;

        default:
            break;
    }

    if (ctx->active_dtc_fault != 0 || ctx->battery_soc == 0) {
        ctx->state = VEHICLE_FAULT;
    } else if (ctx->state == VEHICLE_FAULT) {
        ctx->state = VEHICLE_READY;
    }
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
