#include <stdio.h>
#include "vcu_core.h"

void execute_vcu_powertrain_strategy(VCU_VehicleContext_t* ctx, CAN_Frame_t* rxFrame) {
    if (rxFrame->can_id == 0x0101) {
        ctx->pedal_input = rxFrame->data[0];
        ctx->vehicle_speed = rxFrame->data[1]; 
    } 
    else if (rxFrame->can_id == 0x0201) {
        ctx->battery_soc = rxFrame->data[0];
    } 
    else if (rxFrame->can_id == 0x0401) {
        if (ctx->pedal_input > 2) {
            printf("\033[1;31m[VCU REJECTION] Shifting blocked! Release acceleration pedal.\033[0m\n");
        } else {
            ctx->gear_position = rxFrame->data[0];
        }
    }
    else if (rxFrame->can_id == 0x0501) {
        ctx->active_dtc_fault = rxFrame->data[0]; 
    }

    if (ctx->active_dtc_fault > 0) {
        printf("\033[1;33m[VCU DIAGNOSTIC WARNING] Active DTC Alarm: ");
        if (ctx->active_dtc_fault == 0x11) printf("P0A80 - Battery Pack Low Voltage\033[0m\n");
        if (ctx->active_dtc_fault == 0x22) printf("P1A10 - Motor Current Overload\033[0m\n");
    }

    if (!ctx->is_ready_to_drive) {
        ctx->authorized_pwm = 0;
        printf("[VCU] IGNITION: OFF | SoC: %d%% | Gear: %c | Speed: %d km/h | PROPULSION LOCKED\n",
               ctx->battery_soc, 
               (ctx->gear_position==0)?'P':(ctx->gear_position==1)?'R':(ctx->gear_position==2)?'N':'D',
               ctx->vehicle_speed);
    } 
    else if (ctx->battery_soc == 0) {
        ctx->authorized_pwm = 0;
        printf("\033[1;31m[VCU EMERGENCY] VEHICLE INOPERABLE: BATTERY DEPLETED!\033[0m\n");
    }
    else if (ctx->gear_position == 0 || ctx->gear_position == 2) {
        ctx->authorized_pwm = 0;
        printf("[VCU] IGNITION: READY | SoC: %d%% | Gear: %c | Speed: %d km/h | TRACTION ISOLATED\n",
               ctx->battery_soc, (ctx->gear_position==0)?'P':'N', ctx->vehicle_speed);
    }
    else {
        int base_pwm = (ctx->pedal_input * 255) / 100;

        if (ctx->drive_mode == MODE_ECO) {
            ctx->authorized_pwm = (uint8_t)(base_pwm * 0.5);
        } 
        else if (ctx->drive_mode == MODE_NORMAL) {
            ctx->authorized_pwm = (uint8_t)base_pwm;
        } 
        else if (ctx->drive_mode == MODE_SPORT) {
            if (ctx->pedal_input > 0 && ctx->pedal_input < 30) {
                ctx->authorized_pwm = (uint8_t)(base_pwm * 1.5);
            } else {
                ctx->authorized_pwm = (uint8_t)base_pwm;
            }
            if (ctx->authorized_pwm > 255) ctx->authorized_pwm = 255;
        }

        if (ctx->gear_position == 1 && ctx->authorized_pwm > 100) {
            ctx->authorized_pwm = 100;
        }

        printf("[VCU] PROFILE: %s | SoC: %d%% | Gear: %c | Speed: %d km/h | Target PWM -> %d\n",
               (ctx->drive_mode == MODE_ECO) ? "ECO" : (ctx->drive_mode == MODE_NORMAL) ? "NORMAL" : "SPORT",
               ctx->battery_soc, (ctx->gear_position==1)?'R':'D', ctx->pedal_input, ctx->authorized_pwm);
    }
}