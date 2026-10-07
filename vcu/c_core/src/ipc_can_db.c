#include <string.h>
#include "ipc_can_db.h"

#define CRC8_POLYNOMIAL 0x1DU
#define CRC8_INITIAL 0xFFU
#define CRC8_XOR_OUT 0xFFU

static uint8_t crc8_update(uint8_t crc, uint8_t value) {
    crc ^= value;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        crc = (crc & 0x80U) ? (uint8_t)((crc << 1U) ^ CRC8_POLYNOMIAL)
                            : (uint8_t)(crc << 1U);
    }
    return crc;
}

static void frame_init(CAN_Frame_t* frame, uint16_t id, uint8_t dlc) {
    memset(frame, 0, sizeof(*frame));
    frame->can_id = id;
    frame->can_dlc = dlc;
}

uint8_t ipc_can_crc8(const CAN_Frame_t* frame) {
    if (frame == NULL || frame->can_id > IPC_CAN_STANDARD_ID_MAX
        || frame->can_dlc == 0 || frame->can_dlc > IPC_CAN_MAX_DLC) {
        return 0;
    }

    uint8_t crc = CRC8_INITIAL;
    crc = crc8_update(crc, (uint8_t)(frame->can_id >> 8U));
    crc = crc8_update(crc, (uint8_t)(frame->can_id & 0xFFU));
    crc = crc8_update(crc, frame->can_dlc);
    for (uint8_t index = 0; index + 1U < frame->can_dlc; ++index) {
        crc = crc8_update(crc, frame->data[index]);
    }
    return (uint8_t)(crc ^ CRC8_XOR_OUT);
}

bool ipc_can_finalize(CAN_Frame_t* frame) {
    if (frame == NULL || frame->can_id > IPC_CAN_STANDARD_ID_MAX
        || frame->can_dlc == 0 || frame->can_dlc > IPC_CAN_MAX_DLC) {
        return false;
    }
    frame->data[frame->can_dlc - 1U] = ipc_can_crc8(frame);
    return true;
}

bool ipc_can_validate(const CAN_Frame_t* frame, uint16_t expected_id, uint8_t expected_dlc) {
    return frame != NULL
        && frame->can_id == expected_id
        && frame->can_id <= IPC_CAN_STANDARD_ID_MAX
        && frame->can_dlc == expected_dlc
        && frame->can_dlc > 0
        && frame->data[frame->can_dlc - 1U] == ipc_can_crc8(frame);
}

void ipc_can_trace(FILE* stream, const char* channel, const CAN_Frame_t* frame) {
    if (stream == NULL || frame == NULL) {
        return;
    }
    fprintf(stream, "%-8s %03X#", channel != NULL ? channel : "CAN",
            (unsigned int)frame->can_id);
    for (uint8_t index = 0; index < frame->can_dlc; ++index) {
        fprintf(stream, "%02X", frame->data[index]);
    }
    fprintf(stream, "\n");
}

bool ipc_can_pack_ecu_drive_status(CAN_Frame_t* frame, const EcuDriveStatus_t* status) {
    if (frame == NULL || status == NULL || status->pedal_percent > 100U
        || status->gear > IPC_GEAR_D) {
        return false;
    }
    frame_init(frame, CAN_ID_ECU_DRIVE_STATUS, 6);
    frame->data[0] = status->pedal_percent;
    frame->data[1] = status->speed_kmh;
    frame->data[2] = status->gear;
    frame->data[3] = status->applied_pwm;
    frame->data[4] = (uint8_t)(status->alive_counter & 0x0FU);
    return ipc_can_finalize(frame);
}

bool ipc_can_unpack_ecu_drive_status(const CAN_Frame_t* frame, EcuDriveStatus_t* status) {
    if (status == NULL || !ipc_can_validate(frame, CAN_ID_ECU_DRIVE_STATUS, 6)
        || frame->data[0] > 100U || (frame->data[2] & 0x03U) > IPC_GEAR_D) {
        return false;
    }
    status->pedal_percent = frame->data[0];
    status->speed_kmh = frame->data[1];
    status->gear = frame->data[2];
    status->applied_pwm = frame->data[3];
    status->alive_counter = (uint8_t)(frame->data[4] & 0x0FU);
    return true;
}

bool ipc_can_pack_ecu_energy_status(CAN_Frame_t* frame, const EcuEnergyStatus_t* status) {
    if (frame == NULL || status == NULL || status->battery_soc > 100U) {
        return false;
    }
    frame_init(frame, CAN_ID_ECU_ENERGY_STATUS, 3);
    frame->data[0] = status->battery_soc;
    frame->data[1] = (uint8_t)(status->alive_counter & 0x0FU);
    return ipc_can_finalize(frame);
}

bool ipc_can_unpack_ecu_energy_status(const CAN_Frame_t* frame, EcuEnergyStatus_t* status) {
    if (status == NULL || !ipc_can_validate(frame, CAN_ID_ECU_ENERGY_STATUS, 3)
        || frame->data[0] > 100U) {
        return false;
    }
    status->battery_soc = frame->data[0];
    status->alive_counter = (uint8_t)(frame->data[1] & 0x0FU);
    return true;
}

bool ipc_can_pack_ecu_diag_status(CAN_Frame_t* frame, const EcuDiagStatus_t* status) {
    if (frame == NULL || status == NULL) {
        return false;
    }
    frame_init(frame, CAN_ID_ECU_DIAG_STATUS, 4);
    frame->data[0] = status->active_dtc;
    frame->data[1] = (uint8_t)(status->fault_flags & 0x0FU);
    frame->data[2] = (uint8_t)(status->alive_counter & 0x0FU);
    return ipc_can_finalize(frame);
}

bool ipc_can_unpack_ecu_diag_status(const CAN_Frame_t* frame, EcuDiagStatus_t* status) {
    if (status == NULL || !ipc_can_validate(frame, CAN_ID_ECU_DIAG_STATUS, 4)) {
        return false;
    }
    status->active_dtc = frame->data[0];
    status->fault_flags = (uint8_t)(frame->data[1] & 0x0FU);
    status->alive_counter = (uint8_t)(frame->data[2] & 0x0FU);
    return true;
}

bool ipc_can_pack_vcu_motor_command(CAN_Frame_t* frame, const VcuMotorCommand_t* command) {
    if (frame == NULL || command == NULL || command->expected_gear > IPC_GEAR_D) {
        return false;
    }
    frame_init(frame, CAN_ID_VCU_MOTOR_COMMAND, 4);
    frame->data[0] = command->authorized_pwm;
    frame->data[1] = (uint8_t)((command->expected_gear & 0x03U)
        | (command->torque_enable ? 0x04U : 0U));
    frame->data[2] = (uint8_t)(command->alive_counter & 0x0FU);
    return ipc_can_finalize(frame);
}

bool ipc_can_unpack_vcu_motor_command(const CAN_Frame_t* frame, VcuMotorCommand_t* command) {
    if (command == NULL || !ipc_can_validate(frame, CAN_ID_VCU_MOTOR_COMMAND, 4)
        || (frame->data[1] & 0x03U) > IPC_GEAR_D) {
        return false;
    }
    command->authorized_pwm = frame->data[0];
    command->expected_gear = (uint8_t)(frame->data[1] & 0x03U);
    command->torque_enable = (frame->data[1] & 0x04U) != 0;
    command->alive_counter = (uint8_t)(frame->data[2] & 0x0FU);
    return true;
}
